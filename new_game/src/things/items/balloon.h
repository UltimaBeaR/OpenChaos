#ifndef THINGS_ITEMS_BALLOON_H
#define THINGS_ITEMS_BALLOON_H

#include "engine/core/types.h"

// uc_orig: BALLOON_Point (fallen/Headers/balloon.h)
typedef struct {
    SLONG x;
    SLONG y;
    SLONG z;
    SLONG dx;
    SLONG dy;
    SLONG dz;
} BALLOON_Point;

// uc_orig: BALLOON_POINTS_PER_BALLOON (fallen/Headers/balloon.h)
#define BALLOON_POINTS_PER_BALLOON 4

// uc_orig: BALLOON_TYPE_UNUSED (fallen/Headers/balloon.h)
#define BALLOON_TYPE_UNUSED 0
// uc_orig: BALLOON_TYPE_YELLOW (fallen/Headers/balloon.h)
#define BALLOON_TYPE_YELLOW 1
// uc_orig: BALLOON_TYPE_RED (fallen/Headers/balloon.h)
#define BALLOON_TYPE_RED 2
// uc_orig: BALLOON_TYPE_NUMBER (fallen/Headers/balloon.h)
#define BALLOON_TYPE_NUMBER 3

// uc_orig: BALLOON_Balloon (fallen/Headers/balloon.h)
typedef struct {
    UBYTE type;
    UBYTE next; // Next balloon in linked list attached to this person.
    UWORD yaw; // 0xffff => This balloon is unused.
    UWORD pitch;
    UWORD thing; // The thing this balloon is attached to.
    BALLOON_Point bp[BALLOON_POINTS_PER_BALLOON];
} BALLOON_Balloon;

// uc_orig: BALLOON_MAX_BALLOONS (fallen/Headers/balloon.h)
#define BALLOON_MAX_BALLOONS 32

// uc_orig: BALLOON_init (fallen/Source/balloon.cpp)
void BALLOON_init(void);

// uc_orig: BALLOON_create (fallen/Source/balloon.cpp)
UBYTE BALLOON_create(UWORD thing, UBYTE type);

// uc_orig: BALLOON_release (fallen/Source/balloon.cpp)
void BALLOON_release(UBYTE balloon);

// uc_orig: BALLOON_find_grab (fallen/Source/balloon.cpp)
void BALLOON_find_grab(UWORD thing);

// uc_orig: BALLOON_process (fallen/Source/balloon.cpp)
void BALLOON_process(void);

#endif // THINGS_ITEMS_BALLOON_H
