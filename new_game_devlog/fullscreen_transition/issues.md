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

## 🟣 Render scale follow-ups — resolved or moved out

Render scale + scene FBO + composition pass are implemented (see
"Resolved" at the bottom for the full summary).

- ~~**Split UI from main render.**~~ **DONE.** HUD / menu / overlay /
  text / console / debug overlays now render in a post-composition UI
  pass directly on the default framebuffer at native resolution,
  untouched by FXAA / bilinear upscale. Scene still goes
  scene-FBO → composition → default FB; UI draws on top.
  Implementation history: [`split_ui_from_scene_plan.md`](split_ui_from_scene_plan.md).
  (Possible stragglers — UI draws still living inside scene FBO — can
  be hunted with `OC_DEBUG_HIGHLIGHT_NON_UI = true`; main UI is
  confirmed clean post-Stage 12.)
- **Replace the simplified FXAA with a proper shader AA.** Moved to
  global issue list (not fullscreen-transition specific) →
  [`new_game_planning/known_issues_and_bugs.md`](../../new_game_planning/known_issues_and_bugs.md)
  "Composition AA — заменить упрощённый FXAA…". Doesn't gate 1.0.

---

## 🟡 HUD / UI layout — pillarboxed and small (partial)

### Status

**Resolved for** main menu, options, save/load, mission briefing, pause,
score, loading screen, mission map, transitions, attract mode background
— all centered in a 4:3 framed region with black bars (Stage 3a in
[`ui_coords_plan.md`](ui_coords_plan.md)).

**Resolved for the bottom-row in-game HUD** (radar / health / weapon /
ammo / beacons / crime-rate / grenade / mission-timer inside panel /
speech-and-message stack / weapon-switch popup) — see "HUD bottom-row
anchors" in the Resolved section below. Layout: radar at `LEFT_BOTTOM`
(flush with FBO left edge), message stack at `CENTER_BOTTOM` (centred
between radar and the symmetric right gap with equal rubber padding).
Timer and inventory popup share the radar's `LEFT_BOTTOM` scope so they
align with it. On 4:3 pixel-identical to original; on portrait everything
scales uniformly by `g_frame_scale = ScreenW/640` to fit width; on
widescreen radar/empty stay at the original size pinned to the corners.

**Resolved in subsequent passes:**
- **Top-of-screen mission countdown** — already framed all along.
  `PANEL_draw_timer(time, x, y)` stores `x`/`y` in `PANEL_store` but
  `PANEL_draw_buffered` ignores them and draws at the radar position
  (`m_iPanelXPos + 171, m_iPanelYPos - 118`) inside the radar's
  `LEFT_BOTTOM` scope. The `(320, 50)` argument from `eway.cpp` is
  dead.
- **Search-mode progress bar / bubble / text** — wrapped in
  `UIModeScope(CENTER_CENTER)` in `PANEL_last`. Bubble + bar + text
  now centred on screen on any aspect.
- **Road-sign flashes** — wrapped in `UIModeScope(CENTER_CENTER)` in
  `PANEL_last`. Sign quad stays inside the centred 4:3 framed region;
  `bPanelIsAtBottomOfScreen` keeps placing it at virtual y=100 (top
  half) or y=380 (bottom half) within the frame, mirroring original
  panel-vs-sign positioning. Note: this only fixes the *framing* — a
  separate issue tracks that wrong-way road signs may not actually
  trigger in vehicle missions (see `known_issues_and_bugs.md` Геймплей).

**Still pending** — other in-game HUD elements that weren't touched by
the bottom-row pass:
- **"PSX mode" / version-overlay debug text** — `PANEL_last` ~lines
  2494-2540, drawn at virtual (5, 15) and (20, 20). Debug-only
  overlays; should be `LEFT_TOP`.
- **Kibble / toss animations** — `PANEL_do_tosses()` etc., anchored
  top-centre visually. Needs scope audit.
- **Gun sights, enemy health bars, tracked-enemy markers** — in-world
  overlays rendered through `OVERLAY_draw_*` from
  [`ui/hud/overlay.cpp`](../../new_game/src/ui/hud/overlay.cpp). These
  use world→screen projection, not virtual HUD coords, so they may be
  correct already; verify on wide/portrait.

**Action item — enumerate all remaining HUD draw sites.** Before
starting the next pass, `git grep` the following to build a complete
punch list (so no element ships mis-anchored):
- `FONT2D_DrawString*`, `FONT2D_DrawStringCentred`, `FONT_draw_*`
- `PANEL_draw_*`, `PANEL_new_*`, `PANEL_last_*`, `PANEL_inv_*`
- `OVERLAY_*` (mostly world-space but double-check)
- `DRAW2D_*`
- `CONSOLE_text*`, `MSG_*`
- any literal 320/240 (screen centre virtual coords) or `DisplayWidth/2`
  that doesn't already sit inside a `UIModeScope`.

Each site → classify as `LEFT_BOTTOM` / `CENTER_TOP` / `CENTER_CENTER`
/ etc. per intended anchor; then wrap call site (or function body) in
the scope. Most are one-liner wraps like
`PolyPage::UIModeScope _scope(ui_coords::UIAnchor::...);`.

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

## 🟢 `s_work_screen_buf` 640×480 legacy buffer — resolved by deletion

Audit confirmed there were zero live consumers in `new_game/src`. This
was D3D6-era locked-backbuffer plumbing (DirectDraw `Lock` / `Unlock`
on the back surface, pixel writes, `Unlock` / present) whose original
2D draw routines — `DrawLine`, `DrawBox`, `DrawPxl`, `Sprites`,
`QuickTxt` — were all replaced by the OpenGL poly pipeline during
Stage 3. Only the declarations and stub functions remained. Removed:

- [`core.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp):
  `s_work_screen_buf[640*480*4]`, globals `WorkScreen`, `WorkScreenDepth`,
  `WorkScreenWidth/Height/PixelWidth`, `WorkWindow`,
  `WorkWindowWidth/Height`, `WorkWindowRect`, `CurrentPalette[]`, plus
  stub functions `LockWorkScreen`, `UnlockWorkScreen`,
  `ShowWorkScreen`, `ClearWorkScreen`.
- [`uc_common.h`](../../new_game/src/engine/platform/uc_common.h):
  corresponding `extern` declarations and the `FLAGS_USE_WORKSCREEN`
  macro.
- [`game.cpp:265`](../../new_game/src/game/game.cpp#L265): `OpenDisplay`
  call no longer ORs `FLAGS_USE_WORKSCREEN` into the flags argument
  (which itself is ignored, but cleaned up for clarity).

Frees ~1.2 MB of BSS and removes an obsolete abstraction layer. No
behaviour change.

---

## 🔴 NIGHT lighting pool overflow at wide/tall FOV — post-1.0

### Symptom

`ASSERT failed: WITHIN(NIGHT_square_free, 1, NIGHT_MAX_SQUARES - 1)` in
[`night.cpp:1247`](../../new_game/src/engine/graphics/lighting/night.cpp#L1247)
(outdoor) or [`:1330`](../../new_game/src/engine/graphics/lighting/night.cpp#L1330)
(inside). Game aborts.

**Repro recipe (confirmed 2026-04-24):**
1. `OC_WINDOWED_WIDTH = 480`, `OC_WINDOWED_HEIGHT = 1920` in
   [`config.h`](../../new_game/src/config.h) (tall portrait window).
2. `OC_FOV_MULTIPLIER = 2.0` (heavily widened FOV).
3. First mission ("cop car circles" — the driving-school-style intro).
4. Look at the moon (sky billboard in the distance), then pitch the
   camera down to the floor. Crash fires during the pitch.

### Root cause

`NIGHT_square_free` is a **`UBYTE`**
([`night_globals.h:29`](../../new_game/src/engine/graphics/lighting/night_globals.h#L29))
indexing a 256-slot pool
([`night.h:128`](../../new_game/src/engine/graphics/lighting/night.h#L128)).
Slot 0 is a sentinel, so 255 squares are actually usable. A normal 4:3
view gamut is ~16×16 = 256 lo-res squares — already at the ceiling.

Widening the lens (FOV multiplier > 1, and/or auto-zoom on narrow
aspects) makes `AENG_calc_gamut` yield more squares, so
`AENG_do_cached_lighting_old` calls `NIGHT_cache_create` more times
per frame than the pool has slots. Eventually `NIGHT_square_free`
walks off to 0 (sentinel) and the next `NIGHT_cache_create` asserts.

### Why a graceful fallback in `NIGHT_cache_create` isn't enough

`NIGHT_cache[x][z]` consumers elsewhere — e.g.
[`aeng.cpp:7286`](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L7286),
[`night.cpp:963`](../../new_game/src/engine/graphics/lighting/night.cpp#L963)
— ASSERT that the slot is non-NULL and then dereference
`NIGHT_square[idx].colour`. If we just skip cache creation on pool
exhaustion, the next frame asserts in one of those consumers instead
(and in Release without ASSERT, dereferences `NIGHT_square[0].colour`
which is NULL → segfault).

Fixing it properly means lifting the architectural 255-cap, not
silently no-oping.

### Fix plan (deferred to post-1.0)

Refactor the pool index type from `UBYTE` to `UWORD` and raise
`NIGHT_MAX_SQUARES` to ~1024 (comfortable margin for any realistic
FOV × aspect combination).

**Touch list:**
- [`night.h`](../../new_game/src/engine/graphics/lighting/night.h):
  `NIGHT_Square::next` UBYTE → UWORD; `NIGHT_MAX_SQUARES 256` → `1024`;
  `NIGHT_cache_destroy(UBYTE)` → `UWORD`.
- [`night_globals.{h,cpp}`](../../new_game/src/engine/graphics/lighting/night_globals.h):
  `NIGHT_square_free` UBYTE → UWORD; `NIGHT_cache[][]` UBYTE → UWORD.
- All call sites passing / receiving these indices — audit for
  `(UBYTE)` casts.

**Save-format constraint — must preserve binary compatibility with
original MuckyFoot quicksave layout.** The project invariant: our
code must read any original save file; the reverse direction
(original game reading our saves) is not required. That means we
can't bump `NIGHT_MAX_SQUARES` in the on-disk format or change
`sizeof(NIGHT_Square)` on disk. Approach:

- Keep the runtime pool at `UWORD` / 1024 slots.
- Save/load in
  [`memory.cpp:1454`](../../new_game/src/missions/memory.cpp#L1454) and
  [`:1615`](../../new_game/src/missions/memory.cpp#L1615) stays **byte-identical**
  to the legacy 256-UBYTE layout — via a conversion buffer that
  narrows UWORD → UBYTE (values ≥ 256 serialise as 0 = NULL). The
  legacy save only carries `flag / lo_map_x / lo_map_z` as useful
  payload anyway; `next` links and `colour*` are junk because
  `NIGHT_cache_recalc()` rebuilds everything on load. Slots with
  runtime index ≥ 256 that get serialised as NULL will simply be
  re-created next frame by `AENG_do_cached_lighting_old`, same way
  they were originally populated. No visible regression.

### Workaround for 1.0

**Do not expose `OC_FOV_MULTIPLIER` as a user-facing runtime setting.**
It stays in [`config.h`](../../new_game/src/config.h) as a compile-time
constant (default `1.0`) and remains available internally (cutscenes,
mechanics), but **no FOV slider ships in the options menu** until the
UWORD refactor lands. Narrow aspects below `OC_FOV_MIN_ASPECT` also
widen the effective gamut via the auto-zoom floor; in practice only
users with rotated / portrait monitors would ever hit the ceiling
without a FOV multiplier, so left as-is for 1.0.

---

## 🟢 Star size resolution-independent — resolved

Stars in `SKY_draw_stars` now scale with resolution instead of being
hard-pinned to 1 real framebuffer pixel. Size = `SLONG(PolyPage::s_YScale)`
clamped to ≥1, so:
- 4:3 640×480: 1 px (unchanged, original behaviour)
- 1080p: ≈2 px
- 4K: ≈4 px

Spread ("plus-shape") offsets also multiply by the same factor so the
shape stays proportional. Change is in
[`sky.cpp`](../../new_game/src/engine/graphics/geometry/sky.cpp):
`star_emit_pixel` takes a `size` parameter (default 1) and emits a
size×size quad; `SKY_draw_stars` passes the per-frame computed scale
factor to every emit.

---

## 🟡 Wibble amplitude doesn't scale with resolution — moved out

Not specifically a fullscreen-transition issue (puddle ripple amplitude
is wrong at every render scale ≠ 1×, including stock 4:3 if the FBO
size differs from 640×480). Moved to global issue list →
[`new_game_planning/known_issues_and_bugs.md`](../../new_game_planning/known_issues_and_bugs.md)
"Wibble amplitude не скейлится с разрешением". Full root-cause / fix
plan kept there.

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

## 🟢 Moon rendering — resolved

Aspect-dependent pop-out (a, d) and FOV-dependent oversize solved by
two fixes in
[`sky.cpp`](../../new_game/src/engine/graphics/geometry/sky.cpp)
`SKY_draw_poly_moon` + `SKY_draw_moon_reflection`:

1. **Bounds check uses real projection-space dimensions.** The visibility
   test compared `mid.X` (output of `POLY_transform`, lives in
   `[0, POLY_screen_width]`) against `DisplayWidth` (= 640 constant).
   On aspects where `POLY_screen_width > 640` the right edge culled
   early, producing the asymmetric yaw-left pop-out (d) and the
   head-on disappearance on ultra-wide (a). Switched to
   `POLY_screen_width` / `POLY_screen_height`.
2. **Moon radius compensates for our FOV adjustments.** Radius was
   `DisplayWidth × 0.15` (fixed 96 px). With `OC_FOV_MULTIPLIER > 1`
   or auto-zoom the scene shrinks but the moon didn't → it grew
   relative to world objects. Now divided by `POLY_our_zoom`
   (= `OC_FOV_MULTIPLIER × auto_zoom`), mirroring the
   `POLY_sprite_scale` compensation used for light flares and rain.

New export: `POLY_our_zoom` in
[`poly.h`](../../new_game/src/engine/graphics/pipeline/poly.h) /
[`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
(was a local `our_zoom` inside `POLY_camera_set`).

**(b) retail wobble — out of scope.** The slight size oscillation on
camera pitch is original-game behaviour (confirmed in retail). Our
earlier amplification of it via aspect scaling is gone with the fix
above; what remains is the subtle retail-baseline wobble, not a bug
we introduced.

**(c) moon reflection through ground — resolved as part of the same
change.** Confirmed by user after the bounds-check + radius fix.

---

## 🟢 Fish-eye at wide aspect ratios — resolved via FOV cap + pillarbox

### Status — resolved

The horizontal FOV is now capped at `OC_FOV_CAP_ASPECT` (default 16:9).
Real aspects wider than the cap render the scene at the cap aspect,
centred horizontally, with black pillarbox bars filling the remaining
real-framebuffer columns. The cap is bypassed at 4:3 / 16:10 / 16:9
(no behaviour change). A companion `OC_FOV_MULTIPLIER` config knob
(default 1.0, no change) lets the user widen/narrow the rectilinear
FOV via the `POLY_cam_lens` path. Both settings live in
[`config.h`](../../new_game/src/config.h) and are intended to move
into a runtime settings menu later.

Files touched:
- [`config.h`](../../new_game/src/config.h) — two new defines with
  documented semantics.
- [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  `POLY_camera_set` — clamps `effective_aspect` to the cap,
  centres `g_viewData.dwX`, divides the incoming `lens` by the
  FOV multiplier.
- [`aeng.cpp`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  `AENG_clear_viewport` — paints the pillar regions black after the
  standard clear, using the exact same `render_x / render_w`
  geometry as `POLY_camera_set` so the bars abut the 3D viewport
  with no gaps.
- [`game_graphics_engine.h`](../../new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h)
  + backend [`core.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)
  — new `ge_fill_rect` helper (scissored `glClear`) used for the
  pillar paint; snapshots scissor + clear-colour so caller state
  is unchanged.

The fish-eye itself is an inherent property of any rectilinear
perspective projection at wide horizontal FOVs, not a bug — see
below for the math. The cap avoids it by refusing to widen the
horizontal FOV past `OC_FOV_CAP_ASPECT`.

### Original issue — background for future reference

### Root cause (confirmed)

Not a bug — it's a fundamental property of any rectilinear (planar)
perspective projection. The game uses rectilinear projection
(`POLY_perspective` in [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)).

Horizontal half-FOV grows as `atan(W/H)` where `W = POLY_screen_width,
H = POLY_screen_height`. Our Hor+ sets `POLY_screen_width = DisplayHeight ×
RealW/RealH`:

| Screen   | Half-FOV horiz | Full-FOV horiz | Appearance |
|----------|----------------|----------------|------------|
| 4:3      | 53.1° (atan 4/3)   | 106.3°     | Original  |
| 16:10    | 58.0°              | 116.0°     | Fine      |
| 16:9     | 60.6°              | 121.3°     | Slight    |
| 21:9     | 66.8°              | 133.6°     | Noticeable|
| 32:9     | 74.3°              | 148.5°     | Strong    |
| 4:1      | 76.0°              | 152.0°     | Extreme   |

Rectilinear projection maps each angular degree at off-axis angles to
progressively more pixels — at the screen edge of a 120° horizontal
FOV, 1° of world angle covers twice as many pixels as 1° near the
centre. No bug: this is `tan()` stretching and is unavoidable without
leaving rectilinear projection.

Vertical FOV is a constant 90° (45° half, `atan(ZCLIP/ZCLIP) = atan(1)`)
since `MATRIX_skew` only skews X — `POLY_screen_height` doesn't enter
the vertical FOV.

### Fix options (pick one, none is free)

1. **Cap horizontal FOV at some cutoff, pillarbox beyond.** E.g. above
   21:9 stop widening `POLY_screen_width` and add black bars on the
   left/right. Keeps the cap on 4:3 and everything up to the cutoff
   unchanged; at ultra-wide the player loses some horizontal screen
   coverage but the image stays natural. **Does not affect 4:3.**
2. **Cap horizontal FOV at 16:9, always pillarbox beyond.** Same idea
   but a tighter cutoff — protects the "looks natural" threshold
   aggressively at the cost of more unused screen on 21:9+.
3. **Cylindrical / barrel projection above a threshold.** More
   invasive — needs a tessellated full-screen shader pass or camera
   rework. Keeps the screen filled at extreme widths without
   fish-eye. Post-1.0 scope.

### Chosen approach

Option 1 — cap at 16:9. See "Status — resolved" above.

---

## 🟢 Tall-aspect zoom / character too large — resolved via auto-zoom + letterbox floor

### Status — resolved

Tall windows (aspect < 4:3) now get an automatic camera zoom-out so
the character's pixel size stays matched to its 4:3-width-equivalent,
with the extra vertical screen space filled by extra sky / ground
instead of pixel-stretching the character. Below `OC_FOV_MIN_ASPECT`
(default 4:3; user-configurable lower if they want the "no bars,
zoom aggressively" range to extend further) the scene is letterboxed
into that floor aspect with black bars top/bottom. Symmetric to the
`OC_FOV_CAP_ASPECT` pillarbox on the ultrawide end.

### Behaviour by zone

| Zone                    | Horizontal FOV           | Character pixel size | Bars |
|-------------------------|--------------------------|----------------------|------|
| `aspect > CAP`          | Capped at CAP            | Scales with RealH    | Left/right pillars |
| `aspect ∈ [4/3, CAP]`   | `atan(aspect)` (Hor+)    | Scales with RealH    | None |
| `aspect ∈ [MIN, 4/3]`   | 4:3 constant (atan(4/3)) | **Scales with RealW**| None — extra vertical fills with world |
| `aspect < MIN`          | 4:3 constant             | **Scales with RealW**| Top/bottom letter |

The character pixel size is continuous through the MIN boundary:
inside the letterbox region we apply the same `base/MIN` zoom-out
that the auto-zoom had at MIN, so the transition isn't a jump.

### Implementation

Primary files:
- [`config.h`](../../new_game/src/config.h) — added
  `OC_FOV_MIN_ASPECT` (default 4:3).
- [`aeng.cpp`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  — both `AENG_lens = …` assignments in `AENG_draw` (splitscreen
  and single-cam paths) now divide by `OC_FOV_MULTIPLIER × auto_zoom`.
  `auto_zoom = base / clamp(real_aspect, MIN, base)` when
  `real_aspect < base`, else 1. Applying it on `AENG_lens` (rather
  than inside `POLY_camera_set`) ensures **both** the rendering
  pipeline and the `AENG_calc_gamut` / `POLY_sphere_visible`
  frustum-culling path see the same widened FOV — applying it only
  in one place produced black cut-outs because gamut culled
  geometry that the widened render path still wanted to draw.
- [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  `POLY_camera_set` — effective aspect clamps to `[MIN, CAP]`;
  uniform "fit scale" overrides `PolyPage::s_XScale/s_YScale` per
  frame so the clamped virtual render rect fits inside the real
  framebuffer (shrinks to the limiting axis, never overshoots);
  `dwX`/`dwY` centre the viewport for both pillar and letter cases;
  `PolyPage::s_XOffset` / `s_YOffset` shift POLY-path vertices into
  the same pixel-space coordinate system as the MM-path characters
  (without this shift, bars-active frames had environment drawn off
  to the left while characters rendered centred — "characters and
  environment had different cameras").
- [`aeng.cpp`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  `AENG_clear_viewport` — paints the pillar + letter bars black
  after the standard clear. Uses the same `render_x / render_y /
  render_w / render_h` formula as `POLY_camera_set` so bars abut
  the 3D viewport with no 1-pixel gaps on odd remainders.
- Sprite / billboard / line / sphere size tracking in
  [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  — `POLY_sprite_scale` (formerly `POLY_SPRITE_SCALE_BASELINE`
  constexpr) is now a file-scope float recomputed per frame in
  `POLY_camera_set` as `(DisplayWidth / 2 / ZCLIP) / (OC_FOV_MULTIPLIER
  × auto_zoom)`. Without this compensation the aspect-auto-zoom and
  user FOV widening made sprites / rain / flares look oversized
  relative to the shrunk scene.

### Gotcha: POLY_cam_lens ≈ 2.25 at baseline

Important when reviewing sprite-scale fixes: the game's default
camera lens (`fc->lens = 0x24000 / 65536 = 2.25`) is **not** a
neutral `1.0`. An early attempt to compensate sprite sizes via
`× POLY_cam_lens` silently inflated every sprite 2.25× at the 4:3
default. The correct approach is to divide by *only* the
adjustments we added on top of the game's own lens
(`OC_FOV_MULTIPLIER × auto_zoom`), leaving the game's baseline and
any cutscene lens changes untouched — the original-game formula
`POLY_screen_mul_x × world/z` is lens-independent, and cutscene
zooms were never expected to change light-sprite sizes.

---

## 🟢 Sprite / rain / line widths — resolved

### Status — resolved

World-sized billboards (light-source glows / flares, moon billboard,
rain drops, bullet-line widths, sphere-to-circle radii, HUD
accuracy rings) are no longer aspect-dependent and also correctly
shrink with any auto-zoom / user FOV-multiplier we apply on top of
the game's lens. They match characters in proportion regardless of
window aspect.

### Pass history (for future-you — trio of bugs, three fixes)

1. **Aspect-scaling (original report)** — sprites grew on wide
   aspects and shrunk on narrow ones because
   `POLY_world_length_to_screen` + `POLY_add_line*` +
   `POLY_get_sphere_circle` all multiplied by `POLY_screen_mul_x`,
   which our Hor+ makes aspect-dependent. **Fix:** introduced
   `POLY_sprite_scale` pinned at the 4:3 design-time value
   `DisplayWidth × 0.5 / POLY_ZCLIP_PLANE`. Same 4:3 output, but
   no longer varies with aspect.
2. **Auto-zoom regression** — after the tall-aspect auto-zoom
   landed, sprites stayed at their 4:3-pinned size while the scene
   got zoomed out, so they read as oversized relative to the
   shrunk character. **Fix:** divide `POLY_sprite_scale` by
   `auto_zoom` so sprites track the scene shrinkage.
3. **FOV-multiplier regression** — same symptom when the user
   bumped `OC_FOV_MULTIPLIER` above 1: scene shrinks, sprites don't.
   **Fix:** also divide by `OC_FOV_MULTIPLIER`. Cutscene lens
   changes (encoded in `fc->lens`) are deliberately **not**
   compensated — the original game never changed sprite size on
   cinematic zoom, and we preserve that.

### Call sites touched

- `POLY_world_length_to_screen` — [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp),
  covers [`sprite.cpp`](../../new_game/src/engine/graphics/geometry/sprite.cpp)
  (all `SPRITE_draw*` incl. bloom light glows + moon), `SHAPE_droplet` / trail
  variants in [`shape.cpp`](../../new_game/src/engine/graphics/geometry/shape.cpp),
  [`pyro.cpp`](../../new_game/src/effects/combat/pyro.cpp) fire sprites,
  and [`panel.cpp`](../../new_game/src/ui/hud/panel.cpp) weapon-accuracy rings.
- `POLY_add_line` / `POLY_add_line_tex_uv` width calc in
  [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  — 3D billboard lines (bullet trails, muzzle lines, spark trails).
- `POLY_get_sphere_circle` radius calc in
  [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  — dev light-editor hit-test.

The perspective-projection uses of `POLY_screen_mul_x` (lines in
`POLY_perspective`) must stay aspect-aware — those weren't touched.

---

## 💡 Ideas — moved out

### CRT / scanline shader as a fullscreen post-process

Not fullscreen-transition specific (post-process composition feature).
Moved to global issue list →
[`new_game_planning/known_issues_and_bugs.md`](../../new_game_planning/known_issues_and_bugs.md)
"CRT / scanline shader как post-process".

---

## 🟡 To verify (not yet tested)

### Outro / cutscenes (RESOLVED — framed)

Outro now renders into the same 4:3 framed region as the rest of the UI
on widescreen, with black bars on the sides matching the original 4:3
layout. See [`ui_coords_plan.md`](ui_coords_plan.md) → "Outro framing"
for the two-level viewport approach used and the per-frame clear that
prevents motion-trail artifacts from outro's sliding-bar transition.

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

- ~~**Pause/Restart/Abandon fade-out cat transition sized wrong on
  non-4:3 aspects.**~~ The iris effect ("кошка" circle shrinking to a
  dot) now describes the full rendered viewport regardless of aspect.
  Fixed in [`panel.cpp`](../../new_game/src/ui/hud/panel.cpp)
  `PANEL_fadeout_draw`: the quad is sized so `cat_disc_diameter ≈
  viewport_diagonal` at full UV visibility — `scale = 2 × diag /
  DisplayWidth` (×2 factor accounts for the cat disc occupying only
  ~half the texture width, `cat_radius ≈ 0.25`), centred inside the
  viewport (`g_viewData.dwX/dwY`), affine save/restored around the
  POLY flush. Verified by user on 4:3, 16:9 and portrait 480×1920.
  Note: this is a point-fix inside `PANEL_fadeout_draw`. The same class
  of "UI formulas need to ignore pillar/letterbox bars" problem exists
  in many other effects and is queued up as a proper architectural
  rework — see
  [`fbo_as_virtual_screen_plan.md`](fbo_as_virtual_screen_plan.md).

- ~~**UI text truncated on narrow / portrait aspects (e.g. "GAME PAUSED"
  rendered as "GAME P" on 480×1920).**~~ Fixed in
  [`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  `POLY_camera_set`: `POLY_screen_clip_right` now uses
  `max(POLY_screen_width, DisplayWidth)` instead of just
  `POLY_screen_width`.
  **Root cause:** `OC_FOV_MIN_ASPECT = 2/3` clamps `effective_aspect` on
  sub-4:3 windows → `POLY_screen_width = 480 × 2/3 = 320` → clip_right
  320. UI draws live in virtual 640 (DisplayWidth); right-half quads had
  all 4 corners above 320 → `POLY_add_quad_fast` treated them as
  fully-offscreen and skipped the draw entirely. **Invariant** to preserve
  in future: `POLY_screen_clip_right` must cover **both** the 3D scene
  (`POLY_screen_width`) and the UI virtual canvas (`DisplayWidth = 640`).
  Same principle would apply to `clip_bottom` if cutscene letterbox ever
  shrinks `POLY_screen_height` below `DisplayHeight`, but not triggered
  yet. Diagnosed via runtime logging of per-quad clip flags through
  `POLY_add_quad_fast`.
- ~~**Pause/won/lost menu darken overlay only covered the 4:3 framed
  region on widescreen / portrait — pillarbox and letterbox bars
  remained lit.**~~ Fixed in
  [`gamemenu.cpp`](../../new_game/src/ui/menus/gamemenu.cpp)
  `GAMEMENU_draw`: wrapped the `PANEL_darken_screen` call in
  `PolyPage::push_fullscreen_ui_mode()` so virtual 640×480 maps across
  the entire real framebuffer. Menu text stays in framed mode
  (`UIModeScope(CENTER_CENTER)`). Darken-sweep animation rate remains
  the original ~640 ms regardless of aspect.
- ~~**Pause-menu darken overlay left a 1-pixel lit line on the right
  (and implicitly bottom) edge of the FBO.**~~ Fixed in
  [`polypage.cpp`](../../new_game/src/engine/graphics/pipeline/polypage.cpp)
  `PolyPage::push_fullscreen_ui_mode`: affine offsets `xo`/`yo` changed
  from `0.0` to `0.5` (physical px) to cancel the D3D6 pixel-center
  `-0.5` subtract applied by the TL vertex shader
  ([`tl_vert.glsl`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/tl_vert.glsl)).
  **Root cause:** virtual `(0..640, 0..480)` mapped to screen
  `(0..fbo_w, 0..fbo_h)`; after the shader's `-0.5` shift the right
  edge landed at `ndc_x = 1 - 1/fbo_w` — one physical pixel short,
  exposing the lit 3D scene through the alpha overlay. The `-0.5`
  shader shift is a global D3D6-compat offset needed to keep text /
  sprite atlases from bleeding, so the compensation lives in the
  fullscreen scope itself rather than in the shader. Applies only to
  `push_fullscreen_ui_mode`; framed/normal UI modes unaffected (their
  right edge sits inside the FBO and doesn't need edge-to-edge
  coverage).
- ~~**Stars misaligned on non-4:3 aspects — "flying like flies" around
  the sky instead of attached to it.**~~ Fixed in
  [`sky.cpp`](../../new_game/src/engine/graphics/geometry/sky.cpp)
  `SKY_draw_stars`: the real-pixel mapping used
  `RealDisplayWidth/DisplayWidth` as the X multiplier, which was correct
  only when `POLY_transform` output lived in `[0, 640]` (pre-Hor+).
  After Hor+ the output lives in `[0, POLY_screen_width]` which is
  aspect-dependent — the old multiplier projected stars onto real
  coords well outside the 3D viewport. Replaced with the same affine
  `PolyPage::s_XScale/s_YScale + s_XOffset/s_YOffset` that
  `PolyPage::AddFan` uses, so stars land in the same pixel space as the
  rest of the scene.
- ~~**Stars hardcoded to 1 real framebuffer pixel — invisible on 1080p+
  displays.**~~ Fixed in
  [`sky.cpp`](../../new_game/src/engine/graphics/geometry/sky.cpp):
  `star_emit_pixel` takes a `size` parameter (default 1) and emits a
  size×size quad; `SKY_draw_stars` passes `SLONG(PolyPage::s_YScale)`
  (≥1) so stars scale with resolution. 4:3 640×480 → 1 px (unchanged
  original behaviour), 1080p ≈ 2 px, 4K ≈ 4 px. Spread ("plus-shape")
  offset also multiplied so the shape stays proportional.
- ~~**`s_work_screen_buf` 640×480 legacy buffer unused in OpenGL backend.**~~
  Audited — zero live consumers. Removed the 1.2 MB buffer, its
  associated globals (`WorkScreen*`, `WorkWindow*`, `WorkWindowRect`,
  `CurrentPalette[]`, `WorkScreenDepth`), four stub functions
  (`LockWorkScreen`, `UnlockWorkScreen`, `ShowWorkScreen`,
  `ClearWorkScreen`), and the `FLAGS_USE_WORKSCREEN` macro with its one
  call-site in `OpenDisplay`. See "`s_work_screen_buf` 640×480 legacy
  buffer — resolved by deletion" section above.
- ~~**Cutscene video #3 (PCINTRO_withsound.bik) cropped on widescreen.**~~
  All three videos now aspect-fit into the 4:3 framed region of the
  screen instead of the full window. Result: video #1 and #2 (640×480)
  fill the framed region exactly with pillarbox bars on the sides, and
  video #3 (640×340 ≈ 16:9) gets letterbox bars top/bottom inside the
  framed region plus pillarbox outside it — stylistically consistent
  with the framed UI. Implemented in
  [`ge_video_draw_and_swap`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)
  by computing aspect-fit against `ui_coords::g_frame_w_px / g_frame_h_px`
  scaled by `frame_w_ndc / frame_h_ndc`. Snapshot/restore scissor around
  video draw so the surrounding UI scope's framed clip doesn't crop the
  video to a tiny square.
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

- ~~**Cut-scene dialogue box (Darci / other speakers) mis-positioned on
  non-4:3 aspects.**~~ Evolution across the session as new aspects were
  tested:
  1. **Left-pinned on widescreen (16:9+).** `PANEL_new_widescreen` drew
     portraits, text and the cinematic top/bottom black bars in virtual
     640×480 coords without any UI-mode scope; default affine anchored
     virtual (0,0) at FBO (0,0), so the 640-wide frame landed at the
     FBO left edge.
  2. **Floating inside the 4:3 region on tall/portrait aspects.**
     Wrapping the function in `UIModeScope(CENTER_CENTER)` centred the
     frame but on windows narrower than 4:3 the cinematic bars drifted
     away from the FBO top/bottom edges into the middle of the screen.
  3. **"Double bar" on tall aspects (UI bar + visible 3D letterbox).**
     Split to `UIModeScope(CENTER_TOP)` for the top bar/content and
     `UIModeScope(CENTER_BOTTOM)` for the bottom, pinning them to the
     FBO edges. Exposed an aspect-mismatch with the 3D viewport's
     cutscene letterbox (`wideify = 80 virtual` in `POLY_camera_set`):
     UI bars scaled by `g_frame_scale` (width-limited on portrait), 3D
     letterbox scaled by `ScreenHeight/480`, so on tall aspects a
     visible black gap appeared between the UI bar and the 3D scene.
  4. **Final — bars flush with 3D letterbox.** Kept the split
     `CENTER_TOP` / `CENTER_BOTTOM` UI scopes, and scaled `wideify` in
     `POLY_camera_set` by `g_frame_scale / (ScreenHeight/480)`. On 4:3
     or wider FBOs the two scales coincide → `wideify = 80` unchanged.
     On tall/portrait FBOs wideify shrinks proportionally so the 3D
     viewport letterbox matches the UI-bar height exactly. Verified by
     user at 1920×480 and 480×1920.

  **Invariant for future edits:** UI bar scale and 3D viewport letterbox
  scale must use the same ratio. If either changes, the other must
  track it — otherwise the "extra black gap" returns on non-4:3 FBOs.
  Virtual sizes (640-wide frame, text-wrap 600, portrait positions,
  bar height 80 virtual) untouched — layout content is tuned to fit
  that box.

- ~~**Tutorial panel (`EWAY_tutorial_string` speech bubble on early
  missions) only darkened the FBO's left 4:3 column.**~~ Same
  root-cause class as the cut-scene dialogue: `PANEL_last` drew
  `PANEL_darken_screen(640)` + text + bubble in virtual 640 coords with
  no scope, so the semi-transparent backdrop only covered the left
  4:3-shaped region. Fixed in
  [`panel.cpp`](../../new_game/src/ui/hud/panel.cpp) with a split
  scope: the darken quad + its immediate `POLY_frame_draw` flush run
  inside `PolyPage::FullscreenUIModeScope` (non-uniform stretch → full
  FBO), then a `UIModeScope(CENTER_CENTER)` wraps the bubble + text so
  they stay framed and don't stretch. Mirrors the existing pattern in
  `GAMEMENU_draw` (fullscreen darken + framed menu text).

- ~~**HUD bottom-row anchors (radar / msg stack / weapon popup / mission
  timer inside panel).**~~ Pre-fix, everything in `PANEL_last`,
  `PANEL_inventory`, and `PANEL_draw_buffered` used virtual 640×480
  coords under the default scope, which scales by
  `uniform_scale = ScreenHeight/480`. On widescreen the panel drifted
  away from the FBO corner; on portrait it bloomed way past the radar
  because the width-limited `g_frame_scale = ScreenW/640` was ignored
  and the panel got `uniform_scale` (much larger on tall aspects).
  Fixed in [`panel.cpp`](../../new_game/src/ui/hud/panel.cpp) with a
  two-anchor layout that matches the user's spec:
  - **Radar / health / weapon / ammo / crime-rate / beacons / grenade /
    panel mission timer (`PANEL_draw_buffered`) / weapon-switch popup
    (`PANEL_inventory`)** → `UIModeScope(LEFT_BOTTOM)`. Flush with the
    FBO bottom-left corner. `PANEL_inventory` and `PANEL_draw_buffered`
    are wrapped at the function body since they're called from
    `OVERLAY_handle` outside `PANEL_last`'s scope.
  - **Speech / radio / on-screen-message stack** (the `PLT_X/PLT_Y`
    block inside `PANEL_last`) → `UIModeScope(CENTER_BOTTOM)`.

  **Why the algebra works.** Virtual radar width = 214, empty-right = 32,
  msg block = 394 (centred at virtual x=411). Under `LEFT_BOTTOM` +
  `CENTER_BOTTOM` + (notional) `RIGHT_BOTTOM`, on any aspect the msg
  block's centre and the gap centre between radar-end and empty-start
  both equal `ScreenW/2 + 91 × g_frame_scale`. Identically centred → one
  scope pair produces the "rubber-padding equal on both sides" layout
  the spec asked for, and on 4:3 all three frames collapse to the same
  origin so the layout is pixel-identical to the original. Verified by
  user with temporary magenta / green debug rects (removed after
  confirmation).

  **Still pending** — top-of-screen mission countdown (virtual (320, 50)
  from `eway.cpp`), road-sign flashes, search bar, PSX / version debug
  overlays. Same `PolyPage::push_ui_mode` infra — only call sites left
  to wrap (mostly `CENTER_TOP`). See "Still pending" in the HUD /
  UI-layout section above.
