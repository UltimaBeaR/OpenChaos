#ifndef EFFECTS_COMBAT_SPARK_H
#define EFFECTS_COMBAT_SPARK_H

#include "engine/core/types.h"

// Animated electric spark effect: each spark is a branching lightning bolt between two world
// points. Sparks live for a random number of ticks, wiggle via velocity, and spawn GLITTER
// particles at their endpoints. A mapwho z-bucket index enables efficient range queries.
//
// Rendering: caller iterates SPARK_get_start / SPARK_get_next to get SPARK_Info segments,
// then submits them as coloured line segments.

// uc_orig: SPARK_MAX_POINTS (fallen/Headers/spark.h)
#define SPARK_MAX_POINTS 5

// uc_orig: SPARK_TYPE_POINT (fallen/Headers/spark.h)
#define SPARK_TYPE_POINT 1
// uc_orig: SPARK_TYPE_LIMB (fallen/Headers/spark.h)
#define SPARK_TYPE_LIMB 2
// uc_orig: SPARK_TYPE_GROUND (fallen/Headers/spark.h)
#define SPARK_TYPE_GROUND 3
// uc_orig: SPARK_TYPE_CIRCULAR (fallen/Headers/spark.h)
#define SPARK_TYPE_CIRCULAR 4

// uc_orig: SPARK_FLAG_CLAMP_X (fallen/Headers/spark.h)
#define SPARK_FLAG_CLAMP_X (1 << 0)
// uc_orig: SPARK_FLAG_CLAMP_Y (fallen/Headers/spark.h)
#define SPARK_FLAG_CLAMP_Y (1 << 1)
// uc_orig: SPARK_FLAG_CLAMP_Z (fallen/Headers/spark.h)
#define SPARK_FLAG_CLAMP_Z (1 << 2)
// uc_orig: SPARK_FLAG_FAST (fallen/Headers/spark.h)
#define SPARK_FLAG_FAST (1 << 3)
// uc_orig: SPARK_FLAG_SLOW (fallen/Headers/spark.h)
#define SPARK_FLAG_SLOW (1 << 4)
// uc_orig: SPARK_FLAG_DART_ABOUT (fallen/Headers/spark.h)
#define SPARK_FLAG_DART_ABOUT (1 << 5)
// uc_orig: SPARK_FLAG_STILL (fallen/Headers/spark.h)
#define SPARK_FLAG_STILL (SPARK_FLAG_CLAMP_X | SPARK_FLAG_CLAMP_Y | SPARK_FLAG_CLAMP_Z)

// Endpoint descriptor used when creating a spark.
// uc_orig: SPARK_Pinfo (fallen/Headers/spark.h)
typedef struct
{
    UBYTE type;
    UBYTE flag;
    UBYTE padding;
    UBYTE dist; // distance from first point (for circular type)
    UWORD x;
    UWORD y;
    UWORD z;
    THING_INDEX person; // used when type == SPARK_TYPE_LIMB
    UWORD limb;
} SPARK_Pinfo;

// Output descriptor for one rendered spark segment (5-point polyline).
// uc_orig: SPARK_Info (fallen/Headers/spark.h)
typedef struct
{
    SLONG num_points;
    ULONG colour;
    SLONG size;
    SLONG x[SPARK_MAX_POINTS];
    SLONG y[SPARK_MAX_POINTS];
    SLONG z[SPARK_MAX_POINTS];
} SPARK_Info;

// uc_orig: SPARK_init (fallen/Headers/spark.h)
void SPARK_init(void);

// uc_orig: SPARK_create (fallen/Headers/spark.h)
// Creates a branching lightning bolt between p1 and p2, lasting up to max_life ticks.
void SPARK_create(SPARK_Pinfo* point1, SPARK_Pinfo* point2, UBYTE max_life);

// uc_orig: SPARK_show_electric_fences (fallen/Headers/spark.h)
// Creates sparks along electrified DFacets on the map (called once per 32 ticks).
void SPARK_show_electric_fences(float dt_ms);

// uc_orig: SPARK_process (fallen/Headers/spark.h)
// Advances all active sparks: moves points, ages, re-branches, spawns GLITTER.
void SPARK_process(void);

// uc_orig: SPARK_get_start (fallen/Headers/spark.h)
// Initialises the render iterator for the given z-row and x range.
void SPARK_get_start(UBYTE xmin, UBYTE xmax, UBYTE z);

// uc_orig: SPARK_get_next (fallen/Headers/spark.h)
// Returns the next spark line segment, or NULL when done.
SPARK_Info* SPARK_get_next(void);

#endif // EFFECTS_COMBAT_SPARK_H
