#ifndef GAME_DEBUG_TIMING_OVERLAY_H
#define GAME_DEBUG_TIMING_OVERLAY_H

#include "debug_config.h"

#if OC_DEBUG_PHYSICS_TIMING

// Physics/render timing overlay: "phys: N  lock: M fps  IP: on  fps: Z.Z".
// Registered as the engine diagnostic overlay callback — fires on every
// present path during gameplay. Only compiled when OC_DEBUG_PHYSICS_TIMING.
void debug_timing_overlay_render_font2d(void);

#endif // OC_DEBUG_PHYSICS_TIMING
#endif // GAME_DEBUG_TIMING_OVERLAY_H
