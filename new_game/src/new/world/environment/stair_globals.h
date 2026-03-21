#ifndef WORLD_ENVIRONMENT_STAIR_GLOBALS_H
#define WORLD_ENVIRONMENT_STAIR_GLOBALS_H

#include "world/environment/id.h"
#include "core/types.h"

// Max building footprint in tile units (must be a multiple of 8 for bitfield packing).
// uc_orig: STAIR_MAX_SIZE (fallen/Source/Stair.cpp)
#define STAIR_MAX_SIZE 32

// uc_orig: STAIR_MAX_STAIRS (fallen/Source/Stair.cpp)
#define STAIR_MAX_STAIRS 32

// uc_orig: STAIR_MAX_PER_STOREY (fallen/Source/Stair.cpp)
#define STAIR_MAX_PER_STOREY 3

// uc_orig: STAIR_MAX_STOREYS (fallen/Source/Stair.cpp)
#define STAIR_MAX_STOREYS 32

// uc_orig: STAIR_MAX_LINKS (fallen/Source/Stair.cpp)
#define STAIR_MAX_LINKS 128

// One staircase placement: two adjacent tiles (x1,z1)-(x2,z2).
// up/down are indices into STAIR_storey[]. Index 0 is the NULL sentinel.
// uc_orig: STAIR_Stair (fallen/Source/Stair.cpp)
typedef struct
{
    UBYTE flag;
    UBYTE up;
    UBYTE down;
    UBYTE x1;
    UBYTE z1;
    UBYTE x2;
    UBYTE z2;
} STAIR_Stair;

// One floor of a building under construction.
// square[][] is a packed bitfield: bit (x_off & 7) of square[z_off][x_off >> 3] marks inside tiles.
// uc_orig: STAIR_Storey (fallen/Source/Stair.cpp)
typedef struct
{
    SLONG handle;
    UBYTE opp_x1;
    UBYTE opp_z1;
    UBYTE opp_x2;
    UBYTE opp_z2;
    UBYTE height;
    UBYTE stair[STAIR_MAX_PER_STOREY];
    UBYTE square[STAIR_MAX_SIZE][STAIR_MAX_SIZE / 8];
} STAIR_Storey;

// Scanline edge list node for inside-square computation.
// uc_orig: STAIR_Link (fallen/Source/Stair.cpp)
typedef struct
{
    UBYTE next;
    UBYTE square;
    UWORD pos;  // 8-bit fixed point x position
} STAIR_Link;

// Scanline link type constants.
// uc_orig: STAIR_LINK_T_ENTER (fallen/Source/Stair.cpp)
#define STAIR_LINK_T_ENTER 1
// uc_orig: STAIR_LINK_T_LEAVE (fallen/Source/Stair.cpp)
#define STAIR_LINK_T_LEAVE 2

// Bounding box of the current building in tile coordinates.
// uc_orig: STAIR_x1 (fallen/Source/Stair.cpp)
extern UBYTE STAIR_x1;
// uc_orig: STAIR_z1 (fallen/Source/Stair.cpp)
extern UBYTE STAIR_z1;
// uc_orig: STAIR_x2 (fallen/Source/Stair.cpp)
extern UBYTE STAIR_x2;
// uc_orig: STAIR_z2 (fallen/Source/Stair.cpp)
extern UBYTE STAIR_z2;

// Maximum floor height + 1.
// uc_orig: STAIR_roof_height (fallen/Source/Stair.cpp)
extern UBYTE STAIR_roof_height;

// Array of all staircases. Index 0 is the NULL sentinel.
// uc_orig: STAIR_stair (fallen/Source/Stair.cpp)
extern STAIR_Stair STAIR_stair[STAIR_MAX_STAIRS];
// uc_orig: STAIR_stair_upto (fallen/Source/Stair.cpp)
extern SLONG STAIR_stair_upto;

// Array of all storeys for the current building.
// uc_orig: STAIR_storey (fallen/Source/Stair.cpp)
extern STAIR_Storey STAIR_storey[STAIR_MAX_STOREYS];
// uc_orig: STAIR_storey_upto (fallen/Source/Stair.cpp)
extern SLONG STAIR_storey_upto;

// Scanline edge link storage. Index 0 is the NULL sentinel.
// uc_orig: STAIR_link (fallen/Source/Stair.cpp)
extern STAIR_Link STAIR_link[STAIR_MAX_LINKS];
// uc_orig: STAIR_link_upto (fallen/Source/Stair.cpp)
extern SLONG STAIR_link_upto;

// Per-z-row head-of-linked-list index into STAIR_link[].
// uc_orig: STAIR_edge (fallen/Source/Stair.cpp)
extern UBYTE STAIR_edge[STAIR_MAX_SIZE];

// LCG random seed used for deterministic stair placement.
// uc_orig: STAIR_rand_seed (fallen/Source/Stair.cpp)
extern ULONG STAIR_rand_seed;

// Output buffer filled by STAIR_get(). Caller receives a pointer to this array.
// uc_orig: STAIR_id_stair (fallen/Source/Stair.cpp)
extern ID_Stair STAIR_id_stair[STAIR_MAX_PER_STOREY];

#endif // WORLD_ENVIRONMENT_STAIR_GLOBALS_H
