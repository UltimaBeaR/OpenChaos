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
- **FBO-as-virtual-screen refactor** — game now treats the scene FBO
  as "the screen"; outer pillar/letterbox bars painted by the
  composition layer, game code blind to them. Aspect clamp runs once
  at FBO creation (`OpenDisplay`), not per-frame. Gutted
  `POLY_camera_set` (no `effective_aspect`, no `base_x/y` centring,
  no `fit_scale` — uniform scale, viewport fills FBO). Deleted outer
  bar painting in `AENG_clear_viewport`. Renamed `RealDisplayWidth/
  Height` → `ScreenWidth/Height`, `ui_coords::g_real_*_px` →
  `g_screen_*_px`. Mouse events mapped from window → FBO via
  `composition_window_to_fbo`; events in outer bars dropped. Bonus:
  video (`ge_video_draw_and_swap`) now renders into the scene FBO
  too, gets outer bars from composition automatically. Deviation
  from plan: `push_fullscreen_ui_mode` kept (plan proposed removing
  it, but invariant I5 requires fullscreen effects cover the whole
  FBO, which default uniform affine can't do on non-4:3 FBOs).
  Plan: [`fbo_as_virtual_screen_plan.md`](fbo_as_virtual_screen_plan.md).
- **Cut-scene dialogue split-anchored to top/bottom; 3D letterbox
  tracks the UI bars.** `PANEL_new_widescreen` wraps the top
  bar+portrait+text in `UIModeScope(CENTER_TOP)` and the bottom in
  `UIModeScope(CENTER_BOTTOM)`, pinning the cinematic bars flush with
  the FBO top/bottom edges on any aspect. `wideify` in
  [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  `POLY_camera_set` is scaled by `g_frame_scale / (ScreenHeight/480)`
  so the 3D viewport letterbox matches the UI-bar height exactly on
  non-4:3 FBOs (stays 80 virtual on 4:3-or-wider). No visible black
  gap between UI bar and 3D scene. Virtual sizes (640 frame, text
  wrap 600, 80-tall bar, portrait anchors) untouched — layout is
  tuned to those. Invariant to preserve: UI bar scale and 3D letterbox
  scale must track each other.
- **Tutorial panel split-scope framing** — `PANEL_last`'s
  `EWAY_tutorial_string` path now runs the `PANEL_darken_screen(640)`
  backdrop inside `FullscreenUIModeScope` (covers the whole FBO) and
  the speech bubble + text inside `UIModeScope(CENTER_CENTER)` (stays
  framed, no stretch). Mirrors the `GAMEMENU_draw` pattern. Adds a
  ~1-second input grace at the start of the dialogue so a held
  jump/punch/select button doesn't dismiss it instantly — see
  [GAMEPLAY_CHANGES.md](../../GAMEPLAY_CHANGES.md) "Туториал".
- **HUD bottom-row anchors.** Radar / health / weapon / ammo / crime
  rate / beacons / grenade / panel mission timer
  (`PANEL_draw_buffered`) / weapon-switch popup (`PANEL_inventory`) are
  wrapped in `UIModeScope(LEFT_BOTTOM)` — flush with FBO bottom-left
  on any aspect. Speech / radio / on-screen-message stack (the
  `PLT_X/PLT_Y` block) is wrapped in `UIModeScope(CENTER_BOTTOM)` —
  centres between the radar and the mirrored right-side empty gap with
  equal rubber padding on both sides (algebraic identity: msg centre
  and gap centre both equal `ScreenW/2 + 91 × g_frame_scale`). On 4:3
  pixel-identical to the original; on portrait the full layout scales
  uniformly by `g_frame_scale = ScreenW/640`; on widescreen radar /
  empty stay original-sized at the corners, msg slides to the gap
  centre. See `PANEL_inventory` / `PANEL_draw_buffered` / `PANEL_last`
  in [`panel.cpp`](../../new_game/src/ui/hud/panel.cpp). Debug flags
  `OC_DEBUG_COMPOSITION_BARS_RED` and `OC_DEBUG_SOUND_DISABLED` added
  in [`config.h`](../../new_game/src/config.h) to aid layout / mute
  debugging (ternary runtime check, not `#if` — preprocessor treats
  C++ `true` / `false` as zero).

Remaining work — see [`issues.md`](issues.md):
- **Other in-game HUD elements still unframed** — top-of-screen mission
  countdown (`PANEL_draw_timer` at virtual (320, 50) from `eway.cpp`),
  road-sign flashes, search progress bar, PSX / version debug
  overlays. Same `PolyPage::push_ui_mode` infra — just call sites left
  to wrap (mostly `CENTER_TOP`).
- **Runtime window resize (post-1.0)** — see the dedicated TODO
  section at the top of [`issues.md`](issues.md).
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
  `ScreenWidth` vs `POLY_screen_width`, what `s_XScale/s_YScale` does,
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
