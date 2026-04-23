#include <iostream>
#include "engine/order_book.hpp"

int main() {
    OrderBook book;

    // Case 1: Simple match
    std::cout << "\n--- Case 1: Simple Match ---\n";
    book.addOrder(Order(1, Side::BUY, 100, 5, 1));
    book.addOrder(Order(2, Side::SELL, 100, 5, 2));
    book.printOrderBook();

    // Case 2: Partial match
    std::cout << "\n--- Case 2: Partial Match ---\n";
    book.addOrder(Order(3, Side::BUY, 100, 10, 3));
    book.addOrder(Order(4, Side::SELL, 100, 4, 4));
    book.printOrderBook();

    // Case 3: Multi-level match
    std::cout << "\n--- Case 3: Multi-level ---\n";
    book.addOrder(Order(5, Side::SELL, 101, 5, 5));
    book.addOrder(Order(6, Side::SELL, 102, 5, 6));
    book.addOrder(Order(7, Side::BUY, 105, 8, 7)); // should eat 101 first, then 102
    book.printOrderBook();

    // Case 4: Cancel existing
    std::cout << "\n--- Case 4: Cancel Existing ---\n";
    book.cancelOrder(3); // remaining from partial
    book.printOrderBook();

    // Case 5: Cancel already matched order
    std::cout << "\n--- Case 5: Cancel Matched ---\n";
    book.cancelOrder(2); // already gone
    book.printOrderBook();

    // Case 6: Cancel non-existent
    std::cout << "\n--- Case 6: Cancel Invalid ---\n";
    book.cancelOrder(999);
    book.printOrderBook();

    // Case 7: FIFO check
    std::cout << "\n--- Case 7: FIFO ---\n";
    book.addOrder(Order(8, Side::BUY, 100, 5, 8));
    book.addOrder(Order(9, Side::BUY, 100, 5, 9));
    book.addOrder(Order(10, Side::SELL, 100, 6, 10)); // should hit id=8 first
    book.printOrderBook();

    return 0;
}