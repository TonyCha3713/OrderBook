#include "./include/utils.hpp"
#include "./include/OrderBook.hpp"
#include "./include/Server.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <sstream>
#include <thread>

using namespace std;

int main() {
    OrderServer server;

    // Start server in a separate thread
    std::thread serverThread([&server]() {
        server.start();
    });

    std::cout << "Order Entry System\n";
    std::cout << "Commands:\n";
    std::cout << "MARKET [BUY/SELL] quantity\n";
    std::cout << "GTC/FOK/FAK/GTD [BUY/SELL] price quantity\n";
    std::cout << "quit to exit\n\n";

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit") break;

        try {
            std::istringstream iss(input);
            std::string typeStr, sideStr;
            double price;
            int quantity;

            iss >> typeStr >> sideStr;
            
            // Convert side string to enum
            OrderSide side = (sideStr == "BUY") ? OrderSide::BUY : OrderSide::SELL;

            if (typeStr == "MARKET") {
                iss >> quantity;
                server.submitOrder(OrderType::Market, side, 0.0, quantity);
            } 
            else {
                iss >> price >> quantity;
                OrderType type;
                
                if (typeStr == "GTC") type = OrderType::GoodTillCancel;
                else if (typeStr == "FOK") type = OrderType::FillOrKill;
                else if (typeStr == "FAK") type = OrderType::FillAndKill;
                else if (typeStr == "GTD") type = OrderType::GoodForDay;
                else throw std::invalid_argument("Invalid order type");

                server.submitOrder(type, side, price, quantity);
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
            std::cout << "Usage:\n";
            std::cout << "MARKET [BUY/SELL] quantity\n";
            std::cout << "GTC/FOK/FAK/GTD [BUY/SELL] price quantity\n";
        }
    }

    server.stop();
    serverThread.join();
    return 0;
}
