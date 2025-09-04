#pragma once
#include "Order.hpp"
#include <vector>
#include <stack>

class OrderPool {
private:
    std::vector<Order> pool;
    std::stack<int> freeList;

public:
    static constexpr size_t DEFAULT_CAPACITY = 1024;
    static constexpr size_t MAX_CAPACITY = 10000000;

    explicit OrderPool(size_t capacity = DEFAULT_CAPACITY);
    Order* acquire();   // Get available order
    void release(Order* order); // Return order back to poo
    bool isEmpty() const noexcept { return freeList.empty(); }
    size_t availableOrders() const noexcept { return freeList.size(); }
    size_t capacity() const noexcept { return pool.size(); }
    
    
};
