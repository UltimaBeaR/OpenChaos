#ifndef NAVIGATION_WAND_H
#define NAVIGATION_WAND_H

#include "engine/core/types.h"

// uc_orig: WAND_init (fallen/Headers/wand.h)
// Marks walkable squares near roads with PAP_FLAG_WANDER.
void WAND_init(void);

// uc_orig: WAND_get_next_place (fallen/Headers/wand.h)
// Picks a nearby wander destination for p_person, written to wand_world_x/z.
void WAND_get_next_place(
    Thing* p_person,
    SLONG* wand_world_x,
    SLONG* wand_world_z);

// uc_orig: WAND_square_is_wander (fallen/Headers/wand.h)
// Returns UC_TRUE if the given hi-res map square has PAP_FLAG_WANDER set.
SLONG WAND_square_is_wander(SLONG map_x, SLONG map_z);

// uc_orig: WAND_draw (fallen/Headers/wand.h)
// Debug: draws crosses over wander squares near (map_x, map_z).
void WAND_draw(SLONG map_x, SLONG map_z);

// Not declared in the original wand.h but used externally via forward declarations:

// uc_orig: WAND_find_good_start_point (fallen/Source/wand.cpp)
// Finds a wander square at approximately DRAW_DIST away from the player.
SLONG WAND_find_good_start_point(SLONG* mapx, SLONG* mapz);

// uc_orig: WAND_find_good_start_point_near (fallen/Source/wand.cpp)
// Like WAND_find_good_start_point but searches near the given position.
SLONG WAND_find_good_start_point_near(SLONG* mapx, SLONG* mapz);

// uc_orig: WAND_find_good_start_point_for_car (fallen/Source/wand.cpp)
// Finds a valid spawn point on a road for a vehicle.
SLONG WAND_find_good_start_point_for_car(SLONG* posx, SLONG* posz, SLONG* yaw, SLONG anywhere);

#endif // NAVIGATION_WAND_H
