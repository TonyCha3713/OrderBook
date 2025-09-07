#include "../include/OrderBook.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

void OrderBook::addOrder(const Order& order) {
    Order incoming = order;
    removeExpiredOrders(bids);
    removeExpiredOrders(asks);
    // Check FOK orders can be fully filled first
    if (incoming.getType() == OrderType::FillOrKill && !canFillOrder(incoming)) {
        return;  // Reject FOK if can't fully fill
    }

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
            priceLevel.reserve(32);
        }
        
        priceLevel.push_back(incoming);
        orderIndex[incoming.getId()] = &priceLevel.back();
    }
}

bool OrderBook::canFillOrder(const Order& order) const {
    int remainingQty = order.getQuantity();
    
    if (order.getSide() == OrderSide::BUY) {
        for (const auto& [price, orders] : asks) {
            if (order.getType() != OrderType::Market && price > order.getPrice()) {
                break;
            }
            for (const auto& resting : orders) {
                remainingQty -= resting.getRemaining();
                if (remainingQty <= 0) return true;
            }
        }
    } else {
        for (auto it = bids.rbegin(); it != bids.rend(); ++it) {
            if (order.getType() != OrderType::Market && it->first < order.getPrice()) {
                break;
            }
            for (const auto& resting : it->second) {
                remainingQty -= resting.getRemaining();
                if (remainingQty <= 0) return true;
            }
        }
    }
    
    return false;
}

void OrderBook::cancelOrder(int orderId) {
    auto it = orderIndex.find(orderId);
    if (it == orderIndex.end()) {
        return;
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

        if (orders.empty()) {
            bookSide.erase(priceIt);
        }
    }

    orderIndex.erase(orderId);
}

void OrderBook::removeExpiredOrders(PriceMap& side) {
    for (auto it = side.begin(); it != side.end();) {
        auto& orders = it->second;
        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                [this](const Order& o) { 
                    if (isOrderExpired(o)) {
                        orderIndex.erase(o.getId());
                        return true;
                    }
                    return false;
                }
            ),
            orders.end()
        );
        
        if (orders.empty()) {
            it = side.erase(it);
        } else {
            ++it;
        }
    }
}

bool OrderBook::isOrderExpired(const Order& order) const {
    if (order.getType() != OrderType::GoodForDay) {
        return false;
    }
    
    // Check if more than 24 hours have passed since order timestamp
    int64_t now = currentTimestamp();
    constexpr int64_t DAY_IN_NANOS = 24LL * 60LL * 60LL * 1000000000LL;
    return (now - order.getTimestamp()) > DAY_IN_NANOS;
}

void OrderBook::printBook(int depth) const {
    std::cout << "\nOrderBook Snapshot:\n";
    std::cout << std::setfill('=') << std::setw(60) << "\n" << std::setfill(' ');
    
    std::cout << std::setw(25) << "BIDS" << " | " << std::setw(25) << "ASKS\n";
    std::cout << std::setfill('-') << std::setw(60) << "\n" << std::setfill(' ');

    auto bidIt = bids.rbegin();
    auto askIt = asks.begin();

    for (int i = 0; i < depth; ++i) {
        // Print bid side
        if (bidIt != bids.rend()) {
            int bidQty = 0;
            for (const auto& order : bidIt->second) {
                bidQty += order.getRemaining();
            }
            std::cout << std::fixed << std::setprecision(2) 
                     << std::setw(8) << bidQty << " @ " 
                     << std::setw(8) << bidIt->first;
            ++bidIt;
        } else {
            std::cout << std::setw(20) << "";
        }

        std::cout << " | ";

        // Print ask side
        if (askIt != asks.end()) {
            int askQty = 0;
            for (const auto& order : askIt->second) {
                askQty += order.getRemaining();
            }
            std::cout << std::fixed << std::setprecision(2)
                     << std::setw(8) << askQty << " @ " 
                     << std::setw(8) << askIt->first;
            ++askIt;
        } else {
            std::cout << std::setw(20) << "";
        }
        std::cout << "\n";
    }
    
    std::cout << std::setfill('=') << std::setw(60) << "\n" << std::setfill(' ');
}

void OrderBook::matchBids(Order& order) {
    auto it = bids.rbegin();
    while (it != bids.rend() && order.getRemaining() > 0) {
        if (order.getType() != OrderType::Market && it->first < order.getPrice()) {
            break;
        }

        auto& orders = it->second;
        for (auto& resting : orders) {
            if (order.getRemaining() == 0) break;
            executeMatch(order, resting);
        }

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
            break;
        }

        auto& orders = it->second;
        for (auto& resting : orders) {
            if (order.getRemaining() == 0) break;
            executeMatch(order, resting);
        }

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