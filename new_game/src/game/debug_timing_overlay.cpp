#include "game/debug_timing_overlay.h"

#include <stdio.h>

#include "engine/core/types.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // ge_set_ui_mode
#include "engine/graphics/pipeline/poly.h" // POLY_frame_init/draw
#include "engine/graphics/text/font2d.h" // FONT2D_DrawString
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks
#include "game/game_globals.h" // g_physics_hz / g_render_fps_cap

// FONT2D scale: 384 = 1.5× normal (256). Keeps overlay readable on small
// windows without dominating the screen.
static constexpr SLONG DEBUG_FONT_SCALE = 384;

// Sliding wall-clock window for FPS averaging. Short enough to react
// quickly when FPS changes (≤0.5 s lag), long enough to absorb the
// integer-ms tick granularity at 30 Hz (33/34 ms alternation that
// otherwise flickers the displayed value between 29 and 30).
static constexpr uint64_t FPS_WINDOW_MS = 500;

// Power-of-two ring buffer of frame timestamps. Sized to hold one full
// window even at very high FPS — at 240 FPS over 0.5 s = 120 entries,
// 256 gives plenty of headroom and lets us mask instead of modulo.
static constexpr SLONG  FPS_RING_SIZE = 256;
static constexpr SLONG  FPS_RING_MASK = FPS_RING_SIZE - 1;

static uint64_t s_fps_ring[FPS_RING_SIZE] = { 0 };
static SLONG    s_fps_head  = 0;     // next write slot
static SLONG    s_fps_count = 0;     // valid entries in ring (≤ FPS_RING_SIZE)
static float    s_fps_value = 0.0f;  // last computed displayed value

static void update_fps()
{
    const uint64_t now = sdl3_get_ticks();

    // Append current timestamp (ring overwrites oldest if full).
    s_fps_ring[s_fps_head] = now;
    s_fps_head = (s_fps_head + 1) & FPS_RING_MASK;
    if (s_fps_count < FPS_RING_SIZE) s_fps_count += 1;

    // Find the oldest entry still inside the window.
    const uint64_t cutoff = (now > FPS_WINDOW_MS) ? (now - FPS_WINDOW_MS) : 0;
    SLONG    in_window     = 0;
    uint64_t oldest_in_win = now;
    for (SLONG i = 0; i < s_fps_count; i++) {
        const SLONG    idx = (s_fps_head - 1 - i) & FPS_RING_MASK;
        const uint64_t ts  = s_fps_ring[idx];
        if (ts < cutoff) break;
        oldest_in_win = ts;
        in_window    += 1;
    }

    // FPS = (frames-1) intervals over the elapsed window time.
    if (in_window >= 2) {
        const uint64_t span_ms = now - oldest_in_win;
        if (span_ms > 0) {
            s_fps_value = float(in_window - 1) * 1000.0f / float(span_ms);
        }
    }
}

void debug_timing_overlay_render_font2d(void)
{
    update_fps();

    // POLY_frame_init transitively calls ge_begin_scene which routes by
    // ui_mode: with ui_mode off it would re-bind the scene FBO and our
    // FONT2D draws would silently land there (after composition has
    // already run). Wrap the draw in ui_mode so it stays on the default
    // FB where the composed scene already lives.
    ge_set_ui_mode(true);
    extern bool g_render_interp_enabled;
    const char* ip_str = g_render_interp_enabled ? "on" : "off";

    char tbuf[128];
    if (g_render_fps_cap <= 0) {
        snprintf(tbuf, sizeof(tbuf), "phys: %d  lock: unlim  IP: %s  fps: %.1f",
            g_physics_hz, ip_str, s_fps_value);
    } else {
        snprintf(tbuf, sizeof(tbuf), "phys: %d  lock: %d fps  IP: %s  fps: %.1f",
            g_physics_hz, g_render_fps_cap, ip_str, s_fps_value);
    }
    POLY_frame_init(UC_FALSE, UC_FALSE);
    FONT2D_DrawString((CBYTE*)tbuf, 4, 4, 0xffff00, DEBUG_FONT_SCALE);
    POLY_frame_draw(UC_FALSE, UC_FALSE);
    ge_set_ui_mode(false);
}
