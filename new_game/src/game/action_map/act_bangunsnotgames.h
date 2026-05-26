#ifndef GAME_ACTION_MAP_ACT_BANGUNSNOTGAMES_H
#define GAME_ACTION_MAP_ACT_BANGUNSNOTGAMES_H

// =============================================================================
// ACT_BANG_* — action constants for debug-key hotkeys active ONLY after the
// player types "bangunsnotgames" into the dev console (allow_debug_keys flag).
// Includes F1 (legend), F2 (CRT toggle), F8 (single-step), F10 (far-facet
// debug), F11 (input debug panel), Ctrl+* (various debug toggles), Ctrl+Q
// (quit), Ctrl+L (outside camera), and so on.
//
// Not "general debugging" — this is a specific MODE that the player explicitly
// enables. Constants here are KKEY-only (no gamepad bindings for these).
//
// Naming rules, prefix table, suffix table → see
//   new_game_devlog/input_system/action_map/rules.md
// Step plan (this header is populated in step 3c.3) → see
//   new_game_devlog/input_system/action_map/plan.md
//
// Device codes used as values (KKEY_*) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// (Constants will be filled here in step 3c.3 — bangunsnotgames.)

#endif // GAME_ACTION_MAP_ACT_BANGUNSNOTGAMES_H
