# 49 — Fire/smoke order flickers from near-batching (left for performance)

- Status: ❄️ deferred
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
For fire/bonfires close to the camera, when the camera moves, **flickering** is noticeable — the fire and smoke swap draw order for a couple of frames every few steps. Cause: for the sake of FPS, the fire/smoke has **near-batching by depth layers** enabled — sorting is quantized into coarse layers, this cuts draw calls (near a burning car 332 micro-calls → ~20-30, GPU ≈ −2× on a scene with fire) and **actually solves the FPS drop**. Without batching the fire is drawn correctly but eats FPS. **The solution was deliberately left** — FPS is more important than this artifact. Improvement options: **(1) clustering by source** — all sprites of one fire source (bonfire, burning car, etc.) sorted as a single cluster at the depth of the source itself, not each sprite at its own depth. The source depth is world-anchored → moves smoothly with the camera → no jumps at layer boundaries → no flicker. To account for during implementation: both flame and smoke must be in the cluster. Smoke is particles (`PARTICLE_Draw`), the bonfire flame is immediate sprites; if you cluster only the flame, the smoke particles are sorted separately and creep over the fire. So a particle must know the world position of its source (remember it at spawn), and the sorting depth of the particle is computed from it. Within the cluster the fire should be drawn over the smoke. **(2) Atlas** — all the fire and all the smoke into one texture atlas → everything can be batched in one chunk, the very problem of "2 batches, smoke over fire or vice versa" disappears. **(3)** others are possible. **Where the current system sits in the code:** all in `poly.cpp` — the `s_sprite_layers` table, the `poly_alpha_key` function, the sort comparator inside `POLY_frame_draw`. The system is written generically (for any class of sprites), but apart from fire/smoke it's not used anywhere and probably won't be — if a better solution is found, it can be removed entirely.

Old-list status: *Return to after 1.0*.
