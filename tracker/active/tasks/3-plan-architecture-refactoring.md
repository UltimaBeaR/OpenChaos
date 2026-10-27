# 3 — Plan: architecture refactoring

- Area: architecture
- Tags: plan
- Kind: targeted
- Created: 2026-10-27

## Goal
A clean final architecture: clear engine / game separation, no dead legacy branches, a testing safety net. Testing infrastructure goes first — the refactoring touches all code. Former stage 9 (part of stage 13).

## Steps
1. ⬜ [#88](88-testing-infrastructure-behavior-snapshots-before-refactoring.md) — testing infrastructure (must come first: the safety net)
2. ⬜ [#89](89-remove-the-dead-multiplayer-code-cnet.md) — remove the dead multiplayer code (CNET)
3. ⬜ Clean UC-specific collision response out of `engine/physics/` (generic detection = engine, response = game)
4. ⬜ Invert the engine → game dependency in the startup chain (the game registers its entry point in the engine)
5. ⬜ Rewrite the outro on top of the main engine (`outro/core/` is a self-contained mini-engine)
6. ⬜ Unify the input pipeline for menus
7. ⬜ Move all headers to `#pragma once`
8. ⬜ C++20 modules (consider)
9. ⬜ #107 — the renderer must not change game state
10. ⬜ Decide the game-object (Thing) design — ECS or inheritance — and refactor Thing (lists → containers, unions → variants, revisit the 700-object limit; [porting_notes.md](../../../archive/new_game_planning/porting_notes.md) §Game Objects, [tech_and_architecture.md](../../../archive/new_game_planning/tech_and_architecture.md) L238-245)
   > Details of steps 3–8: [stage13_architecture_refactoring.md](../../../archive/new_game_planning/stage13_architecture_refactoring.md).

## Notes
- Not urgent; nothing here is started.
