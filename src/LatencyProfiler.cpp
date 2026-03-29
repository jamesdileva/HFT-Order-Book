// LatencyProfiler.cpp
#include "LatencyProfiler.hpp"
#include <fstream>
#include <stdexcept>
#include <unordered_map>

void LatencyProfiler::recordEntry(uint64_t orderId, TimePoint t) {
    pending_[orderId] = t;
    sortDirty_ = true;
}

void LatencyProfiler::recordFill(uint64_t orderId, TimePoint t) {
    auto it = pending_.find(orderId);
    if (it == pending_.end()) return;   // fill for unknown order — ignore

    int64_t ns = std::chrono::duration_cast<Nanos>(t - it->second).count();
    samples_.push_back({ orderId, ns });
    pending_.erase(it);
    sortDirty_ = true;
}

// ── Lazy sort ─────────────────────────────────────────────────────────────────
void LatencyProfiler::ensureSorted() const {
    if (!sortDirty_) return;

    sorted_.resize(samples_.size());
    std::transform(samples_.begin(), samples_.end(),
                   sorted_.begin(),
                   [](const LatencySample& s) { return s.nanoseconds; });

    std::sort(sorted_.begin(), sorted_.end());
    sortDirty_ = false;
}

// ── Stats ─────────────────────────────────────────────────────────────────────
double LatencyProfiler::meanNs() const {
    if (samples_.empty()) return 0.0;
    int64_t sum = std::accumulate(samples_.begin(), samples_.end(), int64_t{0},
        [](int64_t acc, const LatencySample& s) { return acc + s.nanoseconds; });
    return static_cast<double>(sum) / samples_.size();
}

double LatencyProfiler::medianNs() const {
    ensureSorted();
    if (sorted_.empty()) return 0.0;
    std::size_t mid = sorted_.size() / 2;
    return (sorted_.size() % 2 == 0)
        ? (sorted_[mid - 1] + sorted_[mid]) / 2.0
        : static_cast<double>(sorted_[mid]);
}

double LatencyProfiler::p99Ns() const {
    ensureSorted();
    if (sorted_.empty()) return 0.0;
    std::size_t idx = static_cast<std::size_t>(sorted_.size() * 0.99);
    return static_cast<double>(sorted_[std::min(idx, sorted_.size() - 1)]);
}

double LatencyProfiler::p50Ns() const {
    return medianNs();
}

int64_t LatencyProfiler::minNs() const {
    ensureSorted();
    return sorted_.empty() ? 0 : sorted_.front();
}

int64_t LatencyProfiler::maxNs() const {
    ensureSorted();
    return sorted_.empty() ? 0 : sorted_.back();
}

// ── Report ────────────────────────────────────────────────────────────────────
void LatencyProfiler::report() const {
    if (samples_.empty()) {
        std::cout << "No latency samples recorded.\n";
        return;
    }

    std::cout << "\n════════════════════════════════════\n";
    std::cout << "       LATENCY REPORT (nanoseconds) \n";
    std::cout << "════════════════════════════════════\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Samples   : " << samples_.size()  << "\n";
    std::cout << "  Mean      : " << meanNs()          << " ns\n";
    std::cout << "  Median    : " << medianNs()        << " ns\n";
    std::cout << "  p50       : " << p50Ns()           << " ns\n";
    std::cout << "  p99       : " << p99Ns()           << " ns\n";
    std::cout << "  Min       : " << minNs()           << " ns\n";
    std::cout << "  Max       : " << maxNs()           << " ns\n";
    std::cout << "════════════════════════════════════\n\n";
}

// ── CSV export ────────────────────────────────────────────────────────────────
void LatencyProfiler::toCSV(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Could not open file: " + filename);

    file << "order_id,latency_ns\n";
    for (const auto& s : samples_)
        file << s.orderId << "," << s.nanoseconds << "\n";

    std::cout << "Latency data written to: " << filename << "\n";
}