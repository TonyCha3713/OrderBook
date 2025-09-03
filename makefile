CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

SRC = main.cpp src/OrderBook.cpp src/OrderPool.cpp src/utils.cpp
INC = -Iinclude
OUT = orderbook

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(INC) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)

