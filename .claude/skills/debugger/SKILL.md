---
name: debugger
description: >
  Attach to the running game process with cdb (Windows debugger) to inspect
  stack traces, variables, and game state. Use for crash investigation,
  freeze debugging, and understanding what the game is currently doing.
  TRIGGER: user says "attach", "where am I", "what's happening", "it froze",
  "hang", "зависло", "подключись", "посмотри что происходит".
---

# Debugger — live attach to the running game

## When to use

- **Freeze/hang**: game stops responding, crash handler does NOT fire. Attach and read the stack.
- **Understanding game state**: user asks "where am I?", "what screen is this?", what variable has what value.
- **Crash investigation**: when crash_log.txt is insufficient and you need more context.
- **Development**: user is on a level/screen and wants to discuss a feature — attach to see context.

**⚠️ NEVER attach without explicit, unambiguous user permission.** Always ask and wait for confirmation.

## Prerequisites

cdb.exe (Windows SDK Debugging Tools) must be installed. Check:
```
ls "/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe"
```
If not found, run the `debugger-install` skill.

## Core commands

### CDB path
```bash
CDB="/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe"
```

### Find the game process
```bash
tasklist | grep -i fallen
```
Note the PID (first number after the name).

### Attach and get stack of main thread
```bash
"$CDB" -p <PID> -y "C:\\WORK\\OpenChaos\\new_game\\build\\Debug" -c "~0kb;qd" 2>&1 | grep "Fallen!" | head -15
```
- `-p <PID>` — attach to process
- `-y "..."` — symbol path (PDB location, Debug build required for symbols)
- `-c "commands"` — execute and exit
- `~0kb` — stack of thread 0 (main thread), `~*kb` — all threads
- `qd` — detach and quit (game continues running!)

### Read a global variable
```bash
"$CDB" -p <PID> -y "..." -c "dd Fallen!variable_name L1;qd" 2>&1 | grep "^00007"
```
- `dd` — display DWORDs, `da` — display ASCII string, `db` — display bytes
- `L1` — show 1 element

### Dump a struct
```bash
"$CDB" -p <PID> -y "..." -c "dt Fallen!struct_variable;qd" 2>&1 | grep "+0x"
```

### Find variables by pattern
```bash
"$CDB" -p <PID> -y "..." -c "x Fallen!*pattern*;qd" 2>&1 | grep "Fallen!"
```

### Symbolize RVA to source line
After getting an RVA from crash_log.txt or cdb:
```bash
llvm-symbolizer -e c:/WORK/OpenChaos/new_game/build/Debug/Fallen.exe --relative-address <RVA>
```
`--relative-address` is mandatory!

## Interpreting the stack

The stack tells you WHERE the game is. Key function names:

| Stack contains | Meaning |
|----------------|---------|
| `game_attract_mode` + `AENG_flip` (no FRONTEND) | Main menu, attract mode |
| `game_attract_mode` + `FRONTEND_loop` + `OS_hack` | Outro screen (3D character) |
| `game_attract_mode` + `FRONTEND_loop` (no OS_hack) | Frontend submenu (Start, Options, etc.) |
| `game_loop` + `process_things` | In-game, processing game logic |
| `game_loop` + `draw_screen` + `AENG_draw` | In-game, rendering |
| `PLAYCUTS_Play` | Cutscene playing |

**Always check the stack FIRST, then variables.** Stack doesn't lie.
Variables can be stale from a previous screen.

## Useful game state variables

| Variable | Type | What it tells you |
|----------|------|-------------------|
| `menu_state` (struct) | `.title`, `.selected`, `.mode`, `.stackpos` | Current menu screen and selection |
| `mission_selected` | SWORD | Selected mission index in mission_cache[] |
| `mission_cache[N]` | struct | `.name` = mission name for index N |
| `district_selected` | SWORD | Selected district on map |
| `GAME_TURN` | ULONG | Current game tick (increases each frame in gameplay) |

## Filtering cdb output

cdb prints a lot of noise (NatVis, module loads, copyright). Filter with:
```bash
2>&1 | grep "Fallen!\|+0x0"    # for symbols and struct fields
2>&1 | grep "^00007"            # for raw memory dumps
```

## Important notes

- **Always use `qd` (quit detach)** — not `q` (quit kill)! `q` terminates the game.
- **Debug build required** for symbol names. Release build shows only RVAs.
- **Attach is non-destructive** — game pauses briefly during attach, then resumes after `qd`.
- **Game must be running** — check with `tasklist | grep -i fallen` first.
- If game crashes during attach, it was already in a bad state.
