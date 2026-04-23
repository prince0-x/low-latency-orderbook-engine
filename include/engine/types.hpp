#pragma once
#include <cstdint>

// Side of an order in the book
enum class Side : uint8_t {
    BUY,
    SELL
};

// Public-facing Order struct — same layout as before so existing callers compile unchanged
struct Order {
    int       order_id;
    Side      side;
    int       price;
    int       quantity;
    int64_t   timestamp;

    Order(int id, Side s, int p, int q, int64_t ts) noexcept
        : order_id(id), side(s), price(p), quantity(q), timestamp(ts) {}
};
