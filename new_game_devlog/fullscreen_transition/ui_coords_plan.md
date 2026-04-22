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

### PolyPage UI mode

Today every UI draw eventually hits `PolyPage::AddFan` which multiplies by
`s_XScale = s_YScale = RealH/480`. After this rework, UI call sites
already produce real-pixel coords — they must NOT be re-multiplied.

Add to `polypage.{h,cpp}`:

```cpp
class PolyPage {
    static void push_ui_mode();   // saves s_XScale/s_YScale, sets to 1.0
    static void pop_ui_mode();    // restores
};
```

Wrap every high-level UI draw API (PANEL_*, FONT_*, MSG_*, CONSOLE_*,
debug overlay draws) in push/pop. After the wrap, those APIs **accept
real screen pixels** (semantic flip).

Open question: do we wrap at the top-level draw call (e.g. inside
`PANEL_draw_quad` itself) so call sites don't need to know, or at the
caller's loop boundary (one push/pop per frame's UI block)? Default plan
is **at the top-level draw call** — minimizes call-site churn at the
cost of redundant push/pops within a frame. Revisit if profiling shows
overhead.

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

### Stage 4 — black bars

For framed mode the area outside the 4:3 frame must be black. Two
approaches:
- (a) `glClear(black)` once per frame before any UI draw. Simplest. May
  conflict with pause backdrop which currently draws 3D-ish background.
- (b) Draw two black quads (left+right or top+bottom) in the gap.

Pick during stage 4 after seeing what menus actually do.

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

### Stage 3a — Frontend / menus

**Deliverables:**
- All hardcoded pixel calls in `frontend.cpp` / `widget.cpp` routed
  through `old_px_to_screen_pixels(..., CENTER_CENTER)`.
- High-level draw funcs they call wrapped in PolyPage UI mode.

**Acceptance:**
- Builds clean.
- At 1920×1080: main menu and pause menu fill a centered 4:3 region with
  black bars on the sides. Layout looks like 4:3 boxed inside a 16:9
  screen, NOT a pillarboxed mess in the bottom-left.
- At 640×480: pixel-perfect identical to before this stage.

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
   `ui_coords`, `framed`, `anchor`, `PolyPage UI mode` — the latest one
   tells you which stage was last completed.
4. Resume at the next stage. Each stage has clear acceptance criteria
   above; don't move on until the current stage's acceptance holds.
5. **Do not** silently change the design choices in this doc. If you
   need to, write the new decision into this file with a note explaining
   why — don't just diverge.
"