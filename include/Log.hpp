#pragma once
#include <cstdint>

struct Log {
    int buyOrderId;
    int sellOrderId;
    double price;
    int quantity;
    int64_t timestamp;
};