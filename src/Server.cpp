#include "../include/Server.hpp"
#include "../include/utils.hpp"

void OrderServer::processNextOrder() {
    OrderRequest request;
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return !orderQueue.empty() || !running; });
        if (!running) return;
        request = orderQueue.front();
        orderQueue.pop();
    }

    Order order(
        nextOrderId++,
        request.type,
        request.side,
        request.price,
        request.quantity,
        currentTimestamp()
    );

    book.addOrder(order);
    book.printBook();
}