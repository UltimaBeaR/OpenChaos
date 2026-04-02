# Setup Guide — OpenChaos (new game)

## Prerequisites

### Windows

#### Build tools

Install the following (all available via `winget`):

```bash
winget install Kitware.CMake        # CMake build system
winget install Ninja-build.Ninja    # Ninja build tool
winget install LLVM.LLVM            # Clang/Clang++ compiler (version 20+)
```

After installing, **restart your terminal** so the new PATH entries take effect.

Verify:
```bash
cmake --version    # 3.25+
ninja --version    # any
clang++ --version  # 20+
```

#### Windows SDK + MSVC runtime

`clang++` needs Windows SDK headers/libs and the MSVC C++ runtime to compile and link.
It auto-detects the installation — no manual environment setup (`vcvarsall.bat`) required.

**Option A — VS Build Tools (lightweight, ~3 GB):**
```bash
winget install Microsoft.VisualStudio.2022.BuildTools
```
Then open **Visual Studio Installer**, click **Modify** on Build Tools, and select
the **Desktop development with C++** workload.

**Option B — Full Visual Studio** (if you already have it):
Download [Visual Studio 2022/2026 Community](https://visualstudio.microsoft.com/) and select
the **Desktop development with C++** workload during installation.

#### vcpkg

vcpkg is used for dependency management (SDL3, OpenAL, fmt).
Included with VS Build Tools / Visual Studio (C++ workload) — no extra setup needed.
The build scripts auto-detect the installation via `vswhere`.

#### GNU make

```bash
winget install GnuWin32.Make
```

Also included with [Git for Windows](https://gitforwindows.org/) (available in Git Bash).

### macOS (Apple Silicon M1+)

#### Xcode Command Line Tools

Provides Apple Clang (C/C++ compiler), linker, and system headers.

```bash
xcode-select --install
```

If you already have Xcode installed, this step is not needed.

#### Homebrew packages

```bash
brew install cmake ninja pkg-config
```

Verify:
```bash
cmake --version    # 3.25+
ninja --version    # any
clang++ --version  # Apple clang 15+
```

#### vcpkg

Clone the vcpkg repository into Homebrew's share directory:

```bash
git clone https://github.com/microsoft/vcpkg.git "$(brew --prefix)/share/vcpkg"
```

That's it — the Makefile auto-detects this location via `brew --prefix`.
vcpkg packages (SDL3, OpenAL, fmt) are installed automatically during `make configure-opengl`.

### Common

#### Urban Chaos (legal copy)

Steam version recommended: [store.steampowered.com/app/243060/Urban_Chaos/](https://store.steampowered.com/app/243060/Urban_Chaos/)

---

## First-time setup

### Step 1 — Copy game resources into the repository

Copy everything from your Urban Chaos installation folder **except `.exe` and `.dll` files**
into `original_game_resources/` in the repository root.

Steam default path (Windows):
```
C:\Program Files (x86)\Steam\steamapps\common\Urban Chaos\
```

After this, `original_game_resources/` should contain folders like `clumps/`, `levels/`,
`Textures/`, `bink/`, `text/`, and the file `config.ini`.

**Save files:** if you want to carry over saves, place them in `original_game_resources/saves/`
as `slot0.wag`, `slot1.wag`, etc. They will be copied into the build output folders
together with the rest of the resources (see Step 3).

### Step 2 — Configure

Choose a graphics backend and configure:

```bash
# from repository root
make configure-opengl   # OpenGL 4.1 — cross-platform (recommended)
make configure-d3d6     # DirectX 6 — Windows-only legacy backend
```

This runs CMake with Ninja Multi-Config generator. vcpkg is auto-detected
(via `vswhere` on Windows, `VCPKG_ROOT` env var on macOS/Linux).
**vcpkg packages (SDL3, OpenAL, fmt) are installed automatically** into `new_game/vcpkg_installed/`
— no separate vcpkg command needed. Re-run when switching backends or after `CMakeLists.txt` changes.

### Step 3 — Prepare build output (copy resources into build folders)

```bash
make copy-resources
```

Copies `original_game_resources/` contents into `new_game/build/Debug/` and `new_game/build/Release/`.
Re-run if resources change.

### Step 4 — Build

```bash
make build-release   # full rebuild, Release
make build-debug     # full rebuild, Debug
```

### Step 5 — Run

```bash
make run-release
make run-debug
```

---

## All make commands

| Command | Description |
|---------|-------------|
| `make configure-opengl` | CMake configure with OpenGL backend — run once, or after `CMakeLists.txt` changes |
| `make configure-d3d6` | CMake configure with DirectX 6 backend (Windows-only) |
| `make build` | Full rebuild — Debug and Release |
| `make build-debug` | Full rebuild — Debug |
| `make build-release` | Full rebuild — Release |
| `make build-increment-debug` | Incremental build — Debug |
| `make build-increment-release` | Incremental build — Release |
| `make copy-resources` | Copy game resources into Debug and Release output folders |
| `make copy-resources-debug` | Copy resources into Debug only |
| `make copy-resources-release` | Copy resources into Release only |
| `make run-debug` | Launch Debug build |
| `make run-release` | Launch Release build |
| `make r` | Build Release + launch (stops if build fails) |
| `make d` | Build Debug + launch (stops if build fails) |

---

## Build output layout

```
new_game/
  build/
    Debug/
      Fallen (Fallen.exe on Windows), text/, config.ini   ← runtime output
    Release/
      Fallen (Fallen.exe on Windows), text/, config.ini   ← runtime output
    CMakeFiles/                                            ← intermediate .obj files
  vcpkg_installed/                                         ← vcpkg packages (gitignored)
```

`build/` and `vcpkg_installed/` are gitignored and not committed.
