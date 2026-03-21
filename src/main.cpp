#include <iostream>
#include <chrono>
#include "order_book.hpp"

int main()
{
    

    auto start = std::chrono::high_resolution_clock::now();
    OrderBook book;
    book.processOrdersFromFile("data/orders.txt");
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Total Time (microseconds): " << duration << std::endl;
    std::cout << "Orders Processed: 300000" << std::endl;
    std::cout << "Avg Latency per Order (us): " << (double)duration / 300000 << std::endl;
    std::cout << "Throughput (orders/sec): " << (300000.0 * 1e6) / duration << std::endl;
    return 0;
}