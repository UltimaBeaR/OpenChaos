# 60 — Steam Deck detection → Deck button glyphs

- Status: 🔵 open
- Area: input
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
`active_input_device` knows only KEYBOARD_MOUSE / XBOX / DUALSENSE; the `deck` glyph atlas is already embedded but never selected (`atlas_for_active_device()` in `input_glyphs.cpp` has no Deck branch). Detect a Steam Deck and add the branch. Details: [stage13_gamepad.md §Steam Deck detection](../../../archive/new_game_planning/stage13_gamepad.md).
