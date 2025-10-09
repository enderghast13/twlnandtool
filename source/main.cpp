// SPDX-License-Identifier: GPL-2.0-or-later
// (c) 2024 TuxSH

#include "commands.hpp"

namespace {

    constexpr const char *VersionMessage = "twlnandtool v1.0 - (c) 2024 TuxSH";
    constexpr const char *UsageMessage =
        "\nUsage: twlnandtool <command> [args]\n"
        "\n"
        "Commands:\n\n"
        "    - nandcrypt\n"
        "    - syslog\n"
        "    - modcrypt\n"
        "    - firmware\n"
        "    - help, -h, --help (displays this usage message)\n"
        "\n"
        "twlnandtool nandcrypt: encrypt/decrypt TWL NAND files\n\n"
        "    - Usage: twlnandtool nandcrypt infile outfile\n"
        "    - Encrypts or decrypts the TWL NAND file located at infile, and writes\n"
        "      the result to outfile\n"
        "    - 3DS NAND dumps are not supported\n"
        "    - On Windows, OSFMount can be used to mount a decrypted NAND image\n"
        "    - On macOS, image can be mounted with:\n"
        "        hdiutil attach -imagekey diskimage-class=CRawDiskImage nand_dec.bin\n"
        "\n"
        "twlnandtool syslog: prints the contents of twln:/sys/log/sysmenu.log to stdout\n\n"
        "    - Usage: twlnandtool syslog infile\n"
        "    - Infile is the input NAND file, either decrypted or encrypted\n"
        "    - Useful for troubleshooting fatal error displays in System Menu\n"
        "\n"
        "twlnandtool modcrypt: encrypt/decrypt modcrypted system titles\n\n"
        "    - Usage: twlnandtool modcrypt infile outfile\n"
        "    - Infile is the input SRL file, either decrypted or encrypted\n"
        "    - Output is the name of the output SRL file to create\n"
        "\n"
        "twlnandtool firmware: decrypts and extract firmware (\"stage2\") files of any kind\n\n"
        "    - Usage: twlnandtool firmware infile outprefix\n"
        "    - Infile is the input file to extract the firmware from, this can be\n"
        "      whole NAND dumps, ntrboot gamecard dumps, SPI flash dumps or standalone files\n"
        "    - Output sections are written to outprefix concatenated with 'arm9.bin'\n"
        "      and 'arm7.bin'\n";

    void PrintHelpAndVersion() {
        // Also prints version and (c)
        std::puts(VersionMessage);
        std::puts(UsageMessage);
    }

    void PrintUsage() {
        std::puts(UsageMessage);
    }

    template<typename Func, size_t ...I>
    int CallCommandImpl(Func &&f, char *argv[], std::index_sequence<I...>) {
        return f(argv[2 + I]...);
    }

    template<size_t NumArgs, typename Func>
    int ValidateAndCallCommand(const char *name, Func &&f, int argc, char *argv[]) {
        if (argc - 2 != NumArgs) {
            std::fprintf(stderr, "Error: %s takes %zu arguments (%d given)\n", name, NumArgs, argc - 2);
            PrintUsage();
            return EXIT_FAILURE;
        }

        return CallCommandImpl(std::forward<Func>(f), argv, std::make_index_sequence<NumArgs>{});
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Error: no command specified\n");
        PrintUsage();
        return EXIT_FAILURE;
    }

    std::string cmd = argv[1];

    if (cmd == "nandcrypt") {
        return ValidateAndCallCommand<2>("nandcrypt", CommandNandcrypt, argc, argv);
    } else if (cmd == "syslog") {
        return ValidateAndCallCommand<1>("syslog", CommandSyslog, argc, argv);
    } else if (cmd == "modcrypt") {
        return ValidateAndCallCommand<2>("modcrypt", CommandModcrypt, argc, argv);
    } else if (cmd == "firmware") {
        return ValidateAndCallCommand<2>("firmware", CommandFirmware, argc, argv);
    } else if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        PrintHelpAndVersion();
        return EXIT_SUCCESS;
    } else {
        std::fprintf(stderr, "Unrecognized command: %s\n", cmd.c_str());
        PrintUsage();
        return EXIT_FAILURE;
    }
}
