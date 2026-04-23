#pragma once
#include "types.hpp"
#include "pool_allocator.hpp"
#include "price_level.hpp"
#include "hw_utils.hpp"
#include <cstdint>
#include <memory>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// OrderBook — sub-microsecond limit-order book
//
// Performance stack (target: 1–10 ns / op)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Layer 1 — Algorithmic  (already done in previous version)
//    • Flat PriceLevel[] array → O(1) level access
//    • Intrusive doubly-linked list → O(1) cancel anywhere
//    • Pool allocator → zero malloc/free on hot path
//    • Cached best_bid_ / best_ask_ integers
//
//  Layer 2 — Hardware  (this version)
//    • Active-level bitset + TZCNT/LZCNT intrinsics
//        Level scan: O(BITSET_WORDS/64) → effectively O(1) with hardware
//        bit-count.  Finding the next active level after exhaustion costs
//        one TZCNT instruction instead of a loop over empty PriceLevels.
//    • Software prefetch (hw::prefetch)
//        Issued while trade qty is being computed so the order_lookup_
//        cache line arrives before it is written.
//    • ALWAYS_INLINE on every hot helper
//        pool_alloc, push_back, pop_front, remove, scan_* — all inlined
//        into the matching loop; no call overhead or register spills.
//    • HOT_FN + FLATTEN_FN on matching methods
//        Tells GCC to optimise these functions hardest and inline all
//        callees (pool_alloc, pop_front, scan_best_ask …) into them.
//    • [[likely]] / [[unlikely]] branch hints
//        Common path (partial resting fill) marked likely so the CPU's
//        branch predictor doesn't mispredict on it.
//
//  Layer 3 — Build flags (CMakeLists / compile command)
//    • -O3 -march=native  (auto-vectorise, AVX2, BMI2)
//    • -funroll-loops      (unroll the matching while-loop)
//    • -flto               (inline addOrder into benchmark loop across TUs)
//    • -fomit-frame-pointer (one extra register in the hot loop)
// ─────────────────────────────────────────────────────────────────────────────
class OrderBook {
public:
    static constexpr int MAX_PRICE    = 1 << 15;          // 32 768 price levels
    static constexpr int MAX_ORDERS   = 1 << 19;          // 524 288 order slots
    static constexpr int BITSET_WORDS = MAX_PRICE >> 6;   // 512 × uint64 = 4 KB / side

private:
    // ── Flat price-level arrays (heap, 512 KB / side) ────────────────────────
    std::unique_ptr<PriceLevel[]> bids_;
    std::unique_ptr<PriceLevel[]> asks_;

    // ── Best-price integers (stay in registers across the matching loop) ──────
    int best_bid_ = -1;          // highest active bid price; -1 if empty
    int best_ask_ = MAX_PRICE;   // lowest  active ask price; MAX_PRICE if empty

    // ── Active-level bitsets (heap, 4 KB / side) ─────────────────────────────
    // Bit p is set iff PriceLevel p has at least one resting order.
    // Used by scan_best_ask / scan_best_bid; TZCNT / LZCNT resolve in 1 cycle.
    std::unique_ptr<uint64_t[]> bid_active_;
    std::unique_ptr<uint64_t[]> ask_active_;

    // ── Per-order cancel support (heap, 8 MB) ─────────────────────────────────
    struct OrderRef {
        OrderNode* node  = nullptr;
        int        price = 0;
        bool       is_ask = false;
    };
    std::unique_ptr<OrderRef[]> order_lookup_;

    // ── Memory pool (heap, 16 MB) ─────────────────────────────────────────────
    std::unique_ptr<PoolAllocator<OrderNode, MAX_ORDERS>> pool_;

    // ── Hot-path helpers — all force-inlined ─────────────────────────────────

    ALWAYS_INLINE OrderNode* pool_alloc() noexcept { return pool_->allocate(); }
    ALWAYS_INLINE void       pool_free(OrderNode* n) noexcept { pool_->deallocate(n); }

    // Bitset set / clear — single OR / AND-NOT on a 64-bit word
    ALWAYS_INLINE void set_bid_active(int p) noexcept {
        bid_active_[p >> 6] |=  (uint64_t(1) << (p & 63));
    }
    ALWAYS_INLINE void clr_bid_active(int p) noexcept {
        bid_active_[p >> 6] &= ~(uint64_t(1) << (p & 63));
    }
    ALWAYS_INLINE void set_ask_active(int p) noexcept {
        ask_active_[p >> 6] |=  (uint64_t(1) << (p & 63));
    }
    ALWAYS_INLINE void clr_ask_active(int p) noexcept {
        ask_active_[p >> 6] &= ~(uint64_t(1) << (p & 63));
    }

    // Advance best_ask_ to the next non-empty ask level.
    // Uses TZCNT (hw::lsb) — one instruction per 64-level word scanned.
    ALWAYS_INLINE void scan_best_ask() noexcept {
        clr_ask_active(best_ask_);
        int w   = best_ask_ >> 6;
        int bit = (best_ask_ & 63) + 1;          // start from next bit

        // Remaining bits in the current word above best_ask_
        if (bit < 64) {
            uint64_t tail = ask_active_[w] >> bit;
            if (tail) { best_ask_ = (w << 6) + bit + hw::lsb(tail); return; }
        }
        // Subsequent words
        for (++w; w < BITSET_WORDS; ++w) {
            if (ask_active_[w]) {
                best_ask_ = (w << 6) + hw::lsb(ask_active_[w]); return;
            }
        }
        best_ask_ = MAX_PRICE;
    }

    // Retreat best_bid_ to the next non-empty bid level.
    // Uses LZCNT (hw::msb) — one instruction per 64-level word scanned.
    ALWAYS_INLINE void scan_best_bid() noexcept {
        clr_bid_active(best_bid_);
        int w   = best_bid_ >> 6;
        int bit = best_bid_ & 63;                // bits 0..(bit-1) remain

        // Remaining bits in the current word below best_bid_
        if (bit > 0) {
            // Safe mask: keep bits 0..(bit-1)
            uint64_t lo = bid_active_[w] & (bit < 64 ? ((uint64_t(2) << (bit - 1)) - 1) : ~uint64_t(0));
            if (lo) { best_bid_ = (w << 6) + hw::msb(lo); return; }
        }
        // Previous words
        for (--w; w >= 0; --w) {
            if (bid_active_[w]) {
                best_bid_ = (w << 6) + hw::msb(bid_active_[w]); return;
            }
        }
        best_bid_ = -1;
    }

public:
    OrderBook();
    ~OrderBook() = default;
    OrderBook(const OrderBook&)            = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // ── Core API ─────────────────────────────────────────────────────────────
    HOT_FN void addOrder(const Order& order);
    HOT_FN void cancelOrder(int order_id) noexcept;

    ALWAYS_INLINE int bestBid() const noexcept { return best_bid_; }
    ALWAYS_INLINE int bestAsk() const noexcept {
        return (best_ask_ < MAX_PRICE) ? best_ask_ : -1;
    }

    // ── Matching engine ───────────────────────────────────────────────────────
    HOT_FN FLATTEN_FN void matchBuyOrder(Order& order)  noexcept;
    HOT_FN FLATTEN_FN void matchSellOrder(Order& order) noexcept;

    void executeTrade(int price, int qty, int buyOrderId, int sellOrderId);

    // ── Debug / admin ─────────────────────────────────────────────────────────
    void printOrderBook() const;
    void processOrdersFromFile(const std::string& filename);
};
