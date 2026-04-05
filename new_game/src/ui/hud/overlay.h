#ifndef UI_HUD_OVERLAY_H
#define UI_HUD_OVERLAY_H

#include "engine/platform/uc_common.h"
#include "game/game_types.h"

// uc_orig: OVERLAY_handle (fallen/Source/overlay.cpp)
extern void OVERLAY_handle(void);

// uc_orig: track_enemy (fallen/Source/overlay.cpp)
extern void track_enemy(Thing* p_thing);

// uc_orig: track_gun_sight (fallen/Source/overlay.cpp)
extern void track_gun_sight(Thing* p_thing, SLONG accuracy);

// uc_orig: init_overlay (fallen/Source/overlay.cpp)
extern void init_overlay(void);

// uc_orig: overlay_beacons (fallen/Source/overlay.cpp)
extern void overlay_beacons(void);

// uc_orig: add_damage_value (fallen/Source/overlay.cpp)
extern void add_damage_value(SWORD x, SWORD y, SWORD z, SLONG value);

// uc_orig: add_damage_text (fallen/Source/overlay.cpp)
extern void add_damage_text(SWORD x, SWORD y, SWORD z, CBYTE* text);

// uc_orig: add_damage_value_thing (fallen/Source/overlay.cpp)
extern void add_damage_value_thing(Thing* p_thing, SLONG value);

// uc_orig: should_i_add_message (fallen/Source/overlay.cpp)
extern SLONG should_i_add_message(SLONG type);

// Queue a view line (shooter→target) for deferred rendering during overlay pass.
// Called from pcom during game tick; drawn after POLY_frame_init in render pass.
extern void OVERLAY_queue_view_line(Thing* shooter, Thing* target);

#endif // UI_HUD_OVERLAY_H
