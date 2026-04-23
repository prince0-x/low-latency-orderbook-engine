#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "engine/order_book.hpp"

// ---------------------------------------------------------------------------
// Command record — pre-parsed from the data file so I/O is excluded from the
// timing window entirely
// ---------------------------------------------------------------------------
struct Command {
    int type;   // 0 = BUY, 1 = SELL, 2 = CANCEL
    int id;
    int price;
    int qty;
};

int main() {
    std::cout << "Orderbook benchmark\n";
    std::cout << "-------------------\n";

    // ── Phase 1: load commands (not timed) ──────────────────────────────────
    std::vector<Command> cmds;
    cmds.reserve(300000);

    std::ifstream file("data/orders.txt");
    if (!file.is_open()) {
        std::cerr << "ERROR: could not open data/orders.txt\n";
        return 1;
    }

    std::string type;
    int next_id = 1;

    while (file >> type) {
        if (type == "BUY") {
            int price, qty;
            file >> price >> qty;
            cmds.push_back({0, next_id++, price, qty});
        } else if (type == "SELL") {
            int price, qty;
            file >> price >> qty;
            cmds.push_back({1, next_id++, price, qty});
        } else if (type == "CANCEL") {
            int id;
            file >> id;
            cmds.push_back({2, id, 0, 0});
        }
    }

    std::cout << "Loaded " << cmds.size() << " commands\n";
    if (cmds.empty()) { std::cerr << "No commands loaded\n"; return 1; }

    // ── Phase 2: warm up the book and pool memory ────────────────────────────
    OrderBook book;

    // ── Phase 3: timed run ───────────────────────────────────────────────────
    // No I/O, no modulo checks — pure matching loop
    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& c : cmds) {
        if (c.type == 0) {
            book.addOrder(Order(c.id, Side::BUY,  c.price, c.qty, 0));
        } else if (c.type == 1) {
            book.addOrder(Order(c.id, Side::SELL, c.price, c.qty, 0));
        } else {
            book.cancelOrder(c.id);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    // ── Phase 4: report ──────────────────────────────────────────────────────
    long long us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "\nResults:\n";
    std::cout << "  Total time        : " << us << " µs\n";
    std::cout << "  Commands processed: " << cmds.size() << "\n";
    std::cout << "  Avg latency       : " << (static_cast<double>(us) / cmds.size() * 1000.0)
              << " ns / op\n";
    std::cout << "  Throughput        : "
              << static_cast<long long>(cmds.size() * 1'000'000LL / us)
              << " ops / sec\n";

    return 0;
}
