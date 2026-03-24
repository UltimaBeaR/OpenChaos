#ifndef WORLD_ENVIRONMENT_DOOR_H
#define WORLD_ENVIRONMENT_DOOR_H

#include "engine/core/types.h"

// uc_orig: DOOR_Door (fallen/Headers/door.h)
typedef struct
{
    UWORD facet; // 0 = unused slot

} DOOR_Door;

// uc_orig: DOOR_MAX_DOORS (fallen/Headers/door.h)
#define DOOR_MAX_DOORS 4

// uc_orig: DOOR_open (fallen/Headers/door.h)
void DOOR_open(SLONG world_x, SLONG world_z);

// uc_orig: DOOR_shut (fallen/Headers/door.h)
void DOOR_shut(SLONG world_x, SLONG world_z);

// uc_orig: DOOR_process (fallen/Headers/door.h)
void DOOR_process(void);

#endif // WORLD_ENVIRONMENT_DOOR_H
