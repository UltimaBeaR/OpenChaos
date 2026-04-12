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

#### Git for Windows

Required — the Makefile uses `bash` and Unix utilities (`sed`, `zip`, etc.) which come bundled with Git for Windows.

Download: [gitforwindows.org](https://gitforwindows.org/)

During installation, select **"Use Git and optional Unix tools from the Command Prompt"** — this adds `bash`, `sed`, `zip` and other tools to PATH, making them available from `cmd`.

#### GNU make

```bash
winget install GnuWin32.Make
```

Also included with Git for Windows (available in Git Bash).

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
vcpkg packages (SDL3, OpenAL, fmt) are installed automatically during `make configure`.

### Linux (Steam Deck / SteamOS)

Tested on Steam Deck (SteamOS 3.x). Other distros may work but are not tested.

#### Build tools

SteamOS has a read-only root filesystem — system packages installed via `pacman` get wiped on OS updates.
The tested approach uses Flatpak SDK extensions and user-local installs.

**CMake, Ninja, Make** — should already be available if you have VS Code Flatpak with
`org.freedesktop.Sdk` (which includes `cmake`, `ninja`, `make`). Verify with the
commands at the end of this section.

**Clang 20:**

1. Install the Flatpak SDK extension:
   ```bash
   flatpak install flathub org.freedesktop.Sdk.Extension.llvm20//25.08
   ```

2. The extension files live inside Flatpak's OSTree store and are only directly accessible
   from within a Flatpak sandbox (e.g. VS Code terminal). To make them available system-wide,
   copy the binaries and libs to `~/.local/bin/`:

   From a **VS Code terminal** (inside Flatpak sandbox), the extension is at `/usr/lib/sdk/llvm20/`.
   Copy the needed files:
   ```bash
   mkdir -p ~/.local/bin/llvm20/lib

   # Binaries
   cp /usr/lib/sdk/llvm20/bin/clang    ~/.local/bin/llvm20/
   cp /usr/lib/sdk/llvm20/bin/clang++  ~/.local/bin/llvm20/
   cp /usr/lib/sdk/llvm20/bin/clang-20 ~/.local/bin/llvm20/
   cp /usr/lib/sdk/llvm20/bin/lld      ~/.local/bin/llvm20/

   # Shared libraries (required at runtime)
   cp /usr/lib/sdk/llvm20/lib/libclang-cpp.so.20.1 ~/.local/bin/llvm20/lib/
   cp /usr/lib/sdk/llvm20/lib/libLLVM-20.so        ~/.local/bin/llvm20/lib/
   cp /usr/lib/sdk/llvm20/lib/libLLVM.so.20.1      ~/.local/bin/llvm20/lib/

   # Clang resource directory (headers, builtins)
   cp -r /usr/lib/sdk/llvm20/lib/clang ~/.local/bin/llvm20/lib/

   # Symlink so clang finds its resource dir
   mkdir -p ~/.local/bin/lib/clang
   ln -sf ~/.local/bin/llvm20/lib/clang/20 ~/.local/bin/lib/clang/20
   ```

3. Create wrapper scripts that set `LD_LIBRARY_PATH`:
   ```bash
   cat > ~/.local/bin/clang << 'EOF'
   #!/bin/sh
   export LD_LIBRARY_PATH="/home/deck/.local/bin/llvm20/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
   exec /home/deck/.local/bin/llvm20/clang "$@"
   EOF

   cat > ~/.local/bin/clang++ << 'EOF'
   #!/bin/sh
   export LD_LIBRARY_PATH="/home/deck/.local/bin/llvm20/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
   exec /home/deck/.local/bin/llvm20/clang++ "$@"
   EOF

   chmod +x ~/.local/bin/clang ~/.local/bin/clang++
   ```

   Note: replace `/home/deck` with your actual home directory if different.

`~/.local/bin` is added to `PATH` automatically by the Makefile (Linux only).

Verify:
```bash
cmake --version    # 3.25+
ninja --version    # any
clang++ --version  # 20+
```

#### vcpkg

Clone vcpkg into your home directory and bootstrap it:

```bash
git clone --depth=1 https://github.com/microsoft/vcpkg ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
```

The Makefile auto-detects `~/vcpkg`. Alternatively, set `VCPKG_ROOT` env var
to point to your vcpkg installation.

vcpkg packages (SDL3, OpenAL, FFmpeg) are installed automatically during `make configure`.
On the first run this takes ~15-20 minutes (vcpkg builds everything from source for Linux).

If `make configure` fails because vcpkg can't find the baseline commit, fetch it:
```bash
cd ~/vcpkg && git fetch --depth=1 origin <commit-hash-from-error-message>
```

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

```bash
# from repository root
make configure
```

This runs CMake with Ninja Multi-Config generator. vcpkg is auto-detected
(via `vswhere` on Windows, `VCPKG_ROOT` env var on macOS/Linux).
**vcpkg packages (SDL3, OpenAL, fmt) are installed automatically** into `new_game/vcpkg_installed/`
— no separate vcpkg command needed. Re-run after `CMakeLists.txt` changes.

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
| `make configure` | CMake configure — run once, or after `CMakeLists.txt` changes |
| `make configure-asan` | CMake configure with AddressSanitizer enabled |
| `make build-debug` | Full rebuild — Debug |
| `make build-release` | Full rebuild — Release |
| `make copy-resources` | Copy game resources into Debug and Release output folders |
| `make copy-resources-debug` | Copy resources into Debug only |
| `make copy-resources-release` | Copy resources into Release only |
| `make run-debug` | Launch Debug build |
| `make run-release` | Launch Release build |
| `make r` | Build Release + launch (stops if build fails) |
| `make d` | Build Debug + launch (stops if build fails) |
| `make release-package VERSION=X.Y.Z` | Package Release build into a zip for distribution |

---

## Build output layout

```
new_game/
  build/
    Debug/
      OpenChaos (OpenChaos.exe on Windows), text/, config.ini   ← runtime output
    Release/
      OpenChaos (OpenChaos.exe on Windows), text/, config.ini   ← runtime output
    CMakeFiles/                                            ← intermediate .obj files
  vcpkg_installed/                                         ← vcpkg packages (gitignored)
```

`build/` and `vcpkg_installed/` are gitignored and not committed.
