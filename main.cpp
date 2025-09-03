#include "./include/utils.hpp"
#include "./include/OrderBook.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>

using namespace std;

int main() {
    OrderBook book;
    auto start = std::chrono::high_resolution_clock::now();
    book.addOrder(Order(1,  OrderType::GoodTillCancel, OrderSide::BUY,  100, 10, 10, currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(2,  OrderType::GoodTillCancel, OrderSide::BUY,  101, 5,  5,  currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(3,  OrderType::GoodForDay,     OrderSide::BUY,   99,  7,  7,  currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(4,  OrderType::GoodTillCancel, OrderSide::SELL, 102, 10, 10, currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(5,  OrderType::GoodTillCancel, OrderSide::SELL, 103, 5,  5,  currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(6,  OrderType::GoodTillCancel, OrderSide::SELL, 101, 6,  6,  currentTimestamp()));
    book.printBook(10);

    book.addOrder(Order(7,  OrderType::FillOrKill,     OrderSide::BUY,  100, 5,  5,  currentTimestamp())); // maybe not fully fillable
    book.printBook(10);
    book.addOrder(Order(8,  OrderType::FillOrKill,     OrderSide::SELL, 101, 5,  5,  currentTimestamp())); // should match
    book.printBook(10);
    book.addOrder(Order(9,  OrderType::FillAndKill,    OrderSide::BUY,  105, 5,  5,  currentTimestamp())); // try to match
    book.printBook(10);
    book.addOrder(Order(10, OrderType::FillAndKill,    OrderSide::SELL, 98,  5,  5,  currentTimestamp())); // try to match
    book.printBook(10);

    book.addOrder(Order(11, OrderType::Market,         OrderSide::BUY,  0,   3,  3,  currentTimestamp())); // market buy
    book.printBook(10);
    book.addOrder(Order(12, OrderType::Market,         OrderSide::SELL, 0,   6,  6,  currentTimestamp())); // market sell
    book.printBook(10);

    book.addOrder(Order(13, OrderType::GoodForDay,     OrderSide::SELL, 102, 2,  2,  currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(14, OrderType::GoodForDay,     OrderSide::BUY,  100, 4,  4,  currentTimestamp()));
    book.printBook(10);

    book.addOrder(Order(15, OrderType::FillOrKill,     OrderSide::SELL, 101, 6,  6,  currentTimestamp())); // should fail
    book.printBook(10);
    book.addOrder(Order(16, OrderType::FillOrKill,     OrderSide::BUY,  103, 1,  1,  currentTimestamp())); // should succeed
    book.printBook(10);

    book.addOrder(Order(17, OrderType::GoodTillCancel, OrderSide::SELL, 99,  7,  7,  currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(18, OrderType::GoodForDay,     OrderSide::BUY,  98,  9,  9,  currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(19, OrderType::FillAndKill,    OrderSide::SELL, 100, 3,  3,  currentTimestamp()));
    book.printBook(10);
    book.addOrder(Order(20, OrderType::Market,         OrderSide::BUY,  0,   6,  6,  currentTimestamp()));

    std::cout << "\n=== Final Order Book ===\n";
    book.printBook(10);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns\n";
    exportTradesToCSV(book.tradeLog, "trades.csv");
    return 0;
}
