// main.cpp
#include <thread>
#include <chrono>
#include "Benchmark.hpp"
#include "SimState.hpp"
#include "GUI.hpp"
#include <SDL.h>

// ── Simulation thread ─────────────────────────────────────────────────────────
void runSimulation(SimState& state, uint64_t totalOrders) {
    GeneratorConfig gcfg;
    BenchmarkConfig bcfg;
    bcfg.totalOrders = totalOrders;

    OrderGenerator generator(gcfg);
    MatchingEngine  engine([&](const Fill& fill) {
        // Compute latency
        auto now = std::chrono::high_resolution_clock::now();
        double latNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now - fill.timestamp).count();

        std::lock_guard<std::mutex> lock(state.mtx);
        state.pushFill(
            fill.makerOrderId % 2 == 0,
            fill.price,
            fill.quantity,
            latNs / 1000.0
        );
    });

    LatencyProfiler profiler;
    std::vector<double> latSamples;

    state.running.store(true);
    auto wallStart = std::chrono::high_resolution_clock::now();

    for (uint64_t i = 0; i < totalOrders; ) {
        // Check for reset
        if (state.resetFlag.load()) {
            state.resetFlag.store(false);
            i = 0;
            latSamples.clear();
            profiler = LatencyProfiler{};
            std::lock_guard<std::mutex> lock(state.mtx);
            state.recentFills.clear();
            state.processedOrders = 0;
            wallStart = std::chrono::high_resolution_clock::now();
            continue;
        }

        // Pausing
        if (state.paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        // Speed throttle — at 1x insert small sleeps so you can watch it
        int speed = state.speedMulti.load();
        if (speed < 100 && i % 500 == 0) {
            int delayMs = std::max(1, 16 / speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }

        // Market maker refresh every 100 orders
        if (i % 100 == 0) {
            auto [bid, ask] = generator.nextMarketMakerQuote();
            engine.submitOrder(bid);
            engine.submitOrder(ask);
        }

        Order* order = generator.nextRandom();
        auto entryTime = std::chrono::high_resolution_clock::now();
        engine.submitOrder(order);
        auto fillTime = std::chrono::high_resolution_clock::now();

        double latNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            fillTime - entryTime).count();
        latSamples.push_back(latNs);

        ++i;

        // Update shared state every 200 orders
        if (i % 200 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(now - wallStart).count();

            std::lock_guard<std::mutex> lock(state.mtx);
            state.processedOrders = i;
            state.totalFills      = engine.totalFills();
            state.fillRate        = (double)engine.totalFills() / i;
            state.ordersPerSec    = elapsed > 0 ? i / elapsed : 0;

            // Latency stats from samples
            if (!latSamples.empty()) {
                auto sorted = latSamples;
                std::sort(sorted.begin(), sorted.end());
                state.medianLatNs = sorted[sorted.size() / 2];
                state.p99LatNs    = sorted[(std::size_t)(sorted.size() * 0.99)];
                state.minLatNs    = sorted.front();
                state.maxLatNs    = sorted.back();
                double sum = 0;
                for (double v : sorted) sum += v;
                state.meanLatNs   = sum / sorted.size();
                state.buildHistogram(latSamples);
            }

            // Book snapshot
            state.bids.clear();
            state.asks.clear();
            auto& bids = engine.book().bids();
            auto& asks = engine.book().asks();

            uint32_t total = 0;
            int count = 0;
            for (auto& [price, level] : asks) {
                total += level.totalQty;
                state.asks.push_back({ price, level.totalQty, total });
                if (++count >= 8) break;
            }
            total = 0; count = 0;
            for (auto& [price, level] : bids) {
                total += level.totalQty;
                state.bids.push_back({ price, level.totalQty, total });
                if (++count >= 8) break;
            }

            auto bid = engine.book().bestBid();
            auto ask = engine.book().bestAsk();
            state.spread = (bid && ask) ? *ask - *bid : 0.0;
        }
    }

    state.finished.store(true);
    state.running.store(false);
}

// ── Entry point ───────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    uint64_t totalOrders = 100000;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--orders" && i + 1 < argc)
            totalOrders = std::stoull(argv[++i]);
    }

    SimState state;
    state.targetOrders = totalOrders;

    // Launch simulation on background thread
    std::thread simThread(runSimulation, std::ref(state), totalOrders);

    // Run GUI on main thread
    GUI gui;
    if (!gui.init("HFT Order Book", 1200, 700)) return 1;

    while (gui.render(state)) {}

    gui.shutdown();
    state.paused.store(false);
    state.finished.store(true);
    simThread.join();
    return 0;
}