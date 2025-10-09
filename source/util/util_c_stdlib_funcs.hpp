// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "../defines.hpp"

extern "C" void *_utilAlignedAlloc(size_t alignment, size_t size);
extern "C" void _utilAlignedFree(void *ptr);

namespace util {

    // aligned_alloc is not present in C++'s <cstdlib> till C++17, and
    // not at all with MinGW/MSVC. It is present since C11, except for
    // MinGW/MSVC. Furthermore, IIRC, POSIX's memalign is also missing
    // from MinGW (I might be wrong about this).

    inline void *AlignedAlloc(size_t alignment, size_t size) noexcept {
        // Some platforms don't provide aligned_alloc. When that the case,
        // it is technically UB (outside lifetime model) but in practice this
        // is absolutely fine (compiler will not optimize opaque pointers it
        // can't track the provenance of)
        void *ptr = _utilAlignedAlloc(alignment, size);
        ASSUME_ALIGNED(ptr, alignment);
        return ptr;
    }

    inline void AlignedFree(void *ptr) {
        _utilAlignedFree(ptr);
    }
}
