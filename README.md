# OpenChaos — Fan Modernization of Urban Chaos

Fan modernization of the game Urban Chaos (1999, MuckyFoot Productions) — AI-assisted analysis and
reimplementation with modern tech: updated C++ standard, modern graphics API, controller support, etc.

This repository contains two versions of the game:

- **Original game** — the original 1999 source code by Mike Diskett, updated by community contributors
  to build and run on modern Windows (SDL2/OpenAL audio, VS2017+ support, vcpkg integration, rendering fixes).
  Kept as a read-only reference — used to verify correct behavior when modernizing.
  Upstream source: https://github.com/dizzy2003/MuckyFoot-UrbanChaos
- **New game** — the modernized version under active development (`new_game/`).
  Iterative refactor of the original: updated compiler, modern build system, eventually new renderer and cross-platform support — while keeping gameplay identical.

Both versions require a legal copy of the game for resource files (textures, levels, sounds).

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

## Setup

Game resource files (textures, levels, sounds, etc.) are **not included** in this repository —
they are copyrighted by the publisher and not covered by the MIT license of this project.

- **New game** — prerequisites, build, run: [`new_game/SETUP.md`](new_game/SETUP.md)
- **Original game** — prerequisites, build, run: [`original_game/SETUP.md`](original_game/SETUP.md)

## Development

Development is structured into phases and stages. See [`new_game_planning/phases.md`](new_game_planning/phases.md) for the current status and roadmap.

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
