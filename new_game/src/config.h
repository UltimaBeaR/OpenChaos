#pragma once

// Hardcoded initial configuration. Placeholder until a real runtime config
// file (openchaos.cfg) is wired up — the values below are what the game
// will boot with. Flip constants here and rebuild to try different
// resolutions / window modes.

// true  = start fullscreen at the monitor's native resolution.
// false = start windowed at OC_WINDOWED_WIDTH × OC_WINDOWED_HEIGHT.
#define OC_FULLSCREEN false

// Windowed-mode resolution. Ignored when OC_FULLSCREEN is true.
#define OC_WINDOWED_WIDTH  640
#define OC_WINDOWED_HEIGHT 480

// Enable VSync. Concrete strategy (driver VSync vs DwmFlush) depends on
// window mode and is selected inside sdl3_gl_configure_vsync().
#define OC_VSYNC true

// Internal render resolution scale (0.25 .. 1.0).
// Scene renders into an offscreen FBO of (physical × scale) and is then
// upscaled to the native window by the composition pass. 1.0 = pixel-perfect,
// lower = blurrier but cheaper on the GPU. Useful on 4K / Retina monitors
// where a full-resolution frame is more than needed.
// NOTE: config-only for now; will move into a runtime graphics menu later.
#define OC_RENDER_SCALE 1.00F

// Enable shader-based anti-aliasing (FXAA 3.11) in the composition pass.
// This is a graphics quality setting: ON gives cheap post-process AA over
// the whole frame (including alpha-test edges that MSAA never covered);
// OFF gives pixel-perfect output. Do NOT toggle this as a workaround for
// unrelated rendering issues.
#define OC_AA_ENABLE true

// Horizontal/vertical FOV multiplier applied to the camera lens parameter.
// 1.0 = original game FOV (unchanged). Higher = wider FOV, world appears
// smaller (more visible horizontally and vertically, zoom-out effect).
// Lower = narrower FOV, zoom-in. Applied symmetrically — this is a
// standard FPS-style "FOV slider" over the existing rectilinear
// projection, not a projection-type switch; does not eliminate fish-eye
// at large values. Sensible range ~0.75..1.30. Intended to move into a
// runtime settings menu later.
#define OC_FOV_MULTIPLIER 1.0F
