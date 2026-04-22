# Fullscreen / arbitrary resolution transition

Working notes for moving the game off its hardcoded 640×480 4:3 viewport to
arbitrary resolution + fullscreen support. Targets Stage 12 / Release 1.0.

## Current approach

- Initial config lives in [`new_game/src/config.h`](../new_game/src/config.h)
  (hardcoded flags: `OC_FULLSCREEN`, `OC_WINDOWED_WIDTH/HEIGHT`, `OC_VSYNC`).
  Will be replaced by a real runtime config (`openchaos.cfg`) later.
- 3D rendering uses **Hor+** FOV extension: virtual render width scales
  with monitor aspect (`POLY_screen_width = DisplayHeight · RealW/RealH`),
  so widescreen monitors get wider horizontal FOV instead of stretched
  geometry. Vertical FOV is preserved at the original 640×480 value.
- `PolyPage::SetScaling` is uniform (by vertical), so no non-uniform
  stretch is introduced downstream.

## Known issues to fix before 1.0

- [ ] **HUD is tiny and pillarboxed.** HUD is drawn in virtual 640×480
  space and scaled by `s_YScale` (= RealH / 480). On 1080p it occupies a
  1440×1080 region on the left; text and UI look small and off-center.
  Needs a dedicated HUD layout pass: either place widgets relative to
  actual screen edges, or size the HUD to a fixed fraction of screen
  height. Probably both.
- [ ] **Water reflections render incorrectly.** Character draws fine but
  the reflected scene / effect is missing or broken. Spotted during first
  windowed-resolution test at 1920×1080. Not investigated yet — revisit
  after HUD work.
- [ ] **Focus callback cursor show/hide breaks linker.** In `host.cpp`,
  `on_focus_gained` / `on_focus_lost` call `sdl3_hide_cursor()` /
  `sdl3_show_cursor()` gated on `ge_is_fullscreen()` as a belt-and-suspenders
  against driver quirks around alt-tab. The build fails at link time with
  this code present; symbol not yet diagnosed. Testing showed SDL handles
  alt-tab cursor visibility correctly on its own, so this is defensive
  code, not functionally required.

## Resolved

- ~~**Fullscreen rendered only a 1920×1080 region in the bottom-left of
  higher-res monitors.**~~ Fixed in `sdl3_window_create` by explicitly
  querying `SDL_GetDesktopDisplayMode` and forcing the fullscreen mode
  to NULL (borderless-desktop) after creation. SDL3 was taking the
  passed `(width, height)` as a fullscreen mode request.
- ~~**OS cursor was visible over the fullscreen game.**~~ `SDL_HideCursor()`
  is called in `sdl3_window_create` when fullscreen is requested. SDL
  restores the OS cursor automatically when focus leaves the window
  (alt-tab etc.) and re-hides it on focus return — no explicit handling
  needed in the common path.
