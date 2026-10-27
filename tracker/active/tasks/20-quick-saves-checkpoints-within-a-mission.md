# 20 — Quick saves (checkpoints) within a mission

- Status: 🔵 open
- Area: gameplay
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Limited in-mission checkpoints so a death doesn't force replaying the whole mission; reset when leaving the mission. Inter-mission saves are untouched. ⚠️ Dead code `MEMORY_quick_save` / `MEMORY_quick_load` (`missions/memory.cpp`) + `X_SAVE_GAME` / `X_LOAD_GAME` in `ui/menus/gamemenu.cpp` already exist — check whether they work and reuse them. Details: [stage13.md §Quick saves within a mission](../../../archive/new_game_planning/stage13.md).
