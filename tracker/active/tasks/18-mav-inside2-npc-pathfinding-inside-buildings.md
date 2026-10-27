# 18 — MAV INSIDE2: NPC pathfinding inside buildings

- Status: 🔵 open
- Area: gameplay
- Tags: feature, prerelease
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
File: `inside2.cpp`. `INSIDE2_mav_inside() → ASSERT(0)` — a stub in the pre-release. **NPCs can't path-find inside buildings.** Noticeable in indoor missions (police station, club) — related to the old bug about NPCs on the club dance floor (also post-1.0). A big job: the missing navigation system needs to be implemented. A known limitation.

Pre-release stub/commented-out code in MuckyFoot's sources; **in the final release this feature works.**
