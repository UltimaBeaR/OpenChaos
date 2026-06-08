# Architecture refactoring (part of Stage 13)

Formerly Stage 9. Done AFTER the test infrastructure.

**Goal:** clean final architecture.

- Clear engine / game separation
- Refactor the Thing system
- Improve test coverage
- Other architectural improvements

## Known tasks

### Cut out the dead multiplayer code

`game/network_state_globals.*` — a stub (CNET_network_game is always FALSE, CNET_num_players is always 1).
~50 places in the code check these variables (`if (CNET_network_game)`, loops over `CNET_num_players`).
All these branches are dead code: multiplayer has been removed. Cut the branches, remove the globals, simplify the code.

### Clean UC-specific logic out of `engine/physics/`

`collide.cpp` contains collision response tied to UC gameplay. Generic collision detection — engine, UC-specific response — the game layer.

### Invert the engine → game dependency in the startup chain

Currently: `main.cpp → engine/platform/host.cpp → game/startup.cpp → game/game.cpp`
The engine directly calls the game (HOST_run → MF_main) — a DAG violation.

Correct: the game registers its entry point (callback) in the engine during setup,
and the engine calls it when ready. Dependency: game → engine, not the other way around.

### Rewrite the outro on top of the main engine

`outro/core/` — a self-contained mini-engine for the end credits (its own renderer, matrices, font, TGA loader, camera). Duplicates of engine systems. Replace with calls into the main engine.

### Unify the input pipeline for menus

**Problem (discovered in Stage 5.1):**

Currently the keyboard and gamepad reach menus via two parallel paths glued together with `||`:

```cpp
// frontend.cpp, pause menu, gamemenu, widget — this pattern everywhere:
if (Keys[KB_UP] || (input & INPUT_MASK_FORWARDS)) { ... }
```

**Keyboard:** `Keys[]` → filled directly by Win32/DirectInput → menus read it without intermediaries.
No debounce — a held key fires every frame. Had to add a separate `kb_dir_ticker`.

**Gamepad (stick):** `the_state.lX/lY` → each menu computes the deadzone and hysteresis **itself** →
sets the `INPUT_MASK_*` bits → passes them through its own `dir_ticker`. Deadzones/hysteresis are duplicated
between frontend.cpp, gamemenu.cpp, pause.cpp.

**Gamepad (D-Pad):** the `lX/lY` axes are set in `gamepad.cpp` (0 or 65535), follow the same path
as the stick, but without the wobble problem (the values are discrete).

**Problems with the current approach:**
- Two separate tickers (keyboard + gamepad) with different parameters in each menu
- Deadzone and hysteresis for the stick are duplicated in every menu file (frontend, gamemenu, pause)
- Gamepad buttons are partly via `INPUT_MASK_*` (through `get_hardware_input`), partly
  via direct reads of `the_state.rgbButtons[]` — there is no uniformity
- `gamemenu.cpp` injects `Keys[KB_ESC/UP/DOWN/ENTER] = 1` for compatibility with keyboard-only
  code, which creates side effects (kicking Darci when leaving pause — had to add
  `gamepad_consume_until_released()`)
- Every new menu requires copy-pasting all the input logic

**Target architecture:**

```
All devices → single queue of menu actions → menus read actions
```

A single `menu_input` module (or an extension of the gamepad layer):
- Takes raw input from all devices (keyboard `Keys[]`, gamepad `the_state`, mouse in the future)
- Deadzone, hysteresis, dominant-axis — in one place
- Emits a `MenuAction` enum: `MENU_UP`, `MENU_DOWN`, `MENU_LEFT`, `MENU_RIGHT`,
  `MENU_CONFIRM`, `MENU_CANCEL`, `MENU_START`
- Repeat/debounce — once, with the same parameters for all devices
- Consumption when a menu closes — built into the system (not a separate hack)

A menu just does:
```cpp
MenuAction action = menu_input_poll();
if (action == MENU_UP) { ... }
if (action == MENU_CONFIRM) { ... }
```

**Files to refactor:**
- `ui/frontend/frontend.cpp` — `FRONTEND_input()` (~100 lines of input logic)
- `ui/menus/gamemenu.cpp` — `GAMEMENU_process()` (~60 lines of gamepad injection)
- `ui/menus/pause.cpp` — `PAUSE_handler()` (~40 lines)
- `ui/menus/widget.cpp` — `FORM_Process()` (~20 lines)
- `engine/input/gamepad.cpp` — `gamepad_consume_until_released()` (a crutch, replace with the built-in mechanism)

### Move all headers to `#pragma once`

Some headers (especially the old ones from `src/`) use classic include guards (`#ifndef`/`#define`/`#endif`). New code (libDualsense, etc.) is already on `#pragma once`. Unify the whole project on `#pragma once` — supported by Clang, GCC, MSVC.

### C++20 modules

Conversion of .h/.cpp → C++20 modules. Was planned as part of Stage 4, but moved here —
a stable architecture is needed first (after replacing the renderer and cleaning up dependencies).
The dependency DAG is already built (`tools/st_4_2_dep_graph.py`), the folder structure is finalized.
Requires Clang 20+ and CMake 3.28+ (CXX_MODULE_STD). The modules standard is the same in C++20 and C++23.

> **Note:** the tasks for separating D3D and splitting up `uc_common.h` were moved into the pre-stage of [stage 7](stage7.md) and completed. The D3D6 backend was removed (2026-04-08).
