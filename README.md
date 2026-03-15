# Urban chaos AI refactor

This is attempt to analyze original urban chaos game with AI and refactor/remake it with modern tech like language version, graphics api, controller support etc.

## Original game source code

Original code is in `original_game` folder, but it is modified to be able to build and run on modern windows (tested on windows 11) with support for Visual Studio 2026 with vcpkg package manager.

Original source code initially came from this repo - https://github.com/dizzy2003/MuckyFoot-UrbanChaos

## Building and running

Requires Visual Studio 2026 with vcpkg. Run commands from the repository root using `make`.

| Command | Description |
|---------|-------------|
| `make build-debug` | Full rebuild, Debug configuration |
| `make build-release` | Full rebuild, Release configuration |
| `make run-debug` | Run the Debug build |
| `make run-release` | Run the Release build |

