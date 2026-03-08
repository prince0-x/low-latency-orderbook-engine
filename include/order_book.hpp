#pragma once
#include <map>
#include <deque>
#include <unordered_map>
#include "order.hpp"

class OrderBook {

public:
    void addOrder(const Order& order);
    void cancelOrder(int order_id);

private:

    // highest price first
    std::map<int, std::deque<Order>, std::greater<int>> bids;

    // lowest price first
    std::map<int, std::deque<Order>> asks;

    std::unordered_map<int, Order*> order_lookup;
};