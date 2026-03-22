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
            auto &level = bids[newOrder.price];
            level.push_back(newOrder);
            order_lookup[newOrder.order_id] = std::prev(level.end());
        }
        else
        {
            auto &level = asks[newOrder.price];
            level.push_back(newOrder);
            order_lookup[newOrder.order_id] = std::prev(level.end());
        }
    }
}

void OrderBook::cancelOrder(int order_id)
{
    auto it = order_lookup.find(order_id);
    if(it == order_lookup.end()) return;

    auto order_it = it->second;

    // CRITICAL GUARD
    if (order_it == std::deque<Order>::iterator{}) {
        order_lookup.erase(it);
        return;
    }

    int price = order_it->price;
    Side side = order_it->side;

    if(side == Side::BUY)
    {
        auto levelIt = bids.find(price);
        if(levelIt != bids.end()){
            auto &level = levelIt->second;

            // VERIFY iterator still belongs
            bool found = false;
            for(auto itr = level.begin(); itr != level.end(); ++itr){
                if(itr == order_it){
                    level.erase(itr);
                    found = true;
                    break;
                }
            }

            if(!found){
                order_lookup.erase(it);
                return;
            }

            if(level.empty()) bids.erase(levelIt);
        }
    }
    else
    {
        auto levelIt = asks.find(price);
        if(levelIt != asks.end()){
            auto &level = levelIt->second;

            bool found = false;
            for(auto itr = level.begin(); itr != level.end(); ++itr){
                if(itr == order_it){
                    level.erase(itr);
                    found = true;
                    break;
                }
            }

            if(!found){
                order_lookup.erase(it);
                return;
            }

            if(level.empty()) asks.erase(levelIt);
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
        if(asks.empty()) break;

        auto it = asks.begin();
        int bestAsk = it->first;

        if(bestAsk > order.price) break;

        auto &level = it->second;
        auto &restingOrder = level.front();

        if (restingOrder.quantity == 0) {
            order_lookup.erase(restingOrder.order_id);
            level.pop_front();
            continue;
        }

        int tradeQty = std::min(order.quantity, restingOrder.quantity);
        order.quantity -= tradeQty;
        restingOrder.quantity -= tradeQty;

        if(restingOrder.quantity == 0){
            order_lookup.erase(restingOrder.order_id);
            level.pop_front();
        }

        if(level.empty()){
            asks.erase(it);
        }
    }
}

void OrderBook::matchSellOrder(Order &order)
{
    while(order.quantity > 0 && !bids.empty()){
        auto it = bids.begin();
        int bestBid = it->first;

        if(bestBid < order.price) break;

        auto &level = it->second;
        auto &restingOrder = level.front();

        if (restingOrder.quantity == 0) {
            order_lookup.erase(restingOrder.order_id);
            level.pop_front();
            continue;
        }

        int tradeQty = std::min(order.quantity, restingOrder.quantity);
        order.quantity -= tradeQty;
        restingOrder.quantity -= tradeQty;

        if(restingOrder.quantity == 0){
            order_lookup.erase(restingOrder.order_id);
            level.pop_front();
        }

        if(level.empty()){
            bids.erase(it);
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

void OrderBook::processOrdersFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    std::string type;

    int next_id = 1;
    int timestamp = 1;

    while(file >> type)
    {
        if(type == "BUY")
        {
            int price, qty;
            file >> price >> qty;

            Order o(next_id++, Side::BUY, price, qty, timestamp++);
            addOrder(o);
        }
        else if(type == "SELL")
        {
            int price, qty;
            file >> price >> qty;

            Order o(next_id++, Side::SELL, price, qty, timestamp++);
            addOrder(o);
        }
        else if(type == "CANCEL")
        {
            int id;
            file >> id;

            cancelOrder(id);
        }

        // debug (keep for now)
        // printOrderBook();
    }
}