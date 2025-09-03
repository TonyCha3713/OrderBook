#include "../include/OrderPool.hpp"

OrderPool::OrderPool(size_t capacity) {
    pool.resize(capacity);
    for (int i = static_cast<int>(capacity - 1); i >= 0; --i) {
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
    int index = static_cast<int>(order - &pool[0]);
    freeList.push(index);
}
