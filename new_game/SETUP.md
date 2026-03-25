# Setup Guide — OpenChaos (new game)

## Prerequisites

### Visual Studio 2026 Community

Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/).

During installation, select the **Desktop development with C++** workload —
this includes MSVC, Windows SDK, vcpkg, and other essentials automatically.

Additionally, in "Individual components", make sure these are checked:

- **C++ Clang Compiler for Windows** (provides `clang-cl`)
- **C++ Clang-cl tools for Windows** (x86/x64)
- **C++ CMake tools for Windows** (provides `cmake.exe` used by the build scripts)

### GNU make

Included with [Git for Windows](https://gitforwindows.org/) (available in Git Bash).
Or install via Chocolatey: `choco install make`

### Urban Chaos (legal copy)

Steam version recommended: [store.steampowered.com/app/243060/Urban_Chaos/](https://store.steampowered.com/app/243060/Urban_Chaos/)

---

## First-time setup

### Step 1 — Copy game resources into the repository

Copy everything from your Urban Chaos installation folder **except `.exe` and `.dll` files**
into `original_game_resources/` in the repository root.

Steam default path:
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

This sets up the VS x86 build environment (`vcvarsall.bat x86`) and runs CMake.
**vcpkg packages (SDL3, OpenAL, fmt) are installed automatically** into `new_game/vcpkg_installed/`
— no separate vcpkg command needed. Re-run if `CMakeLists.txt` changes.

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
| `make reconfigure` | Alias for `make configure` |
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
      Fallen.exe, *.dll, text/, config.ini   ← runtime output
    Release/
      Fallen.exe, *.dll, text/, config.ini   ← runtime output
    CMakeFiles/                               ← intermediate .obj files (per-config, separated)
  vcpkg_installed/                           ← vcpkg packages (gitignored, like node_modules)
```

`build/` and `vcpkg_installed/` are gitignored and not committed.

