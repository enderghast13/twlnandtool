// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "crypto_aes.hpp"

#include <nettle/aes.h>
#include <nettle/ctr.h>

namespace crypto {

    void AesCtr(AesKey key, AesCounter iv, u8 *dst, const u8 *src, size_t sz) {
        struct aes128_ctx ctx;

        aes128_set_encrypt_key(&ctx, key.data); // CTR decryption and encryption are the same. Nettle only uses "encrypt" for CTR
        ctr_crypt(&ctx, reinterpret_cast<nettle_cipher_func *>(aes128_encrypt), AES_BLOCK_SIZE, iv.data, sz, dst, src);
    }

}
