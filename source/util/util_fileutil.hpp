// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "../defines.hpp"
#include <memory>
#include <cstdio>
#include <string>

using std::FILE;

namespace util {

    using ScopedFile = std::unique_ptr<FILE, int (*)(FILE *)>;
    using ScopedBuffer = std::unique_ptr<u8[], void (*)(void *)>;

    /// Mkdir wrapper
    int MakeDirectory(const char *path, bool ignoreExists = true);
    inline int MakeDirectory(const std::string &path, bool ignoreExists = true) { return MakeDirectory(path.c_str(), ignoreExists); }

    /// Create a directory if needed (dir paths must end with '/' or '\'). This is intended for files where it creates the containing directory
    int EnsureDirectoryCreated(std::string path);

    /// Open a file.
    ScopedFile OpenFile(const char *filename, const char *mode = "rb");

    /// Open a file and get its size (using rewind).
    ScopedFile OpenFileAndGetSize(size_t &outSize, const char *filename, const char *mode = "rb");

    /// Read from a file, given offset and size.
    ScopedBuffer ReadFileContents(FILE *f, size_t offset, size_t size);

    /// Read from a file, given offset and size.
    int ReadFileContentsInto(void *out, FILE *f, size_t offset, size_t size);

    /// Write to a file, given offset and size.
    int WriteFileContentsFrom(FILE *f, const void *in, size_t offset, size_t size);

    /// Open a file and write to it, then close the file
    int DumpToFile(const char *filename, const void *in, size_t size, const char *mode = "wb+");

    inline int DumpToFile(const std::string &filename, const void *in, size_t size, const char *mode = "wb+") {
        return DumpToFile(filename.c_str(), in, size, mode);
    }
}
