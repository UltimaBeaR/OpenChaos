# Known issues & fix plans

Read [`concepts.md`](concepts.md) first for terminology.

---

## 🔴 HUD / UI layout — pillarboxed and small (blocker)

### Symptoms

On non-4:3 resolutions (e.g. 1920×1080, 2560×1080):
- All HUD elements — health bar, radar/compass, weapon indicator, ammo,
  body-part damage, wristwatch, on-screen messages — live in the **left
  1440×1080 region** of the 1920×1080 framebuffer. Right ~480 px shows
  background/empty space.
- Elements look small: a UI panel 200 px wide in virtual 640×480 space
  becomes 450 px on screen — fine on a laptop, tiny on a 27" 1080p.
- Text rendered through the HUD path shares this scaling.
- Main menu, pause menu, options dialogs, mission briefing, attract mode,
  save/load — same issue (they share the HUD layout pipeline).

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

### Frontend fade curtain

[`frontend.cpp:397-402`](../../new_game/src/ui/frontend/frontend.cpp#L397)
has an `if (RealDisplayWidth == 640)` branch; at `:1794` a switch on
`RealDisplayWidth` with preset sizes. These work by accident on modern
resolutions but the literal 640 is a marker that the original code
didn't envision arbitrary resolution. Likely fine to leave as part of
the broader HUD pass (Option A above); flag during that pass.

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
