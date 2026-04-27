#ifndef EFFECTS_ENVIRONMENT_FIRE_H
#define EFFECTS_ENVIRONMENT_FIRE_H

#include "engine/core/types.h"

// Pool-based fire effect: up to 8 simultaneous fires, 256 shared flames.
// Each fire is a linked list of FIRE_Flame nodes drawn from a global pool.
// Fire intensity (size) decreases over time; flames die naturally as size shrinks.
// This system is separate from the particle system (psystem.cpp).
// The renderer initializes iteration via FIRE_get_start.

// Max flame control points per flame sprite.
// uc_orig: FIRE_MAX_FLAME_POINTS (fallen/Source/fire.cpp)
#define FIRE_MAX_FLAME_POINTS 4

// uc_orig: FIRE_MAX_FLAMES (fallen/Source/fire.cpp)
#define FIRE_MAX_FLAMES 256

// uc_orig: FIRE_MAX_FIRE (fallen/Source/fire.cpp)
#define FIRE_MAX_FIRE 8

// Individual flame: one node in a fire's linked list.
// Animated each tick: control points wiggle via angle/offset arrays.
// uc_orig: FIRE_Flame (fallen/Source/fire.cpp)
typedef struct
{
    SBYTE dx; // horizontal offset from fire center
    SBYTE dz; // depth offset from fire center
    UBYTE die; // lifetime limit (flame dies when counter >= die, range 32..63)
    UBYTE counter; // age counter, incremented every tick
    UBYTE height; // visual height = 255 - (|dx| + |dz|); center flames tallest
    UBYTE next; // intrusive linked-list next index (0 = end)
    UBYTE points; // number of animated control points (2..4, derived from height)
    UBYTE shit; // padding / unused

    UBYTE angle[FIRE_MAX_FLAME_POINTS]; // per-control-point animated angle (wraps 0..255)
    UBYTE offset[FIRE_MAX_FLAME_POINTS]; // per-control-point animated radial offset
} FIRE_Flame;

// Top-level fire object. Slot is free when num == 0.
// uc_orig: FIRE_Fire (fallen/Source/fire.cpp)
typedef struct
{
    UBYTE num; // number of active flames (0 = slot unused)
    UBYTE next; // head of this fire's flame linked list
    UBYTE size; // current intensity; decreases over time; controls target flame count
    UBYTE shrink; // burn-out speed (subtracted from size every 4 ticks)
    UWORD x; // world X position
    SWORD y; // world Y position
    UWORD z; // world Z position
} FIRE_Fire;

// uc_orig: FIRE_MAX_POINTS (fallen/Source/fire.cpp)
#define FIRE_MAX_POINTS 16

// Renderer output type: a triangle strip of world-space points.
// uc_orig: FIRE_Point (fallen/Headers/fire.h)
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    UBYTE u;
    UBYTE v;
    UBYTE alpha;
    UBYTE shit; // padding
} FIRE_Point;

// uc_orig: FIRE_Info (fallen/Headers/fire.h)
typedef struct
{
    SLONG num_points;
    FIRE_Point* point;
} FIRE_Info;

// uc_orig: FIRE_get_start (fallen/Headers/fire.h)
void FIRE_get_start(UBYTE z, UBYTE x_min, UBYTE x_max);

#endif // EFFECTS_ENVIRONMENT_FIRE_H
