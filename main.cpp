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

    // Initial liquidity setup - GoodTillCancel orders
    book.addOrder(Order(1, OrderType::GoodTillCancel, OrderSide::BUY, 99.50, 1000, currentTimestamp()));
    book.addOrder(Order(2, OrderType::GoodTillCancel, OrderSide::BUY, 99.45, 1500, currentTimestamp()));
    book.addOrder(Order(3, OrderType::GoodTillCancel, OrderSide::BUY, 99.40, 2000, currentTimestamp()));
    book.addOrder(Order(4, OrderType::GoodTillCancel, OrderSide::SELL, 100.50, 1000, currentTimestamp()));
    book.addOrder(Order(5, OrderType::GoodTillCancel, OrderSide::SELL, 100.55, 1500, currentTimestamp()));
    book.addOrder(Order(6, OrderType::GoodTillCancel, OrderSide::SELL, 100.60, 2000, currentTimestamp()));

    // Aggressive trading - Market orders
    book.addOrder(Order(7, OrderType::Market, OrderSide::BUY, 0.0, 500, currentTimestamp()));
    book.addOrder(Order(8, OrderType::Market, OrderSide::SELL, 0.0, 750, currentTimestamp()));
    book.addOrder(Order(9, OrderType::Market, OrderSide::BUY, 0.0, 300, currentTimestamp()));
    
    // Add more liquidity - GoodForDay orders
    book.addOrder(Order(10, OrderType::GoodForDay, OrderSide::BUY, 99.48, 1200, currentTimestamp()));
    book.addOrder(Order(11, OrderType::GoodForDay, OrderSide::SELL, 100.52, 800, currentTimestamp()));
    
    // FillOrKill testing
    book.addOrder(Order(12, OrderType::FillOrKill, OrderSide::BUY, 100.55, 1500, currentTimestamp()));
    book.addOrder(Order(13, OrderType::FillOrKill, OrderSide::SELL, 99.40, 2000, currentTimestamp()));
    
    // Cancel some orders
    book.cancelOrder(1);
    book.cancelOrder(4);

    // Large market impact
    book.addOrder(Order(14, OrderType::Market, OrderSide::BUY, 0.0, 3000, currentTimestamp()));
    book.addOrder(Order(15, OrderType::Market, OrderSide::SELL, 0.0, 2500, currentTimestamp()));

    // Rapid fire limit orders
    for(int i = 16; i < 46; ++i) {
        double price = (i % 2 == 0) ? 99.45 + (i * 0.01) : 100.55 - (i * 0.01);
        OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        book.addOrder(Order(i, OrderType::GoodTillCancel, side, price, 100 + i, currentTimestamp()));
    }

    // FillAndKill orders
    book.addOrder(Order(46, OrderType::FillAndKill, OrderSide::BUY, 100.50, 500, currentTimestamp()));
    book.addOrder(Order(47, OrderType::FillAndKill, OrderSide::SELL, 99.50, 700, currentTimestamp()));
    
    // More cancellations
    for(int i = 16; i < 26; ++i) {
        book.cancelOrder(i);
    }

    // Market stress test
    for(int i = 48; i < 68; ++i) {
        book.addOrder(Order(i, OrderType::Market, OrderSide::BUY, 0.0, 100, currentTimestamp()));
        book.addOrder(Order(i+20, OrderType::Market, OrderSide::SELL, 0.0, 100, currentTimestamp()));
    }

    // Add depth to book
    for(int i = 88; i < 98; ++i) {
        book.addOrder(Order(i, OrderType::GoodTillCancel, OrderSide::BUY, 99.0 - (i-88)*0.1, 500, currentTimestamp()));
        book.addOrder(Order(i+10, OrderType::GoodTillCancel, OrderSide::SELL, 101.0 + (i-88)*0.1, 500, currentTimestamp()));
    }
    book.printBook();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Elapsed: " 
              << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() 
              << " ns\n";
    
    exportTradesToCSV(book.tradeLog, "trades.csv");
    return 0;
}
