#include "../include/utils.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
using namespace std;

void exportTradesToCSV(const vector<Log>& trades, const string& filename) {
    bool writeHeader = false;
    ifstream infile(filename);
    if (!infile.good() || infile.peek() == ifstream::traits_type::eof()) {
        writeHeader = true;
    }
    infile.close();

    ofstream file(filename, ios::app); // Open in append mode
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    // Write header only if file was empty
    if (writeHeader) {
        file << "BuyOrderID,SellOrderID,Price,Quantity,Timestamp\n";
    }

    for (const auto& t : trades) {
        file << t.buyOrderId << ","
             << t.sellOrderId << ","
             << t.price << ","
             << t.quantity << ","
             << t.timestamp << "\n";
    }

    file.close();
    cout << "Trades exported (appended) to " << filename << "\n";
}

