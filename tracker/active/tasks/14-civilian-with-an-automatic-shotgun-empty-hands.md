# 14 — Civilian with an automatic/shotgun: empty hands + effects at a fixed point

- Status: 🔵 open
- Area: gameplay
- Tags: bug, retail-bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
For a PERSON_CIV with a **pistol** everything is fine, but with an **automatic/shotgun** — **empty hands** (the weapon model isn't drawn) and the smoke/shot effects appear at a **fixed world coordinate** (not from the barrel); the effects at the hit point are drawn. The cop/hooker/others — all weapons with effects are fine. **Cause:** `set_persons_personid` ([person.cpp](../../../new_game/src/things/characters/person.cpp)) for CIV does an early `return` (historically a civ doesn't carry a held weapon), so the "weapon bit" (`PersonID>>5`) isn't set for the civilian → in `figure.cpp` neither the held model nor the muzzle flash is drawn, and `GunMuzzle` (the smoke/sparks position) isn't updated and stays with the old/zero value. **Fix direction:** for CIV determine the weapon from `SpecialUse` (not from `PersonID>>5`) and compute `GunMuzzle` from the hand bone — analogous to what was done for the roper (see resolved "Roper: shot effects…"). Repro via `bangunsnotgames` + F1+N (morph into a civilian). A rare case, doesn't block.

Old-list status: *Investigate*.
