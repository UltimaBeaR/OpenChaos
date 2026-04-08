# CMake configure. Run once on first clone, re-run after CMakeLists.txt changes.
# Installs vcpkg packages (SDL3, OpenAL, FFmpeg) automatically.
.PHONY: configure

# Configure with AddressSanitizer enabled for memory error detection.
# Detects heap overflows, use-after-free, stack-use-after-scope, etc.
.PHONY: configure-asan

# Full rebuild of both Debug and Release.
.PHONY: build

# Full rebuild, Debug config. Requires: make configure (run once).
.PHONY: build-debug

# Full rebuild, Release config. Requires: make configure (run once).
.PHONY: build-release

# Incremental build, Debug config. Only recompiles changed files.
.PHONY: build-increment-debug

# Incremental build, Release config. Only recompiles changed files.
.PHONY: build-increment-release

# Copy original_game_resources/ into both Debug and Release output dirs.
# Run once after clone or when game resources change.
.PHONY: copy-resources

# Copy game resources into Debug output dir only.
.PHONY: copy-resources-debug

# Copy game resources into Release output dir only.
.PHONY: copy-resources-release

# Launch Debug build. Redirects stderr to stderr.log.
.PHONY: run-debug

# Launch Release build. Redirects stderr to stderr.log.
.PHONY: run-release

# Build Release + launch. Stops if build fails.
.PHONY: r

# Build Debug + launch. Stops if build fails.
.PHONY: d

# Package Release build into a zip for distribution.
# Usage: make release-package VERSION=X.Y.Z
.PHONY: release-package

SHELL      := bash
SRC_DIR    := new_game
BUILD_DIR  := new_game/build
RESOURCES  := original_game_resources
CMAKE      := cmake

include make/platform.mk
include make/configure.mk
include release/release.mk

build: build-debug build-release

build-debug:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
	  echo "ERROR: Build directory not found. Run 'make configure' first." >&2; \
	  exit 1; \
	fi
	$(CMAKE) --build $(BUILD_DIR) --config Debug --clean-first

build-release:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
	  echo "ERROR: Build directory not found. Run 'make configure' first." >&2; \
	  exit 1; \
	fi
	$(CMAKE) --build $(BUILD_DIR) --config Release --clean-first

build-increment-debug:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
	  echo "ERROR: Build directory not found. Run 'make configure' first." >&2; \
	  exit 1; \
	fi
	$(CMAKE) --build $(BUILD_DIR) --config Debug

build-increment-release:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
	  echo "ERROR: Build directory not found. Run 'make configure' first." >&2; \
	  exit 1; \
	fi
	$(CMAKE) --build $(BUILD_DIR) --config Release

copy-resources: copy-resources-debug copy-resources-release

copy-resources-debug:
	$(CMAKE) -E copy_directory $(RESOURCES) $(BUILD_DIR)/Debug

copy-resources-release:
	$(CMAKE) -E copy_directory $(RESOURCES) $(BUILD_DIR)/Release

run-debug:
	cd $(BUILD_DIR)/Debug && rm -f stderr.log && ./$(EXE_NAME) 2>stderr.log

run-release:
	cd $(BUILD_DIR)/Release && rm -f stderr.log && ./$(EXE_NAME) 2>stderr.log

r: build-release run-release

d: build-debug run-debug
