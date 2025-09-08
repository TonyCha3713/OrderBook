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
    };
    
    vector<BenchmarkResult> results;

public:
    PerformanceBenchmark() : rng(chrono::steady_clock::now().time_since_epoch().count()) {}
    
    // High-resolution timing utility
    inline double getCurrentTimeUs() {
        auto now = chrono::high_resolution_clock::now();
        return chrono::duration<double, micro>(now.time_since_epoch()).count();
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
        
        BenchmarkResult result = {
            testName,
            orderCount,
            totalTime / 1000.0,  // Convert to ms
            avgLatency,
            *minMax.first,
            *minMax.second,
            tradesExecuted,
            (orderCount * 1000000.0) / totalTime  // Orders per second
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
        
        BenchmarkResult result = {
            "Depth Impact Test",
            additionalOrders,
            totalTime / 1000.0,
            avgLatency,
            *minMax.first,
            *minMax.second,
            0,  // We're not counting trades for this test
            (additionalOrders * 1000000.0) / totalTime
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
        
        BenchmarkResult result = {
            "Real Market Data",
            orderCount,
            totalTime / 1000.0,
            avgLatency,
            *minMax.first,
            *minMax.second,
            static_cast<int>(book.tradeLog.size()),
            (orderCount * 1000000.0) / totalTime
        };
        
        results.push_back(result);
        return result;
    }
    
    // Run comprehensive benchmark suite
    void runBenchmarkSuite() {
        cout << "=== OrderBook Performance Benchmark Suite ===" << endl;
        cout << "Testing various order types and scenarios..." << endl << endl;
        
        // Clear any existing state
        book = OrderBook();
        allTrades.clear();
        results.clear();
        
        // Test different order types
        benchmarkOrderType(OrderType::GoodTillCancel, 1000, "GTC Orders");
        benchmarkOrderType(OrderType::FillOrKill, 1000, "FOK Orders");
        benchmarkOrderType(OrderType::FillAndKill, 1000, "FAK Orders");
        benchmarkOrderType(OrderType::GoodForDay, 1000, "GTD Orders");
        benchmarkOrderType(OrderType::Market, 1000, "Market Orders");
        
        // Test depth impact
        benchmarkDepthImpact(5000, 1000);
        
        // Test with real market data
        benchmarkRealData("dataset/AAPL_cleaned.csv", 5000);
        
        // Generate report
        generateReport();
    }
    
    // Generate comprehensive performance report
    void generateReport() {
        cout << "\n=== PERFORMANCE BENCHMARK REPORT ===" << endl;
        cout << setfill('=') << setw(120) << "" << setfill(' ') << endl;
        
        cout << left << setw(20) << "Test Name"
             << setw(12) << "Orders"
             << setw(12) << "Total(ms)"
             << setw(12) << "Avg(μs)"
             << setw(12) << "Min(μs)"
             << setw(12) << "Max(μs)"
             << setw(12) << "Trades"
             << setw(15) << "Throughput(ops/s)" << endl;
        
        cout << setfill('-') << setw(120) << "" << setfill(' ') << endl;
        
        for (const auto& result : results) {
            cout << left << setw(20) << result.testName
                 << setw(12) << result.orderCount
                 << setw(12) << fixed << setprecision(2) << result.totalTimeMs
                 << setw(12) << fixed << setprecision(2) << result.avgLatencyUs
                 << setw(12) << fixed << setprecision(2) << result.minLatencyUs
                 << setw(12) << fixed << setprecision(2) << result.maxLatencyUs
                 << setw(12) << result.tradesExecuted
                 << setw(15) << fixed << setprecision(0) << result.throughputOrdersPerSec << endl;
        }
        
        cout << setfill('=') << setw(120) << "" << setfill(' ') << endl;
        
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
            [](const BenchmarkResult& r) { return r.avgLatencyUs < 700.0; });
        
        cout << "700μs Target Achievement: " << (meetsTarget ? "✅ PASSED" : "❌ FAILED") << endl;
        
        // Save detailed report to file
        saveReportToFile("benchmark_report.csv");
    }
    
    void saveReportToFile(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to create report file: " << filename << endl;
            return;
        }
        
        file << "TestName,OrderCount,TotalTimeMs,AvgLatencyUs,MinLatencyUs,MaxLatencyUs,TradesExecuted,ThroughputOpsPerSec\n";
        
        for (const auto& result : results) {
            file << result.testName << ","
                 << result.orderCount << ","
                 << result.totalTimeMs << ","
                 << result.avgLatencyUs << ","
                 << result.minLatencyUs << ","
                 << result.maxLatencyUs << ","
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
