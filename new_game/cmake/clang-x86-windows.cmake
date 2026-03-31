# Toolchain: clang++ targeting 32-bit Windows (i686-pc-windows-msvc).
# Uses standalone clang++ (GNU-style frontend) instead of clang-cl (MSVC frontend).
# clang++ auto-detects MSVC installation — no vcvarsall.bat needed.
# Used via VCPKG_CHAINLOAD_TOOLCHAIN_FILE so vcpkg remains the primary toolchain.

set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(CMAKE_C_COMPILER   "clang")
set(CMAKE_CXX_COMPILER "clang++")

# Target 32-bit x86 with MSVC-compatible ABI.
# This must be in FLAGS_INIT (evaluated before first try_compile) rather than
# target_compile_options, because CMake uses try_compile during configuration.
set(CMAKE_C_FLAGS_INIT   "--target=i686-pc-windows-msvc")
set(CMAKE_CXX_FLAGS_INIT "--target=i686-pc-windows-msvc")

# Tell vcpkg to use x86-windows packages.
set(VCPKG_TARGET_TRIPLET "x86-windows" CACHE STRING "")
