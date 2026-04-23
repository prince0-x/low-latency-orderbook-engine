#pragma once
#include "order_book.hpp"
#include <string>

// ---------------------------------------------------------------------------
// MatchingEngine — thin wrapper that owns an OrderBook and adds file-based
// order ingestion.  Keeps I/O concerns out of the OrderBook core.
// ---------------------------------------------------------------------------
class MatchingEngine {
    OrderBook book_;

public:
    MatchingEngine() = default;

    // Process a file of BUY / SELL / CANCEL commands (see OrderBook::processOrdersFromFile)
    void processFile(const std::string& filename) {
        book_.processOrdersFromFile(filename);
    }

    OrderBook&       book()       noexcept { return book_; }
    const OrderBook& book() const noexcept { return book_; }
};
