# 7 — Crash in debug mode on [ / ] (camera-2 cycle)

- Status: 🔵 open
- Area: crash
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
In `bangunsnotgames` mode, pressing `[` / `]` (camera-2 follow prev/next through characters, enables splitscreen) — ASSERT `WITHIN(NIGHT_square_free, 1, NIGHT_MAX_SQUARES - 1)` in [`night.cpp:1215`](../../../new_game/src/engine/graphics/lighting/night.cpp#L1215). The camera-2 splitscreen duplicates NIGHT source-square allocations, the pool is exhausted. Only in debug mode.

Old-list status: *Post-1.0*.

## Important notes
- Same pool, second trigger (from the archive sweep, 2026-10-27): the NIGHT lighting pool also overflows at a wide FOV
  or on portrait screens. A fix plan exists there: widen the index from UBYTE to UWORD (the pool is still UBYTE/256 in
  code), mind save compatibility, and gate the FOV slider until then. Source:
  [fullscreen_transition/issues.md](../../../archive/new_game_devlog/fullscreen_transition/issues.md) L239-329.
