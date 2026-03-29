// OrderGenerator.hpp
#pragma once

#include <cstdint>
#include <random>
#include <functional>
#include "Order.hpp"
#include "ObjectPool.hpp"

// ── Generator config ──────────────────────────────────────────────────────────
struct GeneratorConfig {
    double   midPrice       = 100.0;  // starting reference price
    double   tickSize       = 0.01;   // minimum price increment
    double   spread         = 0.10;   // initial bid/ask spread
    double   priceVolatility= 0.05;   // how much mid price drifts per order
    uint32_t minQty         = 1;
    uint32_t maxQty         = 100;
    double   marketOrderPct = 0.20;   // 20% of orders are market orders
    double   cancelPct      = 0.10;   // 10% chance a resting order gets cancelled
};

// ── Order Generator ───────────────────────────────────────────────────────────
class OrderGenerator {
public:
    explicit OrderGenerator(GeneratorConfig cfg = {},
                            uint64_t seed = 42);

    // Random trader — noise, no strategy
    Order* nextRandom();

    // Market maker — always quotes both sides of the spread
    // Returns two orders: a bid and an ask
    std::pair<Order*, Order*> nextMarketMakerQuote();

    // Current simulated mid price
    double midPrice() const { return cfg_.midPrice; }

    // Pool access so the engine can release orders back
    ObjectPool<Order>& pool() { return pool_; }

private:
    Order* makeOrder(OrderSide side,
                     OrderType type,
                     double    price,
                     uint32_t  qty);

    double roundToTick(double price) const;

    GeneratorConfig    cfg_;
    uint64_t           nextId_ = 1;
    ObjectPool<Order>  pool_;

    // Random number generation
    std::mt19937_64                        rng_;
    std::uniform_real_distribution<double> sideDist_{0.0, 1.0};
    std::uniform_real_distribution<double> typeDist_{0.0, 1.0};
    std::uniform_int_distribution<uint32_t> qtyDist_;
    std::normal_distribution<double>       priceDrift_{0.0, 1.0};
};