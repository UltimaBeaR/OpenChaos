#ifndef WORLD_ENVIRONMENT_STAIR_H
#define WORLD_ENVIRONMENT_STAIR_H

#include "world/environment/id.h"

// Stair placement algorithm for multi-floor buildings.
// Call sequence: STAIR_init -> STAIR_set_bounding_box -> for each storey: STAIR_storey_new +
// STAIR_storey_wall calls + STAIR_storey_finish -> STAIR_calculate -> STAIR_get.

// uc_orig: STAIR_init (fallen/Headers/stair.h)
void STAIR_init(void);

// uc_orig: STAIR_set_bounding_box (fallen/Headers/stair.h)
void STAIR_set_bounding_box(UBYTE x1, UBYTE z1, UBYTE x2, UBYTE z2);

// uc_orig: STAIR_storey_new (fallen/Headers/stair.h)
void STAIR_storey_new(SLONG handle, UBYTE height);
// uc_orig: STAIR_storey_wall (fallen/Headers/stair.h)
void STAIR_storey_wall(UBYTE x1, UBYTE z1, UBYTE x2, UBYTE z2, SLONG opposite);
// uc_orig: STAIR_storey_finish (fallen/Headers/stair.h)
SLONG STAIR_storey_finish(void);

// uc_orig: STAIR_calculate (fallen/Headers/stair.h)
void STAIR_calculate(UWORD seed);

// uc_orig: STAIR_get (fallen/Headers/stair.h)
SLONG STAIR_get(SLONG handle, ID_Stair** stair, SLONG* num_stairs);

#endif // WORLD_ENVIRONMENT_STAIR_H
