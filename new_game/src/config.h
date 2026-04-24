#pragma once

// Hardcoded initial configuration. Placeholder until a real runtime config
// file (openchaos.cfg) is wired up — the values below are what the game
// will boot with. Flip constants here and rebuild to try different
// resolutions / window modes.

// true  = start fullscreen at the monitor's native resolution.
// false = start windowed at OC_WINDOWED_WIDTH × OC_WINDOWED_HEIGHT.
#define OC_FULLSCREEN false

// Windowed-mode resolution. Ignored when OC_FULLSCREEN is true.

#define OC_WINDOWED_WIDTH  int(1920)
#define OC_WINDOWED_HEIGHT int(480)

// #define OC_WINDOWED_WIDTH  int(480)
// #define OC_WINDOWED_HEIGHT int(1920)

// #define OC_WINDOWED_WIDTH  int(900)
// #define OC_WINDOWED_HEIGHT int(480)

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

// Maximum horizontal aspect ratio the 3D scene is rendered at. Screens
// wider than this get pillarboxed (black bars on left/right) with the
// scene centred at the cap aspect. Protects against rectilinear fish-eye
// at ultra-wide FOVs, which is a property of the projection itself, not
// a bug. Default 16:9 — preserves 4:3 / 16:10 / 16:9 behaviour exactly
// (cap never triggers up to 16:9); 21:9 and wider users get pillarboxed
// 16:9 content instead of stretched fish-eye edges.
#define OC_FOV_CAP_ASPECT (18.0F / 9.0F)

// Minimum horizontal aspect ratio the 3D scene is rendered at. Screens
// narrower (taller) than this get letterboxed (black bars top+bottom)
// with the scene centred at the floor aspect. Symmetric to
// OC_FOV_CAP_ASPECT. Purpose: avoid "character looks huge" on portrait
// / tall windows — rectilinear projection with isotropic pixels forces
// a very narrow horizontal FOV at those aspects (atan(W/H) ≈ 14° at
// 1:4 aspect vs 53° at 4:3), which visually reads as extreme zoom.
// Default 4:3 — 5:4 / 16:10 / 16:9 never trigger; portrait or square
// screens fall back to 4:3 centred inside the window.
#define OC_FOV_MIN_ASPECT (2.0F / 3.0F)
