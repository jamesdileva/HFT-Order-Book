// SimState.hpp
#pragma once

#include <mutex>
#include <vector>
#include <deque>
#include <string>
#include <cstdint>
#include <atomic>

// ── One row in the order book display ────────────────────────────────────────
struct BookLevel {
    double   price;
    uint32_t size;
    uint32_t total;
};

// ── One entry in the recent fills feed ───────────────────────────────────────
struct FillEntry {
    bool        isBuy;
    double      price;
    uint32_t    quantity;
    double      latencyUs;  // microseconds for display
};

// ── Latency histogram bucket ──────────────────────────────────────────────────
struct HistoBucket {
    double   rangeMin;
    double   rangeMax;
    uint64_t count;
};

// ── Full shared state between sim thread and GUI thread ───────────────────────
struct SimState {
    std::mutex mtx;

    // Book snapshot
    std::vector<BookLevel> asks;   // sorted lowest first
    std::vector<BookLevel> bids;   // sorted highest first
    double spread = 0.0;

    // Stats
    uint64_t totalOrders    = 0;
    uint64_t totalFills     = 0;
    double   fillRate       = 0.0;
    double   ordersPerSec   = 0.0;
    double   medianLatNs    = 0.0;
    double   p99LatNs       = 0.0;
    double   minLatNs       = 0.0;
    double   maxLatNs       = 0.0;
    double   meanLatNs      = 0.0;

    // Progress
    uint64_t targetOrders   = 100000;
    uint64_t processedOrders= 0;

    // Recent fills — capped at 64 entries
    std::deque<FillEntry> recentFills;
    static constexpr std::size_t MAX_FILLS = 64;

    // Latency histogram
    std::vector<HistoBucket> histogram;

    // Simulation control
    std::atomic<bool> running    { false };
    std::atomic<bool> paused     { false };
    std::atomic<bool> finished   { false };
    std::atomic<bool> resetFlag  { false };
    std::atomic<int>  speedMulti { 1 };

    // ── Writer helpers (call from sim thread while holding mtx) ──────────────
    void pushFill(bool isBuy, double price, uint32_t qty, double latUs) {
        recentFills.push_front({ isBuy, price, qty, latUs });
        if (recentFills.size() > MAX_FILLS)
            recentFills.pop_back();
    }

    void buildHistogram(const std::vector<double>& samples) {
        if (samples.empty()) return;
        histogram.clear();

        // Log-scale buckets from 100ns to 200ms
        std::vector<double> edges = {
            100, 300, 600, 1000, 2000, 5000,
            10000, 50000, 100000, 500000,
            1000000, 5000000, 200000000
        };

        for (std::size_t i = 0; i + 1 < edges.size(); ++i) {
            uint64_t count = 0;
            for (double s : samples)
                if (s >= edges[i] && s < edges[i+1]) ++count;
            histogram.push_back({ edges[i], edges[i+1], count });
        }
    }
};