#include "GameManager.hpp"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iostream>
#include "MatchingEngine.hpp"
#include "SimState.hpp"

GameManager::GameManager() : riskEngine_(42) {
    initStocks();
}

GameManager::~GameManager() { stop(); }

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

void GameManager::selectMode(AppMode mode) {
    stopFlag_.store(true);
    if (simThread_.joinable())
        simThread_.join();
    stopFlag_.store(false);

    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        state_.appMode         = mode;
        state_.processedOrders = 0;
        state_.recentFills.clear();
        state_.histogram.clear();
        state_.totalFills      = 0;
        state_.fillRate        = 0.0;
        state_.medianLatNs     = 0.0;
        state_.p99LatNs        = 0.0;
        state_.minLatNs        = 0.0;
        state_.maxLatNs        = 0.0;
        state_.meanLatNs       = 0.0;
        state_.spread          = 0.0;
        state_.bids.clear();
        state_.asks.clear();

        if (mode == AppMode::Benchmark)
            state_.phase = GamePhase::Benchmark;
        else
            state_.phase = GamePhase::StockPicker;
    }

    // Start thread first, then signal it
    simThread_ = std::thread(&GameManager::simLoop, this);

    // Give thread time to enter its wait loop before firing the flag
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    state_.startRequested.store(true);
}

void GameManager::selectStock(int idx) {
    std::lock_guard<std::mutex> lock(state_.mtx);
    state_.selectedStockIdx = idx;
    state_.player.reset(state_.stocks[idx].profile.ticker);
    state_.phase = GamePhase::TradingReady;  // wait for Start
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
    // Full reset — go back to main menu
    std::lock_guard<std::mutex> lock(state_.mtx);
    state_.selectedStockIdx = -1;
    state_.phase = GamePhase::MainMenu;
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
    // Wait for mode selection
    while (!stopFlag_.load()) {
        if (state_.startRequested.load()) {
            state_.startRequested.store(false);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    if (stopFlag_.load()) return;

    // Route to correct loop
    AppMode mode;
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        mode = state_.appMode;
    }

    if (mode == AppMode::Benchmark)
        benchmarkLoop();
    else
        gameLoop();
}

// ── Benchmark loop — max speed, original behavior ─────────────────────────────
void GameManager::benchmarkLoop() {
    std::mt19937_64 rng(42);
    std::vector<double> latSamples;
    latSamples.reserve(1000000);

    GeneratorConfig gcfg;
    OrderGenerator  generator(gcfg);
    
// In benchmarkLoop, inside the MatchingEngine lambda:

MatchingEngine engine([&](const Fill& fill) {
    auto now = std::chrono::high_resolution_clock::now();
    double latNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now - fill.timestamp).count();
    if (latNs > 0 && latNs < 1e9)
        latSamples.push_back(latNs);
});
    uint64_t totalOrders;
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        totalOrders = state_.benchmarkOrders;
    }

    state_.simRunning.store(true);
    auto wallStart = std::chrono::steady_clock::now();

    for (uint64_t i = 0; i < totalOrders && !stopFlag_.load(); ++i) {

        if (state_.resetRequested.load()) {
            state_.resetRequested.store(false);
            i = 0;
            latSamples.clear();
            wallStart = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.processedOrders = 0;
            state_.recentFills.clear();
            continue;
        }

        if (state_.paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            --i;
            continue;
        }

        if (i % 100 == 0) {
            auto [bid, ask] = generator.nextMarketMakerQuote();
            engine.submitOrder(bid);
            engine.submitOrder(ask);
        }

        Order* order = generator.nextRandom();
        engine.submitOrder(order);

        // Every 200 orders — throughput and book snapshot ONLY
        // No sorting, no histogram — keeps hot path fast
        if (i % 5000 == 0) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(
                now - wallStart).count();

            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.processedOrders = i;
            state_.totalFills      = engine.totalFills();
            state_.fillRate        = (double)engine.totalFills() / (i + 1);
            state_.ordersPerSec    = elapsed > 0 ? i / elapsed : 0;

            // Book snapshot so GUI updates during run
            state_.bids.clear();
            state_.asks.clear();
            uint32_t total = 0; int count = 0;
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
    }

    // ── Single sort after loop completes — no overhead during run ─────────────
    if (latSamples.size() > 10) {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto sorted = latSamples;
        std::sort(sorted.begin(), sorted.end());
        state_.medianLatNs = sorted[sorted.size() / 2];
        state_.p99LatNs    = sorted[(std::size_t)(sorted.size() * 0.99)];
        state_.minLatNs    = sorted.front();
        state_.maxLatNs    = sorted.back();
        double sum = 0;
        for (double v : sorted) sum += v;
        state_.meanLatNs = sum / sorted.size();

        state_.histogram.clear();
        std::vector<double> edges = {
            100,300,600,1000,2000,5000,10000,
            50000,100000,500000,1000000,5000000,200000000
        };
        for (std::size_t e = 0; e + 1 < edges.size(); ++e) {
            uint64_t count = 0;
            for (double s : latSamples)
                if (s >= edges[e] && s < edges[e+1]) ++count;
            state_.histogram.push_back({ edges[e], edges[e+1], count });
        }
    }

    // ── Final state update ────────────────────────────────────────────────────
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        state_.processedOrders = state_.benchmarkOrders;
    }

    state_.simRunning.store(false);
}
uint64_t fillCount = 0;
// ── Game loop — throttled, player interactive ─────────────────────────────────
void GameManager::gameLoop() {
    // Wait for stock selection and player to press Start
    while (!stopFlag_.load()) {
        bool ready = false;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            ready = (state_.phase == GamePhase::Trading);
        }
        if (ready) break;

        if (state_.resetRequested.load()) {
            state_.resetRequested.store(false);
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    if (stopFlag_.load()) return;

    std::mt19937_64 rng(42);
    std::vector<double> latSamples;

    int idx;
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        idx = state_.selectedStockIdx;
    }

    StockState& activeStock = state_.stocks[idx];
    state_.simRunning.store(true);

    GeneratorConfig gcfg = activeStock.profile.toGeneratorConfig(
        activeStock.currentPrice, activeStock.currentRegime);
    OrderGenerator generator(gcfg);

uint64_t fillCount = 0;
MatchingEngine engine([&](const Fill& fill) {
    auto now = std::chrono::steady_clock::now();
    double latNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch() -
        fill.timestamp.time_since_epoch()).count();
    if (latNs > 0 && latNs < 1e9) {
        latSamples.push_back(latNs);
        if (++fillCount % 10 == 0) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.pushFill(
                fill.takerOrderId % 2 == 0,
                fill.price,
                fill.quantity,
                latNs / 1000.0
            );
        }
    }
});

    uint64_t totalOrders;
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        totalOrders = state_.totalOrders;
    }

    auto wallStart = std::chrono::steady_clock::now();

    for (uint64_t i = 0; i < totalOrders && !stopFlag_.load(); ) {

        // Reset — go back to main menu
        if (state_.resetRequested.load()) {
            state_.resetRequested.store(false);
            state_.simRunning.store(false);
            // Restart sim loop from top to wait for new mode selection
            simLoop();
            return;
        }

        // Pause — wait here
        if (state_.paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        // Session over
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.player.sessionOver) {
                state_.phase = GamePhase::SessionEnd;
                break;
            }
        }

        // Speed throttle — 1x = observable, MAX = fast
        if (i % 100 == 0) {
            int setting = state_.speedSetting.load();
            // ms delay per 100 orders at each speed setting
            static const int delays[] = { 40, 20, 8, 4, 1 };
            int delayMs = delays[std::clamp(setting, 0, 4)];
            std::this_thread::sleep_for(
                std::chrono::milliseconds(delayMs));
}

        // Regime check
        ++activeStock.ordersSinceRegimeCheck;
        if (activeStock.ordersSinceRegimeCheck >=
            static_cast<uint64_t>(activeStock.profile.regimeCheckEvery))
        {
            activeStock.ordersSinceRegimeCheck = 0;

            std::unordered_map<std::string, double> prices;
            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                for (auto& s : state_.stocks)
                    prices[s.profile.ticker] = s.currentPrice;
            }

            std::vector<StockProfile> profiles;
            for (auto& s : state_.stocks)
                profiles.push_back(s.profile);

            MarketModifiers mods = riskEngine_.tick(
                profiles, prices, activeStock.profile.ticker);

            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                for (auto& s : state_.stocks) {
                    auto it = prices.find(s.profile.ticker);
                    if (it != prices.end())
                        s.currentPrice = std::max(0.01, it->second);
                }
                if (std::abs(mods.priceDelta) > 0.001)
                    activeStock.currentPrice = std::max(0.01,
                        activeStock.currentPrice + mods.priceDelta);

                activeStock.volMultiplier    = mods.volMultiplier;
                activeStock.spreadMultiplier = mods.spreadMultiplier;

                state_.visibleBanners.clear();
                for (const auto& b : riskEngine_.banners())
                    if (b.isVisible())
                        state_.visibleBanners.push_back(b);
            }

            updateRegime(activeStock, rng);

            gcfg = activeStock.profile.toGeneratorConfig(
                activeStock.currentPrice,
                activeStock.currentRegime,
                activeStock.volMultiplier,
                activeStock.spreadMultiplier);
            generator = OrderGenerator(gcfg);
        }

        if (i % 100 == 0) {
            auto [bid, ask] = generator.nextMarketMakerQuote();
            engine.submitOrder(bid);
            engine.submitOrder(ask);
        }

        Order* order = generator.nextRandom();
        engine.submitOrder(order);

        updatePrice(activeStock, rng,
            { activeStock.volMultiplier,
              activeStock.spreadMultiplier, 0.0 });

        ++i;

        processPlayerActions(state_.player, activeStock.currentPrice);

        if (i % 150 == 0) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(
                now - wallStart).count();

            std::lock_guard<std::mutex> lock(state_.mtx);

            state_.processedOrders = i;
            state_.ordersPerSec    = elapsed > 0 ? i / elapsed : 0;

            if (latSamples.size() > 10) {
                auto sorted = latSamples;
                std::sort(sorted.begin(), sorted.end());
                state_.medianLatNs = sorted[sorted.size() / 2];
                state_.p99LatNs    =
                    sorted[(std::size_t)(sorted.size() * 0.99)];
            }

            state_.player.update(activeStock.currentPrice);
            state_.player.checkWinCondition(i, totalOrders);

            activeStock.pushPrice(activeStock.currentPrice);
            snapshotBook(engine);
        }
    }

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

void GameManager::updateRegime(StockState& s, std::mt19937_64& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double roll = dist(rng);
    const RegimeWeights& w = s.profile.regimeWeights;
    double cum = 0.0;

    if (roll < (cum += w.toMomentum))
        s.currentRegime = Regime::Momentum;
    else if (roll < (cum += w.toMeanReversion))
        s.currentRegime = Regime::MeanReversion;
    else if (roll < (cum += w.toVolatile))
        s.currentRegime = Regime::Volatile;
    else
        s.currentRegime = Regime::Quiet;
}

void GameManager::updatePrice(StockState& s,
                               std::mt19937_64& rng,
                               const MarketModifiers& mods)
{
    std::normal_distribution<double> noise(0.0, 1.0);
    double vol   = s.profile.baseVolatility * mods.volMultiplier;
    double shock = noise(rng) * vol;

    s.momentum = s.momentum * s.profile.momentumDecay
               + shock * s.profile.momentumStrength;

    double reversion = (s.profile.meanReversionTarget - s.currentPrice)
                     * s.profile.meanReversionSpeed;

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

void GameManager::snapshotBook(MatchingEngine& engine) {
    state_.bids.clear();
    state_.asks.clear();

    uint32_t total = 0; int count = 0;
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

void GameManager::processPlayerActions(PlayerState& player,
                                        double currentPrice)
{
    std::lock_guard<std::mutex> lock(state_.mtx);
    while (!state_.actionQueue.empty()) {
        auto action = state_.actionQueue.front();
        state_.actionQueue.pop_front();
        switch (action.type) {
            case GameState::PlayerAction::Type::Buy:
                player.buy(currentPrice, action.quantity);  break;
            case GameState::PlayerAction::Type::Sell:
                player.sell(currentPrice, action.quantity); break;
            case GameState::PlayerAction::Type::Flatten:
                player.flatten(currentPrice);               break;
        }
    }
}