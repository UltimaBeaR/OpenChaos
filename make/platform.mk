# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Darwin)
    ifeq ($(UNAME_M),arm64)
        TOOLCHAIN := $(abspath $(SRC_DIR)/cmake/clang-arm64-macos.cmake)
    else
        TOOLCHAIN := $(abspath $(SRC_DIR)/cmake/clang-x64-macos.cmake)
    endif
    EXE_NAME := OpenChaos
else ifeq ($(UNAME_S),Linux)
    TOOLCHAIN := $(abspath $(SRC_DIR)/cmake/clang-x64-linux.cmake)
    EXE_NAME := OpenChaos
else
    # Windows (MSYS2/Git Bash)
    TOOLCHAIN := $(abspath $(SRC_DIR)/cmake/clang-x64-windows.cmake)
    EXE_NAME := OpenChaos.exe
endif
