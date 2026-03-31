SHELL      := bash
SRC_DIR    := new_game
BUILD_DIR  := new_game/build
RESOURCES  := original_game_resources
POWERSHELL := /c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe
CMAKE      := cmake

.PHONY: build build-release build-debug build-increment-release build-increment-debug \
        run-release run-debug \
        configure-opengl configure-d3d6 \
        copy-resources-debug copy-resources-release copy-resources \
        r d

# ---------------------------------------------------------------------------
# Configure (run once on first clone, re-run after CMakeLists.txt changes).
# Choose a backend: opengl or d3d6 (legacy DirectX 6).
#
# What it does:
#   1. Runs vcvarsall.bat x86 to set up the VS build environment
#      (makes Windows SDK headers/libs visible to clang-cl).
#   2. Runs CMake with Ninja Multi-Config generator — one configure covers
#      both Debug and Release builds.
#   3. Installs vcpkg packages (SDL3, OpenAL, fmt) into new_game/vcpkg_installed/
#      automatically via vcpkg manifest mode (vcpkg.json).
# ---------------------------------------------------------------------------

configure-opengl:
	$(POWERSHELL) -ExecutionPolicy Bypass -File $(SRC_DIR)/scripts/configure.ps1 -Backend opengl

configure-d3d6:
	$(POWERSHELL) -ExecutionPolicy Bypass -File $(SRC_DIR)/scripts/configure.ps1 -Backend d3d6

# ---------------------------------------------------------------------------
# Build (requires configure-opengl or configure-d3d6 to be run first)
# build-debug/release: full rebuild; build-increment-*: incremental
# ---------------------------------------------------------------------------

build: build-debug build-release

build-debug:
	$(POWERSHELL) -ExecutionPolicy Bypass -File $(SRC_DIR)/scripts/build.ps1 -Config Debug -Clean

build-release:
	$(POWERSHELL) -ExecutionPolicy Bypass -File $(SRC_DIR)/scripts/build.ps1 -Config Release -Clean

build-increment-debug:
	$(POWERSHELL) -ExecutionPolicy Bypass -File $(SRC_DIR)/scripts/build.ps1 -Config Debug

build-increment-release:
	$(POWERSHELL) -ExecutionPolicy Bypass -File $(SRC_DIR)/scripts/build.ps1 -Config Release

# ---------------------------------------------------------------------------
# Copy game resources (original_game_resources/ → build output dirs)
# Run once after fresh clone or when resources change.
# NOTE: original_game_resources/ is not committed (copyright).
# ---------------------------------------------------------------------------

copy-resources-debug:
	$(POWERSHELL) -ExecutionPolicy Bypass -File $(SRC_DIR)/scripts/copy_resources.ps1 -Config Debug

copy-resources-release:
	$(POWERSHELL) -ExecutionPolicy Bypass -File $(SRC_DIR)/scripts/copy_resources.ps1 -Config Release

copy-resources: copy-resources-debug copy-resources-release

# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------

run-debug:
	$(POWERSHELL) -Command "Start-Process -FilePath '$(BUILD_DIR)/Debug/Fallen.exe' -WorkingDirectory (Resolve-Path '$(BUILD_DIR)/Debug')"

run-release:
	$(POWERSHELL) -Command "Start-Process -FilePath '$(BUILD_DIR)/Release/Fallen.exe' -WorkingDirectory (Resolve-Path '$(BUILD_DIR)/Release')"

# ---------------------------------------------------------------------------
# Build + Run shortcuts
# ---------------------------------------------------------------------------

r: build-release run-release

d: build-debug run-debug
