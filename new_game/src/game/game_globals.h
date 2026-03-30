#ifndef GAME_GAME_GLOBALS_H
#define GAME_GAME_GLOBALS_H

#include "engine/platform/uc_common.h"
#include "things/core/thing.h"     // Pool type definitions (Vehicle, Person, Animal, etc.) needed by game_types.h
#include "game/game_types.h"
#include "ui/menus/widget.h"
#include "ui/menus/widget_globals.h"

// Number of tips shown during loading screens.
#define NUM_BULLETS 15

// Pause menu entry count.
#define PAUSE_MENU_SIZE 2

// Camera position/orientation set each frame (from either FC or EWAY cinematic camera).
// uc_orig: CAM_cur_x (fallen/Source/Game.cpp)
extern SLONG CAM_cur_x;

// uc_orig: CAM_cur_y (fallen/Source/Game.cpp)
extern SLONG CAM_cur_y;

// uc_orig: CAM_cur_z (fallen/Source/Game.cpp)
extern SLONG CAM_cur_z;

// uc_orig: CAM_cur_yaw (fallen/Source/Game.cpp)
extern SLONG CAM_cur_yaw;

// uc_orig: CAM_cur_pitch (fallen/Source/Game.cpp)
extern SLONG CAM_cur_pitch;

// uc_orig: CAM_cur_roll (fallen/Source/Game.cpp)
extern SLONG CAM_cur_roll;

// PSX-style pause menu options (unused on PC — PC uses GAMEMENU).
// uc_orig: pause_menu (fallen/Source/Game.cpp)
extern CBYTE* pause_menu[PAUSE_MENU_SIZE];

// uc_orig: game_paused_key (fallen/Source/Game.cpp)
extern UBYTE game_paused_key;

// uc_orig: game_paused_highlight (fallen/Source/Game.cpp)
extern SBYTE game_paused_highlight;

// Loading-screen tips shown while the level loads.
// uc_orig: bullet_point (fallen/Source/Game.cpp)
extern CBYTE* bullet_point[NUM_BULLETS];

// Index and countdown for the currently-displayed loading tip.
// uc_orig: bullet_upto (fallen/Source/Game.cpp)
extern SLONG bullet_upto;

// uc_orig: bullet_counter (fallen/Source/Game.cpp)
extern SLONG bullet_counter;

// Scratch buffer used by GAME_map_draw() for the plan-view screenshot.
// uc_orig: screen_mem (fallen/Source/Game.cpp)
extern UBYTE screen_mem[640 * 3][480];

// Tick used to debounce the "leaving map" edge warning.
// uc_orig: already_warned_about_leaving_map (fallen/Source/Game.cpp)
// BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
extern uint64_t already_warned_about_leaving_map;

// When UC_TRUE, the overhead map is shown instead of the 3D view.
// uc_orig: draw_map_screen (fallen/Source/Game.cpp)
extern UBYTE draw_map_screen;

// When UC_TRUE, the game advances one tick per comma-key press.
// uc_orig: single_step (fallen/Source/Game.cpp)
extern UBYTE single_step;

// Active "Leave map?" dialog, or NULL if not shown.
// uc_orig: form_leave_map (fallen/Source/Game.cpp)
extern Form* form_leave_map;

// Frame counter since the "leave map" dialog appeared.
// uc_orig: form_left_map (fallen/Source/Game.cpp)
extern SLONG form_left_map;

// Debug render state (fudge mode) — last message/camera values used.
// uc_orig: last_fudge_message (fallen/Source/Game.cpp)
extern UWORD last_fudge_message;

// uc_orig: last_fudge_camera (fallen/Source/Game.cpp)
extern UWORD last_fudge_camera;

// Set to UC_TRUE during the credits/outro sequence after completing the final mission.
// uc_orig: the_end (fallen/Source/Game.cpp)
extern UBYTE the_end;

// Target frame rate read from config.ini "max_frame_rate" (default 30).
// uc_orig: env_frame_rate (fallen/Source/Game.cpp)
extern UWORD env_frame_rate;

// Ring buffer of the last 4 TICK_RATIO values for smooth tick averaging (vehicle movement).
// In the original these were file-scope statics in Game.cpp.
// uc_orig: tick_ratios (fallen/Source/Game.cpp)
extern SLONG tick_ratios[4];

// Write pointer into tick_ratios ring buffer (0..3).
// uc_orig: wptr (fallen/Source/Game.cpp)
extern SLONG wptr;

// Count of values written so far (0..4). Below 4 means buffer not yet full.
// uc_orig: number (fallen/Source/Game.cpp)
extern SLONG number;

// Running sum of all 4 tick_ratios values (for O(1) average).
// uc_orig: sum (fallen/Source/Game.cpp)
extern SLONG sum;

// File paths for gameplay recording and playback (.pkt) files.
// uc_orig: playback_name (fallen/Source/Game.cpp)
extern CBYTE* playback_name;

// uc_orig: verifier_name (fallen/Source/Game.cpp)
extern CBYTE* verifier_name;


#endif // GAME_GAME_GLOBALS_H
