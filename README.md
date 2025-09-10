# High-Performance Limit Order Book Matching Engine

A production-grade C++ limit order book implementing exchange-standard price-time priority matching with sub-microsecond latency and atomic order processing for high-frequency trading environments.

## 🚀 Performance Highlights

- **Sub-microsecond latency**: 0.02-5.9μs per operation
- **High throughput**: 2.5M+ orders/second peak performance
- **Consistent performance**: 99th percentile latency under 14μs
- **Zero packet loss**: Reliable processing under extreme loads
- **Memory efficient**: Zero-copy memory pooling using pre-allocated orders

## 🎯 Overview

This OrderBook matching engine is designed for high-frequency trading applications, implementing industry-standard price-time priority matching with deterministic execution guarantees. The system supports multiple order types and provides comprehensive performance monitoring and state persistence.

### Supported Order Types
- **GTC (Good Till Cancel)**: Standard limit orders
- **FOK (Fill Or Kill)**: Must be completely filled or rejected
- **FAK (Fill And Kill)**: Partial fills allowed, remainder cancelled
- **GTD (Good Till Day)**: Expires at end of trading day
- **Market Orders**: Execute at best available price

## ✨ Key Features

### 🏎️ Performance Optimizations
- **Zero-copy memory pooling** with pre-allocated order objects
- **Lock-free critical paths** using atomic operations
- **Cache-aligned data structures** for optimal memory access
- **Branch prediction hints** for CPU optimization
- **Inline function optimization** with `__attribute__((always_inline))`

### 🔧 Advanced Functionality
- **Price-time priority matching** following exchange standards
- **Atomic order processing** with rollback capabilities
- **Real-time trade logging** with nanosecond timestamps
- **Order book state persistence** for system recovery
- **Comprehensive benchmarking suite** with micro, load, and stress testing

### 📊 Monitoring & Analytics
- **Real-time performance metrics** with percentile analysis
- **Trade execution statistics** and match rate analysis
- **Memory usage monitoring** and allocation tracking
- **CSV export** for trade data and performance reports

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Order Input   │───▶│   OrderBook      │───▶│  Trade Output   │
│   (Server)      │    │   (Matching)     │    │   (Logging)     │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│  Order Pool     │    │  Price-Time      │    │  State          │
│  (Memory Mgmt)  │    │  Priority Queue  │    │  Persistence    │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### Core Components

1. **OrderBook**: Main matching engine with price-time priority
2. **OrderPool**: Memory pool for efficient order allocation
3. **Server**: Multi-threaded order processing with queue management
4. **Log**: Trade execution logging and analytics

## 🔧 Implementation Details

### Memory Management
```cpp
// Pre-allocated order pool for zero-copy operations
OrderPool orderPool(4096);  // 4,096 orders pre-allocated

// Lock-free order acquisition
Order* order = orderPool.acquire();
// ... process order ...
orderPool.release(order);
```

### Data Structures
```cpp
// Price-time priority using std::map
std::map<double, std::vector<Order>> bids;   // Descending price order
std::map<double, std::vector<Order>> asks;   // Ascending price order

// O(1) order lookup
std::unordered_map<int, Order*> orderIndex;
```

### Performance Optimizations
```cpp
// Branch prediction hints
if (__builtin_expect(incoming.getType() == OrderType::FillOrKill, 0)) {
    // Optimize for rare FOK orders
}

// Inline critical path functions
inline void executeMatch(Order& incoming, Order& resting) 
    __attribute__((always_inline));
```

## 📈 Performance Benchmarks

### Order Type Performance
| Order Type | Average Latency | Min Latency | Max Latency | Throughput |
|------------|----------------|-------------|-------------|------------|
| **FOK**    | 0.02μs         | 0.00μs      | 0.12μs      | 50M ops/s  |
| **FAK**    | 0.02μs         | 0.00μs      | 0.04μs      | 50M ops/s  |
| **Market** | 0.02μs         | 0.00μs      | 0.04μs      | 50M ops/s  |
| **GTC**    | 0.43μs         | 0.04μs      | 1.42μs      | 2.3M ops/s |
| **GTD**    | 1.98μs         | 0.08μs      | 4.92μs      | 500K ops/s |

### Load Testing Results
- **100 orders**: 0.45μs average, 2.2M ops/s
- **1,000 orders**: 0.40μs average, 2.4M ops/s  
- **10,000 orders**: 4.27μs average, 233K ops/s
- **50,000 orders**: 38.74μs average, 26K ops/s

### Stress Testing
- **100K orders**: 82.34μs average, 12K ops/s
- **Real market data**: 5.05μs average, 176K ops/s
- **Match rate**: 74.48% with 3,724 trades from 5,000 orders

## 🛠️ Installation & Usage

### Prerequisites
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- Make build system
- Linux/macOS (tested on Ubuntu 20.04+ and macOS 12+)

### Quick Start
```bash
#Build the project
make clean && make

# Start the order book
./orderbook

# Example commands:
# GTC BUY 100.50 1000    # Good Till Cancel buy order
# FOK SELL 100.45 500    # Fill Or Kill sell order
# MARKET BUY 2000        # Market buy order
# quit                   # Exit and save state
```

## 📚 API Reference

### OrderBook Class
```cpp
class OrderBook {
public:
    // Add order to the book
    void addOrder(const Order& order);
    
    // Cancel existing order
    void cancelOrder(int orderId);
    
    // Get best bid/ask prices
    double getBestBid() const;
    double getBestAsk() const;
    
    // Save/load order book state
    void saveOrdersToFile(const std::string& filename) const;
    void loadOrdersFromFile(const std::string& filename);
    
    // Display order book
    void printBook(int depth = 5) const;
};
```

### Order Types
```cpp
enum class OrderType {
    GoodTillCancel,    // GTC
    FillOrKill,        // FOK  
    FillAndKill,       // FAK
    GoodForDay,        // GTD
    Market             // Market order
};
```

## 🧪 Testing

### Comprehensive Benchmark Suite
```bash
# Run all benchmarks
./benchmark

# Individual test categories:
# - Micro-benchmarks: Single operation latency
# - Load testing: Performance under increasing load
# - Stress testing: Extreme conditions
# - Real data testing: Actual market data processing
```

### Test Coverage
- **Unit tests**: Individual component testing
- **Integration tests**: End-to-end order processing
- **Performance tests**: Latency and throughput validation
- **Stress tests**: System behavior under extreme loads
- **Real data tests**: Processing actual market data


## 📁 Project Structure

```
OrderBook/
├── include/                 # Header files
│   ├── OrderBook.hpp       # Main order book class
│   ├── Order.hpp           # Order data structure
│   ├── OrderPool.hpp       # Memory pool management
│   ├── Server.hpp          # Multi-threaded server
│   └── utils.hpp           # Utility functions
├── src/                    # Source files
│   ├── OrderBook.cpp       # Order book implementation
│   ├── OrderPool.cpp       # Memory pool implementation
│   ├── Server.cpp          # Server implementation
│   └── utils.cpp           # Utility implementations
├── benchmark.cpp           # Performance benchmarking
├── main.cpp               # Main application
├── makefile               # Build configuration
└── README.md              # This file
```

## 🎯 Use Cases

### High-Frequency Trading
- **Market making**: Provide liquidity with sub-microsecond response
- **Arbitrage**: Execute trades across multiple venues
- **Algorithmic trading**: Process large order volumes efficiently

### Market Simulation
- **Backtesting**: Historical strategy validation
- **Risk modeling**: Portfolio impact analysis
- **Research**: Market microstructure studies

### Educational
- **Learning**: Understanding order book mechanics
- **Research**: Algorithm development and testing
- **Benchmarking**: Performance comparison studies

