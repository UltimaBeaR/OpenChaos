#ifndef EFFECTS_GLITTER_GLOBALS_H
#define EFFECTS_GLITTER_GLOBALS_H

#include "effects/glitter.h"

// Shared spark pool. Index 0 is never used.
// uc_orig: GLITTER_spark (fallen/Source/glitter.cpp)
extern GLITTER_Spark GLITTER_spark[GLITTER_MAX_SPARKS];
// uc_orig: GLITTER_spark_free (fallen/Source/glitter.cpp)
extern UBYTE GLITTER_spark_free;

// Active glitter array.
// uc_orig: GLITTER_glitter (fallen/Source/glitter.cpp)
extern GLITTER_Glitter GLITTER_glitter[GLITTER_MAX_GLITTER];
// uc_orig: GLITTER_glitter_last (fallen/Source/glitter.cpp)
extern SLONG GLITTER_glitter_last;

// Spatial index: one z-row per mapwho z-coordinate.
// uc_orig: GLITTER_mapwho (fallen/Source/glitter.cpp)
extern UBYTE GLITTER_mapwho[GLITTER_MAPWHO];

// Iterator state for GLITTER_get_start / GLITTER_get_next.
// uc_orig: GLITTER_get_z (fallen/Source/glitter.cpp)
extern UBYTE GLITTER_get_z;
// uc_orig: GLITTER_get_xmin (fallen/Source/glitter.cpp)
extern UBYTE GLITTER_get_xmin;
// uc_orig: GLITTER_get_xmax (fallen/Source/glitter.cpp)
extern UBYTE GLITTER_get_xmax;
// uc_orig: GLITTER_get_glitter (fallen/Source/glitter.cpp)
extern UBYTE GLITTER_get_glitter;
// uc_orig: GLITTER_get_spark (fallen/Source/glitter.cpp)
extern UBYTE GLITTER_get_spark;
// uc_orig: GLITTER_get_info (fallen/Source/glitter.cpp)
extern GLITTER_Info GLITTER_get_info;

#endif // EFFECTS_GLITTER_GLOBALS_H
