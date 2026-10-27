# 22 — NPCs on the club dance floor can't find the exit (Gatecrasher)

- Status: ❄️ deferred
- Area: gameplay
- Tags: bug, retail-bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Gatecrasher — when shooting/a fight starts on the club dance floor, civilians try to run away but don't see the nearby stairs. They bump into walls and get stuck, trying to jump up. There are 2 staircases nearby — they don't see either one. One leads to the exit, the other doesn't (maybe they shouldn't use it). The staircase to the exit doesn't look like a normal staircase — more like a series of low platforms, possibly the pathfinding doesn't recognize it as walkable. At the same time, civilians NOT on the dance floor run away normally through the main entrance. Apparently the pathfinding doesn't see the stairs as walkable or doesn't know how to walk on them. Possibly related to the disabled INSIDE2 navigation. **Verified against retail 2026-04-14: reproduces identically** — an original retail bug, not our regression.

Old-list status: *Known (retail bug)*.
