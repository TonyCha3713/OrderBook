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

class Order {
public:
    Order() = default;
    Order(int _id, OrderType _type, OrderSide _side, double _price, int _quantity, int64_t _timestamp)
    : id(_id), type(_type), side(_side), price(_price), quantity(_quantity), remaining(_quantity), timestamp(_timestamp) 
    {
        validate();
    }
    
    int getId() const noexcept { return id; }
    OrderType getType() const noexcept { return type; }
    OrderSide getSide() const noexcept { return side; }
    double getPrice() const noexcept { return price; }
    int getQuantity() const noexcept { return quantity; }
    int getRemaining() const noexcept { return remaining; }
    int64_t getTimestamp() const noexcept { return timestamp; }

    void setRemaining(int _remaining) {
        if (_remaining < 0 || _remaining > quantity) throw std::invalid_argument("Invalid remaining quantity");
        remaining = _remaining;
    }

    void setPrice(double _price) {
        if (_price < 0.0) throw std::invalid_argument("Price cannot be negative");
        price = _price;
    }

    void reset() noexcept {
        id = 0;
        type = OrderType::GoodTillCancel;
        side = OrderSide::BUY;
        price = 0.0;
        quantity = 0;
        remaining = 0;
        timestamp = 0;
    }

    bool isFilled() const noexcept { return remaining == 0; }
    bool isMarketOrder() const noexcept { return type == OrderType::Market; }

private:
    int id{0};
    OrderType type{OrderType::GoodTillCancel};
    OrderSide side{OrderSide::BUY};
    double price{0.0};
    int quantity{0};
    int remaining{0};
    int64_t timestamp{0}; 

    void validate() const {
        if (id <= 0) throw std::invalid_argument("Order ID must be positive.");
        if (price < 0.0) throw std::invalid_argument("Price cannot be negative");
        if (quantity <= 0) throw std::invalid_argument("Quantity must be positive");
        if (timestamp < 0) throw std::invalid_argument("Timestamp cannot be negative");
        if (type == OrderType::Market && price != 0.0) throw std::invalid_argument("Market orders should have zero price");
        
    }
};