#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include "hw_utils.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// PoolAllocator<T, Capacity>
//
// O(1) allocate / deallocate via an intrusive singly-linked free list.
// All storage is heap-allocated once at construction — zero malloc traffic
// on the hot matching path.
//
// allocate() and deallocate() are force-inlined so the compiler can fold
// them directly into the matching loop with no call overhead.
// ─────────────────────────────────────────────────────────────────────────────
template<typename T, std::size_t Capacity>
class PoolAllocator {
    static_assert(sizeof(T)  >= sizeof(void*), "T too small for intrusive free list");
    static_assert(alignof(T) >= alignof(void*), "T alignment insufficient");

    struct alignas(alignof(T)) Slot { std::byte data[sizeof(T)]; };
    struct FreeNode { FreeNode* next; };

    std::unique_ptr<Slot[]> storage_;
    FreeNode*               free_head_;

public:
    explicit PoolAllocator()
        : storage_(new Slot[Capacity])
        , free_head_(nullptr)
    {
        for (std::size_t i = Capacity; i-- > 0;) {
            auto* node  = reinterpret_cast<FreeNode*>(&storage_[i]);
            node->next  = free_head_;
            free_head_  = node;
        }
    }

    PoolAllocator(const PoolAllocator&)            = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    // O(1): pop from free list — inlined into caller, no call overhead
    [[nodiscard]] ALWAYS_INLINE T* allocate() noexcept {
        if (!free_head_) [[unlikely]] return nullptr;
        FreeNode* node = free_head_;
        free_head_     = node->next;
        return reinterpret_cast<T*>(node);
    }

    // O(1): push back onto free list
    ALWAYS_INLINE void deallocate(T* p) noexcept {
        auto* node  = reinterpret_cast<FreeNode*>(p);
        node->next  = free_head_;
        free_head_  = node;
    }

    static constexpr std::size_t capacity() noexcept { return Capacity; }
};
