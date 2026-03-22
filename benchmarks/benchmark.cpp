#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include "order_book.hpp"

struct Command {
    int type; // 0=BUY, 1=SELL, 2=CANCEL
    int id;
    int price;
    int qty;
};

int main() {

    std::cout << "START\n";

    std::vector<Command> cmds;
    cmds.reserve(300000);

    std::ifstream file("data/orders.txt");

    if (!file.is_open()) {
        std::cout << "ERROR: could not open data/orders.txt\n";
        return 0;
    }

    std::string type;
    int next_id = 1;

    while(file >> type)
    {
        if(type == "BUY"){
            int price, qty;
            file >> price >> qty;
            cmds.push_back({0, next_id++, price, qty});
        }
        else if(type == "SELL"){
            int price, qty;
            file >> price >> qty;
            cmds.push_back({1, next_id++, price, qty});
        }
        else if(type == "CANCEL"){
            int id;
            file >> id;
            cmds.push_back({2, id, 0, 0});
        }
    }

    std::cout << "Loaded commands: " << cmds.size() << "\n";

    if (cmds.empty()) {
        std::cout << "No commands loaded\n";
        return 0;
    }

    OrderBook book;

    auto start = std::chrono::high_resolution_clock::now();

    int counter = 0;

for(auto &c : cmds){
    counter++;

    if(counter % 50000 == 0){
        std::cout << "Processed: " << counter << "\n";
    }

    if(c.type == 0){
        book.addOrder(Order(c.id, Side::BUY, c.price, c.qty, 0));
    }
    else if(c.type == 1){
        book.addOrder(Order(c.id, Side::SELL, c.price, c.qty, 0));
    }
    else{
        book.cancelOrder(c.id);
    }
}

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Total Time (microseconds): " << duration << "\n";
    std::cout << "Orders Processed: " << cmds.size() << "\n";
    std::cout << "Avg Latency (nano-seconds): " << ((double)duration / cmds.size())*1000 << "\n";
    std::cout << "Throughput (orders/sec): " << (cmds.size() * 1e6) / duration << "\n";

    return 0;
}