# OpenChaos — Urban Chaos Modernization

Unofficial modernization of Urban Chaos (1999, MuckyFoot Productions) — built with AI on top of the
[original source code](https://github.com/dizzy2003/MuckyFoot-UrbanChaos).

## How to play

1. Go to [Releases](https://github.com/UltimaBeaR/OpenChaos/releases) and download the archive for your platform
2. Extract the archive and follow the instructions in `OpenChaos-readme.txt` inside

## Current status

Work in progress — alpha builds available. The game is playable through all missions on Windows, macOS (Apple Silicon) and Linux (Steam Deck). Preparing for release 1.0.

- Runs on modern systems: Windows, macOS (Apple Silicon), Linux (Steam Deck)
- Gamepad support: Xbox controllers (including Steam Deck) and DualSense (adaptive triggers, vibration, LED lightbar — functional but not yet polished)
- Not yet stable — crashes and bugs are expected

<details>
<summary>Technical details (for developers)</summary>

- Graphics: OpenGL 4.1 Core Profile (replacing original Direct3D 6)
- Ported from 32-bit to 64-bit (x64 on Windows/Linux, arm64 on macOS)
- Build system: CMake + Ninja + Clang++, vcpkg (replacing original MSVC project files)
- Codebase restructured into modules (`engine/`, `game/`, `ui/`, `outro/`, etc.) — original architecture preserved, every moved entity traceable via `// uc_orig:` comments
- Various bug fixes (original pre-release and new)
- Removed unused platform code (PSX, Dreamcast, Glide), cut features, and the level editor; almost no preprocessor conditionals left

</details>

## Roadmap

### Release 1.0 — the original game, polished

- Stability: all missions completable without crashes
- Visuals matching the original retail PC release
- Full gamepad support (Xbox, DualSense with adaptive triggers and vibration)
- Resolution support (1080p–4K), fullscreen/windowed mode, proper aspect ratio on all devices
- New config file format (replacing the original INI) for graphics, audio, controls
- Fully tested and polished on all supported platforms

### Post-1.0 — improvements

These are rough ideas, not commitments. Plans may change, and not everything listed here will necessarily be implemented.

- Gameplay tweaks and quality-of-life changes
- Visual enhancements (better lighting, effects, post-processing)
- Control scheme improvements (mouse aiming, remappable controls)
- In-mission quicksaves
- Architecture refactoring and test infrastructure
- Custom maps/missions (AI-assisted level design)
- Third-party mod support

## Repository

This repository contains two versions of the game:

- **New game** — the modernized version under active development (`new_game/`).
- **Original game** — the original 1999 source code by Mike Diskett, updated by community contributors
  to build and run on modern Windows (SDL2/OpenAL audio, VS2017+ support, vcpkg integration, rendering fixes).
  Kept as a read-only reference — used to verify correct behavior when modernizing.
  Upstream source: https://github.com/dizzy2003/MuckyFoot-UrbanChaos

Both versions require a legal copy of the game for resource files (textures, levels, sounds).

### Structure

```
original_game/                — original source code (read-only reference)
original_game_resources/      — game resource files (not committed, see Setup below)
original_game_knowledge_base/ — detailed documentation on the original game
new_game/                     — modernized game (work in progress)
libDualsense/                 — OpenChaos-owned standalone DualSense support library (used by new_game)
new_game_planning/            — planning and architecture docs (phases, stages, tech decisions)
new_game_devlog/              — development log and technical notes
release/                      — release packaging (scripts, per-platform assets)
make/                         — Makefile modules (platform detection, configure)
tools/                        — development utilities
legal/                        — rights history, attribution, legal details
```

## Setup

Game resource files (textures, levels, sounds, etc.) are **not included** in this repository —
they are copyrighted by the publisher and not covered by the MIT license of this project.

- **New game** — prerequisites, build, run: [`new_game/SETUP.md`](new_game/SETUP.md)
- **Original game** — prerequisites, build, run: [`original_game/SETUP.md`](original_game/SETUP.md)

## Development

Development is structured into phases and stages. See [`new_game_planning/phases.md`](new_game_planning/phases.md) for the current status.

## Legal

> **Urban Chaos™** is a trademark of [My Little Planet Ltd](https://store.steampowered.com/app/243060/Urban_Chaos/).
> Original game © 1999 Mucky Foot Productions Ltd.

This is an unofficial, non-commercial, fan-made project.
The author does not claim any ownership of the Urban Chaos IP, trademark, or original game assets.

- **Source code:** [MIT license](LICENSE) (based on the [original release](https://github.com/dizzy2003/MuckyFoot-UrbanChaos) by Mike Diskett) — covers code only
- **Game assets:** property of the rights holder — **not included**, bring your own from a [legal copy](https://store.steampowered.com/app/243060/Urban_Chaos/)
- **Custom content** (new maps, HD models, mods): original work by project contributors

See [`legal/`](legal/) for full rights history and details.

## Credits

This project is based on the following fork chain:

| Fork                                                                                                                | Contribution                                                                                                                                     |
| ------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Mike Diskett** ([dizzy2003/MuckyFoot-UrbanChaos](https://github.com/dizzy2003/MuckyFoot-UrbanChaos))              | Original Urban Chaos source code, MIT release (2017)                                                                                             |
| **Kai Stüdemann** ([kstuedem/MuckyFoot-UrbanChaos](https://github.com/kstuedem/MuckyFoot-UrbanChaos))               | VS2017 support, SDL2/OpenAL audio, removed legacy D3D/DirectSound/DirectPlay, rendering fixes, inline assembly removal                           |
| **Jordan Davidson** ([jordandavidson/MuckyFoot-UrbanChaos](https://github.com/jordandavidson/MuckyFoot-UrbanChaos)) | vcpkg integration, VS2019 support                                                                                                                |
| **PieroZ** ([PieroZ/MuckyFoot-UrbanChaos](https://github.com/PieroZ/MuckyFoot-UrbanChaos))                          | Rendering fixes, editor work, mods, utilities. OpenChaos is technically forked from this repo; work used as reference for fixes and improvements |

## Language

This README and public-facing documentation are in English.
In-depth project documentation (planning, knowledge base, dev notes) is written in Russian — this is the developer's native language, chosen for convenience during solo development. If the project gains active community contributors, all internal documentation can be translated to English — this would be easy to do.
