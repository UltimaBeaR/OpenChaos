# Toolchain: clang++ targeting 64-bit Linux (x86_64-unknown-linux-gnu).
# Used via VCPKG_CHAINLOAD_TOOLCHAIN_FILE so vcpkg remains the primary toolchain.

set(CMAKE_SYSTEM_NAME    Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   "clang")
set(CMAKE_CXX_COMPILER "clang++")

# Tell vcpkg to use x64-linux packages.
set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "")
