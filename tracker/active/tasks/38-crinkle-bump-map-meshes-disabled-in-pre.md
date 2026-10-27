# 38 — Crinkle (bump-map meshes) disabled in pre-release

- Status: 🔵 open
- Area: render
- Tags: bug, prerelease
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
File: `mesh.cpp`. `return NULL` — crinkle (bumpmap mesh) is fully disabled in the pre-release, in the final PC it works. This is **different** from the already-fixed "bump-mapping flickers" (the 0x20 timer fix, see archive/new_game_planning/prerelease_fixes.md). A visual feature, its absence is unnoticeable unless comparing with retail side-by-side.

Pre-release stub/commented-out code in MuckyFoot's sources; **in the final release this feature works.**
