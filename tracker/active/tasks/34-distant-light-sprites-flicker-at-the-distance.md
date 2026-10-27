# 34 — Distant light sprites flicker at the distance-cull boundary (RTA)

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Repro (RTA):** exit the police station onto the street — you can see how **distant lights** appear and disappear by a strange principle. Illustrative case: at the **distance-cull boundary** from the camera — if you look **straight ahead** at a light (a light sprite), it **disappears**; turn the camera a little **to the side** — it **appears**. If you come closer — everything is fine (the lights are stable). So the bug is exactly at the distance cull boundary of the lights — it seems the light culling algorithm depends not only on distance but also on the view direction / position in the frame, and at the boundary gives a false cull.

Old-list status: *Investigate*.
