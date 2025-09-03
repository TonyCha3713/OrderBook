#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "Log.hpp"

using namespace std;

inline long long currentTimestamp() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
};
void exportTradesToCSV(const vector<Log>& trades, const string& filename);