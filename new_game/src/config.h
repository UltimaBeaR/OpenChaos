#pragma once

// Hardcoded initial configuration. Placeholder until a real runtime config
// file (openchaos.cfg) is wired up — the values below are what the game
// will boot with. Flip constants here and rebuild to try different
// resolutions / window modes.

// 1 = start fullscreen at the monitor's native resolution.
// 0 = start windowed at OC_WINDOWED_WIDTH × OC_WINDOWED_HEIGHT.
#define OC_FULLSCREEN 0

// Windowed-mode resolution. Ignored when OC_FULLSCREEN is 1.
#define OC_WINDOWED_WIDTH  1920
#define OC_WINDOWED_HEIGHT 1080

// 1 = enable VSync. Concrete strategy (driver VSync vs DwmFlush) depends on
// window mode and is selected inside sdl3_gl_configure_vsync().
#define OC_VSYNC 1
