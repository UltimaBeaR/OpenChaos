# FBO-as-virtual-screen refactor — full handoff

Status: proposed, not started. This document is the **authoritative
spec**. If you're implementing, treat every "must / never / always" as
binding. Assume nothing. If something here is unclear or contradicts
reality when you start coding, stop and ask — don't improvise.

---

## Core principle

Read this section first. Everything else in the document is
mechanics; this is the intent. If any proposed change contradicts
this principle, the principle wins.

**The game is written as if the scene FBO is the entire screen.**

- The game never reads, asks for, or depends on the real window's
  size, position, aspect ratio, or DPI. Not directly. Not indirectly.
  Not through a helper. Never.
- Everything the game calls "screen width / height / aspect / coords"
  refers to the scene FBO. The FBO is a plain rectangle the game
  fully owns, and the game's entire coordinate system is defined
  inside it.
- When the user configures a window aspect the game cannot render at
  (outside `[OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT]`), the FBO is
  made smaller than the real window — narrower in one dimension to
  stay in the allowed aspect range. The game does not know or care
  that this happened. It still sees its own "screen"; that screen
  just has different pixel dimensions than someone else might expect.

**The real window is exclusively a composition concern.**

- Exactly one layer of code — the composition pass at end-of-frame
  — knows the real window size. It reads the FBO (the game's
  output), centres it in the real window, blits it with an
  aspect-preserving scale, and paints anything outside that blit
  rectangle black. That black border exists **only** in the
  composition layer's brief awareness; the game has no representation
  of it at all.
- The only other exception is mouse-event mapping, which must map a
  real-window-pixel position back to the game's FBO-pixel position.
  One function. Every other game-side consumer of mouse input reads
  FBO coords, not real-window coords.

**Why this principle.**

- Every full-screen effect — current (fade-out cat, menu darken,
  stars, moon) and future (SSAO, bloom, colour grading, screen-space
  fog, new transitions) — can be written as if there were only one
  screen, without knowing about bars. They allocate their surfaces
  at FBO size, sample at FBO coords, draw at FBO coords. Composition
  handles bars once, in one place.
- There is one bar-handling layer, not N. Adding a new effect costs
  you exactly zero bar-awareness plumbing.
- The game's coordinate system is stable and explainable: "pixels
  from (0, 0) to (ScreenWidth, ScreenHeight), no exceptions, no
  offsets, no bars". Reviewers and future authors don't have to hold
  a mental model of the real window at all.

**What this principle does not say.**

- It does not eliminate the game's own concept of 4:3 framed UI.
  Menus, pause screens, frontend, briefing, etc. still render inside
  a 4:3 region centred inside the FBO, with the game drawing its
  own bars around that region when the FBO is not 4:3. That's a
  *game UI style* decision, not an architecture leak — it lives
  strictly inside the FBO and is done by game code at UI-draw time,
  using the same drawing primitives as every other game draw. It
  has no connection to the real window.

Everything below is mechanics that implement this principle. Keep
coming back here if you get lost.

---

## Table of contents

0. [Core principle](#core-principle) *(read first)*
1. [TL;DR](#tldr)
2. [Motivation](#motivation)
3. [Current architecture (as of now)](#current-architecture-as-of-now)
4. [Target architecture](#target-architecture)
5. [Invariants (load-bearing, do not violate)](#invariants-load-bearing-do-not-violate)
6. [FBO sizing rule](#fbo-sizing-rule)
7. [Composition blit](#composition-blit)
8. [Outer bars vs inner bars (two independent layers)](#outer-bars-vs-inner-bars-two-independent-layers)
9. [Post-process effect pipeline (SSAO, bloom, future effects)](#post-process-effect-pipeline-ssao-bloom-future-effects)
10. [Input handling](#input-handling)
11. [Rename pass](#rename-pass)
12. [File-by-file changes](#file-by-file-changes)
13. [Code to delete](#code-to-delete)
14. [Per-effect cleanup opportunities](#per-effect-cleanup-opportunities)
15. [Edge cases & gotchas](#edge-cases--gotchas)
16. [Staged implementation plan](#staged-implementation-plan)
17. [Testing checklist](#testing-checklist)
18. [Rollback](#rollback)
19. [Out of scope](#out-of-scope)
20. [Open questions](#open-questions)

---

## TL;DR

Make the scene FBO function as "the screen" from the game's point of
view. The game never reads the real window size. Artificial pillar /
letterbox bars needed when physical aspect is outside the allowed
range are painted by the **composition layer** only, when it blits
the FBO into the real backbuffer. Everything else in the engine
treats the FBO as an opaque rectangle it fully owns.

In addition, the game has a **second, independent** concept of 4:3
framed UI that lives strictly inside the FBO (not the real window).
When active, it draws its own inner bars inside the FBO. Composition
doesn't care about these; the game draws them just like any other UI
element.

After this refactor, per-effect "remember the black bars exist and
offset my quads" code disappears: there's one place that handles
aspect bars (composition), one place that handles framed UI
(`push_ui_mode` — stays as-is semantically but its coordinate space
is now the FBO, not the real window).

Expected commit footprint: 15–25 files, mostly small edits, one
larger edit in `poly.cpp`. Estimated 1–2 days for someone familiar
with the renderer.

---

## Motivation

The current architecture lets "real window size" leak into game
coordinate math. Every time we add a full-screen effect, we hit the
same class of bug: the effect draws relative to the real framebuffer
and spills onto (or leaves gaps in) the pillar/letterbox bars. We
have fixed this ad-hoc three times already:

- Pause menu darken — wrapped in `push_fullscreen_ui_mode` + custom
  animation rate scaling.
- Pause fade-out cat — overrides `PolyPage::s_X/YScale/Offset`
  manually, uses `g_viewData` viewport rectangle, has a magic ×2
  multiplier to compensate for the texture's cat-disc radius.
- Stars — real-pixel mapping moved from
  `RealDisplayWidth/DisplayWidth` to `PolyPage::s_XScale/XOffset`
  because the first broke after Hor+.
- Menu darken anim rate — had to hand-tune so it doesn't race across
  wide windows.

Every future full-screen effect (SSAO, bloom, screen-space fog, any
post-process filter, new UI transitions) will have to re-learn this.
That's unsustainable. One-place fix: make "the screen" be the FBO
for the game, and make bars a pure composition-layer concern.

The fade-out cat fix is the most recent example. The fix works, but
it's a 40-line ritual of save/override/restore affine with a magic
×2 multiplier. All of that goes away after this refactor — it becomes
a plain `(0, 0)–(ScreenWidth, ScreenHeight)` quad in ordinary UI
coords.

---

## Current architecture (as of now)

Rendering flow:

1. **`sdl3_window_create`** — creates the real window, physical pixel
   size = `OC_WINDOWED_WIDTH × OC_WINDOWED_HEIGHT` (or desktop
   resolution in fullscreen). HiDPI handled via two-pass resize.
2. **`OpenDisplay`** — queries physical drawable size, multiplies by
   `OC_RENDER_SCALE` to get FBO size. Creates scene FBO at that size.
   Sets `RealDisplayWidth/Height` = FBO size.
3. **`POLY_camera_set`** (called per 3D frame) — computes
   `effective_aspect` = `clamp(real_aspect, OC_FOV_MIN_ASPECT,
   OC_FOV_CAP_ASPECT)`, then `POLY_screen_width = DisplayHeight *
   effective_aspect`. Computes `base_x / base_y` as the centring
   offset for the clamped region inside the real framebuffer. Calls
   `ge_set_viewport(base_x, base_y, render_w, render_h)` — viewport
   is the "allowed aspect region" centred in the real framebuffer.
   `PolyPage::s_XScale/YScale` set to `fit_scale` (uniform); `s_X/
   YOffset` set to the centring offsets.
4. **`AENG_clear_viewport`** — clears the viewport, then paints the
   left-over pillar/letterbox bars black using `ge_fill_rect` on the
   real framebuffer outside the viewport rectangle.
5. **Game draws** — 3D scene, HUD, UI, effects, all using
   `RealDisplayWidth/Height` as "the screen". Some effects use
   `ui_coords::g_real_*`. Some use `g_viewData.dw*` (viewport).
   Some use `PolyPage::s_X/YScale/Offset`. No single source of truth.
6. **`ge_flip` / composition** — FXAA + bilinear upscale blit of FBO
   into the real backbuffer, sized to match the window.

Concepts currently in use and how they relate:

| Symbol | Meaning today | Who reads it |
|--------|---------------|--------------|
| `DisplayWidth / DisplayHeight` | Virtual 4:3 design canvas, constant 640×480 | Everyone |
| `RealDisplayWidth / RealDisplayHeight` | Scene FBO size (physical×scale) | Everyone (often wrongly) |
| `ui_coords::g_real_w_px / g_real_h_px` | Scene FBO size | UI layer |
| `ui_coords::g_frame_w_px / g_frame_h_px` | 4:3 centred inside the FBO | UI layer |
| `POLY_screen_width / POLY_screen_height` | 3D scene viewport size in virtual-projection units | Renderer |
| `g_viewData.dwWidth / dwHeight / dwX / dwY` | Viewport rectangle inside the real framebuffer | Some effects |
| `PolyPage::s_X/YScale / s_X/YOffset` | Affine from virtual 640×480 to real framebuffer pixels | `PolyPage::AddFan` |

The problem: **the real window's existence leaks into the game** at
level 3–5. Every full-screen-capable drawing surface needs to
special-case the bars.

---

## Target architecture

**Two independent layers of black bars** — keep them straight in
your head at all times:

```
     ┌──────────── real window / backbuffer ─────────────┐
     │                                                   │
     │ OUTER bar          ┌──── scene FBO ────┐  OUTER   │
     │ (composition,      │                   │   bar    │
     │  physical aspect   │    GAME SEES      │          │
     │  outside CAP/MIN)  │    ONLY THIS      │          │
     │                    │    SURFACE        │          │
     │                    │                   │          │
     │                    │  ┌─INNER bar─┐    │          │
     │                    │  │ (game,    │    │          │
     │                    │  │  drawn    │    │          │
     │                    │  │  when     │    │          │
     │                    │  │  framed   │    │          │
     │                    │  │  UI       │    │          │
     │                    │  │  active   │    │          │
     │                    │  │  + FBO    │    │          │
     │                    │  │  is not   │    │          │
     │                    │  │  4:3)     │    │          │
     │                    │  └───────────┘    │          │
     │                    │                   │          │
     │                    └───────────────────┘          │
     │                                                   │
     └───────────────────────────────────────────────────┘
```

- **Outer bars** (between real window edge and FBO edge) — painted by
  the composition layer only. Appear iff physical window aspect is
  outside `[OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT]`. Game is blind to
  these; they are not in any coordinate space the game operates in.
- **Inner bars** (between FBO edge and the 4:3 framed UI region) —
  painted by the game itself, inside the FBO, as ordinary UI draws.
  Appear iff **(a)** the FBO aspect is not 4:3 **and (b)** a framed
  UI scope is active (menu / pause / frontend / briefing / etc.).
  HUD call sites that anchor to FBO corners do *not* want inner
  bars. Inner bars are the **game's choice**, not forced by the
  architecture.

From composition's point of view, the FBO is an opaque blit source.
From the game's point of view, the FBO *is* the screen, and 4:3
framed UI is a strictly inner concept.

---

## Invariants (load-bearing, do not violate)

These are the properties of the architecture that the refactor is
buying us. If you break any, you are regressing to the current mess.

### I1. The game never reads the real window size.

- No `new_game/src/**/*.{cpp,h}` file outside
  `engine/graphics/graphics_engine/backend_opengl/postprocess/composition.*`
  and `engine/platform/sdl3_bridge.*` may reference the real window
  size by any means.
- The SDL bridge may still expose the real window size because input
  events come in real window coordinates — but only the composition
  layer and the mouse-mapping code (a single well-defined function)
  may read it.
- **Debug aid for implementers**: after the refactor, grep for
  `sdl3_window_get_size`, `sdl3_window_get_drawable_size`,
  `SDL_GetWindowSize`, `SDL_GetWindowSizeInPixels`,
  `gl_context_get_width`, `gl_context_get_height`. Any non-
  composition, non-input-mapping usage is a bug.

### I2. Everything the game calls "the screen" is the FBO.

- `ScreenWidth / ScreenHeight` (renamed from `RealDisplayWidth/Height`)
  equal FBO size. Game UI, effects, post-process surfaces, scissor
  rects, viewport rects — all operate in FBO pixel space.
- 3D projection math uses these as the framebuffer it's rendering
  into. POLY_screen_width / POLY_screen_mul_x etc. are derived from
  them.

### I3. Outer bars are invisible to the game.

- The composition layer knows "real backbuffer width / height" and
  "FBO width / height". It handles the difference.
- Game code never sees outer-bar pixel coordinates. If game code ever
  produces a pixel coordinate outside the FBO, that's a bug in the
  game code — nothing catches it for the game.

### I4. Inner bars are ordinary game draws.

- When a framed UI scope is active and FBO aspect ≠ 4:3, the bars
  outside the framed 4:3 region (but inside the FBO) are painted by
  the game like any other UI rect. They use the same page
  infrastructure, the same blend rules, etc.
- They are scissored the same way as any other game draw (inside
  the FBO).

### I5. Full-screen effects cover the whole FBO.

- Full-screen effects (SSAO, bloom, colour grading, fade-out
  transitions, menu darken) cover the entire FBO — from `(0, 0)` to
  `(ScreenWidth, ScreenHeight)`.
- They do not know and do not care whether a framed UI scope is
  active. If the framed UI wants its 4:3 region to sit on top of the
  effect (e.g. menu darken behind the framed menu text), the menu
  code handles ordering: draw effect first, draw framed UI on top.

### I6. FBO aspect is clamped at creation, not per-frame.

- The aspect clamp (`OC_FOV_MIN_ASPECT ≤ fbo_w / fbo_h ≤
  OC_FOV_CAP_ASPECT`) happens once, when the FBO is allocated.
- `POLY_camera_set` no longer re-applies the clamp per frame; it
  uses the FBO aspect as-is.

### I7. Render scale and aspect clamp compose.

- `OC_RENDER_SCALE` scales the FBO dimensions uniformly; aspect
  clamp shapes them to the allowed range. Order of operations:
  1. Start from physical window pixels.
  2. Multiply by render scale.
  3. Clamp aspect to `[MIN, CAP]`.
- Both operations reduce FBO size; they compose without special
  cases.

---

## FBO sizing rule

The exact formula used by `OpenDisplay`:

```cpp
// Inputs:
//   phys_w, phys_h         — physical window drawable size (HiDPI-aware).
//   OC_RENDER_SCALE        — config float, clamped to [MIN, 1.0].
//   OC_FOV_CAP_ASPECT      — config float, widest allowed aspect (e.g. 18/9 = 2.0).
//   OC_FOV_MIN_ASPECT      — config float, narrowest allowed aspect (e.g. 2/3 ≈ 0.667).

// Step 1 — apply render scale.
const int scaled_w = int(phys_w * render_scale + 0.5f);
const int scaled_h = int(phys_h * render_scale + 0.5f);
const float scaled_aspect = float(scaled_w) / float(scaled_h);

// Step 2 — clamp aspect. Reduce the dimension that violates the bound.
int fbo_w, fbo_h;
if (scaled_aspect > OC_FOV_CAP_ASPECT) {
    // Too wide → reduce width.
    fbo_h = scaled_h;
    fbo_w = int(float(scaled_h) * OC_FOV_CAP_ASPECT + 0.5f);
} else if (scaled_aspect < OC_FOV_MIN_ASPECT) {
    // Too tall / narrow → reduce height.
    fbo_w = scaled_w;
    fbo_h = int(float(scaled_w) / OC_FOV_MIN_ASPECT + 0.5f);
} else {
    // Aspect already in range.
    fbo_w = scaled_w;
    fbo_h = scaled_h;
}

// Guard against zero / negative.
if (fbo_w < 1) fbo_w = 1;
if (fbo_h < 1) fbo_h = 1;

// Step 3 — allocate scene FBO at (fbo_w, fbo_h). Publish as
// ScreenWidth / ScreenHeight (= renamed RealDisplayWidth/Height).
ScreenWidth  = fbo_w;
ScreenHeight = fbo_h;
```

Numerical examples:

| Physical window | render_scale | Before clamp | Aspect | Clamp? | FBO |
|----------------|--------------|--------------|--------|--------|-----|
| 640×480        | 1.0          | 640×480      | 4:3    | no     | 640×480 |
| 1920×1080      | 1.0          | 1920×1080    | 16:9   | no (2.0 cap, 16:9 = 1.78) | 1920×1080 |
| 1920×1080      | 0.5          | 960×540      | 16:9   | no     | 960×540 |
| 3440×1440      | 1.0          | 3440×1440    | 21:9 ≈ 2.39 | yes (>CAP=2.0) | 2880×1440 |
| 480×1920       | 1.0          | 480×1920     | 1:4 = 0.25 | yes (<MIN=0.667) | 480×720 |
| 2160×3840 (portrait 4K) | 1.0 | 2160×3840  | 9:16 ≈ 0.56 | yes (<MIN=0.667) | 2160×3240 |

---

## Composition blit

Composition runs once per frame, at the end, with inputs:

- **Source**: scene FBO, size `fbo_w × fbo_h` (after any post-process
  effects have run on it).
- **Destination**: default framebuffer (real backbuffer), size
  `real_w × real_h` as reported by `gl_context_get_width/height` or
  equivalent.

Steps the composition pass must perform, **in this order**:

1. Bind default framebuffer. No scene FBO binding leaks past this
   point.
2. If `(fbo_w, fbo_h) != (real_w, real_h)` (i.e. outer bars exist
   anywhere), clear the real backbuffer to **black**
   (`glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT)`).
   Unconditional clear is also acceptable (free on modern GPUs); just
   be consistent. *Post-1.0 this can become a user-chosen colour or
   gradient; for now, black.*
3. Compute target rectangle on the real backbuffer. The FBO is
   blitted into this rectangle, aspect-preserving, scaled to fill as
   much of the real backbuffer as possible while preserving FBO
   aspect:
   ```
   fbo_aspect  = fbo_w / fbo_h
   real_aspect = real_w / real_h
   if real_aspect > fbo_aspect:
       // Real is wider than FBO → pillarbox (bars on left/right).
       dst_h = real_h
       dst_w = int(real_h * fbo_aspect + 0.5)
       dst_x = (real_w - dst_w) / 2
       dst_y = 0
   else if real_aspect < fbo_aspect:
       // Real is taller than FBO → letterbox (bars top/bottom).
       dst_w = real_w
       dst_h = int(real_w / fbo_aspect + 0.5)
       dst_x = 0
       dst_y = (real_h - dst_h) / 2
   else:
       // Same aspect → fill, no bars.
       dst_w = real_w
       dst_h = real_h
       dst_x = 0
       dst_y = 0
   ```
   Note: because of the aspect clamp at FBO creation, `fbo_aspect ∈
   [MIN, CAP]` while `real_aspect` is unconstrained. The `real_aspect
   > fbo_aspect` branch triggers on ultra-wide physical windows; the
   `<` branch on ultra-tall/portrait.
4. Run the FXAA + bilinear upscale pass (current pipeline) to draw
   the FBO into the target rectangle. Use `glViewport(dst_x, dst_y,
   dst_w, dst_h)` and draw a full-screen triangle in viewport coords.
   Scissor disabled during the upscale (viewport is enough; the
   triangle is clipped by NDC bounds).
5. Swap buffers.

Store the composition's `(dst_x, dst_y, dst_w, dst_h, real_w, real_h,
fbo_w, fbo_h)` somewhere **input mapping** can query it. See next
section.

### Render scale + outer bars interaction

When `OC_RENDER_SCALE < 1.0` and the physical aspect is within the
allowed range:
- FBO is smaller than real backbuffer, but same aspect.
- Composition upscales FBO to fill the real backbuffer (`dst_w =
  real_w, dst_h = real_h`). No bars.

When render scale is `1.0` and aspect is outside the allowed range:
- FBO equals clamped physical size, narrower aspect than real window.
- Composition blits FBO at 1:1 in the centre, bars fill the rest.

When render scale < 1.0 **and** aspect outside range:
- FBO equals `phys × render_scale`, clamped further by aspect.
- Composition scales FBO up to the largest aspect-preserving rect
  that fits in real backbuffer, bars fill the rest.

All three cases handled by the same formula above; no special cases
in code.

---

## Outer bars vs inner bars (two independent layers)

A re-statement, because this is the most-likely thing to be confused
about during implementation:

| Property | Outer bars | Inner bars |
|----------|-----------|-----------|
| Where | Real backbuffer, outside FBO target rect | Inside FBO, outside 4:3 framed region |
| Who draws | Composition layer (GL clear + aspect-fit blit) | Game UI code (a plain quad in `POLY_PAGE_ALPHA` or similar) |
| When they exist | Physical window aspect outside `[MIN, CAP]` | FBO aspect ≠ 4:3 AND a framed UI scope is active |
| Colour | Black (constant) | Black (for now; could be anything) |
| Coordinate space | Real backbuffer pixels | FBO pixels (= game coords) |
| Can the game see them | No | Yes, trivially, they're game draws |

**Common mistake to avoid**: treating the two kinds of bars as one
problem solvable at one layer. They are not. Outer exists because
*the real window aspect may not match the FBO aspect*. Inner exists
because *the FBO aspect may not match 4:3 and we want legacy 4:3 UI
centred*. One is a physical-window constraint, the other is a
game-UI-style choice. They can coexist, stack, or be absent
independently.

Examples:

- 1920×1080 window, FBO 1920×1080 (no outer bars), menu active
  (framed UI, inner bars on left/right): only inner bars visible.
- 3440×1440 (21:9), FBO clamped to 2880×1440 (16:9, outer bars on
  left/right), gameplay (no framed UI, HUD anchored to FBO corners):
  only outer bars visible.
- Same 21:9, FBO 2880×1440, menu active: both outer bars *and*
  inner bars visible, in different layers of the visual stack.
- 640×480 4:3, no scale, no framed UI complications: neither kind
  of bar.

---

## Post-process effect pipeline (SSAO, bloom, future effects)

The pipeline becomes:

```
   3D draw          → Scene FBO (game's "screen")          ─┐
                                                           │  all post-
   Post FX 1 (e.g. SSAO) → Scene FBO (or aux FBO, same size)│  process
                                                           │  operates
   Post FX 2 (bloom downscale chain, each stage at          │  in FBO
              fbo / 2^n)                                    │  space
                                                           │
   Post FX N                                             ─┘
  ─────────────────────────── stage boundary ───────────────────────
   Composition = FXAA + centred blit into real backbuffer
                                ↓
                   Real backbuffer  + outer bars
                   (only this layer knows real window size)
```

Rules for any future post-process effect author:

- The effect's input and output surfaces are sized at `ScreenWidth ×
  ScreenHeight` (= FBO size) unless there's a specific reason to
  use a different size (e.g. bloom downscales — but still based on
  FBO size, e.g. `ScreenWidth/2, ScreenWidth/4, …`).
- Any `glViewport` call inside the effect operates in FBO coords.
- Any screen-space kernel (SSAO sample radius, blur kernel
  displacement, etc.) is expressed in FBO pixels or FBO-relative
  fractions, not real-window pixels.
- Any `glScissor` call inside the effect operates in FBO coords.

The effect doesn't know outer bars exist, because at the time the
effect runs, the default framebuffer hasn't been touched yet — the
pass is inside the FBO.

For this refactor there's no new effect to wire up, but the
invariants must be preserved so new effects can land cleanly.

---

## Input handling

The only input stream affected by this refactor is **mouse**.
Keyboard, gamepad, text input — unchanged.

### Mouse mapping

Mouse events come in from SDL3 in **logical window coordinates**.
After HiDPI handling (two-pass resize in `sdl3_window_create`),
logical points translate 1:1 to physical pixels — but only after that
resize. Check the HiDPI invariant before touching the mapping.

Transformation chain from raw event to game coord:

```
SDL mouse event (logical points)
    ↓  scale by HiDPI density (→ physical pixels)
window_px
    ↓  subtract composition dst_x, dst_y; divide by (dst_w/fbo_w, dst_h/fbo_h)
fbo_px
    ↓  divide by ScreenWidth/Height, multiply by DisplayWidth/Height (= 640, 480)
game_virtual (0..640, 0..480)
```

The composition layer must expose `(dst_x, dst_y, dst_w, dst_h)` for
the current frame via a getter (or similar). Mouse mapping is the one
place in the game that may read it.

### What happens when the mouse is in an outer bar

Possible behaviours:

- **(a) Drop the event** — event is not delivered to game UI. From
  the game's perspective the mouse is "off-screen".
- **(b) Clamp to FBO edge** — event is delivered with the FBO edge
  coordinate. UI sees "mouse pinned to edge".
- **(c) Passthrough with out-of-range coordinate** — UI gets
  `fbo_px` that's negative or > `ScreenWidth/Height`; UI logic must
  check.

**Chosen behaviour: (a)**. Rationale:
- UI buttons are inside the framed 4:3 region, which is inside the
  FBO. Clicks in outer bars cannot plausibly intend to hit a UI
  element.
- Clamping to edge (b) would cause spurious edge-button activation
  when the user moves the mouse off the game area — wrong.
- Out-of-range passthrough (c) adds a null-check burden to every UI
  call site — avoidable.

Implementation: in the mouse-mapping function, if the raw
`window_px` is outside the composition target rectangle `[dst_x,
dst_x + dst_w) × [dst_y, dst_y + dst_h)`, either (a.i) don't deliver
the event, or (a.ii) deliver with a sentinel (e.g. (-1, -1)) that the
input layer treats as "no position". Pick (a.i) — cleanest — unless
there's an SDL3 reason it has to pass through.

### What happens when the mouse is in an inner bar

Delivered normally to game coords. Inner bars are inside the FBO;
`fbo_px` is in-range. Game UI logic then decides (typically the UI
in that area is a non-interactive bar — no buttons, so clicks do
nothing).

---

## Rename pass

Renaming is part of this refactor, not a separate follow-up. After
the invariants hold, the old names become actively misleading and
must be fixed up to avoid confusion in future work.

| Current name | New name | Scope |
|--------------|----------|-------|
| `RealDisplayWidth` | `ScreenWidth` | Everywhere (`new_game/src/**`, public header) |
| `RealDisplayHeight` | `ScreenHeight` | Everywhere |
| `ui_coords::g_real_w_px` | `ui_coords::g_screen_w_px` | Inside `ui_coords.cpp/h` and all consumers |
| `ui_coords::g_real_h_px` | `ui_coords::g_screen_h_px` | Same |

Not renamed (semantics unchanged):

| Name | Meaning |
|------|---------|
| `DisplayWidth / DisplayHeight` | Virtual 4:3 design canvas 640×480 |
| `ui_coords::g_frame_w_px / g_frame_h_px` | 4:3 framed region centred inside FBO |
| `ui_coords::g_frame_scale` | Scale factor from virtual 4:3 to framed pixel region |
| `POLY_screen_width / POLY_screen_height` | 3D projection scene size |
| `g_viewData.dw*` | 3D viewport rectangle — after refactor equals `(0, 0, ScreenWidth, ScreenHeight)` always |

Recommended: single mechanical search-and-replace over the repo,
followed by a build-clean and a manual pass over search hits to
confirm the new name still reads correctly. Commit the rename as its
own commit so `git log -p -S"RealDisplayWidth"` stays clean.

---

## File-by-file changes

Every file listed here must be touched. If you end up touching a
file not listed here, stop and think — something in the plan is off,
or you're doing more than the refactor needs.

### `engine/graphics/graphics_engine/backend_opengl/game/core.cpp`

**`OpenDisplay`**:
- Replace the existing FBO sizing block with the formula from
  [FBO sizing rule](#fbo-sizing-rule) above.
- After computing `fbo_w, fbo_h`, call `composition_init(fbo_w,
  fbo_h)` — composition reads real backbuffer size from
  `gl_context_get_*` itself, doesn't need a param.
- Publish `ScreenWidth = fbo_w; ScreenHeight = fbo_h;` (renamed
  `RealDisplayWidth/Height`).
- Make sure the existing stderr printout still makes sense — maybe
  print both FBO size and real window size for debugging.

**`ge_set_viewport`** / **`ge_set_scissor`**:
- No semantic change needed; these already operate on whatever
  framebuffer is currently bound. After the refactor, game calls
  them with FBO-space coordinates, composition calls them with real
  backbuffer-space coordinates — the function itself is neutral.

**`ge_begin_scene` / `ge_end_scene` / `ge_flip`**:
- Should continue to bind / unbind the scene FBO as they currently
  do.

### `engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp`

Add the dst-rect computation from [Composition blit](#composition-blit)
above. Blit into the dst rect instead of stretching across the full
real backbuffer. Clear real backbuffer to black before blit.

Expose `composition_get_dst_rect(int* dst_x, int* dst_y, int* dst_w,
int* dst_h)` or similar — mouse mapping needs it.

### `engine/graphics/pipeline/poly.cpp`

**`POLY_camera_set`** — substantial simplification:
- Delete `effective_aspect` computation (no clamping per frame).
  `real_aspect = ScreenWidth / ScreenHeight`, use as-is.
- `POLY_screen_width = DisplayHeight * real_aspect` (effectively:
  the game's 3D viewport in virtual units matches the FBO aspect
  exactly).
- `POLY_screen_clip_right = max(POLY_screen_width, DisplayWidth)`
  stays — this is for UI virtual 640 coverage (see earlier fix for
  "GAME P" truncation). Still required because framed UI uses
  virtual 640 regardless of FBO aspect.
- `PolyPage::SetScaling(ScreenHeight / DisplayHeight, ScreenHeight /
  DisplayHeight)` — uniform.
- Delete `fit_scale` / `base_x` / `base_y` computation. They were
  all artefacts of centring inside the real framebuffer. After the
  refactor the viewport always fills the FBO, so:
  - `g_viewData.dwX = 0`
  - `g_viewData.dwY = 0`
  - `g_viewData.dwWidth = ScreenWidth`
  - `g_viewData.dwHeight = ScreenHeight`
- Delete `PolyPage::s_XOffset / s_YOffset` shift (they were set to
  `base_x / base_y` to move POLY-path vertex output into the MM-path
  bone-matrix coord system). After the refactor `base_x = base_y =
  0`, so the shift is zero; the MM-path no longer needs compensation.
  Keep `s_X/YOffset` as globals (`PolyPage::AddFan` still reads
  them), just always zero from this code path.
- `g_dw3DStuffHeight / g_dw3DStuffY` — these were letterbox-bar
  metadata read by `figure.cpp` to bake into MM character matrices.
  After refactor: set them to `ScreenHeight` / `0`. Consider
  deleting them entirely if no other reader exists; verify with a
  grep.

**Cutscene letterbox (`wideify`)** — the original game applies a
temporary vertical letterbox during dialogue cutscenes by subtracting
`wideify * 2` from `POLY_screen_height`. This is a game-design
feature unrelated to aspect clamping. **Leave it in place.** It's
now cleanly separable: cutscene bars are a different thing from the
architectural aspect bars, and only the game knows about them.

### `engine/graphics/pipeline/aeng.cpp`

**`AENG_clear_viewport`** — delete the pillar/letterbox bar painting
(the `ge_fill_rect` calls for `AENG_clear_bars_*`). Keep the viewport
clear itself. The scene FBO always fills the viewport edge-to-edge;
there are no pixels outside the viewport to paint.

### `engine/graphics/ui_coords.cpp` / `.h`

- `recompute(int fbo_w, int fbo_h)` — parameter semantics unchanged
  (they were always "size of the thing the game calls the screen"),
  but callers pass FBO size, which is now the thing.
- Rename `g_real_w_px / g_real_h_px` → `g_screen_w_px / g_screen_h_px`.
- `g_frame_scale / g_frame_w_px / g_frame_h_px` formulas unchanged.
  They now compute the 4:3 framed region inside the FBO rather than
  inside the real framebuffer — same math, different conceptual
  container. Document the semantic change in a comment at the top
  of the file.

### `engine/platform/sdl3_bridge.cpp`

- Unchanged externally. Still creates the window, handles HiDPI
  resize, delivers raw events.
- Internally, remember that only this file and `composition.cpp` may
  legitimately reference the real window size.

### `game/game.cpp`

**Mode-change callback** — currently computes `uniform_scale =
RealHeight / DisplayHeight` and calls `PolyPage::SetScaling`. After
rename: use `ScreenHeight`. After `POLY_camera_set` is simplified,
this callback may become redundant (SetScaling call may be moved or
deleted — verify).

**`OpenDisplay` call site** — drop `FLAGS_USE_WORKSCREEN` (already
gone). No other change expected.

### `ui/hud/panel.cpp`

**`PANEL_fadeout_draw`** — full rewrite back to the original simple
form:
```cpp
void PANEL_fadeout_draw(void) {
    if (PANEL_fadeout_time) {
        POLY_frame_init(UC_FALSE, UC_FALSE);

        float angle = ...
        // Quad (0, 0)-(DisplayWidth, DisplayHeight) in virtual
        // coords, rendered with whatever affine the caller had set.
        // After the refactor, the affine maps virtual 640×480 to the
        // entire FBO (= the screen), and the cat disc automatically
        // fills it. No special sizing, no save/restore.
        // ...original pp[0..3] setup...

        POLY_add_quad(quad, POLY_PAGE_FADECAT, UC_FALSE, UC_TRUE);

        if (sdl3_get_ticks() > PANEL_fadeout_time + 768) {
            // black centre quad (unchanged)
        }

        POLY_frame_draw(UC_FALSE, UC_FALSE);
    }
}
```

The ×2 magic multiplier, the viewport override, the affine
save/restore — all gone. The effect becomes what it was originally
intended to be: a virtual-canvas-sized quad that fills "the screen".

**`PANEL_darken_screen`** — unchanged; still draws a quad from
`(640 - x, 0)` to `(640, 480)` in virtual coords. After the
refactor, `x = 640` (the saturated value) maps to the full FBO via
the default affine.

### `ui/menus/gamemenu.cpp`

**`GAMEMENU_draw`**:
- Remove the `PolyPage::push_fullscreen_ui_mode()` wrap around
  `PANEL_darken_screen`. Darken now always covers the whole FBO
  using the default affine.
- Remove the `ge_set_scissor` dance.
- Keep the `UIModeScope(CENTER_CENTER)` for the menu *text* draws —
  text is still framed 4:3.
- Remove the special `GAMEMENU_background += millisecs * ...`
  scaling (the earlier "slow it down on wide screens" patch was
  compensating for fullscreen darken being wider than framed
  darken; after the refactor the darken animation rate is always
  right because the canvas is fixed in virtual units).

### `engine/graphics/geometry/sky.cpp`

**`SKY_draw_stars`** — simplify. The current code uses
`PolyPage::s_XScale + XOffset` as the affine, which works, but the
comment mentions "post-Hor+" and the `RealDisplayWidth` caveat.
After the refactor, the mapping can be expressed more cleanly:
either keep the affine (still correct) or switch to a direct
`ScreenWidth / POLY_screen_width`-style ratio if that reads better.
Leave functionality the same; simplify the comment if you choose to
leave the code as-is.

**`SKY_draw_poly_moon` / `SKY_draw_moon_reflection`** — unchanged.
Bounds check against `POLY_screen_width` / `POLY_screen_height`
(already fixed in previous work). Radius division by
`POLY_our_zoom` stays (unrelated to this refactor).

### `ui/hud/overlay.cpp`

`ge_set_viewport` is called at `:296` to reset to full screen before
drawing some overlays. After refactor, "full screen" means
`(0, 0, ScreenWidth, ScreenHeight)` — same as before in numeric
terms but semantically cleaner.

### `ui/frontend/frontend.cpp`

The `if (RealDisplayWidth == 640)` fallback branch near line 397 —
delete. It's already stale; `RealDisplayWidth` (renamed
`ScreenWidth`) won't equal 640 reliably any more (e.g. on non-4:3
scaled configurations).

### `game/game_tick.cpp`

Mouse mapping block around `:3446` — update per
[Input handling](#input-handling).

### `engine/graphics/pipeline/polypage.cpp` / `.h`

- Delete `PolyPage::push_fullscreen_ui_mode()`. Grep for call sites
  first to verify none remain after the other file changes. If any
  do, migrate them to plain code (there's no replacement — the
  concept is gone).
- `push_ui_mode(anchor)` — stays. Its semantics are unchanged:
  set up a 4:3 framed region centred in "the screen" (= FBO after
  refactor), with scissor. The scissor / offset math is relative to
  `g_screen_w_px / g_screen_h_px`, which now equal FBO size.

---

## Code to delete

Explicit list — if you leave any of these in, the refactor is
incomplete.

- `AENG_clear_viewport` pillar/letterbox bar painting. (Keep the
  viewport clear.)
- `POLY_camera_set`: `effective_aspect` clamping logic.
- `POLY_camera_set`: `base_x / base_y` centring computation.
- `POLY_camera_set`: `fit_scale` computation.
- `POLY_camera_set`: the shift that assigns `base_x / base_y` into
  `PolyPage::s_XOffset / s_YOffset`.
- `PANEL_fadeout_draw`: the affine save/override/restore block + ×2
  multiplier + `g_viewData`-based viewport selection.
- `GAMEMENU_draw`: the `push_fullscreen_ui_mode` wrap around
  darken + the `ge_set_scissor` dance.
- `GAMEMENU_process`: the `GAMEMENU_background` increment scaling
  by `DisplayWidth / real_w` (it was itself a workaround).
- `PolyPage::push_fullscreen_ui_mode` definition + declaration + all
  its state-management code in `polypage.cpp`.
- `frontend.cpp`: the `if (RealDisplayWidth == 640)` branch for the
  fade curtain (confirmed stale).
- Any orphan `extern` declarations that become unused after deletion
  (clean-up pass).

---

## Per-effect cleanup opportunities

Not strictly required, but do while you're in there (and test
thoroughly):

- `SKY_draw_stars` — simplify affine expression if desired. Comment
  block can be trimmed.
- Cutscene letterbox (`wideify`) — verify it still works after
  `POLY_camera_set` simplification. The code should still operate
  on `POLY_screen_height` locally, unchanged.
- `POLY_our_zoom` / `POLY_sprite_scale` — untouched by this
  refactor. They compensate for `OC_FOV_MULTIPLIER` and auto-zoom,
  both of which still exist and still affect the projection.

---

## Edge cases & gotchas

### E1. Very small windows

If the user sets `OC_WINDOWED_WIDTH/HEIGHT` very small (say, 80×60),
the FBO ends up tiny. The aspect clamp still applies. Texture
atlases and HUD text were designed for 640×480; expect ugliness but
no crashes. Not a refactor bug; was always the case.

### E2. Render scale = 0.25 plus ultra-wide

Physical window 3440×1440, render scale 0.25, FBO before clamp =
860×360 (aspect 2.39). Clamp to CAP (2.0): FBO = 720×360. Smallest
config we'll realistically see. Verify composition blit upscales
cleanly to `(dst_w, dst_h)` inside the real backbuffer.

### E3. HiDPI portrait

Rotated 4K monitor physically 2160×3840 (9:16 aspect). Clamp to MIN
(2/3): FBO = 2160×3240. Still very large. Verify no overflow in
intermediate math (use `int` not `int16_t`).

### E4. Dynamic window resize

Not supported today. `SetDisplay` is a TODO. If window resize lands
after this refactor, `OpenDisplay` (or equivalent reallocation path)
must rerun the FBO sizing formula and recreate the scene FBO +
composition's aux buffers. Post-resize, also recompute `ui_coords`
and any cached bar rectangles. Flag as follow-up if/when it lands.

### E5. Multi-monitor

SDL3 handles window placement; per-monitor DPI quirks already live
in `sdl3_bridge.cpp`. This refactor doesn't introduce any new
per-monitor concerns.

### E6. `POLY_screen_clip_right` UI fix

Previous fix: `POLY_screen_clip_right = max(POLY_screen_width,
DisplayWidth)` to keep UI virtual-640 coordinates from being
`CLIP_RIGHT`-rejected on narrow aspects. **Keep this fix.** After
the refactor, `POLY_screen_width = DisplayHeight * (ScreenWidth /
ScreenHeight)`, which can still be < 640 on portrait-clamped FBOs
(e.g. a 480×720 FBO gives `POLY_screen_width = 480 * 1.5 = 720`;
OK, not < 640 here; but a 480×640 FBO with MIN = 4/3 gives
`POLY_screen_width = 480 * 1.333 = 640`, exactly at the boundary).
Better safe: keep the `max(...)` guard.

### E7. Cutscene letterbox × aspect-clamp interaction

Cutscene `wideify = 80` subtracts 160 from `POLY_screen_height` for
a ~3/4 letterbox during dialogues. On an already-letterbox-clamped
FBO, cutscene letterbox stacks on top inside the FBO. That's fine
visually — the game adds its own bars inside the FBO, composition
adds outer bars outside. No conflict.

### E8. Resource reload / mode change mid-game

If `OC_FOV_MIN_ASPECT` or `OC_FOV_CAP_ASPECT` change at runtime
(currently they can't — compile-time constants — but just in case):
the FBO needs recreation. Currently, `OpenDisplay` is one-shot per
process. Don't invent runtime mutability; treat them as constants
throughout the refactor.

### E9. Composition's target rect origin is in GL coords

GL has y-up origin at bottom-left; our pixel math treats y-down
origin at top-left. The composition blit calls need to either:
(a) compute `dst_y` in top-left then flip for `glViewport`, or
(b) keep y-down throughout and only flip at the `glViewport` call.
Existing composition code already handles this somewhere; follow
its convention. This is a low-level implementation detail, flagging
only because it's a common place for off-by-one pixel errors.

### E10. Scene FBO binding invariant

Current `core.cpp` maintains the invariant that the scene FBO stays
bound as the active draw target for the entire lifetime of the GL
context — see the comment in `OpenDisplay` about
`composition_bind_scene()`. **Preserve this invariant.** After
composition presents, rebind the scene FBO. Game code's `ge_clear()`
calls rely on it.

---

## Staged implementation plan

Do this in the listed order. Commit after each stage. Verify the
build clean and run-through works on at least 4:3 + 16:9 at each
stage before moving on.

### Stage 1 — FBO aspect clamp, no behaviour elsewhere changes

- Apply the FBO sizing rule in `OpenDisplay`. FBO is now
  aspect-clamped.
- Update `composition_blit` to centre + clear — compute dst rect
  from FBO aspect vs real aspect, clear real backbuffer to black,
  blit into centred rect.
- Everything else (game, UI, effects) still thinks
  `RealDisplayWidth/Height` = FBO size. Because that's now clamped,
  the game now sees a "wider / taller" screen than before on ultra-
  wide / portrait physical windows, but the game doesn't know the
  *real* window size is bigger.
- **Verify**: on 16:9 window, no visible change. On 21:9 window,
  game renders in a 16:9 FBO inside a 21:9 window, with outer
  pillarbox bars composited. On portrait 1:4 window, game renders
  in a 2:3 FBO with outer letterbox bars. Test all five aspects
  from the testing checklist.
- **Known mid-stage weirdness**: some effects may look odd on
  clamped aspects because their workaround code still tries to
  compensate for bars that no longer exist in the game's view. That's
  fine — later stages clean up.

### Stage 2 — Gut `POLY_camera_set`

- Delete `effective_aspect` clamp, `base_x / base_y`, `fit_scale`,
  `s_X/YOffset` shift.
- `g_viewData.dw*` = full FBO.
- `PolyPage::SetScaling(ScreenH / 480, ScreenH / 480)` stays but
  becomes trivially right.
- `POLY_screen_width = DisplayHeight * (ScreenW / ScreenH)`.
- **Verify**: 3D world renders correctly on every aspect. Character
  proportions match across aspects. Mouse-aim lands on the right
  target.

### Stage 3 — Remove `AENG_clear_viewport` bar painting

- Delete the `ge_fill_rect` calls for outer bars. Keep the viewport
  clear.
- **Verify**: no bar double-painting / flicker between AENG clear
  and composition bar paint. Composition bars now own the outer
  space; if AENG also painted, you'd see a brief flash.

### Stage 4 — Rename pass

- `RealDisplayWidth → ScreenWidth`, `RealDisplayHeight →
  ScreenHeight`, `ui_coords::g_real_* → g_screen_*`. Mechanical.
- Build clean, run smoke test. No behaviour change.

### Stage 5 — Per-effect cleanup

- `PANEL_fadeout_draw` back to original simple form.
- `GAMEMENU_draw` — remove `push_fullscreen_ui_mode` and scissor
  dance.
- `GAMEMENU_process` — remove animation rate scaling.
- `SKY_draw_stars` — optional simplification.
- Delete `push_fullscreen_ui_mode`.
- **Verify**: pause / restart / abandon transitions look right on
  every aspect. Stars, moon, menu darken behave correctly.

### Stage 6 — Input mapping

- Update mouse-mapping to use composition dst rect.
- Implement "mouse in outer bar → event dropped" behaviour.
- **Verify**: menu buttons clickable on every aspect. No spurious
  clicks from bar area. HUD reticle aligns with 3D-space crosshair.

### Stage 7 — Full testing sweep

- Run through the entire testing checklist end-to-end.
- Regression-check 4:3 against a pre-refactor reference
  (screenshot / recording). Pixel identity not required; no
  visible differences is required.

---

## Testing checklist

Run the game on each of these configurations and verify:

### Configurations
1. **640×480 windowed** (native 4:3).
2. **1280×720 windowed** (16:9, mid-resolution).
3. **1920×1080 windowed** (16:9, full HD).
4. **2560×1080 windowed** (21:9, near CAP — no clamp needed).
5. **3440×1440 windowed** (21:9, past CAP — outer pillarbox
   expected).
6. **480×1920 windowed** (1:4 portrait, past MIN — outer letterbox
   expected).
7. **1920×1080 fullscreen** (native).
8. **Native desktop resolution, fullscreen**, whatever it is.
9. (Optional) **OC_RENDER_SCALE = 0.5** on a couple of the above, to
   stress render-scale + aspect-clamp combination.

### Per-configuration checks
- [ ] Main menu renders at the correct size, framed 4:3 centred,
      inner bars on non-4:3 FBO aspects drawn by the menu itself.
- [ ] Frontend intro videos aspect-fit into the framed area.
- [ ] Start a mission → loading screen framed correctly.
- [ ] In-game HUD visible, in the expected corner(s). Note: HUD
      anchor rework (Stage 3c) is separate; current pillarbox HUD
      behaviour should be preserved.
- [ ] Pause menu: darken covers full FBO (and inner 4:3 bars black
      if not 4:3 FBO). Menu text centred, not truncated.
- [ ] Pause → Restart → fade-out cat iris: circle describes the
      whole FBO, no gaps, no visible leaked backdrop near corners.
- [ ] Pause → Abandon → same as Restart.
- [ ] Stars visible at night, round, correctly positioned, correct
      size (≈2 px at 1080p, ≈4 px at 4K).
- [ ] Moon visible, circular, doesn't pop out on yaw.
- [ ] Mouse clicks land on UI buttons where the user clicked.
- [ ] Mouse movement in outer bar area: no UI activation (mouse
      "off-screen" behaviour).
- [ ] Mouse movement in inner bar area (when a framed scope is
      active): no UI activation (no interactive elements there).
- [ ] Text not truncated on narrow aspects (regression check for the
      earlier `POLY_screen_clip_right` fix).
- [ ] FPS overlay text visible if enabled.
- [ ] On resume from pause: game continues correctly, no render
      state corruption.

### Regression checks
- [ ] `OC_FOV_MULTIPLIER = 1.0` and `1.5` both render correctly
      (sprite scale, moon, stars all compensated).
- [ ] Cutscene letterbox (dialogue `EWAY_stop_player_moving`) still
      narrows vertically during conversations.
- [ ] ~~Save/load works~~ (not affected, but smoke-test).

---

## Rollback

This refactor is a single-session change across many files. Don't
attempt partial rollback — restart from main.

Keep the commit history clean: one commit per stage from
[Staged implementation plan](#staged-implementation-plan) above so
that `git bisect` can pinpoint regressions to a specific stage.

Before landing, ask the user to run the game on at least two
different aspects. The user has been testing these changes
interactively; don't merge without their sign-off.

---

## Out of scope

Deliberately excluded from this refactor. Don't mix in:

- **HUD anchor rework (Stage 3c)** — anchoring radar / ammo /
  weapon / wristwatch to FBO corners. Separate todo. Depends on this
  refactor landing first (easier after FBO = screen), but the
  implementation is independent.
- **Replacing simplified FXAA with canonical FXAA 3.11 or SMAA 1x**
   — unrelated.
- **Splitting UI from scaled scene FBO** (rendering UI into the
  default framebuffer at native resolution so FXAA doesn't soften
  text) — unrelated architectural change. Can be layered on top
  later if still desired after this refactor.
- **Wibble amplitude scaling with resolution** — unrelated.
- **NIGHT lighting pool UWORD refactor** — post-1.0.
- **Runtime window resize support** — `SetDisplay` is still a TODO.

---

## Open questions

Before you start, resolve these with the user if they haven't
already been resolved in this document:

1. **Outer bar colour** — document says black. User has flagged this
   may become configurable post-1.0 (brand-colour gradient?). For
   this refactor: black, unconditionally. No config knob.
2. **Where FBO aspect clamp lives** — this document puts it in
   `OpenDisplay`. That's the clean place because `OpenDisplay` also
   publishes `ScreenWidth/Height`. Don't move it to `composition.cpp`
   — composition should not decide the FBO size.
3. **`SetDisplay` (runtime reconfigure)** — currently a stub. Not
   touched by this refactor. Post-refactor, if/when runtime resize
   lands, the implementer must rerun the FBO sizing formula and
   reallocate. Document that requirement wherever `SetDisplay` is
   eventually implemented.
4. **Per-monitor DPI** — SDL3 handles it; this refactor doesn't add
   new concerns. Smoke-test on a HiDPI monitor if available.

If you think of another open question that isn't listed here and the
answer affects the refactor's correctness, raise it **before**
writing code. The cost of asking is low; the cost of guessing wrong
is high.
