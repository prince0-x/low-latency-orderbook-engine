#include <iostream>
#include "order_book.hpp"

int main() {

    OrderBook book;

    Order o1(1, Side::BUY, 100, 10, 1);
    Order o2(2, Side::SELL, 105, 5, 2);

    book.addOrder(o1);
    book.addOrder(o2);

    std::cout << "Orders inserted successfully\n";
    
    book.cancelOrder(1);

    std::cout<< "Order Cancelled Successfully\n";

    return 0;
}