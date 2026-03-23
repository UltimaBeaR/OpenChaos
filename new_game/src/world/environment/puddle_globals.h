#ifndef WORLD_ENVIRONMENT_PUDDLE_GLOBALS_H
#define WORLD_ENVIRONMENT_PUDDLE_GLOBALS_H

#include "world/environment/puddle.h"

// uc_orig: PUDDLE_TYPE_STRIP1 (fallen/Source/puddle.cpp)
#define PUDDLE_TYPE_STRIP1 0
// uc_orig: PUDDLE_TYPE_STRIP2 (fallen/Source/puddle.cpp)
#define PUDDLE_TYPE_STRIP2 1
// uc_orig: PUDDLE_TYPE_CORNER (fallen/Source/puddle.cpp)
#define PUDDLE_TYPE_CORNER 2
// uc_orig: PUDDLE_TYPE_WHOLE (fallen/Source/puddle.cpp)
#define PUDDLE_TYPE_WHOLE 3
// uc_orig: PUDDLE_TYPE_NUMBER (fallen/Source/puddle.cpp)
#define PUDDLE_TYPE_NUMBER 4

// uc_orig: PUDDLE_Puddle (fallen/Source/puddle.cpp)
typedef struct
{
    UWORD x1;
    UWORD z1;
    UWORD x2;
    UWORD z2;
    SWORD y;
    UBYTE type;
    UBYTE rotate_uvs;
    UBYTE map_x;
    UBYTE next;
    UBYTE y1;
    UBYTE y2;
    UBYTE g1;
    UBYTE g2;
    UBYTE s1;
    UBYTE s2;

} PUDDLE_Puddle;

// uc_orig: PUDDLE_MAX_PUDDLES (fallen/Source/puddle.cpp)
#define PUDDLE_MAX_PUDDLES 256

// uc_orig: PUDDLE_MAPWHO_SIZE (fallen/Source/puddle.cpp)
#define PUDDLE_MAPWHO_SIZE 128

// uc_orig: PUDDLE_puddle (fallen/Source/puddle.cpp)
extern PUDDLE_Puddle PUDDLE_puddle[PUDDLE_MAX_PUDDLES];

// uc_orig: PUDDLE_puddle_upto (fallen/Source/puddle.cpp)
extern SLONG PUDDLE_puddle_upto;

// uc_orig: PUDDLE_mapwho (fallen/Source/puddle.cpp)
// Per-z-line linked list heads for the puddle spatial index.
extern UBYTE PUDDLE_mapwho[PUDDLE_MAPWHO_SIZE];

// Iterator state for PUDDLE_get_start / PUDDLE_get_next
// uc_orig: PUDDLE_get_upto (fallen/Source/puddle.cpp)
extern UBYTE PUDDLE_get_upto;
// uc_orig: PUDDLE_get_z (fallen/Source/puddle.cpp)
extern UBYTE PUDDLE_get_z;
// uc_orig: PUDDLE_get_x_min (fallen/Source/puddle.cpp)
extern UBYTE PUDDLE_get_x_min;
// uc_orig: PUDDLE_get_x_max (fallen/Source/puddle.cpp)
extern UBYTE PUDDLE_get_x_max;
// uc_orig: PUDDLE_get_info (fallen/Source/puddle.cpp)
extern PUDDLE_Info PUDDLE_get_info;

#endif // WORLD_ENVIRONMENT_PUDDLE_GLOBALS_H
