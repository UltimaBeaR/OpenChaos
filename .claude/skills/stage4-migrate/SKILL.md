---
name: stage4-migrate
description: >
  Stage 4 migration iteration for OpenChaos — moving code from legacy headers
  (src/fallen/, src/MFStdLib/) into the new structure (src/).
  Use this skill whenever the user asks to do a migration iteration, continue porting,
  move files/functions to new structure, or says things like "давай итерацию",
  "продолжаем перенос", "следующий батч", "мигрируй", "перенеси".
  Also trigger when discussing what to migrate next, migration order, or DAG dependencies.
---

# Stage 4 — Migration Iteration Workflow

You are performing a code restructuring of Urban Chaos (1999) original codebase.

## Current State (Stage 4.0, final phase)

The bulk migration is done. `.cpp` files are already in the new structure under `src/`.
What remains: **legacy headers** in `src/fallen/` and `src/MFStdLib/` that still contain
types, structs, macros, constants, and function declarations. ~200 files, ~10K lines, ~290KB.

These headers are referenced via `// Temporary:` includes (~684 occurrences in ~141 files).

```
new_game/src/
├── core/              ← new structure (already populated)
├── engine/            ← new structure (already populated)
├── world/             ← new structure (already populated)
├── actors/            ← new structure (already populated)
├── assets/            ← ...
├── effects/           ← ...
├── ai/                ← ...
├── missions/          ← ...
├── ui/                ← ...
├── fallen/            ← LEGACY headers to eliminate (Headers/, DDEngine/, DDLibrary/, Source/, outro/)
└── MFStdLib/          ← LEGACY headers to eliminate (Headers/)
```

**Goal:** move every entity from `fallen/` and `MFStdLib/` into proper headers in the new
structure, update all `// Temporary:` includes to point to the new locations, delete the
legacy directories. After this, `src/` contains only the new structure.

**C++23 modules — not yet.** Plain `.h/.cpp` with include guards. Module conversion is a separate
step after the structure is complete — only then will there be a meaningful DAG to convert.

## Before Starting

1. Check `new_game_devlog/stage4_log.md` for the last iteration number and what was done.
2. Check current state: what's left in `src/fallen/` and `src/MFStdLib/`.

## Iteration Size

**~2000-3000 lines per iteration** (larger than before — this is mostly declarations, not logic).
Pick thematically related headers. Use subagents for parallel work on large batches.

## Migration Order (bottom-up by DAG)

Move headers in the same dependency order as before:

```
1.  MFStdLib/           → core/          — base types, math, file stubs
2.  fallen/DDLibrary/   → engine/io/, engine/graphics/graphics_api/, engine/graphics/resources/
3.  fallen/DDEngine/    → engine/graphics/, engine/animation/, engine/lighting/
4.  fallen/Headers/     → world/, actors/, ai/, effects/, missions/, ui/, assets/
5.  fallen/outro/       → ui/cutscenes/outro/
```

Breaking order = depending on something not yet migrated. If that happens: keep the
`// Temporary:` include until the dependency is migrated.

## Per-Header Migration Flow

1. **Read the legacy header** — identify all entities (types, structs, macros, constants,
   function declarations, extern variables)
2. **For each entity — decide where it goes:**
   - Already exists in a new header (just needs the declaration added)? → add it there
   - Belongs to an existing module that has no header for it yet? → create new header
   - Use the placement criteria below to decide
3. **Move entities** into the appropriate new header(s) with `uc_orig` comments
4. **Update consumers** — find all files that had `#include "fallen/Headers/Foo.h" // Temporary:`
   and replace with `#include "proper/path/foo.h"`. Remove the `// Temporary:` comment.
   - **grep before replacing** — don't assume which files include what
   - Some consumers may need multiple new includes to replace one old include
5. **Delete the legacy header** when empty (all entities moved out)
6. **Compile** (`make build-release`) — must pass
7. **Add entity_map entries** via `python tools/entity_map.py add`

### Important: entities may already be migrated

Many entities from these headers were already moved during the main migration (they exist in
new headers with `uc_orig` comments). In that case:
- The legacy header just has a leftover declaration that's now redundant
- Delete it from the legacy header
- The `// Temporary:` include in the consumer may already be unnecessary — check and remove
- Do NOT add a duplicate entity_map entry

### Important: some legacy headers are "umbrella" includes

Headers like `Game.h`, `MFStdLib.h` pull in dozens of other headers. When migrating these:
- Don't try to create one replacement — that defeats the purpose
- Each consumer needs only the specific new headers it actually uses
- Replace the umbrella include with the specific includes each consumer needs

## What's Allowed

- Move entities from legacy headers to new structure headers
- Delete entities that are already present in new headers (duplicates/redirects)
- Delete unused entities and obvious dead code (don't migrate them)
- Delete ALL original comments and write new ones (English, for a developer seeing the code for the first time)
- Add `// uc_orig:` comments (mandatory on every entity — see below)

## What's Forbidden

- Changing code logic
- Renaming entities (sole exception: name conflicts, see below)
- Changing function signatures
- Merging or splitting functions

## Name Conflicts

When an entity with internal linkage (static / anonymous namespace in .cpp) goes public and conflicts:

```
Format:  <filename_without_ext>_<original_name>

Examples:
  thing.cpp:    static void helper()  →  thing_helper()
  vehicle.cpp:  static void helper()  →  vehicle_helper()
```

Only on actual conflict, not preventively. Record in entity mapping with `--conflict`.

## Borderline Entities

If an entity mixes multiple areas and it's unclear where it belongs — put it where it fits best
by feel. This is tech debt; mark with a comment nearby explaining the placement decision.

---

# uc_orig Comments — MANDATORY on EVERY Entity

This is the most important tracking rule. Every moved entity gets a comment right before it.
Entity = any named definition: function, class, struct, type, variable, #define macro.

## The comment goes on EVERY occurrence:
- Declaration in `.h`
- Definition in `.cpp`
- Forward declaration in `.cpp`

A definition in `.cpp` without `uc_orig` is a bug. A forward declaration without `uc_orig` is also a bug.

```cpp
// === file.h ===
// uc_orig: Thing_alloc (fallen/Source/Thing.cpp)
void Thing_alloc(int type);

// === file.cpp ===
// Forward declaration also needs uc_orig!
// uc_orig: Helper (fallen/Source/Thing.cpp)
static void Helper(int x);

// uc_orig: Thing_alloc (fallen/Source/Thing.cpp)
void Thing_alloc(int type) { ... }

// uc_orig: Helper (fallen/Source/Thing.cpp)
static void Helper(int x) { ... }
```

More examples:
```cpp
// uc_orig: THING (fallen/Headers/Thing.h)
struct Thing { ... };

// uc_orig: g_thing_count (fallen/Source/Thing.cpp)
int g_thing_count = 0;
```

**Format:** `// uc_orig: OriginalName (fallen/path/to/file.ext)`
- Name as in `original_game/` (not in `new_game/`)
- Path from original game root (`fallen/...`)
- On a separate line before declaration/definition

**NO EXCEPTIONS. EVERYTHING. ABSOLUTELY EVERYTHING.**

Every occurrence of every entity moved from the original gets a `uc_orig` comment AND an entry
in entity mapping. Declaration, definition, forward declaration, macro, variable, struct, typedef,
static, extern, inline, helper, internal, trivial, obvious — **EVERYTHING**. There is no category
of "this doesn't need marking". If a line of code came from the original and contains a named
entity — it gets `uc_orig` and a mapping entry. Period.

**Why:** When working on the renderer/AI/effects in future stages — quickly find the analog in
`original_game/` right from the current line. Especially important for features under `#ifdef`
that were removed — they only exist in the original.

---

# Global Variables — ALL Go to `_globals` Files

## THIS RULE HAS NO EXCEPTIONS.

Every global variable goes to `_globals.cpp` (definition) + `_globals.h` (extern declaration).

**It does not matter:**
- whether the variable was `static` in the original — **still goes to _globals**
- whether it's private or public — **still goes to _globals**
- whether it's declared in a header or not — **still goes to _globals**
- whether it's only used inside one file — **still goes to _globals**
- whether it seems like an implementation detail — **still goes to _globals**

**ANY global variable → `_globals.cpp` + `_globals.h`. Period.**

In `_globals.cpp` the variable is NOT marked `static` — even if it was `static` in the original.
This doesn't change program behavior (nobody touches it externally) but ensures a uniform rule:
**all global state lives in `_globals` files**.

**NO global variables in regular `.cpp` files. None. Zero.**

### Initialization order:
1. Trivial initializer (`= 0`, `= nullptr`, literal) → move freely
2. Non-trivial initializer (function call, constructor with side effects) → move with dependencies, preserve original order
3. Interdependent globals → same file, original order
4. Unclear case → describe the situation, ask

---

# Include Guards

Format: `PATH_FROM_SRC_UPPERCASED_H` — reflects file path from `src/` root.

```cpp
// File: src/core/types.h
#ifndef CORE_TYPES_H
#define CORE_TYPES_H
...
#endif // CORE_TYPES_H

// File: src/engine/graphics/pipeline/bucket.h
#ifndef ENGINE_GRAPHICS_PIPELINE_BUCKET_H
#define ENGINE_GRAPHICS_PIPELINE_BUCKET_H
...
#endif // ENGINE_GRAPHICS_PIPELINE_BUCKET_H
```

---

# Entity Mapping

**File:** `new_game_planning/entity_mapping.json`
**Script:** `tools/entity_map.py`

> **LEGACY:** `new_game_planning/entity_mapping.md` — old file from Stage 2 (human-readable, not used as a tool).
> Data from it has already been transferred to JSON (stage 2 entries). `.md` is kept as a historical document.

### Entry types (--kind)

| kind | Meaning | `file` field | `stage` |
|------|---------|-------------|---------|
| `file` | File move: file in `old/` came from a different path in `original_game/` | path in `old/` | 2 |
| `function` | Function moved | path in new structure | 4 |
| `struct` | Struct moved | path in new structure | 4 |
| `class` | Class moved | path in new structure | 4 |
| `variable` | Global variable moved | path in new structure | 4 |
| `type` | Typedef/type alias moved | path in new structure | 4 |
| `macro` | #define macro moved | path in new structure | 4 |

The `orig_file` field is **always a path in `original_game/`**, not in `new_game/`.
Simple rule: the path must exist in `original_game/`, not in `new_game/`.

### Workflow: before each `add` — always `find` first

```
python tools/entity_map.py find --name ENTITY_NAME
```

- **No results** → no entry exists. Use the path in `original_game/` as `--orig-file`.
- **Has `kind: file` entry (stage 2)** → file came from a different path in `original_game/`.
  Use `orig_file` from that entry as `--orig-file` for new entity entries.
  Example: `anim.h` came from `fallen/Editor/Headers/Anim.h` → all entities from it
  get `--orig-file "fallen/Editor/Headers/Anim.h"`, not `fallen/Headers/anim.h`.
- **Has entity entry (stage 4)** → already migrated. Don't add a duplicate.

### Commands
```
python tools/entity_map.py add    --name NAME --file FILE --orig-name ORIG --orig-file ORIG_FILE --kind KIND [--conflict] [--stage N]
python tools/entity_map.py find   --name NAME            # search by name and orig_name (substring)
python tools/entity_map.py list   [--kind KIND] [--stage N]
python tools/entity_map.py stats
python tools/entity_map.py rename --name OLD --new-name NEW   # for future renames
```

---

# Target Structure of `src/`

See `new_game/README.md` for the full annotated directory tree.

```
src/
├── engine/                    — game engine (UC-agnostic, reusable)
│   ├── core/                  — types, math, memory, heap, timer
│   ├── platform/              — uc_common.h (legacy umbrella), host, wind_procs
│   ├── graphics/
│   │   ├── graphics_api/      — D3D device, DDManager, vertex_buffer, render_state, d3d_texture
│   │   ├── pipeline/          — aeng, bucket, poly, polypage, draw2d, qeng
│   │   ├── geometry/          — mesh, facet, superfacet, farfacet, fastprim, figure, cone, oval, shape, sprite, sky, aa
│   │   ├── lighting/          — gamut, night, shadow, smap, crinkle
│   │   ├── postprocess/       — bloom, wibble
│   │   └── text/              — font, font2d, menufont, truetype, text
│   ├── console/               — debug console + on-screen messages
│   ├── io/                    — file, async_file, env
│   ├── audio/                 — mfx, music, sound, soundenv
│   ├── input/                 — keyboard, mouse, joystick
│   ├── animation/             — morph, anim_types
│   ├── effects/               — psystem, flamengine
│   ├── physics/               — collide, hm, sm
│   └── compression/           — delta compression, S3 block compression
│
├── game/                      — game loop, startup, input mapping, game state
├── things/                    — Thing system (core, characters, vehicles, items, animals)
├── map/                       — heightfield map, roads, supermap, sewers, ob
├── buildings/                 — building geometry, prim, stair, outline, id, ware
├── navigation/                — walkable, inside2, wmove, wand
├── world_objects/             — door, plat, tripwire, puddle, dirt
├── effects/                   — UC effects (weather/, combat/, environment/)
├── camera/                    — follow camera, modes
├── combat/                    — melee combat system
├── shooting/                  — guns (auto-aim), projectile pool
├── ai/                        — pathfinding (mav), NPC AI (pcom)
├── missions/                  — EWAY scripting, playcuts, save, memory
├── assets/                    — UC resources (formats/, texture, sound_id, xlat_str)
├── ui/                        — UI (frontend/, hud/, menus/)
├── outro/                     — end credits (core/ engine + content)
└── main.cpp
```

## Placement Criteria

- **`engine/`** = "HOW" — algorithm, mechanism, service. Doesn't know about UC characters/items. Reusable in another game.
- **`engine/graphics/`** = "HOW to draw geometry and textures." Knows about vertices, polygons, textures. Doesn't know what a "cop" or "fire" is.
- **`engine/graphics/graphics_api/`** = Thinnest wrapper over DirectX. When replacing DirectX with OpenGL/Vulkan → **only this layer** changes.
- **`engine/io/`** = Raw file I/O: open, read bytes, buffering. Doesn't know about file formats.
- **`assets/formats/`** = UC-specific format parsers: .gob, .ilf, .ucm, animation files. Uses `engine/io/` for reading.
- **Everything else** (things, map, buildings, ai, missions, ui, effects, combat, shooting, camera, etc.) = "WHAT does X do in Urban Chaos." Implements mechanics by combining engine functions.

Effects boundary:
- `engine/effects/` — PSys infrastructure, flame texture generator → **engine**
- `effects/` (game) — fire, pyro, spark, fog — specific UC effects → **game**

Dependency order (only downward):
```
engine/core → engine/io → engine/graphics/graphics_api → engine/graphics → engine/* → game-layer modules
```

---

# Comments

- **Language: English.** All comments in code — English only.
- Delete all original comments — they remain in `original_game/` as reference.
- Write for a **regular developer** seeing the code for the first time. Not cryptic KB jargon.
- Comment explains "what it does" and "why" if non-obvious. Don't comment obvious code.
- No `// claude-ai:` prefixes.

---

# Iteration Rules

1. **Iteration size — 2000-3000 lines:**
   Each iteration = ~2000-3000 lines total (all files in batch). This phase is mostly
   declarations/types, so larger batches are safe. Pick thematically related headers.
   Use subagents for parallel work on large batches.

2. **Compilation** — Claude compiles independently (`make build-release`) after each iteration.
   Don't bother the user — just report the result.

3. **Smoke test** — user runs the game periodically. Claude does not ask for smoke tests.

4. **Self-review after ANY changes** — run `review` + `stage4-review` skills. After every iteration,
   after every fix, after every edit. Not just before commits — always.

5. **Do NOT start the next iteration** without user's command.

6. **Log** in `new_game_devlog/stage4_log.md` — **maximally compact**:
   - Header: iteration number + short name (which modules/headers)
   - Body: **only notes** — non-obvious decisions, found bugs, name conflicts, deleted entities with reason
   - **DO NOT write (RE-READ BEFORE EVERY LOG ENTRY):**
     - List of migrated files/entities — visible from code and mapping
     - Compilation status — assumed OK by default
     - What was deleted from where — visible from diff
     - File sizes, line counts, table sizes — visible from code
     - Any facts derivable from looking at code, diff, or mapping — don't duplicate in log
   - **Test:** before writing, ask yourself: "is this visible from code/diff/mapping?" If yes — don't write.
   - If nothing interesting — one line: `Iterations 5–9: routine`

---

# After Migration: Compile and Self-Review

1. **Compile:** `make build-release` — must pass.
2. **Self-review:** Run the `review` skill (general) + `stage4-review` skill (Stage 4 checks A–H).
   This happens after EVERY completed piece of work — after finishing an iteration, after fixes
   requested by the user, after any edits. Not just before commits. The user commits manually
   and may not commit at all — review is still mandatory.
3. **Do NOT start next iteration** without user's command.

---

# Warnings

- **Grep before replacing includes** — don't assume which files include what. A header may be
  included from unexpected places.
- **Umbrella headers** (Game.h, MFStdLib.h) — don't create a 1:1 replacement. Each consumer
  gets only the specific new headers it needs.
- **Dangling if/else** — when deleting a line, check it's not the sole body of an `if` without braces.
- **`/Zp1`** (struct member alignment) — don't change, critical for binary resource formats.
- **`-fno-inline-functions`** — don't remove until the body parts rendering bug is figured out.
- **Empty headers** — when everything is moved out, delete the file.
- **Don't create empty directories** — create only when the first file goes in.
- **Already-migrated entities** — many entities in legacy headers already exist in new headers.
  Check before creating duplicates. Use `entity_map.py find` or grep.
- **Transitive include breakage after batch replace** — when replacing a legacy header with a
  narrower new one, some consumers may have relied on transitive includes from the legacy header.
  Example: `balloon.cpp` got `Thing`/`ASSERT` transitively through `animate.h → game.h`; after
  replacing `animate.h` with `anim_ids.h` (no such chain) — it broke.
  Fix pattern: for each broken file, open the original in `original_game/` and compare includes —
  whatever was there explicitly and is now missing, add it (as `// Temporary:` if still legacy).

---

# KB Usage During Migration

Don't load KB proactively — for most migrations it's not needed.

- Migration is clear → work without KB
- Need to understand what a subsystem does → load only the specific `subsystems/X.md`
- DENSE_SUMMARY during migration → don't load (only for architectural questions)

---

# Future (after migration is complete)

- **C++23 modules** — analyze DAG on the new fine-grained structure, then convert
- **Namespaces** — introduce together with modules. All under `namespace oc` — discuss separately
- **Globals** — consider replacing global state with explicit dependency passing
