// Order.hpp
#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <unordered_map>

// ── Order types ──────────────────────────────────────────────────────────────
enum class OrderType  { Limit, Market, IOC };   // IOC = Immediate-Or-Cancel
enum class OrderSide  { Buy, Sell };
enum class OrderStatus{ Pending, Filled, PartialFill, Cancelled };

// ── Core Order struct ─────────────────────────────────────────────────────────
struct alignas(64) Order {
    uint64_t    id;
    OrderType   type;
    OrderSide   side;
    OrderStatus status;
    double      price;      // limit price (ignored for Market orders)
    uint32_t    quantity;   // original quantity
    uint32_t    filled;     // how much has been filled so far
    std::chrono::high_resolution_clock::time_point timestamp;

    // Convenience helpers
    uint32_t remaining() const { return quantity - filled; }
    bool     isDone()    const {
        return status == OrderStatus::Filled ||
               status == OrderStatus::Cancelled;
    }
};


