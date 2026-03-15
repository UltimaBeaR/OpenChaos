# OpenChaos — new game

Modernized version of Urban Chaos (1999, MuckyFoot Productions).

Based on the original source code by Mike Diskett: https://github.com/dizzy2003/MuckyFoot-UrbanChaos

For full setup instructions (prerequisites, resources, building) see the [root README](../README.md).

---

## Quick start

```bash
# from repository root
make build-release   # full rebuild, Release
make run-release     # launch

make build-debug     # full rebuild, Debug
make run-debug       # launch
```

After the first build, copy game resources from `original_game_resources/` into the output folder
(`fallen/Release/` or `fallen/Debug/`) — see [root README](../README.md) for details.

---

## Code formatting

The project uses `clang-format` to keep code style consistent. Run it periodically, especially after bulk edits.

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
