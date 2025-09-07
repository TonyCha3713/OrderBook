#pragma once
#include "OrderBook.hpp"
#include "utils.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class OrderServer {
public:
    struct OrderRequest {
        OrderType type;     // Use existing OrderType enum
        OrderSide side;
        double price;
        int quantity;
    };

    OrderServer() : nextOrderId(1), running(true) {}
    
    void start() {
        while (running) {
            processNextOrder();
        }
    }

    void stop() { running = false; cv.notify_one(); }

    // Client interface methods
    void submitOrder(OrderType type, OrderSide side, double price, int quantity) {
        std::lock_guard<std::mutex> lock(mutex);
        orderQueue.push({type, side, price, quantity});
        cv.notify_one();
    }

    // Order book persistence methods
    void saveOrderBook(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex);
        book.saveOrdersToFile(filename);
    }

    void loadOrderBook(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex);
        book.loadOrdersFromFile(filename);
    }

    void exportTrades(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex);
        exportTradesToCSV(book.tradeLog, filename);
    }

private:
    OrderBook book;
    std::queue<OrderRequest> orderQueue;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> nextOrderId;
    std::atomic<bool> running;

    void processNextOrder();
};