// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "crypto_aes.hpp"
#include "crypto_crc.hpp"
#include "crypto_rsa.hpp"
#include "crypto_sha.hpp"
#include "crypto_twl_key_constants.hpp"

#include <thread>

// DSi crypto is a mess: they wired all the AES FIFOs wrong (including data FIFOs!).
// Do note that all the AES registers on the DSi need to be accessed via 32-bit
// accesses only.

// FIFOs as a little-endian 128-bit integer by the AES engine, meaning byteswaps need to
// be done all over the place.

// There is an optimization that can be done for AES-CTR: decrypt/encrypt a bunch of
// zeroes while the file is being read, then do the byteswap+xor altogether

namespace crypto {

    /// Modcrypt algorithm type (unencrypted/retail/dev)
    enum TwlModcryptType {
        TwlModcryptType_None   = 0,
        TwlModcryptType_Retail = 1,
        TwlModcryptType_Dev    = 2,
    };

    /// Modcrypt parameters parsed from rom header.
    struct TwlModcryptParameters {
        TwlNormalKey key;
        TwlCounter arm9SectionIv;
        TwlCounter arm7SectionIv;
        TwlModcryptType type;
        size_t arm9SectionOffset;
        size_t arm9SectionSize; // 0 if modcrypt section is disabled
        size_t arm7SectionOffset;
        size_t arm7SectionSize; // 0 if modcrypt section is disabled
        bool enabled;
    };

    enum TwlFirmwareType {
        TwlFirmwareType_Nand        = 0,
        TwlFirmwareType_Gamecard    = 1, // "ntrboot"
        TwlFirmwareType_Nor         = 2, // aka "nvram"
        TwlFirmwareType_3ds         = 3,
        TwlFirmwareType_Invalid     = 4,
    };

    struct TwlFirmwareParameters {
        TwlNormalKey key;
        TwlCounter arm9SectionIv;
        TwlCounter arm7SectionIv;
        size_t arm9SectionOffset;
        size_t arm9SectionSize;
        u32 arm9SectionRamAddress;
        size_t arm9SectionSizeOnMedium;
        size_t arm7SectionOffset;
        size_t arm7SectionSize;
        u32 arm7SectionRamAddress;
        size_t arm7SectionSizeOnMedium;
        TwlFirmwareType type;
        size_t headerOffset;
        u8 flags;
    };

    /// Derive TWL key X for TWL NAND encryption, given console ID from OTP.
    constexpr TwlKeyX DeriveTwlNandKeyX(u64 consoleId) {
        // Console ID can look like this: 0x08201nnnnnnnn1nn
        u32 hi = static_cast<u32>(consoleId >> 32);
        u32 lo = static_cast<u32>(consoleId >>  0);

        return TwlKeyX::FromRegWrites(lo, lo ^ 0x24EE6906, hi ^ 0xE65B601D, hi);
    }

    /// Derive AES normal key for TWL NAND encryption, given console ID from OTP.
    constexpr TwlNormalKey DeriveTwlNandKey(u64 consoleId, bool isDev3ds = false) {
        return DeriveTwlNandKeyX(consoleId) + (isDev3ds ? TwlNand3dsDevKeyY : TwlNandKeyY);
    }

    /// Derive IV (as u128) for TWL NAND encryption, given NAND CID.
    inline TwlCounter DeriveTwlNandIv(const u8 cid[16]) {
        u8 digest[Sha1DigestSize] = {};

        Sha1(digest, cid, 16);
        return TwlCounter::FromBytes(digest);
    }

    /// Derive all modcrypt parameters from romheader
    constexpr TwlModcryptParameters DeriveModcryptParameters(const u8 romheader[0x1000]) {
        TwlModcryptParameters ret = {};

        u8 unitcode = romheader[0x12];
        u8 twlBaseFlags = romheader[0x1C];
        u8 twlAppFlags = romheader[0x1BF];

        if ((unitcode & 2) == 0) {
            // Unitcode indicates this is a NTR only app,
            // therefore modcrypt is not implemented
            ret.type = TwlModcryptType_None;
            return ret;
        }

        // Derive parameters even if modcrypt is disabled
        ret.enabled = (twlBaseFlags & 2) != 0;

        // Check which type of modcrypt it is
        // Debugger modcrypt or dev app = dev, otherwise retail
        if ((twlBaseFlags & 4) != 0 || (twlAppFlags & 0x80) != 0) {
            ret.type = TwlModcryptType_Dev;
            ret.key = TwlNormalKey::FromBytes(romheader + 0);
        } else {
            // Retail modcrypt. If gamecode is HNAP, then keyX is NintendoHNAPPANH
            // (in a byte view of the FIFO, before conversion to LE u128).
            u32 gamecode = util::ReadLe32(romheader + 0xC);
            u32 emagcode = util::ReadBe32(romheader + 0xC);
            TwlKeyX keyX = TwlKeyX::FromRegWrites(
                util::ReadLe32("Nint"),
                util::ReadLe32("endo"),
                gamecode,
                emagcode
            );
            TwlKeyY keyY = TwlKeyY::FromBytes(romheader + 0x350); // Arm9i HMAC

            ret.type = TwlModcryptType_Retail;
            ret.key = keyX + keyY;
        }

        // Get offsets and sizes
        ret.arm9SectionOffset   = util::ReadLe32(romheader + 0x220);
        ret.arm9SectionSize     = util::ReadLe32(romheader + 0x224);
        ret.arm7SectionOffset   = util::ReadLe32(romheader + 0x228);
        ret.arm7SectionSize     = util::ReadLe32(romheader + 0x22C);

        // Get IVs (offset *not* preapplied)
        // From non-i arm9, arm7 section HMACs
        ret.arm9SectionIv = TwlCounter::FromBytes(romheader + 0x300);
        ret.arm7SectionIv = TwlCounter::FromBytes(romheader + 0x314);

        return ret;
    }

    /// Enable or disable modcrypt in romhdr
    constexpr void SetModcryptEnabledInRomhdr(u8 romheader[0x1000], bool enabled) {
        romheader[0x1C] &= ~2;
        romheader[0x1C] |= enabled ? 2 : 0; // twlBaseFlags

        // Fix header CRC. Do not fix header signature, however
        util::WriteLe16(romheader + 0x15E, Crc16Arc(romheader, 0x15E));
    }

    inline TwlFirmwareParameters DeriveFirmwareParameters(const u8 medium[0x400]) {
        TwlFirmwareParameters ret = {};
        alignas(16) u8 signedBlock[0x74] = {};
        const u8 *header = nullptr;

        // Autodetect firmware type by trying out all moduli
        constexpr size_t headerOffsets[] = { 0x200, 0, 0x200, 0x200 };
        for (ret.type = TwlFirmwareType_Nand; ret.type < TwlFirmwareType_Invalid; ret.type = static_cast<TwlFirmwareType>(ret.type + 1)) {
            header = medium + headerOffsets[ret.type];
            ret.headerOffset = headerOffsets[ret.type];
            if (RsaDecryptSignaturePkcs<Rsa1024SignatureSize>(
                signedBlock,
                sizeof(signedBlock),
                header + 0x100,
                TwlFirmwareModuli[ret.type],
                0x10001
            )) {
                break;
            }
        }

        // Firmware type (+ header offset) is now validated
        ret.arm9SectionOffset       = util::ReadLe32(header + 0x20);
        ret.arm9SectionSize         = util::ReadLe32(header + 0x24);
        ret.arm9SectionRamAddress   = util::ReadLe32(header + 0x28);
        ret.arm9SectionSizeOnMedium = util::ReadLe32(header + 0x2C);

        ret.arm7SectionOffset       = util::ReadLe32(header + 0x30);
        ret.arm7SectionSize         = util::ReadLe32(header + 0x34);
        ret.arm7SectionRamAddress   = util::ReadLe32(header + 0x38);
        ret.arm7SectionSizeOnMedium = util::ReadLe32(header + 0x3C);

        ret.flags = header[0xFF];

        TwlKeyY keyY = TwlKeyY::FromBytes(signedBlock);
        ret.key = TwlFirmwareKeyX + keyY;

        auto makeSectionIv = [](size_t size) {
            u32 sz = static_cast<u32>(size);
            return TwlCounter::FromRegWrites(sz, -sz, ~sz, 0);
        };

        ret.arm9SectionIv = makeSectionIv(ret.arm9SectionSizeOnMedium);
        ret.arm7SectionIv = makeSectionIv(ret.arm7SectionSizeOnMedium);

        return ret;
    }

    /// Process a block of data, mimicking the botched up TWL AES hardware
    ALWAYS_INLINE void ProcessTwlAesCtrBlock(u8 out[16], const u8 in[16], const u8 xorpad[16]) {
        // Input and output data is transferred as LE u128, meaning it is in the wrong
        // byte order while inside the AES engine. For AES-CTR, we have to resort to
        // tricks like pre-encrypting a bunch of zeroes as not to destroy performance
        // out[i] = data[i] ^ xorpad[15 - i]
        // out128 = data128 ^ __builtin_bswap128(xorpad128)

        // GCC fails to optimize the following (1700 MiB/s -> 1100), clang handles it fine
        // util::WriteLe64(out + 0, util::ReadLe64(in + 0) ^ util::ReadBe64(xorpad + 8));
        // util::WriteLe64(out + 8, util::ReadLe64(in + 8) ^ util::ReadBe64(xorpad + 0));

        u64 out64[2];
        u64 in64[2];
        u64 xorpad64[2];

        std::memcpy(xorpad64, xorpad, 16);
        std::memcpy(in64, in, 16);

        // Host endianness doesn't matter here
        out64[0] = in64[0] ^ __builtin_bswap64(xorpad64[1]);
        out64[1] = in64[1] ^ __builtin_bswap64(xorpad64[0]);

        std::memcpy(out, out64, 16);
    }

    /// Encrypt/decrypt data, mimicking the botched up TWL AES hardware, given key, IV, data, and a work buffer
    inline void RunTwlAesCtrInPlace(AesKey key, TwlCounter iv, u8 *data, size_t size, u8 *workbuf, size_t workbufSize) {
        auto &ctr = iv;
        size_t rem = size;
        while (rem > 0) {
            size_t chunkSize = std::min(rem, workbufSize);
            // I initially intended to put the xorpad preparation in another thread,
            // but this just makes the performance worse (or has no effect)

            // Encrypt a bunch of zeroes, then XOR
            std::fill(workbuf, workbuf + workbufSize, 0);
            AesCtr(key, ctr, workbuf, workbuf, chunkSize);

            for (size_t i = 0; i < chunkSize / 16; i++) {
                ProcessTwlAesCtrBlock(data + 16*i, data + 16*i, workbuf + 16*i);
            }

            rem -= chunkSize;
            data += chunkSize;
            ctr += chunkSize / 16;
        }
    }

}
