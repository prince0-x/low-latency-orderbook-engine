#pragma once
#include <cstdint>

enum class Side {
    BUY,
    SELL
};

struct Order {
    int order_id;
    Side side;
    int price;
    int quantity;
    std::int64_t timestamp;

    Order(int id, Side s, int p, int q, std::int64_t ts)
        : order_id(id), side(s), price(p), quantity(q), timestamp(ts) {}
};