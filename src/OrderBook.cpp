#include "../include/OrderBook.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

void OrderBook::addOrder(const Order& order) {
    Order* incoming = orderPool.acquire();
    if (!incoming) return;
    *incoming = order;
    removeExpiredOrders(bids);
    removeExpiredOrders(asks);
    // Check FOK orders can be fully filled first
    if (__builtin_expect(incoming->getType() == OrderType::FillOrKill, 0) && !canFillOrder(*incoming)) {
        orderPool.release(incoming);
        return;  // Reject FOK if can't fully fill
    }

    // Match order based on side
    if (incoming->getSide() == OrderSide::BUY) {
        matchAsks(*incoming);
    } else {
        matchBids(*incoming);
    }

    // Add remaining quantity to book if applicable
    if (__builtin_expect(incoming->getRemaining() > 0, 1) && 
        (__builtin_expect(incoming->getType() == OrderType::GoodForDay, 0) || 
         __builtin_expect(incoming->getType() == OrderType::GoodTillCancel, 1))) {
        
        auto& bookSide = (incoming->getSide() == OrderSide::BUY) ? bids : asks;
        auto& priceLevel = bookSide[incoming->getPrice()];
        
        if (priceLevel.empty()) {
            priceLevel.reserve(PRICE_LEVEL_CAPACITY);
        }
        
        priceLevel.push_back(incoming);
        orderIndex[incoming->getId()] = incoming;
    } else {
        orderPool.release(incoming); // Release back to pool if fully filled or not to be
    }
}

inline bool OrderBook::canFillOrder(const Order& order) const {
    int remainingQty = order.getQuantity();
    
    if (order.getSide() == OrderSide::BUY) {
        for (const auto& [price, orders] : asks) {
            if (order.getType() != OrderType::Market && price > order.getPrice()) {
                break;
            }
            for (const auto& resting : orders) {
                remainingQty -= resting->getRemaining();
                if (remainingQty <= 0) return true;
            }
        }
    } else {
        for (auto it = bids.rbegin(); it != bids.rend(); ++it) {
            if (order.getType() != OrderType::Market && it->first < order.getPrice()) {
                break;
            }
            for (const auto& resting : it->second) {
                remainingQty -= resting->getRemaining();
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
                [orderId](const Order* o) { return o->getId() == orderId; }),
            orders.end()
        );

        if (orders.empty()) {
            bookSide.erase(priceIt);
        }
    }
    orderPool.release(order);
    orderIndex.erase(orderId);
}

void OrderBook::removeExpiredOrders(PriceMap& side) {
    for (auto it = side.begin(); it != side.end();) {
        auto& orders = it->second;
        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                [this](Order* o) {
                    if (isOrderExpired(*o)) {
                        orderIndex.erase(o->getId());
                        orderPool.release(o);
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

inline bool OrderBook::isOrderExpired(const Order& order) const {
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
                bidQty += order->getRemaining();
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
                askQty += order->getRemaining();
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
            if (__builtin_expect(order.getRemaining() == 0, 0)) break;
            executeMatch(order, *resting);
        }

        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                [](const Order* o) { return o->getRemaining() == 0; }),
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
            if (__builtin_expect(order.getRemaining() == 0, 0)) break;
            executeMatch(order, *resting);
        }

        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
                [](const Order* o) { return o->getRemaining() == 0; }),
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

void OrderBook::saveOrdersToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    // Write header
    file << "OrderID,Type,Side,Price,Quantity,Remaining,Timestamp\n";

    // Save all orders from bids
    for (const auto& [price, orders] : bids) {
        for (const auto& order : orders) {
            file << order->getId() << ","
                 << static_cast<int>(order->getType()) << ","
                 << static_cast<int>(order->getSide()) << ","
                 << std::fixed << std::setprecision(4) << order->getPrice() << ","
                 << order->getQuantity() << ","
                 << order->getRemaining() << ","
                 << order->getTimestamp() << "\n";
        }
    }

    // Save all orders from asks
    for (const auto& [price, orders] : asks) {
        for (const auto& order : orders) {
            file << order->getId() << ","
                 << static_cast<int>(order->getType()) << ","
                 << static_cast<int>(order->getSide()) << ","
                 << std::fixed << std::setprecision(4) << order->getPrice() << ","
                 << order->getQuantity() << ","
                 << order->getRemaining() << ","
                 << order->getTimestamp() << "\n";
        }
    }

    file.close();
    std::cout << "Order book state saved to " << filename << std::endl;
}

void OrderBook::loadOrdersFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "No existing order book file found: " << filename << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // Skip header

    int loadedOrders = 0;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;

        // Parse CSV line
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() != 7) {
            std::cerr << "Invalid line format: " << line << std::endl;
            continue;
        }

        try {
            int id = std::stoi(tokens[0]);
            OrderType type = static_cast<OrderType>(std::stoi(tokens[1]));
            OrderSide side = static_cast<OrderSide>(std::stoi(tokens[2]));
            double price = std::stod(tokens[3]);
            int quantity = std::stoi(tokens[4]);
            int remaining = std::stoi(tokens[5]);
            int64_t timestamp = std::stoll(tokens[6]);
            Order* order = orderPool.acquire();
            *order = Order(id, type, side, price, quantity, timestamp);
            // Create order and add to book
            order->setRemaining(remaining);

            // Add to appropriate side
            auto& bookSide = (side == OrderSide::BUY) ? bids : asks;
            auto& priceLevel = bookSide[price];
            
            if (priceLevel.empty()) {
                priceLevel.reserve(32);
            }
            
            priceLevel.push_back(order);
            orderIndex[id] = order;
            loadedOrders++;
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
        }
    }

    file.close();
    std::cout << "Loaded " << loadedOrders << " orders from " << filename << std::endl;
}