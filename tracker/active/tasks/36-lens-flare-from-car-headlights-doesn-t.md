# 36 — Lens flare from car headlights doesn't trigger at all angles

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
After the 2026-05-03 fix ("Lens flare on Darci / glow rotation / reflection bbox — coord-space mismatch", in resolved) the effect became visible when the headlights hit the camera through Darci's body — but **sometimes it doesn't turn on at certain angles** (light in the frame, angle correct, but no flash). Looks glitchy. **Possible causes:** (1) the LOS check `there_is_a_los(..., LOS_FLAG_IGNORE_PRIMS)` in [`bloom.cpp:46`](../../../new_game/src/engine/graphics/postprocess/bloom.cpp#L46) may falsely cull the flash due to wall-collision at edge angles; (2) the dot-product gate `if ((dot < 0) || (opts & BLOOM_GLOW_ALWAYS))` ([`bloom.cpp:194`](../../../new_game/src/engine/graphics/postprocess/bloom.cpp#L194)) — if the headlight and camera directions are almost perpendicular, dot ≈ 0 and falls into "don't show"; (3) the screen-rect cull `if (pt1.X < 0 || pt1.X > POLY_screen_width...)` ([`bloom.cpp:54`](../../../new_game/src/engine/graphics/postprocess/bloom.cpp#L54)). **Repro:** first mission, drive in reverse / stand next to the car aiming the camera, slowly rotate. Since we've fixed the main effect, let's bring it to a smooth trigger.

Old-list status: *Investigate*.
