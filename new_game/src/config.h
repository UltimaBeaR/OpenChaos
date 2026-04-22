#pragma once

// Hardcoded initial configuration. Placeholder until a real runtime config
// file (openchaos.cfg) is wired up — the values below are what the game
// will boot with. Flip constants here and rebuild to try different
// resolutions / window modes.

// true  = start fullscreen at the monitor's native resolution.
// false = start windowed at OC_WINDOWED_WIDTH × OC_WINDOWED_HEIGHT.
#define OC_FULLSCREEN false

// Windowed-mode resolution. Ignored when OC_FULLSCREEN is true.
#define OC_WINDOWED_WIDTH  int(600)
#define OC_WINDOWED_HEIGHT int(800)

// Enable VSync. Concrete strategy (driver VSync vs DwmFlush) depends on
// window mode and is selected inside sdl3_gl_configure_vsync().
#define OC_VSYNC true
