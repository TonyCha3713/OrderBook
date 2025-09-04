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
    OrderBook() {
        orderIndex.reserve(INITIAL_CAPACITY);
        tradeLog.reserve(INITIAL_CAPACITY);
    }
    OrderPool orderPool;
    vector<Log> tradeLog;
    void addOrder(const Order& order);
    void cancelOrder(int orderId);
    void printBook(int depth = 5) const;
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

    inline void executeMatch(Order& incoming, Order& resting);
    void matchBids(Order& order);
    void matchAsks(Order& order);

};