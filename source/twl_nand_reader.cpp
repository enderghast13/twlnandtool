// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "twl_nand_reader.hpp"

// Needed until C++17
constexpr TwlNandReader::LayoutEntry TwlNandReader::NandLayout[];

bool TwlNandReader::Initialize(const char *filename, util::ScopedBuffer &&workbuf, size_t workbufSize) noexcept {
    size_t nandFileSize = 0;
    auto nandFile = util::OpenFileAndGetSize(nandFileSize, filename, "rb");

    // Check if file could be opened
    if (!nandFile) {
        std::perror("Could not open the input NAND file");
        return false;
    }

    // Validate its size
    if (nandFileSize == NandSizeSamsung || nandFileSize == NandSizeSt) {
        std::fprintf(stderr, "Input NAND file is missing nocash footer\n");
        return false;
    } else if (nandFileSize != NandSizeSamsung + 0x40 && nandFileSize != NandSizeSt + 0x40) {
        std::fprintf(stderr, "Input NAND file has wrong size (got 0x%08zX)\n", nandFileSize);
        std::fprintf(stderr, "File size must be either 0x%08zX or 0x%08zX bytes\n", NandSizeSamsung + 0x40, NandSizeSt + 0x40);
        return false;
    }
    m_NandSize = nandFileSize - 0x40;

    // Grab nocash footer
    if (util::ReadFileContentsInto(m_NocashFooter, nandFile.get(), m_NandSize, 0x40) < 0) {
        std::perror("Could not read nocash footer from the input NAND file");
        return false;
    }

    // Validate nocash footer, and grab eMMC CID and console ID from there
    if (std::memcmp(m_NocashFooter, "DSi eMMC CID/CPU", 16) != 0) {
        std::fprintf(stderr, "Nocash footer is malformed\n");
        return false;
    }
    std::copy(m_NocashFooter + 0x10, m_NocashFooter + 0x20, m_EmmcCid);
    m_ConsoleId = util::ReadLe64(m_NocashFooter + 0x20);

    // Derive encryption parameters
    m_Key = crypto::DeriveTwlNandKey(m_ConsoleId);
    m_Iv = crypto::DeriveTwlNandIv(m_EmmcCid);

    // Read the MBR
    if (util::ReadFileContentsInto(m_EncryptedMbr, nandFile.get(), 0, 0x200) < 0) {
        std::perror("Could not read MBR from the input NAND file");
        return false;
    }

    // Detect if the NAND file is encrypted by checking MBR signature
    unsigned int mbrSig = util::ReadLe16(m_EncryptedMbr + 0x1FE);
    if (mbrSig != 0xAA55) {
        // NAND is encrypted, validate encryption key & IV by decrypting MBR
        alignas(16) u8 mbrWorkbuf[0x200];

        m_EncryptedFile = true;
        std::copy(m_EncryptedMbr, m_EncryptedMbr + 0x200, m_DecryptedMbr);
        crypto::RunTwlAesCtrInPlace(m_Key, m_Iv, m_DecryptedMbr, 0x200, mbrWorkbuf, sizeof(mbrWorkbuf));

        mbrSig = util::ReadLe16(m_DecryptedMbr + 0x1FE);
        if (mbrSig != 0xAA55) {
            std::fprintf(stderr, "Could not decrypt the input NAND file's MBR properly (0x%04X != 0xAA55)\n", mbrSig);
            // Print console unique info from nocash footer to facilitate debugging
            PrintConsoleUniqueInfo();
            return false;
        }
    } else {
        // Keep an encrypted copy of the MBR, this will be useful later
        alignas(16) u8 mbrWorkbuf[0x200];

        m_EncryptedFile = false;
        std::copy(m_EncryptedMbr, m_EncryptedMbr + 0x200, m_DecryptedMbr);
        crypto::RunTwlAesCtrInPlace(m_Key, m_Iv, m_EncryptedMbr, 0x200, mbrWorkbuf, sizeof(mbrWorkbuf));
    }

    // Check partition offsets and sizes.
    // Supporting arbitrary partitioning would be quite a pain w/r/t how CryptEntireNand
    // is implemented, and would make the codebase harder to read (need to parse firmware
    // offsets, etc.). It's not useful at all until there are tools that arbitratily
    // reformat the NAND in the wild, either.
    // Most likely, NAND not being formatted the way all other NANDs are is an accident,
    // and we shall return an error to the user (most tools don't even bother doing as such).
    // This might be reconsidered in the future, shall such new tools appear (hopefully not)
    size_t partitionOffsets[4] = {};
    size_t partitionSizes[4] = {};
    for (size_t i = 0; i < 4; i++) {
        partitionOffsets[i] = util::ReadLe32(m_DecryptedMbr + 0x1BE + 16*i +  8) << 9;
        partitionSizes[i]   = util::ReadLe32(m_DecryptedMbr + 0x1BE + 16*i + 12) << 9;
    }
    // Only check TWLN (partition 0) and TWLP (partition 1), we don't care about the rest
    // as we know it's unencrypted (partition 2) or not attributed (partition 3)
    if (partitionOffsets[0] != 0x0010EE00) {
        std::fprintf(stderr, "TWLN partition has wrong offset (expected 0x0010EE00, got 0x%08zX)\n", partitionOffsets[0]);
        return false;
    }
    if (partitionSizes[0] != 0x0CF00000 - 0x0010EE00) {
        std::fprintf(stderr, "TWLN partition has wrong size (expected 0x0CDF1200, got 0x%08zX)\n", partitionSizes[0]);
        return false;
    }
    if (partitionOffsets[1] != 0x0CF09A00) {
        std::fprintf(stderr, "TWLP partition has wrong offset (expected 0x0CF09A00, got 0x%08zX)\n", partitionOffsets[1]);
        return false;
    }
    if (partitionSizes[1] != 0x0EFC0000 - 0x0CF09A00) {
        std::fprintf(stderr, "TWLP partition has wrong size (expected 0x020B6600, got 0x%08zX)\n", partitionSizes[1]);
        return false;
    }

    // All good, now take ownership of the resources
    m_File = std::move(nandFile);
    m_Workbuf = std::move(workbuf);
    m_WorkbufSize = workbufSize;

    m_DiskInserted = true;

    return true;
}

DRESULT TwlNandReader::ReadDisk(u8 *out, LBA_t sectorOffset, UINT numSectors) noexcept {
    size_t off = static_cast<size_t>(sectorOffset << 9);
    size_t sz = static_cast<size_t>(numSectors << 9);

    // m_WorkbufSize / 2 for L2 cache locality
    bool res = ReadBytes(out, off, sz, m_Workbuf.get(), m_WorkbufSize / 2, false);
    return res ? RES_OK : RES_ERROR;
}

bool TwlNandReader::ReadBytes(void *out, size_t offset, size_t size, u8 *workbuf, size_t workbufSize, bool wantEncrypted) {
    bool encrypted = false;
    u8 *out8 = reinterpret_cast<u8 *>(out);

    if (!ValidateNandAccess(encrypted, offset, size)) {
        std::fprintf(stderr, "Invalid NAND read: offset = 0x%08zX, size = 0x%08zX\n", offset, size);
        return false;
    }

    // Return cached version of MBR if applicable. Bounds already checked in ValidateNandAccess
    if (offset < 0x200) {
        const u8 *mbr = wantEncrypted ? m_EncryptedMbr : m_DecryptedMbr;
        std::copy(mbr + offset, mbr + offset + size, out8);
    } else {
        if (util::ReadFileContentsInto(out, m_File.get(), offset, size) < 0) {
            std::fprintf(stderr, "Failed to read NAND (offset = 0x%08zX, size = 0x%08zX, outptr = 0x%p): ", offset, size, out);
            std::perror(nullptr);
            return false;
        }

        if (encrypted && (wantEncrypted != m_EncryptedFile)) {
            crypto::RunTwlAesCtrInPlace(m_Key, m_Iv + offset/16, out8, size, workbuf, workbufSize);
        }
    }

    return true;
}

bool TwlNandReader::WriteNocashFooter(FILE *outFile) {
    if (util::WriteFileContentsFrom(outFile, m_NocashFooter, m_NandSize, 0x40) < 0) {
        std::perror("Failed to write nocash footer to the output NAND file");
        return false;
    }
    return true;
}

void TwlNandReader::PrintConsoleUniqueInfo() const {
    char cidHex[33];
    char keyHex[33];
    char ivHex[33];

    // Do not assume whether or not key/iv are TwlFifo instances
    crypto::AesKey key = m_Key;
    crypto::AesCounter iv = m_Iv;

    util::Hexlify(cidHex, m_EmmcCid, 16);
    util::Hexlify(keyHex, key.data, 16);
    util::Hexlify(ivHex, iv.data, 16);

    std::printf("Console ID:          0x%016" PRIX64 "\n", m_ConsoleId);
    std::printf("NAND eMMC CID:       %s\n", cidHex);
    std::printf("NAND encryption key: %s\n", keyHex);
    std::printf("NAND encryption IV:  %s\n", ivHex);
}

bool TwlNandReader::CryptEntireNand(const char *outFilename) {
    util::ScopedFile outFile = util::OpenFile(outFilename, "wb+");
    if (!outFile) {
        std::perror("Failed to create the output NAND file");
        return false;
    }

    // No visible impact with 5900X, MinGW64 with SSD and with HDD, avg. 128 tries
    // on same file
    //std::setvbuf(m_File.get(), nullptr, _IONBF, 0);
    //std::setvbuf(outFile.get(), nullptr, _IONBF, 0);

    u8 *outbuf = m_Workbuf.get();
    size_t bufSize = m_WorkbufSize / 2;
    u8 *workbuf = outbuf + bufSize;

    ASSUME_ALIGNED(outbuf, 16);
    ASSUME_ALIGNED(workbuf, 16);
    ASSUME_ALIGNED(bufSize, 16);
    ASSUME_NO_OVERFLOW(m_Workbuf.get(), m_WorkbufSize);

    for (const auto &entry: NandLayout) {
        u8 *out = outbuf;
        size_t off = entry.start;
        size_t end = entry.end == 0x0F000000 ? m_NandSize : entry.end;
        size_t rem = end - entry.start;

        while (rem > 0) {
            size_t chunkSize = std::min(rem, bufSize);
            bool wantEncrypted = !m_EncryptedFile;

            if (!ReadBytes(out, off, chunkSize, workbuf, bufSize, wantEncrypted)) {
                // Error message already printed
                return false;
            }

            if (util::WriteFileContentsFrom(outFile.get(), out, off, chunkSize) < 0) {
                std::perror("Failed to write to the output NAND file");
                return false;
            }

            off += chunkSize;
            rem -= chunkSize;
        }
    }

    return WriteNocashFooter(outFile.get());
}
