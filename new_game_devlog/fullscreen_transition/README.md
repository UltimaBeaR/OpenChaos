# Fullscreen / arbitrary resolution transition

Working notes for moving the game off its hardcoded 640×480 4:3 viewport
to arbitrary resolution + fullscreen support. Targets Stage 12 / Release 1.0.

## Status at a glance

Already done:
- Fullscreen window creation (native monitor resolution, borderless-desktop).
- 3D projection is aspect-aware (Hor+ FOV, widescreen = wider horizontal field).
- Uniform vertical scaling for the rendered world.
- Config knobs in [`new_game/src/config.h`](../../new_game/src/config.h):
  `OC_FULLSCREEN`, `OC_WINDOWED_WIDTH/HEIGHT`, `OC_VSYNC`,
  `OC_RENDER_SCALE`, `OC_AA_ENABLE`.
- Water wibble post-process repositioned correctly on non-4:3 resolutions.
- OS cursor hidden in fullscreen.
- **Native physical pixels across platforms** — `OC_WINDOWED_WIDTH/
  HEIGHT` always mean physical pixels (Retina / HiDPI-aware via
  two-pass `SDL_GetWindowPixelDensity` resize).
- **Render scale + scene FBO + composition pass** — the game renders
  into an offscreen FBO sized `physical × OC_RENDER_SCALE`; a
  composition pass (simplified FXAA + bilinear upscale) presents it
  to the window. MSAA removed. See "Resolved" in
  [`issues.md`](issues.md) for file-by-file details and the invariants
  future work needs to preserve.
- **Framed UI coord system** — main menu, options, save/load, briefing,
  pause, score, loading screen and frontend transitions render in a
  centered 4:3 region with black bars on widescreen. Backed by an
  affine transform in `PolyPage::AddFan` + hardware scissor, opt-in
  per UI scope via `PolyPage::UIModeScope`. Background images
  (`ge_show_back_image`) and transitions (`ge_blit_surface_to_backbuffer`)
  always framed. Kibble particles auto-clip via scissor. See
  [`ui_coords_plan.md`](ui_coords_plan.md) for design + diff map.

Remaining work — see [`issues.md`](issues.md):
- **In-game HUD** still pillarboxed in the framed centre — needs
  per-element anchors (radar = `RIGHT_TOP`, ammo = `LEFT_BOTTOM` etc.).
  Same `PolyPage::push_ui_mode` infra, just call sites left to wrap.
  This is Stage 3c in the plan.
- Split UI rendering from the scaled scene FBO (part of the UI rework).
- Rename `RealDisplayWidth/Height` (it now means scene-FBO size, not
  physical display — misleading).
- Replace the stand-in simplified FXAA with canonical FXAA 3.11 or
  SMAA 1x.
- `s_work_screen_buf` hardcoded to 640×480 bytes, needs audit.
- Wibble amplitude doesn't scale with resolution — effect too subtle at 1080p+.
- Focus callback cursor show/hide breaks linker (low priority, defensive code).
- Outro / verification pass.
- Cutscene video #3 is cropped vertically on widescreen (videos #1, #2 fine).

## Files in this folder

- **[`concepts.md`](concepts.md)** — terminology and pipeline flow. **Read this
  first** if you're new to the code: explains `DisplayWidth` vs
  `RealDisplayWidth` vs `POLY_screen_width`, what `s_XScale/s_YScale` does,
  and how a vertex travels from virtual game coords to framebuffer pixels.
- **[`testing.md`](testing.md)** — how to reproduce bugs. Critical: most bugs
  only show up on **non-4:3** resolutions. Testing only at 4:3 lies.
- **[`issues.md`](issues.md)** — detailed breakdown of each known issue, with
  symptoms, root cause, entry-point files, and proposed fix approaches.
- **[`audit.md`](audit.md)** — exhaustive inventory of every place in
  `new_game/src/` where resolution or aspect ratio influences behavior.
  Verdict: works / broken / to-verify. Use as reference when touching
  render/UI/input code.
- **[`ui_coords_plan.md`](ui_coords_plan.md)** — execution plan for the
  HUD / UI coordinate-system rework (framed coords + anchored HUD, fixes
  the pillarbox issue). Read this before doing any UI scaling work; it's
  the agreed design + stage-by-stage to-do.

## Workflow

1. Pick an issue from [`issues.md`](issues.md).
2. Read [`concepts.md`](concepts.md) if any terminology there is unfamiliar.
3. Apply fix. Never edit files without user consent — propose first.
4. Test per [`testing.md`](testing.md). Verify the fix at multiple aspects,
   not just one.
5. Move the entry from "Known issues" → "Resolved" in [`issues.md`](issues.md)
   with a short note on what was done.
