#include "../include/OrderPool.hpp"

OrderPool::OrderPool(size_t capacity) {
    if (capacity == 0 || capacity > MAX_CAPACITY) {
        throw std::invalid_argument("Capacity must be between 1 and " + std::to_string(MAX_CAPACITY));
    }
    pool.resize(capacity);
    for (int i = static_cast<int> (capacity - 1); i >= 0; --i) {
        freeList.push(i);
    }
}

Order* OrderPool::acquire() {
    if (freeList.empty()) {
        grow();
        if (freeList.empty()) return nullptr; // Still empty after grow
    }
    int index = freeList.top();
    freeList.pop();
    return &pool[index];
}

void OrderPool::release(Order* order) {
    if (!order) return;

    for (size_t i = 0; i < pool.size(); ++i) {
        if (&pool[i] == order) {
            order->reset();
            freeList.push(static_cast<int>(i));
            return;
        }
    }

    throw std::invalid_argument("Order does not belong to this pool");
}
 
void OrderPool::grow() {
    size_t current = pool.size();
    size_t additional = current / 2;  // grow by 50%
    if (additional < 1024) additional = 1024;  // minimum growth
    size_t newSize = current + additional;
    if (newSize > MAX_CAPACITY) {
        newSize = MAX_CAPACITY;
        if (newSize <= current) return;  // Can't grow further
    }

    pool.resize(newSize);
    for (int i = static_cast<int>(newSize - 1); i >= static_cast<int>(current); --i) {
        freeList.push(i);
    }
}
