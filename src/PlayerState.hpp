// PlayerState.hpp
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cstdint>

// ── A single completed trade in the player's history ─────────────────────────
struct Trade {
    std::string ticker;
    bool        isLong;         // true = bought, false = sold short
    double      entryPrice;
    double      exitPrice;
    uint32_t    quantity;
    double      realizedPnl;    // exitPrice - entryPrice * qty (long)
    std::chrono::steady_clock::time_point timestamp;
};

// ── One open position in a single stock ──────────────────────────────────────
struct Position {
    std::string ticker;
    int32_t     quantity;       // positive = long, negative = short
    double      avgEntryPrice;  // volume-weighted average entry
    double      unrealizedPnl;  // mark-to-market against current price
    double      realizedPnl;    // locked in from partial closes

    // ── Update unrealized P&L against current market price ───────────────────
    void markToMarket(double currentPrice) {
        unrealizedPnl = (currentPrice - avgEntryPrice) * quantity;
    }

    // ── Total P&L for this position ───────────────────────────────────────────
    double totalPnl() const { return unrealizedPnl + realizedPnl; }

    bool isFlat()  const { return quantity == 0; }
    bool isLong()  const { return quantity > 0; }
    bool isShort() const { return quantity < 0; }
};

// ── One entry on the leaderboard ─────────────────────────────────────────────
struct LeaderboardEntry {
    std::string playerName;
    std::string ticker;
    double      finalPnl;
    double      peakPnl;
    double      maxDrawdown;
    int         totalTrades;
    double      winRate;        // fraction of trades that were profitable
    std::chrono::steady_clock::time_point timestamp;

    bool operator>(const LeaderboardEntry& o) const {
        return finalPnl > o.finalPnl;
    }
};

// ── Full player state ─────────────────────────────────────────────────────────
struct PlayerState {
    // Identity
    std::string playerName  = "TRADER_01";
    std::string activeTicker;       // which stock they're trading

    // Capital
    double startingCapital  = 100000.00;
    double cash             = 100000.00;  // available cash
    double marginUsed       = 0.0;        // capital tied up in open positions
    double maxMargin        = 50000.00;   // maximum allowed margin (50% of capital)

    // Active position in the current stock
    Position activePosition;

    // Completed trades this session
    std::vector<Trade> tradeHistory;

    // Running P&L metrics
    double realizedPnl      = 0.0;   // sum of all closed trade P&L
    double unrealizedPnl    = 0.0;   // current open position mark-to-market
    double peakPnl          = 0.0;   // highest total P&L reached this session
    double maxDrawdown      = 0.0;   // largest peak-to-trough drop
    double totalPnl         = 0.0;   // realizedPnl + unrealizedPnl

    // Session state
    bool   isMarginCalled   = false;
    bool   sessionOver      = false;
    double sessionEndPnl    = 0.0;

    // Recent P&L history for the sparkline chart — last 200 samples
    std::deque<double> pnlHistory;
    static constexpr std::size_t MAX_PNL_HISTORY = 200;

    // ── Place a buy order ─────────────────────────────────────────────────────
    // Returns false if insufficient capital or margin exceeded
    bool buy(double price, uint32_t qty) {
        double cost = price * qty;
        if (cost > cash + (maxMargin - marginUsed)) return false;

        if (activePosition.isFlat()) {
            // Opening a new long
            activePosition.ticker        = activeTicker;
            activePosition.quantity      = static_cast<int32_t>(qty);
            activePosition.avgEntryPrice = price;
            activePosition.realizedPnl   = 0.0;
        }
        else if (activePosition.isLong()) {
            // Adding to long — recalculate VWAP entry price
            double totalCost = activePosition.avgEntryPrice
                             * activePosition.quantity
                             + price * qty;
            activePosition.quantity      += static_cast<int32_t>(qty);
            activePosition.avgEntryPrice  = totalCost / activePosition.quantity;
        }
        else {
            // Covering a short
            closePartial(price, qty);
            return true;
        }

        double cashCost = std::min(cost, cash);
        cash       -= cashCost;
        marginUsed += std::max(0.0, cost - cashCost);
        return true;
    }

    // ── Place a sell order ────────────────────────────────────────────────────
    bool sell(double price, uint32_t qty) {
        if (activePosition.isFlat()) {
            // Opening a short
            activePosition.ticker        = activeTicker;
            activePosition.quantity      = -static_cast<int32_t>(qty);
            activePosition.avgEntryPrice = price;
            activePosition.realizedPnl   = 0.0;
            marginUsed += price * qty;
        }
        else if (activePosition.isShort()) {
            // Adding to short
            double totalCost = activePosition.avgEntryPrice
                             * std::abs(activePosition.quantity)
                             + price * qty;
            activePosition.quantity     -= static_cast<int32_t>(qty);
            activePosition.avgEntryPrice = totalCost
                                         / std::abs(activePosition.quantity);
            marginUsed += price * qty;
        }
        else {
            // Closing a long
            closePartial(price, qty);
            return true;
        }
        return true;
    }

    // ── Flatten entire position at market ─────────────────────────────────────
    void flatten(double currentPrice) {
        if (activePosition.isFlat()) return;
        uint32_t qty = static_cast<uint32_t>(std::abs(activePosition.quantity));
        closePartial(currentPrice, qty);
    }

    // ── Update mark-to-market and P&L metrics ─────────────────────────────────
    void update(double currentPrice) {
        activePosition.markToMarket(currentPrice);
        unrealizedPnl = activePosition.unrealizedPnl;
        totalPnl      = realizedPnl + unrealizedPnl;

        // Track peak and drawdown
        if (totalPnl > peakPnl) peakPnl = totalPnl;
        double drawdown = peakPnl - totalPnl;
        if (drawdown > maxDrawdown) maxDrawdown = drawdown;

        // Margin call check — if losses exceed 80% of starting capital
        if (totalPnl < -(startingCapital * 0.80)) {
            isMarginCalled = true;
            sessionOver    = true;
            sessionEndPnl  = totalPnl;
            flatten(currentPrice);
        }

        // P&L history for sparkline
        pnlHistory.push_back(totalPnl);
        if (pnlHistory.size() > MAX_PNL_HISTORY)
            pnlHistory.pop_front();
    }

    // ── Win condition check ────────────────────────────────────────────────────
    void checkWinCondition(uint64_t ordersProcessed, uint64_t targetOrders) {
        if (ordersProcessed >= targetOrders && !sessionOver) {
            sessionOver   = true;
            sessionEndPnl = totalPnl;
        }
    }

    // ── Session stats ──────────────────────────────────────────────────────────
    double winRate() const {
        if (tradeHistory.empty()) return 0.0;
        int wins = std::count_if(tradeHistory.begin(), tradeHistory.end(),
            [](const Trade& t) { return t.realizedPnl > 0; });
        return static_cast<double>(wins) / tradeHistory.size();
    }

    int totalTrades() const {
        return static_cast<int>(tradeHistory.size());
    }

    // ── Build a leaderboard entry from current session ─────────────────────────
    LeaderboardEntry toLeaderboardEntry() const {
        return {
            .playerName  = playerName,
            .ticker      = activeTicker,
            .finalPnl    = sessionEndPnl,
            .peakPnl     = peakPnl,
            .maxDrawdown = maxDrawdown,
            .totalTrades = totalTrades(),
            .winRate     = winRate(),
            .timestamp   = std::chrono::steady_clock::now()
        };
    }

    // ── Reset for a new session ────────────────────────────────────────────────
    void reset(const std::string& ticker) {
        activeTicker    = ticker;
        cash            = startingCapital;
        marginUsed      = 0.0;
        activePosition  = Position{};
        tradeHistory.clear();
        pnlHistory.clear();
        realizedPnl     = 0.0;
        unrealizedPnl   = 0.0;
        peakPnl         = 0.0;
        maxDrawdown     = 0.0;
        totalPnl        = 0.0;
        isMarginCalled  = false;
        sessionOver     = false;
        sessionEndPnl   = 0.0;
    }

private:
    // ── Close part or all of an open position ──────────────────────────────────
    void closePartial(double exitPrice, uint32_t qty) {
        int32_t closeQty = std::min(static_cast<uint32_t>(
            std::abs(activePosition.quantity)), qty);

        double pnl = activePosition.isLong()
            ? (exitPrice - activePosition.avgEntryPrice) * closeQty
            : (activePosition.avgEntryPrice - exitPrice) * closeQty;

        activePosition.realizedPnl += pnl;
        activePosition.quantity    += activePosition.isLong()
            ? -closeQty : closeQty;

        realizedPnl += pnl;
        marginUsed   = std::max(0.0, marginUsed - exitPrice * closeQty);
        cash        += exitPrice * closeQty + pnl;

        if (activePosition.isFlat()) {
            // Record completed trade
            tradeHistory.push_back({
                .ticker      = activeTicker,
                .isLong      = pnl > 0 ? true : false,
                .entryPrice  = activePosition.avgEntryPrice,
                .exitPrice   = exitPrice,
                .quantity    = static_cast<uint32_t>(closeQty),
                .realizedPnl = pnl,
                .timestamp   = std::chrono::steady_clock::now()
            });
            activePosition = Position{};
            marginUsed     = 0.0;
        }
    }
};

// ── Leaderboard — persists across sessions ────────────────────────────────────
struct Leaderboard {
    std::vector<LeaderboardEntry> entries;
    static constexpr int MAX_ENTRIES = 10;

    void submit(const LeaderboardEntry& entry) {
        entries.push_back(entry);
        std::sort(entries.begin(), entries.end(),
            [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                return a.finalPnl > b.finalPnl;
            });
        if (entries.size() > MAX_ENTRIES)
            entries.resize(MAX_ENTRIES);
    }

    bool isEmpty() const { return entries.empty(); }

    const LeaderboardEntry* best() const {
        return entries.empty() ? nullptr : &entries.front();
    }
};