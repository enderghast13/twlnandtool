// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "commands.hpp"
#include "crypto/crypto_twl.hpp"
#include "util/util_fileutil.hpp"
#include "util/util_bound_checks.hpp"

#include <cstdio>
#include <cstdlib>
#include <thread>

namespace {
    void PrintModcryptParams(const crypto::TwlModcryptParameters &params, const u8 romhdr[0x1000]) {
        char keyHex[33];
        char iv9Hex[33];
        char iv7Hex[33];
        char gamecode[5] = {};

        crypto::AesKey key = params.key;
        crypto::AesCounter iv9 = params.arm9SectionIv;
        crypto::AesCounter iv7 = params.arm7SectionIv;

        util::Hexlify(keyHex, key.data, 16);
        util::Hexlify(iv9Hex, iv9.data, 16);
        util::Hexlify(iv7Hex, iv7.data, 16);
        std::copy(romhdr + 12, romhdr + 16, gamecode);

        std::printf("Gamecode:                     %s\n", gamecode);
        std::printf("Encryption key:               %s\n\n", keyHex);

        if (params.arm9SectionSize != 0) {
            std::printf("Arm9 modcrypt section IV:     %s\n", iv9Hex);
            std::printf("Arm9 modcrypt section offset: 0x%zX\n", params.arm9SectionOffset);
            std::printf("Arm9 modcrypt section size:   0x%zX\n", params.arm9SectionSize);
        } else {
            std::printf("Arm9 modcrypt section:        <none>\n");
        }

        std::printf("\n");
        if (params.arm7SectionSize != 0) {
            std::printf("Arm7 modcrypt section IV:     %s\n", iv7Hex);
            std::printf("Arm7 modcrypt section offset: 0x%zX\n", params.arm7SectionOffset);
            std::printf("Arm7 modcrypt section size:   0x%zX\n", params.arm7SectionSize);
        } else {
            std::printf("Arm7 modcrypt section:        <none>\n");
        }
    }

    bool ValidateSection(const char *what, size_t offset, size_t size, size_t fileSize) {
        if (size == 0) {
            // Empty modcrypt sections are skipped
            return true;
        }

        if (offset % 16 != 0) {
            std::fprintf(stderr, "%s section offset is not aligned to a 16B boundary\n", what);
            return false;
        }

        if (size % 16 != 0) {
            std::fprintf(stderr, "%s section size is not aligned to a 16B boundary\n", what);
            return false;
        }

        if (!util::CheckBounds(offset, size, 0x4000, fileSize)) {
            std::fprintf(stderr, "%s section is out of bounds", what);
            return false;
        }

        return true;
    }
}

int CommandModcrypt(const char *inFilename, const char *outFilename) {
    size_t inFileSize = 0;
    util::ScopedFile inFile = util::OpenFileAndGetSize(inFileSize, inFilename, "rb");

    util::ScopedBuffer workbuf = AllocateWorkBuffer();
    if (!workbuf) {
        std::fprintf(stderr, "Failed to allocate the work buffer\n");
        return EXIT_FAILURE;
    }

    // Read the input file
    if (!inFile) {
        std::perror("Failed to open the input file");
        return EXIT_FAILURE;
    }

    if (inFileSize < 0x4000) {
        std::fprintf(stderr, "Input file must be at least 0x4000-byte large (got 0x%zx)\n", inFileSize);
        return EXIT_FAILURE;
    }

    util::ScopedBuffer data = util::ReadFileContents(inFile.get(), 0, inFileSize);
    if (!data) {
        std::perror("Failed to read the input file");
        return EXIT_FAILURE;
    }

    // Get modcrypt parameters, do some validation to prevent OOB
    auto params = crypto::DeriveModcryptParameters(data.get());
    if (params.type == crypto::TwlModcryptType_None) {
        std::fprintf(stderr, "Input file is not a TWL SRL, nothing to do\n");
        return EXIT_FAILURE;
    }

    if (!ValidateSection("Arm9 modcrypt", params.arm9SectionOffset, params.arm9SectionSize, inFileSize)) {
        // Error message already printed
        return EXIT_FAILURE;
    }

    if (!ValidateSection("Arm7 modcrypt", params.arm7SectionOffset, params.arm7SectionSize, inFileSize)) {
        // Error message already printed
        return EXIT_FAILURE;
    }

    if (params.arm9SectionSize > 0 && !util::CheckBounds(params.arm9SectionOffset, params.arm9SectionSize, 0x4000, inFileSize)) {
        std::fprintf(stderr, "Arm9 modcrypt section is out of bounds\n");
        return EXIT_FAILURE;
    }

    if (params.arm7SectionSize > 0 && !util::CheckBounds(params.arm7SectionOffset, params.arm7SectionSize, 0x4000, inFileSize)) {
        std::fprintf(stderr, "Arm7 modcrypt section is out of bounds\n");
        return EXIT_FAILURE;
    }

    const char *action = params.enabled ? "Decrypting" : "Encrypting";
    const char *typeStr = params.type == crypto::TwlModcryptType_Retail ? "retail" : "dev";
    std::printf("%s %s modcrypt...\n\n", action, typeStr);
    PrintModcryptParams(params, data.get());

    // Encrypt/decrypt sections (if size == 0, nothing is done).
    // Workbuf/4 instead of Workbuf/2 because we want the workbuf for Arm7 to fit in cache at the same time
    std::thread t([data9Buf = data.get() + params.arm9SectionOffset, workbuf9 = workbuf.get() + WorkBufferSize/4, &params]{
        crypto::RunTwlAesCtrInPlace(params.key, params.arm9SectionIv, data9Buf, params.arm9SectionSize, workbuf9, WorkBufferSize/4);
    });

    [data7Buf = data.get() + params.arm7SectionOffset, workbuf7 = workbuf.get(), &params]{
        crypto::RunTwlAesCtrInPlace(params.key, params.arm7SectionIv, data7Buf, params.arm7SectionSize, workbuf7, WorkBufferSize/4);
    }();

    t.join();

    // Flip modcrypt flag
    crypto::SetModcryptEnabledInRomhdr(data.get(), !params.enabled);

    // Create output directory if needed
    if (util::EnsureDirectoryCreated(outFilename) < 0) {
        std::perror("Failed to create output directory");
        return EXIT_FAILURE;
    }

    // Write the result
    if (util::DumpToFile(outFilename, data.get(), inFileSize, "wb+") < 0) {
        std::perror("Failed to create or to write to the output file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
