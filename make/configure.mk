# Auto-detect vcpkg.cmake:
#   1. VCPKG_ROOT env var (cross-platform)
#   2. Linux: ~/vcpkg (default bootstrap location)
#   3. macOS: Homebrew vcpkg
#   4. Windows: vswhere (finds VS / Build Tools installation)
VCPKG_CMAKE := $(shell \
  if [ -n "$$VCPKG_ROOT" ] && [ -f "$$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then \
    echo "$$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"; \
  elif [ "$(UNAME_S)" = "Linux" ]; then \
    if [ -f "$$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then \
      echo "$$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"; \
    fi; \
  elif [ "$(UNAME_S)" = "Darwin" ]; then \
    BREW_PREFIX=$$(brew --prefix 2>/dev/null); \
    if [ -n "$$BREW_PREFIX" ] && [ -f "$$BREW_PREFIX/share/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then \
      echo "$$BREW_PREFIX/share/vcpkg/scripts/buildsystems/vcpkg.cmake"; \
    fi; \
  else \
    VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"; \
    if [ -f "$$VSWHERE" ]; then \
      VS_PATH=$$("$$VSWHERE" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>/dev/null); \
      if [ -n "$$VS_PATH" ] && [ -f "$$VS_PATH/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then \
        echo "$$VS_PATH/VC/vcpkg/scripts/buildsystems/vcpkg.cmake"; \
      fi; \
    fi; \
  fi)

define run_configure
	@if [ -z "$(VCPKG_CMAKE)" ]; then \
	  echo "ERROR: vcpkg.cmake not found." >&2; \
	  echo "  Set VCPKG_ROOT env var, or:" >&2; \
	  if [ "$(UNAME_S)" = "Darwin" ]; then \
	    echo "  brew install vcpkg" >&2; \
	  elif [ "$(UNAME_S)" = "Linux" ]; then \
	    echo "  git clone https://github.com/microsoft/vcpkg ~/vcpkg && ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics" >&2; \
	  else \
	    echo "  Install VS Build Tools with C++ workload" >&2; \
	  fi; \
	  exit 1; \
	fi
	$(CMAKE) -S $(SRC_DIR) -B $(BUILD_DIR) -G "Ninja Multi-Config" \
	  "-DCMAKE_TOOLCHAIN_FILE=$(VCPKG_CMAKE)" \
	  "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$(TOOLCHAIN)" \
	  "-DVCPKG_INSTALLED_DIR=$(abspath $(SRC_DIR)/vcpkg_installed)" \
	  "-DCMAKE_MAKE_PROGRAM=$(shell which ninja)" \
	  $(CMAKE_EXTRA_ARGS)
endef

# Usage: make configure
# Run once on first clone, re-run after CMakeLists.txt changes.
# CMake + Ninja Multi-Config, vcpkg packages installed automatically.
configure:
	$(call run_configure)

# Usage: make configure-asan
# Enable AddressSanitizer for memory error detection. Windows only.
configure-asan:
ifeq ($(UNAME_S),Linux)
	@echo "ERROR: configure-asan is only supported on Windows." >&2; exit 1
else ifeq ($(UNAME_S),Darwin)
	@echo "ERROR: configure-asan is only supported on Windows." >&2; exit 1
else
	CMAKE_EXTRA_ARGS="-DENABLE_ASAN=ON" $(MAKE) configure
	@ASAN_DLL=$$(clang -print-file-name=clang_rt.asan_dynamic-x86_64.dll); \
	  if [ -f "$$ASAN_DLL" ]; then \
	    cp "$$ASAN_DLL" $(BUILD_DIR)/Debug/ 2>/dev/null; \
	    cp "$$ASAN_DLL" $(BUILD_DIR)/Release/ 2>/dev/null; \
	    echo "ASan DLL copied to build dirs"; \
	  else \
	    echo "WARNING: ASan DLL not found at $$ASAN_DLL"; \
	  fi
endif
