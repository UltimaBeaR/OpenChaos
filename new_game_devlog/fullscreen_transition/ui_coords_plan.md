"# UI coordinate system — implementation plan

Refactor of UI rendering to fix the pillarbox / undersized HUD on widescreen.
Read [`concepts.md`](concepts.md) and [`issues.md`](issues.md) first for the
background. This file is the **execution plan** — what to build, in what
order, with what acceptance criteria. Designed so another agent can pick up
mid-flight without losing context.

---

## Design (agreed with user, do not re-litigate)

### Coordinate system

All UI coordinates are **`float [0..1]`** screen coordinates. No virtual
640×480 pixels at the call site (long term). For the migration period a
helper accepts old-style `(x_px640, y_px480)` and converts internally.

### Three classes of UI

| Class           | Examples                                              | Coord rule                                                                 |
|-----------------|-------------------------------------------------------|----------------------------------------------------------------------------|
| **Framed**      | Main menu, pause backdrop, mission map, briefing, video bars, cinematic dialog bars (initially) | Drawn inside a virtual 4:3 frame centered in the screen; black bars fill the rest. Default for everything until proven otherwise. |
| **Anchored HUD**| Radar, weapon icon, ammo, health bar, on-screen messages | Anchored to a side/corner of the **real** screen; size scaled by `g_frame_scale` (so element retains its 4:3-equivalent physical size). |
| **3D-attached** | NPC HP bars over heads, 3D debug text                | Position from 3D projection (`POLY_perspective`), no anchor. Size in real pixels (fix existing 640×480 hardcodes). |

### Frame scale

```
g_frame_scale = min(RealH / DisplayHeight, RealW / DisplayWidth)
              = min(RealH / 480,           RealW / 640)
g_frame_w_px  = DisplayWidth  * g_frame_scale   // = 640 * scale
g_frame_h_px  = DisplayHeight * g_frame_scale   // = 480 * scale
```

On any screen wider than 4:3 → reduces to `RealH / 480`. On a (hypothetical)
narrower-than-4:3 screen → reduces to `RealW / 640`. Always: a virtual 4:3
frame fits inside the real screen, touching the constraining axis.

### Anchor enum

```cpp
enum class UIAnchor {
    LEFT_TOP,    CENTER_TOP,    RIGHT_TOP,
    LEFT_CENTER, CENTER_CENTER, RIGHT_CENTER,
    LEFT_BOTTOM, CENTER_BOTTOM, RIGHT_BOTTOM,
};
```

Anchor describes where the **framed 4:3 region** is pinned inside the real
screen. `CENTER_CENTER` is the default and what every framed element uses
unless we have a reason to anchor differently. Anchored HUD elements pick
their own anchor per element (radar = `RIGHT_TOP`, ammo = `LEFT_BOTTOM`,
etc.).

### API

In `new_game/src/engine/graphics/ui_coords.{h,cpp}`:

```cpp
struct Vec2f { float x, y; };

// Cached per-mode (recomputed in game_mode_changed).
extern float g_frame_scale;
extern float g_frame_w_px;
extern float g_frame_h_px;
void ui_coords_recompute();   // called from game_mode_changed

// [0..1] framed coord -> [0..1] screen coord.
Vec2f frame_to_screen(Vec2f framed01, UIAnchor anchor);

// Inverse: [0..1] screen coord -> [0..1] framed coord (mouse picking).
Vec2f screen_to_frame(Vec2f screen01, UIAnchor anchor);

// Migration helper: old 640x480 pixel coords -> [0..1] screen.
Vec2f old_px_to_screen(float x_px640, float y_px480, UIAnchor anchor);

// Convenience: [0..1] screen -> real framebuffer pixels.
Vec2f screen_to_pixels(Vec2f screen01);

// Convenience: old 640x480 pixels -> real framebuffer pixels (most common).
Vec2f old_px_to_screen_pixels(float x_px640, float y_px480, UIAnchor anchor);
```

`old_px_to_screen_pixels` is the bulk-migration entry point. Replace
`PANEL_draw_quad(450, 200, ...)` with
`Vec2f p = old_px_to_screen_pixels(450, 200, UIAnchor::CENTER_CENTER); PANEL_draw_quad(p.x, p.y, ...)`.

### PolyPage UI mode (backend-transform approach)

`PolyPage::AddFan` was extended from a pure scale to an affine transform:

```cpp
pv[ii].SetSC(pts[ii]->X * s_XScale + s_XOffset,
             pts[ii]->Y * s_YScale + s_YOffset, ...);
```

`s_XOffset`/`s_YOffset` default to 0 (so 3D rendering is unaffected).
`SetScaling` resets them to 0 explicitly.

`push_ui_mode(UIAnchor)` snapshots the current `s_XScale/s_YScale/
s_XOffset/s_YOffset`, then sets `s_XScale = s_YScale = g_frame_scale` and
`s_XOffset/s_YOffset` to the framed-region origin in real pixels for the
chosen anchor. Effectively, every existing call site that submits virtual
640×480 vertex coords is now drawn into a 4:3 framed region inside the
real screen, anchored as requested. No call-site changes needed.

`pop_ui_mode()` restores the snapshot. A depth counter makes push/pop
pairs reentrant (only outermost push/pop applies).

`PolyPage::UIModeScope` is a small RAII wrapper:

```cpp
struct UIModeScope {
    UIModeScope(UIAnchor anchor) { push_ui_mode(anchor); }
    ~UIModeScope() { pop_ui_mode(); }
};
```

Use as the first local at the top of any UI render entry-point. The
destructor handles every return path (early-returns, exceptions if any).

This approach replaced the original "rewrite each call site" plan because
the affine extension was 4 lines in `AddFan` while the call-site rewrite
would have touched ~120 sites. Per-element anchors for the in-game HUD
(Stage 3c) work the same way — just `UIModeScope` with a non-CENTER
anchor at the function top.

Trade-off: this requires that `PolyPage::AddFan` is the only path UI
geometry takes to the GPU. `AddWirePoly` was empty (PC stub) and
verified. **Discovered during integration:** `POLY_add_triangle_fast`,
`POLY_add_quad_fast`, `POLY_add_nearclipped_triangle` (all in
`poly.cpp`) write vertex screen coords directly through `pp->s_XScale`
without going via `AddFan` — those 10 sites also got the `+ s_XOffset` /
`+ s_YOffset` extension. Any future UI primitive path must apply the
same affine.

### Hardware scissor for the framed clip

`push_ui_mode` programs `glScissor` to the framed pixel rectangle in
addition to the affine. This clips draws that would otherwise spill
onto the black bars — kibble particles in the main menu were the
visible offender, and any future fullscreen-style UI effect (rain,
snow, etc.) is now contained for free.

Backend additions (`engine/graphics/graphics_engine`):
- `ge_set_scissor(x, y, w, h)` / `ge_disable_scissor()` — public API,
  honour the OpenGL bottom-up Y flip same as `ge_set_viewport`.
- `ge_set_viewport` no longer overwrites scissor when
  `PolyPage::in_ui_mode()` is true — the pipeline owns scissor in that
  state. Outside UI mode, default behaviour (scissor == viewport)
  unchanged.
- `ge_clear` snapshots and disables scissor for the duration of
  `glClear` so the bars get cleared too — without this, `ge_clear`
  inside a UI scope only cleared the framed region and previous-frame
  garbage (e.g. loading screen) bled onto the bars.
- `ge_video_draw_and_swap` snapshots and disables scissor around the
  video present, restores after — without this the video was clipped
  to the framed UI rectangle when called from inside a UI scope.

### Fullscreen UI mode (escape hatch for full-screen effects)

Some intentional full-screen effects need to ignore the framed clip.
`push_fullscreen_ui_mode()` / `FullscreenUIModeScope` push an affine
that stretches virtual 640×480 across the entire framebuffer (non-
uniform on widescreen) and **disables** scissor. Currently used by:
- `FRONTEND_show_xition` had it briefly, then reverted — transition is
  now framed (see "Framed background and transition" below).
- No live consumers right now. Keep the API for future fullscreen
  overlays (e.g. screen flashes that intentionally cover the whole
  display).

### UI mode stack

The original two-value snapshot was replaced with a proper 8-deep
stack of `{xs, ys, xo, yo, scissor_active, sx, sy, sw, sh}` so nested
push/pop pairs with **different** modes (framed inside framed, or
fullscreen inside framed for an escape-hatch effect) restore correctly.
The depth limit fires a loud assert if exhausted.

### Framed background and transition

`ge_blit_background_surface` (the path behind every `ge_show_back_image`)
now **always** renders the 640×480 background image into the framed
4:3 region with a black `glClear` first, regardless of UI mode state.
That's because backgrounds are inherently 4:3 source content (the BMPs
are 640×480) and the function is called from both UI scopes
(`FRONTEND_display`) and non-scope loaders (`ATTRACT_loadscreen_init`,
`MainScreen`). Always-framed means every caller works without needing
to know about the UI scope.

**Invariant — don't break it:** `ge_blit_background_surface` snapshots
`s_vp_*`, calls `ge_set_viewport(0, 0, gl_context_get_width(),
gl_context_get_height())` for the duration of the blit, then restores.
The TL vertex shader maps pixel coords to NDC through `u_viewport`
which tracks the most-recent `ge_set_viewport`. Game init paths
commonly leave `s_vp_*` at the virtual 640×480 viewport before the
first background blit happens — without the snapshot/restore the
framed quad gets squashed into a 640×480 patch in the corner of the
real FBO (the symptom we hit pre-fix). Don't drop the snapshot.

`ge_blit_background_surface` also calls `ui_coords::recompute(
gl_context_get_width(), gl_context_get_height())` on every invocation
and bails out early if the GL context dimensions are still 0. This is
a defensive lazy-init — covers the very early init window where the
mode-change callback hasn't fired yet, plus any future window resize
that doesn't propagate through the callback.

`ge_blit_surface_to_backbuffer` (the only consumer is
`FRONTEND_show_xition`) now interprets its input `(x, y, w, h)` as
**framed-area pixel coordinates**: source UVs are mapped through the
framed dimensions and destination pixels are offset by the framed
origin. `FRONTEND_init_xition` / `FRONTEND_show_xition` were updated
to compute `MidX/MidY/ScaleX/ScaleY` and the wipe rect in framed
pixel space (`ui_coords::g_frame_w_px / g_frame_h_px`) instead of
`RealDisplayWidth/Height`. Wipe and iris transitions now stay inside
the framed region instead of expanding to fullscreen mid-animation.

---

## Files we will touch

### New files

- `new_game/src/engine/graphics/ui_coords.h`
- `new_game/src/engine/graphics/ui_coords.cpp`
- CMake registration: `new_game/src/engine/graphics/CMakeLists.txt` (or
  whichever list aggregates this directory; check before adding).

### Modified files (Stage 2 — backend)

- `new_game/src/engine/graphics/pipeline/polypage.{h,cpp}` — add
  `push_ui_mode` / `pop_ui_mode`.
- `new_game/src/game/game.cpp` — call `ui_coords_recompute()` from
  `game_mode_changed()` alongside the existing `PolyPage::SetScaling`.

### Modified files (Stage 3 — call sites, in this order)

3a. **Frontend / menus / pause** (framed, CENTER_CENTER everywhere):
- `new_game/src/ui/frontend/frontend.cpp`
- `new_game/src/ui/menus/widget.cpp`

3b. **Cinematic bars / on-screen messages** (framed CENTER_CENTER initially):
- `new_game/src/game/input_actions.cpp` (MSG_add and friends)
- whichever module owns the dialog bars (find during work)

3c. **In-game HUD** (per-element anchors):
- `new_game/src/ui/hud/panel.cpp` — radar, weapon, ammo, health,
  body-part, wristwatch
- `new_game/src/ui/hud/overlay.cpp` — damage / book / kick overlays,
  panel frames
Anchor assignment per element (initial guesses, verify in game):
- Radar / compass → `RIGHT_TOP`
- Weapon icon / ammo → `LEFT_BOTTOM`
- Health bar → `LEFT_BOTTOM`
- Body-part damage → `LEFT_BOTTOM` (next to health)
- Wristwatch → `RIGHT_BOTTOM`
- On-screen messages → `CENTER_BOTTOM` (cinematic) or `CENTER_TOP`
  (verify in game)

3d. **Debug text & primitives**:
- `new_game/src/engine/debug/input_debug/input_debug.cpp`
- text-rendering call sites (use `Grep` for `CONSOLE_text`,
  `FONT2D_DrawString`, `draw_text_at`, `MSG_add`)
- hud-rendering primitives (use `Grep` for `AENG_draw_rect*`,
  `POLY_add_rect`, `POLY_add_quad`, `PANEL_draw_quad`,
  `PANEL_draw_health_bar`, etc.)
Initial anchor: `CENTER_CENTER` (framed). Adjust per-call after visual
verification.

3e. **3D-attached HUD** (NPC HP bars, 3D debug text):
Find via `Grep` for callers of `POLY_perspective` that draw UI-like
overlays. Replace any 640/480 hardcodes with `RealW/RealH` reads.
This is the trickiest stage and may need its own design pass.

### Stage 4 — black bars (DONE incidentally)

Resolved as a side effect of two changes in Stage 3a:
- `ge_clear` now snapshots and disables scissor while clearing, so the
  per-frame `ge_clear` inside `FRONTEND_display` wipes the entire
  framebuffer (not just the framed region). Whatever the UI doesn't
  draw stays the clear colour — black.
- `ge_blit_background_surface` clears the screen black first, then
  blits the 640×480 background into the framed area only. Bars are
  guaranteed black even on call sites outside any UI scope.

No explicit black-quad rendering needed.

### Stage 5 — mouse

`screen_to_frame` will exist from stage 1. Wire it into menu picking
when we get to active mouse support. **Note:** user says mouse in main
menu doesn't work today, this is not a regression — it's pre-existing.
Land the inverse helper now, hook it up later.

### Stage 6 — docs cleanup

- Update [`concepts.md`](concepts.md) with new terminology (framed coords,
  screen coords, UIAnchor, frame scale).
- In [`issues.md`](issues.md) move HUD pillarbox → Resolved.
- Add a `🟣 Out of scope` line for the explicitly-deferred items: split UI
  from scaled FBO, render-time AA upgrade, wibble amplitude scaling,
  user-tunable HUD multiplier.

---

## Stage-by-stage execution

### Stage 1 — Module scaffolding

**Deliverables:**
- `ui_coords.h` with API as above (declarations + `Vec2f` + enum).
- `ui_coords.cpp` with implementations + globals + `ui_coords_recompute()`.
- CMake entry.
- `ui_coords_recompute()` wired into `game_mode_changed` so globals are
  populated on first frame.

**Acceptance:**
- Builds clean (`make build-release`, exit code 0, `Linking CXX executable`
  in tail).
- Game launches, behaves identically to today (no call sites use the new
  API yet).
- Mentally check: at 1920×1080, `g_frame_scale == 2.25`, `g_frame_w_px == 1440`,
  `g_frame_h_px == 1080`.

### Stage 2 — PolyPage UI mode

**Deliverables:**
- `PolyPage::push_ui_mode` / `pop_ui_mode` implementation.
- No call site uses it yet.

**Acceptance:**
- Builds clean.
- Game behaves identically (still no integration).

### Stage 3a — Frontend / menus / loading / pause / scores (DONE)

Wrapped these UI render entry-points with
`PolyPage::UIModeScope _ui_scope(UIAnchor::CENTER_CENTER);`:

- `widget.cpp:FORM_Draw`            — generic form rendering (incl. `form_leave_map`)
- `gamemenu.cpp:GAMEMENU_draw`      — pause/won/lost overlay
- `attract.cpp:ScoresDraw`          — end-of-mission score screen
- `attract.cpp:ATTRACT_loadscreen_draw` — loading bar
- `frontend.cpp:FRONTEND_display`   — main menu, options, save/load, briefing

Each is one-liner at the function top — RAII ensures pop on every return.
Existing per-pixel literals untouched; the affine transform in `AddFan`
maps them to a 4:3 region centered in the real framebuffer.

Verified at 1920×1080:
- Menu / pause / score / briefing render in centered 4:3 region, bars black.
- Loading screen too (background path made always-framed — covers the call
  sites that aren't inside a UI scope, like `ATTRACT_loadscreen_init`).
- Kibble particles (leaves/rain/snow) in the main menu clip cleanly at the
  framed border thanks to scissor — no bleed onto the bars.
- Transition wipes/iris animate within the framed area (not "expanding to
  fullscreen and snapping back" as before).

At 640×480: identity (`g_frame_scale == 1`, offsets 0, scissor =
viewport) — pixel-perfect compatible with the original layout.

### Outro framing (DONE — extension of Stage 3a)

Outro has its own backend (`engine/graphics/graphics_engine/backend_opengl/
outro/core.cpp`) that draws via the TL pipeline using pixel coords
scaled by `OS_screen_width / OS_screen_height`. To frame it like the
rest of the UI on widescreen, `oge_init` was changed:

1. Compute framed dims via `ui_coords::recompute(ge_get_screen_width(),
   ge_get_screen_height())`.
2. `OS_screen_width / OS_screen_height` set to the framed dims so all
   outro coords are generated relative to a virtual 4:3 viewport.
3. **Two-level viewport**: `ge_set_viewport(0, 0, frame_w, frame_h)`
   programs the shader-side `u_viewport` (used by `tl_vert.glsl` for
   pixel→NDC math, so outro coords 0..frame_w map cleanly to NDC
   -1..+1), then `glViewport(origin_x, origin_y, frame_w, frame_h)`
   is called directly to override the GL-side viewport, placing the
   NDC quad in the centred screen rect. Without this two-level split,
   the shader would map outro coords through `u_viewport.x = origin_x`
   and they'd fall outside NDC, leaving outro mostly invisible.
4. `glScissor(0, 0, screen_w, screen_h)` — open scissor to the full
   render target. `glViewport` alone confines rasterization to the
   framed region; using a tight scissor caused motion-trail artifacts
   on outro's sliding-bar transition (partial fragments at the edge
   couldn't be cleared the next frame, leaving a static strip).
5. Per-frame clear in `oge_scene_begin`: outro's main loop calls
   `OS_clear_screen` exactly once at startup; the design relied on
   `BACK_draw` fully repainting the framebuffer every frame. In our
   framed setup `BACK_draw` doesn't always cover the very last pixel
   of the framed area, and stale fragments from the bar transition
   leave a thin strip. Adding `ge_clear(true, true)` per frame in
   `oge_scene_begin` clears anything `BACK_draw` misses to black.
6. Outro textures changed from `GL_REPEAT` to `GL_CLAMP_TO_EDGE` —
   bilinear sampling at `UV=1.0` was wrapping to the opposite-colour
   leftmost texel, visible as a thin strip on the right edge of outro
   sprites/photos at higher resolutions.

**Don't break:** the two-level viewport split is load-bearing. If a
future change collapses it back to a single `ge_set_viewport(origin,
origin, frame_w, frame_h)`, outro will mostly disappear (vertices map
outside NDC). The per-frame `ge_clear` in `oge_scene_begin` is also
load-bearing — without it the bar-transition trail returns.

### Video framing (DONE — extension of Stage 3a)

`ge_video_draw_and_swap` aspect-fits each video into the framed 4:3
region instead of the full window:

```cpp
const float frame_w_ndc = frame_w / (float)win_w;
const float frame_h_ndc = frame_h / (float)win_h;
const float fa = frame_w / frame_h;
const float aspect_sx = (va > fa) ? 1.0f : va / fa;
const float aspect_sy = (va > fa) ? fa / va : 1.0f;
const float sx = aspect_sx * frame_w_ndc;
const float sy = aspect_sy * frame_h_ndc;
```

So a 4:3 video (Eidos, logo24) fills the framed region exactly →
pillarbox on the sides matches the surrounding UI bars; a 16:9-ish
video (PCINTRO ~ 640×340) gets letterbox top/bottom inside the framed
region plus pillarbox outside — visually consistent with the framed
UI rather than randomly fullscreen.

`ge_video_draw_and_swap` also snapshots and restores GL scissor around
the video draw (`glGetBooleanv(GL_SCISSOR_TEST)` / `glGetIntegerv(
GL_SCISSOR_BOX)`) so a parent UI scope's tight scissor doesn't crop
the video into a small square — without this the video appeared as a
glitched square in the framed region during play.

### Stage 3b — Messages, cinematic bars

Same shape as 3a, applied to MSG_*. Cinematic dialog bars are framed
CENTER_CENTER initially.

**Acceptance:**
- 1920×1080: messages appear centered in the framed region.
- 640×480: identical to before.

### Stage 3c — In-game HUD

Per-element anchors per the table above. This is the visible win for
players: HUD ends up at the corners of the real screen, sized
proportionally to screen height.

**Acceptance:**
- 1920×1080: radar in top-right corner, ammo/health in bottom-left, etc.
- 4:3 fallback: identical layout to before (since at 4:3 anchors
  collapse to the same pixel positions).
- 21:9 ultrawide: HUD elements still hug the corners of the actual screen.

### Stage 3d — Debug text & primitives

Wraps remaining UI calls. Initially CENTER_CENTER, adjust per-element
on visual review.

**Acceptance:**
- F1 legend (bangunsnotgames) appears at consistent visual size and
  position across 640×480, 1920×1080, 2560×1080.
- DualSense debug overlays (stick rectangles + their text) overlap
  correctly — no drift between text and primitives.

### Stage 3e — 3D-attached HUD

NPC HP bars and 3D debug text. Find 640/480 hardcodes, replace with
RealW/RealH or projection-derived sizes.

**Acceptance:**
- HP bars sit on character heads at all resolutions.
- 3D debug text appears where it should in world space.

### Stage 4 — Black bars

Whatever menu/frontend doesn't already paint outside the 4:3 frame —
clear black. Decide between `glClear` and explicit quads after
inspecting.

**Acceptance:**
- 1920×1080: outside-frame area is solid black during menus, no
  garbage / leftover scene buffer.

### Stage 5 — Mouse picking inverse helper

Use `screen_to_frame` in any active picking path. Defer integration
until mouse menu support is being worked on (separate task).

### Stage 6 — Docs

Update concepts.md, close HUD pillarbox in issues.md, document deferred
items.

---

## Out of scope (explicitly NOT doing in this rework)

- **Split UI from scaled scene FBO.** UI continues to draw into the scaled
  scene FBO and pass through FXAA + upscale. User accepts this for now;
  FXAA softening of text is tolerable.
- **Render-time AA upgrade** (canonical FXAA 3.11 / SMAA 1x). Separate
  follow-up.
- **Wibble amplitude scaling.** Separate follow-up.
- **User-configurable HUD scale multiplier.** Future work; HUD scale is
  hardwired to `g_frame_scale` for now.
- **Renaming `RealDisplayWidth/Height`.** Separate follow-up.
- **Aspect ratio strategy beyond framed-CENTER_CENTER for menus.** If
  framed menus look wrong on ultrawide we'll revisit — for now framed +
  CENTER_CENTER is the only mode.

---

## Open questions / known unknowns

- **Where in the call stack to do push/pop UI mode.** Default plan: inside
  the high-level draw func. May change if it turns out PANEL_draw_quad is
  also reused by 3D-pipeline-adjacent code (verify).
- **Black bars approach** — `glClear` vs explicit quads. Decide in stage 4.
- **3D-attached HUD** — may need its own concept beyond what's planned
  here. Reassess at stage 3e.
- **MSG / cinematic bar anchor.** Initial CENTER_CENTER; user may want
  CENTER_BOTTOM after seeing it. Iterate.
- **Existing `PolyPage::SetScaling` callers** — verify nobody calls it
  directly from a UI path expecting the old behavior. The push/pop pair
  must save+restore exactly what SetScaling left, not assume `RealH/480`.

---

## Testing matrix

After each visible stage, verify at:
- **640×480 windowed** — must be pixel-identical to pre-rework (this
  catches regressions where the migration broke 4:3).
- **1920×1080 fullscreen** — primary widescreen target.
- **2560×1080 (21:9)** — ultrawide stress test (framed area takes only
  ~58% of the screen).
- **1280×800 (Steam Deck-ish)** — small + slightly-non-4:3 sanity.

For each: walk through main menu → start mission → in-game HUD → pause
menu → mission map. Compare visually against pre-rework screenshots if
available, otherwise against memory of \"correct\" layout.

---

## If you (next agent) are picking this up

1. Read this file top to bottom.
2. Read [`concepts.md`](concepts.md) for the pipeline / scale formulas
   and [`issues.md`](issues.md) §HUD pillarbox for symptom context.
3. Run `git log --oneline -20` and look for commits with subjects like
   `ui_coords`, `framed`, `anchor`, `PolyPage UI mode`, `outro framed`,
   `video framed` — the latest one tells you which stage was last
   completed.
4. Resume at the next stage. Each stage has clear acceptance criteria
   above; don't move on until the current stage's acceptance holds.
5. **Do not** silently change the design choices in this doc. If you
   need to, write the new decision into this file with a note explaining
   why — don't just diverge.

### Current state (as of last commit on `resolution_change`)

**DONE** (Stage 3a + outro + video):
- `ui_coords` module + `PolyPage::UIModeScope` infra (Stages 1, 2).
- Frontend / menus / loading / pause / score wrapped in framed scope.
- Background image (`ge_show_back_image`) always renders framed (with
  black-clear of bars), with viewport snapshot/restore around the blit
  so it doesn't depend on whoever set s_vp_* last.
- Frontend transitions (wipe / iris) framed.
- Kibble particles auto-clip to framed area via scissor.
- Outro (credits, wireframe sequence) framed via two-level viewport,
  open scissor, per-frame `ge_clear`, and `CLAMP_TO_EDGE` on textures.
- Intro / cutscene videos aspect-fit into framed area.
- `ge_clear` ignores scissor so it wipes the full FBO including bars.
- `ge_video_draw_and_swap` snapshots/restores scissor.
- `OpenDisplay` clears the freshly-allocated scene FBO before the
  first present so no flicker from uninitialised GPU memory.

**NEXT** (in order):
- Stage 3b — MSG / cinematic bars (small).
- Stage 3c — In-game HUD with per-element anchors (radar=`RIGHT_TOP`,
  ammo=`LEFT_BOTTOM` etc.). This is where the visible HUD pillarbox
  in [`issues.md`](issues.md) actually goes away.
- Stage 3d — Debug text / primitives.
- Stage 3e — 3D-attached HUD (HP bars over heads).
- Stage 4 — Black bars: already incidentally resolved, no work needed.
- Stage 5 — Mouse picking inverse helper.
- Stage 6 — Docs cleanup.

**Out-of-scope follow-ups** in [`issues.md`](issues.md):
- Fish-eye distortion at extreme aspect ratios (e.g. 1920×480) —
  3D world only, low priority.
- Wibble amplitude scaling, render-time AA upgrade, `RealDisplay*`
  rename, `s_work_screen_buf` audit, focus-callback linker fix,
  split UI from scene FBO.
"