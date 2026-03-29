// OrderBook.cpp
#include "OrderBook.hpp"
#include <iostream>
#include <iomanip>

void OrderBook::addOrder(Order* order) {
    if (order->side == OrderSide::Buy) {
        bids_[order->price].price = order->price;
        bids_[order->price].add(order);
    } else {
        asks_[order->price].price = order->price;
        asks_[order->price].add(order);
    }
    index_[order->id] = order;
}

bool OrderBook::cancelOrder(uint64_t orderId) {
    auto it = index_.find(orderId);
    if (it == index_.end()) return false;

    Order* order = it->second;
    order->status = OrderStatus::Cancelled;

    if (order->side == OrderSide::Buy) {
        auto levelIt = bids_.find(order->price);
        if (levelIt != bids_.end()) {
            levelIt->second.remove(order);
            auto& q = levelIt->second.orders;
            q.erase(std::remove(q.begin(), q.end(), order), q.end());
            if (q.empty()) bids_.erase(levelIt);
        }
    } else {
        auto levelIt = asks_.find(order->price);
        if (levelIt != asks_.end()) {
            levelIt->second.remove(order);
            auto& q = levelIt->second.orders;
            q.erase(std::remove(q.begin(), q.end(), order), q.end());
            if (q.empty()) asks_.erase(levelIt);
        }
    }

    index_.erase(it);
    return true;
}

std::optional<double> OrderBook::bestBid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<double> OrderBook::bestAsk() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<double> OrderBook::spread() const {
    auto bid = bestBid();
    auto ask = bestAsk();
    if (!bid || !ask) return std::nullopt;
    return *ask - *bid;
}

void OrderBook::printBook(int levels) const {
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "  ASKS\n";
    int count = 0;
    for (auto it = asks_.rbegin(); it != asks_.rend() && count < levels; ++it, ++count)
        std::cout << "    $" << std::setw(8) << it->first
                  << "  qty: " << it->second.totalQty << "\n";

    auto bid = bestBid();
    auto ask = bestAsk();
    if (bid && ask)
        std::cout << "  --- spread: $" << (*ask - *bid) << " ---\n";

    std::cout << "  BIDS\n";
    count = 0;
    for (auto it = bids_.begin(); it != bids_.end() && count < levels; ++it, ++count)
        std::cout << "    $" << std::setw(8) << it->first
                  << "  qty: " << it->second.totalQty << "\n";

    std::cout << "==================\n\n";
}