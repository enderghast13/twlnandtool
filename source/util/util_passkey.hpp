// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

namespace util {
    template<typename Factory>
    class PassKey {
        private:
            friend Factory;
            constexpr PassKey() noexcept { /* need to be explicitely defined until C++20 */ }
            // Note: implicit public default copy ctor
    };
}
