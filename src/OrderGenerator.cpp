// OrderGenerator.cpp
#include "OrderGenerator.hpp"
#include <cmath>

OrderGenerator::OrderGenerator(GeneratorConfig cfg, uint64_t seed)
    : cfg_(cfg)
    , rng_(seed)
    , qtyDist_(cfg.minQty, cfg.maxQty)
{}

// ── Random trader ─────────────────────────────────────────────────────────────
Order* OrderGenerator::nextRandom() {
    // Drift the mid price slightly each order — simulates a moving market
    cfg_.midPrice += priceDrift_(rng_) * cfg_.priceVolatility;
    cfg_.midPrice  = std::max(cfg_.midPrice, cfg_.tickSize); // never go negative

    // Randomly pick side
    OrderSide side = (sideDist_(rng_) < 0.5)
        ? OrderSide::Buy
        : OrderSide::Sell;

    // Randomly pick type — mostly limits, some markets
    OrderType type = (typeDist_(rng_) < cfg_.marketOrderPct)
        ? OrderType::Market
        : OrderType::Limit;

    // Limit price is within a few ticks of mid
    double offset = priceDrift_(rng_) * cfg_.spread * 2.0;
    double price  = roundToTick(cfg_.midPrice + offset);

    uint32_t qty = qtyDist_(rng_);

    return makeOrder(side, type, price, qty);
}

// ── Market maker ──────────────────────────────────────────────────────────────
std::pair<Order*, Order*> OrderGenerator::nextMarketMakerQuote() {
    // Market maker always quotes symmetrically around mid
    // They profit if they can buy the bid and sell the ask (capture the spread)
    double halfSpread = cfg_.spread / 2.0;

    double bidPrice = roundToTick(cfg_.midPrice - halfSpread);
    double askPrice = roundToTick(cfg_.midPrice + halfSpread);

    // Market makers quote fixed size — keeps risk manageable
    uint32_t quoteQty = 10;

    Order* bid = makeOrder(OrderSide::Buy,  OrderType::Limit, bidPrice, quoteQty);
    Order* ask = makeOrder(OrderSide::Sell, OrderType::Limit, askPrice, quoteQty);

    return { bid, ask };
}

// ── Internal helpers ──────────────────────────────────────────────────────────
Order* OrderGenerator::makeOrder(OrderSide side,
                                  OrderType type,
                                  double    price,
                                  uint32_t  qty)
{
    Order* o    = pool_.acquire();       // pull from pool — no heap allocation
    o->id       = nextId_++;
    o->type     = type;
    o->side     = side;
    o->status   = OrderStatus::Pending;
    o->price    = price;
    o->quantity = qty;
    o->filled   = 0;
    o->timestamp = std::chrono::high_resolution_clock::now();
    return o;
}

double OrderGenerator::roundToTick(double price) const {
    return std::round(price / cfg_.tickSize) * cfg_.tickSize;
}