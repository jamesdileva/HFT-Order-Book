// Order.hpp
#pragma once

#include <cstdint>
#include <chrono>
#include <string>

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

// ── Simple memory pool ────────────────────────────────────────────────────────
template<typename T, std::size_t PoolSize = 4096>
class ObjectPool {
public:
    ObjectPool() {
        for (std::size_t i = 0; i < PoolSize - 1; ++i)
            pool_[i].next = &pool_[i + 1];
        pool_[PoolSize - 1].next = nullptr;
        head_ = &pool_[0];
    }

    T* acquire() {
        if (!head_) return new T{};      // fallback to heap if pool exhausted
        Node* node = head_;
        head_ = head_->next;
        return reinterpret_cast<T*>(node->data);
    }

    void release(T* obj) {
        obj->~T();
        Node* node = reinterpret_cast<Node*>(obj);
        node->next = head_;
        head_ = node;
    }

private:
    union Node {
        alignas(T) char data[sizeof(T)];
        Node* next;
    };
    Node   pool_[PoolSize];
    Node*  head_;
};