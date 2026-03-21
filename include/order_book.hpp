#pragma once
#include <map>
#include <deque>
#include <unordered_map>
#include<string>
#include <fstream>
#include "order.hpp"

class OrderBook {

public:
    void addOrder(const Order& order);
    void cancelOrder(int order_id);
    int bestBid() const;
    int bestAsk() const;
    

    //matching engine

    void matchBuyOrder(Order &order);
    void matchSellOrder(Order &order);

    // wrapping trade
    void executeTrade(int price, int qty, int buyOrderId, int sellOrderId);

    //printing current state
    void printOrderBook() const;

    //reading directly from file
    void processOrdersFromFile(const std::string& filename);

private:

    // highest price first
    std::map<int, std::deque<Order>, std::greater<int>> bids;

    // lowest price first
    std::map<int, std::deque<Order>> asks;

    std::unordered_map<int, Order*> order_lookup;
};