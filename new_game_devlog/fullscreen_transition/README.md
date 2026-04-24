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
- **FOV cap + pillarbox for ultra-wide aspects** — scene clamped at
  `OC_FOV_CAP_ASPECT` (default 16:9), remaining framebuffer columns
  painted black. Avoids rectilinear fish-eye at 21:9+. Controlled
  from [`config.h`](../../new_game/src/config.h).
- **Tall-aspect auto zoom-out + letterbox floor** — windows narrower
  than 4:3 no longer pixel-blow-up the character. In
  `[OC_FOV_MIN_ASPECT, 4:3]` (default MIN = 4:3) the camera zooms
  out so horizontal FOV stays 4:3-equivalent and the extra
  vertical space fills with sky / ground. Below MIN the scene is
  letterboxed into the MIN aspect with top/bottom bars — symmetric
  mirror of the pillarbox cap. Auto-zoom is applied at the
  `AENG_lens` source so both render and frustum culling see the
  same widened FOV (no black holes on the periphery).
- **Runtime FOV multiplier (`OC_FOV_MULTIPLIER`)** — divides the
  camera lens by the user's multiplier. Default 1.0 = no change;
  higher = wider, lower = narrower. Standard FPS-style slider; does
  not alter the projection type (fish-eye returns if cranked high).
- **Environment vs characters coord-space consistency on pillarbox** —
  `PolyPage::s_XOffset` / `s_YOffset` shift POLY-path vertex output
  by `dwX`/`dwY` so it matches the MM-path (figure.cpp characters)
  which bakes those offsets into its bone matrices. Without this
  fix, pillar/letterbox active frames rendered environment offset
  left while characters stayed centred ("different cameras").
- **Sprite / rain / line widths now aspect-independent _and_
  track auto-zoom / FOV multiplier** — world-sized billboards
  (light-source glows, moon, rain drops, bullet trails, crosshair
  rings, sphere circles) go through `POLY_sprite_scale`, a file-
  scope float recomputed per frame in `POLY_camera_set` as
  `(DisplayWidth/2/ZCLIP) / (OC_FOV_MULTIPLIER × auto_zoom)`.
  Pinned at the 4:3 design-time baseline (no aspect dependency)
  and compensated for the two zoom adjustments we add on top of
  the game's own lens (so sprites shrink in proportion with the
  shrinking scene). Does NOT include the game's own
  `POLY_cam_lens` — cutscene zooms never changed sprite sizes in
  the original and shouldn't now.
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
- **Outro framed** — final outro (credits + 3D wireframe sequence)
  renders into the same 4:3 framed region. Two-level viewport (shader
  vs GL), per-frame clear to kill bar-transition trails, `CLAMP_TO_EDGE`
  on outro textures to fix bilinear-edge wrap. See "Outro framing" in
  [`ui_coords_plan.md`](ui_coords_plan.md).
- **Videos framed** — intro Bink videos (Eidos, logo24, PCINTRO) all
  aspect-fit into the framed 4:3 region with appropriate letterbox /
  pillarbox depending on the source aspect. `ge_video_draw_and_swap`
  snapshots/restores GL scissor so a parent UI scope doesn't crop the
  video.
- **Moon billboard aspect / FOV behaviour** — bounds check uses
  `POLY_screen_width` / `POLY_screen_height` (was `DisplayWidth` /
  `DisplayHeight`) so it no longer pops out early on the right edge
  when yawing past it on widescreen; radius compensated by
  `POLY_our_zoom` so widened FOV multiplier / auto-zoom doesn't blow
  up its size relative to world objects. See "Moon rendering —
  resolved" in [`issues.md`](issues.md).
- **Menu darken overlay full-screen** — pause/won/lost dim layer now
  covers the entire real framebuffer (pillarbox + letterbox bars
  included), not just the 4:3 framed region. Menu text stays framed.
  Animation rate preserved at original ~640 ms.
- **Stars rendering** — two fixes:
  real-pixel mapping uses `PolyPage::s_XScale/s_XOffset` (was
  `RealDisplayWidth/DisplayWidth`, broken post-Hor+ — stars "flew like
  flies"); star-pixel size scales with resolution
  (`max(1, s_YScale)` — 1 px at 4:3 640×480, ≈2 at 1080p, ≈4 at 4K)
  so they stay visible on high-DPI displays.
- **UI text clipping on narrow / portrait windows** —
  `POLY_screen_clip_right = max(POLY_screen_width, DisplayWidth)`
  so UI draws (virtual 640-wide) aren't rejected as offscreen when
  `OC_FOV_MIN_ASPECT` clamps the 3D scene below 4:3 (e.g. 320-wide
  scene on 480×1920 window). Symptom was "GAME PAUSED" rendered as
  "GAME P". See "UI text truncated" in [`issues.md`](issues.md) for
  the clip-flag diagnostic approach.
- **`s_work_screen_buf` 640×480 legacy buffer deleted** — audit
  confirmed no consumers in `new_game/src`. Removed the 1.2 MB buffer,
  all `WorkScreen*` / `WorkWindow*` / `CurrentPalette` globals, four
  stub functions, and the `FLAGS_USE_WORKSCREEN` macro.

Remaining work — see [`issues.md`](issues.md):
- **🔴 FBO-as-virtual-screen refactor** — architectural cleanup that
  makes the game treat the scene FBO as "the screen" and moves all
  pillarbox/letterbox bar painting into the composition layer. Eats
  a whole cluster of smaller todos: removes per-effect bar-workaround
  code (fade-out, menu darken, stars, moon), renames
  `RealDisplayWidth/Height` → `ScreenWidth/Height`, deletes
  `push_fullscreen_ui_mode`. **Full handoff:**
  [`fbo_as_virtual_screen_plan.md`](fbo_as_virtual_screen_plan.md).
- **In-game HUD** still pillarboxed in the framed centre — needs
  per-element anchors (radar = `RIGHT_TOP`, ammo = `LEFT_BOTTOM` etc.).
  Same `PolyPage::push_ui_mode` infra, just call sites left to wrap.
  This is Stage 3c in the plan.
- **NIGHT lighting pool overflow at wide/tall FOV (post-1.0).**
  `UBYTE`-indexed 256-slot pool overflows when the view gamut exceeds
  255 lo-res map squares (triggered by wide `OC_FOV_MULTIPLIER` or
  narrow/portrait aspects). Fix is a UWORD refactor with preserved
  save-file compatibility. **Workaround for 1.0: no user-facing FOV
  slider** — `OC_FOV_MULTIPLIER` stays a compile-time constant.
- Split UI rendering from the scaled scene FBO (part of the UI rework).
- Replace the stand-in simplified FXAA with canonical FXAA 3.11 or
  SMAA 1x.
- Wibble amplitude doesn't scale with resolution — effect too subtle at 1080p+.
- Focus callback cursor show/hide breaks linker (low priority, defensive code).

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
- **[`fbo_as_virtual_screen_plan.md`](fbo_as_virtual_screen_plan.md)** —
  full handoff for the next architectural refactor: make the game see
  the scene FBO as "the screen", move pillar/letterbox bar painting to
  the composition layer, rename `RealDisplayWidth/Height` → `Screen*`,
  delete the per-effect bar-workaround code. 1-2 days of focused work;
  eats a whole cluster of todos in one pass.

## Workflow

1. Pick an issue from [`issues.md`](issues.md).
2. Read [`concepts.md`](concepts.md) if any terminology there is unfamiliar.
3. Apply fix. Never edit files without user consent — propose first.
4. Test per [`testing.md`](testing.md). Verify the fix at multiple aspects,
   not just one.
5. Move the entry from "Known issues" → "Resolved" in [`issues.md`](issues.md)
   with a short note on what was done.
