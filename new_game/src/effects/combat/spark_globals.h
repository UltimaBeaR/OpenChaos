#ifndef EFFECTS_COMBAT_SPARK_GLOBALS_H
#define EFFECTS_COMBAT_SPARK_GLOBALS_H

#include "effects/combat/spark.h"
#include "map/map.h"

// Internal spark structure (not exposed in the public API header).
// uc_orig: SPARK_Point (fallen/Source/spark.cpp)
typedef struct
{
    UBYTE type;
    UBYTE flag;
    UBYTE dist;
    SBYTE dx;
    SBYTE dy;
    SBYTE dz;
    union {
        struct
        {
            UWORD x;
            UWORD y;
            UWORD z;
        } Pos;
        struct
        {
            UWORD person;
            UWORD limb;
        } Peep;
    };
} SPARK_Point;

// uc_orig: SPARK_MAX_SPARKS (fallen/Source/spark.cpp)
#define SPARK_MAX_SPARKS 32

// A live spark: two primary endpoints plus up to two auxiliary branches.
// uc_orig: SPARK_Spark (fallen/Source/spark.cpp)
typedef struct
{
    UBYTE used;
    UBYTE die;         // ticks remaining; 0 = dead
    UBYTE num_points;
    UBYTE next;        // next spark in the mapwho z-bucket linked list
    UBYTE map_z;
    UBYTE map_x;
    UBYTE glitter;     // associated GLITTER index (NULL = none)
    UBYTE useless_padding;
    SPARK_Point point[4];
} SPARK_Spark;

// All active sparks.
// uc_orig: SPARK_spark (fallen/Source/spark.cpp)
extern SPARK_Spark SPARK_spark[SPARK_MAX_SPARKS];

// Round-robin allocation cursor.
// uc_orig: SPARK_spark_last (fallen/Source/spark.cpp)
extern SLONG SPARK_spark_last;

// Spatial index: one entry per z-row, stores head of linked list.
// uc_orig: SPARK_mapwho (fallen/Source/spark.cpp)
extern UBYTE SPARK_mapwho[MAP_HEIGHT];

// --- Iterator state for SPARK_get_start / SPARK_get_next ---

// uc_orig: SPARK_get_z (fallen/Source/spark.cpp)
extern UBYTE SPARK_get_z;
// uc_orig: SPARK_get_xmin (fallen/Source/spark.cpp)
extern UBYTE SPARK_get_xmin;
// uc_orig: SPARK_get_xmax (fallen/Source/spark.cpp)
extern UBYTE SPARK_get_xmax;
// uc_orig: SPARK_get_spark (fallen/Source/spark.cpp)
extern UBYTE SPARK_get_spark;
// uc_orig: SPARK_get_point (fallen/Source/spark.cpp)
extern UBYTE SPARK_get_point;
// uc_orig: SPARK_get_info (fallen/Source/spark.cpp)
extern SPARK_Info SPARK_get_info;
// uc_orig: SPARK_get_x1 (fallen/Source/spark.cpp)
extern SLONG SPARK_get_x1;
// uc_orig: SPARK_get_y1 (fallen/Source/spark.cpp)
extern SLONG SPARK_get_y1;
// uc_orig: SPARK_get_z1 (fallen/Source/spark.cpp)
extern SLONG SPARK_get_z1;
// uc_orig: SPARK_get_x2 (fallen/Source/spark.cpp)
extern SLONG SPARK_get_x2;
// uc_orig: SPARK_get_y2 (fallen/Source/spark.cpp)
extern SLONG SPARK_get_y2;
// uc_orig: SPARK_get_z2 (fallen/Source/spark.cpp)
extern SLONG SPARK_get_z2;

#endif // EFFECTS_COMBAT_SPARK_GLOBALS_H
