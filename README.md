# twlnandtool

TWL decryption tool (NAND/modcrypt/firmware) with a focus on maximum performance and (hopefully) readability.

`twlnandtool` is loosely inspired from `twltool` and fixes a few long-standing critical issues of the latter:

- `twltool` miscompiling on recent GCC versions due to not adhering to strict aliasing rules, resulting in corrupt output files
- `twltool` not being able to decrypt non-retail firmware (a.k.a `stage2`) files
  - `twlnandtool` properly derives encryption keys, and not just for NAND Flash firmware: NAND/NTRCARD/NOR dumps are all supported
- `twltool` not flipping the "modcrypt enable" flag in the resulting file, when performing "modcrypt" encryption/decryption

On my MacBook Air M4 10C, it is able to encrypt/decrypt NAND at a rate between 1.0 GiB/s when cold-loaded from SSD, to 3.0GiB/s when in disk cache (1.7 MiB/s on 5900X). This usually is at least 6 times faster than `twltool` and at least 50 times faster than `ninfs` for the same task.

`twlnandtool` is written in C++14, as an exercise and because it originally targeted GCC versions as low as GCC 7.1, for maximum compatibility.

It is licensed under the terms of the GNU GPL version 2 or any later version. (c) 2024 TuxSH

## Usage and features

```
Usage: twlnandtool <command> [args]

Commands:

    - nandcrypt
    - syslog
    - modcrypt
    - firmware
    - help, -h, --help (displays this usage message)

twlnandtool nandcrypt: encrypt/decrypt TWL NAND files

    - Usage: twlnandtool nandcrypt infile outfile
    - Encrypts or decrypts the TWL NAND file located at infile, and writes
      the result to outfile
    - 3DS NAND dumps are not supported
    - On Windows, OSFMount can be used to mount a decrypted NAND image
    - On macOS, image can be mounted with:
        hdiutil attach -imagekey diskimage-class=CRawDiskImage nand_dec.bin

twlnandtool syslog: prints the contents of twln:/sys/log/sysmenu.log to stdout

    - Usage: twlnandtool syslog infile
    - Infile is the input NAND file, either decrypted or encrypted
    - Useful for troubleshooting fatal error displays in System Menu

twlnandtool modcrypt: encrypt/decrypt modcrypted system titles

    - Usage: twlnandtool modcrypt infile outfile
    - Infile is the input SRL file, either decrypted or encrypted
    - Output is the name of the output SRL file to create

twlnandtool firmware: decrypts and extract firmware ("stage2") files of any kind

    - Usage: twlnandtool firmware infile outprefix
    - Infile is the input file to extract the firmware from, this can be
      whole NAND dumps, ntrboot gamecard dumps, SPI flash dumps or standalone files
    - Output sections are written to outprefix concatenated with 'arm9.bin'
      and 'arm7.bin'
```

## Building

`twlnandtool` has the following dependencies:

- `cmake` 3.13 or higher, with `git` support
- `nettle`
- `GMP`

On Windows, MSVC is not supported. MSYS2 is required instead (MINGW64 and CLANG64 both work).

To ensure smooth deployment and ease of use, static versions of the libraries are used, including static `libgcc` and `libstdc++` for MinGW.

Additionally, this project uses CMake's `FetchContent` API to fetch [fatfspp](https://github.com/TuxSH/fatfspp), itself fetching [devkitPro's customized fork of FatFS](https://github.com/devkitPro/fatfs-mod).

Use the following to build:

```shell
mkdir build && cd build
cmake .. -G"Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja
```

or

```shell
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## Misc.

The TWL keyscrambler code in this project is fully `constexpr`, with user-defined literals.
