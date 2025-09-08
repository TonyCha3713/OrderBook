#include "./include/OrderBook.hpp"
#include "./include/utils.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace std;

// Simple micro-benchmark for specific scenarios
class MicroBenchmark {
private:
    mt19937 rng;
    
public:
    MicroBenchmark() : rng(chrono::steady_clock::now().time_since_epoch().count()) {}
    
    inline double getCurrentTimeUs() {
        auto now = chrono::high_resolution_clock::now();
        return chrono::duration<double, micro>(now.time_since_epoch()).count();
    }
    
    // Test single order latency
    void testSingleOrderLatency() {
        cout << "=== Single Order Latency Test ===" << endl;
        
        OrderBook book;
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
            book.addOrder(order);
            double end = getCurrentTimeUs();
            
            latencies.push_back(end - start);
        }
        
        // Calculate statistics
        sort(latencies.begin(), latencies.end());
        double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double p50 = latencies[static_cast<size_t>(latencies.size() * 0.5)];
        double p95 = latencies[static_cast<size_t>(latencies.size() * 0.95)];
        double p99 = latencies[static_cast<size_t>(latencies.size() * 0.99)];
        double min = latencies[0];
        double max = latencies.back();
        
        cout << "Iterations: " << iterations << endl;
        cout << "Average: " << fixed << setprecision(2) << avg << " μs" << endl;
        cout << "Min: " << fixed << setprecision(2) << min << " μs" << endl;
        cout << "Max: " << fixed << setprecision(2) << max << " μs" << endl;
        cout << "P50: " << fixed << setprecision(2) << p50 << " μs" << endl;
        cout << "P95: " << fixed << setprecision(2) << p95 << " μs" << endl;
        cout << "P99: " << fixed << setprecision(2) << p99 << " μs" << endl;
        cout << "700μs Target: " << (avg < 700.0 ? "✅ PASSED" : "❌ FAILED") << endl;
        cout << endl;
    }
    
    // Test order type comparison
    void testOrderTypeComparison() {
        cout << "=== Order Type Performance Comparison ===" << endl;
        
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
            OrderBook book;
            vector<double> latencies;
            
            uniform_real_distribution<double> priceDist(100.0, 110.0);
            uniform_int_distribution<int> qtyDist(1, 100);
            
            for (int j = 0; j < iterations; ++j) {
                // Market orders must have zero price
                double price = (types[i] == OrderType::Market) ? 0.0 : priceDist(rng);
                int quantity = qtyDist(rng);
                OrderSide side = (j % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
                
                Order order(j + 1, types[i], side, price, quantity, getCurrentTimeUs());
                
                double start = getCurrentTimeUs();
                book.addOrder(order);
                double end = getCurrentTimeUs();
                
                latencies.push_back(end - start);
            }
            
            double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
            auto minMax = minmax_element(latencies.begin(), latencies.end());
            
            cout << typeNames[i] << ": "
                 << "avg=" << fixed << setprecision(2) << avg << "μs, "
                 << "min=" << fixed << setprecision(2) << *minMax.first << "μs, "
                 << "max=" << fixed << setprecision(2) << *minMax.second << "μs, "
                 << "trades=" << static_cast<int>(book.tradeLog.size()) << endl;
        }
        cout << endl;
    }
    
    // Test matching performance
    void testMatchingPerformance() {
        cout << "=== Order Matching Performance Test ===" << endl;
        
        OrderBook book;
        const int orders = 5000;
        vector<double> latencies;
        
        uniform_real_distribution<double> priceDist(100.0, 102.0);  // Tight spread for more matches
        uniform_int_distribution<int> qtyDist(1, 50);
        
        for (int i = 0; i < orders; ++i) {
            double price = priceDist(rng);
            int quantity = qtyDist(rng);
            OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
            
            Order order(i + 1, OrderType::GoodTillCancel, side, price, quantity, getCurrentTimeUs());
            
            double start = getCurrentTimeUs();
            book.addOrder(order);
            double end = getCurrentTimeUs();
            
            latencies.push_back(end - start);
        }
        
        double avg = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        auto minMax = minmax_element(latencies.begin(), latencies.end());
        
        cout << "Orders processed: " << orders << endl;
        cout << "Trades executed: " << static_cast<int>(book.tradeLog.size()) << endl;
        cout << "Match rate: " << fixed << setprecision(2) 
             << (static_cast<double>(book.tradeLog.size()) / orders * 100.0) << "%" << endl;
        cout << "Average latency: " << fixed << setprecision(2) << avg << " μs" << endl;
        cout << "Min latency: " << fixed << setprecision(2) << *minMax.first << " μs" << endl;
        cout << "Max latency: " << fixed << setprecision(2) << *minMax.second << " μs" << endl;
        cout << endl;
    }
    
    void runAllTests() {
        cout << "=== Micro-Benchmark Suite ===" << endl;
        cout << "Testing specific performance scenarios..." << endl << endl;
        
        testSingleOrderLatency();
        testOrderTypeComparison();
        testMatchingPerformance();
    }
};

int main() {
    MicroBenchmark benchmark;
    benchmark.runAllTests();
    return 0;
}
