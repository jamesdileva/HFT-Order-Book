// Benchmark.cpp
#include "Benchmark.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdexcept>

Benchmark::Benchmark(BenchmarkConfig cfg)
    : cfg_(cfg)
    , engine_([this](const Fill& fill) {
        // This lambda is the FillCallback wired into the matching engine
        // Every fill calls back here — we record the fill timestamp immediately
        profiler_.recordFill(fill.takerOrderId,  fill.timestamp);
        profiler_.recordFill(fill.makerOrderId,  fill.timestamp);
    })
{}

// ── Main simulation loop ──────────────────────────────────────────────────────
BenchmarkResult Benchmark::run() {
    std::cout << "Running benchmark: "
              << cfg_.totalOrders << " orders...\n";

    auto start = Clock::now();

    for (uint64_t i = 0; i < cfg_.totalOrders; ++i) {

        if (i % 100 == 0) {
            // Every 100 orders inject a market maker quote (bid + ask pair)
            // This keeps the book populated so market orders have something to hit
            auto [bid, ask] = generator_.nextMarketMakerQuote();

            profiler_.recordEntry(bid->id, bid->timestamp);
            profiler_.recordEntry(ask->id, ask->timestamp);

            engine_.submitOrder(bid);
            engine_.submitOrder(ask);
        }

        // Random trader order
        Order* order = generator_.nextRandom();
        profiler_.recordEntry(order->id, order->timestamp);
        engine_.submitOrder(order);

        if (cfg_.printBookUpdates && i % cfg_.printInterval == 0)
            engine_.book().printBook(3);
    }

    auto end     = Clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    // ── Assemble results ──────────────────────────────────────────────────────
    BenchmarkResult r;
    r.totalOrders     = cfg_.totalOrders;
    r.totalFills      = engine_.totalFills();
    r.totalVolume     = engine_.totalVolume();
    r.fillRate        = static_cast<double>(r.totalFills) / r.totalOrders;
    r.ordersPerSecond = static_cast<double>(cfg_.totalOrders) / elapsed;
    r.meanLatencyNs   = profiler_.meanNs();
    r.medianLatencyNs = profiler_.medianNs();
    r.p99LatencyNs    = profiler_.p99Ns();
    r.minLatencyNs    = static_cast<double>(profiler_.minNs());
    r.maxLatencyNs    = static_cast<double>(profiler_.maxNs());
    r.elapsedSeconds  = elapsed;

    profiler_.toCSV(cfg_.latencyCSV);
    resultsToCSV(r);

    return r;
}

// ── Console output ────────────────────────────────────────────────────────────
void Benchmark::printResults(const BenchmarkResult& r) {
    std::cout << "\n════════════════════════════════════════\n";
    std::cout << "         BENCHMARK RESULTS              \n";
    std::cout << "════════════════════════════════════════\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Orders submitted  : " << r.totalOrders            << "\n";
    std::cout << "  Fills generated   : " << r.totalFills             << "\n";
    std::cout << "  Total volume      : " << r.totalVolume            << "\n";
    std::cout << "  Fill rate         : " << r.fillRate * 100         << "%\n";
    std::cout << "  Elapsed           : " << r.elapsedSeconds         << "s\n";
    std::cout << "  Orders/sec        : " << r.ordersPerSecond        << "\n";
    std::cout << "────────────────────────────────────────\n";
    std::cout << "  Mean latency      : " << r.meanLatencyNs          << " ns\n";
    std::cout << "  Median latency    : " << r.medianLatencyNs        << " ns\n";
    std::cout << "  p50               : " << r.medianLatencyNs        << " ns\n";
    std::cout << "  p99               : " << r.p99LatencyNs           << " ns\n";
    std::cout << "  Min               : " << r.minLatencyNs           << " ns\n";
    std::cout << "  Max               : " << r.maxLatencyNs           << " ns\n";
    std::cout << "════════════════════════════════════════\n\n";
}

// ── CSV summary ───────────────────────────────────────────────────────────────
void Benchmark::resultsToCSV(const BenchmarkResult& r) const {
    std::ofstream file(cfg_.csvOutput);
    if (!file.is_open())
        throw std::runtime_error("Could not open: " + cfg_.csvOutput);

    file << "metric,value\n";
    file << "total_orders,"     << r.totalOrders     << "\n";
    file << "total_fills,"      << r.totalFills      << "\n";
    file << "total_volume,"     << r.totalVolume     << "\n";
    file << "fill_rate,"        << r.fillRate        << "\n";
    file << "orders_per_sec,"   << r.ordersPerSecond << "\n";
    file << "mean_latency_ns,"  << r.meanLatencyNs   << "\n";
    file << "median_latency_ns,"<< r.medianLatencyNs << "\n";
    file << "p99_latency_ns,"   << r.p99LatencyNs    << "\n";
    file << "min_latency_ns,"   << r.minLatencyNs    << "\n";
    file << "max_latency_ns,"   << r.maxLatencyNs    << "\n";
    file << "elapsed_seconds,"  << r.elapsedSeconds  << "\n";

    std::cout << "Summary written to: " << cfg_.csvOutput << "\n";
}