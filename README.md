# HFT Order Book Simulator

A high-frequency trading order book simulator built in C++ featuring a price-time priority matching engine capable of processing **1.7–2 million orders per second** with **sub-100 nanosecond median latency**.

---

## Screenshots

**Benchmark Mode** — four-panel performance dashboard  
**Trading Game** — live order book, price chart, P&L curve, speed controls

---

## Performance

| Metric | Result |
|---|---|
| Throughput | 1.7 – 2M orders/sec |
| Median latency (P50) | 100 ns |
| P99 latency | 100 ns |
| Min latency | 100 ns |
| Max latency | ~2.5 ms (OS scheduling) |
| Mean latency | ~142 ns |
| Fill rate | 100.6% |
| Total fills (500K run) | 497,957 |
| Elapsed (500K orders) | ~0.26 seconds |

---

## Features

### Benchmark Mode
- 500,000 order run at maximum speed
- Live latency histogram (log-scale, 13 buckets, p99+ highlighted)
- Four-panel dashboard — order book, histogram, latency stats, session stats
- Hardware grade rating (EXCELLENT / GOOD / FAIR)
- Pause / Resume / Reset controls

### Trading Game
- Six stock personalities — NOVA, UTIL, MEME, BANK, CRUDE, DEFN
- Markov chain regime system — Momentum, Mean Reversion, Volatile, Quiet
- Ornstein-Uhlenbeck price evolution with four forces
- Risk event engine — vol spikes, price jumps, spread widening
- Cross-stock correlations (BANK/CRUDE anti-correlate, DEFN safe haven)
- 100,000 order sessions, configurable speed (0.25x – 5x)
- $100,000 starting capital, margin call at -$80,000
- Live order book, price chart, P&L curve, fill feed
- VWAP entry tracking, max drawdown, win rate
- Persistent leaderboard with session history

---

## Architecture

```
src/
  Order.hpp           — Order struct, alignas(64), fixed-width types
  ObjectPool.hpp      — Template memory pool, zero heap in hot path
  OrderBook.hpp/cpp   — Bid/ask maps, price levels, spread
  MatchingEngine.hpp  — Price-time priority, limit/market/IOC, FillCallback
  OrderGenerator.hpp  — Random trader + market-maker, mt19937_64
  LatencyProfiler.hpp — Nanosecond timing, p50/p99/min/max
  Benchmark.hpp/cpp   — Benchmark harness, CSV output
  StockProfile.hpp    — 6 personalities, Markov chain, OU mean reversion
  PlayerState.hpp     — Signed position, VWAP, drawdown, margin call
  RiskEventEngine.hpp — News events, correlations, two time systems
  GameManager.hpp/cpp — Coordinator, benchmarkLoop, gameLoop, threading
  GUI.hpp/cpp         — ImGui + SDL2, 7-phase state machine
```

**Key design decisions:**
- Object pool allocator — no malloc in hot path
- `alignas(64)` on Order struct — prevents false sharing
- Two completely separate loops — benchmark (max speed) vs game (throttled)
- Lock-free fill callback in benchmark — zero mutex in hot path
- Single latency sort after loop completion — not during run
- `std::atomic` for single flags, `std::mutex` for compound state

---

## Build Instructions

### Prerequisites
- MSYS2 with MinGW-w64 toolchain
- CMake 3.15+
- SDL2 development libraries

### Build
```bash
git clone https://github.com/yourusername/HFT-Order-Book.git
cd HFT-Order-Book
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make
./hft.exe
```

### Clean rebuild
```bash
rm -rf build
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make
```

---

## How to Play

1. Launch the app and select **Trading Game** from the main menu
2. Browse the six stock cards — each has a distinct personality, volatility rating, and description
3. Click **Trade This Stock** to select, read the stock info on the confirmation screen
4. Press **Start Trading** when ready
5. Use **BUY / SELL / FLAT** buttons to manage your position
6. Watch the regime indicator — **MOM** (trending), **REV** (mean reverting), **VOL** (chaotic), **QUT** (calm)
7. Manage your $100,000 capital — margin call fires at -$80,000
8. Session ends at 100,000 orders or margin call — submit your score to the leaderboard

---

## Tech Stack

- **Language:** C++17
- **GUI:** Dear ImGui
- **Windowing:** SDL2 + OpenGL3
- **Build:** CMake + MinGW
- **Random:** std::mt19937_64
- **Threading:** std::thread, std::mutex, std::atomic

---

## License

MIT

---

*Built by James Dileva — 2026*