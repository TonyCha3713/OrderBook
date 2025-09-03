#pragma once
#include "Order.hpp"
#include <vector>
#include <stack>

class OrderPool {
private:
    std::vector<Order> pool;
    std::stack<int> freeList;

public:
    explicit OrderPool(size_t capacity = 1024);

    Order* acquire();   // Get available order
    void release(Order* order); // Return order back to pool
};
