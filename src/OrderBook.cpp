#include "../include/OrderBook.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

void OrderBook::addOrder(const Order& order) {
    Order incoming = order;
    
    // Match order based on side
    if (incoming.getSide() == OrderSide::BUY) {
        matchAsks(incoming);
    } else {
        matchBids(incoming);
    }

    // Add remaining quantity to book if applicable
    if (incoming.getRemaining() > 0 && 
        (incoming.getType() == OrderType::GoodForDay || 
         incoming.getType() == OrderType::GoodTillCancel)) {
        
        auto& bookSide = (incoming.getSide() == OrderSide::BUY) ? bids : asks;
        auto& priceLevel = bookSide[incoming.getPrice()];
        
        if (priceLevel.empty()) {
            priceLevel.reserve(32);  // Pre-allocate space for efficiency
        }
        
        priceLevel.push_back(incoming);
        orderIndex[incoming.getId()] = &priceLevel.back();
    }
}

void OrderBook::cancelOrder(int orderId) {
    auto it = orderIndex.find(orderId);
    if (it == orderIndex.end()) {
        return;  // Order not found
    }

    Order* order = it->second;
    auto& bookSide = (order->getSide() == OrderSide::BUY) ? bids : asks;
    
    auto priceIt = bookSide.find(order->getPrice());
    if (priceIt != bookSide.end()) {
        auto& orders = priceIt->second;
        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                [orderId](const Order& o) { return o.getId() == orderId; }),
            orders.end()
        );

        // Remove empty price levels
        if (orders.empty()) {
            bookSide.erase(priceIt);
        }
    }

    orderIndex.erase(orderId);
}

void OrderBook::printBook(int depth) const {
    std::cout << "\nOrderBook Snapshot:\n";
    std::cout << std::setfill('=') << std::setw(40) << "\n" << std::setfill(' ');
    
    // Print asks (in ascending order)
    int levelCount = 0;
    for (auto it = asks.begin(); it != asks.end() && levelCount < depth; ++it, ++levelCount) {
        int totalQty = 0;
        for (const auto& order : it->second) {
            totalQty += order.getRemaining();
        }
        std::cout << "ASK: " << std::setw(10) << std::fixed << std::setprecision(2) 
                  << it->first << " x " << std::setw(6) << totalQty << "\n";
    }

    std::cout << std::setfill('-') << std::setw(40) << "\n" << std::setfill(' ');

    // Print bids (in descending order)
    levelCount = 0;
    for (auto it = bids.rbegin(); it != bids.rend() && levelCount < depth; ++it, ++levelCount) {
        int totalQty = 0;
        for (const auto& order : it->second) {
            totalQty += order.getRemaining();
        }
        std::cout << "BID: " << std::setw(10) << std::fixed << std::setprecision(2) 
                  << it->first << " x " << std::setw(6) << totalQty << "\n";
    }
    
    std::cout << std::setfill('=') << std::setw(40) << "\n" << std::setfill(' ');
}

void OrderBook::matchBids(Order& order) {
    auto it = bids.rbegin();
    while (it != bids.rend() && order.getRemaining() > 0) {
        if (order.getType() != OrderType::Market && it->first < order.getPrice()) {
            break;  // Price not matched for limit orders
        }

        auto& orders = it->second;
        for (auto& resting : orders) {
            if (order.getRemaining() == 0) break;
            executeMatch(order, resting);
        }

        // Remove filled orders and empty price levels
        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                [](const Order& o) { return o.getRemaining() == 0; }),
            orders.end()
        );

        if (orders.empty()) {
            it = decltype(it)(bids.erase(--(it.base())));
        } else {
            ++it;
        }
    }
}

void OrderBook::matchAsks(Order& order) {
    auto it = asks.begin();
    while (it != asks.end() && order.getRemaining() > 0) {
        if (order.getType() != OrderType::Market && it->first > order.getPrice()) {
            break;  // Price not matched for limit orders
        }

        auto& orders = it->second;
        for (auto& resting : orders) {
            if (order.getRemaining() == 0) break;
            executeMatch(order, resting);
        }

        // Remove filled orders and empty price levels
        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                [](const Order& o) { return o.getRemaining() == 0; }),
            orders.end()
        );

        if (orders.empty()) {
            it = asks.erase(it);
        } else {
            ++it;
        }
    }
}

inline void OrderBook::executeMatch(Order& incoming, Order& resting) {
    int tradeQty = std::min(incoming.getRemaining(), resting.getRemaining());
    
    incoming.setRemaining(incoming.getRemaining() - tradeQty);
    resting.setRemaining(resting.getRemaining() - tradeQty);
    
    tradeLog.push_back({
        incoming.getSide() == OrderSide::BUY ? incoming.getId() : resting.getId(),
        incoming.getSide() == OrderSide::BUY ? resting.getId() : incoming.getId(),
        resting.getPrice(),
        tradeQty,
        currentTimestamp()
    });
}