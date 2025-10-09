// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "commands.hpp"
#include "twl_nand_reader.hpp"

#include <cstdio>
#include <chrono>

int CommandNandcrypt(const char *inFilename, const char *outFilename) {
    util::ScopedBuffer workbuf = AllocateWorkBuffer();
    if (!workbuf) {
        std::fprintf(stderr, "Failed to allocate the work buffer\n");
        return EXIT_FAILURE;
    }

    auto nandReader = TwlNandReader::Create(inFilename, std::move(workbuf), WorkBufferSize);
    if(!nandReader) {
        // Error message already printed
        return EXIT_FAILURE;
    }

    // Create output directory if needed
    if (util::EnsureDirectoryCreated(outFilename) < 0) {
        std::perror("Failed to create output directory");
        return EXIT_FAILURE;
    }

    nandReader->PrintConsoleUniqueInfo();
    if (nandReader->IsEncryptedFile()) {
        std::printf("\nDecrypting NAND...\n");
    } else {
        std::printf("\nEncrypting NAND...\n");
    }

    auto t0 = std::chrono::steady_clock::now();
    bool res = nandReader->CryptEntireNand(outFilename);
    auto t1 = std::chrono::steady_clock::now();

    if (!res) {
        // Error message already printed
        return EXIT_FAILURE;
    }

    float dt = std::chrono::duration_cast<std::chrono::duration<float> >(t1 - t0).count();
    s64 dtUs = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    float mibPerSec = (1000 * 1000ull * nandReader->GetNandSize()) / (1024.0f * 1024.0f * dtUs);
    if (nandReader->IsEncryptedFile()) {
        std::printf("NAND decrypted to %s in %.3fs (%.2f MiB/s)\n", outFilename, dt, mibPerSec);
    } else {
        std::printf("NAND encrypted to %s in %.3fs (%.2f MiB/s)\n", outFilename, dt, mibPerSec);
    }

    return EXIT_SUCCESS;
}
