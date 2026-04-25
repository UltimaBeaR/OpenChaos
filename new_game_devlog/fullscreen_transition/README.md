# Fullscreen / arbitrary resolution transition

Working notes for moving the game off its hardcoded 640×480 4:3 viewport
to arbitrary resolution + fullscreen support. Targets Stage 12 / Release 1.0.

## Status: ✅ COMPLETED (2026-04-25, with known caveats)

Workstream is **functionally done** — game runs at arbitrary aspect /
resolution, fullscreen, with proper FOV handling, framed UI, post-
composition UI pass, sharp text, no 1-pixel gaps in fullscreen overlays
on the SetSC/POLY-path side, etc. See full "Already done" list below.

**Known caveats (not blocking 1.0, won't be addressed inside this
workstream):**

- Some general-purpose render / composition issues that surfaced during
  this work but aren't fullscreen-specific have been **moved to the
  global issue list** at
  [`../../new_game_planning/known_issues_and_bugs.md`](../../new_game_planning/known_issues_and_bugs.md):
  - **Wibble amplitude doesn't scale with resolution** — water ripple
    effect ослаблен на 1080p+.
  - **Composition AA** — упрощённый FXAA, заменить на canonical FXAA
    3.11 / SMAA 1x.
  - **CRT / scanline shader** — стилистическая идея (post-process в
    composition layer).
  - **Зоопарк `+0.5` / `-0.5` пиксельных компенсаций** в TL-vertex
    путях (D3D6 → OpenGL convention mismatch). Архитектурная задача
    отдельной итерацией. Подробный план →
    [`../pixel_half_offset_plan.md`](../pixel_half_offset_plan.md).

- **Stragglers внутри workstream'а** (низкий приоритет, можно делать
  если найдётся время):
  - PSX / version debug overlays unframed (`PANEL_last` ~ строки
    2494-2540, virtual `(5, 15)` и `(20, 20)`) — debug-only, нужен
    `LEFT_TOP` scope.
  - Enumerate all remaining HUD draw sites — `git grep` punch list для
    `FONT2D_DrawString*`, `PANEL_draw_*`, `OVERLAY_*`, `DRAW2D_*`,
    `MSG_*`, литералов `320`/`240`. Каждый — в нужный `UIModeScope`.
    UI **в целом выглядит ок** на момент завершения, но возможно
    остались ещё неудобные кейсы на нестандартных аспектах.
  - **NIGHT lighting pool overflow** на wide/tall FOV — UWORD рефактор
    (post-1.0). Workaround: `OC_FOV_MULTIPLIER` остаётся compile-time,
    user-facing FOV slider не делаем до фикса.
  - **Focus callback cursor show/hide breaks linker** — defensive код,
    SDL и сам справляется. Удалить или дебажить.

Полный реестр недоработок и резолвед-фиксов → [`issues.md`](issues.md).

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
- **Road-sign flashes & search-mode progress bar framed at CENTER_CENTER.**
  Both blocks in `PANEL_last` ([`panel.cpp`](../../new_game/src/ui/hud/panel.cpp))
  wrapped in `UIModeScope(CENTER_CENTER)` — sign quad and search
  bubble/bar/text now stay anchored inside the centred 4:3 region on
  any aspect. Previously they sat in default uniform-scale scope which
  pushed them into the lower-left quadrant on widescreen / non-4:3 FBOs.
  Mission-countdown timer (`PANEL_draw_timer` arg) was already framed
  via `PANEL_draw_buffered`'s `LEFT_BOTTOM` scope — the (320, 50)
  argument from `eway.cpp` is dead (stored but ignored, real position
  is `m_iPanelXPos + 171, m_iPanelYPos - 118` inside the radar).
- **UI rendering split from scaled scene FBO** — HUD (PANEL_draw_buffered
  / PANEL_last / PANEL_inventory / cheat==2 FPS overlay), pause/won/lost
  menu, fade-out, console, F1/F11 debug overlays, AENG_draw_messages,
  FONT_buffer flush all moved to a post-composition pass on the default
  framebuffer. Means UI no longer goes through FXAA / bilinear upscale
  → text stays pixel-sharp at `OC_AA_ENABLE = true` and any
  `OC_RENDER_SCALE`. Scene-side `OVERLAY_handle` keeps only the
  depth-dependent overlays (gun sights, deferred view lines, enemy
  health) wrapped in their own `PANEL_start`/`PANEL_finish` POLY batch.
  HUD block in the UI pass is gated on `NET_PERSON(0) && NET_PLAYER(0)`
  (the actual precondition the PANEL_* code requires) — a `GAME_STATE`
  bit check is insufficient because `ATTRACT_loadscreen_init` runs
  flips with the gameplay state bits set but no materialised player.
  Plan + handoff: [`split_ui_from_scene_plan.md`](split_ui_from_scene_plan.md).
- **Latent UI-scissor offset bug surfaced and fixed.** On windows
  wider than `FOV_CAP_ASPECT` the composition pass centres the FBO
  with pillar bars (`dst_x ≠ 0`), but `ge_set_scissor` /
  `ge_disable_scissor` weren't accounting for that offset — `glScissor`
  takes absolute FB coords while `push_ui_mode` hands it scissor
  relative to the dst rect (assumes `(0, 0)` origin like the shader's
  `u_viewport`). Symptom: pause menu / cutscene dialog / any
  `UIModeScope` UI clipped to a narrow band on the right where
  scissor and viewport intersected. Fix: `s_ui_vp_x/y` track the
  actual viewport origin on the default FB, scissor adds it.
- **Universal drag-resize via last-frame snapshot.** Interactive
  window-edge drag pauses the game's main loop (Windows owns the
  message pump), so re-running the rendering pipeline on each drag
  tick saw stale / consumed per-frame UI state — timer queues had
  been emptied, dirty bits had been cleared, etc., so individual UI
  elements would flicker / vanish until the drag ended. Replaced
  the re-run approach with a snapshot: every normal flip captures
  the finished default-FB into a backing texture (one
  `glBlitFramebuffer`), and `ge_present_for_drag` simply stretches
  that snapshot to the new window with `GL_LINEAR`. Whatever the
  user saw a moment ago, they keep seeing — distorted to match the
  live window aspect during the drag, snapping back to fresh content
  once drag ends. Universal: works for any current and future UI
  element regardless of its state-cycle. The three flip sites
  (`ge_flip`, `ge_blit_back_buffer`, `ge_video_draw_and_swap`) all
  call a shared `present_and_swap` helper so the snapshot can't be
  forgotten when adding a new flip path.
- **`ge_set_viewport` / `ge_clear` made UI-mode aware.** Symmetric
  to the scissor fix — `ge_set_viewport(0, 0, w, h)` from inside
  the UI pass now lands the viewport at the dst rect origin, not
  at framebuffer (0, 0); `ge_clear` keeps scissor enabled in UI
  mode so the clear stays bounded to the UI viewport instead of
  wiping composition's outer pillarbox bars. Made the
  `FRONTEND_display` split below possible — without these two
  fixes any UI-pass call site that resets viewport/clears would
  misbehave on non-matching-aspect windows.
- **Frontend menu split into background + overlay halves.**
  `FRONTEND_display` now does only the scene-FBO half (background
  image, transition, kibble particles); the new
  `FRONTEND_display_overlay` does menu items / arrows / title /
  district map markers / mission select / corner tab buttons /
  in-dev moving-panel preview, called from the post-composition
  UI pass. Means main menu / options / save-load / map screen
  text + map dots stay crisp; the leaf particles still pass
  through composition AA where they look right. Gated by a
  per-frame "dirty bit" raised by the background half — cleared
  when `FRONTEND_loop` returns a transition action and at the
  entry of `ATTRACT_loadscreen_init` / `MAIN_main` (outro) so the
  overlay doesn't bleed into loadscreen / outro frames.
- **Engine-internal constants moved out of `config.h`.** Aspect
  clamps `FOV_CAP_ASPECT` / `FOV_MIN_ASPECT` (renamed from
  `OC_FOV_*_ASPECT`) live in
  [`engine/graphics/aspect_clamp.h`](../../new_game/src/engine/graphics/aspect_clamp.h)
  — engine hard limits, not user-tuneable. `RENDER_SCALE_MIN`
  is now a `static constexpr` inside the only consumer
  (`compute_fbo_size`). `OC_DEBUG_*` toggles moved to
  [`debug_config.h`](../../new_game/src/debug_config.h) sibling.
  `config.h` keeps only the player-tuneable knobs.
- **Universal 1-pixel right/bottom-edge gap fix.** Every POLY-path 2D
  primitive (untextured rects via `AENG_draw_rect`, textured quads via
  `PANEL_draw_quad`, 3D world geometry, anything that ends up calling
  `PolyPoint2D::SetSC`) used to land at NDC `[-1 - 1/W, 1 - 1/W]` after
  the `tl_vert.glsl` D3D6 pixel-center -0.5 shift, leaving the right
  and bottom-most pixel rows undrawn. The compensation was being
  re-discovered and applied per-site (`push_fullscreen_ui_mode` xo/yo,
  `font_atlas` ox/oy, `gl_blit_textured_quad`, `ge_blit_to_framed_area`,
  `sky.cpp` star quads). Folded into `PolyPoint2D::SetSC` (the single
  chokepoint every POLY-path vertex submission goes through — AddFan,
  POLY_add_quad_fast, POLY_add_triangle_fast, the nearclipped fan
  helpers in poly.cpp) as a uniform `PIXEL_HALF_OFFSET = 0.5f` added
  to sx / sy. So every current and future POLY-path draw gets the gap
  fix automatically and gets correct UV centring as a bonus. The
  per-site compensations in direct-TL paths (font_atlas, gl_blit, sky
  stars, composition blits) still apply manually because they bypass
  SetSC; only `push_fullscreen_ui_mode` had to drop its now-redundant
  0.5 to avoid a double shift. **Choosing SetSC over AddFan was load-
  bearing**: AENG_draw_rect / PANEL_draw_quad route through
  `POLY_add_quad_fast` (and friends) which inline their own
  `pv->SetSC(X*scale + offset, ...)` and never enter AddFan — putting
  the offset in AddFan only fixed the AddFan path and missed all the
  fast-path callers, which is what kept the pause-menu darken /
  input-debug backdrop bleeding 1 px of scene through on the right.
- **3D-anchored debug labels (`AENG_world_text`) and the input debug
  panel now share one virtual-coords pipeline.** `FONT_buffer_add` always
  treated `(x, y)` as literal framebuffer pixels, but `pp.X`/`pp.Y` out
  of `POLY_perspective` are virtual coords in the 0..480*aspect × 0..480
  game space, and `input_debug_text` was wrapping with `ScreenWidth/640`
  × `ScreenHeight/480` (different per-axis scale on widescreen).
  Symptoms: Ctrl-held debug labels above NPCs drifted up-left as the
  window grew; panel text and rects de-aligned on non-4:3 windows; on
  portrait / very narrow windows the panel layout overflowed off-screen
  to the right. New `FONT_buffer_add_virtual` companion to
  `FONT_buffer_add` queues the message with a per-message `scale_x` /
  `scale_y` snapshot captured from `PolyPage::s_X/YScale` AT SUBMIT
  time. `FONT_buffer_draw` resolves virtual messages as `(x * scale_x,
  y * scale_y)` at flush time. The submit-time snapshot is required
  because the queue flushes once at the end of the UI pass — by then
  any `FullscreenUIModeScope` / `push_ui_mode` the caller used has
  been popped, so reading the live `s_XScale` at flush would pick up
  the wrong scope. AENG_world_text submits during scene render
  (captures uniform `ScreenHeight/480`); the input debug panel wraps
  its draws in `PolyPage::FullscreenUIModeScope` (captures non-uniform
  `ScreenWidth/640 × ScreenHeight/480`) and uses fixed logical 640×480
  layout, so on portrait the panel compresses horizontally instead of
  overflowing. `debug_help` (F1 legend) keeps the literal-pixel
  `FONT_buffer_add` path — pinned to a fixed top-left pixel corner
  intentionally.
- **Multi-line `FONT_buffer_add_virtual` messages: line-start `x` reset
  on `\n` now uses the SCALED line origin.** First implementation reset
  to raw `fm->x` (unscaled virtual coords), so `AENG_world_text` debug
  blocks above NPCs (multi-line via embedded `\n`) drew line 1 correctly
  but lines 2+ snapped to the upper-left of the framebuffer. Cached the
  resolved `line_start_x` once per message and reset to that on `\n`.
  Line spacing (`y += 10`) intentionally stays unscaled because the
  glyphs themselves render at a fixed 5×9 pixel size — stretching the
  gap between lines independently of the glyph height would break visual
  density on non-1:1 scopes.
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

См. блок **Status: ✅ COMPLETED** в начале файла — там перечислены
оставшиеся stragglers и issues, переехавшие в global. Полный реестр
проблем (включая resolved) → [`issues.md`](issues.md).

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
