#ifndef BUILDINGS_ID_H
#define BUILDINGS_ID_H

#include "engine/core/types.h"

// Interior design module: generates floorplans (rooms, furniture, stairs) for buildings.
// Given the outline of a building storey (walls), produces a layout of rooms and stair positions.
// Also handles collision for inside walls and floor drawing data.

// uc_orig: ID_FLOOR_SIZE (fallen/Headers/id.h)
#define ID_FLOOR_SIZE 256
// uc_orig: ID_PLAN_SIZE (fallen/Headers/id.h)
#define ID_PLAN_SIZE 32

// uc_orig: ID_BLOCK_TYPE_WALL (fallen/Headers/id.h)
#define ID_BLOCK_TYPE_WALL 1
// uc_orig: ID_BLOCK_TYPE_WINDOW (fallen/Headers/id.h)
#define ID_BLOCK_TYPE_WINDOW 2
// uc_orig: ID_BLOCK_TYPE_DOOR (fallen/Headers/id.h)
#define ID_BLOCK_TYPE_DOOR 3

// uc_orig: ID_STAIR_TYPE_BOTTOM (fallen/Headers/id.h)
#define ID_STAIR_TYPE_BOTTOM 1
// uc_orig: ID_STAIR_TYPE_MIDDLE (fallen/Headers/id.h)
#define ID_STAIR_TYPE_MIDDLE 2
// uc_orig: ID_STAIR_TYPE_TOP (fallen/Headers/id.h)
#define ID_STAIR_TYPE_TOP 3

// One staircase returned by STAIR_get() and ID_generate_floorplan().
// Occupies two adjacent grid tiles (x1,z1) and (x2,z2).
// uc_orig: ID_Stair (fallen/Headers/id.h)
typedef struct
{
    UBYTE type; // ID_STAIR_TYPE_BOTTOM/_MIDDLE/_TOP
    UBYTE id;
    UWORD shit;
    SLONG handle_up; // Floor handle above (if going up)
    SLONG handle_down; // Floor handle below (if going down)
    UBYTE x1;
    UBYTE z1;
    UBYTE x2;
    UBYTE z2;
} ID_Stair;

// uc_orig: ID_STOREY_TYPE_HOUSE_GROUND (fallen/Headers/id.h)
#define ID_STOREY_TYPE_HOUSE_GROUND 1
// uc_orig: ID_STOREY_TYPE_HOUSE_UPPER (fallen/Headers/id.h)
#define ID_STOREY_TYPE_HOUSE_UPPER 2
// uc_orig: ID_STOREY_TYPE_OFFICE_GROUND (fallen/Headers/id.h)
#define ID_STOREY_TYPE_OFFICE_GROUND 3
// uc_orig: ID_STOREY_TYPE_OFFICE_UPPER (fallen/Headers/id.h)
#define ID_STOREY_TYPE_OFFICE_UPPER 4
// uc_orig: ID_STOREY_TYPE_WAREHOUSE (fallen/Headers/id.h)
#define ID_STOREY_TYPE_WAREHOUSE 5
// uc_orig: ID_STOREY_TYPE_APARTEMENT_GROUND (fallen/Headers/id.h)
#define ID_STOREY_TYPE_APARTEMENT_GROUND 6
// uc_orig: ID_STOREY_TYPE_APARTEMENT_UPPER (fallen/Headers/id.h)
#define ID_STOREY_TYPE_APARTEMENT_UPPER 7

// Furniture descriptor: position, prim index, and yaw angle.
// uc_orig: ID_Finfo (fallen/Headers/id.h)
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    UWORD prim;
    UWORD yaw;
} ID_Finfo;

// Editor types.
// uc_orig: ID_Wallinfo (fallen/Headers/id.h)
typedef struct
{
    UBYTE door[4];
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;
} ID_Wallinfo;

// uc_orig: ID_Roominfo (fallen/Headers/id.h)
typedef struct
{
    SLONG x;
    SLONG z;
    CBYTE* what;
} ID_Roominfo;

// uc_orig: ID_Stairinfo (fallen/Headers/id.h)
typedef struct
{
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;
} ID_Stairinfo;

#endif // BUILDINGS_ID_H
