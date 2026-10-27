# 5 — ASSERT when pulling up onto a crate

- Status: 🔵 open
- Area: crash
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Repro:** the player pulls up onto a crate → ASSERT(0) in `person.cpp` `SUB_STATE_PULL_UP` handler (lines 9481 / 9489 — `if (face == GRAB_FLOOR)` ASSERT, or the `else` branch when `face == 0`). After the pull-up animation, `find_face_for_this_pos(WorldPos.X>>8, WorldPos.Y>>8, WorldPos.Z>>8, &new_y, ...)` doesn't find a face under the player → assert. Repeats on similar (but different) crates on the same level; the trigger is a class of places/geometry. Probably an original MuckyFoot bug: with certain crate geometry the coordinate after pull-up lands outside any face. Fix: either widen the search (find the nearest face by Y), or a graceful fallback (set_person_idle + keep the current Y). Couldn't reproduce it after recording — happens rarely.

Old-list status: *Investigate*.
