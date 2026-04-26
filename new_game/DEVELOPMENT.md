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

## VSCode IntelliSense (code hints / completions)

If you want code hints, jump-to-definition, completions, and other IntelliSense features to work in VSCode, install the **clangd** extension:

```
llvm-vs-code-extensions.vscode-clangd
```

It uses the `compile_commands.json` produced by CMake to give accurate semantic info (much better than the default C/C++ extension on this codebase). The CMake build already emits `compile_commands.json` into the build directory, so no extra setup is needed beyond installing the extension.

---

## DualSense support

DualSense controller support is provided by **libDualsense** — an
OpenChaos-owned standalone library at the repo root (`../libDualsense/`
relative to `new_game/`). It has its own `CMakeLists.txt` and is pulled
in via `add_subdirectory` from `new_game/CMakeLists.txt`, then linked
as a dependency of `OpenChaos`. See `libDualsense/README.md` and
`libDualsense/API.md` for the lib's own documentation.
