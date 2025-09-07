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

private:
    OrderBook book;
    std::queue<OrderRequest> orderQueue;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> nextOrderId;
    std::atomic<bool> running;

    void processNextOrder();
};