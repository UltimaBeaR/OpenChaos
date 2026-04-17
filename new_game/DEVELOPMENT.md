# Development Notes — OpenChaos (new game)

## Code formatting

The project uses `clang-format` to keep code style consistent. Run it periodically, especially after bulk edits. Not required to build or run the game.

### Install clang-format

**Windows:**
```
winget install LLVM.LLVM
```
Make sure to add LLVM to PATH during installation (or add `C:\Program Files\LLVM\bin` manually).

**macOS:**
```
brew install clang-format
```

### Run formatter

From the `new_game/` directory (Windows: use Git Bash):
```bash
find . -regex '.*\.\(c\|cpp\|h\|hpp\)' -exec clang-format -style=file -i {} \;
```

Config is in `.clang-format` (project root of `new_game/`).

---

## DualSense support

DualSense controller support is provided by **libDualsense** — an
OpenChaos-owned standalone library at the repo root (`../libDualsense/`
relative to `new_game/`). It has its own `CMakeLists.txt` and is pulled
in via `add_subdirectory` from `new_game/CMakeLists.txt`, then linked
as a dependency of `OpenChaos`. See `libDualsense/README.md` and
`libDualsense/API.md` for the lib's own documentation.
