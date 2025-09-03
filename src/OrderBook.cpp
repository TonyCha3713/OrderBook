#include "../include/OrderBook.hpp"
#include "../include/utils.hpp"
#include "../include/Log.hpp"
#include <iostream>
#include <algorithm>
#include <queue>
#include <iomanip>
#include <deque>
#include <map>
#include <sstream>

using namespace std;

OrderBook::OrderBook() {
    cout << "Orderbook created.\n";
}

void OrderBook::addOrder(const Order& order) {
    Order incoming = order;
    if (incoming.side == OrderSide::BUY) matchAsks(incoming);
    else if (incoming.side == OrderSide::SELL) matchBids(incoming);
    if (incoming.remaining > 0) {
        if (incoming.type == OrderType::GoodForDay || incoming.type == OrderType::GoodTillCancel) {
            if (incoming.side == OrderSide::BUY) {
                if (bids[incoming.price].empty()) bids[incoming.price].reserve(32);
                bids[incoming.price].push_back(incoming);
                orderIndex[incoming.id] = &bids[incoming.price].back();
            }
            else {
                if (asks[incoming.price].empty()) asks[incoming.price].reserve(32);
                asks[incoming.price].push_back(incoming);
                orderIndex[incoming.id] = &asks[incoming.price].back();
            }
            cout << "Order " << incoming.id << " added to book.\n";
        }
    }    
}

void OrderBook::cancelOrder(int orderId) {
    if (orderIndex.find(orderId) == orderIndex.end()) {
        cout << "Cancel failed: Order ID " << orderId << " not found.\n";
        return;
    }

    Order* order = orderIndex[orderId];
    vector<Order>* dq = nullptr;
    if (order->side == OrderSide::BUY && bids.count(order->price)) dq = &bids[order->price];
    else if (order->side == OrderSide::SELL && asks.count(order->price)) dq = &asks[order->price];

    if (!dq) {
        cout << "Cancel failed: Order ID " << orderId << " not found in book.\n";
        return;
    }

    dq->erase(remove_if(dq->begin(), dq->end(), [orderId](const Order& o) { return o.id == orderId; }), dq->end());

    orderIndex.erase(orderId);
    cout << "Order " << orderId << " canceled.\n";

    if (dq->empty()) {
        if (order->side == OrderSide::BUY) bids.erase(order->price);
        else if (order->side == OrderSide::SELL) asks.erase(order->price);
    }
}

void OrderBook::printBook(int depth) const {
    std::cout << "========== ORDER BOOK ==========\n";
    std::cout << std::left << std::setw(20) << "Asks (Sell)" << " : " << std::setw(20) << "Bids (Buy)" << "\n";

    auto askIt = asks.begin();
    auto bidIt = bids.begin();

    for (int i = 0; i < depth; ++i) {
        std::ostringstream askStream, bidStream;

        if (askIt != asks.end()) {
            int askQty = 0;
            for (const auto& o : askIt->second) askQty += o.remaining;
            askStream << "$" << std::fixed << std::setprecision(2) << askIt->first << " : " << askQty;
            ++askIt;
        }

        if (bidIt != bids.end()) {
            int bidQty = 0;
            for (const auto& o : bidIt->second) bidQty += o.remaining;
            bidStream << "$" << std::fixed << std::setprecision(2) << bidIt->first << " : " << bidQty;
            ++bidIt;
        }

        std::cout << std::setw(20) << askStream.str() << " : " << bidStream.str() << "\n";
    }

    std::cout << "================================\n";
}

void OrderBook::matchBids(Order& order) {
    int originalRemaining = order.remaining;

    if (order.type == OrderType::FillOrKill || order.type == OrderType::FillAndKill) {
        int simulated = originalRemaining;
        auto it = bids.lower_bound(order.price);
        for (; it != bids.end(); ++it) {
            for (const auto& resting : it->second) {
                simulated -= std::min(simulated, resting.remaining);
                if (simulated <= 0) break;
            }
            if (simulated <= 0) break;
        }
        if (order.type == OrderType::FillOrKill && simulated > 0) {
            order.remaining = originalRemaining;
            return;
        }
    }

    auto it = bids.lower_bound(order.price);
    while (it != bids.end() && order.remaining > 0) {
        auto& dq = it->second;
        for (auto dqIt = dq.begin(); dqIt != dq.end() && order.remaining > 0;) {
            executeMatch(order, *dqIt);
            if (dqIt->remaining == 0) {
                orderIndex.erase(dqIt->id);
                dqIt = dq.erase(dqIt);
            } else {
                ++dqIt;
            }
        }
        if (dq.empty()) it = bids.erase(it);
        else ++it;
    }

    if (order.type == OrderType::FillAndKill && order.remaining > 0) {
        order.remaining = originalRemaining;
        return;
    }
}

void OrderBook::matchAsks(Order& order) {
    int originalRemaining = order.remaining;

    if (order.type == OrderType::FillOrKill || order.type == OrderType::FillAndKill) {
        int simulated = originalRemaining;
        auto it = asks.lower_bound(order.type == OrderType::Market ? 0.0 : order.price);
        for (; it != asks.end(); ++it) {
            for (const auto& resting : it->second) {
                simulated -= std::min(simulated, resting.remaining);
                if (simulated <= 0) break;
            }
            if (simulated <= 0) break;
        }
        if (order.type == OrderType::FillOrKill && simulated > 0) {
            order.remaining = originalRemaining;
            return;
        }
    }

    auto it = asks.lower_bound(order.type == OrderType::Market ? 0.0 : order.price);
    while (it != asks.end() && order.remaining > 0) {
        auto& dq = it->second;
        for (auto dqIt = dq.begin(); dqIt != dq.end() && order.remaining > 0;) {
            executeMatch(order, *dqIt);
            if (dqIt->remaining == 0) {
                orderIndex.erase(dqIt->id);
                dqIt = dq.erase(dqIt);
            } else {
                ++dqIt;
            }
        }
        if (dq.empty()) it = asks.erase(it);
        else ++it;
    }

    if (order.type == OrderType::FillAndKill && order.remaining > 0) {
        order.remaining = originalRemaining;
        return;
    }
}

void OrderBook::executeMatch(Order& incoming, Order& resting) {
    int tradeQty = min(incoming.remaining, resting.remaining);
    incoming.remaining -= tradeQty;
    resting.remaining -= tradeQty;
    tradeLog.push_back({incoming.side == OrderSide::BUY ? incoming.id : resting.id,
                        incoming.side == OrderSide::BUY ? resting.id : incoming.id,
                        resting.price, tradeQty, currentTimestamp()});

    cout << "Executed trade: " << tradeQty << " @ " << resting.price << "\n";
}
