SHELL      := bash
SRC_DIR    := new_game
BUILD_DIR  := new_game/build
RESOURCES  := original_game_resources
CMAKE      := cmake

# ---------------------------------------------------------------------------
# Included modules
# ---------------------------------------------------------------------------

include make/platform.mk

# configure-opengl, configure-d3d6, configure-asan
include make/configure.mk

# release-package VERSION=X.Y.Z
include release/release.mk

# ---------------------------------------------------------------------------

.PHONY: build build-release build-debug build-increment-release build-increment-debug \
        run-release run-debug \
        configure-opengl configure-d3d6 configure-asan \
        copy-resources-debug copy-resources-release copy-resources \
        release-package r d

# Usage: make build
# Full rebuild of both Debug and Release.
build: build-debug build-release

# Usage: make build-debug
# Full rebuild, Debug config. Requires: make configure-opengl (run once).
build-debug:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
	  echo "ERROR: Build directory not found. Run 'make configure-opengl' first." >&2; \
	  exit 1; \
	fi
	$(CMAKE) --build $(BUILD_DIR) --config Debug --clean-first

# Usage: make build-release
# Full rebuild, Release config. Requires: make configure-opengl (run once).
build-release:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
	  echo "ERROR: Build directory not found. Run 'make configure-opengl' first." >&2; \
	  exit 1; \
	fi
	$(CMAKE) --build $(BUILD_DIR) --config Release --clean-first

# Usage: make build-increment-debug
# Incremental build, Debug config. Requires: make configure-opengl (run once).
build-increment-debug:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
	  echo "ERROR: Build directory not found. Run 'make configure-opengl' first." >&2; \
	  exit 1; \
	fi
	$(CMAKE) --build $(BUILD_DIR) --config Debug

# Usage: make build-increment-release
# Incremental build, Release config. Requires: make configure-opengl (run once).
build-increment-release:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
	  echo "ERROR: Build directory not found. Run 'make configure-opengl' first." >&2; \
	  exit 1; \
	fi
	$(CMAKE) --build $(BUILD_DIR) --config Release

# Usage: make copy-resources
# Copy original_game_resources/ into both build output dirs.
# Run once after clone or when resources change.
copy-resources: copy-resources-debug copy-resources-release

# Usage: make copy-resources-debug
copy-resources-debug:
	$(CMAKE) -E copy_directory $(RESOURCES) $(BUILD_DIR)/Debug

# Usage: make copy-resources-release
copy-resources-release:
	$(CMAKE) -E copy_directory $(RESOURCES) $(BUILD_DIR)/Release

# Usage: make run-debug
run-debug:
	cd $(BUILD_DIR)/Debug && rm -f stderr.log && ./$(EXE_NAME) 2>stderr.log

# Usage: make run-release
run-release:
	cd $(BUILD_DIR)/Release && rm -f stderr.log && ./$(EXE_NAME) 2>stderr.log

# Usage: make r
# Build Release + run.
r: build-release run-release

# Usage: make d
# Build Debug + run.
d: build-debug run-debug
