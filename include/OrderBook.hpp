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
    OrderBook();
    OrderPool orderPool;
    vector<Log> tradeLog;
    void addOrder(const Order& order);
    void cancelOrder(int orderId);
    void printBook(int depth = 5) const;

private:
    map<double, vector<Order>, greater<>> bids;
    map<double, vector<Order>> asks;
    unordered_map<int, Order*> orderIndex;

    inline void executeMatch(Order& incoming, Order& resting);
    void matchBids(Order& order);
    void matchAsks(Order& order);

};