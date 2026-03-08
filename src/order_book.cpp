#include "order_book.hpp"

void OrderBook::addOrder(const Order& order) {

    if(order.side == Side::BUY) {
        bids[order.price].push_back(order);
        order_lookup[order.order_id] = &bids[order.price].back();
    }
    else {
        asks[order.price].push_back(order);
        order_lookup[order.order_id] = &asks[order.price].back();
    }
}