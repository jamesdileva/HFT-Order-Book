// GameManager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include "StockProfile.hpp"
#include "PlayerState.hpp"
#include "RiskEventEngine.hpp"
#include "OrderGenerator.hpp"
#include "MatchingEngine.hpp"
#include "SimState.hpp"

// ── Per-stock live state ──────────────────────────────────────────────────────
struct StockState {
    StockProfile        profile;
    double              currentPrice;
    Regime              currentRegime;
    double              momentum       = 0.0;
    double              volMultiplier  = 1.0;
    double              spreadMultiplier = 1.0;
    uint64_t            ordersSinceRegimeCheck = 0;

    // Price history for mini sparkline charts — last 100 prices
    std::deque<double>  priceHistory;
    static constexpr std::size_t MAX_PRICE_HISTORY = 100;

    void pushPrice(double price) {
        priceHistory.push_back(price);
        if (priceHistory.size() > MAX_PRICE_HISTORY)
            priceHistory.pop_front();
    }
};

// ── Game phase ────────────────────────────────────────────────────────────────
enum class GamePhase {
    StockPicker,    // player is choosing a stock
    Trading,        // simulation running, player is active
    SessionEnd,     // session over — show results
    Leaderboard     // show top scores
};

// ── Full shared game state — read by GUI thread, written by sim thread ─────────
struct GameState {
    std::mutex mtx;

    // Game phase
    GamePhase phase = GamePhase::StockPicker;

    // All six stocks
    std::vector<StockState> stocks;

    // Which stock index the player picked
    int selectedStockIdx = -1;

    // Player
    PlayerState player;

    // Leaderboard
    Leaderboard leaderboard;

    // Active news banners
    std::vector<NewsBanner> visibleBanners;

    // Order book snapshot for active stock
    std::vector<BookLevel> asks;
    std::vector<BookLevel> bids;
    double spread = 0.0;

    // Simulation progress
    uint64_t totalOrders     = 200000;
    uint64_t processedOrders = 0;

    // Performance stats
    double ordersPerSec  = 0.0;
    double medianLatNs   = 0.0;
    double p99LatNs      = 0.0;

    // Control flags — written by GUI, read by sim thread
    std::atomic<bool> startRequested  { false };
    std::atomic<bool> resetRequested  { false };
    std::atomic<bool> paused          { false };
    std::atomic<bool> simRunning      { false };

    // Player action queue — GUI pushes, sim thread consumes
    struct PlayerAction {
        enum class Type { Buy, Sell, Flatten } type;
        uint32_t quantity;
    };
    std::deque<PlayerAction> actionQueue;
};

// ── Game Manager ──────────────────────────────────────────────────────────────
class GameManager {
public:
    GameManager();
    ~GameManager();

    // Start the simulation thread
    void start();

    // Stop cleanly
    void stop();

    // Access shared state for GUI
    GameState& state() { return state_; }

    // GUI calls these — they post to the action queue safely
    void requestBuy    (uint32_t qty);
    void requestSell   (uint32_t qty);
    void requestFlatten();
    void selectStock   (int idx);
    void requestReset  ();
    void submitToLeaderboard(const std::string& name);

private:
    void simLoop();

    void initStocks();

    void updateRegime(StockState& s, std::mt19937_64& rng);

    void updatePrice(StockState& s,
                     std::mt19937_64& rng,
                     const MarketModifiers& mods);

    void snapshotBook(MatchingEngine& engine);

    void processPlayerActions(PlayerState& player,
                               double currentPrice);

    GameState       state_;
    RiskEventEngine riskEngine_;
    std::thread     simThread_;
    std::atomic<bool> stopFlag_ { false };
};