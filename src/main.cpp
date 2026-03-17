#include <iostream>
#include "order_book.hpp"

int main()
{
    OrderBook book;

    Order o1(1, Side::BUY, 100, 5, 1);
    Order o2(2, Side::BUY, 100, 7, 2);
    Order o3(3, Side::SELL, 100, 6, 3);

    book.addOrder(o1);
    book.addOrder(o2);
    book.addOrder(o3);
    
    book.printOrderBook();

    return 0;
}