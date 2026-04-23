# Known issues & fix plans

Read [`concepts.md`](concepts.md) first for terminology.

---

## 📝 Design discussions

Aspect ratio + UI coordinate system design is now agreed and partly
implemented. See [`ui_coords_plan.md`](ui_coords_plan.md) — read that
before touching any UI rendering. Summary of what landed:
- All UI is normalised to `[0..1]` floats via `ui_coords` helpers.
- A 4:3 framed region is centred in the real screen for menu / frontend
  / loading / pause. Hardware scissor + always-framed background paths.
- Per-element anchors (HUD radar/ammo/etc.) — design done, code
  pending in Stage 3c.

---

## 🟣 Render scale follow-ups

Render scale + scene FBO + composition pass are implemented (see
"Resolved" at the bottom for the full summary). What's still open:

- **Split UI from main render.** HUD / menu / overlay / text / console
  currently draw **into** the scaled scene FBO, so at scale < 1 they
  share the upscale blur and at any scale FXAA softens text. Fix: draw
  the UI **after** the composition pass, directly into the default
  framebuffer at native resolution. Scene still goes scene-FBO → FXAA +
  upscale → default FB; UI then draws on top, untouched. UI coord
  system and draw APIs stay as they are — only execution order changes.
  Requires enumerating every "UI draw" call site unambiguously (HUD
  panel, overlay, frontend, menu, on-screen messages, console, debug
  overlay). Depends on the aspect-ratio / UI-coord-system design
  discussion at the top of this file.
- **Rename `RealDisplayWidth/Height` → something accurate.** After the
  render-scale change, this global holds the **scaled render target**
  size, not the physical display. The name is now actively misleading
  (game code reading "RealDisplay*" is looking at the scene FBO, not
  the screen). Repo-wide rename to e.g. `RenderTargetWidth/Height` or
  `SceneWidth/Height`. Scope: rename + update all call sites + touch
  up `concepts.md` / `audit.md`. Not a behavior change, just
  nomenclature.
- **Replace the simplified FXAA with a proper shader AA.** The current
  `composite_frag.glsl` has a cut-down FXAA that drops the canonical
  3.11 edge-span search — result is effectively a cheap blur on
  contrast-detected pixels, not real antialiasing. Two upgrade paths,
  pick later:
  1. **Canonical FXAA 3.11** (~2-3 h). Public `Fxaa3_11.h` from NVIDIA,
     one file, no lookup tables, drop-in replacement for our composite
     fragment shader. Still a blur-based filter — better than ours but
     FXAA always softens text.
  2. **SMAA 1x** (~3-4 h). From github.com/iryoku/smaa. Three passes
     (edge detection → blending weights → neighborhood blending), two
     extra FBOs, embeds two precomputed lookup textures (AreaTex ~180
     KB, SearchTex ~1 KB) into the binary. Noticeably better than FXAA
     especially on text and thin details. Scene-FBO infra already
     exists, only the composition module needs the extra-pass runner.

---

## 🟡 HUD / UI layout — pillarboxed and small (partial)

### Status

**Resolved for** main menu, options, save/load, mission briefing, pause,
score, loading screen, mission map, transitions, attract mode background
— all centered in a 4:3 framed region with black bars (Stage 3a in
[`ui_coords_plan.md`](ui_coords_plan.md)).

**Still pending** — in-game HUD (Stage 3c): health bar, radar/compass,
weapon indicator, ammo, body-part damage, wristwatch, on-screen messages.
These need per-element anchors (radar = `RIGHT_TOP`, ammo = `LEFT_BOTTOM`
etc.) so they hug the real screen corners, not the framed centre. Same
infra (`PolyPage::push_ui_mode(anchor)`) — just call sites left to wrap.

### Symptoms (pre-fix; for in-game HUD still apply)

On non-4:3 resolutions (e.g. 1920×1080, 2560×1080):
- HUD elements — health bar, radar/compass, weapon indicator, ammo,
  body-part damage, wristwatch, on-screen messages — live in the **left
  1440×1080 region** of the 1920×1080 framebuffer. Right ~480 px shows
  background/empty space.
- Elements look small: a UI panel 200 px wide in virtual 640×480 space
  becomes 450 px on screen — fine on a laptop, tiny on a 27" 1080p.
- Text rendered through the HUD path shares this scaling.

### Root cause

HUD draws go through
[`PolyPage::AddFan`](../../new_game/src/engine/graphics/pipeline/polypage.cpp),
which multiplies every vertex by `s_XScale` and `s_YScale`. After our
uniform-scale fix in `game_mode_changed()`:

```
s_XScale = s_YScale = RealDisplayHeight / DisplayHeight
```

At 1920×1080 that's 2.25 on both axes. HUD coords are in 640×480 virtual
space, so `640 × 2.25 = 1440` ≠ 1920. Hence the pillarbox.

Keeping `s_XScale / s_YScale` equal is **required** for the 3D world to
render without distortion (see [`concepts.md`](concepts.md)). So the HUD
fix has to happen inside the HUD code, not by perturbing the shared
scale.

### Entry-point files (where HUD draws originate)

- [`ui/hud/panel.cpp`](../../new_game/src/ui/hud/panel.cpp) — in-game
  HUD: health bar, radar, weapon indicator, ammo, body-part damage,
  wristwatch. Hardcoded pixel positions like `(450 - 14, y)`,
  `(c0 × 150 + 5, y)` — all in 640×480 virtual.
- [`ui/hud/overlay.cpp`](../../new_game/src/ui/hud/overlay.cpp) — damage
  overlay, book overlay, kick overlay, panel frames. Resets GL viewport
  to full screen at `:296` before drawing.
- [`ui/frontend/frontend.cpp`](../../new_game/src/ui/frontend/frontend.cpp)
  — attract mode, title screen, mission briefing, transitions. Has one
  `if (RealDisplayWidth == 640)` fallback branch near `:397-402` that
  partly handles non-640 widths for the fade curtain.
- [`ui/menus/widget.cpp`](../../new_game/src/ui/menus/widget.cpp) — main
  menu, options, save/load, pause. Uses global mouse position and
  subtracts window position for hit-testing.
- [`game/input_actions.cpp`](../../new_game/src/game/input_actions.cpp)
  — on-screen messages (`MSG_add`).
- [`engine/debug/input_debug/input_debug.cpp`](../../new_game/src/engine/debug/input_debug/input_debug.cpp)
  — debug overlays. Dev-only, low priority but same layout issue.

### Fix approaches

**Option A — Separate HUD scale, with anchors.** Introduce a dedicated
HUD scale factor (independent of `s_YScale`). Each HUD element declares
an **anchor** (e.g. top-left, top-right, center, bottom-left) and an
offset in virtual pixels. Layout math at draw time resolves anchor +
offset against the real framebuffer dimensions; interior layout (widths,
paddings, etc.) stays in virtual coords and is scaled by the HUD scale.

- Pros: Most interior HUD layout stays untouched — just reposition the
  anchor. Clean mental model for future UI work.
- Cons: Need to distinguish HUD draws from 3D draws at the call site, or
  introduce a separate draw API for HUD. Anchor decisions must be made
  for every existing element (some are obvious — health bar → bottom-left
  corner; radar → top-right; others need design calls).

**Option B — Full rewrite to real-pixel coords.** Every HUD element is
rewritten to compute positions in real-pixel space directly. Drop the
virtual-640×480 abstraction for HUD entirely.

- Pros: Cleanest long-term.
- Cons: Huge refactor; every hardcoded pixel value needs reinterpretation.
  Likely a multi-day effort.

**Option C — Pillarbox as style.** Accept HUD stays 4:3, draw decorative
bars on the sides.

- Pros: Trivial code change.
- Cons: Violates stage12.md spec ("image must fill monitor"). Wastes
  screen.

### Recommended path

**A** as a first pass. Most panels have natural anchor corners; interior
layout can mostly be preserved. Ship that for 1.0. **B** can come later
if needed.

### Sub-tasks once A is chosen

1. Add `s_HUD_Scale` (probably just a single float — Hor+ UI makes no
   sense, we want uniform HUD scaling equal to `s_YScale`, which is
   already the world scale). Actually this may reduce to just "recognize
   HUD draws and use anchors for them" without adding a new scale.
2. Identify how to distinguish HUD from 3D draws. Options: explicit API
   (`PANEL_draw_quad_anchored(...)`), or a flag on the current API, or
   a separate draw path for HUD that bypasses PolyPage scaling and
   applies anchor math directly in real-pixel space.
3. Go module by module (panel → overlay → frontend → menus → messages)
   assigning anchors.
4. Test per [`testing.md`](testing.md) — verify at 4:3, 16:9, 16:10, 21:9.

---

## 🔴 `s_work_screen_buf` hardcoded to 640×480 (blocker until audited)

### File / lines

[`core.cpp:2322-2335`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp#L2322):

```cpp
static UBYTE s_work_screen_buf[640 * 480 * 4];
UBYTE* WorkScreen = s_work_screen_buf;
SLONG WorkScreenDepth = 32;
SLONG WorkScreenHeight = 480;
SLONG WorkScreenWidth = 640 * 4;   // pitch
SLONG WorkScreenPixelWidth = 640;
MFRect WorkWindowRect = { 0, 0, 640, 480, 640, 480 };
UBYTE CurrentPalette[256 * 3] = {};
```

### Why it's a concern

This buffer was a D3D6-era locked-backbuffer staging area. On OpenGL the
per-frame `glReadPixels`+writeback path is gone (Stage 3 rewrite),
replaced by GPU passes. But the buffer + globals still exist and may
still be touched by some consumer. If a caller writes pixels assuming
`WorkScreenPixelWidth × WorkScreenHeight == RealDisplay*`, it will
either overflow the fixed 640×480 buffer or silently write to the wrong
offsets on widescreen.

### Action items

1. **Audit callers.** Grep for `LockWorkScreen`, `UnlockWorkScreen`,
   `WorkScreen`, `WorkScreenWidth`, `WorkScreenHeight`,
   `WorkScreenPixelWidth`, `WorkWindowRect`. For each hit, determine
   whether the consumer is live or dead (e.g. an `#if 0`-ed block,
   or a function no-one calls now).
2. **If only the loading screen** (via `ge_init_back_image`) uses it —
   document in a comment that the buffer is legacy-only at a fixed
   640×480 resolution, and close this ticket.
3. **If other live consumers exist** — pick:
   - (a) Allocate the buffer dynamically at
     `RealDisplayWidth × RealDisplayHeight × 4` on mode change. Needs
     lifecycle hook for mode-change events.
   - (b) Allocate once at a worst-case size (e.g. 4K = 33 MB). Simple,
     wastes memory at low resolutions.
   - (c) Rewrite the consumers to not use the locked-buffer paradigm
     (usually means a GPU pass).
4. **`WorkWindowRect`** — if consumed, update its layout to match real
   dimensions.

---

## 🟡 Wibble amplitude doesn't scale with resolution

### Symptom

At 1920×1080 (or higher) the water reflection distortion is visibly
much weaker than on the original 640×480. At 4K it's nearly invisible.
Bbox position is fixed (recent commit); only amplitude remains.

### Root cause

[`wibble_frag.glsl:51-52`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/wibble_frag.glsl#L51):

```glsl
offset = sin(angle1) * u_wibble_s1 * AMP_SCALE;  // AMP_SCALE = 1/8 pixels
```

`u_wibble_s1` is a byte in the original 640×480 pixel space. The shader
uses it as a literal pixel offset. A value like `s1 = 16` means `±2 px`
amplitude regardless of framebuffer size. On 640×480 that's a noticeable
wave; on 1920×1080 the puddle is 2.25× larger on screen, so the same 2 px
amplitude is 2.25× smaller relative to the puddle → barely visible.

### Fix

Scale amplitude by the same `RealH / DisplayHeight` factor we use for
the bbox, in
[`wibble.cpp`](../../new_game/src/engine/graphics/postprocess/wibble.cpp)
just before calling `ge_apply_wibble`:

```cpp
const float scale = float(RealDisplayHeight) / float(DisplayHeight);
UBYTE scaled_s1 = (UBYTE)MIN(255.0f, float(wibble_s1) * scale);
UBYTE scaled_s2 = (UBYTE)MIN(255.0f, float(wibble_s2) * scale);
// pass scaled values to ge_apply_wibble
```

Caveats:
- `wibble_s1/s2` are `UBYTE` (0..255). At 4K (scale ≈ 4.5) the upper end
  will clamp. If that's visibly capped, either:
  - Pass the scaled value through the full float path — the shader
    uniform is already `float`, so the 255 limit is only in our CPU
    intermediate.
  - Add a `u_amp_scale` uniform and do the scaling in the shader, leaving
    `u_wibble_s1` as the un-scaled byte.
- `max_shift` in
  [`wibble_effect.cpp:171`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.cpp#L171)
  is computed from `s1 + s2` — it will scale automatically if we scale
  the inputs on the CPU side. Sanity-check that the scratch-texture
  padding keeps up.

---

## 🟢 Focus callback cursor show/hide breaks linker (low priority)

### File

[`host.cpp`](../../new_game/src/engine/platform/host.cpp) —
`on_focus_gained` / `on_focus_lost`.

### State

`sdl3_hide_cursor()` / `sdl3_show_cursor()` calls are present, gated on
`ge_is_fullscreen()`. The build fails at link time when these are
compiled in; specific unresolved symbol not yet diagnosed.

### Why it's low priority

Defensive code only. Testing showed SDL already handles alt-tab cursor
visibility correctly on its own (the cursor comes back when focus
leaves, disappears when focus returns), so these callbacks aren't
functionally needed.

### Options

1. Delete the callbacks — simplest, behavior is fine without them.
2. Run a verbose build (`make build-release V=1` or similar) to see the
   actual unresolved-symbol message from the linker; likely a missing
   include or a symbol that's only declared in one TU. Fix the root
   cause and keep the defensive code.

User has explicitly restored this code after a prior revert — do not
remove it again without asking.

---

## 🟡 Cutscene video #3 cropped vertically on widescreen

### Symptom

Of the three intro / cutscene videos:
- Videos #1 and #2 — render correctly (4:3 with black side bars on
  widescreen, as intended).
- Video #3 — appears to be **cropped on the top/bottom** when the screen
  is wider than the source aspect ratio, instead of being letterboxed
  with side bars like #1 and #2.

### When to investigate

After UI rework. The video pipeline has its own aspect-handling code
(verified working for #1 and #2), so #3 is likely a content-specific
or per-video parameter difference, not a global pipeline issue. Compare
how the three videos are loaded / submitted; look for differences in
source dimensions or aspect-ratio overrides.

---

## 🟡 To verify (not yet tested)

### Outro / cutscenes

Separate render path with its own projection (`OS_cam_aspect` etc.).
Screen-size globals
([`outro_os_globals.cpp:34-37`](../../new_game/src/outro/core/outro_os_globals.cpp#L34))
correctly read `ge_get_screen_width/height()` (RealDisplay), but the
camera / skew math in `outro_os.cpp` and `outro_cam.cpp` hasn't been
audited for aspect handling. Either play the outro at a widescreen
resolution and watch for distortion, or grep those files for any 640 /
480 / 0.75 / 4.0/3.0 aspect literals.

### Frontend fade curtain (RESOLVED)

`FRONTEND_init_xition` and `FRONTEND_show_xition` were updated to compute
in framed-area pixel coords (`ui_coords::g_frame_w_px / g_frame_h_px`)
instead of `RealDisplayWidth/Height`, and `ge_blit_surface_to_backbuffer`
now takes framed-area input coords. Wipe and iris transitions stay
inside the framed region. The `if (RealDisplayWidth == 640)` branch was
dropped — the framed-coord version handles all resolutions uniformly.

### Menus / widgets

[`widget.cpp`](../../new_game/src/ui/menus/widget.cpp) uses
`sdl3_get_global_mouse_pos()` and subtracts window position to get
window-local coords. No hardcoded resolution found in an initial scan,
but menus draw through the same HUD path — they'll share the HUD
pillarbox issue and get fixed together with Option A.

---

## Resolved

- ~~**Fullscreen rendered only a 1920×1080 region in the bottom-left of
  higher-res monitors.**~~ Fixed in `sdl3_window_create` by explicitly
  querying `SDL_GetDesktopDisplayMode()` and forcing the fullscreen mode
  to NULL (borderless-desktop) after creation. SDL3 was taking the
  passed `(width, height)` as an exclusive-mode request.
- ~~**OS cursor was visible over the fullscreen game.**~~ `SDL_HideCursor()`
  is called in `sdl3_window_create` when fullscreen is requested. SDL
  restores the OS cursor automatically when focus leaves the window
  (alt-tab etc.) and re-hides it on focus return — no explicit handling
  needed in the common path.
- ~~**Water wibble applied to wrong area on non-4:3 resolutions.**~~ Fixed
  in
  [`wibble.cpp`](../../new_game/src/engine/graphics/postprocess/wibble.cpp)
  — bbox scaling was non-uniform (X by `RealW/640`, Y by `RealH/480`),
  which only coincides with the rendered puddle when monitor aspect is
  4:3. Changed both axes to scale by `RealH/480`, matching the uniform
  `s_YScale` used for the world. Wibble amplitude scaling still open —
  see above.
- ~~**3D world stretched on widescreen.**~~ Fixed via Hor+ projection in
  [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp):
  `POLY_screen_width = DisplayHeight × RealW/RealH`, and uniform
  `PolyPage::SetScaling(RealH/480, RealH/480)` in
  [`game.cpp`](../../new_game/src/game/game.cpp).
- ~~**Render scale + native physical pixels across platforms.**~~ Scene
  now renders into an offscreen FBO sized `physical_window × scale`,
  and a composition pass upscales it to the window at frame end. Lets
  the user trade sharpness for GPU cost on 4K / Retina / Steam Deck
  without changing the window size. On every platform, the config
  size (`OC_WINDOWED_WIDTH/HEIGHT`) and the FBO size are both in
  **physical pixels**, so a request for 1920×1080 actually means
  1920×1080 pixels even on a Retina Mac.

  **Files:**
  - Config: [`config.h`](../../new_game/src/config.h) —
    `OC_RENDER_SCALE` (0.25..1.0, default 1.0),
    `OC_RENDER_SCALE_MIN` (floor guard),
    `OC_AA_ENABLE` (default true, graphics-quality toggle).
  - [`sdl3_bridge.cpp`](../../new_game/src/engine/platform/sdl3_bridge.cpp)
    — adds `SDL_WINDOW_HIGH_PIXEL_DENSITY` and does the HiDPI window
    resize (see ⚠️ below).
  - [`gl_context.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/common/gl_context.cpp)
    — `glEnable(GL_MULTISAMPLE)` + MSAA attribute queries **removed**;
    default framebuffer is single-sample now.
  - New module:
    [`backend_opengl/postprocess/composition.{h,cpp}`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp)
    — owns scene FBO (color texture RGBA8 + depth texture
    DEPTH_COMPONENT24), shader program, fullscreen-quad VAO, and
    `composition_blit()` (runs FXAA + upscale).
  - New shader:
    [`shaders/composite_frag.glsl`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/composite_frag.glsl)
    — simplified FXAA (one offset sample, no edge-span search — see
    "🟣 Render scale follow-ups" for the upgrade plan) + bilinear
    upscale via `GL_LINEAR` sampling.
  - [`core.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)
    — `OpenDisplay` allocates FBO at `physical × scale` and sets
    `RealDisplayWidth/Height` to that; `ge_begin_scene` /
    `ge_end_scene` / `ge_flip` / `ge_blit_back_buffer` /
    `ge_video_draw_and_swap` / `CloseDisplay` wired to composition.
  - CMakeLists: new shader added to `embed_shaders.cmake`, new
    `composition.cpp` added to `BACKEND_SOURCES`.

  **Invariant — don't break it:** the scene FBO is kept bound as the
  active draw framebuffer **for the entire lifetime of the GL
  context**, not just inside `ge_begin_scene`/`ge_end_scene`. It's
  bound once in `OpenDisplay` and re-bound after every present
  (`ge_flip`, `ge_blit_back_buffer`, `ge_video_draw_and_swap`). This
  is because several game paths call `ge_clear()` **before** the
  frame's `ge_begin_scene` (e.g. `AENG_clear_viewport` fires before
  `POLY_begin`). If the scene FBO isn't already bound at that point,
  the clear hits the window's default FB and the scene FBO keeps
  last frame's contents — symptom was heavy smear / ghosting in menus.
  Do not "simplify" this to symmetrical begin_scene-bind /
  end_scene-unbind — same bug comes back.

  **⚠️ HiDPI window sizing — two-pass resize needed.** On Retina macOS
  `SDL_CreateWindow` takes logical points (not pixels) even with
  `HIGH_PIXEL_DENSITY`, so a 640-point window becomes 1280 physical
  px — double what the config asked for. The naïve fix,
  `SDL_GetDisplayContentScale`, does **not** help: that function
  returns the OS accessibility text-scale (usually 1.0 even on
  Retina), not the Retina pixel ratio. The working approach in
  `sdl3_window_create` is two-pass:
  1. `SDL_CreateWindow` with the config's width/height (treated by
     SDL as logical points);
  2. query `SDL_GetWindowPixelDensity(window)` on the created
     window — the real Retina ratio;
  3. if density > 1, `SDL_SetWindowSize(window,
     config / density, ...)` and re-center.
  Don't revert this to single-pass `SDL_GetDisplayContentScale`-based
  conversion — it looks "cleaner" but silently doubles the window on
  Retina.

  **Mouse mapping unchanged.** `game_tick.cpp:3446` uses logical
  points from `sdl3_window_get_size` and maps to virtual 640×480
  via `DisplayWidth` ratio — independent of render scale and pixel
  density (it's a ratio-based mapping). Didn't need to touch.

  **Out of scope:** dynamic resolution change (`SetDisplay` still
  TODO), UI separation from scene FBO (still in scaled FBO, see
  "🟣 Render scale follow-ups"), high-quality AA (current FXAA is a
  stand-in).
