#ifndef WORLD_NAVIGATION_NAV_H
#define WORLD_NAVIGATION_NAV_H

#include "core/types.h"

// 2D city navigation: produces waypoint paths between grid cells.
// The city is a 128×128 grid; waypoints are UWORD (x,z) grid coordinates.
// NAV_do() returns the index of the first waypoint in the path (linked list).

// uc_orig: NAV_WAYPOINT_FLAG_LAST (fallen/Headers/Nav.h)
#define NAV_WAYPOINT_FLAG_LAST   (1 << 0)
// uc_orig: NAV_WAYPOINT_FLAG_PULLUP (fallen/Headers/Nav.h)
// If a collision vector is hit going to this waypoint, perform a pull-up.
#define NAV_WAYPOINT_FLAG_PULLUP (1 << 1)

// uc_orig: NAV_Waypoint (fallen/Headers/Nav.h)
typedef struct
{
    UWORD x;
    UWORD z;
    UBYTE flag;
    UBYTE length; // number of waypoints remaining after this one
    UWORD next;

} NAV_Waypoint;

// uc_orig: NAV_MAX_WAYPOINTS (fallen/Headers/Nav.h)
#define NAV_MAX_WAYPOINTS 512

// uc_orig: NAV_waypoint (fallen/Source/Nav.cpp)
extern NAV_Waypoint NAV_waypoint[NAV_MAX_WAYPOINTS];

// uc_orig: NAV_WAYPOINT (fallen/Headers/Nav.h)
#define NAV_WAYPOINT(index) (&NAV_waypoint[index])

// uc_orig: NAV_FLAG_PULLUP (fallen/Headers/Nav.h)
#define NAV_FLAG_PULLUP (1 << 0)

// uc_orig: NAV_init (fallen/Headers/Nav.h)
void NAV_init(void);

// uc_orig: NAV_do (fallen/Headers/Nav.h)
// Returns a 2D navigation path from (x1,z1) to (x2,z2).
// Returns the index of the first NAV_Waypoint in the result chain.
UWORD NAV_do(UWORD x1, UWORD z1, UWORD x2, UWORD z2, UBYTE flag);

// uc_orig: NAV_waypoint_give (fallen/Headers/Nav.h)
void NAV_waypoint_give(UWORD index);

// uc_orig: NAV_path_give (fallen/Headers/Nav.h)
void NAV_path_give(UWORD index);

// uc_orig: NAV_path_draw (fallen/Headers/Nav.h)
void NAV_path_draw(UWORD startx, UWORD startz, UWORD path);

#endif // WORLD_NAVIGATION_NAV_H
