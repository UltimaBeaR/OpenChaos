# 89 — Remove the dead multiplayer code (CNET)

- Status: 🔵 open
- Area: architecture
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`; step of plan [#3](3-plan-architecture-refactoring.md)

## Description
The original had network multiplayer via `CNET_*` (Chaos Net, MuckyFoot's network layer over DirectPlay/IPX). Currently in our codebase **the `CNET_network_game` flag is never set to true anywhere** (zero-init and that's it) — so in fact multiplayer doesn't work. But **CNET branches are scattered through the code** as `if (CNET_network_game) { ... } else { ... }` and `if (!CNET_network_game)` in ~20+ files: `pcom.cpp`, `elev.cpp`, `combat.cpp`, `pyro.cpp`, `music.cpp`, `sound.cpp`, `figure.cpp`, `aeng.cpp`, `game.cpp`, `game_tick.cpp`, `input_actions.cpp`, `eway.cpp`, `memory.cpp`, `playcuts.cpp`, `wand.cpp`, `thing.cpp`, and others. The global is defined in [`network_state_globals.cpp`](../../../new_game/src/game/network_state_globals.cpp). **What needs to be done:** remove the CNET branches entirely (keep only the non-network path), delete the `CNET_network_game` global, delete the `network_state_globals.*` files. Along the way, pre-existing bugs in these branches may be found. **⚠️ IMPORTANT:** in debug mode there's a **split screen** which apparently works — possibly this is that very "network" code (split screen via the same mechanism as multiplayer). Before removal — figure out whether split screen uses the CNET branches. If yes — split screen should either be preserved (a separate path), or also removed as a debug-only feature. Not a blocker. Tech debt.

Old-list status: *Implement (cleanup, accounting for split screen)*.
