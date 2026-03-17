#include "order_book.hpp"
#include<iostream>

void OrderBook::addOrder(const Order& order){
    Order newOrder = order;
    if(newOrder.side == Side::BUY){
        matchBuyOrder(newOrder);
    }else{
        matchSellOrder(newOrder);
    }

    if(newOrder.quantity > 0){
        if(newOrder.side == Side::BUY)
        {
            bids[newOrder.price].push_back(newOrder);
            order_lookup[newOrder.order_id] = &bids[newOrder.price].back();
        }
        else
        {
            asks[newOrder.price].push_back(newOrder);
            order_lookup[newOrder.order_id] = &asks[newOrder.price].back();
        }
    }
}

void OrderBook::cancelOrder(int order_id)
{
    auto it = order_lookup.find(order_id);
    if(it == order_lookup.end()){
      return;
    }
    Order* order = it->second;
    int price = order->price;

    if(order->side == Side::BUY)
    {
        auto levelIt = bids.find(price);
        if(levelIt == bids.end()){
            return;
        }
        auto &level = levelIt->second;
        for(auto itr = level.begin(); itr != level.end(); ++itr)
        {
            if(itr->order_id == order_id)
            {
                level.erase(itr);
                break;
            }
        }

        if(level.empty()){
            bids.erase(levelIt);
        }
    }
    else{
        auto levelIt = asks.find(price);
        if(levelIt == asks.end()){
            return;
        }
        auto &level = levelIt->second;
        for(auto itr = level.begin(); itr != level.end(); ++itr)
        {
            if(itr->order_id == order_id)
            {
                level.erase(itr);
                break;
            }
        }

        if(level.empty()){
            asks.erase(levelIt);
        }
    }
    order_lookup.erase(it);
}

int OrderBook::bestBid() const {

    if(bids.empty()) {
        return -1;
    }

    return bids.begin()->first;
}

int OrderBook::bestAsk() const
{
    if (asks.empty()){
        return -1;
    }

    return asks.begin()->first;
}

// matching logic 

void OrderBook::matchBuyOrder(Order &order)
{
    while(order.quantity > 0){
        if(asks.empty()){
            break;
        }

        int bestAsk = asks.begin()->first;
        if(bestAsk > order.price){
            break;
        }
        auto &level = asks.begin()->second;
        auto &restingOrder = level.front();

        int tradeQty = std::min(order.quantity,restingOrder.quantity);
        order.quantity -= tradeQty;
        restingOrder.quantity -= tradeQty;

        if(restingOrder.quantity == 0){
            level.pop_front();
        }
        if(level.empty()){
            asks.erase(bestAsk);
        }
    }
}

void OrderBook::matchSellOrder(Order &order)
{
    while(order.quantity > 0 && !bids.empty()){
        int bestBid = bids.begin()->first;
        if(bestBid < order.price){
            break;
        }

        auto &level = bids.begin()->second;
        auto &restingOrder = level.front();

        int tradeQty = std::min(order.quantity, restingOrder.quantity);
        executeTrade(bestBid, tradeQty, restingOrder.order_id, order.order_id); // if trade happens prints it
        order.quantity -= tradeQty;
        restingOrder.quantity -= tradeQty;

        if(restingOrder.quantity == 0){
            level.pop_front();
        }
        if(level.empty()){
            bids.erase(bids.begin());
        }
    }
}

void OrderBook::executeTrade(int price, int qty, int buyOrderId, int sellOrderId){
    std::cout
        << "TRADE "
        << "price=" << price
        << " qty=" << qty
        << " buy=" << buyOrderId
        << " sell=" << sellOrderId
        << std::endl;
}

void OrderBook::printOrderBook() const{
    std::cout<<"------------ORDER BOOK------------\n";
    std::cout<<"ASKS\n";
    for(const auto &[price, level]:asks){
        std::cout<<price<<" : ";
        for(const auto & order:level){
            std::cout<<"[id="<<order.order_id<<" qty="<<order.quantity<<"] ";
        }
        std::cout<<"\n";
    }
    std::cout<<"BIDS\n";
    for(const auto & [price, level]:bids){
        std::cout<<price<<" : ";
        for(const auto & order:level){
            std::cout<<"[id="<<order.order_id<<" qty="<<order.quantity<<"] ";
        }
        std::cout<<"\n";
    }
    std::cout << "------------------------\n";
}