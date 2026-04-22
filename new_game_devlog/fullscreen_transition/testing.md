# Testing

## How to change resolution

All initial config lives in
[`new_game/src/config.h`](../../new_game/src/config.h). Flip the constants
and rebuild (`make build-release` or `make r`):

```c
#define OC_FULLSCREEN      false       // true = native monitor fullscreen
#define OC_WINDOWED_WIDTH  1920        // ignored in fullscreen
#define OC_WINDOWED_HEIGHT 1080        // ignored in fullscreen
#define OC_VSYNC           true
```

A real runtime config (`openchaos.cfg`) will replace these later — for
now this is how we iterate.

## ⚠️ Test matrix (must cover multiple aspects)

Many resolution bugs only surface on **non-4:3** aspect ratios. Testing
only at 640×480 or 1024×768 (both 4:3) can silently pass broken code.
Discovered the hard way with the water wibble bug.

At minimum, before declaring any resolution-related fix done:

| Resolution | Aspect | Why                                              |
|------------|--------|--------------------------------------------------|
| 640×480    | 4:3    | Baseline. Must match the original exactly.      |
| 1024×768   | 4:3    | Same aspect, bigger — catches pure-scale bugs.  |
| 1920×1080  | 16:9   | The common widescreen target.                    |
| 1280×800   | 16:10  | Steam Deck's native — important for 1.0.        |
| 2560×1080  | 21:9   | Ultrawide — stresses aspect math hardest.       |
| OC_FULLSCREEN true | native | Sanity check on whatever the user's monitor is. |

Not every fix needs all six — but at least one 4:3 and one non-4:3 are
mandatory before closing a ticket. A fix that only works at 4:3 doesn't
actually work.

## What to look for on each aspect

- **Geometry proportions.** Pick a scene with recognizable rectangular
  objects (buildings, crates). They should look square/rectangular, not
  squashed or stretched, regardless of aspect.
- **Horizontal FOV.** On widescreen you should see **more** to the sides
  than on 4:3 (Hor+). Same vertical amount. Not a wider but squashed view.
- **HUD elements.** Health bar, radar, weapon indicator, messages.
  Currently pillarboxed to the left 4:3 area on widescreen — that's the
  known HUD bug.
- **Water reflections / wibble.** Stand next to a puddle and see the
  distortion waves. Should be visible at all aspects.
- **Menus / frontend.** Attract mode, title screen, options menu.
  Currently shares the HUD layout issue.
- **Mouse targeting / menu clicks.** Hit-testing should feel right — no
  offset between cursor and clicked target.
- **Cutscenes / outro.** Separate render path; not yet aspect-audited.

## Common symptoms ↔ likely cause

| Symptom                                    | Likely cause                             |
|--------------------------------------------|-----------------------------------------|
| HUD sits in a box on left side, right empty| HUD pillarbox (known)                   |
| 3D world stretched horizontally            | `POLY_screen_width` or `s_XScale` wrong |
| Effect applied to wrong area (not on target) | Non-uniform bbox scaling (like wibble was) |
| Works at 4:3, broken at 16:9               | Mixed `RealW/640` + `RealH/480` math    |
| Both aspects broken the same way           | Hardcoded 640 / 480 somewhere           |
| Mouse offset from click target             | Transform uses wrong scale factor       |
