# 52 — Hanging on a wall — the near plane clips geometry

- Status: ❄️ deferred
- Area: camera
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Darci hangs on a ledge; the camera snaps onto her (min_d_hit from the wall probe < VC_WALL_OFFSET — the wall is flush against the focus), the near-clip plane clips the wall geometry nearby → the map backdrop is visible through the hole. **Fix attempts didn't work:** (1) `VC_MIN_FOCUS_DIST` (keep the camera at 0.5m instead of a snap) — the focus→camera direction often goes INTO the wall, the camera is in the material, the near-plane clips even harder; (2) flip-opposite (camera to the side opposite the wall) — gives an auto-rotation, violates "manual rotation only". Current state — the original snap-to-focus (the lesser evil). **Needed:** determine the free side of the wall WITHOUT auto-rotation — widen the probe (wall normal / multiple hits), or via `FC_calc_focus` STATE_DANGLING focus_yaw, or special handling of DANGLING/CLIMBING as a scripted override.

Old-list status: *Deferred*.
