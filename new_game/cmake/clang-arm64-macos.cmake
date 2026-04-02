# Toolchain: Apple Clang targeting macOS ARM64 (Apple Silicon M1+).
# On macOS the system clang is used — no --target flag needed, CMake auto-detects.
# Used via VCPKG_CHAINLOAD_TOOLCHAIN_FILE so vcpkg remains the primary toolchain.

set(CMAKE_SYSTEM_NAME    Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# Apple Clang from Xcode / Command Line Tools.
set(CMAKE_C_COMPILER   "clang")
set(CMAKE_CXX_COMPILER "clang++")

# Build for ARM64 only (Apple Silicon native).
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")

# Minimum macOS version: 15.0 (Sequoia) — matches vcpkg-built libraries.
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "")

# Tell vcpkg to use arm64-osx packages.
set(VCPKG_TARGET_TRIPLET "arm64-osx" CACHE STRING "")
