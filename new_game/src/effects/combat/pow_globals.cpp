#include "effects/combat/pow_globals.h"

// uc_orig: POW_sprite (fallen/Headers/pow.h)
POW_Sprite POW_sprite[POW_MAX_SPRITES];

// uc_orig: POW_sprite_free (fallen/Headers/pow.h)
UBYTE POW_sprite_free;

// uc_orig: POW_pow (fallen/Headers/pow.h)
POW_Pow POW_pow[POW_MAX_POWS];

// uc_orig: POW_pow_free (fallen/Headers/pow.h)
UBYTE POW_pow_free;

// uc_orig: POW_pow_used (fallen/Headers/pow.h)
UBYTE POW_pow_used;

// uc_orig: POW_mapwho (fallen/Headers/pow.h)
UBYTE POW_mapwho[PAP_SIZE_LO];

// Static descriptor table for all explosion types.
// uc_orig: POW_type (fallen/Source/pow.cpp)
POW_Type POW_type[POW_TYPE_NUMBER] = {
    // Unused
    { POW_ARRANGE_NOTHING },

    // Basic large sphere
    {
        POW_ARRANGE_SPHERE,
        1, // speed
        3, // density
        0, // framespeed
        0, // damp
        0, // padding
        1, // life
    },

    // Basic medium sphere
    {
        POW_ARRANGE_SPHERE,
        2, // speed
        1, // density
        1, // framespeed
        0, // damp
        0, // padding
        1, // life
    },

    // Basic small sphere
    {
        POW_ARRANGE_SPHERE,
        3, // speed
        0, // density
        0, // framespeed
        1, // damp
        0, // padding
        1, // life
    },

    // Large bang spawning 3 medium bangs
    {
        POW_ARRANGE_SPHERE,
        1, // speed
        3, // density
        0, // framespeed
        0, // damp
        0, // padding
        POW_TICKS_PER_SECOND * 4 >> 3,
        { { POW_TICKS_PER_SECOND * 3 >> 3, POW_TYPE_BASIC_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF },
          { POW_TICKS_PER_SECOND * 2 >> 3, POW_TYPE_BASIC_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF },
          { POW_TICKS_PER_SECOND * 1 >> 3, POW_TYPE_BASIC_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF } } },

    // Medium bang spawning 3 small bangs
    {
        POW_ARRANGE_SPHERE,
        2, // speed
        1, // density
        1, // framespeed
        0, // damp
        0, // padding
        POW_TICKS_PER_SECOND * 4 >> 3,
        { { POW_TICKS_PER_SECOND * 3 >> 3, POW_TYPE_BASIC_SPHERE_SMALL, POW_SPAWN_FLAG_AWAY },
          { POW_TICKS_PER_SECOND * 2 >> 3, POW_TYPE_BASIC_SPHERE_SMALL, POW_SPAWN_FLAG_AWAY },
          { POW_TICKS_PER_SECOND * 1 >> 3, POW_TYPE_BASIC_SPHERE_SMALL, POW_SPAWN_FLAG_AWAY } } },

    // Large bang spawning 3 spawning-medium bangs
    {
        POW_ARRANGE_SPHERE,
        1, // speed
        3, // density
        0, // framespeed
        0, // damp
        0, // padding
        POW_TICKS_PER_SECOND * 4 >> 3,
        { { POW_TICKS_PER_SECOND * 3 >> 3, POW_TYPE_SPAWN_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF },
          { POW_TICKS_PER_SECOND * 2 >> 3, POW_TYPE_SPAWN_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF },
          { POW_TICKS_PER_SECOND * 1 >> 3, POW_TYPE_SPAWN_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF } } },

    // Large semisphere bang spawning 3 spawning-medium bangs
    {
        POW_ARRANGE_SEMISPHERE,
        1, // speed
        3, // density
        0, // framespeed
        0, // damp
        0, // padding
        POW_TICKS_PER_SECOND * 4 >> 3,
        { { POW_TICKS_PER_SECOND * 3 >> 3, POW_TYPE_SPAWN_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF },
          { POW_TICKS_PER_SECOND * 2 >> 3, POW_TYPE_SPAWN_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF },
          { POW_TICKS_PER_SECOND * 1 >> 3, POW_TYPE_SPAWN_SPHERE_MEDIUM, POW_SPAWN_FLAG_AWAY | POW_SPAWN_FLAG_FAR_OFF } } },
};
