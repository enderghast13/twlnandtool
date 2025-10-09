// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "../util/util_bitutil.hpp"

namespace crypto {

    static constexpr size_t Sha1DigestSize = 20;
    static constexpr size_t Sha256DigestSize = 32;

    void Sha1(u8 outDigest[Sha1DigestSize], const u8 *data, size_t sz);
    void Sha256(u8 outDigest[Sha256DigestSize], const u8 *data, size_t sz);
}
