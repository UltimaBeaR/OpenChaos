#ifndef AI_MAV_ACTION_H
#define AI_MAV_ACTION_H

#include "core/types.h"

// Single navigation action result returned by MAV_do().
// Describes one step the NPC should take to reach its destination:
// which kind of movement (MAV_ACTION_*), direction (MAV_DIR_*),
// and the target map-square coordinates.
// uc_orig: MAV_Action (fallen/Headers/Structs.h)
typedef struct
{
    UBYTE action;
    UBYTE dir;
    UBYTE dest_x;
    UBYTE dest_z;

} MAV_Action;

#endif // AI_MAV_ACTION_H
