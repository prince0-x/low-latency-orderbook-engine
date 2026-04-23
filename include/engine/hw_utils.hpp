#pragma once
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// Portability macros — resolve to hardware instructions on x86-64 with
// -march=native (GCC/Clang) or /arch:AVX2 (MSVC).
// ─────────────────────────────────────────────────────────────────────────────

#if defined(_MSC_VER)
#  include <intrin.h>
#  define ALWAYS_INLINE   __forceinline
#  define HOT_FN
#  define FLATTEN_FN
#elif defined(__GNUC__)|| defined(__clang__)
#  define ALWAYS_INLINE   __attribute__((always_inline)) inline
#  define HOT_FN          __attribute__((hot))
#  define FLATTEN_FN      __attribute__((flatten))
#else
#  define ALWAYS_INLINE   inline
#  define HOT_FN
#  define FLATTEN_FN
#endif

// ─────────────────────────────────────────────────────────────────────────────
// hw::lsb — bit index of the lowest set bit  (hardware: TZCNT)
// hw::msb — bit index of the highest set bit (hardware: LZCNT + inversion)
//
// Both are undefined if x == 0.
// ─────────────────────────────────────────────────────────────────────────────
namespace hw {

ALWAYS_INLINE int lsb(uint64_t x) noexcept {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, x);
    return static_cast<int>(idx);
#else
    return __builtin_ctzll(x);   // → TZCNT with -march=native
#endif
}

ALWAYS_INLINE int msb(uint64_t x) noexcept {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanReverse64(&idx, x);
    return static_cast<int>(idx);
#else
    return 63 - __builtin_clzll(x);  // → LZCNT with -march=native
#endif
}

// Software prefetch hint
//   rw       : 0 = load hint, 1 = store hint
//   locality : 0 = non-temporal (no cache), 1 = L3, 2 = L2, 3 = L1
template<int rw = 0, int locality = 2>
ALWAYS_INLINE void prefetch(const void* addr) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(addr, rw, locality);
#elif defined(_MSC_VER)
    constexpr int hint = (locality == 0) ? _MM_HINT_NTA
                       : (locality == 1) ? _MM_HINT_T2
                       : (locality == 2) ? _MM_HINT_T1
                       :                   _MM_HINT_T0;
    _mm_prefetch(static_cast<const char*>(addr), hint);
#endif
}

} // namespace hw
