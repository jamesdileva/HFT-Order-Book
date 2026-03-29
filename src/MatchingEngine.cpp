// MatchingEngine.cpp
#include "MatchingEngine.hpp"
#include <algorithm>

MatchingEngine::MatchingEngine(FillCallback onFill)
    : onFill_(std::move(onFill)) {}

// ── Public entry point ────────────────────────────────────────────────────────
void MatchingEngine::submitOrder(Order* order) {
    order->timestamp = std::chrono::high_resolution_clock::now();

    switch (order->type) {
        case OrderType::Limit:  matchLimit(order);  break;
        case OrderType::Market: matchMarket(order); break;
        case OrderType::IOC:    matchIOC(order);    break;
    }
}

// ── Limit order ───────────────────────────────────────────────────────────────
void MatchingEngine::matchLimit(Order* order) {
    bool filled = false;

    if (order->side == OrderSide::Buy) {
        // Buy limit — match against asks at or below our limit price
        filled = matchAgainstBook(order, book_.asks(),
            [&](double askPrice) { return askPrice <= order->price; });
    } else {
        // Sell limit — match against bids at or above our limit price
        filled = matchAgainstBook(order, book_.bids(),
            [&](double bidPrice) { return bidPrice >= order->price; });
    }

    // If not fully filled, rest the remaining quantity in the book
    if (!filled && order->remaining() > 0) {
        order->status = OrderStatus::Pending;
        book_.addOrder(order);
    }
}

// ── Market order ──────────────────────────────────────────────────────────────
void MatchingEngine::matchMarket(Order* order) {
    if (order->side == OrderSide::Buy) {
        // Accept any ask price — sweep until filled or book exhausted
        matchAgainstBook(order, book_.asks(),
            [](double) { return true; });
    } else {
        matchAgainstBook(order, book_.bids(),
            [](double) { return true; });
    }

    // Market orders never rest — cancel whatever didn't fill
    if (order->remaining() > 0)
        order->status = OrderStatus::Cancelled;
}

// ── IOC order ─────────────────────────────────────────────────────────────────
void MatchingEngine::matchIOC(Order* order) {
    // IOC behaves like a limit order but never rests
    if (order->side == OrderSide::Buy) {
        matchAgainstBook(order, book_.asks(),
            [&](double askPrice) { return askPrice <= order->price; });
    } else {
        matchAgainstBook(order, book_.bids(),
            [&](double bidPrice) { return bidPrice >= order->price; });
    }

    // Cancel whatever wasn't immediately filled
    if (order->remaining() > 0 && !order->isDone())
        order->status = OrderStatus::Cancelled;
}

// ── Core match loop ───────────────────────────────────────────────────────────
template<typename MapType>
static bool matchLoop(Order* taker,
                      MapType& book,
                      std::function<bool(double)> priceOk,
                      std::function<void(Order*, Order*, double, uint32_t)> onFill)
{
    auto levelIt = book.begin();

    while (levelIt != book.end() && taker->remaining() > 0) {
        if (!priceOk(levelIt->first)) break;

        PriceLevel& level = levelIt->second;

        while (!level.orders.empty() && taker->remaining() > 0) {
            Order* maker = level.orders.front();

            // Skip cancelled orders that weren't cleaned up yet
            if (maker->isDone()) {
                level.orders.pop_front();
                continue;
            }

            uint32_t tradeQty = std::min(taker->remaining(), maker->remaining());
            double   tradePrice = maker->price;

            // Apply the fill to both sides
            maker->filled += tradeQty;
            taker->filled += tradeQty;
            level.totalQty -= tradeQty;

            maker->status = (maker->remaining() == 0)
                ? OrderStatus::Filled : OrderStatus::PartialFill;
            taker->status = (taker->remaining() == 0)
                ? OrderStatus::Filled : OrderStatus::PartialFill;

            onFill(maker, taker, tradePrice, tradeQty);

            if (maker->isDone())
                level.orders.pop_front();
        }

        // Remove exhausted price levels
        if (level.orders.empty())
            levelIt = book.erase(levelIt);
        else
            ++levelIt;
    }

    return taker->remaining() == 0;
}

bool MatchingEngine::matchAgainstBook(
    Order* taker,
    std::map<double, PriceLevel>& side,
    std::function<bool(double)> priceAcceptable)
{
    return matchLoop(taker, side, priceAcceptable,
        [this](Order* m, Order* t, double p, uint32_t q) {
            recordFill(m, t, p, q);
        });
}

bool MatchingEngine::matchAgainstBook(
    Order* taker,
    std::map<double, PriceLevel, std::greater<double>>& side,
    std::function<bool(double)> priceAcceptable)
{
    return matchLoop(taker, side, priceAcceptable,
        [this](Order* m, Order* t, double p, uint32_t q) {
            recordFill(m, t, p, q);
        });
}

// ── Fill recording ────────────────────────────────────────────────────────────
void MatchingEngine::recordFill(Order* maker, Order* taker,
                                 double price, uint32_t qty) {
    Fill f {
        .makerOrderId = maker->id,
        .takerOrderId = taker->id,
        .price        = price,
        .quantity     = qty,
        .timestamp    = std::chrono::high_resolution_clock::now()
    };

    fills_.push_back(f);
    totalVolume_ += qty;

    if (onFill_) onFill_(f);
}