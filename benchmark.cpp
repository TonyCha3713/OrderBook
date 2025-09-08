#include "./include/OrderBook.hpp"
#include "./include/utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <algorithm>
#include <map>

using namespace std;

class PerformanceBenchmark {
private:
    OrderBook book;
    vector<Log> allTrades;
    mt19937 rng;
    
    // Performance metrics
    struct BenchmarkResult {
        string testName;
        int orderCount;
        double totalTimeMs;
        double avgLatencyUs;
        double minLatencyUs;
        double maxLatencyUs;
        int tradesExecuted;
        double throughputOrdersPerSec;
        double p50Us;
        double p95Us;
        double p99Us;
        double stdDevUs;
    };
    
    vector<BenchmarkResult> results;

public:
    PerformanceBenchmark() : rng(chrono::steady_clock::now().time_since_epoch().count()) {}
    
    // High-resolution timing utility
    inline double getCurrentTimeUs() {
        auto now = chrono::high_resolution_clock::now();
        return chrono::duration<double, micro>(now.time_since_epoch()).count();
    }
    
    // Calculate percentiles and statistics from latency data
    void calculateStatistics(const vector<double>& latencies, double& p50, double& p95, double& p99, double& stdDev) {
        if (latencies.empty()) {
            p50 = p95 = p99 = stdDev = 0.0;
            return;
        }
        
        vector<double> sortedLatencies = latencies;
        sort(sortedLatencies.begin(), sortedLatencies.end());
        
        p50 = sortedLatencies[static_cast<size_t>(sortedLatencies.size() * 0.5)];
        p95 = sortedLatencies[static_cast<size_t>(sortedLatencies.size() * 0.95)];
        p99 = sortedLatencies[static_cast<size_t>(sortedLatencies.size() * 0.99)];
        
        // Calculate standard deviation
        double mean = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double variance = 0.0;
        for (double latency : latencies) {
            variance += (latency - mean) * (latency - mean);
        }
        stdDev = sqrt(variance / latencies.size());
    }
    
    // Generate random orders for testing
    vector<Order> generateRandomOrders(int count, OrderType type, double priceRange = 10.0) {
        vector<Order> orders;
        uniform_real_distribution<double> priceDist(100.0, 100.0 + priceRange);
        uniform_int_distribution<int> qtyDist(1, 1000);
        uniform_int_distribution<int> sideDist(0, 1);
        
        for (int i = 0; i < count; ++i) {
            // Market orders must have zero price
            double price = (type == OrderType::Market) ? 0.0 : priceDist(rng);
            int quantity = qtyDist(rng);
            OrderSide side = static_cast<OrderSide>(sideDist(rng));
            int64_t timestamp = static_cast<int64_t>(getCurrentTimeUs());
            
            orders.emplace_back(i + 1, type, side, price, quantity, timestamp);
        }
        return orders;
    }
    
    // Micro-benchmark: Single order latency test
    BenchmarkResult testSingleOrderLatency() {
        cout << "=== MICRO-BENCHMARK: Single Order Latency Test ===" << endl;
        
        OrderBook testBook;
        vector<double> latencies;
        const int iterations = 10000;
        
        uniform_real_distribution<double> priceDist(100.0, 110.0);
        uniform_int_distribution<int> qtyDist(1, 100);
        
        for (int i = 0; i < iterations; ++i) {
            double price = priceDist(rng);
            int quantity = qtyDist(rng);
            OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
            
            Order order(i + 1, OrderType::GoodTillCancel, side, price, quantity, getCurrentTimeUs());
            
            double start = getCurrentTimeUs();
            testBook.addOrder(order);
            double end = getCurrentTimeUs();
            
            latencies.push_back(end - start);
        }
        
        // Calculate statistics
        double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        auto minMax = minmax_element(latencies.begin(), latencies.end());
        double p50, p95, p99, stdDev;
        calculateStatistics(latencies, p50, p95, p99, stdDev);
        
        cout << "Iterations: " << iterations << endl;
        cout << "Average: " << fixed << setprecision(2) << avg << " μs" << endl;
        cout << "Min: " << fixed << setprecision(2) << *minMax.first << " μs" << endl;
        cout << "Max: " << fixed << setprecision(2) << *minMax.second << " μs" << endl;
        cout << "P50: " << fixed << setprecision(2) << p50 << " μs" << endl;
        cout << "P95: " << fixed << setprecision(2) << p95 << " μs" << endl;
        cout << "P99: " << fixed << setprecision(2) << p99 << " μs" << endl;
        cout << "StdDev: " << fixed << setprecision(2) << stdDev << " μs" << endl;
        cout << "10μs Target: " << (avg < 10.0 ? "✅ PASSED" : "❌ FAILED") << endl;
        cout << endl;
        
        BenchmarkResult result = {
            "Single Order Latency",
            iterations,
            0.0, // Will be calculated properly
            avg,
            *minMax.first,
            *minMax.second,
            static_cast<int>(testBook.tradeLog.size()),
            0.0, // Will be calculated properly
            p50,
            p95,
            p99,
            stdDev
        };
        
        results.push_back(result);
        return result;
    }
    
    // Micro-benchmark: Order type comparison
    void testOrderTypeComparison() {
        cout << "=== MICRO-BENCHMARK: Order Type Performance Comparison ===" << endl;
        
        vector<OrderType> types = {
            OrderType::GoodTillCancel,
            OrderType::FillOrKill,
            OrderType::FillAndKill,
            OrderType::GoodForDay,
            OrderType::Market
        };
        
        vector<string> typeNames = {
            "GTC", "FOK", "FAK", "GTD", "Market"
        };
        
        const int iterations = 1000;
        
        for (size_t i = 0; i < types.size(); ++i) {
            OrderBook testBook;
            vector<double> latencies;
            
            uniform_real_distribution<double> priceDist(100.0, 110.0);
            uniform_int_distribution<int> qtyDist(1, 100);
            
            double startTime = getCurrentTimeUs();
            
            for (int j = 0; j < iterations; ++j) {
                // Market orders must have zero price
                double price = (types[i] == OrderType::Market) ? 0.0 : priceDist(rng);
                int quantity = qtyDist(rng);
                OrderSide side = (j % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
                
                Order order(j + 1, types[i], side, price, quantity, getCurrentTimeUs());
                
                double orderStart = getCurrentTimeUs();
                testBook.addOrder(order);
                double orderEnd = getCurrentTimeUs();
                
                latencies.push_back(orderEnd - orderStart);
            }
            
            double endTime = getCurrentTimeUs();
            double totalTime = endTime - startTime;
            
            double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
            auto minMax = minmax_element(latencies.begin(), latencies.end());
            double p50, p95, p99, stdDev;
            calculateStatistics(latencies, p50, p95, p99, stdDev);
            
            cout << typeNames[i] << ": "
                 << "avg=" << fixed << setprecision(2) << avg << "μs, "
                 << "min=" << fixed << setprecision(2) << *minMax.first << "μs, "
                 << "max=" << fixed << setprecision(2) << *minMax.second << "μs, "
                 << "trades=" << static_cast<int>(testBook.tradeLog.size()) << endl;
            
            BenchmarkResult result = {
                typeNames[i] + " Micro-Benchmark",
                iterations,
                totalTime / 1000.0,
                avg,
                *minMax.first,
                *minMax.second,
                static_cast<int>(testBook.tradeLog.size()),
                (iterations * 1000000.0) / totalTime,
                p50,
                p95,
                p99,
                stdDev
            };
            
            results.push_back(result);
        }
        cout << endl;
    }
    
    // Micro-benchmark: Matching performance test
    BenchmarkResult testMatchingPerformance() {
        cout << "=== MICRO-BENCHMARK: Order Matching Performance Test ===" << endl;
        
        OrderBook testBook;
        const int orders = 5000;
        vector<double> latencies;
        
        uniform_real_distribution<double> priceDist(100.0, 102.0);  // Tight spread for more matches
        uniform_int_distribution<int> qtyDist(1, 50);
        
        double startTime = getCurrentTimeUs();
        
        for (int i = 0; i < orders; ++i) {
            double price = priceDist(rng);
            int quantity = qtyDist(rng);
            OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
            
            Order order(i + 1, OrderType::GoodTillCancel, side, price, quantity, getCurrentTimeUs());
            
            double orderStart = getCurrentTimeUs();
            testBook.addOrder(order);
            double orderEnd = getCurrentTimeUs();
            
            latencies.push_back(orderEnd - orderStart);
        }
        
        double endTime = getCurrentTimeUs();
        double totalTime = endTime - startTime;
        
        double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        auto minMax = minmax_element(latencies.begin(), latencies.end());
        double p50, p95, p99, stdDev;
        calculateStatistics(latencies, p50, p95, p99, stdDev);
        
        cout << "Orders processed: " << orders << endl;
        cout << "Trades executed: " << static_cast<int>(testBook.tradeLog.size()) << endl;
        cout << "Match rate: " << fixed << setprecision(2) 
             << (static_cast<double>(testBook.tradeLog.size()) / orders * 100.0) << "%" << endl;
        cout << "Average latency: " << fixed << setprecision(2) << avg << " μs" << endl;
        cout << "Min latency: " << fixed << setprecision(2) << *minMax.first << " μs" << endl;
        cout << "Max latency: " << fixed << setprecision(2) << *minMax.second << " μs" << endl;
        cout << "P99 latency: " << fixed << setprecision(2) << p99 << " μs" << endl;
        cout << "Latency jitter (std dev): " << fixed << setprecision(2) << stdDev << " μs" << endl;
        cout << endl;
        
        BenchmarkResult result = {
            "Matching Performance",
            orders,
            totalTime / 1000.0,
            avg,
            *minMax.first,
            *minMax.second,
            static_cast<int>(testBook.tradeLog.size()),
            (orders * 1000000.0) / totalTime,
            p50,
            p95,
            p99,
            stdDev
        };
        
        results.push_back(result);
        return result;
    }
    
    // Load testing: Test performance under increasing load
    void testLoadScaling() {
        cout << "=== LOAD TESTING: Performance Under Increasing Load ===" << endl;
        
        vector<int> loadLevels = {100, 500, 1000, 5000, 10000, 25000, 50000};
        
        for (int load : loadLevels) {
            cout << "Testing load: " << load << " orders..." << endl;
            
            OrderBook testBook;
            vector<double> latencies;
            latencies.reserve(load);
            
            auto orders = generateRandomOrders(load, OrderType::GoodTillCancel, 5.0);
            
            double startTime = getCurrentTimeUs();
            
            for (const auto& order : orders) {
                double orderStart = getCurrentTimeUs();
                testBook.addOrder(order);
                double orderEnd = getCurrentTimeUs();
                latencies.push_back(orderEnd - orderStart);
            }
            
            double endTime = getCurrentTimeUs();
            double totalTime = endTime - startTime;
            
            double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
            auto minMax = minmax_element(latencies.begin(), latencies.end());
            double p50, p95, p99, stdDev;
            calculateStatistics(latencies, p50, p95, p99, stdDev);
            
            cout << "  Load " << load << ": avg=" << fixed << setprecision(2) << avg << "μs, "
                 << "throughput=" << fixed << setprecision(0) << (load * 1000000.0) / totalTime << " ops/s, "
                 << "trades=" << static_cast<int>(testBook.tradeLog.size()) << endl;
            
            BenchmarkResult result = {
                "Load Test " + to_string(load),
                load,
                totalTime / 1000.0,
                avg,
                *minMax.first,
                *minMax.second,
                static_cast<int>(testBook.tradeLog.size()),
                (load * 1000000.0) / totalTime,
                p50,
                p95,
                p99,
                stdDev
            };
            
            results.push_back(result);
        }
        cout << endl;
    }
    
    // Stress testing: Test system behavior under extreme conditions
    void testStressConditions() {
        cout << "=== STRESS TESTING: Extreme Conditions ===" << endl;
        
        // Stress test 1: Massive order book
        cout << "Stress Test 1: Massive Order Book (100K orders)..." << endl;
        {
            OrderBook testBook;
            vector<double> latencies;
            const int stressLoad = 100000;
            latencies.reserve(stressLoad);
            
            auto orders = generateRandomOrders(stressLoad, OrderType::GoodTillCancel, 20.0);
            
            double startTime = getCurrentTimeUs();
            
            for (const auto& order : orders) {
                double orderStart = getCurrentTimeUs();
                testBook.addOrder(order);
                double orderEnd = getCurrentTimeUs();
                latencies.push_back(orderEnd - orderStart);
            }
            
            double endTime = getCurrentTimeUs();
            double totalTime = endTime - startTime;
            
            double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
            auto minMax = minmax_element(latencies.begin(), latencies.end());
            double p50, p95, p99, stdDev;
            calculateStatistics(latencies, p50, p95, p99, stdDev);
            
            cout << "  Massive Book: avg=" << fixed << setprecision(2) << avg << "μs, "
                 << "throughput=" << fixed << setprecision(0) << (stressLoad * 1000000.0) / totalTime << " ops/s" << endl;
            
            BenchmarkResult result = {
                "Stress Test - Massive Book",
                stressLoad,
                totalTime / 1000.0,
                avg,
                *minMax.first,
                *minMax.second,
                static_cast<int>(testBook.tradeLog.size()),
                (stressLoad * 1000000.0) / totalTime,
                p50,
                p95,
                p99,
                stdDev
            };
            
            results.push_back(result);
        }
        
        // Stress test 2: High-frequency market orders
        cout << "Stress Test 2: High-Frequency Market Orders (50K orders)..." << endl;
        {
            OrderBook testBook;
            vector<double> latencies;
            const int stressLoad = 50000;
            latencies.reserve(stressLoad);
            
            auto orders = generateRandomOrders(stressLoad, OrderType::Market, 0.0);
            
            double startTime = getCurrentTimeUs();
            
            for (const auto& order : orders) {
                double orderStart = getCurrentTimeUs();
                testBook.addOrder(order);
                double orderEnd = getCurrentTimeUs();
                latencies.push_back(orderEnd - orderStart);
            }
            
            double endTime = getCurrentTimeUs();
            double totalTime = endTime - startTime;
            
            double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
            auto minMax = minmax_element(latencies.begin(), latencies.end());
            double p50, p95, p99, stdDev;
            calculateStatistics(latencies, p50, p95, p99, stdDev);
            
            cout << "  Market Orders: avg=" << fixed << setprecision(2) << avg << "μs, "
                 << "throughput=" << fixed << setprecision(0) << (stressLoad * 1000000.0) / totalTime << " ops/s, "
                 << "trades=" << static_cast<int>(testBook.tradeLog.size()) << endl;
            
            BenchmarkResult result = {
                "Stress Test - Market Orders",
                stressLoad,
                totalTime / 1000.0,
                avg,
                *minMax.first,
                *minMax.second,
                static_cast<int>(testBook.tradeLog.size()),
                (stressLoad * 1000000.0) / totalTime,
                p50,
                p95,
                p99,
                stdDev
            };
            
            results.push_back(result);
        }
        
        // Stress test 3: Mixed order types under load
        cout << "Stress Test 3: Mixed Order Types (25K orders)..." << endl;
        {
            OrderBook testBook;
            vector<double> latencies;
            const int stressLoad = 25000;
            latencies.reserve(stressLoad);
            
            vector<OrderType> types = {OrderType::GoodTillCancel, OrderType::FillOrKill, 
                                      OrderType::FillAndKill, OrderType::Market};
            uniform_int_distribution<int> typeDist(0, types.size() - 1);
            
            double startTime = getCurrentTimeUs();
            
            for (int i = 0; i < stressLoad; ++i) {
                OrderType type = types[typeDist(rng)];
                double price = (type == OrderType::Market) ? 0.0 : (100.0 + (i % 100) * 0.01);
                int quantity = 1 + (i % 1000);
                OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
                
                Order order(i + 1, type, side, price, quantity, getCurrentTimeUs());
                
                double orderStart = getCurrentTimeUs();
                testBook.addOrder(order);
                double orderEnd = getCurrentTimeUs();
                latencies.push_back(orderEnd - orderStart);
            }
            
            double endTime = getCurrentTimeUs();
            double totalTime = endTime - startTime;
            
            double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
            auto minMax = minmax_element(latencies.begin(), latencies.end());
            double p50, p95, p99, stdDev;
            calculateStatistics(latencies, p50, p95, p99, stdDev);
            
            cout << "  Mixed Types: avg=" << fixed << setprecision(2) << avg << "μs, "
                 << "throughput=" << fixed << setprecision(0) << (stressLoad * 1000000.0) / totalTime << " ops/s, "
                 << "trades=" << static_cast<int>(testBook.tradeLog.size()) << endl;
            
            BenchmarkResult result = {
                "Stress Test - Mixed Types",
                stressLoad,
                totalTime / 1000.0,
                avg,
                *minMax.first,
                *minMax.second,
                static_cast<int>(testBook.tradeLog.size()),
                (stressLoad * 1000000.0) / totalTime,
                p50,
                p95,
                p99,
                stdDev
            };
            
            results.push_back(result);
        }
        cout << endl;
    }
    
    // Benchmark individual order type performance
    BenchmarkResult benchmarkOrderType(OrderType type, int orderCount, const string& testName) {
        cout << "Benchmarking " << testName << " with " << orderCount << " orders..." << endl;
        
        auto orders = generateRandomOrders(orderCount, type);
        vector<double> latencies;
        latencies.reserve(orderCount);
        
        double startTime = getCurrentTimeUs();
        
        for (const auto& order : orders) {
            double orderStart = getCurrentTimeUs();
            book.addOrder(order);
            double orderEnd = getCurrentTimeUs();
            latencies.push_back(orderEnd - orderStart);
        }
        
        double endTime = getCurrentTimeUs();
        double totalTime = endTime - startTime;
        
        // Collect trade statistics
        int tradesExecuted = static_cast<int>(book.tradeLog.size()) - static_cast<int>(allTrades.size());
        allTrades.insert(allTrades.end(), book.tradeLog.begin() + allTrades.size(), book.tradeLog.end());
        
        // Calculate statistics
        auto minMax = minmax_element(latencies.begin(), latencies.end());
        double avgLatency = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double p50, p95, p99, stdDev;
        calculateStatistics(latencies, p50, p95, p99, stdDev);
        
        BenchmarkResult result = {
            testName,
            orderCount,
            totalTime / 1000.0,  // Convert to ms
            avgLatency,
            *minMax.first,
            *minMax.second,
            tradesExecuted,
            (orderCount * 1000000.0) / totalTime,  // Orders per second
            p50,
            p95,
            p99,
            stdDev
        };
        
        results.push_back(result);
        return result;
    }
    
    // Benchmark order book depth impact
    BenchmarkResult benchmarkDepthImpact(int baseOrders, int additionalOrders) {
        cout << "Benchmarking depth impact: " << baseOrders << " base + " << additionalOrders << " additional orders..." << endl;
        
        // First, create a deep order book
        auto baseOrdersList = generateRandomOrders(baseOrders, OrderType::GoodTillCancel, 5.0);
        for (const auto& order : baseOrdersList) {
            book.addOrder(order);
        }
        
        // Now benchmark adding more orders
        auto testOrders = generateRandomOrders(additionalOrders, OrderType::GoodTillCancel, 5.0);
        vector<double> latencies;
        latencies.reserve(additionalOrders);
        
        double startTime = getCurrentTimeUs();
        
        for (const auto& order : testOrders) {
            double orderStart = getCurrentTimeUs();
            book.addOrder(order);
            double orderEnd = getCurrentTimeUs();
            latencies.push_back(orderEnd - orderStart);
        }
        
        double endTime = getCurrentTimeUs();
        double totalTime = endTime - startTime;
        
        auto minMax = minmax_element(latencies.begin(), latencies.end());
        double avgLatency = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double p50, p95, p99, stdDev;
        calculateStatistics(latencies, p50, p95, p99, stdDev);
        
        BenchmarkResult result = {
            "Depth Impact Test",
            additionalOrders,
            totalTime / 1000.0,
            avgLatency,
            *minMax.first,
            *minMax.second,
            0,  // We're not counting trades for this test
            (additionalOrders * 1000000.0) / totalTime,
            p50,
            p95,
            p99,
            stdDev
        };
        
        results.push_back(result);
        return result;
    }
    
    // Benchmark real market data
    BenchmarkResult benchmarkRealData(const string& filename, int maxOrders = 10000) {
        cout << "Benchmarking with real market data from " << filename << "..." << endl;
        
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to open " << filename << endl;
            return {};
        }
        
        string line;
        getline(file, line); // Skip header if exists
        
        vector<double> latencies;
        int orderCount = 0;
        double startTime = getCurrentTimeUs();
        
        while (getline(file, line) && orderCount < maxOrders) {
            istringstream iss(line);
            string token;
            vector<string> tokens;
            
            while (getline(iss, token, ',')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() >= 6) {
                try {
                    // Parse LOBSTER format: time, type, id, size, price, direction
                    double price = stod(tokens[4]) / 10000.0;  // Convert from LOBSTER format
                    int quantity = stoi(tokens[3]);
                    OrderSide side = (stoi(tokens[5]) == 1) ? OrderSide::BUY : OrderSide::SELL;
                    OrderType type = OrderType::GoodTillCancel;  // Assume GTC for real data
                    
                    double orderStart = getCurrentTimeUs();
                    Order order(orderCount + 1, type, side, price, quantity, getCurrentTimeUs());
                    book.addOrder(order);
                    double orderEnd = getCurrentTimeUs();
                    latencies.push_back(orderEnd - orderStart);
                    
                    orderCount++;
                } catch (const exception& e) {
                    cerr << "Error parsing line: " << line << " - " << e.what() << endl;
                }
            }
        }
        
        file.close();
        double endTime = getCurrentTimeUs();
        double totalTime = endTime - startTime;
        
        auto minMax = minmax_element(latencies.begin(), latencies.end());
        double avgLatency = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double p50, p95, p99, stdDev;
        calculateStatistics(latencies, p50, p95, p99, stdDev);
        
        BenchmarkResult result = {
            "Real Market Data",
            orderCount,
            totalTime / 1000.0,
            avgLatency,
            *minMax.first,
            *minMax.second,
            static_cast<int>(book.tradeLog.size()),
            (orderCount * 1000000.0) / totalTime,
            p50,
            p95,
            p99,
            stdDev
        };
        
        results.push_back(result);
        return result;
    }
    
    // Run comprehensive benchmark suite
    void runBenchmarkSuite() {
        cout << "=== ENHANCED ORDERBOOK PERFORMANCE BENCHMARK SUITE ===" << endl;
        cout << "Testing micro-benchmarks, load testing, stress testing, and comprehensive scenarios..." << endl << endl;
        
        // Clear any existing state
        book = OrderBook();
        allTrades.clear();
        results.clear();
        
        // MICRO-BENCHMARKS
        cout << "🔬 MICRO-BENCHMARKS" << endl;
        cout << "===================" << endl;
        testSingleOrderLatency();
        testOrderTypeComparison();
        testMatchingPerformance();
        
        // LOAD TESTING
        cout << "📈 LOAD TESTING" << endl;
        cout << "===============" << endl;
        testLoadScaling();
        
        // STRESS TESTING
        cout << "💪 STRESS TESTING" << endl;
        cout << "=================" << endl;
        testStressConditions();
        
        // COMPREHENSIVE BENCHMARKS
        cout << "📊 COMPREHENSIVE BENCHMARKS" << endl;
        cout << "===========================" << endl;
        benchmarkOrderType(OrderType::GoodTillCancel, 1000, "GTC Orders");
        benchmarkOrderType(OrderType::FillOrKill, 1000, "FOK Orders");
        benchmarkOrderType(OrderType::FillAndKill, 1000, "FAK Orders");
        benchmarkOrderType(OrderType::GoodForDay, 1000, "GTD Orders");
        benchmarkOrderType(OrderType::Market, 1000, "Market Orders");
        
        // Test depth impact
        benchmarkDepthImpact(5000, 1000);
        
        // Test with real market data
        benchmarkRealData("dataset/AAPL_cleaned.csv", 5000);
        
        // Generate enhanced report
        generateEnhancedReport();
    }
    
    // Generate enhanced performance report
    void generateEnhancedReport() {
        cout << "\n=== ENHANCED PERFORMANCE BENCHMARK REPORT ===" << endl;
        cout << setfill('=') << setw(160) << "" << setfill(' ') << endl;
        
        cout << left << setw(25) << "Test Name"
             << setw(10) << "Orders"
             << setw(12) << "Total(ms)"
             << setw(10) << "Avg(μs)"
             << setw(10) << "Min(μs)"
             << setw(10) << "Max(μs)"
             << setw(8) << "P50(μs)"
             << setw(8) << "P95(μs)"
             << setw(8) << "P99(μs)"
             << setw(8) << "StdDev"
             << setw(10) << "Trades"
             << setw(15) << "Throughput(ops/s)" << endl;
        
        cout << setfill('-') << setw(160) << "" << setfill(' ') << endl;
        
        for (const auto& result : results) {
            cout << left << setw(25) << result.testName
                 << setw(10) << result.orderCount
                 << setw(12) << fixed << setprecision(2) << result.totalTimeMs
                 << setw(10) << fixed << setprecision(2) << result.avgLatencyUs
                 << setw(10) << fixed << setprecision(2) << result.minLatencyUs
                 << setw(10) << fixed << setprecision(2) << result.maxLatencyUs
                 << setw(8) << fixed << setprecision(2) << result.p50Us
                 << setw(8) << fixed << setprecision(2) << result.p95Us
                 << setw(8) << fixed << setprecision(2) << result.p99Us
                 << setw(8) << fixed << setprecision(2) << result.stdDevUs
                 << setw(10) << result.tradesExecuted
                 << setw(15) << fixed << setprecision(0) << result.throughputOrdersPerSec << endl;
        }
        
        cout << setfill('=') << setw(160) << "" << setfill(' ') << endl;
        
        // Analysis
        cout << "\n=== ANALYSIS ===" << endl;
        auto fastest = min_element(results.begin(), results.end(), 
            [](const BenchmarkResult& a, const BenchmarkResult& b) {
                return a.avgLatencyUs < b.avgLatencyUs;
            });
        
        auto slowest = max_element(results.begin(), results.end(), 
            [](const BenchmarkResult& a, const BenchmarkResult& b) {
                return a.avgLatencyUs < b.avgLatencyUs;
            });
        
        cout << "Fastest Order Type: " << fastest->testName 
             << " (avg: " << fixed << setprecision(2) << fastest->avgLatencyUs << " μs)" << endl;
        cout << "Slowest Order Type: " << slowest->testName 
             << " (avg: " << fixed << setprecision(2) << slowest->avgLatencyUs << " μs)" << endl;
        
        // Check against 700μs target
        bool meetsTarget = all_of(results.begin(), results.end(),
            [](const BenchmarkResult& r) { return r.avgLatencyUs < 10.0; });
        
        cout << "10μs Target Achievement: " << (meetsTarget ? "✅ PASSED" : "❌ FAILED") << endl;
        
        // Save detailed report to file
        saveReportToFile("benchmark_report.csv");
    }
    
    void saveReportToFile(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to create report file: " << filename << endl;
            return;
        }
        
        file << "TestName,OrderCount,TotalTimeMs,AvgLatencyUs,MinLatencyUs,MaxLatencyUs,P50Us,P95Us,P99Us,StdDevUs,TradesExecuted,ThroughputOpsPerSec\n";
        
        for (const auto& result : results) {
            file << result.testName << ","
                 << result.orderCount << ","
                 << result.totalTimeMs << ","
                 << result.avgLatencyUs << ","
                 << result.minLatencyUs << ","
                 << result.maxLatencyUs << ","
                 << result.p50Us << ","
                 << result.p95Us << ","
                 << result.p99Us << ","
                 << result.stdDevUs << ","
                 << result.tradesExecuted << ","
                 << result.throughputOrdersPerSec << "\n";
        }
        
        file.close();
        cout << "Detailed report saved to: " << filename << endl;
    }
};

int main() {
    PerformanceBenchmark benchmark;
    benchmark.runBenchmarkSuite();
    return 0;
}
