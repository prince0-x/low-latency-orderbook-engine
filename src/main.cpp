#include <iostream>
#include "order_book.hpp"

int main()
{
    OrderBook book;

    Order o1(1, Side::BUY, 100, 10, 1);
    Order o2(2, Side::BUY, 105, 5, 2);
    Order o3(3, Side::BUY, 103, 7, 3);

    Order o4(4, Side::SELL, 101, 8, 4);

    book.addOrder(o1);
    book.addOrder(o2);
    book.addOrder(o3);

    book.addOrder(o4);   // should trigger matching

    std::cout << "Best Bid: " << book.bestBid() << std::endl;
    std::cout << "Best Ask: " << book.bestAsk() << std::endl;

    return 0;
}