#include "engine/order_book.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Construction — one-time heap allocation; zero malloc traffic afterwards
// ─────────────────────────────────────────────────────────────────────────────

OrderBook::OrderBook()
    : bids_        (std::make_unique<PriceLevel[]>(MAX_PRICE))
    , asks_        (std::make_unique<PriceLevel[]>(MAX_PRICE))
    , bid_active_  (std::make_unique<uint64_t[]>(BITSET_WORDS))  // zero-init
    , ask_active_  (std::make_unique<uint64_t[]>(BITSET_WORDS))  // zero-init
    , order_lookup_(std::make_unique<OrderRef[]>(MAX_ORDERS))
    , pool_        (std::make_unique<PoolAllocator<OrderNode, MAX_ORDERS>>())
    , best_bid_(-1)
    , best_ask_(MAX_PRICE)
{}

// ─────────────────────────────────────────────────────────────────────────────
// addOrder
// ─────────────────────────────────────────────────────────────────────────────

HOT_FN void OrderBook::addOrder(const Order& order) {
    Order o = order;

    if (o.side == Side::BUY) {
        matchBuyOrder(o);
    } else {
        matchSellOrder(o);
    }

    if (o.quantity <= 0) [[likely]] return;   // fully matched — common for heavy matching datasets

    OrderNode* node = pool_alloc();
    node->init(o.order_id, o.price, o.quantity, o.side);

    if (o.side == Side::BUY) {
        bids_[o.price].push_back(node);
        set_bid_active(o.price);
        if (o.price > best_bid_) best_bid_ = o.price;
        order_lookup_[o.order_id] = {node, o.price, false};
    } else {
        asks_[o.price].push_back(node);
        set_ask_active(o.price);
        if (o.price < best_ask_) best_ask_ = o.price;
        order_lookup_[o.order_id] = {node, o.price, true};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// cancelOrder — O(1): one array load + doubly-linked-list splice
// ─────────────────────────────────────────────────────────────────────────────

HOT_FN void OrderBook::cancelOrder(int order_id) noexcept {
    if (order_id <= 0 || order_id >= MAX_ORDERS) [[unlikely]] return;

    OrderRef& ref = order_lookup_[order_id];
    if (!ref.node) [[unlikely]] return;

    OrderNode* node  = ref.node;
    int        price = ref.price;

    if (ref.is_ask) {
        asks_[price].remove(node);
        if (asks_[price].empty()) [[unlikely]] {
            clr_ask_active(price);
            if (price == best_ask_) scan_best_ask();
        }
    } else {
        bids_[price].remove(node);
        if (bids_[price].empty()) [[unlikely]] {
            clr_bid_active(price);
            if (price == best_bid_) scan_best_bid();
        }
    }

    pool_free(node);
    ref.node = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// matchBuyOrder
//
// Hardware optimisations in this function:
//   • FLATTEN_FN  — pop_front, pool_free, scan_best_ask all inlined here;
//                   no call overhead, no register save/restore between them.
//   • prefetch    — the order_lookup_ write slot is prefetched for store
//                   while trade_qty arithmetic runs (hides ~40-cycle L3 miss).
//   • [[likely]]  — the fast path (resting order partially filled, loop exits)
//                   is marked likely so the CPU's branch predictor defaults to it.
// ─────────────────────────────────────────────────────────────────────────────

HOT_FN FLATTEN_FN void OrderBook::matchBuyOrder(Order& order) noexcept {
    while (order.quantity > 0
           && best_ask_ < MAX_PRICE
           && best_ask_ <= order.price)
    {
        PriceLevel& level   = asks_[best_ask_];
        OrderNode*  resting = level.head;

        // Prefetch the order_lookup_ entry while we do the arithmetic below.
        // By the time we need to nullify it, the cache line has arrived.
        hw::prefetch</*write*/1, /*L1*/3>(&order_lookup_[resting->order_id]);

        int trade_qty = std::min(order.quantity, resting->quantity);
        order.quantity    -= trade_qty;
        resting->quantity -= trade_qty;

        if (resting->quantity == 0) [[unlikely]] {
            order_lookup_[resting->order_id].node = nullptr;  // cache line already warm
            level.pop_front();
            pool_free(resting);

            if (level.empty()) [[unlikely]] {
                clr_ask_active(best_ask_);
                scan_best_ask();
            }
        }
        // likely path: resting partially filled → order.quantity == 0 → loop exits
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// matchSellOrder — mirror of matchBuyOrder
// ─────────────────────────────────────────────────────────────────────────────

HOT_FN FLATTEN_FN void OrderBook::matchSellOrder(Order& order) noexcept {
    while (order.quantity > 0
           && best_bid_ >= 0
           && best_bid_ >= order.price)
    {
        PriceLevel& level   = bids_[best_bid_];
        OrderNode*  resting = level.head;

        hw::prefetch<1, 3>(&order_lookup_[resting->order_id]);

        int trade_qty = std::min(order.quantity, resting->quantity);
        order.quantity    -= trade_qty;
        resting->quantity -= trade_qty;

        if (resting->quantity == 0) [[unlikely]] {
            order_lookup_[resting->order_id].node = nullptr;
            level.pop_front();
            pool_free(resting);

            if (level.empty()) [[unlikely]] {
                clr_bid_active(best_bid_);
                scan_best_bid();
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// executeTrade — notification hook; not called internally by the hot path
// ─────────────────────────────────────────────────────────────────────────────

void OrderBook::executeTrade(int price, int qty, int buyOrderId, int sellOrderId) {
    std::cout << "TRADE"
              << " price=" << price
              << " qty="   << qty
              << " buy="   << buyOrderId
              << " sell="  << sellOrderId
              << '\n';
}

// ─────────────────────────────────────────────────────────────────────────────
// printOrderBook — debug snapshot (not on hot path)
// ─────────────────────────────────────────────────────────────────────────────

void OrderBook::printOrderBook() const {
    std::cout << "------------ORDER BOOK------------\n";
    std::cout << "ASKS (lowest to highest):\n";
    for (int p = 0; p < MAX_PRICE; ++p) {
        if (asks_[p].empty()) continue;
        std::cout << p << " : ";
        for (OrderNode* n = asks_[p].head; n; n = n->next)
            std::cout << "[id=" << n->order_id << " qty=" << n->quantity << "] ";
        std::cout << '\n';
    }
    std::cout << "BIDS (highest to lowest):\n";
    for (int p = MAX_PRICE - 1; p >= 0; --p) {
        if (bids_[p].empty()) continue;
        std::cout << p << " : ";
        for (OrderNode* n = bids_[p].head; n; n = n->next)
            std::cout << "[id=" << n->order_id << " qty=" << n->quantity << "] ";
        std::cout << '\n';
    }
    std::cout << "------------------------\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// processOrdersFromFile — file I/O wrapper; not on hot path
// ─────────────────────────────────────────────────────────────────────────────

void OrderBook::processOrdersFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string   type;
    int next_id   = 1;
    int timestamp = 1;

    while (file >> type) {
        if (type == "BUY") {
            int price, qty;
            file >> price >> qty;
            addOrder(Order(next_id++, Side::BUY, price, qty, timestamp++));
        } else if (type == "SELL") {
            int price, qty;
            file >> price >> qty;
            addOrder(Order(next_id++, Side::SELL, price, qty, timestamp++));
        } else if (type == "CANCEL") {
            int id;
            file >> id;
            cancelOrder(id);
        }
    }
}
