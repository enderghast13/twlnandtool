cmake_minimum_required(VERSION 3.13)
project(twlnandtool LANGUAGES CXX C)

if(NOT(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    message(FATAL_ERROR "Sorry, only GCC and Clang are supported")
endif()

if(WIN32 OR APPLE)
    # On Windows and macOS, use static linking for 3rd party libs.
    # This ensures smooth distribution and usage.
    # On Windows, static link everything (besides kernel32 and msvcrt),
    # as this avoids deps like the winpthread DLL, etc.
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    if(WIN32)
        add_link_options(-static)
    endif()
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_compile_options(-ffunction-sections -fdata-sections)
if(APPLE)
    add_link_options(-Wl,-dead_strip)
else()
    add_link_options(-Wl,--gc-sections)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(Nettle REQUIRED)
find_package(GMP REQUIRED)

# By not putting the fetched content in the build directory, we can easily provide
# custom replacements, like here:
# https://github.com/devkitPro/pacman-packages/blob/master/libfat/PKGBUILD#L53-L62
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/fatfspp/CMakeLists.txt")
    include(FetchContent)
    FetchContent_Populate(FatFsPP
        GIT_REPOSITORY https://github.com/TuxSH/fatfspp.git
        GIT_TAG        master
        SOURCE_DIR     ${CMAKE_CURRENT_LIST_DIR}/fatfspp
    )
endif()
add_subdirectory(fatfspp)

add_executable(twlnandtool
    source/main.cpp
    source/command_firmware.cpp
    source/command_modcrypt.cpp
    source/command_nandcrypt.cpp
    source/command_syslog.cpp
    source/crypto/crypto_aes.cpp
    source/crypto/crypto_rsa.cpp
    source/crypto/crypto_sha.cpp
    source/util/util_c_stdlib_funcs.c
    source/util/util_fileutil.cpp
    source/twl_nand_reader.cpp
)

target_compile_definitions(FatFs PUBLIC FF_CONF_ADDENDUM=\"${CMAKE_CURRENT_LIST_DIR}/source/ffcustom.h\")
target_link_libraries(twlnandtool PRIVATE ${Nettle_LIBRARY} ${GMP_C_LIBRARIES} FatFsPP)
target_include_directories(twlnandtool PRIVATE ${Nettle_INCLUDE_DIR} ${GMP_C_INCLUDES})
target_compile_options(twlnandtool PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(twlnandtool PRIVATE -O3 -ftree-vectorize)
