#pragma once

// Hardcoded initial configuration. Placeholder until a real runtime config
// file (openchaos.cfg) is wired up — the values below are what the game
// will boot with. Flip constants here and rebuild to try different
// resolutions / window modes.

// true  = start fullscreen at the monitor's native resolution.
// false = start windowed at OC_WINDOWED_WIDTH × OC_WINDOWED_HEIGHT.
#define OC_FULLSCREEN true

// Windowed-mode resolution. Ignored when OC_FULLSCREEN is true.
#define OC_WINDOWED_WIDTH  int(1280)
#define OC_WINDOWED_HEIGHT int(960)

// Enable VSync. Concrete strategy (driver VSync vs DwmFlush) depends on
// window mode and is selected inside sdl3_gl_configure_vsync().
#define OC_VSYNC true

// Internal render resolution scale (0.25 .. 1.0).
// Scene renders into an offscreen FBO of (physical × scale) and is then
// upscaled to the native window by the composition pass. 1.0 = pixel-perfect,
// lower = blurrier but cheaper on the GPU. Useful on 4K / Retina monitors
// where a full-resolution frame is more than needed.
// NOTE: config-only for now; will move into a runtime graphics menu later.
#define OC_RENDER_SCALE 1.0f

// Minimum value guarded by the composition pass — below this the scene
// becomes unreadable and GL driver texture-size math gets wobbly on some
// platforms.
#define OC_RENDER_SCALE_MIN 0.25f

// Enable shader-based anti-aliasing (FXAA 3.11) in the composition pass.
// This is a graphics quality setting: ON gives cheap post-process AA over
// the whole frame (including alpha-test edges that MSAA never covered);
// OFF gives pixel-perfect output. Do NOT toggle this as a workaround for
// unrelated rendering issues.
#define OC_AA_ENABLE false
