// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "../defines.hpp"

namespace util {

    constexpr bool CheckBounds(uintptr_t addrOrOffset, size_t size, uintptr_t startAddrOrOffset, size_t maxSizeFromStart) {
        return
            addrOrOffset >= startAddrOrOffset &&
            addrOrOffset + size >= addrOrOffset &&
            addrOrOffset + size <= startAddrOrOffset + maxSizeFromStart;
    }

    ALWAYS_INLINE bool CheckBounds(const volatile void *addr, size_t size, const volatile void *startAddr, size_t maxSizeFromStart) {
        return CheckBounds(reinterpret_cast<uintptr_t>(addr), size, reinterpret_cast<uintptr_t>(startAddr), maxSizeFromStart);
    }

    static_assert(sizeof(uintptr_t) == sizeof(size_t), "Memory model not linear");
}
