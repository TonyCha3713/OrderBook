# OrderBook
A fully custom limit order book implemented in C++ designed to simulate real exchange behavior. Optimized for low latency using custom data structures (std::map, vector), inline critical paths, and preallocated order pooling to minimize heap allocation overhead. Processes 20+ operations in under 700µs.
