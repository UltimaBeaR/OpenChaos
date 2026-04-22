# Fullscreen / arbitrary resolution transition

Working notes for moving the game off its hardcoded 640×480 4:3 viewport
to arbitrary resolution + fullscreen support. Targets Stage 12 / Release 1.0.

## Status at a glance

Already done:
- Fullscreen window creation (native monitor resolution, borderless-desktop).
- 3D projection is aspect-aware (Hor+ FOV, widescreen = wider horizontal field).
- Uniform vertical scaling for the rendered world.
- Config knobs in [`new_game/src/config.h`](../../new_game/src/config.h):
  `OC_FULLSCREEN`, `OC_WINDOWED_WIDTH/HEIGHT`, `OC_VSYNC`.
- Water wibble post-process repositioned correctly on non-4:3 resolutions.
- OS cursor hidden in fullscreen.

Remaining work — see [`issues.md`](issues.md):
- **Render scale + native physical pixels (in progress).** Window in real
  pixels everywhere (fixes macOS Retina clipping); game renders into a
  scaled FBO composited up to the window. UI not split from 3D for now.
- HUD / UI is pillarboxed and too small on widescreen **(blocker)** —
  pending design discussion on aspect ratio + UI coord system before
  rework.
- `s_work_screen_buf` hardcoded to 640×480 bytes, needs audit.
- Wibble amplitude doesn't scale with resolution — effect too subtle at 1080p+.
- Focus callback cursor show/hide breaks linker (low priority, defensive code).
- Outro / frontend fade / menus need a verification pass.

## Files in this folder

- **[`concepts.md`](concepts.md)** — terminology and pipeline flow. **Read this
  first** if you're new to the code: explains `DisplayWidth` vs
  `RealDisplayWidth` vs `POLY_screen_width`, what `s_XScale/s_YScale` does,
  and how a vertex travels from virtual game coords to framebuffer pixels.
- **[`testing.md`](testing.md)** — how to reproduce bugs. Critical: most bugs
  only show up on **non-4:3** resolutions. Testing only at 4:3 lies.
- **[`issues.md`](issues.md)** — detailed breakdown of each known issue, with
  symptoms, root cause, entry-point files, and proposed fix approaches.
- **[`audit.md`](audit.md)** — exhaustive inventory of every place in
  `new_game/src/` where resolution or aspect ratio influences behavior.
  Verdict: works / broken / to-verify. Use as reference when touching
  render/UI/input code.

## Workflow

1. Pick an issue from [`issues.md`](issues.md).
2. Read [`concepts.md`](concepts.md) if any terminology there is unfamiliar.
3. Apply fix. Never edit files without user consent — propose first.
4. Test per [`testing.md`](testing.md). Verify the fix at multiple aspects,
   not just one.
5. Move the entry from "Known issues" → "Resolved" in [`issues.md`](issues.md)
   with a short note on what was done.
