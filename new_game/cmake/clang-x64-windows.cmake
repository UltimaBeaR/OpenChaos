# Toolchain: clang++ targeting 64-bit Windows (x86_64-pc-windows-msvc).
# Uses standalone clang++ (GNU-style frontend) instead of clang-cl (MSVC frontend).
# clang++ auto-detects MSVC installation — no vcvarsall.bat needed.
# Used via VCPKG_CHAINLOAD_TOOLCHAIN_FILE so vcpkg remains the primary toolchain.

set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   "clang")
set(CMAKE_CXX_COMPILER "clang++")

# Target 64-bit x86 with MSVC-compatible ABI.
# This must be in FLAGS_INIT (evaluated before first try_compile) rather than
# target_compile_options, because CMake uses try_compile during configuration.
set(CMAKE_C_FLAGS_INIT   "--target=x86_64-pc-windows-msvc")
set(CMAKE_CXX_FLAGS_INIT "--target=x86_64-pc-windows-msvc")

# Tell vcpkg to use x64-windows packages.
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
