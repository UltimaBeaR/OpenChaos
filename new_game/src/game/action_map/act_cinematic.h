#ifndef GAME_ACTION_MAP_ACT_CINEMATIC_H
#define GAME_ACTION_MAP_ACT_CINEMATIC_H

// =============================================================================
// ACT_CINE_* — action constants for cinematic / non-gameplay sequences: outro,
// playcuts (in-level cutscenes), video player. Skip-only semantics, no
// navigation needed.
//
// Naming rules, prefix table, suffix table → see
//   new_game_devlog/input_system/action_map/rules.md
// Step plan (this header is populated in step 3c.1 — pilot context) → see
//   new_game_devlog/input_system/action_map/plan.md
//
// Device codes used as values (KKEY_*, GBTN_*, ...) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// ---- Video player (FMV) skip ------------------------------------------------
// Pressing any of these keys skips the currently playing video. Multiple
// bindings for the same logical "skip video" action. Read in
// video_player.cpp::video_play (skip-on-input loop).
//
// Gamepad: video skip also fires on ANY gamepad button — implemented as a
// loop over rgbButtons[0..16] in video_player.cpp, not a single binding.
// No GBTN constant defined for that wildcard; the loop stays as-is.

constexpr int ACT_CINE_VIDEO_SKIP_1_KKEY = KKEY_ESCAPE;
constexpr int ACT_CINE_VIDEO_SKIP_2_KKEY = KKEY_ENTER;
constexpr int ACT_CINE_VIDEO_SKIP_3_KKEY = KKEY_SPACE;

// ---- Outro (credits / end sequence) skip ------------------------------------
// Read in outro_main.cpp::outro_main (top-level outro skip check). Skips the
// entire outro sequence and returns to the menu.

constexpr int ACT_CINE_OUTRO_SKIP_1_KKEY = KKEY_ESCAPE;
constexpr int ACT_CINE_OUTRO_SKIP_2_KKEY = KKEY_ENTER;
constexpr int ACT_CINE_OUTRO_SKIP_3_KKEY = KKEY_SPACE;
constexpr int ACT_CINE_OUTRO_SKIP_1_GBTN = GBTN_SOUTH; // DS: Cross, Xbox: A
constexpr int ACT_CINE_OUTRO_SKIP_2_GBTN = GBTN_EAST;  // DS: Circle, Xbox: B

// ---- Outro framework quit ---------------------------------------------------
// Read in outro_os.cpp::OS_process_messages — sets KEY_on[KEY_ESCAPE] which
// downstream code interprets as "leave outro". Distinct from OUTRO_SKIP above:
// SKIP is the top-level "user wants the credits over"; QUIT is the OS-layer
// signal used by various outro animation routines.
//
// GBTN_START is bound directly so the gamepad Start button closes the outro;
// it's read in parallel with KKEY_ESCAPE at the call site.

constexpr int ACT_CINE_OUTRO_QUIT_KKEY = KKEY_ESCAPE;
constexpr int ACT_CINE_OUTRO_QUIT_GBTN = GBTN_START; // DS: Options, Xbox: Start

// ---- In-mission cutscene (playcut) skip -------------------------------------
// Read in playcuts.cpp::play_cutscene as an extra rising-edge break out of the
// playcut loop. The playcut loop also calls hardware_input_continue() (see
// ACT_CINE_GENERIC_SKIP_* below), so this SPACE check is effectively an
// additional early-exit channel using the same edge-detect semantics.

constexpr int ACT_CINE_PLAYCUT_SKIP_KKEY = KKEY_SPACE;

// ---- Replay playback exit --------------------------------------------------
// In GS_PLAYBACK mode (watching a recorded replay), pressing any of these
// keys / any gamepad face button exits playback (sets GAME_STATE = 0). Read
// in game.cpp::playback_game_keys.

constexpr int ACT_CINE_PLAYBACK_EXIT_1_KKEY = KKEY_SPACE;
constexpr int ACT_CINE_PLAYBACK_EXIT_2_KKEY = KKEY_ENTER;
constexpr int ACT_CINE_PLAYBACK_EXIT_3_KKEY = KKEY_NUMPAD_ENTER;
// Gamepad: loop over buttons 0..9 (face buttons + L1/R1 etc.) — any rising
// edge exits playback. Same "any button wildcard" pattern as video skip in
// video_player.cpp; no single GBTN constant.

// ---- Generic "press anything to continue" -----------------------------------
// Read in game.cpp::hardware_input_continue. Used by playcuts and any other
// blocking screen that wants "press anything sensible to continue". Includes
// keyboard skip + gamepad Start/Triangle (read in parallel with the KKEY
// channel). Cross/A for confirm and R3 for select are already covered through
// the gameplay INPUT_MASK_JUMP / INPUT_MASK_SELECT branch in the same
// function and need no separate GBTN constants here.

constexpr int ACT_CINE_GENERIC_SKIP_1_KKEY = KKEY_SPACE;
constexpr int ACT_CINE_GENERIC_SKIP_2_KKEY = KKEY_ESCAPE;
constexpr int ACT_CINE_GENERIC_SKIP_3_KKEY = KKEY_Z;
constexpr int ACT_CINE_GENERIC_SKIP_4_KKEY = KKEY_X;
constexpr int ACT_CINE_GENERIC_SKIP_5_KKEY = KKEY_C;
constexpr int ACT_CINE_GENERIC_SKIP_6_KKEY = KKEY_ENTER;
constexpr int ACT_CINE_GENERIC_SKIP_1_GBTN = GBTN_START; // DS: Options, Xbox: Start
constexpr int ACT_CINE_GENERIC_SKIP_2_GBTN = GBTN_EAST;  // DS: Circle, Xbox: B

#endif // GAME_ACTION_MAP_ACT_CINEMATIC_H
