# OpenChaos — Urban Chaos Modernization

<div align="center">
  <img src="./openchaos_logo_animated.gif" alt="OpenChaos - Urban Chaos Modernization">
</div>

Unofficial modernization of Urban Chaos (1999, MuckyFoot Productions) — developed using AI on top of the
[original source code](https://github.com/dizzy2003/MuckyFoot-UrbanChaos).

## How to play

1. Get a legal copy of Urban Chaos in any form (Steam, GOG, or a disc / disc image). OpenChaos needs the original game's files to run and will not work without an installed copy. Any localization/language version of the game works.
2. Go to [Releases](https://github.com/UltimaBeaR/OpenChaos/releases), open the **Assets** section of the latest release, and download the archive for your platform (for Windows it's `OpenChaos-v<version>-windows-x64.zip`)
3. Extract the archive. On Windows: copy `OpenChaos.exe` into your Urban Chaos game folder and run it. For other platforms and full details, see `OpenChaos-readme.txt` inside the downloaded archive

## How OpenChaos differs from the Urban Chaos PC release

OpenChaos is the same Urban Chaos — not a remake or a reimagining, but literally the same game. The changes here aren't meant to alter the experience; they're quality-of-life improvements that make it run and play well on modern systems: no slowdowns, glitches or crashes, proper screen resolutions, comfortable modern controls, and so on.

OpenChaos is fully playable from start to finish. It runs on three platforms — Windows, macOS (Apple Silicon) and Linux (Steam Deck) — with good performance, has no critical bugs, supports gamepads (Xbox and DualSense), and works at any screen resolution. The controls have been fully reworked and are now much more convenient.

### Gameplay changes

The game stays faithful to the original while improving and adjusting a lot of things — reworked controls and camera, combat and vehicle tweaks, and more. Full list: [GAMEPLAY_CHANGES.md](GAMEPLAY_CHANGES.md).

### Technical changes

The engine was heavily modernized under the hood — new OpenGL renderer, 64-bit, cross-platform support, unlocked frame rate, new audio and video backends, and much more. Full list: [TECHNICAL_CHANGES.md](TECHNICAL_CHANGES.md).

## Config file

OpenChaos uses its own JSON config file (replacing the original `config.ini`, except for the language option, which still stays in `config.ini`). Everything adjustable in the in-game options menu is saved there, and the file also exposes extra settings that aren't in the menu. See [CONFIG_REFERENCE.md](CONFIG_REFERENCE.md) for the file location and a full list of keys.

## Mods and localizations

**Localizations.** Any localization of the game works, including unofficial fan translations. The current language is selected the same way as in the original — via the `language` key in the `[Game]` section of `config.ini`.

**Mods.** Mods for the original game that only change game resources (game data) should work. Mods that replace or patch the game's `.exe` or `.dll` files will not work — OpenChaos is its own executable and does not run the original exe or its DLLs. If you previously installed mods into the game folder, it's best to remove those mods' `.dll` files from the folder before running OpenChaos, although most likely there won't be any problems even if you leave them in place.

## Repository

This repository contains two versions of the game:

- **New game** — the modernized version under active development (`new_game/`).
- **Original game** — the original 1999 source code by Mike Diskett, updated by community contributors
  to build and run on modern Windows (SDL2/OpenAL audio, VS2017+ support, vcpkg integration, rendering fixes).
  Kept as a read-only reference — used to verify correct behavior when modernizing.
  Upstream source: https://github.com/dizzy2003/MuckyFoot-UrbanChaos

Game resource files (textures, levels, sounds, etc.) are **not included** in this repository — they are copyrighted by the publisher and not covered by the project's MIT license. Both versions require a legal copy of the game to provide them.

### Structure

```
original_game/                — original source code (read-only reference)
original_game_resources/      — game resource files (not committed, see Setup below)
original_game_knowledge_base/ — detailed documentation on the original game
new_game/                     — modernized game (work in progress)
libDualsense/                 — OpenChaos-owned standalone DualSense support library (used by new_game)
new_game_planning/            — planning and architecture docs (phases, stages, tech decisions)
new_game_devlog/              — development log and technical notes
known_issues_and_bugs/        — known issues and bugs (active list + resolved archive)
release/                      — release packaging (scripts, per-platform assets)
make/                         — Makefile modules (platform detection, configure)
tools/                        — development utilities
legal/                        — rights history, attribution, legal details
```

## Development

Development is structured into phases and stages. See [`new_game_planning/phases.md`](new_game_planning/phases.md) for the current status.

### Future plans

Development continues after 1.0. Rough ideas and planned improvements are collected in [stage13.md](new_game_planning/stage13.md) — these are not commitments and may change.

### Issues and bugs

A large number of bugs have been fixed — both the original game's retail and pre-release bugs, and many new bugs introduced during the modernization itself. The current state is still not perfect and some bugs remain, but they are not critical.

Known issues and bugs are tracked in a separate list: [active issues](known_issues_and_bugs/known_issues_and_bugs.md) and the [resolved archive](known_issues_and_bugs/known_issues_and_bugs_resolved.md). If you run into a bug, you can also report it on [GitHub Issues](https://github.com/UltimaBeaR/OpenChaos/issues).

### Setup (for development)

- **New game** — prerequisites, build, run: [`new_game/SETUP.md`](new_game/SETUP.md)
- **Original game** — prerequisites, build, run: [`original_game/SETUP.md`](original_game/SETUP.md)

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
