// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "commands.hpp"
#include "crypto/crypto_twl.hpp"
#include "util/util_bound_checks.hpp"

#include <cstdio>
#include <string>
#include <thread>

namespace {
    void PrintFirmwareParams(const crypto::TwlFirmwareParameters &params) {
        char keyHex[33];
        char iv9Hex[33];
        char iv7Hex[33];

        crypto::AesKey key = params.key;
        crypto::AesCounter iv9 = params.arm9SectionIv;
        crypto::AesCounter iv7 = params.arm7SectionIv;

        util::Hexlify(keyHex, key.data, 16);
        util::Hexlify(iv9Hex, iv9.data, 16);
        util::Hexlify(iv7Hex, iv7.data, 16);

        static constexpr const char *typeStrs[] = { "NAND", "Gamecard", "NOR", "3DS", "Invalid" };

        std::printf("Firmware type:                 %s\n", typeStrs[params.type]);
        std::printf("Encryption key:                %s\n\n", keyHex);

        bool compressed9 = (params.flags & 1) != 0;
        std::printf("Arm9 section IV:               %s\n", iv9Hex);
        std::printf("Arm9 section offset on medium: 0x%zX\n", params.arm9SectionOffset);
        std::printf("Arm9 section size on medium:   0x%zX%s\n", params.arm9SectionSizeOnMedium, compressed9 ? " (compressed)" : "");
        std::printf("Arm9 section RAM address:      0x%08" PRIX32 "\n", params.arm9SectionRamAddress);
        std::printf("Arm9 section size in RAM:      0x%zX\n\n", params.arm9SectionSize);

        bool compressed7 = (params.flags & 2) != 0;
        std::printf("Arm7 section IV:               %s\n", iv7Hex);
        std::printf("Arm7 section offset on medium: 0x%zX\n", params.arm7SectionOffset);
        std::printf("Arm7 section size on medium:   0x%zX%s\n", params.arm7SectionSizeOnMedium, compressed7 ? " (compressed)" : "");
        std::printf("Arm7 section RAM address:      0x%08" PRIX32 "\n", params.arm7SectionRamAddress);
        std::printf("Arm7 section size in RAM:      0x%zX\n", params.arm7SectionSize);

        if (compressed9 || compressed7) {
            std::fprintf(stderr, "\nWarning: auto-decompression of compressed sections not yet supported\n");
        }
    }

    bool ValidateSection(const char *what, size_t offset, size_t size, size_t headerOffset, size_t fileSize) {
        if (size < 0x200) {
            // 0x200 is one sector
            std::fprintf(stderr, "%s section is empty\n", what);
            return false;
        }

        if (offset % 0x200 != 0) {
            std::fprintf(stderr, "%s section offset is not aligned to a 512B boundary\n", what);
            return false;
        }

        if (size % 0x200 != 0) {
            std::fprintf(stderr, "%s section size is not aligned to a 512B boundary\n", what);
            return false;
        }

        if (!util::CheckBounds(offset, size, headerOffset + 0x200, fileSize)) {
            std::fprintf(stderr, "%s section is out of bounds", what);
            return false;
        }

        return true;
    }
}

int CommandFirmware(const char *inFilename, const char *outPrefix) {
    size_t inFileSize = 0;
    util::ScopedFile inFile = util::OpenFileAndGetSize(inFileSize, inFilename, "rb");
    alignas(16) u8 mediumHeader[0x400];

    util::ScopedBuffer workbuf = AllocateWorkBuffer();
    if (!workbuf) {
        std::fprintf(stderr, "Failed to allocate the work buffer\n");
        return EXIT_FAILURE;
    }

    // Read the input file (header and sections only, because input file can be a whole NAND dump!)
    if (!inFile) {
        std::perror("Failed to open the input file");
        return EXIT_FAILURE;
    }

    if (inFileSize < 0x400) {
        std::fprintf(stderr, "Input file must be at least 0x400-byte large (got 0x%zx)\n", inFileSize);
        return EXIT_FAILURE;
    }

    if (util::ReadFileContentsInto(mediumHeader, inFile.get(), 0, 0x400) < 0) {
        std::perror("Failed to read the input file");
        return EXIT_FAILURE;
    }

    auto params = crypto::DeriveFirmwareParameters(mediumHeader);
    PrintFirmwareParams(params);

    if (!ValidateSection("Arm9", params.arm9SectionOffset, params.arm9SectionSizeOnMedium, params.headerOffset, inFileSize)) {
        // Error message already printed
        return EXIT_FAILURE;
    }

    if (!ValidateSection("Arm7", params.arm7SectionOffset, params.arm7SectionSizeOnMedium, params.headerOffset, inFileSize)) {
        // Error message already printed
        return EXIT_FAILURE;
    }

    // Check output file names
    bool compressed9 = (params.flags & 1) != 0;
    bool compressed7 = (params.flags & 2) != 0;
    std::string outFilename9 = (std::string(outPrefix) + "arm9.bin") + (compressed9 ? ".lz" : "");
    std::string outFilename7 = (std::string(outPrefix) + "arm7.bin") + (compressed7 ? ".lz" : "");

    if (inFilename == outFilename9 || inFilename == outFilename7) {
        std::fprintf(stderr, "Error: output files cannot have the same name as the input file (%s)\n", inFilename);
        return EXIT_FAILURE;
    }

    // Read input sections
    util::ScopedBuffer data9 = util::ReadFileContents(inFile.get(), params.arm9SectionOffset, params.arm9SectionSizeOnMedium);
    if (!data9) {
        std::perror("Failed to read Arm9 section from the input file");
        return EXIT_FAILURE;
    }

    util::ScopedBuffer data7 = util::ReadFileContents(inFile.get(), params.arm7SectionOffset, params.arm7SectionSizeOnMedium);
    if (!data7) {
        std::perror("Failed to read Arm7 section from the input file");
        return EXIT_FAILURE;
    }

    // Decrypt in parallel.
    // Workbuf/4 instead of Workbuf/2 because we want the workbuf for both threads to fit in cache at the same time
    std::thread t([data9Buf = data9.get(), workbuf9 = workbuf.get(), &params]{
        crypto::RunTwlAesCtrInPlace(params.key, params.arm9SectionIv, data9Buf, params.arm9SectionSizeOnMedium, workbuf9, WorkBufferSize / 4);
    });

    // Decrypt Arm7 section in same thread
    [data7Buf = data7.get(), workbuf7 = workbuf.get() + WorkBufferSize/4, &params]{
        crypto::RunTwlAesCtrInPlace(params.key, params.arm7SectionIv, data7Buf, params.arm7SectionSizeOnMedium, workbuf7, WorkBufferSize / 4);
    }();

    t.join();

    // Create output directory if needed
    if (util::EnsureDirectoryCreated(outPrefix) < 0) {
        std::perror("Failed to create output directory");
        return EXIT_FAILURE;
    }

    // Write the sections.
    // Because we do not support decompression yet, size on medium will be used instead of size in RAM if the section
    // is compressed
    size_t outSize9 = compressed9 ? params.arm9SectionSizeOnMedium : params.arm9SectionSize;
    if (util::DumpToFile(outFilename9, data9.get(), outSize9, "wb+") < 0) {
        std::fprintf(stderr, "\nFailed to create or write to the Arm9 section output file (%s): ", outFilename9.c_str());
        std::perror(nullptr);
        return EXIT_FAILURE;
    }

    size_t outSize7 = compressed7 ? params.arm7SectionSizeOnMedium : params.arm7SectionSize;
    if (util::DumpToFile(outFilename7, data7.get(), outSize7, "wb+") < 0) {
        std::fprintf(stderr, "\nFailed to create or write to the Arm7 section output file (%s): ", outFilename7.c_str());
        std::perror(nullptr);
        return EXIT_FAILURE;
    }

    std::printf("\nWrote Arm9 section to: %s\n", outFilename9.c_str());
    std::printf("Wrote Arm7 section to: %s\n", outFilename7.c_str());

    return EXIT_SUCCESS;
}
