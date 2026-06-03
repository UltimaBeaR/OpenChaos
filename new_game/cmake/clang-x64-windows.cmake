# Toolchain: clang++ targeting 64-bit Windows (x86_64-pc-windows-msvc).
# Uses standalone clang++ (GNU-style frontend) instead of clang-cl (MSVC frontend).
# clang++ auto-detects MSVC installation — no vcvarsall.bat needed.
# Used via VCPKG_CHAINLOAD_TOOLCHAIN_FILE so vcpkg remains the primary toolchain.

set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   "clang")
set(CMAKE_CXX_COMPILER "clang++")

# Resource compiler for the .rc that embeds the .exe icon. Standalone clang
# ships llvm-rc; rc.exe (Windows SDK) is not on PATH without vcvarsall, so
# point CMake at llvm-rc explicitly.
set(CMAKE_RC_COMPILER "llvm-rc")

# Target 64-bit x86 with MSVC-compatible ABI.
# This must be in FLAGS_INIT (evaluated before first try_compile) rather than
# target_compile_options, because CMake uses try_compile during configuration.
set(CMAKE_C_FLAGS_INIT   "--target=x86_64-pc-windows-msvc")
set(CMAKE_CXX_FLAGS_INIT "--target=x86_64-pc-windows-msvc")

# Tell vcpkg to build all packages as STATIC libraries with a static CRT
# (x64-windows-static), so the final OpenChaos.exe is a single self-contained
# file with no DLLs next to it. The exe then always starts (only OS libraries
# are needed), which lets the "game files not found" screen render even when the
# user drops the bare exe in the wrong place. Linux/macOS already use static
# vcpkg triplets by default. Our own translation units are matched to the static
# CRT in CMakeLists.txt (-fms-runtime-lib).
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
