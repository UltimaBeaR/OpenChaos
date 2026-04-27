#ifndef EFFECTS_COMBAT_POW_GLOBALS_H
#define EFFECTS_COMBAT_POW_GLOBALS_H

#include "effects/combat/pow.h"

// Internal struct for child-spawning behaviour of an explosion type.
// uc_orig: POW_Spawn (fallen/Source/pow.cpp)
typedef struct
{
    UWORD when; // Spawn when the parent's timer drops below this value. 0 = unused.
    UBYTE type;
    UBYTE flag;
} POW_Spawn;

// Arrangement styles for initial sprite placement.
// uc_orig: POW_ARRANGE_SPHERE (fallen/Source/pow.cpp)
#define POW_ARRANGE_SPHERE 0
// uc_orig: POW_ARRANGE_SEMISPHERE (fallen/Source/pow.cpp)
#define POW_ARRANGE_SEMISPHERE 1
// uc_orig: POW_ARRANGE_CIRCLE (fallen/Source/pow.cpp)
#define POW_ARRANGE_CIRCLE 2
// uc_orig: POW_ARRANGE_NOTHING (fallen/Source/pow.cpp)
#define POW_ARRANGE_NOTHING 3

// uc_orig: POW_TYPE_MAX_SPAWN (fallen/Source/pow.cpp)
#define POW_TYPE_MAX_SPAWN 3

// Descriptor for an explosion type: sprite arrangement, speed, density, and child spawning.
// uc_orig: POW_Type (fallen/Source/pow.cpp)
typedef struct
{
    unsigned int arrange : 2;
    unsigned int speed : 2;
    unsigned int density : 2;
    unsigned int framespeed : 2;
    unsigned int damp : 2;
    unsigned int padding : 6;

    UWORD life;
    POW_Spawn spawn[POW_TYPE_MAX_SPAWN];
} POW_Type;

// Table of descriptor data for each explosion type, indexed by POW_TYPE_*.
// uc_orig: POW_type (fallen/Source/pow.cpp)
extern POW_Type POW_type[POW_TYPE_NUMBER];

// Spawn flag constants.
// uc_orig: POW_SPAWN_FLAG_MIDDLE (fallen/Source/pow.cpp)
#define POW_SPAWN_FLAG_MIDDLE (1 << 0)
// uc_orig: POW_SPAWN_FLAG_AWAY (fallen/Source/pow.cpp)
#define POW_SPAWN_FLAG_AWAY (1 << 1)
// uc_orig: POW_SPAWN_FLAG_UPPER (fallen/Source/pow.cpp)
#define POW_SPAWN_FLAG_UPPER (1 << 2)
// uc_orig: POW_SPAWN_FLAG_FAR_OFF (fallen/Source/pow.cpp)
#define POW_SPAWN_FLAG_FAR_OFF (1 << 3)

// Internal timing constants.
// uc_orig: POW_TICKS_PER_SECOND (fallen/Source/pow.cpp)
#define POW_TICKS_PER_SECOND 2000
// uc_orig: POW_DELTA_SHIFT (fallen/Source/pow.cpp)
#define POW_DELTA_SHIFT 4
// uc_orig: POW_MAX_FRAME_SPEED (fallen/Source/pow.cpp)
#define POW_MAX_FRAME_SPEED (POW_TICKS_PER_SECOND / 25)

// Internal spawn flag bit patterns.
// uc_orig: POW_FLAG_SPAWNED_1 (fallen/Source/pow.cpp)
#define POW_FLAG_SPAWNED_1 (1 << 0)
// uc_orig: POW_FLAG_SPAWNED_2 (fallen/Source/pow.cpp)
#define POW_FLAG_SPAWNED_2 (1 << 1)
// uc_orig: POW_FLAG_SPAWNED_3 (fallen/Source/pow.cpp)
#define POW_FLAG_SPAWNED_3 (1 << 2)

#endif // EFFECTS_COMBAT_POW_GLOBALS_H
