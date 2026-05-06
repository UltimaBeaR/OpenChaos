#ifndef GAME_GAME_GLOBALS_H
#define GAME_GAME_GLOBALS_H

#include "engine/platform/uc_common.h"
#include "things/core/thing.h" // Pool type definitions (Vehicle, Person, Animal, etc.) needed by game_types.h
#include "game/game_types.h"
#include "ui/menus/widget.h"
#include "ui/menus/widget_globals.h"

// uc_orig: game_paused_key (fallen/Source/Game.cpp)
extern UBYTE game_paused_key;

// Index and countdown for the currently-displayed loading tip.
// uc_orig: bullet_upto (fallen/Source/Game.cpp)
extern SLONG bullet_upto;

// uc_orig: bullet_counter (fallen/Source/Game.cpp)
extern SLONG bullet_counter;

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

// Runtime-adjustable physics tick rate. Default = UC_PHYSICS_DESIGN_HZ
// (= 20 Hz, the design rate hardcoded into Vehicle GRAVITY, EWAY mission
// timer, network sync, etc — see game_types.h). Diagnostic hotkeys
// (1/9/0) wiggle this value at runtime to verify physics<->render
// decoupling: visual wall-clock effects (rain, drip, puddle, mist, ribbon,
// sparks, VISUAL_TURN-gated visuals) MUST remain unchanged when this
// varies — any deviation is a decoupling bug. Production logic must read
// this variable (not the constant directly) so diagnostic changes
// propagate. Clamped 1..UC_PHYSICS_DESIGN_HZ.
//
// TODO(fps_unlock): once decoupling is fully verified across the whole
// codebase and all rate-dependent code is honest about which rate it
// uses, this variable can either be removed (replaced everywhere by
// UC_PHYSICS_DESIGN_HZ) or hidden behind a debug-build flag while the
// hotkeys stay as a dev-only diagnostic aid.
extern SLONG g_physics_hz;

// Default production render fps cap. We don't ship "unlimited" because
// unbounded redraws in the main menu/attract burn the GPU for nothing on
// some setups. 300 is well above any reasonable display refresh, so
// gameplay still feels uncapped while the GPU stays well-behaved when the
// scene is trivial. lock_frame_rate(0) still means "no cap" in the API —
// a debug overlay or future config could opt back into 0 if desired.
static constexpr SLONG RENDER_FPS_DEFAULT_CAP = 300;

// Runtime-adjustable render frame cap in fps. Default = RENDER_FPS_DEFAULT_CAP;
// 0 = unlimited (no cap) is still supported by lock_frame_rate() but not
// the production default. Diagnostic hotkey 2 toggles default<->25 to test
// how visuals behave at low render rate (interpolation correctness, stutter
// on cap transitions). Used by lock_frame_rate() in every render loop
// (main game, FMV, attract, outro, cutscenes).
extern SLONG g_render_fps_cap;

// VISUAL_TURN is declared in game_types.h alongside GAME_TURN so any
// consumer of GAME_TURN already sees both counters through the same
// include. Helper to advance it lives here.

// Advance VISUAL_TURN by one or more ticks based on wall-clock dt. Call
// once per render frame from any render loop that displays game-world
// visuals (main game loop, cutscene playback). Free-running across
// loops — never reset between frames or scenes.
void visual_turn_tick(float dt_ms);

// Wall-clock dt of the current render frame in milliseconds. Updated each
// iteration of every render loop (game.cpp main loop, playcuts.cpp cutscene
// loop). Read by render-side visual effects that must remain frame-rate
// independent: rain droplet density, per-puddle rain-drip spawn probability.
// Default 33.33 ms (one 30 Hz frame) — used before the first render tick.
extern float g_frame_dt_ms;

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

#endif // GAME_GAME_GLOBALS_H
