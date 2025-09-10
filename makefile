CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -flto -funroll-loops -ffast-math -DNDEBUG

SRC = main.cpp src/OrderBook.cpp src/OrderPool.cpp src/Server.cpp src/utils.cpp
BENCH_SRC = benchmark.cpp src/OrderBook.cpp src/OrderPool.cpp src/utils.cpp
TEST_SRC = test_price_time_priority.cpp src/OrderBook.cpp src/OrderPool.cpp src/utils.cpp
INC = -Iinclude
OUT = orderbook
BENCH_OUT = benchmark
TEST_OUT = test_priority

all: $(OUT) $(BENCH_OUT) $(TEST_OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(INC) $(SRC) -o $(OUT)

$(BENCH_OUT): $(BENCH_SRC)
	$(CXX) $(CXXFLAGS) $(INC) $(BENCH_SRC) -o $(BENCH_OUT)

$(TEST_OUT): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(INC) $(TEST_SRC) -o $(TEST_OUT)

clean:
	rm -f $(OUT) $(BENCH_OUT) $(TEST_OUT)

