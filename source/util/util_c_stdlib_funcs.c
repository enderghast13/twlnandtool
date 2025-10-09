// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include <stdlib.h>

#if defined(_WIN32) || defined(__MINGW__)

void *_utilAlignedAlloc(size_t alignment, size_t size) {
    return _aligned_malloc(size, alignment);
}

void _utilAlignedFree(void *ptr) {
    _aligned_free(ptr);
}

#else

void *_utilAlignedAlloc(size_t alignment, size_t size) {
    return aligned_alloc(alignment, size);
}

void _utilAlignedFree(void *ptr) {
    free(ptr);
}

#endif
