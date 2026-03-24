#ifndef WORLD_ENVIRONMENT_WARE_GLOBALS_H
#define WORLD_ENVIRONMENT_WARE_GLOBALS_H

#include <string.h>
#include "engine/core/types.h"
#include "world/environment/ware.h"

// All warehouse descriptor records. Index 0 unused; 1..WARE_ware_upto-1 are valid.
// uc_orig: WARE_ware (fallen/Source/ware.cpp)
extern WARE_Ware* WARE_ware;
// uc_orig: WARE_ware_upto (fallen/Source/ware.cpp)
extern UWORD WARE_ware_upto;

// Flat pool of warehouse navigation data (MAV_Opt indices), shared across all warehouses.
// uc_orig: WARE_nav (fallen/Source/ware.cpp)
extern UWORD* WARE_nav;
// uc_orig: WARE_nav_upto (fallen/Source/ware.cpp)
extern UWORD WARE_nav_upto;

// Flat pool of per-cell floor heights inside warehouses (SBYTE = MAVHEIGHT value).
// uc_orig: WARE_height (fallen/Source/ware.cpp)
extern SBYTE* WARE_height;
// uc_orig: WARE_height_upto (fallen/Source/ware.cpp)
extern UWORD WARE_height_upto;

// Flat pool of rooftop texture indices for all warehouses.
// uc_orig: WARE_rooftex (fallen/Source/ware.cpp)
extern UWORD* WARE_rooftex;
// uc_orig: WARE_rooftex_upto (fallen/Source/ware.cpp)
extern UWORD WARE_rooftex_upto;

// Non-zero while the player is inside a warehouse; holds the warehouse index.
// uc_orig: WARE_in (fallen/Source/ware.cpp)
extern UBYTE WARE_in;

// Per-mapsquare rooftop texture table loaded from the .map file during WARE_init.
// Indexed as WARE_roof_tex[hi_x][hi_z].
// uc_orig: WARE_roof_tex (fallen/Source/ware.cpp)
extern UWORD WARE_roof_tex[128][128]; // PAP_SIZE_HI x PAP_SIZE_HI

#endif // WORLD_ENVIRONMENT_WARE_GLOBALS_H
