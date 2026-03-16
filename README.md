# OpenChaos — Fan Modernization of Urban Chaos

Fan modernization of the game Urban Chaos (1999, MuckyFoot Productions) — AI-assisted analysis and
reimplementation with modern tech: updated C++ standard, modern graphics API, controller support, etc.

Original Urban Chaos source code: https://github.com/dizzy2003/MuckyFoot-UrbanChaos

## Repository structure

```
original_game/                — original source code (read-only reference)
original_game_resources/      — game resource files (not committed, see Setup below)
original_game_knowledge_base/ — detailed documentation on the original game
new_game/                     — modernized game (work in progress)
new_game_planning/            — planning and architecture docs (phases, stages, tech decisions)
new_game_devlog/              — development log and technical notes
tools/                        — development utilities (coan, etc.)
legal/                        — rights history, attribution, legal details
```

## Prerequisites

- **Visual Studio 2026** with the C++ Desktop workload and vcpkg integration
- **GNU make** (via [Chocolatey](https://chocolatey.org/): `choco install make`, or bundled with Git for Windows)
- **A copy of Urban Chaos** — Steam version recommended

## Setup: game resources

Game resource files (textures, levels, sounds, etc.) are **not included** in this repository.
They are copyrighted by the publisher and not covered by the MIT license of this project.

**Step 1 — Copy resources from your game installation into the repo:**

Copy everything from your Urban Chaos installation folder **except `.exe` and `.dll` files**
into `original_game_resources/`:

```
# Steam default path:
C:\Program Files (x86)\Steam\steamapps\common\Urban Chaos\
```

After this, `original_game_resources/` should contain folders like `clumps/`, `levels/`,
`Textures/`, `bink/`, etc.

**Step 2 — Copy resources into the build output folders:**

After building (see below), copy the contents of `original_game_resources/` into each
output folder you want to run:

```
# For the new game — Debug build:
new_game/fallen/Debug/

# For the new game — Release build:
new_game/fallen/Release/

# For the original game:
original_game/fallen/Debug/
```

> **Note:** `config.ini` will be overwritten during copy — this is expected.

## Building and running

Run all commands from the **repository root** using `make`:

| Command | Description |
|---------|-------------|
| `make build-debug` | Full rebuild — new game, Debug |
| `make build-release` | Full rebuild — new game, Release |
| `make run-debug` | Run the new game, Debug build |
| `make run-release` | Run the new game, Release build |

To build and run the **original game**, open `original_game/fallen/Fallen.sln` in Visual Studio
and build the Debug configuration. Then copy the resources (Step 2 above) and run
`original_game/fallen/Debug/Fallen.exe` directly.

## Development

Development is structured into phases and stages. See [`new_game_planning/phases.md`](new_game_planning/phases.md) for the current status and roadmap.

See [`new_game/README.md`](new_game/README.md) for code formatting and other development notes.

## Legal

> **Urban Chaos™** is a trademark of [My Little Planet Ltd](https://store.steampowered.com/app/243060/Urban_Chaos/).
> Original game © 1999 Mucky Foot Productions Ltd.

This is an unofficial, non-commercial, fan-made project.
The author does not claim any ownership of the Urban Chaos IP, trademark, or original game assets.

- **Source code:** [MIT license](LICENSE.md) (based on the [original release](https://github.com/dizzy2003/MuckyFoot-UrbanChaos) by Mike Diskett) — covers code only
- **Game assets:** property of the rights holder — **not included**, bring your own from a [legal copy](https://store.steampowered.com/app/243060/Urban_Chaos/)
- **Custom content** (new maps, HD models, mods): original work by project contributors

See [`legal/`](legal/) for full rights history and details.

## Credits

This project is based on the following fork chain:

| Fork | Contribution |
|------|-------------|
| **Mike Diskett** ([dizzy2003/MuckyFoot-UrbanChaos](https://github.com/dizzy2003/MuckyFoot-UrbanChaos)) | Original Urban Chaos source code, MIT release (2017) |
| **Kai Stüdemann** ([kstuedem/MuckyFoot-UrbanChaos](https://github.com/kstuedem/MuckyFoot-UrbanChaos)) | VS2017 support, SDL2/OpenAL audio, removed legacy D3D/DirectSound/DirectPlay, rendering fixes, inline assembly removal |
| **Jordan Davidson** ([jordandavidson/MuckyFoot-UrbanChaos](https://github.com/jordandavidson/MuckyFoot-UrbanChaos)) | vcpkg integration, VS2019 support |
| **PieroZ** ([PieroZ/MuckyFoot-UrbanChaos](https://github.com/PieroZ/MuckyFoot-UrbanChaos)) | Rendering fixes, editor work, mods, utilities. OpenChaos is technically forked from this repo; work used as reference for fixes and improvements |

## Language

This README and public-facing documentation are in English.
In-depth project documentation (planning, knowledge base, dev notes) is written in Russian.
