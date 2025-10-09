// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "crypto_sha.hpp"

#include <nettle/sha.h>

namespace crypto {

    void Sha1(u8 outDigest[Sha1DigestSize], const u8 *data, size_t sz) {
        struct sha1_ctx ctx;

        sha1_init(&ctx);
        sha1_update(&ctx, sz, data);
        sha1_digest(&ctx, sz, outDigest);
    }

    void Sha256(u8 outDigest[Sha256DigestSize], const u8 *data, size_t sz) {
        struct sha256_ctx ctx;

        sha256_init(&ctx);
        sha256_update(&ctx, sz, data);
        sha256_digest(&ctx, sz, outDigest);
    }
}
