#include "../include/utils.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
using namespace std;

void exportTradesToCSV(const vector<Log>& trades, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    file << "BuyOrderID,SellOrderID,Price,Quantity,Timestamp\n";

    for (const auto& t : trades) {
        file << t.buyOrderId << ","
             << t.sellOrderId << ","
             << t.price << ","
             << t.quantity << ","
             << t.timestamp << "\n";
    }

    file.close();
    cout << "Trades exported to " << filename << "\n";
}