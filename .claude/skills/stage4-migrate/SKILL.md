---
name: stage4-migrate
description: >
  Stage 4 migration iteration for OpenChaos — moving code from src/old/ to src/new/.
  Use this skill whenever the user asks to do a migration iteration, continue porting,
  move files/functions to new structure, or says things like "давай итерацию",
  "продолжаем перенос", "следующий батч", "мигрируй", "перенеси".
  Also trigger when discussing what to migrate next, migration order, or DAG dependencies.
---

# Stage 4 — Migration Iteration Workflow

You are performing a code restructuring of Urban Chaos (1999) original codebase.
Code lives in `new_game/src/old/` (original, being emptied) and `new_game/src/new/` (new structure, being filled).
Your job: move entities from `old/` to `new/` preserving logic 1:1, with proper tracking.

## How It Works

Two sibling directories under `src/`:
```
new_game/src/
├── old/    ← all current code moved here (fallen/ + MFStdLib/)
└── new/    ← new structure, .h/.cpp with include guards
```

Entities (functions, classes, types, variables) are moved from `old/` into files in `new/`.
`old/` includes new headers from `new/` instead of the old ones — always compilable state.
Entry point (`Main.cpp`) stays in `old/` until the very end.

**C++23 modules — not yet.** Plain `.h/.cpp` with include guards. Module conversion is a separate
step after the structure is complete — only then will there be a meaningful DAG to convert.

Both include directories are in CMakeLists.txt:
- `src/new/` — so `old/` can write `#include "actors/core/thing.h"`
- `src/old/` — so `new/` can include not-yet-migrated headers via `#include "fallen/Headers/Map.h"` (temporary, goes away as migration progresses)

## Before Starting

1. Read `new_game_planning/stage4_rules.md` for the redirect — it points here.
2. Check `new_game_devlog/stage4_log.md` for the last iteration number and what was done.
3. Check current state: what's already in `src/new/`, what's left in `src/old/`.

## Iteration Size

~1200-1500 lines total per iteration (all files in the batch combined).
Pick thematically related files. For files >2000 lines — split into ~1500 line chunks.
Use subagents for parallel file creation on large batches.

## Migration Order (bottom-up by DAG)

Each layer depends only on already-migrated layers:

```
1.  core/                    — no dependencies
2.  engine/io/               — depends on core
3.  engine/graphics/         — depends on core, io
4.  engine/animation/        — depends on core, graphics
5.  engine/lighting/         — depends on core, graphics
6.  engine/effects/          — depends on core, graphics
7.  engine/physics/          — depends on core
8.  engine/audio/            — depends on core, io
9.  engine/input/            — depends on core
10. assets/                  — depends on engine/io/
11. world/                   — depends on engine/*, assets
12. actors/                  — depends on engine/*, world, assets
13. effects/ (game level)    — depends on engine/effects/, actors
14. ai/                      — depends on actors, world
15. missions/                — depends on actors, world, ai, assets
16. ui/                      — depends on actors, engine/graphics
```

Breaking order = depending on something not yet migrated. If that happens: either migrate the dependency first, or temporarily keep it in `old/` with a `// Temporary:` include.

## Per-File Migration Flow

1. Create `new/path/file.h` + `new/path/file.cpp` with entities and `uc_orig` comments
2. Delete migrated entities from the `old/` file
3. Add `#include "path/file.h"` in the corresponding `old/` header
4. Compile (`make build-release`) — must pass
5. Add entries via `python tools/entity_map.py add`

## What's Allowed

- Move entities from `old/` to `new/`
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
- Name as in `original_game/` (not `old/`)
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
4. Unclear case → leave in `old/`, export via `extern`, describe the situation, ask

---

# Include Guards

Format: `PATH_FROM_NEW_UPPERCASED_H` — reflects file path from `new/` root.

```cpp
// File: new/core/types.h
#ifndef CORE_TYPES_H
#define CORE_TYPES_H
...
#endif // CORE_TYPES_H

// File: new/engine/graphics/pipeline/bucket.h
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
| `function` | Function moved from `old/` to `new/` | path in `new/` | 4 |
| `struct` | Struct moved | path in `new/` | 4 |
| `class` | Class moved | path in `new/` | 4 |
| `variable` | Global variable moved | path in `new/` | 4 |
| `type` | Typedef/type alias moved | path in `new/` | 4 |
| `macro` | #define macro moved | path in `new/` | 4 |

The `orig_file` field is **always a path in `original_game/`**, not in `old/` and not in `new/`.
Simple rule: the path must exist in `original_game/`, not in `new_game/`.

### Workflow: before each `add` — always `find` first

```
python tools/entity_map.py find --name ENTITY_NAME
```

- **No results** → no entry exists. Use the path in `original_game/` as `--orig-file`.
- **Has `kind: file` entry (stage 2)** → file in `old/` came from a different path in `original_game/`.
  Use `orig_file` from that entry as `--orig-file` for new entity entries.
  Example: `anim.h` came from `fallen/Editor/Headers/Anim.h` → all entities from it
  get `--orig-file "fallen/Editor/Headers/Anim.h"`, not `fallen/Headers/anim.h`.
- **Has entity entry (stage 4)** → already migrated. Don't add a duplicate.

### JSON format

```json
{
  "entries": [
    {
      "name": "Thing_alloc",
      "file": "new/actors/core/thing.h",
      "orig_name": "Thing_alloc",
      "orig_file": "fallen/Source/Thing.cpp",
      "kind": "function",
      "conflict": false,
      "stage": 4,
      "date": "2026-03-20"
    }
  ]
}
```

### Commands
```
python tools/entity_map.py add    --name NAME --file FILE --orig-name ORIG --orig-file ORIG_FILE --kind KIND [--conflict] [--stage N]
python tools/entity_map.py find   --name NAME            # search by name and orig_name (substring)
python tools/entity_map.py list   [--kind KIND] [--stage N]
python tools/entity_map.py stats
python tools/entity_map.py rename --name OLD --new-name NEW   # for future renames
```

---

# Target Structure of `new/`

```
new/
├── core/                      — math, memory, utilities. No dependencies.
│
├── engine/                    — game engine (doesn't know about Urban Chaos)
│   ├── graphics/              — graphics engine (all DirectX — only here)
│   │   ├── graphics_api/      — D3D device, DDManager, GDisplay, GWorkScreen
│   │   ├── pipeline/          — facets, buckets, pages, poly submission
│   │   ├── geometry/          — mesh, shape, poly, cone, oval
│   │   └── resources/         — D3DTexture, fonts, sprites (D3D-specific resources)
│   ├── io/                    — raw file I/O: Drive, AsyncFile. Only open/read files,
│   │                            no knowledge of formats. Separated from parsing.
│   ├── animation/             — figure system, vertex morphing (tween)
│   ├── lighting/              — gamut, crinkle (bump mapping), shadow, light
│   ├── effects/               — effects infrastructure: PSys, billboard rendering
│   ├── physics/               — collision detection, movement (tech debt: response code here too)
│   ├── audio/                 — MSS32 wrapper + sound management
│   └── input/                 — DirectInput, keyboard, mouse
│
├── world/                     — UC game world
│   ├── map/                   — map, supermap, qmap, road, water
│   ├── environment/           — building, terrain, doors, elevators, env
│   └── navigation/            — walkable data, inside detection, nav mesh
│
├── actors/                    — Thing system (UC game objects)
│   ├── core/                  — Thing allocation, base types, drawtype
│   ├── characters/            — Person, Player, Cop, Darci, Thug, Roper...
│   ├── vehicles/              — Vehicle, Chopper
│   ├── items/                 — Barrel, Balloon, Grenade, Guns...
│   └── animals/               — Animal, Canid, Bat
│
├── assets/                    — UC-specific loaders and format parsers:
│                                FileClump (.gob/.ilf), Tga, BinkClient, level loading,
│                                texture atlases, animations, .ucm missions.
│                                Everything that knows about specific UC binary formats — here.
│
├── effects/                   — UC-specific effects: fire, pyro, spark, ribbon, mist
│                                (UC logic — not engine; rendering via engine/effects/)
│
├── ai/                        — NPC behavior, state machines, UC combat
│
├── missions/                  — EWAY engine + UC mission content
│
└── ui/                        — UC interface
    ├── frontend/
    ├── hud/
    ├── menus/
    └── cutscenes/
```

**About `outro/`:** Contains the last game level and specific mechanics. Common mechanics
(e.g. file loading) go to `engine/io/` or appropriate engine module. UC-specific content →
wherever it fits by meaning (`world/`, `missions/`, etc.).

## Placement Criteria

- **`engine/`** = "HOW" — algorithm, mechanism, service. Low-level as possible. Doesn't know about UC characters/items. Reusable in another game.
- **`engine/graphics/`** = "HOW to draw geometry and textures." Knows about vertices, polygons, textures. Doesn't know what a "cop" or "fire" is.
- **`engine/graphics/graphics_api/`** = Thinnest wrapper over DirectX. When replacing DirectX with OpenGL/Vulkan → **only this layer** changes.
- **`engine/io/`** = Raw file I/O: open, read bytes, buffering. Doesn't know about file formats. If a function just reads bytes from disk → it's here.
- **`assets/`** = UC-specific format parsers: knows about .gob, .ilf, .ucm, animation file structure. Uses `engine/io/` for reading, only parses format structure. Everything where code knows about a specific UC binary format — only here.
- **Everything else** (actors, world, ai, missions, ui, effects) = "WHAT does X do in Urban Chaos." Implements mechanics by combining engine functions.

Effects boundary example:
- `engine/effects/` — PSys infrastructure, billboard rendering → **engine**
- `effects/` (game) — fire.cpp, pyro.cpp, specific UC fire → **game**

Dependency order (only downward):
```
core → io → graphics_api → graphics → engine/* → world/actors → ai → missions/ui
```
A dependency from outside `engine/` directly into `engine/graphics/` is a visible architecture violation.

---

# Comments

- **Language: English.** All comments in `new/` code — English only.
- Delete all original comments — they remain in `old/` and `original_game/` as reference.
- Write for a **regular developer** seeing the code for the first time. Not cryptic KB jargon.
- Comment explains "what it does" and "why" if non-obvious. Don't comment obvious code.
- No `// claude-ai:` prefixes — in `new/` all comments are written by Claude anyway.

---

# Iteration Rules

1. **Iteration size — 1200-1500 lines:**
   Each iteration = ~1200-1500 lines total (all files in batch). Pick thematically related files,
   combining small and medium ones. For files >2000 lines — split into ~1500 line chunks.
   Use subagents for parallel file creation on large batches.

2. **Compilation** — Claude compiles independently (`make build-release`) after each iteration.
   Don't bother the user — just report the result.

3. **Smoke test** — user runs the game every 10-20 iterations. Claude does not ask more often.

4. **Self-review after ANY changes** — run `review` + `stage4-review` skills. After every iteration,
   after every fix, after every edit. Not just before commits — always.

5. **Do NOT start the next iteration** without user's command.

6. **Log** in `new_game_devlog/stage4_log.md` — **maximally compact**:
   - Header: iteration number + short name (which modules)
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

- **Includes in `old/` are non-linear** — before moving any entity, grep who uses it. Don't guess by filename.
- **Dangling if/else** — when deleting a line, check it's not the sole body of an `if` without braces. If it is, delete the `if` too or add `{}` / `;`.
- **`/Zp1`** (struct member alignment) — don't change, critical for binary resource formats.
- **`-fno-inline-functions`** — don't remove until the body parts rendering bug is figured out.
- **Empty files in `old/`** — when everything is moved out, delete the file.
- **Don't create empty directories** — create only when the first file goes in.

---

# KB Usage During Migration

Don't load KB proactively — for most migrations it's not needed.

- Migration is clear → work without KB
- Need to understand what a subsystem does → load only the specific `subsystems/X.md`
- DENSE_SUMMARY during migration → don't load (only for architectural questions)

---

# Future (after migration is complete)

- **C++23 modules** — analyze DAG on the new fine-grained structure, then convert
- **Namespaces** — introduce together with modules. All `new/` under `namespace oc` — discuss separately
- **Globals** — consider replacing global state with explicit dependency passing
