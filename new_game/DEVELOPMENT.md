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

## Vendored libraries

### Dualsense-Multiplatform

Located in `libs/dualsense-multiplatform/`. This library is vendored (not managed by vcpkg) and must be updated manually when needed. See [libs/dualsense-multiplatform/README.md](libs/dualsense-multiplatform/README.md) for update instructions.
