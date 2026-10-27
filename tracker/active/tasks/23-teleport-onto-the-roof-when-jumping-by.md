# 23 — Teleport onto the roof when jumping by a building wall

- Status: ❄️ deferred
- Area: gameplay
- Tags: bug, retail-bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
A known retail bug (Steam discussions). **Reproduction steps:** (1) walk right up facing a wall, press "action" (C) — the protagonist presses her back against the wall; (2) press "kick" (X) + "back" — the back kick slightly shifts the protagonist into the geometry; (3) turn back with the arrow keys to face the building — the character is standing inside the wall; (4) press "action" (C) — the protagonist again "presses against the wall", moves even further; (5) press "back" — teleport onto the roof. **Probable cause:** during the back kick from the wall-lean position, the collision doesn't block the shift into the wall. Then the ground-height correction check finds the roof. **The bug is widely known among players (many use it intentionally).** **⚠️ Important:** if we fix it — **be sure to provide support for this trick as a cheat**, since players are used to it.

Old-list status: *Known (retail bug)*.
