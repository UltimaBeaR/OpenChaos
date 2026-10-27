# 86 — N (model cycler) doesn't show all character variants

- Status: 🔵 open
- Area: devtools
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
The debug key **N** in `bangunsnotgames` mode cycles Darci's model through all types and the combinations of `(MeshID, PersonID)` visible in the code. The current table in [`game_tick.cpp`](../../../new_game/src/game/game_tick.cpp) (`VARIANTS[]`) is built from an enumeration of spawn sites in the code (`alloc_person`, `pcom.cpp` boredom branch). Covered: 15 PersonTypes + mesh variants for CIV (7/8/9 × PersonID 6-9/0/0) and THUG_RASTA (0/1/2). **Not covered:** in the game you can see at least **2 colors of female civilians (PERSON_CIV — not SLAG)** that the N-cycle doesn't show. Where they spawn from — I couldn't find via grep over the code, also checked exhaustive enumeration of MeshID 7/8/9 × PersonID 0..9 (30 combinations) — they still didn't show up. Most likely the spawn branch sits somewhere in a hidden path (an EWAY event with an explicit PersonID, non-obvious code in combat/spawn, or per-level data). Bring the N-cycle to a full enumeration of all the character kinds that actually appear in the game.

Old-list status: *Post-1.0*.
