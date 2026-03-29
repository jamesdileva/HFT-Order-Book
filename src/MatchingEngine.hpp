// MatchingEngine.hpp
#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include "Order.hpp"
#include "OrderBook.hpp"

// ── Fill report — emitted every time a trade occurs ───────────────────────────
struct Fill {
    uint64_t makerOrderId;   // resting order that was sitting in the book
    uint64_t takerOrderId;   // incoming order that triggered the fill
    double   price;          // price the trade executed at
    uint32_t quantity;       // how many units changed hands
    std::chrono::high_resolution_clock::time_point timestamp;
};

// ── Matching Engine ───────────────────────────────────────────────────────────
class MatchingEngine {
public:
    using FillCallback = std::function<void(const Fill&)>;

    explicit MatchingEngine(FillCallback onFill = nullptr);

    // Submit an order — routes to correct handler based on type
    void submitOrder(Order* order);

    // Direct access to the book for inspection / benchmarking
    OrderBook& book() { return book_; }

    // All fills recorded this session
    const std::vector<Fill>& fills() const { return fills_; }

    uint64_t totalFills()    const { return fills_.size(); }
    uint64_t totalVolume()   const { return totalVolume_; }

private:
    void matchLimit (Order* order);
    void matchMarket(Order* order);
    void matchIOC   (Order* order);

    // Core matching loop — shared by all order types
    // Returns true if the incoming order is fully filled
    bool matchAgainstBook(Order* taker,
                          std::map<double, PriceLevel>& side,
                          std::function<bool(double)> priceAcceptable);

    bool matchAgainstBook(Order* taker,
                          std::map<double, PriceLevel,
                          std::greater<double>>& side,
                          std::function<bool(double)> priceAcceptable);

    void recordFill(Order* maker, Order* taker, double price, uint32_t qty);

    OrderBook        book_;
    FillCallback     onFill_;
    std::vector<Fill> fills_;
    uint64_t         totalVolume_ = 0;
};