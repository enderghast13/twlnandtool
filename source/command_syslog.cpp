// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "commands.hpp"
#include "twl_nand_reader.hpp"

#include <cstdio>
#include <chrono>

int CommandSyslog(const char *inFilename) {
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

    auto twln = nandReader->MountPartition(TwlNandReader::Twln);
    if (!twln) {
        nandReader->PrintFileError("Failed to mount TWLN partition");
        return EXIT_FAILURE;
    }

    auto syslog = twln->OpenFile("/sys/log/sysmenu.log", FA_READ);
    if (!syslog) {
        twln->PrintFileError("Failed to open /sys/log/sysmenu.log");
        return EXIT_FAILURE;
    }

    syslog->ForwardToStream(+[](const BYTE *in, UINT sz) -> UINT {
        if (in == nullptr) {
            // Stream is ready
            return 1;
        } else {
            // Forward the string to stdout
            return static_cast<UINT>(std::fwrite(in, 1, sz, stdout));
        }
    });
    std::puts("\n");

    if (syslog->GetFileError() != FR_OK) {
        syslog->PrintFileError("\nFailed to print the contents of /sys/log/sysmenu.log");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
