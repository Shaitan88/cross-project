# Cross-compilation toolchain for 32-bit Windows (x86) with MinGW-w64.
#
# Use with:
#   cmake -S . -B build-windows-x86/project \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-i686.cmake ...
#
# Keep this file free of application-specific or Wine-specific linker flags;
# they belong to the project configure step, not to the toolchain used to build
# third-party dependencies such as SDL2.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

set(CMAKE_AR i686-w64-mingw32-ar)
set(CMAKE_RANLIB i686-w64-mingw32-ranlib)
set(CMAKE_STRIP i686-w64-mingw32-strip)

# Headers and import libraries shipped by the distribution's MinGW-w64 sysroot.
set(_MINGW_SYSROOT /usr/i686-w64-mingw32)

# A caller may pre-seed CMAKE_FIND_ROOT_PATH (for example with a local
# dependency prefix). Always keep the MinGW-w64 sysroot in the list so that
# standard Windows headers and import libraries remain discoverable.
if(DEFINED CMAKE_FIND_ROOT_PATH AND NOT CMAKE_FIND_ROOT_PATH STREQUAL "")
    list(APPEND CMAKE_FIND_ROOT_PATH "${_MINGW_SYSROOT}")
    list(REMOVE_DUPLICATES CMAKE_FIND_ROOT_PATH)
else()
    set(CMAKE_FIND_ROOT_PATH "${_MINGW_SYSROOT}")
endif()

# Programs are executed by the host, so never look for them inside the sysroot.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Libraries, headers and CMake packages come from the Windows sysroot / prefix.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
