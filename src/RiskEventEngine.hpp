// RiskEventEngine.hpp
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <random>
#include <chrono>
#include <algorithm>
#include "StockProfile.hpp"

// ── A live event currently affecting the market ───────────────────────────────
struct ActiveEvent {
    RiskEvent   event;
    std::string ticker;         // which stock it fired on
    int         ordersRemaining;// how many orders until effect expires
    bool        isExpired() const { return ordersRemaining <= 0; }
};

// ── A news banner entry shown in the GUI ──────────────────────────────────────
struct NewsBanner {
    std::string headline;
    std::string ticker;
    bool        isPositive;     // green or red color in GUI
    std::chrono::steady_clock::time_point firedAt;

    // How long the banner stays visible in seconds
    bool isVisible() const {
        auto age = std::chrono::steady_clock::now() - firedAt;
        return std::chrono::duration_cast<std::chrono::seconds>(age).count() < 8;
    }
};

// ── Current market modifiers from all active events ───────────────────────────
struct MarketModifiers {
    double volMultiplier    = 1.0;
    double spreadMultiplier = 1.0;
    double priceDelta       = 0.0; // one-time price shock — consumed on next tick

    void reset() {
        volMultiplier    = 1.0;
        spreadMultiplier = 1.0;
        priceDelta       = 0.0;
    }

    // Stack multiple active events multiplicatively
    void apply(const ActiveEvent& e) {
        volMultiplier    *= e.event.volMultiplier;
        spreadMultiplier *= e.event.spreadMultiplier;
    }
};

// ── Risk Event Engine ─────────────────────────────────────────────────────────
class RiskEventEngine {
public:
    explicit RiskEventEngine(uint64_t seed = 137)
        : rng_(seed)
        , dist_(0.0, 1.0)
    {}

    // ── Called every N orders from the simulation loop ────────────────────────
    // stocks      — full universe, needed to find cross-stock events
    // prices      — current price of each stock by ticker
    // activeTicker— which stock the player is currently trading
    // Returns the modifiers that should be applied this tick
    MarketModifiers tick(
        const std::vector<StockProfile>& stocks,
        std::unordered_map<std::string, double>& prices,
        const std::string& activeTicker)
    {
        // Age out expired events
        activeEvents_.erase(
            std::remove_if(activeEvents_.begin(), activeEvents_.end(),
                [](const ActiveEvent& e) { return e.isExpired(); }),
            activeEvents_.end());

        // Decrement remaining duration on all active events
        for (auto& e : activeEvents_)
            --e.ordersRemaining;

        // Try to fire new events on each stock
        for (const auto& stock : stocks) {
            if (stock.events.empty()) continue;

            double roll = dist_(rng_);
            double threshold = stock.eventProbability
                             / static_cast<double>(stock.regimeCheckEvery);

            if (roll < threshold) {
                // Pick a random event from this stock's event list
                std::uniform_int_distribution<std::size_t> pick(
                    0, stock.events.size() - 1);
                const RiskEvent& evt = stock.events[pick(rng_)];

                // Fire it
                fireEvent(evt, stock.ticker, prices, stocks);
            }
        }

        // Compute combined modifiers for active ticker
        MarketModifiers mods;
        for (const auto& e : activeEvents_) {
            if (e.ticker == activeTicker || e.event.affectsAll)
                mods.apply(e);
        }

        // Consume any pending price delta for this ticker
        auto it = pendingDeltas_.find(activeTicker);
        if (it != pendingDeltas_.end()) {
            mods.priceDelta = it->second;
            pendingDeltas_.erase(it);
        }

        return mods;
    }

    // ── Access news banners for GUI rendering ─────────────────────────────────
    const std::deque<NewsBanner>& banners() const { return banners_; }

    // ── Check if a specific ticker has any active events ──────────────────────
    bool hasActiveEvent(const std::string& ticker) const {
        return std::any_of(activeEvents_.begin(), activeEvents_.end(),
            [&](const ActiveEvent& e) {
                return e.ticker == ticker || e.event.affectsAll;
            });
    }

    // ── Current combined vol multiplier for a ticker ──────────────────────────
    double currentVolMultiplier(const std::string& ticker) const {
        double mult = 1.0;
        for (const auto& e : activeEvents_)
            if (e.ticker == ticker || e.event.affectsAll)
                mult *= e.event.volMultiplier;
        return mult;
    }

    void reset() {
        activeEvents_.clear();
        banners_.clear();
        pendingDeltas_.clear();
    }

private:
    void fireEvent(
        const RiskEvent& evt,
        const std::string& ticker,
        std::unordered_map<std::string, double>& prices,
        const std::vector<StockProfile>& stocks)
    {
        // Apply immediate price delta
        if (evt.affectsAll) {
            for (auto& [t, price] : prices)
                price += evt.priceDelta;
        } else {
            prices[ticker] += evt.priceDelta;

            // Apply anti-correlation to related stocks
            applyCorrelation(evt, ticker, prices, stocks);
        }

        // Register as active event
        activeEvents_.push_back({
            .event            = evt,
            .ticker           = ticker,
            .ordersRemaining  = evt.durationOrders
        });

        // Store price delta for consumption on next tick
        if (evt.affectsAll) {
            for (const auto& s : stocks)
                pendingDeltas_[s.ticker] += evt.priceDelta;
        } else {
            pendingDeltas_[ticker] += evt.priceDelta;
        }

        // Push news banner
        pushBanner(evt, ticker);
    }

    // ── Apply cross-stock correlations ────────────────────────────────────────
    void applyCorrelation(
        const RiskEvent& evt,
        const std::string& sourceTicker,
        std::unordered_map<std::string, double>& prices,
        const std::vector<StockProfile>& stocks)
    {
        // BANK and CRUDE anti-correlate on rate events
        if (sourceTicker == "BANK") {
            auto it = prices.find("CRUDE");
            if (it != prices.end())
                it->second -= evt.priceDelta * 0.4;
        }
        if (sourceTicker == "CRUDE") {
            auto it = prices.find("BANK");
            if (it != prices.end())
                it->second -= evt.priceDelta * 0.4;
        }

        // DEFN rises slightly when any crash event fires
        if (evt.priceDelta < -3.0) {
            auto it = prices.find("DEFN");
            if (it != prices.end())
                it->second += std::abs(evt.priceDelta) * 0.15;
        }

        // UTIL rises slightly on rate cut events
        if (sourceTicker == "BANK" && evt.priceDelta > 0) {
            auto it = prices.find("UTIL");
            if (it != prices.end())
                it->second += evt.priceDelta * 0.2;
        }
    }

    // ── Push a news banner — cap at 8 visible at once ─────────────────────────
    void pushBanner(const RiskEvent& evt, const std::string& ticker) {
        NewsBanner banner {
            .headline   = evt.headline,
            .ticker     = ticker,
            .isPositive = evt.priceDelta >= 0,
            .firedAt    = std::chrono::steady_clock::now()
        };

        banners_.push_front(banner);

        // Trim expired banners
        while (!banners_.empty() && !banners_.back().isVisible())
            banners_.pop_back();
    }

    std::mt19937_64                        rng_;
    std::uniform_real_distribution<double> dist_;
    std::vector<ActiveEvent>               activeEvents_;
    std::deque<NewsBanner>                 banners_;
    std::unordered_map<std::string, double> pendingDeltas_;
};