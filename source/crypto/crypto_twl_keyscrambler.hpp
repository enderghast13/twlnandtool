// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

// Key X/Y/Normal and IV FIFO definitions.
// See comments in crypto_twl.hpp

#pragma once

#include "../util/util_bitutil.hpp"
#include "crypto_aes.hpp"

namespace crypto {

    struct TwlNormalKey;
    struct TwlKeyX;
    struct TwlKeyY;

    namespace detail {
        constexpr TwlNormalKey CombineTwlKeys(const TwlKeyX &keyX, const TwlKeyY &keyY);

        template<typename Derived>
        struct TwlAesFifo {
            alignas(16) u32 words[4];

            ALWAYS_INLINE static constexpr Derived FromRegWrites(u32 a, u32 b, u32 c, u32 d) {
                // Aggregate-init of derived classes not available until C++17
                Derived ret = {};
                ret.words[0] = a;
                ret.words[1] = b;
                ret.words[2] = c;
                ret.words[3] = d;
                return ret;
            }

            ALWAYS_INLINE static constexpr Derived FromRegWrites(const u32 words[4]) {
                return FromRegWrites(words[0], words[1], words[2], words[3]);
            }

            static constexpr Derived FromBytes(const u8 in[16]) {
                Derived ret = {};
                util::ReadLe128(ret.words, in);
                return ret;
            }

            static constexpr Derived FromBytes(const char in[16]) {
                Derived ret = {};
                util::ReadLe128(ret.words, in);
                return ret;
            }

            ALWAYS_INLINE static Derived FromBytes(const void *in) {
                return FromBytes(reinterpret_cast<const u8 *>(in));
            }

            constexpr friend bool operator==(const Derived &lhs, const Derived &rhs) {
                return  lhs.words[0] == rhs.words[0] &&
                        lhs.words[1] == rhs.words[1] &&
                        lhs.words[2] == rhs.words[2] &&
                        lhs.words[3] == rhs.words[3];
            }

            ALWAYS_INLINE constexpr friend bool operator!=(const Derived &lhs, const Derived &rhs) {
                return !(lhs == rhs);
            }
        };
    }

    struct TwlCounter : public detail::TwlAesFifo<TwlCounter> {
        constexpr AesCounter ToAesCounter() const {
            AesCounter ret = {};
            util::WriteBe128(ret.data, words);
            return ret;
        }

        ALWAYS_INLINE operator AesCounter() const { return ToAesCounter(); }

        ALWAYS_INLINE constexpr TwlCounter &Increment(u32 val) {
            // GCC doesn't properly optimize AesCounter (maybe optimizer
            // recursion level too deep?), thus we should rewrite it here
            // (with u32s). words[0] is the lowest significant word of the
            // resulting LE u128.
            TwlCounter old = *this;
            words[0] += val;
            words[1] += words[0] < old.words[0] ? 1 : 0;
            words[2] += words[1] < old.words[1] ? 1 : 0;
            words[3] += words[2] < old.words[2] ? 1 : 0;

            return *this;
        }

        ALWAYS_INLINE constexpr TwlCounter &operator+=(u32 val) { return Increment(val); }
        ALWAYS_INLINE friend constexpr TwlCounter operator+(TwlCounter lhs, u32 rhs) { return lhs.Increment(rhs); }
        ALWAYS_INLINE constexpr TwlCounter &operator++() { return Increment(1); }
        ALWAYS_INLINE constexpr TwlCounter &operator++(int) { TwlCounter tmp = *this; return tmp.Increment(1); }

    };

    struct TwlNormalKey : public detail::TwlAesFifo<TwlNormalKey> {
        constexpr AesKey ToAesKey() const {
            AesKey ret = {};
            util::WriteBe128(ret.data, words);
            return ret;
        }

        ALWAYS_INLINE operator AesKey() const { return ToAesKey(); }
    };

    namespace detail {
        template<typename Derived, typename Other>
        struct TwlPartialKey : public TwlAesFifo<Derived> {
            ALWAYS_INLINE constexpr friend TwlNormalKey operator+(const Derived &lhs, const Other &rhs) {
                return lhs.CombineWithOther(rhs);
            }
        };
    }

    struct TwlKeyX : public detail::TwlPartialKey<TwlKeyX, TwlKeyY> {
        ALWAYS_INLINE constexpr TwlNormalKey CombineWithOther(const TwlKeyY &keyY) const {
            return detail::CombineTwlKeys(*this, keyY);
        }
    };

    struct TwlKeyY : public detail::TwlPartialKey<TwlKeyY, TwlKeyX> {
        ALWAYS_INLINE constexpr TwlNormalKey CombineWithOther(const TwlKeyX &keyX) const {
            return detail::CombineTwlKeys(keyX, *this);
        }
    };

    namespace detail {
        constexpr TwlNormalKey CombineTwlKeys(const TwlKeyX &keyX, const TwlKeyY &keyY) {
            // Key = ((Key_X XOR Key_Y) + FFFEFB4E295902582A680F5F1A4F3E79h) ROL 42

            // XOR step
            u32 step1[4] = {};
            step1[0] = keyX.words[0] ^ keyY.words[0];
            step1[1] = keyX.words[1] ^ keyY.words[1];
            step1[2] = keyX.words[2] ^ keyY.words[2];
            step1[3] = keyX.words[3] ^ keyY.words[3];

            // ADD step
            u32 step2[4] = {};
            step2[0] = step1[0] + 0x1A4F3E79;
            step2[1] = step1[1] + 0x2A680F5F + (step2[0] < step1[0] ? 1 : 0);
            step2[2] = step1[2] + 0x29590258 + (step2[1] < step1[1] ? 1 : 0);
            step2[3] = step1[3] + 0xFFFEFB4E + (step2[2] < step1[2] ? 1 : 0);

            // ROL 42 step, much easier to do with u64 (because 42 < 64)
            u64 step3[2] = {};
            u64 ov = 0;
            step3[0] = static_cast<u64>(step2[1]) << 32 | step2[0];
            step3[1] = static_cast<u64>(step2[3]) << 32 | step2[2];
            ov = (step3[1] >> (64 - 42));
            step3[1] = (step3[1] << 42) | (step3[0] >> (64 - 42));
            step3[0] = (step3[0] << 42) | ov;

            return TwlNormalKey::FromRegWrites(
                static_cast<u32>(step3[0] >>  0),
                static_cast<u32>(step3[0] >> 32),
                static_cast<u32>(step3[1] >>  0),
                static_cast<u32>(step3[1] >> 32)
            );
        }

        template<typename T>
        constexpr T UnhexlifyTwlFifo(const char in[16], size_t sz) {
            u8 data[16] = {};

            if (sz % 2 != 0 || sz < 1 || sz > 32 || !util::Unhexlify(data + (16 - sz/2), in, sz/2, false)) {
                // cannot error in c++14?
                return T::FromBytes(data);
            }
            return T::FromBytes(data);
        }
    }
}

constexpr crypto::TwlNormalKey operator""_TwlNormalKey(const char *in, size_t sz) {
    return crypto::detail::UnhexlifyTwlFifo<crypto::TwlNormalKey>(in, sz);
}

constexpr crypto::TwlKeyX operator""_TwlKeyX(const char *in, size_t sz) {
    return crypto::detail::UnhexlifyTwlFifo<crypto::TwlKeyX>(in, sz);
}

constexpr crypto::TwlKeyY operator""_TwlKeyY(const char *in, size_t sz) {
    return crypto::detail::UnhexlifyTwlFifo<crypto::TwlKeyY>(in, sz);
}

static_assert("484e415050414e486f646e65746e694e"_TwlKeyX + "50782caab7dfa845a86487c142416841"_TwlKeyY == "dfa197c5a3e40d41f8fe2060d5a52443"_TwlNormalKey, "Keyscrambler fail");
