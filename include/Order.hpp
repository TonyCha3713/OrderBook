#pragma once
#include <string>
#include <cstdint>

enum class OrderType {
    GoodTillCancel,
    FillOrKill,
    FillAndKill,
    GoodForDay,
    Market
};

enum class OrderSide {
    BUY,
    SELL
};

struct Order {
    int id;
    OrderType type;
    OrderSide side;
    double price;
    int quantity;
    int remaining;
    int64_t timestamp; 

    Order()
        : id(0), type(OrderType::GoodTillCancel), side(OrderSide::BUY),
          price(0.0), quantity(0), remaining(0), timestamp(0) {}

    Order(int _id, OrderType _type, OrderSide _side, double _price, int _quantity, int _remaining, int64_t _timestamp)
    : id(_id), type(_type), side(_side), price(_price), quantity(_quantity), remaining(_remaining), timestamp(_timestamp) 
    {}

    void reset() {
        id = 0;
        type = OrderType::GoodTillCancel;
        side = OrderSide::BUY;
        price = 0.0;
        quantity = 0;
        remaining = 0;
        timestamp = 0;
    }
};