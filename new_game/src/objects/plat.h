#ifndef OBJECTS_PLAT_H
#define OBJECTS_PLAT_H

#include "engine/core/types.h"

// Moving platforms: dynamic mesh prims that follow waypoint paths.
// Platforms can patrol between waypoints or stop permanently.

// Forward declaration: Thing is defined in fallen/Headers/thing.h (not yet migrated).
struct Thing;

// Platform data attached to each CLASS_PLAT Thing.
// uc_orig: Plat (fallen/Headers/Plat.h)
struct Plat {
    UBYTE used;
    UBYTE colour;
    UBYTE group;
    UBYTE move;

    UBYTE state;
    UBYTE wspeed;   // Desired speed
    UBYTE speed;    // Current speed
    UBYTE flag;
    UWORD counter;  // Countdown timer in milliseconds
    UWORD waypoint;
};

// uc_orig: RPLAT_MAX_PLATS (fallen/Headers/Plat.h)
#define RPLAT_MAX_PLATS 32
// save_table slot determines actual max at runtime.
// uc_orig: PLAT_MAX_PLATS (fallen/Headers/Plat.h)
#define PLAT_MAX_PLATS (save_table[SAVE_TABLE_PLATS].Maximum)

// uc_orig: TO_PLAT (fallen/Headers/Plat.h)
#define TO_PLAT(x) &PLAT_plat[x]
// uc_orig: PLAT_NUMBER (fallen/Headers/Plat.h)
#define PLAT_NUMBER(x) (UWORD)(x - TO_PLAT(0))

// uc_orig: PlatPtr (fallen/Headers/Plat.h)
typedef struct Plat* PlatPtr;

// Movement mode constants.
// uc_orig: PLAT_MOVE_STILL (fallen/Headers/Plat.h)
#define PLAT_MOVE_STILL 0
// uc_orig: PLAT_MOVE_PATROL (fallen/Headers/Plat.h)
#define PLAT_MOVE_PATROL 1
// uc_orig: PLAT_MOVE_PATROL_RAND (fallen/Headers/Plat.h)
#define PLAT_MOVE_PATROL_RAND 2

// Public platform flag bits.
// uc_orig: PLAT_FLAG_LOCK_MOVE (fallen/Headers/Plat.h)
#define PLAT_FLAG_LOCK_MOVE (1 << 0)
// uc_orig: PLAT_FLAG_LOCK_ROT (fallen/Headers/Plat.h)
#define PLAT_FLAG_LOCK_ROT (1 << 1)
// uc_orig: PLAT_FLAG_BODGE_ROCKET (fallen/Headers/Plat.h)
#define PLAT_FLAG_BODGE_ROCKET (1 << 2)

// uc_orig: PLAT_init (fallen/Source/plat.cpp)
void PLAT_init(void);

// uc_orig: PLAT_process (fallen/Source/plat.cpp)
void PLAT_process(Thing* p_thing);

// uc_orig: PLAT_create (fallen/Headers/Plat.h)
UWORD PLAT_create(
    UBYTE colour,
    UBYTE group,
    UBYTE move,
    UBYTE flag,
    UBYTE speed,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z);

#endif // OBJECTS_PLAT_H
