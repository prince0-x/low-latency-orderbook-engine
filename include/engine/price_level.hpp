#pragma once
#include "types.hpp"
#include "hw_utils.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// OrderNode — intrusive doubly-linked list node
//
// 32 bytes, aligned to 32 so two nodes share one 64-byte cache line.
// Doubly-linked prev/next allow O(1) removal from any position (cancel).
// ─────────────────────────────────────────────────────────────────────────────
struct alignas(32) OrderNode {
    int        order_id;
    int        price;
    int        quantity;
    Side       side;
    char       _pad[3];
    OrderNode* prev;
    OrderNode* next;

    ALWAYS_INLINE void init(int id, int p, int q, Side s) noexcept {
        order_id = id;
        price    = p;
        quantity = q;
        side     = s;
        prev = next = nullptr;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// PriceLevel — FIFO queue at one price point
//
// 16 bytes: two levels fit in one 32-byte cache line.
// All operations are O(1) with no heap traffic.
// ─────────────────────────────────────────────────────────────────────────────
struct PriceLevel {
    OrderNode* head = nullptr;
    OrderNode* tail = nullptr;

    ALWAYS_INLINE bool empty() const noexcept { return head == nullptr; }

    // Enqueue at back — FIFO arrival order
    ALWAYS_INLINE void push_back(OrderNode* node) noexcept {
        node->prev = tail;
        node->next = nullptr;
        if (tail) tail->next = node;
        else      head = node;
        tail = node;
    }

    // Dequeue from front — used when head order is fully matched
    ALWAYS_INLINE void pop_front() noexcept {
        OrderNode* old = head;
        head = old->next;
        if (head) head->prev = nullptr;
        else      tail = nullptr;
        old->prev = old->next = nullptr;
    }

    // Remove arbitrary node — used for cancel; O(1) via doubly-linked pointers
    ALWAYS_INLINE void remove(OrderNode* node) noexcept {
        if (node->prev) node->prev->next = node->next;
        else            head = node->next;
        if (node->next) node->next->prev = node->prev;
        else            tail = node->prev;
        node->prev = node->next = nullptr;
    }
};
