// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "defines.hpp"
#include "util/util_fileutil.hpp"
#include "util/util_c_stdlib_funcs.hpp"
#include "crypto/crypto_aes.hpp"

/// Estimated L2 cache size, should be what a lot of recent x64 CPUs have.
static constexpr size_t EstimatedL2CacheSize = 512u << 10;

/// Work buffer alignment (must be at least 16 and a divisor of allocated size).
/// Choose 64 because it is a common cache line size.
static constexpr size_t WorkBufferAlignment  = 64u;

/// @brief Work buffer size.
/// @note  Chosen so that xorpad in \ref crypto::RunTwlAesCtrInPlace often exactly
///        match the L2 cache size; if it exceeds it, then performance significatively
///        decreases. It shouldn't be too small, either (to reduce the number of fread calls).
///        This is the single most important optimization point in this project.
///        The second most important optimization is getting \ref crypto::RunTwlAesCtrInPlace
///        codegen right.
static constexpr size_t WorkBufferSize       = 2 * EstimatedL2CacheSize;

inline util::ScopedBuffer AllocateWorkBuffer(size_t size = WorkBufferSize) {
    u8 *ptr = static_cast<u8 *>(util::AlignedAlloc(WorkBufferAlignment, size));
    return util::ScopedBuffer(ptr, util::AlignedFree);
}

int CommandNandcrypt(const char *inFilename, const char *outFilename);
int CommandModcrypt(const char *inFilename, const char *outFilename);
int CommandFirmware(const char *inFilename, const char *outPrefix);
int CommandSyslog(const char *inFilename);
