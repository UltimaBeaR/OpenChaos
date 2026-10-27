# Stage 13 — Complete dead code cleanup

**Current working cleanup artifact:** [stage13_dead_code_report.md](stage13_dead_code_report.md) — list of candidates from the latest `--gc-sections` run, split into leaf vs transitive, progress by iteration.

## ⚠️ ROCK-SOLID RULES FOR THIS CLEANUP

**Goal — ZERO risk of breaking anything.** No "while we're in here — let's also fix this".

1. **ONLY removing dead symbols.** No refactorings, no "defensive fix"-es, no changes to live code.
2. **The linker `--gc-sections` is the only authority** on what's dead. The symbol must be in the discarded list.
3. **Verify every symbol with grep over `new_game/src/`** — make sure there are no real callers outside the dead-set.
4. **Do NOT remove function calls** — even if the call looks like a no-op. That's a **code change**, not dead-code removal.
5. **Do NOT add early-return / nullptr-check / "defensive" checks** to live functions — that's a code change.
6. **If you find a bug during cleanup** — record it in a separate file under `new_game_devlog/` and DO NOT FIX IT. Hand it off to a separate iteration (not this session).
7. **If in doubt** — skip the symbol. Better to leave dead code than break something live.
8. **Take symbol lists ONLY via `grep` from real files** (`/tmp/dc/all_dead_per_file.txt`, build log, llvm-nm output). **NOT from the model's memory/context** — there was a case where a hallucinated list led to deleting live functions (reverted). Every symbol before deletion — verify with grep that it really exists in the file and really has no external callers.
9. **Do NOT use subagents for deletion.** The risk of handing them a wrong/hallucinated list is higher than the risk of doing everything yourself. Work only in the main context, sequentially, file by file.
10. **Regenerate the dead-list between batches** if the previous regeneration is older than one session — after deletions the linker discarded set changes, and the old list goes stale.
11. **A discarded symbol ≠ you can delete its definition**, if it has source-level callers — even if those callers are themselves dead (also in the discarded list). The non-gc-sections debug build will break, because the compiler processes the calling function as a whole. The only exception: callers are commented out in `/* */` or `#if 0` — then the compiler doesn't see them and deletion is safe.
12. **Before deleting a whole .cpp/.h file** — check not only functions, but also **types** (structs, typedefs, enums) from the .h. If some type from the .h is used in live code in another file — the file cannot be deleted, even if all functions are in the discarded list. Example: `IC_pack`/`IC_unpack` discarded, but `IC_Packet` (a type from the same .h) is used in `compression.cpp` → the file cannot be deleted.

**Why so strict:** experience from past failures showed that any "while we're in here" leads to hidden regressions. The goal is a deterministic, safe cleanup.

## ⚠️ DO NOT COMBINE DEAD_CODE_REPORT WITH ASan

`DEAD_CODE_REPORT=ON` + `ENABLE_ASAN=ON` at the same time gives a **crash at game startup** in `FileSetBasePath` (an MSVC-specific conflict between `-ffunction-sections -fdata-sections` and the ASan annotation of merged string literals). Does not reproduce separately.

**Workflow:**
- Dead-code analysis: `CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON" make configure` (ASan OFF)
- Verify build/runtime: `make configure` or `make configure-asan` (DEAD_CODE_REPORT OFF — `make/configure.mk` resets it by default)
- Do NOT enable both flags at once

`make/configure.mk` already protects against accidentally inheriting `DEAD_CODE_REPORT=ON` when switching to ASan via defaults, but on its own it lets an explicit `-DDEAD_CODE_REPORT=ON` in `CMAKE_EXTRA_ARGS` through. So **don't do** `CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON -DENABLE_ASAN=ON" make configure`.

**Goal:** go through the whole project and remove all truly dead
code (functions with no callers, #if 0 blocks, commented-out branches,
unused constants/globals, dead API) that accumulated after the 1:1
port from the pre-release original + through iterative refactoring.

**Why after 1.0, not now:** the 1.0 blocker is hangs, stability,
visual regressions. Dead-code cleanup is cosmetic + maintenance, it
doesn't affect the player. Touching it now risks accidentally breaking
something wired up non-standardly (via callbacks, fn-pointers) for the
sake of a clean diff. Better as a separate wave with full testing.

**Why it matters:** a lot of dead code remains — Stage 2 cut the first
wave, but some "conditionally dead" code (reachable only from #ifdef
branches, debug paths, legacy API without callers) slipped through.
Stage 4 moved it into the new structure, making it worse. The
accumulated junk:
- Makes the code harder to read (it's not obvious what's used and what isn't).
- Produces false positives in automated checks (static analysis,
  coverage).
- Increases build size, even though the linker drops unused code
  (affects debug build + compile time).
- During review it's unclear "did the original leave this, or do we need to clean it too".

## Overview of C++ tools

### Automatic

**1. `cppcheck --enable=unusedFunction`**
- **+:** Lightweight, fast, doesn't require a build system, cross-platform, installs with one command (`apt/brew/choco install cppcheck`).
- **−:** Many false positives — doesn't understand virtual calls, function pointers, callbacks (`GEPreFlipCallback` etc. will show as unused), template instances.
- **Where it's good:** a first survey of the problem's scope, groups of obviously dead functions.

**2. Linker `--gc-sections` + `--print-gc-sections`**
- **+:** The most precise determination of "truly unreachable from the entry point". Requires minimal build-system changes (`-ffunction-sections -fdata-sections` + `-Wl,--gc-sections,--print-gc-sections`).
- **−:** Doesn't see "conditionally dead" code (functions in `#ifdef DEBUG` that aren't enabled now, but could be). On MSVC the syntax is different (`/OPT:REF`), but we use clang+lld. Functions registered via a callback setter as an address may falsely appear dead.
- **Where it's good:** the final deletion list — the linker guarantees these symbols didn't make it into the binary.

**3. Coverage (`llvm-cov` / `gcov`)**
- **+:** Finds **what's truly unreachable in gameplay** — not just syntactically unreachable, but never executed. Gives exact coverage %.
- **−:** A single pass is never fully covering. Rare branches (`if (rare_condition)`) will appear dead without a real trigger. Requires an instrumented build + runtime playthroughs.
- **Where it's good:** additional verification after passes 1-2 — to find branches that are theoretically reachable but players don't hit.

**4. `clang-tidy`**
- **+:** Integrated into clang, works with `compile_commands.json`, has a set of readability/misc checks.
- **−:** Weak for whole functions — tailored for local issues (unused variables, redundant declarations). `misc-unused-using-decls` is useful for include cleanup, but not for dead functions.
- **Where it's good:** a targeted tool for locals / using-declarations / parameters.

**5. `include-what-you-use` (IWYU)**
- **+:** A separate task — cleaning up unused `#include`s. As a side effect catches unused types.
- **−:** Not directly about dead functions. Runs slowly on large projects.
- **Where it's good:** a parallel headers-cleanup task — can be done as a separate iteration from function cleanup.

**6. LTO + dead-code elimination reports**
- **+:** Built into the compiler, gives optimizations as a bonus.
- **−:** Few direct reports about dropped functions — it's an internal optimization.
- **Where it's good:** not as a cleanup tool, but as a side-effect of an optimized build.

### Semi-manual

**7. `ctags` + `grep`**
- **+:** Works everywhere, simple and transparent.
- **−:** For 1000+ symbols it's slow without scripting. Doesn't understand overloads or namespaces at the grep level.
- **Where it's good:** targeted checking of a specific function ("who calls it?").

**8. IDE — clangd / VSCode / CLion "Find All References"**
- **+:** Interactive, precise (based on the semantic index). All references visible instantly.
- **−:** Not mass-automatable — one symbol at a time.
- **Where it's good:** reviewing candidates from passes 1-2 — quickly check each candidate by hand.

### What C++ lacks compared to other languages

- No direct analog of Go's `deadcode` / Rust's `cargo-machete` — which understand the language semantics and give a precise answer. In C++ these get in the way: ODR, templates (instances exist only after use), macros, preprocessor conditions, function pointers.
- Tools like `Infer` (Facebook) exist, but they're mostly about bugs, not dead code.

## Strategy — three passes

### Pass 1: automatic first cut (cppcheck)

`cppcheck --enable=unusedFunction --project=compile_commands.json -j 8`

Lightweight, fast, finds functions with no callers. Many false positives
— doesn't understand virtual calls, function pointers, callbacks
(`GEPreFlipCallback`, `GEModeChangeCallback`, etc.), template instances.

**Output:** a list of candidate functions (likely hundreds). **Don't
delete automatically** — eyeball each one: is it really
unused? Is there a call through a pointer?

Good for **orientation** — to understand the scope, find obvious groups
(for example all `AENG_lock/unlock/flush` at once).

### Pass 2: precise analysis via linker (`--gc-sections`)

Add a `check-dead-code` target to CMake:
```cmake
-ffunction-sections -fdata-sections
-Wl,--gc-sections,--print-gc-sections
```

The linker builds the binary with **only what's truly reachable** from the entry point.
The `--print-gc-sections` output is a list of everything dropped. This is **strict**
dead code — nothing references it from the final .exe.

**Cons / caveats:**
- Doesn't see "conditionally dead" code (functions in #ifdef DEBUG
  branches that aren't enabled now but could be).
- On Windows (MSVC linker) the syntax is different (`/OPT:REF`). We use clang
  + lld, so `--gc-sections` works.
- Functions registered via a callback setter (`ge_set_*_callback`)
  may appear dead if the setter is never called, but the
  function itself is registered as an address in code — check by hand.

### Pass 3: eyes

For each candidate from passes 1-2, confirm by hand:
- Grep for the symbol name in `new_game/src/`.
- Check `original_game/` — there may be context for why the function
  is needed (for example an API for modifications, plugins, or it'll be
  developed further later).
- Check signatures for function-pointer registration.
- If 100% dead — delete.
- If dead but a public API (in `.h` with no callers in new_game) — **don't
  touch without discussion**: it may be part of an external contract.

## Obvious candidates already known

Spotted during work on other stages, can be deleted first:

- `AENG_lock()` / `AENG_unlock()` / `AENG_flush()` — wrappers over
  `ge_lock_screen` / `ge_unlock_screen`, no callers in `new_game/src/`.
  After the Stage 4 cleanup hang-fix the lock/unlock themselves will be deleted.
- `AENG_draw_FPS()` — [aeng.cpp](../new_game/src/engine/graphics/pipeline/aeng.cpp)
  defined, declared in the header, but **nobody calls it**. Currently it's
  superseded by `AENG_draw_messages` and the Shift+F12 overlay via
  `overlay.cpp`, which uses the `cheat==2` path.
- `ge_plot_pixel` / `ge_plot_formatted_pixel` / `ge_get_pixel` /
  `ge_get_formatted_pixel` — after the Stage 2 hang fix they're left without
  callers. Kill them along with `s_dummy_screen`, `ensure_screen_buffer`,
  `s_screen_locked` / `s_screen_w` / `s_screen_h` / `s_screen_pitch`.
- `game_tick.cpp:428` `ge_get_pixel` — a lone debug call, needs to be either
  reworked or removed.

(These are the ones currently on our minds — during the real cleanup many more will surface.)

## Tools and commands

### cppcheck
```bash
# Ubuntu: apt install cppcheck
# Windows: choco install cppcheck / scoop install cppcheck
cppcheck --enable=unusedFunction \
         --project=new_game/build/Debug/compile_commands.json \
         -j 8 \
         2>cppcheck_dead.txt
```

### clang + lld GC sections
```cmake
# In new_game/CMakeLists.txt — a separate target or flag
target_compile_options(OpenChaos PRIVATE -ffunction-sections -fdata-sections)
target_link_options(OpenChaos PRIVATE -Wl,--gc-sections,--print-gc-sections)
```

### Coverage (additional, optional)
```bash
# Build with coverage flags
clang++ -fprofile-instr-generate -fcoverage-mapping ...
# Play, exit
# LLVM_PROFILE_FILE=coverage.profraw ./OpenChaos.exe
llvm-profdata merge -sparse coverage.profraw -o coverage.profdata
llvm-cov report OpenChaos.exe -instr-profile=coverage.profdata
```

Shows not just "reachable", but **actually executed** in
one or a few playthroughs. Useful for the final pass — if
after 1-2 playthroughs a function never fired, it's a candidate for
deletion (with the caveat that rare paths may not have been hit).

## What exactly to delete, and what not to touch

**Delete:**
- Functions/classes/globals with no callers and no registration via a
  function pointer.
- Commented-out code blocks older than N months (review has passed by then).
- `#if 0 / #if false` branches.
- Dead enum members, unused `#define`s.
- Duplicates — the same function in two places, one unused.

**Don't touch:**
- Public APIs (in headers) even if there are no callers — may be
  reserved for plugins/mods/level editor.
- Stub functions with a "TODO" or "not implemented yet" comment
  — that's a deliberate future-slot.
- Standard SDK/library callbacks (`WinMain`, `dllmain`, etc.).
- Assertion helpers and crash handlers (may have no ordinary callers
  but are invoked by the system via signal/exception).

## Final pass — deleting empty files (once, at the end)

After all symbols are cleaned up — go through all project files
and delete those that became effectively empty as a result of the cleanup.

**Criterion for an empty file:**
- `.h`: only the include guard (`#ifndef / #define / #endif`) and possibly one `#include` — no declarations
- `.cpp`: only one `#include` of itself (or nothing at all) — no definitions

**What to do with an empty file:**
1. Find all `#include "file"` across the project → remove those lines
2. Remove the file from `CMakeLists.txt` (if `.cpp`)
3. Delete the file itself (`rm` or `git rm`)

**Command to find candidates:**
```bash
# Files with ≤1 meaningful line (not blank, not comments, not include/guard)
for f in $(find new_game/src -name "*.cpp" -o -name "*.h"); do
  meaningful=$(grep -v "^[[:space:]]*$\|^[[:space:]]*//" "$f" | \
               grep -v "^#ifndef\|^#define [A-Z_]*_H\|^#endif\|^#pragma once\|^#include" | wc -l)
  [ "$meaningful" -le 1 ] && echo "$f"
done
```

This is done **once at the end** of the whole cleanup, not on every iteration.
Example from practice: `thug_globals.h/.cpp` — after deleting `thug_states[]`
the files became empty and were deleted along with their `#include` in `thug.cpp`
and the line in `CMakeLists.txt`.

## Final pass — cleaning up unneeded #includes (once, at the end)

After removing dead code, many `#include`s in the project become unneeded —
clangd flags them as unused. This is normal and expected: the include graph
relied on the deleted symbols.

**When to do it:** only after the entire dead-code cleanup is finished. Don't do it
during iterations — too many moving parts, easy to miss.

**How to do it:**
- Use clangd/clang-tidy `--checks=misc-include-cleaner` or
  `iwyu` (include-what-you-use) as a guide.
- Or go through the files by hand: clangd in the IDE shows unused includes.
- Delete only `#include`s flagged as unused by clangd —
  don't guess "by eye".
- After deleting a group of includes from a file — **build immediately** (includes
  often pull in transitive dependencies, and removing an "unneeded" one may
  silently remove a needed transitive include).

**Scope:** by observation (2026-04-27) — almost every file has at least
one unneeded `#include`. This will be a full separate iteration.

## Stage 13 dead-code readiness criteria

- cppcheck `unusedFunction` output < N (some small number of
  false positives) — the rest dealt with.
- `--gc-sections` drops nothing substantial — the binary
  contains only what's actually used.
- Code coverage after 2-3 playthroughs ≥ some reasonable threshold
  (90%? — to be determined in practice).
- Review via PRs with specific groups (500 files aren't
  reviewed at once, better by subsystem: graphics dead, audio dead, AI
  dead, etc.).
- **Final pass:** all effectively empty files deleted (see section above).

## Approximate scope

Hard to estimate before the first cppcheck pass, but at a glance based on the feel
from Stage 4 — hundreds of functions, dozens of globals, dozens of dead includes.
Main "hot zones":
- `engine/graphics/` — lots of legacy API (`AENG_*`, `POLY_*` layers).
- `things/characters/person.cpp` — a giant file with a pile of debug branches.
- `missions/eway.cpp` — EWAY scripting, lots of pre-release feature stubs.
- `outro/` — often duplicates the engine API, may have outdated signatures.

## Batch 39 (2026-04-27, session 23) — cppcheck unusedFunction pass

The cppcheck `--enable=unusedFunction` pass is complete. All truly dead static functions removed.

**False positives (left in place):**
- `WAND_*`, `get_person_radius`, `which_side` — `extern` declarations inside function bodies, cppcheck doesn't see them
- `ds_set_haptic_volume`, `ds_set_lightbar_setup` — intentional API stubs (comment in the bridge)
- `GEFont`/`Font` typedef — used in core.cpp for the font_list infrastructure

**Restored after erroneous deletion in this same batch:**
- `PUDDLE_create_do` — cppcheck was wrong, the function is called from `PUDDLE_precalculate`
- `COLLIDE_debug_fastnav`, `COLLIDE_find_seethrough_fences`, `COLLIDE_calc_fastnav_bits`,  
  `collide_box_with_line`, `create_shockwave` — deleted in previous batches, have external callers

**Deleted in batch 39:**
- `polypoint.h`: `SetUV(float&, float&)` declaration + out-of-line definition
- `thing.h`: `set_thing_pos` inline
- `outro_always.h`: `qdist3`
- `outro_imp.cpp`: 5 write-path static functions
- `save.cpp`: 6 static dead functions + their forward declarations
- `eng_map.cpp`: 7 MAP_* static draw functions (~600 lines)
- `core.cpp` (GE backend): `destroy_shaders`, `gl_blit_fullscreen_texture`, 11 GE API stubs
- `game_graphics_engine.h`: the corresponding declarations
- `anim_types.h`: `SetCMatrix`, 18 Set*/Get* Anim accessors, GetCharName/SetCharName/GetMultiObject/SetMultiObject Character
- `collide.cpp`: `slide_around_box_lowstack` (~593 lines)
- `collide.h`: the corresponding declaration
- `input_debug.cpp`: `page_name`
- `hook.cpp`: `HOOK_make_loop`
- `inside2.cpp`: `find_stair_in` (~68 lines)
- `puddle.cpp`: `PUDDLE_create` (and `PUDDLE_create_do` erroneously — restored)
- `puddle.h`: `PUDDLE_create` declaration
- `font2d.cpp`: `FONT2D_DrawString_NoTrueType`
- `env.cpp`: `ini_read_int_mem`, `ini_write_string` (~115 lines)
- `sdl3_bridge.cpp/h`: `sdl3_set_mouse_grab`
- `frontend.cpp`: `DriverEnumState` struct + `driver_enum_callback`
- `pyro.cpp/h`: `IHaveToHaveSomePyroSprites`
- `light.h`: `LIGHT_get_colour` inline (~38 lines)
- `night.h`: `NIGHT_get_colour_and_fade` inline (~36 lines)
- `pow.h`: `POW_create` inline + removed includes that became unneeded

**Build:** OK (make build-release, exit 0)
