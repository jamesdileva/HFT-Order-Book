// LatencyProfiler.hpp
#pragma once

#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>

using Clock     = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;
using Nanos     = std::chrono::nanoseconds;

// ── Single latency sample ─────────────────────────────────────────────────────
struct LatencySample {
    uint64_t orderId;
    int64_t  nanoseconds;   // order-to-fill latency in nanoseconds
};

// ── Latency Profiler ──────────────────────────────────────────────────────────
class LatencyProfiler {
public:
    // Called the moment an order enters the engine
    void recordEntry(uint64_t orderId, TimePoint t);

    // Called the moment a fill is confirmed
    void recordFill(uint64_t orderId, TimePoint t);

    // Compute and print full stats report
    void report() const;

    // Individual stats — computed on demand
    double meanNs()    const;
    double medianNs()  const;
    double p99Ns()     const;
    double p50Ns()     const;
    int64_t minNs()    const;
    int64_t maxNs()    const;

    std::size_t sampleCount() const { return samples_.size(); }

    // Dump raw samples to CSV for portfolio visualization
    void toCSV(const std::string& filename) const;

private:
    // Pending entries waiting for their fill timestamp
    std::unordered_map<uint64_t, TimePoint> pending_;

    // Completed samples — entry matched with fill
    mutable std::vector<LatencySample> samples_;

    // Sorted copy for percentile math — built lazily
    mutable std::vector<int64_t> sorted_;
    mutable bool                 sortDirty_ = true;

    void ensureSorted() const;
};