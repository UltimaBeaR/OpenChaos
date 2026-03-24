#ifndef MISSIONS_GAME_H
#define MISSIONS_GAME_H

#include "missions/game_globals.h"
#include "ui/menus/widget.h"
#include "ui/menus/widget_globals.h"

// Stops all music and sound effects immediately (called on scene transitions).
// uc_orig: stop_all_fx_and_music (fallen/Source/Game.cpp)
void stop_all_fx_and_music(void);

// Loads global data that persists across levels: animation tables, primitives.
// uc_orig: global_load (fallen/Source/Game.cpp)
void global_load(void);

// Initialises all engine subsystems, opens the display, and sets up audio/input.
// Sets GAME_STATE to GS_ATTRACT_MODE on success.
// uc_orig: game_startup (fallen/Source/Game.cpp)
void game_startup(void);

// Shuts down the engine: closes display, kills networking, frees animation resources.
// uc_orig: game_shutdown (fallen/Source/Game.cpp)
void game_shutdown(void);

// Loads a mission by name without saving game state (used to generate texture clumps).
// uc_orig: make_texture_clumps (fallen/Source/Game.cpp)
BOOL make_texture_clumps(CBYTE* mission_name);

// Initialises a single mission: seeds RNG, opens playback file, loads level via ELEV_load_user.
// Returns nonzero if the level loaded successfully.
// uc_orig: game_init (fallen/Source/Game.cpp)
BOOL game_init(void);

// Tears down a mission: stops audio, frees rendering caches, closes playback file.
// uc_orig: game_fini (fallen/Source/Game.cpp)
void game_fini(void);

// Main entry point: calls game_startup, then loops through attract/play/editor states,
// then calls game_shutdown.
// uc_orig: game (fallen/Source/Game.cpp)
void game(void);

// Draws the overhead map view: plan_view_shot → overlay_beacons → blit to display.
// uc_orig: GAME_map_draw (fallen/Source/Game.cpp)
void GAME_map_draw(void);

// Widget callback for the "Leave map?" dialog. Returns UC_TRUE when a button is pushed.
// uc_orig: leave_map_form_proc (fallen/Source/Game.cpp)
BOOL leave_map_form_proc(Form* form, Widget* widget, SLONG message);

// Advances the loading-screen tip display (cycles every 250 frames with fade-in).
// uc_orig: process_bullet_points (fallen/Source/Game.cpp)
void process_bullet_points(void);

// Busy-waits until the current frame has taken at least 1000/fps milliseconds.
// uc_orig: lock_frame_rate (fallen/Source/Game.cpp)
void lock_frame_rate(SLONG fps);

// Processes and draws the "Leave map?" dialog; sets GAME_STATE=0 if confirmed.
// uc_orig: do_leave_map_form (fallen/Source/Game.cpp)
void do_leave_map_form(void);

// Submits the current frame to the display (screenshot hook, debug text, blit/flip).
// uc_orig: screen_flip (fallen/Source/Game.cpp)
void screen_flip(void);

// In GS_PLAYBACK mode: breaks playback when the player presses any button.
// uc_orig: playback_game_keys (fallen/Source/Game.cpp)
void playback_game_keys(void);

// Handles debug hotkeys (Ctrl+Q=quit, quote=single-step, comma=step-once).
// Returns 1 if the game should exit immediately.
// uc_orig: special_keys (fallen/Source/Game.cpp)
SLONG special_keys(void);

// Updates audio each frame: music mode, listener position, ambient/weather sounds.
// uc_orig: handle_sfx (fallen/Source/Game.cpp)
void handle_sfx(void);

// Returns UC_FALSE if the simulation should pause this frame (tutorial, pause menu, etc.).
// uc_orig: should_i_process_game (fallen/Source/Game.cpp)
SLONG should_i_process_game(void);

// Calls AENG_draw (or the map view), and processes the "Leave map?" dialog if open.
// uc_orig: draw_screen (fallen/Source/Game.cpp)
void draw_screen(void);

// Returns 1 if the player pressed the "replay" key (R on PC, Triangle on PSX).
// uc_orig: hardware_input_replay (fallen/Source/Game.cpp)
SLONG hardware_input_replay(void);

// Returns 1 if the player pressed a "continue" key (Space/Esc/gamepad face buttons).
// uc_orig: hardware_input_continue (fallen/Source/Game.cpp)
SLONG hardware_input_continue(void);

// Per-frame game loop for a single mission. Returns 0 to go back to attract mode,
// 1 to exit the application entirely.
// uc_orig: game_loop (fallen/Source/Game.cpp)
UBYTE game_loop(void);

// Resets the 4-frame tick smoothing buffer (called at mission start).
// uc_orig: ResetSmoothTicks (fallen/Source/Game.cpp)
void ResetSmoothTicks(void);

// Returns raw_ticks averaged over the last 4 frames to smooth vehicle movement.
// uc_orig: SmoothTicks (fallen/Source/Game.cpp)
SLONG SmoothTicks(SLONG raw_ticks);

#endif // MISSIONS_GAME_H
