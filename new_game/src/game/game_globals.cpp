// uc_orig: Game.cpp (fallen/Source/Game.cpp)
// Global variables from the top-level game orchestrator.

#include "game/game_globals.h"

// uc_orig: the_game (fallen/Source/Game.cpp)
Game the_game;

// Violence flag: always 1 in standard builds.
// Set to 0 only for German/French PC censored versions (not needed in new game).
// uc_orig: VIOLENCE (fallen/Source/Game.cpp)
UBYTE VIOLENCE = 1;

// uc_orig: game_paused_key (fallen/Source/Game.cpp)
UBYTE game_paused_key = 0;

// uc_orig: bullet_upto (fallen/Source/Game.cpp)
SLONG bullet_upto = 0;

// uc_orig: bullet_counter (fallen/Source/Game.cpp)
SLONG bullet_counter = 0;

// uc_orig: already_warned_about_leaving_map (fallen/Source/Game.cpp)
// BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
uint64_t already_warned_about_leaving_map = 0;

// uc_orig: draw_map_screen (fallen/Source/Game.cpp)
UBYTE draw_map_screen = 0;

// uc_orig: single_step (fallen/Source/Game.cpp)
UBYTE single_step = 0;

// uc_orig: form_leave_map (fallen/Source/Game.cpp)
Form* form_leave_map = NULL;

// uc_orig: form_left_map (fallen/Source/Game.cpp)
SLONG form_left_map = 0;

// uc_orig: last_fudge_message (fallen/Source/Game.cpp)
UWORD last_fudge_message = 0;

// uc_orig: last_fudge_camera (fallen/Source/Game.cpp)
UWORD last_fudge_camera = 0;

// uc_orig: the_end (fallen/Source/Game.cpp)
UBYTE the_end = 0;

// uc_orig: env_frame_rate (fallen/Source/Game.cpp)
UWORD env_frame_rate = 30;

SLONG g_physics_hz     = UC_PHYSICS_DESIGN_HZ;
SLONG g_render_fps_cap = 0;  // 0 = unlimited (no frame cap)

SLONG VISUAL_TURN = 0;

void visual_turn_tick(float dt_ms)
{
    // Wall-clock accumulator — independent of physics rate, render rate
    // and pause state. Same cap as the main loop's frame_dt_ms (200 ms)
    // prevents catch-up bursts after long stalls / debugger pauses.
    static double acc_ms = 0.0;
    acc_ms += double(dt_ms);
    if (acc_ms > 200.0) acc_ms = 200.0;
    while (acc_ms >= double(UC_VISUAL_CADENCE_TICK_MS)) {
        VISUAL_TURN++;
        acc_ms -= double(UC_VISUAL_CADENCE_TICK_MS);
    }
}

// Default to one UC_VISUAL_CADENCE_TICK_MS so callers that read this
// before any render loop has set it still get a sensible value (one
// 30 Hz visual-cadence frame = 33.33 ms).
float g_frame_dt_ms = UC_VISUAL_CADENCE_TICK_MS;

// uc_orig: playback_name (fallen/Source/Game.cpp)
// uc-abs-path: was "C:\Windows\Desktop\UrbanChaosRecordedGame.pkt"
CBYTE* playback_name = "UrbanChaosRecordedGame.pkt";

// player_pos is defined in actors/core/player_globals.cpp (from Player.cpp).

// uc_orig: tick_ratios (fallen/Source/Game.cpp)
SLONG tick_ratios[4];

// uc_orig: wptr (fallen/Source/Game.cpp)
SLONG wptr = 0;

// uc_orig: number (fallen/Source/Game.cpp)
SLONG number = 0;

// uc_orig: sum (fallen/Source/Game.cpp)
SLONG sum = 0;
