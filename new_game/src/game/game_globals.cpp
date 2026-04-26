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
