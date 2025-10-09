// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#pragma once

#include "util/util_fileutil.hpp"
#include "util/util_bound_checks.hpp"
#include "util/util_passkey.hpp"
#include "crypto/crypto_twl.hpp"
#include <fatfspp.hpp>

class TwlNandReader final : public fatfspp::Disk {
    public:
        /// NAND partitions.
        enum PartitionId {
            Twln = 0,
            Twlp = 1,
        };

        /// Total NAND size (Samsung chips) (not including nocash footer).
        static constexpr size_t NandSizeSamsung = 0x0F000000;

        /// Total NAND size (ST chips) (not including nocash footer).
        static constexpr size_t NandSizeSt = 0x0F580000;

        constexpr bool IsEncryptedFile() const  { return m_EncryptedFile; }
        constexpr size_t GetNandSize() const    { return m_NandSize; }

        void PrintConsoleUniqueInfo() const;
        bool CryptEntireNand(const char *outFilename);

        std::shared_ptr<fatfspp::Partition> MountPartition(PartitionId partitionId) {
            if (partitionId != Twln && partitionId != Twlp) {
                return nullptr;
            } else {
                return MountPartitionImpl(1 + static_cast<UINT>(partitionId));
            }
        }

        /// Reserved for Create
        TwlNandReader(const char *filename, util::ScopedBuffer workbuf, size_t workbufSize, util::PassKey<TwlNandReader>) noexcept {
            // Note: passkey idiom needed for make_shared
            Initialize(filename, std::move(workbuf), workbufSize);
        }

        /// Create a TwlNandReader backed by the file located at filename, and provided a work buffer
        static auto Create(const char *filename, util::ScopedBuffer workbuf, size_t workbufSize) {
            std::shared_ptr<TwlNandReader> ret = std::make_shared<TwlNandReader>(filename, std::move(workbuf), workbufSize, util::PassKey<TwlNandReader>{});
            return ret->m_DiskInserted ? ret : nullptr;
        }

        /// Overload for Create
        static auto Create(const std::string &filename, util::ScopedBuffer workbuf, size_t workbufSize) {
            return Create(filename.c_str(), std::move(workbuf), workbufSize);
        }

    protected:
        virtual DRESULT ReadDisk(u8 *out, LBA_t sectorOffset, UINT numSectors) noexcept override;

    private:
        /// NAND layout entry format.
        struct LayoutEntry {
            size_t start;
            size_t end;
            bool encrypted;
        };

        /// NAND layout (not including nocash footer).
        static constexpr LayoutEntry NandLayout[] = {
            { 0x00000000, 0x00000200, true  }, // MBR (encrypted)
            { 0x00000200, 0x0010EE00, false }, // Firmware ("stage2") (unencrypted)
            { 0x0010EE00, 0x0CF00000, true  }, // twln (encrypted)
            { 0x0CF00000, 0x0CF09A00, false }, // free space
            { 0x0CF09A00, 0x0EFC0000, true  }, // twlp (encrypted)
            { 0x0EFC0000, 0x0F000000, false }, // free space and unformatted 3rd partition
            // Some NANDs may have 0x0F000000..0x0F580000, this is taken care of
        };

        constexpr bool ValidateNandAccess(bool &outEncrypted, size_t offset, size_t size) const {
            // Find where start offset is, then validate it does not cross bounds
            for (const auto &entry: NandLayout) {
                size_t end = entry.end == 0x0F000000 ? m_NandSize : entry.end;
                if (offset >= entry.start && offset < end) {
                    outEncrypted = entry.encrypted;
                    return util::CheckBounds(offset, size, entry.start, end - entry.start);
                }
            }
            return false;
        }

        bool Initialize(const char *filename, util::ScopedBuffer &&workbuf, size_t workbufSize) noexcept;
        bool ReadBytes(void *out, size_t offset, size_t size, u8 *workbuf, size_t workbufSize, bool wantEncrypted);
        bool WriteNocashFooter(FILE *outFile);

        util::ScopedFile m_File{nullptr, nullptr};
        util::ScopedBuffer m_Workbuf{nullptr, nullptr};
        size_t m_WorkbufSize = 0;
        size_t m_NandSize = 0;
        bool m_EncryptedFile = false;
        alignas(16) u8 m_NocashFooter[0x40] = {};
        u64 m_ConsoleId = 0;
        alignas(16) u8 m_EmmcCid[0x10] = {};
        crypto::AesKey m_Key{};
        crypto::TwlCounter m_Iv{};
        alignas(16) u8 m_EncryptedMbr[0x200] = {};
        alignas(16) u8 m_DecryptedMbr[0x200] = {};
};
