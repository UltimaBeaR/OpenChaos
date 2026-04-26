#ifndef GAME_GAME_TICK_H
#define GAME_GAME_TICK_H

// Per-frame game controller: inventory management, music context, danger calculation,
// debug console, and the main process_controls() dispatcher.
// Corresponds to fallen/Source/Controls.cpp.

#include "engine/platform/uc_common.h"                 // Basic types: SLONG, UBYTE, etc.

// Forward declaration (full type from Game.h, which includes this header transitively).
struct Thing;

// uc_orig: parse_console (fallen/Source/Controls.cpp)
void parse_console(CBYTE* str);

// uc_orig: tga_dump (fallen/Source/Controls.cpp)
void tga_dump(void);

// uc_orig: plan_view_shot (fallen/Source/Controls.cpp)
void plan_view_shot(void);

// uc_orig: can_i_draw_this_special (fallen/Source/Controls.cpp)
SLONG can_i_draw_this_special(Thing* p_special);

// uc_orig: CONTROLS_set_inventory (fallen/Source/Controls.cpp)
void CONTROLS_set_inventory(Thing* darci, Thing* player, SLONG count);

// uc_orig: CONTROLS_get_selected_item (fallen/Source/Controls.cpp)
SBYTE CONTROLS_get_selected_item(Thing* darci, Thing* player);

// uc_orig: CONTROLS_get_best_item (fallen/Source/Controls.cpp)
SBYTE CONTROLS_get_best_item(Thing* darci, Thing* player);

// uc_orig: CONTROLS_new_inventory (fallen/Source/Controls.cpp)
SLONG CONTROLS_new_inventory(Thing* darci, Thing* player);

// uc_orig: CONTROLS_rot_inventory (fallen/Source/Controls.cpp)
void CONTROLS_rot_inventory(Thing* darci, Thing* player, SBYTE dir, SLONG pull_it_out_ooooerrr);

// uc_orig: context_music (fallen/Source/Controls.cpp)
void context_music(void);

// uc_orig: set_danger_level (fallen/Source/Controls.cpp)
void set_danger_level(void);

// uc_orig: process_controls (fallen/Source/Controls.cpp)
void process_controls(void);

#endif // GAME_GAME_TICK_H
