// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "../defines.hpp"

namespace util {

    ALWAYS_INLINE constexpr u64 ReadBe64(const u8 buf[8]) {
        // Must unroll this, otherwise GCC has trouble
        // optimizing
        return
            static_cast<u64>(buf[0]) << 56 |
            static_cast<u64>(buf[1]) << 48 |
            static_cast<u64>(buf[2]) << 40 |
            static_cast<u64>(buf[3]) << 32 |
            static_cast<u64>(buf[4]) << 24 |
            static_cast<u64>(buf[5]) << 16 |
            static_cast<u64>(buf[6]) <<  8 |
            static_cast<u64>(buf[7]);
    }

    ALWAYS_INLINE constexpr u32 ReadBe32(const u8 buf[4]) {
        return
            static_cast<u32>(buf[0]) << 24 |
            static_cast<u32>(buf[1]) << 16 |
            static_cast<u32>(buf[2]) <<  8 |
            static_cast<u32>(buf[3]);
    }

    ALWAYS_INLINE constexpr u16 ReadBe16(const u8 buf[2]) {
        return
            static_cast<u16>(buf[0]) << 8 |
            static_cast<u16>(buf[1]);
    }

    ALWAYS_INLINE constexpr u64 ReadLe64(const u8 buf[8]) {
        // Must unroll this, otherwise GCC has trouble
        // optimizing
        return
            static_cast<u64>(buf[7]) << 56 |
            static_cast<u64>(buf[6]) << 48 |
            static_cast<u64>(buf[5]) << 40 |
            static_cast<u64>(buf[4]) << 32 |
            static_cast<u64>(buf[3]) << 24 |
            static_cast<u64>(buf[2]) << 16 |
            static_cast<u64>(buf[1]) <<  8 |
            static_cast<u64>(buf[0]);
    }

    ALWAYS_INLINE constexpr u32 ReadLe32(const u8 buf[4]) {
        return
            static_cast<u32>(buf[3]) << 24 |
            static_cast<u32>(buf[2]) << 16 |
            static_cast<u32>(buf[1]) <<  8 |
            static_cast<u32>(buf[0]);
    }

    ALWAYS_INLINE constexpr u16 ReadLe16(const u8 buf[2]) {
        return
            static_cast<u16>(buf[1]) << 8 |
            static_cast<u16>(buf[0]);
    }

    ALWAYS_INLINE constexpr void WriteBe64(u8 buf[8], u64 val) {
        // Must unroll this, otherwise GCC has trouble
        // optimizing
        buf[0] = static_cast<u8>(val >> 56);
        buf[1] = static_cast<u8>(val >> 48);
        buf[2] = static_cast<u8>(val >> 40);
        buf[3] = static_cast<u8>(val >> 32);
        buf[4] = static_cast<u8>(val >> 24);
        buf[5] = static_cast<u8>(val >> 16);
        buf[6] = static_cast<u8>(val >>  8);
        buf[7] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteBe32(u8 buf[4], u32 val) {
        buf[0] = static_cast<u8>(val >> 24);
        buf[1] = static_cast<u8>(val >> 16);
        buf[2] = static_cast<u8>(val >>  8);
        buf[3] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteBe16(u8 buf[2], u16 val) {
        buf[0] = static_cast<u8>(val >> 8);
        buf[1] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteLe64(u8 buf[8], u64 val) {
        // Must unroll this, otherwise GCC has trouble
        // optimizing
        buf[7] = static_cast<u8>(val >> 56);
        buf[6] = static_cast<u8>(val >> 48);
        buf[5] = static_cast<u8>(val >> 40);
        buf[4] = static_cast<u8>(val >> 32);
        buf[3] = static_cast<u8>(val >> 24);
        buf[2] = static_cast<u8>(val >> 16);
        buf[1] = static_cast<u8>(val >>  8);
        buf[0] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteLe32(u8 buf[4], u32 val) {
        buf[3] = static_cast<u8>(val >> 24);
        buf[2] = static_cast<u8>(val >> 16);
        buf[1] = static_cast<u8>(val >>  8);
        buf[0] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteLe16(u8 buf[2], u16 val) {
        buf[1] = static_cast<u8>(val >> 8);
        buf[0] = static_cast<u8>(val);
    }

    // Need to repeat this boilerplate for char, because
    // reinterpret_cast is not allowed in constexpr, and we
    // don't have access to "require/if constexpr" in C++14.

    ALWAYS_INLINE constexpr u64 ReadBe64(const char buf[8]) {
        // Must unroll this, otherwise GCC has trouble
        // optimizing
        return
            static_cast<u64>(buf[0]) << 56 |
            static_cast<u64>(buf[1]) << 48 |
            static_cast<u64>(buf[2]) << 40 |
            static_cast<u64>(buf[3]) << 32 |
            static_cast<u64>(buf[4]) << 24 |
            static_cast<u64>(buf[5]) << 16 |
            static_cast<u64>(buf[6]) <<  8 |
            static_cast<u64>(buf[7]);
    }

    ALWAYS_INLINE constexpr u32 ReadBe32(const char buf[4]) {
        return
            static_cast<u32>(buf[0]) << 24 |
            static_cast<u32>(buf[1]) << 16 |
            static_cast<u32>(buf[2]) <<  8 |
            static_cast<u32>(buf[3]);
    }

    ALWAYS_INLINE constexpr u16 ReadBe16(const char buf[2]) {
        return
            static_cast<u16>(buf[0]) << 8 |
            static_cast<u16>(buf[1]);
    }

    ALWAYS_INLINE constexpr u64 ReadLe64(const char buf[8]) {
        // Must unroll this, otherwise GCC has trouble
        // optimizing
        return
            static_cast<u64>(buf[7]) << 56 |
            static_cast<u64>(buf[6]) << 48 |
            static_cast<u64>(buf[5]) << 40 |
            static_cast<u64>(buf[4]) << 32 |
            static_cast<u64>(buf[3]) << 24 |
            static_cast<u64>(buf[2]) << 16 |
            static_cast<u64>(buf[1]) <<  8 |
            static_cast<u64>(buf[0]);
    }

    ALWAYS_INLINE constexpr u32 ReadLe32(const char buf[4]) {
        return
            static_cast<u32>(buf[3]) << 24 |
            static_cast<u32>(buf[2]) << 16 |
            static_cast<u32>(buf[1]) <<  8 |
            static_cast<u32>(buf[0]);
    }

    ALWAYS_INLINE constexpr u16 ReadLe16(const char buf[2]) {
        return
            static_cast<u16>(buf[1]) << 8 |
            static_cast<u16>(buf[0]);
    }

    ALWAYS_INLINE constexpr void WriteBe64(char buf[8], u64 val) {
        // Must unroll this, otherwise GCC has trouble
        // optimizing
        buf[0] = static_cast<u8>(val >> 56);
        buf[1] = static_cast<u8>(val >> 48);
        buf[2] = static_cast<u8>(val >> 40);
        buf[3] = static_cast<u8>(val >> 32);
        buf[4] = static_cast<u8>(val >> 24);
        buf[5] = static_cast<u8>(val >> 16);
        buf[6] = static_cast<u8>(val >>  8);
        buf[7] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteBe32(char buf[4], u32 val) {
        buf[0] = static_cast<u8>(val >> 24);
        buf[1] = static_cast<u8>(val >> 16);
        buf[2] = static_cast<u8>(val >>  8);
        buf[3] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteBe16(char buf[2], u16 val) {
        buf[0] = static_cast<u8>(val >> 8);
        buf[1] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteLe64(char buf[8], u64 val) {
        // Must unroll this, otherwise GCC has trouble
        // optimizing
        buf[7] = static_cast<u8>(val >> 56);
        buf[6] = static_cast<u8>(val >> 48);
        buf[5] = static_cast<u8>(val >> 40);
        buf[4] = static_cast<u8>(val >> 32);
        buf[3] = static_cast<u8>(val >> 24);
        buf[2] = static_cast<u8>(val >> 16);
        buf[1] = static_cast<u8>(val >>  8);
        buf[0] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteLe32(char buf[4], u32 val) {
        buf[3] = static_cast<u8>(val >> 24);
        buf[2] = static_cast<u8>(val >> 16);
        buf[1] = static_cast<u8>(val >>  8);
        buf[0] = static_cast<u8>(val);
    }

    ALWAYS_INLINE constexpr void WriteLe16(char buf[2], u16 val) {
        buf[1] = static_cast<u8>(val >> 8);
        buf[0] = static_cast<u8>(val);
    }

    /// For TWL AES FIFOs handling, low word first
    constexpr void ReadLe128(u32 outVal[4], const u8 buf[16]) {
        outVal[0] = util::ReadLe32(buf +  0);
        outVal[1] = util::ReadLe32(buf +  4);
        outVal[2] = util::ReadLe32(buf +  8);
        outVal[3] = util::ReadLe32(buf + 12);
    }

    /// For TWL AES FIFOs handling, low word first
    constexpr void ReadLe128(u32 outVal[4], const char buf[16]) {
        outVal[0] = util::ReadLe32(buf +  0);
        outVal[1] = util::ReadLe32(buf +  4);
        outVal[2] = util::ReadLe32(buf +  8);
        outVal[3] = util::ReadLe32(buf + 12);
    }

    /// For TWL AES FIFOs handling, low word first
    constexpr void WriteBe128(u8 buf[16], const u32 val[4]) {
        util::WriteBe32(buf +  0, val[3]);
        util::WriteBe32(buf +  4, val[2]);
        util::WriteBe32(buf +  8, val[1]);
        util::WriteBe32(buf + 12, val[0]);
    }

    /// For TWL AES FIFOs handling, low word first
    constexpr void WriteBe128(char buf[16], const u32 val[4]) {
        util::WriteBe32(buf +  0, val[3]);
        util::WriteBe32(buf +  4, val[2]);
        util::WriteBe32(buf +  8, val[1]);
        util::WriteBe32(buf + 12, val[0]);
    }

    /// For TWL AES FIFOs handling, low u64 first
    constexpr void WriteBe128(u8 buf[16], const u64 val[2]) {
        util::WriteBe64(buf + 0, val[1]);
        util::WriteBe64(buf + 8, val[0]);
    }

    /// For TWL AES FIFOs handling, low u64 first
    constexpr void WriteBe128(char buf[16], const u64 val[2]) {
        util::WriteBe64(buf + 0, val[1]);
        util::WriteBe64(buf + 8, val[0]);
    }

    constexpr int ReadHexDigit(char x) {
        if (x >= '0' && x <= '9') {
            return x - '0';
        } else if (x >= 'a' && x <= 'f') {
            return x - 'a' + 10;
        } else if (x >= 'A' && x <= 'F') {
            return x - 'A' + 10;
        } else {
            return -1;
        }
    }

    constexpr void WriteHexDigit(char *out, u8 x, bool uppercase = true) {
        if (x <= 9) {
            *out = '0' + x;
        } else {
            *out = (uppercase ? 'A' : 'a') + (x - 10);
        }
    }

    constexpr bool Unhexlify(u8 *out, const char *in, size_t sz, bool bigEndian = true) {
        for (size_t i = 0; i < sz; i++) {
            size_t idx = bigEndian ? i : (sz - 1) - i;
            int dig1 = ReadHexDigit(in[2*i + 0]);
            int dig2 = ReadHexDigit(in[2*i + 1]);
            if (dig1 < 0 || dig2 < 0) {
                // Parse error
                return false;
            }
            out[idx] = static_cast<u8>(dig1 << 4) | static_cast<u8>(dig2);
        }

        return true;
    }

    constexpr void Hexlify(char *out, const void *in, size_t sz, bool bigEndian = true, bool uppercase = true) {
        for (size_t i = 0; i < sz; i++) {
            u8 b = static_cast<const u8 *>(in)[bigEndian ? i : (sz - 1) - i];
            WriteHexDigit(out + 2*i + 0, (b >> 4), uppercase);
            WriteHexDigit(out + 2*i + 1, (b & 15), uppercase);
        }
        out[2 * sz] = '\0';
    }

    template<typename IntType>
    constexpr IntType ReverseBits(IntType n) {
        // https://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
        IntType mask = ~IntType{0};
        for (unsigned int shift = (CHAR_BIT * sizeof(IntType)) / 2; shift > 0; shift /= 2) {
            mask ^= mask << shift;
            n = ((n >> shift) & mask) | ((n << shift) & ~mask);
        }
        return n;
    }
    static_assert(ReverseBits<u16>(0xA001) == 0x8005, "ReverseBits test failed");
    static_assert(ReverseBits<u32>(0x04C11DB7) == 0xEDB88320, "ReverseBits test failed");
}
