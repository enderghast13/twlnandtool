// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#undef FF_FS_READONLY
#define FF_FS_READONLY 0

#undef FF_FS_RPATH
#define FF_FS_RPATH 2

#undef FF_FS_NORTC
#define FF_FS_NORTC 1

#undef FF_FS_REENTRANT
#define FF_FS_REENTRANT 0

#undef FF_USE_CHMOD
#define FF_USE_CHMOD 1

#undef FF_USE_FORWARD
#define FF_USE_FORWARD 1

#undef FF_USE_FIND
#define FF_USE_FIND 1

#undef FF_USE_EXPAND
#define FF_USE_EXPAND 1

#undef FF_USE_FASTSEEK
#define FF_USE_FASTSEEK 1
