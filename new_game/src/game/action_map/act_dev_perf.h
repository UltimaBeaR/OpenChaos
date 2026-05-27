#ifndef GAME_ACTION_MAP_ACT_DEV_PERF_H
#define GAME_ACTION_MAP_ACT_DEV_PERF_H

// =============================================================================
// ACT_DEV_PERF_* — action constants for compile-time-gated developer
// performance-diagnostic hotkeys. These are NOT runtime-gated (no
// allow_debug_keys / bangunsnotgames check) — they're only present in the
// binary when the corresponding `#if OC_DEBUG_PERF` / `#if OC_DEBUG_PHYSICS_TIMING`
// compile flag is set. Regular release builds don't include this code.
//
// Naming rules → see new_game_devlog/input_system/action_map/rules.md
//
// Device codes (KKEY_*, DBTN_*) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// All perf-debug keys are compile-time gated (`#if OC_DEBUG_PERF` /
// `#if OC_DEBUG_PHYSICS_TIMING`) — they do not exist in release builds,
// and they do NOT go through the F1-modifier scheme (that's for runtime
// bangunsnotgames keys). Moved off the number row (1..5) and 0/9 because
// 1..8 are weapon hotkeys in gameplay. Physics tuning goes onto the
// numpad (compile-time dev tool — devs running these flags will have a
// numpad even if the user's target hardware doesn't). Perf-panel toggles
// go onto free F-keys F6/F7 (formerly camera-mode keys, deleted with
// the new mouse-driven camera).

// ---- OC_DEBUG_PERF panel hotkeys (perf_diag.cpp) ---------------------------

// F6: toggle the perf-diag panel visibility (was 1-row "4").
constexpr int ACT_DEV_PERF_TOGGLE_PANEL_KKEY = KKEY_F6;

// F7: cycle the "short window" averaging-window option (was 1-row "5").
constexpr int ACT_DEV_PERF_CYCLE_SHORT_WINDOW_KKEY = KKEY_F7;

// ---- OC_DEBUG_PHYSICS_TIMING hotkeys (game.cpp::check_debug_timing_keys) ----

// Numpad 1: toggle physics Hz between design rate (UC_PHYSICS_DESIGN_HZ) and low (5).
constexpr int ACT_DEV_PERF_TOGGLE_PHYS_HZ_KKEY = KKEY_NUMPAD_1;

// Numpad 2 (keyboard) or DualSense touchpad click: toggle render FPS cap between
// default cap and 30 Hz (matches UC_VISUAL_CADENCE_HZ).
constexpr int ACT_DEV_PERF_TOGGLE_RENDER_FPS_KKEY = KKEY_NUMPAD_2;
constexpr int ACT_DEV_PERF_TOGGLE_RENDER_FPS_DBTN = DBTN_TOUCHPAD;

// Numpad 3: toggle render interpolation on / off.
constexpr int ACT_DEV_PERF_TOGGLE_INTERP_KKEY = KKEY_NUMPAD_3;

// Numpad 9: decrement physics Hz by 1 (saturated to 1..design-hz range).
constexpr int ACT_DEV_PERF_PHYS_HZ_DEC_KKEY = KKEY_NUMPAD_9;

// Numpad 0: increment physics Hz by 1 (saturated to 1..design-hz range).
constexpr int ACT_DEV_PERF_PHYS_HZ_INC_KKEY = KKEY_NUMPAD_0;

#endif // GAME_ACTION_MAP_ACT_DEV_PERF_H
