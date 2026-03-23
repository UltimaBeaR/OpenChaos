#ifndef WORLD_ENVIRONMENT_ENTER_H
#define WORLD_ENVIRONMENT_ENTER_H

#include "core/types.h"

// Building entry system: determines if a Thing can enter/leave a building,
// and sets up the interior view (ID module) for a given storey.

// Result of an entry check: the dbuilding index and interior spawn position.
// uc_orig: ENTER_Okay (fallen/Headers/Enter.h)
typedef struct
{
    SLONG dbuilding; // The dbuilding index, or 0 if entry is not possible.
    UBYTE map_x;
    UBYTE map_z;

} ENTER_Okay;

// Returns whether the thing at index 'me' can enter a nearby building.
// If possible, fills in dbuilding and the interior spawn coordinates.
// uc_orig: ENTER_can_i (fallen/Headers/Enter.h)
ENTER_Okay ENTER_can_i(THING_INDEX me);

// Returns TRUE if the thing can leave the building it is in.
// If so, fills in map_x/map_z with the exterior exit position.
// uc_orig: ENTER_leave (fallen/Headers/Enter.h)
SLONG ENTER_leave(THING_INDEX me, UBYTE* map_x, UBYTE* map_z);

// Returns the floor height of the ground storey and the top storey of a building.
// Heights are in quarter-mapsquare units (multiples of 0x40).
// uc_orig: ENTER_get_extents (fallen/Headers/Enter.h)
void ENTER_get_extents(
    SLONG  dbuilding,
    SLONG* height_ground_floor,
    SLONG* height_of_top_storey);

// Sets up the ID module to show the interior of the given storey of a building.
// 'height' is the floor height in quarter-mapsquare units.
// If find_best_layout is true, the ID module searches for the best layout seed
// and stores it in dbuilding.SeedInside.
// Returns TRUE on success.
// uc_orig: ENTER_setup (fallen/Headers/Enter.h)
SLONG ENTER_setup(SLONG dbuilding, SLONG height, UBYTE furnished, UBYTE find_best_layout);

#endif // WORLD_ENVIRONMENT_ENTER_H
