# 30 — Light from mines shines through walls (Target UC)

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Target UC** — mines behind crates with a ladder, the red blinking light is drawn over the crate walls. These effects (`BLOOM_draw` glow spheres + lens flare) are **post-process**: in [`poly_render.cpp:916`](../../../new_game/src/engine/graphics/pipeline/poly_render.cpp#L916) `POLY_PAGE_LENSFLARE` goes with `SetDepthEnabled(false) / SetDepthWrite(false) + additive blending (Src=One, Dst=One)`, and the glow via `SPRITE_draw_rotated` → `POLY_PAGE_BLOOM1` (also without depth). They are **intentionally drawn over all geometry whatsoever** — this is a working by-design lens-bloom effect (as in the original). So "the bulb / light is drawn over a foreground object" is **normal**, not a bug. **The bug — specifically "through a solid wall"**: it should have been culled by `there_is_a_los(...)` ([`bloom.cpp:46`/`170`](../../../new_game/src/engine/graphics/postprocess/bloom.cpp#L46)) — a line-of-sight test from the camera to the source. If LOS returns true when there's a wall — dig into `there_is_a_los` (the LOS_FLAG_IGNORE_PRIMS flag tells it to ignore prims, maybe it also ignores the crate walls). **Check the retail PC version** — if it's the same there, it's an original bug.

Old-list status: *Investigate (verify)*.
