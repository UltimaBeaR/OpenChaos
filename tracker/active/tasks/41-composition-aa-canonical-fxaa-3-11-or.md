# 41 — Composition AA: canonical FXAA 3.11 or SMAA 1x

- Status: 🔵 open
- Area: render
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
The current [`composite_frag.glsl`](../../../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/composite_frag.glsl) is a trimmed FXAA without the canonical 3.11 edge-span search. Essentially a cheap blur on pixels with contrast, not real AA. Two upgrade paths: (1) **Canonical FXAA 3.11** (~2-3 h) — public `Fxaa3_11.h` from NVIDIA, a single file, no LUT, a drop-in replacement of the composite fragment shader. Still blur-based, better than ours but FXAA always softens text. (2) **SMAA 1x** (~3-4 h) — `github.com/iryoku/smaa`. Three passes (edge detection → blending weights → neighborhood blending), two extra FBOs, two precomputed lookup textures (AreaTex ~180 KB, SearchTex ~1 KB) embedded into the binary. Noticeably better than FXAA, especially on text and thin details. The scene-FBO infra already exists, only an extra-pass runner is needed in the composition module.

Old-list status: *Implement*.
