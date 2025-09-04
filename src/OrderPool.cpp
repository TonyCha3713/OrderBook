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
    if (freeList.empty()) return nullptr; // Or throw if you're strict
    int index = freeList.top();
    freeList.pop();
    return &pool[index];
}

void OrderPool::release(Order* order) {
    if (!order) return;

    size_t index = order - &pool[0];
    if (index >= pool.size()) {
        throw std::invalid_argument("Order does not belong to this pool");
    }
    order->reset();
    freeList.push(static_cast<int>(index));
}
