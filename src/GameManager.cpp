// GameManager.cpp
#include "GameManager.hpp"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iostream>

GameManager::GameManager()
    : riskEngine_(42)
{
    initStocks();
}

GameManager::~GameManager() {
    stop();
}

// ── Initialize all six stock states ───────────────────────────────────────────
void GameManager::initStocks() {
    auto universe = buildStockUniverse();
    std::lock_guard<std::mutex> lock(state_.mtx);

    state_.stocks.clear();
    for (auto& profile : universe) {
        StockState s;
        s.profile       = profile;
        s.currentPrice  = profile.startPrice;
        s.currentRegime = profile.startRegime;
        s.pushPrice(profile.startPrice);
        state_.stocks.push_back(std::move(s));
    }
}

// ── Public control interface ──────────────────────────────────────────────────
void GameManager::start() {
    stopFlag_.store(false);
    simThread_ = std::thread(&GameManager::simLoop, this);
}

void GameManager::stop() {
    stopFlag_.store(true);
    state_.paused.store(false);
    if (simThread_.joinable())
        simThread_.join();
}

void GameManager::selectStock(int idx) {
    std::lock_guard<std::mutex> lock(state_.mtx);
    state_.selectedStockIdx = idx;
    state_.player.reset(state_.stocks[idx].profile.ticker);
    state_.phase = GamePhase::Trading;
    state_.startRequested.store(true);
}

void GameManager::requestBuy(uint32_t qty) {
    std::lock_guard<std::mutex> lock(state_.mtx);
    state_.actionQueue.push_back({
        GameState::PlayerAction::Type::Buy, qty });
}

void GameManager::requestSell(uint32_t qty) {
    std::lock_guard<std::mutex> lock(state_.mtx);
    state_.actionQueue.push_back({
        GameState::PlayerAction::Type::Sell, qty });
}

void GameManager::requestFlatten() {
    std::lock_guard<std::mutex> lock(state_.mtx);
    state_.actionQueue.push_back({
        GameState::PlayerAction::Type::Flatten, 0 });
}

void GameManager::requestReset() {
    state_.resetRequested.store(true);
}

void GameManager::submitToLeaderboard(const std::string& name) {
    std::lock_guard<std::mutex> lock(state_.mtx);
    state_.player.playerName = name;
    auto entry = state_.player.toLeaderboardEntry();
    state_.leaderboard.submit(entry);
    state_.phase = GamePhase::Leaderboard;
}

// ── Core simulation loop ──────────────────────────────────────────────────────
void GameManager::simLoop() {
    std::mt19937_64 rng(42);
    std::vector<double> latSamples;
    latSamples.reserve(10000);

    auto wallStart = std::chrono::steady_clock::now();

    // Wait for stock selection
    while (!stopFlag_.load()) {
        if (state_.startRequested.load()) {
            state_.startRequested.store(false);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    if (stopFlag_.load()) return;

    // ── Get selected stock ────────────────────────────────────────────────────
    int idx;
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        idx = state_.selectedStockIdx;
    }

    StockState& activeStock = state_.stocks[idx];
    state_.simRunning.store(true);

    // Build matching engine for this stock
    MatchingEngine engine([&](const Fill& fill) {
        auto now = std::chrono::steady_clock::now();
        double latNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch() - fill.timestamp.time_since_epoch()).count();
        if (latNs > 0 && latNs < 1e9)
            latSamples.push_back(latNs);
    });

    // Build generator from active stock's profile
    GeneratorConfig gcfg = activeStock.profile.toGeneratorConfig(
        activeStock.currentPrice,
        activeStock.currentRegime);
    OrderGenerator generator(gcfg);

    uint64_t totalOrders;
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        totalOrders = state_.totalOrders;
    }

    // ── Main loop ─────────────────────────────────────────────────────────────
    for (uint64_t i = 0; i < totalOrders && !stopFlag_.load(); ) {

        // Reset check
        if (state_.resetRequested.load()) {
            state_.resetRequested.store(false);
            i = 0;
            latSamples.clear();
            wallStart = std::chrono::steady_clock::now();
            riskEngine_.reset();

            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.player.reset(activeStock.profile.ticker);
            state_.processedOrders = 0;
            activeStock.currentPrice = activeStock.profile.startPrice;
            activeStock.currentRegime = activeStock.profile.startRegime;
            activeStock.momentum = 0.0;
            activeStock.priceHistory.clear();
            activeStock.pushPrice(activeStock.profile.startPrice);
            continue;
        }

        // Pause check
        if (state_.paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        // Session over check
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.player.sessionOver) {
                state_.phase = GamePhase::SessionEnd;
                break;
            }
        }

        // ── Regime check ──────────────────────────────────────────────────────
        ++activeStock.ordersSinceRegimeCheck;
        if (activeStock.ordersSinceRegimeCheck >=
            static_cast<uint64_t>(activeStock.profile.regimeCheckEvery))
        {
            activeStock.ordersSinceRegimeCheck = 0;

            // Get market modifiers from risk engine
            std::unordered_map<std::string, double> prices;
            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                for (auto& s : state_.stocks)
                    prices[s.profile.ticker] = s.currentPrice;
            }

            MarketModifiers mods = riskEngine_.tick(
                [&]{ std::vector<StockProfile> p;
                     for (auto& s : state_.stocks) p.push_back(s.profile);
                     return p; }(),
                prices,
                activeStock.profile.ticker);

            // Apply updated prices back
            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                for (auto& s : state_.stocks) {
                    auto it = prices.find(s.profile.ticker);
                    if (it != prices.end())
                        s.currentPrice = std::max(0.01, it->second);
                }

                // Apply one-time price delta to active stock
                if (std::abs(mods.priceDelta) > 0.001)
                    activeStock.currentPrice = std::max(0.01,
                        activeStock.currentPrice + mods.priceDelta);

                activeStock.volMultiplier    = mods.volMultiplier;
                activeStock.spreadMultiplier = mods.spreadMultiplier;

                // Update visible banners
                state_.visibleBanners.clear();
                for (const auto& b : riskEngine_.banners())
                    if (b.isVisible())
                        state_.visibleBanners.push_back(b);
            }

            // Regime transition
            updateRegime(activeStock, rng);

            // Rebuild generator with new config
            gcfg = activeStock.profile.toGeneratorConfig(
                activeStock.currentPrice,
                activeStock.currentRegime,
                activeStock.volMultiplier,
                activeStock.spreadMultiplier);
            generator = OrderGenerator(gcfg);
        }

        // ── Market maker refresh every 100 orders ─────────────────────────────
        if (i % 100 == 0) {
            auto [bid, ask] = generator.nextMarketMakerQuote();
            engine.submitOrder(bid);
            engine.submitOrder(ask);
        }

        // ── Random order ──────────────────────────────────────────────────────
        Order* order = generator.nextRandom();
        engine.submitOrder(order);

        // ── Price update from momentum and mean reversion ─────────────────────
        updatePrice(activeStock, rng, { activeStock.volMultiplier,
                                        activeStock.spreadMultiplier, 0.0 });

        ++i;

        // ── Process player actions ─────────────────────────────────────────────
        processPlayerActions(state_.player, activeStock.currentPrice);

        // ── Shared state update every 150 orders ──────────────────────────────
        if (i % 150 == 0) {
            auto now     = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(
                now - wallStart).count();

            std::lock_guard<std::mutex> lock(state_.mtx);

            state_.processedOrders = i;
            state_.ordersPerSec    = elapsed > 0 ? i / elapsed : 0;

            // Latency stats
            if (latSamples.size() > 10) {
                auto sorted = latSamples;
                std::sort(sorted.begin(), sorted.end());
                state_.medianLatNs = sorted[sorted.size() / 2];
                state_.p99LatNs    = sorted[(std::size_t)(sorted.size() * 0.99)];
            }

            // Player P&L update
            state_.player.update(activeStock.currentPrice);
            state_.player.checkWinCondition(i, totalOrders);

            // Price history
            activeStock.pushPrice(activeStock.currentPrice);

            // Book snapshot
            snapshotBook(engine);
        }
    }

    // ── Session complete ──────────────────────────────────────────────────────
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        if (!state_.player.sessionOver) {
            state_.player.sessionOver   = true;
            state_.player.sessionEndPnl = state_.player.totalPnl;
        }
        state_.phase = GamePhase::SessionEnd;
    }

    state_.simRunning.store(false);
}

// ── Regime transition — Markov chain ─────────────────────────────────────────
void GameManager::updateRegime(StockState& s, std::mt19937_64& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double roll = dist(rng);

    const RegimeWeights& w = s.profile.regimeWeights;
    double cumulative = 0.0;

    if (roll < (cumulative += w.toMomentum))
        s.currentRegime = Regime::Momentum;
    else if (roll < (cumulative += w.toMeanReversion))
        s.currentRegime = Regime::MeanReversion;
    else if (roll < (cumulative += w.toVolatile))
        s.currentRegime = Regime::Volatile;
    else
        s.currentRegime = Regime::Quiet;
}

// ── Price evolution — momentum + mean reversion ───────────────────────────────
void GameManager::updatePrice(StockState& s,
                               std::mt19937_64& rng,
                               const MarketModifiers& mods)
{
    std::normal_distribution<double> noise(0.0, 1.0);

    double vol   = s.profile.baseVolatility * mods.volMultiplier;
    double shock = noise(rng) * vol;

    // Momentum component — prior move amplifies next move
    s.momentum = s.momentum * s.profile.momentumDecay
               + shock * s.profile.momentumStrength;

    // Mean reversion pull
    double reversion = (s.profile.meanReversionTarget - s.currentPrice)
                     * s.profile.meanReversionSpeed;

    // Regime-specific drift modifier
    double driftMod = 1.0;
    switch (s.currentRegime) {
        case Regime::Momentum:      driftMod = 2.0;  break;
        case Regime::MeanReversion: driftMod = 0.0;  break;
        case Regime::Volatile:      driftMod = 1.0;  break;
        case Regime::Quiet:         driftMod = 0.3;  break;
    }

    double totalMove = shock
                     + s.momentum * driftMod
                     + reversion
                     + s.profile.baseDrift;

    s.currentPrice = std::max(0.01, s.currentPrice + totalMove);
}

// ── Book snapshot for GUI ─────────────────────────────────────────────────────
void GameManager::snapshotBook(MatchingEngine& engine) {
    state_.asks.clear();
    state_.bids.clear();

    uint32_t total = 0;
    int count = 0;
    for (auto& [price, level] : engine.book().asks()) {
        total += level.totalQty;
        state_.asks.push_back({ price, level.totalQty, total });
        if (++count >= 8) break;
    }

    total = 0; count = 0;
    for (auto& [price, level] : engine.book().bids()) {
        total += level.totalQty;
        state_.bids.push_back({ price, level.totalQty, total });
        if (++count >= 8) break;
    }

    auto bid = engine.book().bestBid();
    auto ask = engine.book().bestAsk();
    state_.spread = (bid && ask) ? *ask - *bid : 0.0;
}

// ── Process player action queue ───────────────────────────────────────────────
void GameManager::processPlayerActions(PlayerState& player,
                                        double currentPrice)
{
    std::lock_guard<std::mutex> lock(state_.mtx);

    while (!state_.actionQueue.empty()) {
        auto action = state_.actionQueue.front();
        state_.actionQueue.pop_front();

        switch (action.type) {
            case GameState::PlayerAction::Type::Buy:
                player.buy(currentPrice, action.quantity);
                break;
            case GameState::PlayerAction::Type::Sell:
                player.sell(currentPrice, action.quantity);
                break;
            case GameState::PlayerAction::Type::Flatten:
                player.flatten(currentPrice);
                break;
        }
    }
}