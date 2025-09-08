#pragma once

#include "Order.hpp"
#include "OrderPool.hpp"
#include "Log.hpp"
#include <map>
#include <deque>
#include <unordered_map>
#include <vector>
#include <optional>

using namespace std;

class OrderBook {
public:
    static constexpr double INVALID_PRICE = -1.0;
    static constexpr size_t INITIAL_CAPACITY = 1024;
    static constexpr size_t PRICE_LEVEL_CAPACITY = 64;  // Pre-allocate price levels
    
    OrderBook() {
        orderIndex.reserve(INITIAL_CAPACITY);
        tradeLog.reserve(INITIAL_CAPACITY);
        // Note: std::map doesn't support reserve(), but we can optimize the vectors inside
    }
    OrderPool orderPool;
    vector<Log> tradeLog;
    void addOrder(const Order& order);
    void cancelOrder(int orderId);
    void printBook(int depth = 5) const;
    void saveOrdersToFile(const std::string& filename) const;
    void loadOrdersFromFile(const std::string& filename);
    double getBestBid() const {
        return !bids.empty() ? bids.rbegin()->first : INVALID_PRICE;
    }

    double getBestAsk() const {
        return !asks.empty() ? asks.begin()->first : INVALID_PRICE;
    }

    bool hasBids() const { return !bids.empty(); }
    bool hasAsks() const { return !asks.empty(); }

private:
    using PriceMap = map<double, vector<Order>>;
    PriceMap bids;
    PriceMap asks;
    unordered_map<int, Order*> orderIndex;
    
    // Alternative: unordered_map for better performance (but loses price ordering)
    // using PriceMap = unordered_map<double, vector<Order>>;

    inline void executeMatch(Order& incoming, Order& resting) __attribute__((always_inline));
    inline bool canFillOrder(const Order& order) const __attribute__((always_inline));
    inline bool isOrderExpired(const Order& order) const __attribute__((always_inline));
    void removeExpiredOrders(PriceMap& side);
    void matchBids(Order& order);
    void matchAsks(Order& order);

};