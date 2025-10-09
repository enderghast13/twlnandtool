// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "crypto_rsa.hpp"
#include <gmp.h>

namespace {
    /// Imports a big integer from bytes, assuming big-endian byte order
    inline void ImportBigNum(mpz_t out, const u8 *in, size_t inSize) {
        mpz_init(out);
        mpz_import(out, inSize, 1, 1, 0, 0, in);
    }

    /// Exports a big integer to bytes, in big-endian byte order. Returns actual number of bytes (max is checked by caller)
    inline size_t ExportBigNum(u8 *out, const mpz_t in) {
        size_t wr = 0;
        mpz_export(out, &wr, 1, 1, 0, 0, in);
        return wr;
    }
}

namespace crypto {

    size_t ModExp(u8 *res, const u8 *m, size_t mSize, const u8 *n, size_t nSize, const u8 *e, size_t eSize) {
        // resSize is constrained by nSize
        mpz_t mBig;
        mpz_t nBig;
        mpz_t eBig;
        mpz_t resBig;

        ImportBigNum(mBig, m, mSize);
        ImportBigNum(nBig, n, nSize);
        ImportBigNum(eBig, e, eSize);
        mpz_init(resBig);

        mpz_powm(resBig, mBig, eBig, nBig);

        size_t ret = ExportBigNum(res, resBig);

        mpz_clears(mBig, nBig, eBig, resBig, nullptr);

        return ret;
    }

    bool RsaAnyDecryptSignaturePkcs(u8 *sig, size_t &outSigSize, const u8 *m, size_t mSize, const u8 *n, size_t nSize, u32 e) {
        // Crypto libs don't let us interact with the unpadded signature directly nor do textbook RSA, meaning
        // we have to reimplement this ourselves

        // Alias (reuse output buffer to avoid alloc, as it it sufficienctly sized)
        // sig is expected to be nSize
        u8 *padded = sig;

        u8 eBuf[4] = {};
        util::WriteBe32(eBuf, e);

        size_t paddedSize = ModExp(padded, m, mSize, n, nSize, eBuf, 4);

        // Padded signature must be in the format 00 01 FF ... FF 00 || D, and the leading 00 has been removed
        // when exporting the big integer
        if (padded[0] != 0x01) {
            return false;
        }

        size_t numPad;
        for (numPad = 0; 1 + numPad < paddedSize; numPad++) {
            if (padded[1 + numPad] == 0x00) {
                break;
            } else if (padded[1 + numPad] != 0xFF) {
                return false;
            }
        }

        // Must at least have 8 bytes of padding
        if (numPad < 8) {
            return false;
        }

        size_t off = 1 + numPad + 1;
        outSigSize = paddedSize - off;
        std::copy(padded + off, padded + paddedSize, sig);

        return true;
    }

}
