// main.cpp
#include <iostream>
#include <string>
#include "Benchmark.hpp"

void printUsage() {
    std::cout << "\nHFT Order Book Simulator\n";
    std::cout << "Usage:\n";
    std::cout << "  ./hft                        — run with defaults\n";
    std::cout << "  ./hft --orders 500000        — custom order count\n";
    std::cout << "  ./hft --print-book           — print book snapshots\n";
    std::cout << "  ./hft --orders 100000 --print-book\n\n";
}

int main(int argc, char* argv[]) {
    BenchmarkConfig cfg;

    // ── Simple CLI argument parsing ───────────────────────────────────────────
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        else if (arg == "--orders" && i + 1 < argc) {
            cfg.totalOrders = std::stoull(argv[++i]);
        }
        else if (arg == "--print-book") {
            cfg.printBookUpdates = true;
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage();
            return 1;
        }
    }

    // ── Run ───────────────────────────────────────────────────────────────────
    try {
        Benchmark bench(cfg);
        BenchmarkResult result = bench.run();
        Benchmark::printResults(result);
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}