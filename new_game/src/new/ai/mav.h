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
// Pass ignore_warehouses=TRUE to skip warehouse interior height adjustments.
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

#endif // AI_MAV_H
