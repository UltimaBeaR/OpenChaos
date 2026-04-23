# Testing

## How to change resolution

All initial config lives in
[`new_game/src/config.h`](../../new_game/src/config.h). Flip the constants
and rebuild (`make build-release` or `make r`):

```c
#define OC_FULLSCREEN         false        // true = native monitor fullscreen
#define OC_WINDOWED_WIDTH     1920         // ignored in fullscreen
#define OC_WINDOWED_HEIGHT    1080         // ignored in fullscreen
#define OC_VSYNC              true

#define OC_FOV_MULTIPLIER     1.0F         // 1.0 = original; >1 widens FOV
#define OC_FOV_CAP_ASPECT     (16.0F / 9.0F)  // pillarbox beyond this
#define OC_FOV_MIN_ASPECT     (4.0F / 3.0F)   // letterbox below this
```

A real runtime config (`openchaos.cfg`) will replace these later — for
now this is how we iterate.

## ⚠️ Test matrix (must cover multiple aspects)

Many resolution bugs only surface on **non-4:3** aspect ratios. Testing
only at 640×480 or 1024×768 (both 4:3) can silently pass broken code.
Discovered the hard way with the water wibble bug.

At minimum, before declaring any resolution-related fix done:

| Resolution | Aspect | Clamp zone | Why                                              |
|------------|--------|-----------|--------------------------------------------------|
| 640×480    | 4:3    | base      | Baseline. Must match the original exactly.      |
| 1024×768   | 4:3    | base      | Same aspect, bigger — catches pure-scale bugs.  |
| 1920×1080  | 16:9   | at CAP    | The common widescreen target.                    |
| 1280×800   | 16:10  | [base,CAP] | Steam Deck's native — important for 1.0.        |
| 2560×1080  | 21:9   | > CAP     | Ultrawide — exercises pillarbox bars + FOV cap. |
| 800×800    | 1:1    | [MIN,base] | Tall-but-not-extreme — exercises auto-zoom.     |
| 480×1920   | 1:4    | < MIN     | Portrait extreme — exercises letterbox bars.    |
| OC_FULLSCREEN true | native | depends | Sanity check on whatever the user's monitor is. |

Not every fix needs all eight — but at least one aspect inside the
`[MIN..base]` clamp range AND one outside (either pillar or letter)
are mandatory before closing any aspect-related ticket. A fix that
only works in one clamp zone doesn't actually work.

## What to look for on each aspect

- **Geometry proportions.** Pick a scene with recognizable rectangular
  objects (buildings, crates). They should look square/rectangular, not
  squashed or stretched, regardless of aspect.
- **Character pixel size.** Should scale with `RealH` in `[base..CAP]`
  and with `RealW` in `[MIN..base]` and below MIN. Shouldn't pixel-
  explode on tall/portrait windows — auto-zoom prevents that.
- **Horizontal FOV.** 4:3..CAP range: Hor+ widens horizontal (more
  visible sides). At CAP: stops widening, pillar bars appear. MIN..4:3
  range: horizontal FOV stays 4:3 (auto-zoom) — extra height fills
  with more sky/ground, no bars. Below MIN: letter bars on top/bottom.
- **Sprite / rain / line / moon sizes.** Should stay consistent with
  character proportions across all aspects. If light glows look huge
  relative to characters on one aspect, something broke in the
  `POLY_sprite_scale` formula.
- **Pillar / letter bars.** Should be black (from `ge_fill_rect` in
  `AENG_clear_viewport`), abut the 3D viewport with no 1-pixel gap.
- **HUD elements.** Health bar, radar, weapon indicator, messages.
  Currently pillarboxed to the 4:3 framed centre — that's the known
  Stage 3c item.
- **Water reflections / wibble.** Stand next to a puddle and see the
  distortion waves. Should be visible at all aspects. Amplitude
  scaling with resolution is still open (known).
- **Menus / frontend / outro / intro videos.** All framed to 4:3
  via `ui_coords`. Should not pillar/letter-double with the 3D bars.
- **Mouse targeting / menu clicks.** Hit-testing should feel right —
  no offset between cursor and clicked target.

## Common symptoms ↔ likely cause

| Symptom                                    | Likely cause                             |
|--------------------------------------------|-----------------------------------------|
| HUD sits in a box, right of it empty       | HUD pillarbox (Stage 3c, known)         |
| 3D world stretched horizontally            | `POLY_screen_width` or `PolyPage::s_XScale` wrong |
| Environment offset left, characters centred on pillar/letter aspect | `PolyPage::s_XOffset` / `s_YOffset` not set to `base_x` / `base_y` |
| Black cut-outs on scene periphery          | `auto_zoom` / `OC_FOV_MULTIPLIER` applied only inside `POLY_camera_set`, not at `AENG_lens` source (culling desync) |
| Sprite / flare / moon huge 2× relative to character on default 4:3 | Sprite scale multiplied by raw `POLY_cam_lens` (~2.25 baseline). Use only `(OC_FOV_MULTIPLIER × auto_zoom)` divisor |
| Character huge on tall / portrait window   | Auto-zoom not active in `[MIN..base]` — check `AENG_lens` divisor |
| Moon / sprites wrong size on cutscene zoom | Cutscene lens was included in sprite scale (it shouldn't — original behaviour is lens-independent sprites) |
| Effect applied to wrong area (not on target) | Non-uniform bbox scaling (like wibble was) |
| Works at 4:3, broken at 16:9               | Mixed `RealW/640` + `RealH/480` math    |
| Both aspects broken the same way           | Hardcoded 640 / 480 somewhere           |
| Mouse offset from click target             | Transform uses wrong scale factor       |
