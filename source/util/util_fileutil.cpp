// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "util_fileutil.hpp"
#include "util_c_stdlib_funcs.hpp"
#include <cstdlib>
#include <cerrno>

#if defined(_WIN32) || defined(__MINGW__)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace util {

    int MakeDirectory(const char *path, bool ignoreExists) {
        int ret = 0;
#if defined(_WIN32) || defined(__MINGW__)
        ret = _mkdir(path);
#else
        ret = mkdir(path, 0755);
#endif
        if (ignoreExists && ret < 0 && errno == EEXIST) {
            ret = 0;
        }

        return ret;
    }

int EnsureDirectoryCreated(std::string path) {
#ifndef _WIN32
    // Expand "~" at start

    if (!path.empty() && path[0] == '~' && (path.size() == 1 || path[1] == '/')) {
        const char *home = std::getenv("HOME");
        if (!home || home[0] == '\0') {
            return -1;
        }
        path.replace(0, 1, home);
    }
#endif

    // Normalized copy for scanning: '\' -> '/'
    std::string norm = path;
    for (char &c : norm) {
        if (c == '\\') c = '/';
    }

    // Determine scan start after root
    size_t start = 0;
    if (norm.size() >= 2 && norm[0] == '/' && norm[1] == '/') {
        // UNC //server/share/...
        size_t i = 2;
        size_t serverEnd = norm.find('/', i);
        if (serverEnd == std::string::npos) {
            return 0;
        }
        size_t shareEnd = norm.find('/', serverEnd + 1);
        if (shareEnd == std::string::npos) {
            return 0;
        }
        // Start after //server/share/
        start = shareEnd + 1;
    } else if (norm.size() >= 2 && std::isalpha((unsigned char)norm[0]) && norm[1] == ':') {
        // C:/...
        start = (norm.size() >= 3 && norm[2] == '/') ? 3 : 2; // skip "C:" or "C:/"
    } else if (!norm.empty() && norm[0] == '/') {
        // POSIX absolute path
        start = 1;
    }

    int ret = 0;
    size_t pos = norm.find('/', start);
    while (pos != std::string::npos) {
        // Skip root-only prefixes "/" and "C:"
        if (!(pos == 0 || (pos == 2 && std::isalpha((unsigned char)norm[0]) && norm[1] == ':'))) {
            ret = MakeDirectory(path.substr(0, pos), true);
            if (ret < 0) {
                return ret;
            }
        }
        // Skip consecutive separators
        size_t next = pos + 1;
        while (next < norm.size() && norm[next] == '/') {
            ++next;
        }
        pos = norm.find('/', next);
    }
    return ret;
}

    ScopedFile OpenFile(const char *filename, const char *mode) {
        return ScopedFile(std::fopen(filename, mode), &std::fclose);
    }

    ScopedFile OpenFileAndGetSize(size_t &outSize, const char *filename, const char *mode) {
        FILE *f = std::fopen(filename, mode);
        outSize = 0;

        if (f) {
            std::fseek(f, 0, SEEK_END);
            outSize = std::ftell(f);
            std::rewind(f);
        }

        return ScopedFile(f, &std::fclose);
    }

    ScopedBuffer ReadFileContents(FILE *f, size_t offset, size_t size) {
        u8 *buffer = nullptr;

        int rc = std::fseek(f, offset, SEEK_SET);
        if (rc >= 0) {
            // Note: std::aligned_alloc not present in C++ till C++17
            // Align to 16B (same as we do for work buffer)
            buffer = reinterpret_cast<u8 *>(util::AlignedAlloc(16, size));
            if (buffer) {
                size_t rd = std::fread(buffer, 1, size, f);
                if (rd != size) {
                    util::AlignedFree(buffer);
                    buffer = nullptr;
                }
            }
        }

        return ScopedBuffer(buffer, &util::AlignedFree);
    }

    int ReadFileContentsInto(void *out, FILE *f, size_t offset, size_t size) {
        int rc = std::fseek(f, offset, SEEK_SET);
        if (rc < 0) {
            return rc;
        }

        return std::fread(out, 1, size, f) == size ? 0 : -1;
    }

    int WriteFileContentsFrom(FILE *f, const void *in, size_t offset, size_t size) {
        int rc = fseek(f, offset, SEEK_SET);
        if (rc < 0) {
            return rc;
        }

        return std::fwrite(in, 1, size, f) == size ? 0 : -1;
    }

    int DumpToFile(const char *filename, const void *in, size_t size, const char *mode) {
        FILE *f = std::fopen(filename, mode);

        int ret = f && std::fwrite(in, 1, size, f) == size ? 0 : -1;

        if (f) {
            std::fclose(f);
        }

        return ret;
    }

}
