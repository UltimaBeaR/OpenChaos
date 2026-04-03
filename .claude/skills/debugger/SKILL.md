---
name: debugger
description: >
  Attach to the running game process with cdb (Windows debugger) to inspect
  stack traces, variables, and game state. Use for crash investigation,
  freeze debugging, and understanding what the game is currently doing.
  Use this skill whenever the game is frozen/hung and crash handler didn't fire,
  when you need to read a global variable's current value, or when crash_log.txt
  isn't enough and you need live process inspection. Also consider this skill
  when the user describes behavior that suggests the game is stuck in a loop
  or waiting on something.
  TRIGGER: user says "attach", "where am I", "what's happening", "it froze",
  "hang", "зависло", "подключись", "посмотри что происходит", "повисло",
  "не отвечает", "game stuck".
---

# Debugger — live attach to the running game

## Quick reference

```bash
CDB="/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe"
PID=$(tasklist | grep -i openchaos | awk '{print $2}')
"$CDB" -p $PID -y "C:\\WORK\\OpenChaos\\new_game\\build\\Debug" -c "~0kb;qd" 2>&1 | grep "OpenChaos!" | head -15
```

Always use `qd` (detach) not `q` (kills the process).

## When to use

- **Freeze/hang**: game stops responding, crash handler does NOT fire — attach and read the stack
- **Understanding game state**: what screen is this, what variable has what value
- **Crash investigation**: when crash_log.txt is insufficient and you need more context
- **Development**: user is on a level/screen and wants to discuss a feature — attach to see context

**Never attach without the user's explicit permission.** Always ask first and wait for confirmation.

## Prerequisites

cdb.exe must be installed. Check:
```bash
ls "/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe"
```
If not found, use the `debugger-install` skill.

## Commands

### Find the game process
```bash
tasklist | grep -i openchaos
```

### Stack of main thread
```bash
"$CDB" -p <PID> -y "C:\\WORK\\OpenChaos\\new_game\\build\\Debug" -c "~0kb;qd" 2>&1 | grep "OpenChaos!" | head -15
```
- `~0kb` = main thread stack, `~*kb` = all threads
- `-y` = symbol path (PDB location, Debug build needed for symbols)

### Read a global variable
```bash
"$CDB" -p <PID> -y "..." -c "dd OpenChaos!variable_name L1;qd" 2>&1 | grep "^00007"
```
`dd` = DWORDs, `da` = ASCII string, `db` = bytes, `L1` = 1 element

### Dump a struct
```bash
"$CDB" -p <PID> -y "..." -c "dt OpenChaos!struct_variable;qd" 2>&1 | grep "+0x"
```

### Find variables by pattern
```bash
"$CDB" -p <PID> -y "..." -c "x OpenChaos!*pattern*;qd" 2>&1 | grep "OpenChaos!"
```

### Symbolize RVA to source line
```bash
llvm-symbolizer -e c:/WORK/OpenChaos/new_game/build/Debug/OpenChaos.exe --relative-address <RVA>
```
`--relative-address` is mandatory.

## Interpreting the stack

The stack tells you WHERE the game is:

| Stack contains | Meaning |
|----------------|---------|
| `game_attract_mode` + `AENG_flip` (no FRONTEND) | Main menu, attract mode |
| `game_attract_mode` + `FRONTEND_loop` + `OS_hack` | Outro screen (3D character) |
| `game_attract_mode` + `FRONTEND_loop` (no OS_hack) | Frontend submenu |
| `game_loop` + `process_things` | In-game, processing logic |
| `game_loop` + `draw_screen` + `AENG_draw` | In-game, rendering |
| `PLAYCUTS_Play` | Cutscene playing |

Check the stack FIRST, then variables. Stack doesn't lie — variables can be stale.

## Useful game state variables

| Variable | What it tells you |
|----------|-------------------|
| `menu_state` (struct) | Current menu screen and selection |
| `mission_selected` | Selected mission index |
| `district_selected` | Selected district on map |
| `GAME_TURN` | Current game tick |

## Filtering cdb noise

cdb prints a lot of noise. Filter with:
```bash
2>&1 | grep "OpenChaos!\|+0x0"    # symbols and struct fields
2>&1 | grep "^00007"            # raw memory dumps
```

## Important

- **`qd` = detach** (game continues). **`q` = kill** (terminates the game). Always use `qd`.
- **Debug build required** for symbol names. Release shows only RVAs.
- **Attach is non-destructive** — game pauses briefly, then resumes after `qd`.
- If game crashes during attach, it was already in a bad state.
