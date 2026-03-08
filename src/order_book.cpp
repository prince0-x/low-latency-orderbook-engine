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

void OrderBook::cancelOrder(int order_id){
      auto it = order_lookup.find(order_id);
      if(it == order_lookup.end()){
            return;
      }

      Order * order = it->second;

      int price = order->price;
      auto side = order->side;

      if(side == Side::BUY){
            auto & level = bids[price];
            for(auto itr = level.begin(); itr != level.end(); ++itr){
                  if(itr->order_id == order_id){
                        level.erase(itr);
                        break;
                  }
            }
            if(level.empty()){
                  bids.erase(price);
            }
      }else{
            auto & level = asks[price];
            for(auto itr = level.begin(); itr != level.end(); ++itr){
                  if(itr->order_id == order_id){
                        level.erase(itr);
                        break;
                  }
            }
            if(level.empty()){
                  asks.erase(price);
            }
      }
      order_lookup.erase(order_id);
}