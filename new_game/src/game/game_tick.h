#ifndef GAME_GAME_TICK_H
#define GAME_GAME_TICK_H

// Per-frame game controller: inventory management, music context, danger calculation,
// debug console, and the main process_controls() dispatcher.
// Corresponds to fallen/Source/Controls.cpp.

#include "engine/platform/uc_common.h" // Basic types: SLONG, UBYTE, etc.

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

// uc_orig: CONTROLS_new_inventory (fallen/Source/Controls.cpp)
SLONG CONTROLS_new_inventory(Thing* darci, Thing* player);

// OpenChaos: canonical ordered weapon list (pistol, AK, shotgun, grenade, bat,
// knife, other drawables, fist-last). Single source of truth for the inventory
// popup draw order AND the scroll. Fills `out` with item tokens (SPECIAL_GUN for
// the pistol, SpecialType for specials, SPECIAL_NONE for the fist), owned only,
// up to `max` entries; returns the count.
SLONG CONTROLS_build_weapon_list(Thing* darci, CBYTE* out, SLONG max);

// OpenChaos: the weapon Darci is currently using OR actively drawing
// (SPECIAL_GUN for the pistol, SpecialType for specials, SPECIAL_NONE for
// fists). Accounts for the in-progress draw animation so callers see the
// just-selected weapon on the SAME frame instead of a transient "fists" while
// the draw plays. Single source of truth shared by the equip/scroll logic and
// the inventory popup highlight.
SLONG CONTROLS_current_weapon_type(Thing* darci);

// OpenChaos: the special Thing Darci is currently using OR actively drawing
// (NULL on the pistol or fists). In-progress-draw aware like the function above;
// needed where the item's own Thing is required (e.g. ammo/timer readout).
Thing* CONTROLS_current_special_thing(Thing* darci);

// uc_orig: context_music (fallen/Source/Controls.cpp)
void context_music(void);

// uc_orig: set_danger_level (fallen/Source/Controls.cpp)
void set_danger_level(void);

// uc_orig: process_controls (fallen/Source/Controls.cpp)
void process_controls(void);

#endif // GAME_GAME_TICK_H
