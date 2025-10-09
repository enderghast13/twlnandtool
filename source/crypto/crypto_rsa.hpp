// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "../util/util_bitutil.hpp"

namespace crypto {

    static constexpr size_t Rsa1024SignatureSize = 128;
    static constexpr size_t Rsa2048SignatureSize = 256;

    /// Computes (m^e) mod n. res is expected to be at least nSize bytes big
    size_t ModExp(u8 *res, const u8 *m, size_t mSize, const u8 *n, size_t nSize, const u8 *e, size_t eSize);

    /// Decrypts and unpads a signature in PCKS#1 v1.5 type=01 format. sig is expected to be at least nSize bytes big
    bool RsaAnyDecryptSignaturePkcs(u8 *sig, size_t &outSigSize, const u8 *m, size_t mSize, const u8 *n, size_t nSize, u32 e);

    /// Decrypts and unpads a signature in PCKS#1 v1.5 type=01 format, and check the unpadded size
    template<size_t N>
    inline bool RsaDecryptSignaturePkcs(u8 *sig, size_t expectedSigSize, const u8 m[N], const u8 n[N], u32 e) {
        u8 sigBuf[N];
        size_t actualSigSize = 0;
        if (!RsaAnyDecryptSignaturePkcs(sigBuf, actualSigSize, m, N, n, N, e) || actualSigSize != expectedSigSize) {
            return false;
        } else {
            std::copy(sigBuf, sigBuf + expectedSigSize, sig);
            return true;
        }
    }
}
