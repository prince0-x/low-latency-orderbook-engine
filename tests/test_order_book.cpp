#include <cassert>
#include <iostream>
#include "engine/order_book.hpp"

// ---------------------------------------------------------------------------
// Minimal test harness
// ---------------------------------------------------------------------------
static int g_run = 0, g_pass = 0;

#define EXPECT_EQ(got, want) do { \
    ++g_run; \
    if ((got) == (want)) { ++g_pass; } \
    else { \
        std::cerr << "FAIL " << __func__ << " line " << __LINE__ \
                  << ": expected " << (want) << ", got " << (got) << '\n'; \
    } \
} while (0)

// ---------------------------------------------------------------------------
// Test cases (mirrors the scenarios in src/main_dev.cpp)
// ---------------------------------------------------------------------------

// Case 1: Simple complete match — book is empty afterwards
void test_simple_match() {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY,  100, 5, 1));
    book.addOrder(Order(2, Side::SELL, 100, 5, 2));
    EXPECT_EQ(book.bestBid(), -1);
    EXPECT_EQ(book.bestAsk(), -1);
}

// Case 2: Partial fill — incoming SELL consumed only part of the resting BUY
void test_partial_match() {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY,  100, 10, 1));
    book.addOrder(Order(2, Side::SELL, 100,  4, 2));
    EXPECT_EQ(book.bestBid(), 100);   // 6 remaining
    EXPECT_EQ(book.bestAsk(), -1);
}

// Case 3: Multi-level match — BUY at 105 eats 101 fully then partially 102
void test_multilevel_match() {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 101, 5, 1));
    book.addOrder(Order(2, Side::SELL, 102, 5, 2));
    book.addOrder(Order(3, Side::BUY,  105, 8, 3));   // needs 8: takes all 5@101, 3@102
    EXPECT_EQ(book.bestBid(), -1);    // BUY fully matched
    EXPECT_EQ(book.bestAsk(), 102);   // 2 remaining at 102
}

// Case 4: Cancel a resting order
void test_cancel_existing() {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 10, 1));
    book.cancelOrder(1);
    EXPECT_EQ(book.bestBid(), -1);
}

// Case 5: Cancel an already-matched order — must be a no-op
void test_cancel_matched() {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY,  100, 5, 1));
    book.addOrder(Order(2, Side::SELL, 100, 5, 2));
    book.cancelOrder(1);   // already filled — should not crash or change state
    EXPECT_EQ(book.bestBid(), -1);
    EXPECT_EQ(book.bestAsk(), -1);
}

// Case 6: Cancel a non-existent ID — must not crash
void test_cancel_nonexistent() {
    OrderBook book;
    book.cancelOrder(999);
    EXPECT_EQ(book.bestBid(), -1);
    EXPECT_EQ(book.bestAsk(), -1);
}

// Case 7: FIFO priority — first resting order at a level is matched first
void test_fifo_priority() {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 100, 5, 1));
    book.addOrder(Order(2, Side::BUY, 100, 5, 2));
    // SELL 6: should consume all 5 from order 1 then 1 from order 2
    book.addOrder(Order(3, Side::SELL, 100, 6, 3));
    EXPECT_EQ(book.bestBid(), 100);   // 4 remaining from order 2
    EXPECT_EQ(book.bestAsk(), -1);
}

// Case 8: Bid-ask price priority — best bid matched first
void test_bid_price_priority() {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, 99,  5, 1));
    book.addOrder(Order(2, Side::BUY, 101, 5, 2));   // better bid
    book.addOrder(Order(3, Side::SELL, 99, 5, 3));   // should match against 101 first
    EXPECT_EQ(book.bestBid(), 99);    // order 1 still resting
    EXPECT_EQ(book.bestAsk(), -1);
}

// Case 9: Ask price priority — lowest ask matched first
void test_ask_price_priority() {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, 102, 5, 1));
    book.addOrder(Order(2, Side::SELL, 100, 5, 2));   // better ask
    book.addOrder(Order(3, Side::BUY,  102, 5, 3));   // should match against 100 first
    EXPECT_EQ(book.bestAsk(), 102);   // order 1 still resting
    EXPECT_EQ(book.bestBid(), -1);
}

// Case 10: No cross — orders should rest without matching
void test_no_cross() {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY,  99, 5, 1));
    book.addOrder(Order(2, Side::SELL, 101, 5, 2));
    EXPECT_EQ(book.bestBid(), 99);
    EXPECT_EQ(book.bestAsk(), 101);
}

// ---------------------------------------------------------------------------

int main() {
    test_simple_match();
    test_partial_match();
    test_multilevel_match();
    test_cancel_existing();
    test_cancel_matched();
    test_cancel_nonexistent();
    test_fifo_priority();
    test_bid_price_priority();
    test_ask_price_priority();
    test_no_cross();

    std::cout << g_pass << " / " << g_run << " tests passed\n";
    return (g_pass == g_run) ? 0 : 1;
}
