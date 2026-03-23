#ifndef AI_MAV_H
#define AI_MAV_H

#include "fallen/Headers/mav.h"
#include "ai/mav_globals.h"

// Deduplicates MAV_Opt records: finds an identical 4-byte option set in the pool and
// reuses it, or appends a new entry. Updates MAV_NAV(x,z) to the matching index.
// Also used by MAV_precalculate_warehouse_nav (still in old/).
// uc_orig: StoreMavOpts (fallen/Source/mav.cpp)
void StoreMavOpts(SLONG x, SLONG z, UBYTE* opt);

// Navigation grid initialization. Pre-populates the first 16 MAV_opt entries with
// all combinations of 4-direction plain-walk passability (indices 0-15).
// Required by INSIDE2_mav_nav_calc() which addresses these entries by fixed index.
// uc_orig: MAV_init (fallen/Source/mav.cpp)
void MAV_init(void);

// Builds the per-cell height map used by MAV_precalculate.
// Heights in units of 0x40 game-units (1 unit = 64 Y-coords).
// Pass ignore_warehouses=UC_TRUE to skip warehouse interior height adjustments.
// uc_orig: MAV_calc_height_array (fallen/Source/mav.cpp)
void MAV_calc_height_array(SLONG ignore_warehouses);

// Clears plain-walk (MAV_CAPS_GOTO) access into cell (x,z) from its 4 neighbours.
// uc_orig: MAV_turn_off_square (fallen/Source/mav.cpp)
void MAV_turn_off_square(SLONG x, SLONG z);

// Clears ALL movement capabilities into cell (x,z) from its 4 neighbours.
// Stronger than MAV_turn_off_square — used for slopes and NOGO cells.
// uc_orig: MAV_turn_off_whole_square (fallen/Source/mav.cpp)
void MAV_turn_off_whole_square(SLONG x, SLONG z);

// Clears car-nav bits into cell (x,z) from its 4 neighbours.
// uc_orig: MAV_turn_off_whole_square_car (fallen/Source/mav.cpp)
void MAV_turn_off_whole_square_car(SLONG x, SLONG z);

// Removes a wall/fence facet from the car-navigation grid.
// The facet is a line segment from (x1,z1) to (x2,z2) — horizontal or vertical only.
// uc_orig: MAV_remove_facet_car (fallen/Source/mav.cpp)
void MAV_remove_facet_car(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// Turns off movement in one direction from cell (mx,mz) (plain-walk only).
// uc_orig: MAV_turn_movement_off (fallen/Source/mav.cpp)
void MAV_turn_movement_off(UBYTE mx, UBYTE mz, UBYTE dir);

// Turns on plain-walk movement in one direction from cell (mx,mz).
// uc_orig: MAV_turn_movement_on (fallen/Source/mav.cpp)
void MAV_turn_movement_on(UBYTE mx, UBYTE mz, UBYTE dir);

// Main city-grid navigation build pass (PC/editor only, not PSX).
// Calls MAV_calc_height_array, then evaluates all 4-directional edges per cell:
// walk, climb-over fence, fall-off, pull-up, ladder, jump.
// Also populates the car-nav (MAV_CAR) bitfield and marks water tiles.
// uc_orig: MAV_precalculate (fallen/Source/mav.cpp)
void MAV_precalculate(void);

// Draws the navigation grid lines for the given map-square range. Debug/editor function.
// uc_orig: MAV_draw (fallen/Source/mav.cpp)
void MAV_draw(SLONG sx1, SLONG sz1, SLONG sx2, SLONG sz2);

// Returns UC_TRUE if you can walk in a straight line (GOTO-only) from (ax,az) to (bx,bz).
// On failure, MAV_last_mx/mz is the last reachable cell and MAV_dmx/dmz the blocked direction.
// uc_orig: MAV_can_i_walk (fallen/Source/mav.cpp)
SLONG MAV_can_i_walk(UBYTE ax, UBYTE az, UBYTE bx, UBYTE bz);

// Clears the visited flags in the given bounding box of the MAV_flag array.
// Rounds x1 down and x2 up to 4-byte (8-cell) boundaries for efficient SLONG clearing.
// uc_orig: MAV_clear_bbox (fallen/Source/mav.cpp)
void MAV_clear_bbox(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// Path smoothing: walks the MAV_node list from start backwards, tries MAV_can_i_walk
// for each GOTO waypoint to find the furthest directly reachable cell.
// Non-GOTO actions (jump, climb, etc.) are returned immediately without smoothing.
// uc_orig: MAV_get_first_action_from_nodelist (fallen/Source/mav.cpp)
MAV_Action MAV_get_first_action_from_nodelist(void);

// Path reconstruction after A* terminates. Traces from (end_x,end_z) back to
// MAV_start using stored MAV_dir/MAV_action. Fills MAV_node[] in reverse order.
// uc_orig: MAV_create_nodelist_from_pos (fallen/Source/mav.cpp)
void MAV_create_nodelist_from_pos(UBYTE end_x, UBYTE end_z);

// Runs the MAV greedy best-first search from (me_x,me_z) toward (dest_x,dest_z).
// caps filters which movement types the agent can use (MAV_CAPS_* bitmask).
// Returns a MAV_Action describing the first step to take.
// uc_orig: MAV_do (fallen/Source/mav.cpp)
MAV_Action MAV_do(SLONG me_x, SLONG me_z, SLONG dest_x, SLONG dest_z, UBYTE caps);

// Returns UC_TRUE if world-space point (x,y,z) is below the MAVHEIGHT terrain surface.
// uc_orig: MAV_inside (fallen/Source/mav.cpp)
SLONG MAV_inside(SLONG x, SLONG y, SLONG z);

// Fast terrain LOS using the MAVHEIGHT grid (~1 cell step resolution).
// Returns UC_FALSE if any step is underground. Failure point saved in MAV_height_los_fail_*.
// uc_orig: MAV_height_los_fast (fallen/Source/mav.cpp)
SLONG MAV_height_los_fast(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

// Slow terrain LOS — like MAV_height_los_fast but uses WARE_inside() when ware != 0.
// Skips the source point (starts from x1+dx) to avoid self-occlusion.
// uc_orig: MAV_height_los_slow (fallen/Source/mav.cpp)
SLONG MAV_height_los_slow(SLONG ware, SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

// Builds the navigation grid for a single warehouse interior.
// Temporarily redirects MAV_nav to WARE_nav[ww->nav] for the build pass.
// uc_orig: MAV_precalculate_warehouse_nav (fallen/Source/mav.cpp)
void MAV_precalculate_warehouse_nav(UBYTE ware);

// Returns the raw capability bitmask for one direction from a given cell.
// Returns 0 if out of bounds.
// uc_orig: MAV_get_caps (fallen/Source/mav.cpp)
UBYTE MAV_get_caps(UBYTE x, UBYTE z, UBYTE dir);

// Enables car movement through cell (mx,mz) in one direction at runtime.
// uc_orig: MAV_turn_car_movement_on (fallen/Source/mav.cpp)
void MAV_turn_car_movement_on(UBYTE mx, UBYTE mz, UBYTE dir);

// Disables car movement through cell (mx,mz) in one direction at runtime.
// uc_orig: MAV_turn_car_movement_off (fallen/Source/mav.cpp)
void MAV_turn_car_movement_off(UBYTE mx, UBYTE mz, UBYTE dir);

// Inline bit-manipulation helpers for the A* flag/dir arrays.
// Each stores 2 cells per byte (flag) or 4 cells per byte (dir).

// uc_orig: MAV_visited_set (fallen/Source/mav.cpp)
inline void MAV_visited_set(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));
    SLONG byte = x >> 1;
    SLONG bit = 8 << ((x & 0x1) << 2);
    MAV_flag[z][byte] |= bit;
}

// uc_orig: MAV_visited_get (fallen/Source/mav.cpp)
inline SLONG MAV_visited_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));
    SLONG byte = x >> 1;
    SLONG bit = 8 << ((x & 0x1) << 2);
    return (MAV_flag[z][byte] & bit);
}

// Also sets the visited flag.
// uc_orig: MAV_action_set (fallen/Source/mav.cpp)
inline void MAV_action_set(SLONG x, SLONG z, SLONG dir)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));
    SLONG byte = x >> 1;
    SLONG shift = (x & 0x1) << 2;
    dir |= 0x08; // Set the visited flag too.
    MAV_flag[z][byte] &= ~(0x7 << shift);
    MAV_flag[z][byte] |= (dir << shift);
}

// uc_orig: MAV_action_get (fallen/Source/mav.cpp)
inline SLONG MAV_action_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));
    SLONG byte = x >> 1;
    SLONG shift = (x & 0x1) << 2;
    return ((MAV_flag[z][byte] >> shift) & 0x7);
}

// uc_orig: MAV_dir_set (fallen/Source/mav.cpp)
inline void MAV_dir_set(SLONG x, SLONG z, SLONG dir)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));
    SLONG byte = x >> 2;
    SLONG shift = (x & 0x3) << 1;
    MAV_dir[z][byte] &= ~(0x3 << shift);
    MAV_dir[z][byte] |= (dir << shift);
}

// uc_orig: MAV_dir_get (fallen/Source/mav.cpp)
inline SLONG MAV_dir_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));
    SLONG byte = x >> 2;
    SLONG shift = (x & 0x3) << 1;
    return ((MAV_dir[z][byte] >> shift) & 0x3);
}

#endif // AI_MAV_H
