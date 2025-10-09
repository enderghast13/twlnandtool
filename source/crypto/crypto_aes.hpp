// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "../util/util_bitutil.hpp"

namespace crypto {

    struct AesKey {
        alignas(16) u8 data[16];

        static constexpr AesKey FromBytes(const u8 in[16]) {
            // reinterpet_cast and memcpy not available in constexpr
            AesKey ret = {};

            util::WriteLe64(ret.data + 0, util::ReadLe64(in + 0));
            util::WriteLe64(ret.data + 8, util::ReadLe64(in + 8));

            return ret;
        }

        static constexpr AesKey FromBytes(const char in[16]) {
            // reinterpet_cast and memcpy not available in constexpr
            AesKey ret = {};

            util::WriteLe64(ret.data + 0, util::ReadLe64(in + 0));
            util::WriteLe64(ret.data + 8, util::ReadLe64(in + 8));

            return ret;
        }

        static AesKey FromBytes(const void *in) {
            return FromBytes(reinterpret_cast<const u8 *>(in));
        }

        constexpr friend bool operator==(const AesKey &lhs, const AesKey &rhs) {
            return util::ReadLe64(lhs.data) == util::ReadLe64(rhs.data) && util::ReadLe64(lhs.data + 8) == util::ReadLe64(rhs.data + 8);
        }

        ALWAYS_INLINE constexpr friend bool operator!=(const AesKey &lhs, const AesKey &rhs) {
            return !(lhs == rhs);
        }
    };

    struct AesCounter {
        alignas(16) u8 data[16];

        static constexpr AesCounter FromBytes(const u8 in[16]) {
            // reinterpet_cast and memcpy not available in constexpr
            AesCounter ret = {};

            util::WriteLe64(ret.data + 0, util::ReadLe64(in + 0));
            util::WriteLe64(ret.data + 8, util::ReadLe64(in + 8));

            return ret;
        }

        static constexpr AesCounter FromBytes(const char in[16]) {
            // reinterpet_cast and memcpy not available in constexpr
            AesCounter ret = {};

            util::WriteLe64(ret.data + 0, util::ReadLe64(in + 0));
            util::WriteLe64(ret.data + 8, util::ReadLe64(in + 8));

            return ret;
        }

        static AesCounter FromBytes(const void *in) {
            return FromBytes(reinterpret_cast<const u8 *>(in));
        }

        ALWAYS_INLINE constexpr AesCounter &Increment(u32 val) {
            u64 q0 = util::ReadBe64(data + 0);
            u64 q1 = util::ReadBe64(data + 8);

            u64 q2 = q1 + val;
            util::WriteBe64(data + 0, q0 + (q2 < q1 ? 1 : 0));
            util::WriteBe64(data + 8, q2);

            return *this;
        }

        ALWAYS_INLINE constexpr AesCounter &operator+=(u32 val) { return Increment(val); }
        ALWAYS_INLINE friend constexpr AesCounter operator+(AesCounter lhs, u32 rhs) { return lhs.Increment(rhs); }
        ALWAYS_INLINE constexpr AesCounter &operator++() { return Increment(1); }
        ALWAYS_INLINE constexpr AesCounter &operator++(int) { AesCounter tmp = *this; return tmp.Increment(1); }

        ALWAYS_INLINE constexpr friend bool operator==(const AesCounter &lhs, const AesCounter &rhs) {
            return util::ReadLe64(lhs.data) == util::ReadLe64(rhs.data) && util::ReadLe64(lhs.data + 8) == util::ReadLe64(rhs.data + 8);
        }

        ALWAYS_INLINE constexpr friend bool operator!=(const AesCounter &lhs, const AesCounter &rhs) {
            return !(lhs == rhs);
        }
    };

    void AesCtr(AesKey key, AesCounter iv, u8 *dst, const u8 *src, size_t sz);
}
