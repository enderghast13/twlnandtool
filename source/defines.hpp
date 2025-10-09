// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once
#include "types.hpp"

#include <type_traits>

// C headers
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cctype>
#include <cinttypes>

// C++ headers
#include <algorithm>
#include <iterator>
#include <limits>
#include <atomic>
#include <utility>

/// Creates a bitmask from a bit number.
#ifndef BIT
#define BIT(n) (1u<<(n))
#endif

/// Creates a bitmask from a bit number (64-bit).
#ifndef BIT64
#define BIT64(n) (1ull<<(n))
#endif

/// Creates a bitmask representing the n least significant bits.
#ifndef MASK
#define MASK(n) (BIT(n) - 1u)
#endif

/// Creates a bitmask representing the n least significant bits (64-bit).
#ifndef MASK64
#define MASK64(n) (BIT64(n) - 1ull)
#endif

/// Creates a bitmask for bit range extraction.
#ifndef MASK2
#define MASK2(a,b) (MASK((a) + 1u) & ~MASK(b))
#endif

/// Creates a bitmask for bit range extraction (64-bit).
#ifndef MASK2_64
#define MASK2_64(a,b) (MASK64((a) + 1ull) & ~MASK64(b))
#endif


#define NON_COPYABLE(cls) \
    cls(const cls&) = delete; \
    cls& operator=(const cls&) = delete

#define NON_MOVEABLE(cls) \
    cls(cls&&) = delete; \
    cls& operator=(cls&&) = delete

#define INFINITE_LOOP() do { __asm__ __volatile__("" ::: "memory"); } while (1)

#define ALWAYS_INLINE inline __attribute__((always_inline))
#define ALWAYS_INLINE_LAMBDA __attribute__((always_inline))

#define BITSIZEOF(x) (sizeof(x) * CHAR_BIT)

#define ASSUME(expr) do { if (!static_cast<bool>((expr))) { __builtin_unreachable(); } } while (0)
#define ASSUME_ALIGNED(ptr, algn) ASSUME((reinterpret_cast<uintptr_t>((ptr)) & (algn - 1)) == 0)
#define ASSUME_NO_OVERFLOW(ptr, sz) ASSUME(reinterpret_cast<uintptr_t>(ptr) + sz >= reinterpret_cast<uintptr_t>(ptr))
