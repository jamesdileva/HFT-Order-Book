// OrderBook.hpp
#pragma once

#include <map>
#include <deque>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include "Order.hpp"

// ── Price level ───────────────────────────────────────────────────────────────
struct PriceLevel {
    double               price;
    std::deque<Order*>   orders;   // FIFO queue of orders at this price
    uint32_t             totalQty; // sum of remaining quantity at this level

    void add(Order* o)    { orders.push_back(o); totalQty += o->remaining(); }
    void remove(Order* o) { totalQty -= o->remaining(); }
};

// ── Order Book ────────────────────────────────────────────────────────────────
class OrderBook {
public:
    // Add a resting limit order to the book
    void addOrder(Order* order);

    // Cancel an order by ID — returns false if not found
    bool cancelOrder(uint64_t orderId);

    // Best bid (highest buy price) — empty if no bids
    std::optional<double> bestBid() const;

    // Best ask (lowest sell price) — empty if no asks
    std::optional<double> bestAsk() const;

    // Spread between best ask and best bid
    std::optional<double> spread() const;

    // Access internals for the matching engine
    std::map<double, PriceLevel, std::greater<double>>& bids() { return bids_; }
    std::map<double, PriceLevel>&                       asks() { return asks_; }

    // Diagnostics
    void printBook(int levels = 5) const;
    std::size_t orderCount() const { return index_.size(); }

private:
    // Bids: highest price first (std::greater)
    std::map<double, PriceLevel, std::greater<double>> bids_;

    // Asks: lowest price first (default std::less)
    std::map<double, PriceLevel>                       asks_;

    // Fast order lookup by ID for cancellations
    std::unordered_map<uint64_t, Order*> index_;
};