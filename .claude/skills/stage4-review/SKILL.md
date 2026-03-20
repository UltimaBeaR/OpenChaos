---
name: stage4-review
description: >
  Stage 4-specific review extension for OpenChaos migration — runs after ANY changes during Stage 4.
  This skill EXTENDS the general `review` skill — always run `review` first for general checks
  (dangling if/else, diff gathering, code correctness), then run this for Stage 4-specific
  checks (uc_orig, globals, entity mapping, include guards, CMake, comments, DAG order).
  Trigger after ANY code changes during Stage 4 work: after completing a migration iteration,
  after fixes or tweaks requested by the user, after any edits to code in new/ or old/.
  This is NOT just a pre-commit step — the user commits manually and may not commit at all.
  Review is mandatory after every piece of completed work.
  If you are doing Stage 4 work, ALWAYS use both `review` AND `stage4-review` — using only one is incomplete.
---

# Stage 4 — Migration-Specific Review Checks

**Prerequisite:** Run the general `review` skill first (git diff, dangling if/else, general correctness).
This skill adds Stage 4-specific checks A–H on top.

**When to run:** After ANY code changes during Stage 4 — after completing a migration iteration,
after fixes requested by the user, after any edits. Not just before commits. The user commits
manually and may not commit at all — review is still mandatory every time.

Go through EVERY check below for EVERY changed/new file. No shortcuts.

---

## CHECK A: `uc_orig` + Entity Mapping — On EVERY Entity

This is the check that gets skipped most often. Do not skip it.

### Rule (duplicated here because you WILL forget):

Every moved entity gets `// uc_orig: OriginalName (fallen/path/file.ext)` on a **separate line before it**.

Entity = function, struct, class, typedef, variable, #define macro. ALL of them. No exceptions.
Including: static helpers, internal macros, trivial definitions, obvious things. EVERYTHING.

### The comment goes on EVERY occurrence:
- **Declaration in `.h`** — needs `uc_orig`
- **Definition in `.cpp`** — needs `uc_orig`
- **Forward declaration** — needs `uc_orig`

**A definition in `.cpp` without `uc_orig` above it is a BUG.** Fix it before proceeding.

### How to check:
1. Open each new/changed file in `new/`
2. Go through EVERY LINE that defines or declares a named entity
3. Verify `uc_orig` comment is present with correct name and path
4. Verify the entity was added to entity mapping: `python tools/entity_map.py find --name NAME`
5. If `find` returns nothing — the entity is missing from the mapping. Add it.

### Common mistakes:
- uc_orig on the `.h` declaration but missing on the `.cpp` definition
- uc_orig on the function definition but missing on its forward declaration
- Macro definitions (#define) without uc_orig — they need it too
- Static helper functions without uc_orig — they need it too
- Variables defined inside `_globals.cpp` without uc_orig — they need it too

**Why this matters:** In future stages (renderer, AI, effects) you need to quickly find the
original code analog from the current line. Especially for features under `#ifdef` that were
removed — they only exist in `original_game/`.

---

## CHECK B: Global Variables — ONLY in `*_globals` Files

### Rule (duplicated here because you WILL forget):

**EVERY global variable → `_globals.cpp` (definition) + `_globals.h` (extern declaration). NO EXCEPTIONS.**

This applies even if:
- The variable was `static` in the original — **still goes to `_globals`**
- The variable is only used inside one `.cpp` file — **still goes to `_globals`**
- The variable looks like a private implementation detail — **still goes to `_globals`**
- The variable is a simple counter or flag — **still goes to `_globals`**
- You think "it makes more sense next to the function that uses it" — **NO. `_globals`.**

### How to check:
1. Search every new `.cpp` file (NOT `_globals.cpp`) for global variable definitions
2. Any variable defined at file scope (outside of functions) in a non-`_globals` file is a **BUG**
3. Check: are there any `extern` declarations in `.h` files that aren't `_globals.h`? That's also a **BUG**

The rule exists so that ALL global state is instantly visible by looking at `_globals` files.
"But it's only used locally" is not a valid reason to put it elsewhere.

---

## CHECK C: Code Correctness (Stage 4-specific)

- Logic is 1:1 with the original — not changed, not simplified, not "improved"
- Function signatures are unchanged
- Entity names are unchanged (except conflicts with `--conflict`)

---

## CHECK D: Include Guards

Format: `PATH_FROM_NEW_UPPERCASED_H`

Examples:
- `new/core/types.h` → `CORE_TYPES_H`
- `new/engine/input/keyboard.h` → `ENGINE_INPUT_KEYBOARD_H`

Check: does the guard match the actual file path from `new/`?

---

## CHECK E: Comments

- All comments in **English**
- Original comments **deleted** (they remain in `old/` and `original_game/`)
- No `// claude-ai:` prefixes
- Comments explain "what" and "why" for non-obvious code. Obvious code is not commented.

---

## CHECK F: CMakeLists.txt

- New `.cpp` files added to `SOURCES_NEW`
- Old `.cpp` files from `old/` removed (if fully migrated)

---

## CHECK G: Empty Files in `old/`

- If everything was moved out of a file — the file is deleted (or replaced with a redirect-include)

---

## CHECK H: Dependency Order

- Files in `new/` do NOT depend on a layer above them in the DAG:
  ```
  core → io → graphics_api → graphics → engine/* → world/actors → ai → missions/ui
  ```
- Temporary `#include` to `old/` is acceptable, marked with `// Temporary:` comment

---

## After All Checks Pass

Only after confirming that BOTH the general `review` checks AND all Stage 4 checks A–H pass
for **every file**, tell the user:

"Самопроверка пройдена: [N] файлов проверено, общие проверки + A–H ок."

If any check fails — fix the issue first. Do not suggest committing with known issues.
