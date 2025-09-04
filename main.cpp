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

    // Test GoodTillCancel orders
    book.addOrder(Order(1, OrderType::GoodTillCancel, OrderSide::BUY, 100.0, 10, currentTimestamp()));
    book.addOrder(Order(2, OrderType::GoodTillCancel, OrderSide::SELL, 102.0, 5, currentTimestamp()));
    book.printBook();

    // Test GoodForDay orders
    book.addOrder(Order(3, OrderType::GoodForDay, OrderSide::BUY, 101.0, 15, currentTimestamp()));
    book.addOrder(Order(4, OrderType::GoodForDay, OrderSide::SELL, 101.5, 8, currentTimestamp()));
    book.printBook();

    // Test FillOrKill orders
    book.addOrder(Order(5, OrderType::FillOrKill, OrderSide::BUY, 102.0, 3, currentTimestamp()));
    book.addOrder(Order(6, OrderType::FillOrKill, OrderSide::SELL, 100.0, 5, currentTimestamp()));
    book.printBook();

    // Test FillAndKill orders
    book.addOrder(Order(7, OrderType::FillAndKill, OrderSide::BUY, 101.5, 4, currentTimestamp()));
    book.addOrder(Order(8, OrderType::FillAndKill, OrderSide::SELL, 100.0, 6, currentTimestamp()));
    book.printBook();

    // Test Market orders
    book.addOrder(Order(9, OrderType::Market, OrderSide::BUY, 0.0, 5, currentTimestamp()));
    book.addOrder(Order(10, OrderType::Market, OrderSide::SELL, 0.0, 3, currentTimestamp()));
    book.printBook();

    // Add more liquidity
    book.addOrder(Order(11, OrderType::GoodTillCancel, OrderSide::BUY, 99.5, 20, currentTimestamp()));
    book.addOrder(Order(12, OrderType::GoodTillCancel, OrderSide::SELL, 102.5, 15, currentTimestamp()));
    book.printBook();

    // Test order cancellations
    book.cancelOrder(1);  // Cancel GoodTillCancel buy
    book.cancelOrder(2);  // Cancel GoodTillCancel sell
    book.printBook();

    // Test multiple price levels
    book.addOrder(Order(13, OrderType::GoodTillCancel, OrderSide::BUY, 99.0, 10, currentTimestamp()));
    book.addOrder(Order(14, OrderType::GoodTillCancel, OrderSide::BUY, 98.5, 15, currentTimestamp()));
    book.addOrder(Order(15, OrderType::GoodTillCancel, OrderSide::SELL, 103.0, 12, currentTimestamp()));
    book.addOrder(Order(16, OrderType::GoodTillCancel, OrderSide::SELL, 103.5, 8, currentTimestamp()));
    book.printBook();

    // Test matching at multiple price levels
    book.addOrder(Order(17, OrderType::Market, OrderSide::BUY, 0.0, 25, currentTimestamp()));
    book.addOrder(Order(18, OrderType::Market, OrderSide::SELL, 0.0, 20, currentTimestamp()));
    book.printBook();

    // Test FillOrKill with insufficient liquidity
    book.addOrder(Order(19, OrderType::FillOrKill, OrderSide::BUY, 103.0, 50, currentTimestamp()));
    book.addOrder(Order(20, OrderType::FillOrKill, OrderSide::SELL, 98.0, 40, currentTimestamp()));
    book.printBook();

    // Test FillAndKill with partial fills
    book.addOrder(Order(21, OrderType::FillAndKill, OrderSide::BUY, 103.0, 10, currentTimestamp()));
    book.addOrder(Order(22, OrderType::FillAndKill, OrderSide::SELL, 98.0, 8, currentTimestamp()));
    book.printBook();

    // Add more GoodForDay orders
    book.addOrder(Order(23, OrderType::GoodForDay, OrderSide::BUY, 100.5, 15, currentTimestamp()));
    book.addOrder(Order(24, OrderType::GoodForDay, OrderSide::SELL, 101.0, 12, currentTimestamp()));
    book.printBook();

    // Final market orders
    book.addOrder(Order(25, OrderType::Market, OrderSide::BUY, 0.0, 10, currentTimestamp()));
    book.addOrder(Order(26, OrderType::Market, OrderSide::SELL, 0.0, 8, currentTimestamp()));
    
    std::cout << "\n=== Final Order Book ===\n";
    book.printBook();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Elapsed: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns\n";
    
    exportTradesToCSV(book.tradeLog, "trades.csv");
    return 0;
}
