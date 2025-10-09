// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "../util/util_bitutil.hpp"

#include <array>

namespace crypto {

    namespace detail {

        // C++14 array is not constexpr (fixed in C++17)
        template<typename IntType>
        struct CrcLutType {
            IntType data[256];
        };

        template<typename IntType>
        constexpr auto GenerateReflectedCrcLut(IntType poly) {
            CrcLutType<IntType> lut = {0};
            IntType revPoly = util::ReverseBits(poly);

            for (size_t i = 0; i < 256; i++) {
                IntType r = static_cast<IntType>(i);

                for (size_t j = 0; j < 8; j++) {
                    r = (r >> 1) ^ ((r & 1) != 0 ? revPoly : 0);
                }
                lut.data[i] = r;
            }

            return lut;
        }

        // C++14 doesn't have inline variables, an usage of variable templates will result
        // in duplicated LUTs in multiple TUs
        static constexpr auto Crc16ArcLut = GenerateReflectedCrcLut<u16>(0x8005);
        static constexpr auto Crc32Lut = GenerateReflectedCrcLut<u32>(0x04C11DB7);

        /*template<typename IntType, IntType Poly>
        inline constexpr auto ReflectedCrcLut = GenerateReflectedCrcLut<IntType>(Poly);*/

        template<typename IntType, typename T>
        constexpr IntType ReflectedCrc(const T *data, size_t size, IntType initialValue, IntType finalXorValue, const CrcLutType<IntType> &lut) {
            IntType r = initialValue;
            for (size_t i = 0; i < size; i++) {
                r = (r >> 8) ^ lut.data[(r ^ data[i]) & 0xFF];
            }
            return r ^ finalXorValue;
        }
    }

    /// CRC-16-ARC
    constexpr u16 Crc16Arc(const u8 *data, size_t size) {
        return detail::ReflectedCrc<u16>(data, size, 0xFFFF, 0, detail::Crc16ArcLut);
    }

    /// CRC-16-ARC
    constexpr u16 Crc16Arc(const char *data, size_t size) {
        return detail::ReflectedCrc<u16>(data, size, 0xFFFF, 0, detail::Crc16ArcLut);
    }

    /// CRC-16-ARC
    inline u16 Crc16Arc(const void *data, size_t size) {
        return Crc16Arc(reinterpret_cast<const u8 *>(data), size);
    }

    /// CRC-32
    constexpr u32 Crc32(const u8 *data, size_t size) {
        return detail::ReflectedCrc<u32>(data, size, 0xFFFFFFFF, 0xFFFFFFFF, detail::Crc32Lut);
    }

    /// CRC-32
    constexpr u32 Crc32(const char *data, size_t size) {
        return detail::ReflectedCrc<u32>(data, size, 0xFFFFFFFF, 0xFFFFFFFF, detail::Crc32Lut);
    }

    /// CRC-32
    inline u32 Crc32(const void *data, size_t size) {
        return Crc32(reinterpret_cast<const u8 *>(data), size);
    }

    static_assert(Crc16Arc("test", 4) == 0xDC2E, "CRC-16-ARC test failed");
    static_assert(Crc32("test", 4) == 0xD87F7E0C, "CRC-32 test failed");
}
