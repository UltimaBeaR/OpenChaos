SHELL      := bash
SRC_DIR    := new_game
BUILD_DIR  := new_game/build
RESOURCES  := original_game_resources
CMAKE      := cmake
TOOLCHAIN  := $(abspath $(SRC_DIR)/cmake/clang-x64-windows.cmake)

.PHONY: build build-release build-debug build-increment-release build-increment-debug \
        run-release run-debug \
        configure-opengl configure-d3d6 \
        copy-resources-debug copy-resources-release copy-resources \
        r d

# ---------------------------------------------------------------------------
# Auto-detect vcpkg.cmake
#   1. VCPKG_ROOT env var
#   2. Windows: vswhere (finds VS / Build Tools installation)
# ---------------------------------------------------------------------------

VSWHERE := /c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe

VCPKG_CMAKE := $(shell \
  if [ -n "$$VCPKG_ROOT" ] && [ -f "$$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then \
    echo "$$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"; \
  elif [ -f "$(VSWHERE)" ]; then \
    VS_PATH=$$("$(VSWHERE)" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>/dev/null); \
    if [ -n "$$VS_PATH" ] && [ -f "$$VS_PATH/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then \
      echo "$$VS_PATH/VC/vcpkg/scripts/buildsystems/vcpkg.cmake"; \
    fi; \
  fi)

# ---------------------------------------------------------------------------
# Configure (run once on first clone, re-run after CMakeLists.txt changes).
# Choose a backend: opengl or d3d6 (legacy DirectX 6).
#
# Runs CMake with Ninja Multi-Config generator — one configure covers
# both Debug and Release builds. vcpkg packages (SDL3, OpenAL, fmt) are
# installed automatically via manifest mode (vcpkg.json).
# ---------------------------------------------------------------------------

define run_configure
	@if [ -z "$(VCPKG_CMAKE)" ]; then \
	  echo "ERROR: vcpkg.cmake not found. Install VS Build Tools with C++ workload, or set VCPKG_ROOT." >&2; \
	  exit 1; \
	fi
	$(CMAKE) -S $(SRC_DIR) -B $(BUILD_DIR) -G "Ninja Multi-Config" \
	  "-DCMAKE_TOOLCHAIN_FILE=$(VCPKG_CMAKE)" \
	  "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$(TOOLCHAIN)" \
	  "-DVCPKG_INSTALLED_DIR=$(abspath $(SRC_DIR)/vcpkg_installed)" \
	  "-DGRAPHICS_BACKEND=$(1)" \
	  $(CMAKE_EXTRA_ARGS)
endef

configure-opengl:
	$(call run_configure,opengl)

configure-d3d6:
	$(call run_configure,d3d6)

# ASan: configure with AddressSanitizer enabled, copy runtime DLL
configure-asan:
	CMAKE_EXTRA_ARGS="-DENABLE_ASAN=ON" $(MAKE) configure-opengl
	@ASAN_DLL=$$(clang -print-file-name=clang_rt.asan_dynamic-x86_64.dll); \
	  if [ -f "$$ASAN_DLL" ]; then \
	    cp "$$ASAN_DLL" $(BUILD_DIR)/Debug/ 2>/dev/null; \
	    cp "$$ASAN_DLL" $(BUILD_DIR)/Release/ 2>/dev/null; \
	    echo "ASan DLL copied to build dirs"; \
	  else \
	    echo "WARNING: ASan DLL not found at $$ASAN_DLL"; \
	  fi

# ---------------------------------------------------------------------------
# Build (requires configure-opengl or configure-d3d6 to be run first)
# build-debug/release: full rebuild; build-increment-*: incremental
# ---------------------------------------------------------------------------

build: build-debug build-release

build-debug:
	$(CMAKE) --build $(BUILD_DIR) --config Debug --clean-first

build-release:
	$(CMAKE) --build $(BUILD_DIR) --config Release --clean-first

build-increment-debug:
	$(CMAKE) --build $(BUILD_DIR) --config Debug

build-increment-release:
	$(CMAKE) --build $(BUILD_DIR) --config Release

# ---------------------------------------------------------------------------
# Copy game resources (original_game_resources/ → build output dirs)
# Run once after fresh clone or when resources change.
# NOTE: original_game_resources/ is not committed (copyright).
# ---------------------------------------------------------------------------

copy-resources-debug:
	$(CMAKE) -E copy_directory $(RESOURCES) $(BUILD_DIR)/Debug

copy-resources-release:
	$(CMAKE) -E copy_directory $(RESOURCES) $(BUILD_DIR)/Release

copy-resources: copy-resources-debug copy-resources-release

# ---------------------------------------------------------------------------
# Run (launches game from its build output directory)
# ---------------------------------------------------------------------------

run-debug:
	cd $(BUILD_DIR)/Debug && rm -f stderr.log && ./Fallen.exe 2>stderr.log

run-release:
	cd $(BUILD_DIR)/Release && rm -f stderr.log && ./Fallen.exe 2>stderr.log

# ---------------------------------------------------------------------------
# Build + Run shortcuts
# ---------------------------------------------------------------------------

r: build-release run-release

d: build-debug run-debug
