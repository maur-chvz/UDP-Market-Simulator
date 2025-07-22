# UDP-Market-Simulator

## Project Overview

A high-performance C++ simulation engine that emulates real-time market data feeds from major stock exchanges (NYSE, NASDAQ). This simulator generates realistic trading data and broadcasts it over **UDP multicast**, creating a powerful testing environment for **high-frequency trading systems** and **market data infrastructure**.

---

## Key Features

### Multi-Exchange Support

* **NYSE Simulation**: Trades blue-chip stocks (AAPL, GOOGL, MSFT, TSLA, AMZN)
* **NASDAQ Simulation**: Focuses on tech stocks (NVDA, META, NFLX, AMD, INTC)
* **Independent Data Streams**: Each exchange uses separate multicast groups
* **Configurable Message Rates**: Adjustable throughput (default: 800–600 msgs/sec per feed)

### Realistic Market Dynamics

* **Geometric Brownian Motion**: Price movements modeled with realistic volatility
* **Market Sessions**: Simulates open/normal/close activity levels
* **Volume Generation**: Randomized trade sizes (100–10,000 shares)
* **Bid/Ask Simulation**: Randomized buy/sell sides
* **Tick-Size Compliance**: Prices obey stock-specific minimum increments

### High-Performance Architecture

* **Binary Protocol**: Compact 33-byte packed message format
* **UDP Multicast**: Low-latency, one-to-many broadcasting
* **Burst Mode**: 10x message rate during market open/close
* **Lock-Free Design**: Non-blocking, thread-safe message emission
* **Microsecond Timestamps**: Precision for latency analysis

---

## Technical Specifications

### Message Format (33 bytes)

| Field           | Size    | Description                     |
| --------------- | ------- | ------------------------------- |
| Timestamp       | 8 bytes | Microseconds since epoch        |
| Instrument ID   | 4 bytes | Unique stock identifier         |
| Price           | 8 bytes | IEEE 754 double                 |
| Volume          | 8 bytes | Number of shares traded         |
| Side            | 1 byte  | `'B'` = Buy, `'S'` = Sell       |
| Sequence Number | 4 bytes | Per-instrument message ordering |

### Network Configuration

* **NYSE Feed**: `224.1.1.1:9001` (Multicast)
* **NASDAQ Feed**: `224.1.1.2:9002` (Multicast)
* **Protocol**: UDP, TTL = 1
* **Loopback**: Disabled for performance

### Performance Characteristics

* **Base Throughput**: \~1,400 messages/sec
* **Burst Throughput**: Up to 14,000 messages/sec
* **Latency**: Sub-100μs message generation
* **CPU Usage**: Single-threaded per exchange
* **Memory**: Stack-allocated, minimal heap use

---

## Market Simulation Models

### Price Movement Algorithm (Geometric Brownian Motion)

```cpp
price_change = current_price × volatility × random_factor;
new_price = round(old_price + price_change, tick_size);
```

### Volatility Parameters

* **Low Volatility (2.0%)**: MSFT, INTC
* **Medium Volatility (2.5–3.5%)**: AAPL, GOOGL, META
* **High Volatility (4.0–5.0%)**: TSLA, NVDA, AMD

### Trading Patterns

* **Market Open**: 10x burst for first 10 seconds
* **Normal Hours**: Steady baseline
* **Market Close**: Tapered activity
* **Quiet Periods**: Occasional natural gaps in traffic

---

## Use Cases

### High-Frequency Trading Development

* Strategy backtesting
* Latency measurement
* Load testing
* Algorithm validation

### Market Data Infrastructure

* Feed handler development
* Normalization pipeline testing
* Pub/sub system validation
* In-memory caching experiments

### Education & Research

* Study market microstructure
* High-performance C++ architecture
* Custom protocol design
* Real-time risk calculation demos

---

## Deployment & Integration

### Standalone Simulator

```bash
# Build
g++ -o simulator market_feed_simulator.cpp -lpthread

# Run
./simulator
```

* Broadcasts simulated data on NYSE and NASDAQ feeds
* Simulates realistic session behaviors (open, normal, close)

### Integration Points

* Market data systems (UDP subscriber)
* Trading engines (real-time feed)
* Risk platforms (valuation/testing)
* Analytics systems (tick collection & replay)

### Configuration Options

```cpp
simulator.setMessageRate(1000);           // msgs/sec
simulator.setBurstMode(true);             // enable bursts
simulator.setMarketOpen(false);           // simulate after-hours
simulator.addInstrument(id, symbol, base_price, volatility);
```

---

## Future Enhancements

### Protocol Extensions

* Options: strike, expiry, Greeks
* Bonds: yields, credit spreads
* FX: currency pairs, volatility
* Derivatives: futures, swaps

### Advanced Features

* Market maker behavior (depth/spread)
* News events (volatility shocks)
* Circuit breakers (halt/resume)
* Corporate actions (splits/dividends)

### Performance Optimizations

* RDMA & kernel bypass (e.g., DPDK)
* NIC hardware timestamping
* NUMA-aware threading
* SIMD vectorized processing

---

## Requirements

### System Requirements

* **OS**: Linux (Ubuntu 18.04+, CentOS 7+)
* **Compiler**: GCC 7+ / Clang 6+ (C++17)
* **Memory**: 512MB+ RAM
* **Network**: Multicast-capable NIC

### Dependencies

* C++17 STL (threads, atomics, chrono)
* POSIX sockets, `pthread`
* (Optional) CMake 3.10+ for build configuration

### Runtime

* No root required
* UDP firewall rules must allow specified ports
* Enable multicast routing
* Accurate time via NTP recommended

---

## 📎 Summary

This simulator is a production-grade, realistic market data engine designed for **performance testing**, **trading system development**, and **educational exploration**. With its configurable dynamics and low-latency architecture, it's ideal for replicating the behavior of real-world exchanges.
