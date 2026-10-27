# 53 — Small mouse rotation gives a shift, not a turn

- Status: ❄️ deferred
- Area: camera
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
A very small mouse movement is perceived as a small SHIFT of the camera, not a turn; at medium/strong rotation it's unnoticeable. Probably a loss of precision in int-math at small angles: `ORBIT_PT` with an angle of 1-2 game-units → SIN/COS ≈ 0/1, the quotient `(rx*c - rz*s) >> 16` with a small s loses precision → a micro-shift from rounding. **Probably the same class as "Jitter of objects close to the camera (camera jitter in interp)" in the Gameplay section** — a common source in the precision of the render/camera on small deltas. Where to look: `ORBIT_PT` in the mdx block of `FC_process`, `apply_pitch_y_delta`.

Old-list status: *Deferred*.
