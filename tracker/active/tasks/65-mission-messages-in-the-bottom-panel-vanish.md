# 65 — Mission messages in the bottom panel vanish too fast at mission completion

- Status: 🔵 open
- Area: ui
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Repro (RTA — arresting a thug completes the mission):** on arrest a message pops up in the bottom oval plate (the same `PANEL_new_text` / `PANEL_text[]` system as NPC lines), but holds for **~0.5 sec** and disappears — almost immediately the complete-mission trigger fires and ~2 sec later "mission complete" pops up. The player doesn't have time to read it. There may be a similar "too slow" case in the 1st mission with the car (if you don't make it in time), but **the user isn't sure such a message even exists** — need to check. **This is NOT a regression of the font refactor:** `PANEL_new_text` after the refactor (extracting `PANEL_store_text` + the `alt_font` flag) **byte-for-byte repeats** the original logic; the timings (`delay`), fade-out (`PANEL_new_text_process`) and queue clearing **weren't touched.** These are **the resource messages themselves** (they go without `alt_font`, on the game font). **Probable cause (long-standing):** at mission completion / state change the `PANEL_text[]` queue is reset (`PANEL_new_text_init` / scene transition) before the player had time to read; or a short `delay` in the script is interrupted by the next event. **How to confirm it's long-standing:** check on the `last_working_d3d_build` branch (before the font changes) — it should behave the same way. **Fix direction:** at complete-mission show the important message longer / don't clear the queue instantly, or delay the complete-trigger for the reading time.

Old-list status: *Investigate (compare with last_working_d3d_build)*.
