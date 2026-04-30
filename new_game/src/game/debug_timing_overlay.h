#ifndef GAME_DEBUG_TIMING_OVERLAY_H
#define GAME_DEBUG_TIMING_OVERLAY_H

// Self-contained "phys: N  lock: M fps  IP: on  fps: Z.Z" overlay. Registered
// as the engine's diagnostic overlay callback (ge_set_diagnostic_overlay_callback)
// so it fires from every present path — main flip, back-buffer blit and the
// video player. Outro reaches it via oge_show → ge_flip → callback.

// Updates the wall-clock FPS measurement and draws the overlay via FONT2D
// at the top-left corner.
void debug_timing_overlay_render_font2d(void);

#endif // GAME_DEBUG_TIMING_OVERLAY_H
