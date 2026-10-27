# 25 — Z-fighting of leaves and newspapers on the ground / in puddles

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Leaves (POLY_PAGE_LEAF) and newspapers (POLY_PAGE_RUBBISH) z-fight each other on the ground — slight flickering. **In puddles the problem is significantly worse:** leaves and newspapers completely disappear and reappear when rotating the camera (not flickering, but complete disappearance). Probably the same depth (lying on the same plane as the puddle/ground). Possible solution: a small depth bias when adding, or stable_sort that takes the object type into account.

Old-list status: *Investigate*.
