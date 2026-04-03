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

// Add this enum before GamePhase
enum class AppMode {
    Benchmark,
    Game
};

// ── Game phase ────────────────────────────────────────────────────────────────
enum class GamePhase {
    MainMenu,
    StockPicker,
    Trading,
    TradingReady,   // paused, waiting for player to press Start
    SessionEnd,
    Leaderboard,
    Benchmark
};

// ── Full shared game state — read by GUI thread, written by sim thread ─────────
struct GameState {
    std::mutex mtx;
    AppMode appMode = AppMode::Game;
    // Game phase
    GamePhase phase = GamePhase::MainMenu;
    // Staging deque — sim thread writes here lock-free
    //std::deque<FillEntry> pendingFills;
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
    uint64_t benchmarkOrders = 2000000;
    uint64_t totalOrders     = 100000;
    std::atomic<int> speedSetting { 1 };
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

    // Benchmark mode stats — mirrors original SimState
    double   fillRate       = 0.0;
    uint64_t totalFills     = 0;
    double   meanLatNs      = 0.0;
    double   minLatNs       = 0.0;
    double   maxLatNs       = 0.0;
    std::vector<HistoBucket> histogram;
    std::deque<FillEntry>    recentFills;
    static constexpr std::size_t MAX_FILLS = 64;

void pushFill(bool isBuy, double price,
              uint32_t qty, double latUs) {
    recentFills.push_front({ isBuy, price, qty, latUs });
    if (recentFills.size() > MAX_FILLS)
        recentFills.pop_back();
}

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
    void selectMode(AppMode mode);


private:
    void simLoop();
    void benchmarkLoop();  
    void gameLoop();        
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