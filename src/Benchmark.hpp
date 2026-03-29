// Benchmark.hpp
#pragma once

#include <cstdint>
#include <string>
#include "MatchingEngine.hpp"
#include "OrderGenerator.hpp"
#include "LatencyProfiler.hpp"

// ── Benchmark config ──────────────────────────────────────────────────────────
struct BenchmarkConfig {
    uint64_t    totalOrders      = 100000;  // total orders to simulate
    double      makerMixPct      = 0.30;    // 30% market maker orders
    bool        printBookUpdates = false;   // print book state every N orders
    uint64_t    printInterval    = 10000;   // how often to print if enabled
    std::string csvOutput        = "results/benchmark_results.csv";
    std::string latencyCSV       = "results/latency_samples.csv";
};

// ── Benchmark results ─────────────────────────────────────────────────────────
struct BenchmarkResult {
    uint64_t totalOrders;
    uint64_t totalFills;
    uint64_t totalVolume;
    double   fillRate;          // fills / orders
    double   ordersPerSecond;
    double   meanLatencyNs;
    double   medianLatencyNs;
    double   p99LatencyNs;
    double   minLatencyNs;
    double   maxLatencyNs;
    double   elapsedSeconds;
};

// ── Benchmark harness ─────────────────────────────────────────────────────────
class Benchmark {
public:
    explicit Benchmark(BenchmarkConfig cfg = {});

    // Run the full simulation
    BenchmarkResult run();

    // Print formatted results to stdout
    static void printResults(const BenchmarkResult& r);

    // Write summary results to CSV
    void resultsToCSV(const BenchmarkResult& r) const;

private:
    BenchmarkConfig  cfg_;
    MatchingEngine   engine_;
    OrderGenerator   generator_;
    LatencyProfiler  profiler_;
};