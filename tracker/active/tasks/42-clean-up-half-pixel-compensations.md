# 42 — Clean up the +0.5 / -0.5 pixel compensations (TL-vertex paths)

- Status: 🔵 open
- Area: render
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
In the original D3D6 code there were no `0.5`s at all. When porting to OpenGL, "1-pixel gap on the right" artifacts appeared on fullscreen rectangles (pause menu backdrops, debug panels, etc.) + UV-bleeding of textures in atlases — a consequence of different pixel-coordinate conventions (D3D6: integer = pixel center; OpenGL: integer = boundary). It was closed with pointwise `+0.5`s in several places (`PolyPoint2D::SetSC`, `font_atlas`, `sky` stars, `gl_blit_textured_quad`, `ge_blit_to_framed_area`) + one `-0.5` in the `tl_vert.glsl` shader. Every time the bug showed up in a new place — yet another pointwise compensation was added. Currently the MM-path of characters and the outro 3D wireframe are WITHOUT compensation (a half-pixel shift relative to the world, invisible to the user). Architecturally it's a zoo: easy to forget in new code, easy to break by changing one place. A possible universal fix: remove the shader `-0.5` in sync with removing all manual `+0.5`s (mathematically identical to the current state), or: keep the current scheme but document the mandatory `+0.5` for new direct-TL code. Detailed analysis → [`../new_game_devlog/pixel_half_offset_plan.md`](../../../archive/new_game_devlog/pixel_half_offset_plan.md). **Don't touch while ad-hoc** — too easy to break everything. Do it as a separate iteration with full inventory and tests.

Old-list status: *Investigate (separate iteration)*.
