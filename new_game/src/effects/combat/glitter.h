#ifndef EFFECTS_COMBAT_GLITTER_H
#define EFFECTS_COMBAT_GLITTER_H

#include "engine/core/types.h"

// Glitter: collections of coloured line-segment sparks grouped into "glitters".
// Each glitter owns a linked list of GLITTER_Spark nodes from a shared pool.
// Glitters are anchored to map squares via GLITTER_mapwho[]; the renderer
// iterates them by z-slice using GLITTER_get_start / GLITTER_get_next.

// uc_orig: GLITTER_SPARK_LIFE (fallen/Source/glitter.cpp)
#define GLITTER_SPARK_LIFE (32)

// uc_orig: GLITTER_SPARK_GRAVITY (fallen/Source/glitter.cpp)
#define GLITTER_SPARK_GRAVITY (-2)

// uc_orig: GLITTER_MAX_SPARKS (fallen/Source/glitter.cpp)
#define GLITTER_MAX_SPARKS 128

// uc_orig: GLITTER_MAX_GLITTER (fallen/Source/glitter.cpp)
#define GLITTER_MAX_GLITTER 32

// uc_orig: GLITTER_MAPWHO (fallen/Source/glitter.cpp)
#define GLITTER_MAPWHO 128

// One glitter spark: a tiny moving line segment with gravity and limited lifetime.
// uc_orig: GLITTER_Spark (fallen/Source/glitter.cpp)
typedef struct
{
    UWORD x;
    SWORD y;
    UWORD z;
    SBYTE dx;
    SBYTE dy;
    SBYTE dz;
    UBYTE next; // next spark in linked list (0 = end)
    UBYTE die; // counts down to 0; spark removed when die == 0
    UBYTE useless_padding;
} GLITTER_Spark;

// A collection of sparks sharing a colour, anchored to a map-who cell.
// uc_orig: GLITTER_Glitter (fallen/Source/glitter.cpp)
typedef struct
{
    UBYTE flag; // GLITTER_FLAG_* bitmask
    UBYTE spark; // head of this glitter's spark linked list
    UBYTE next; // next glitter in the same mapwho z-bucket
    UBYTE map_x; // map X anchor
    UBYTE map_z; // map Z anchor (indexes GLITTER_mapwho[])
    UBYTE red;
    UBYTE green;
    UBYTE blue;
} GLITTER_Glitter;

// Spark direction constraint flags for GLITTER_create().
// uc_orig: GLITTER_FLAG_DXPOS (fallen/Headers/glitter.h)
#define GLITTER_FLAG_DXPOS (1 << 0)
// uc_orig: GLITTER_FLAG_DXNEG (fallen/Headers/glitter.h)
#define GLITTER_FLAG_DXNEG (1 << 1)
// uc_orig: GLITTER_FLAG_DYPOS (fallen/Headers/glitter.h)
#define GLITTER_FLAG_DYPOS (1 << 2)
// uc_orig: GLITTER_FLAG_DYNEG (fallen/Headers/glitter.h)
#define GLITTER_FLAG_DYNEG (1 << 3)
// uc_orig: GLITTER_FLAG_DZPOS (fallen/Headers/glitter.h)
#define GLITTER_FLAG_DZPOS (1 << 4)
// uc_orig: GLITTER_FLAG_DZNEG (fallen/Headers/glitter.h)
#define GLITTER_FLAG_DZNEG (1 << 5)
// uc_orig: GLITTER_FLAG_USED (fallen/Headers/glitter.h)
#define GLITTER_FLAG_USED (1 << 6)
// uc_orig: GLITTER_FLAG_DESTROY (fallen/Headers/glitter.h)
#define GLITTER_FLAG_DESTROY (1 << 7)

// Renderer output: a coloured line segment in world space.
// uc_orig: GLITTER_Info (fallen/Headers/glitter.h)
typedef struct
{
    SLONG x1;
    SLONG y1;
    SLONG z1;
    SLONG x2;
    SLONG y2;
    SLONG z2;
    ULONG colour;
} GLITTER_Info;

// uc_orig: GLITTER_init (fallen/Headers/glitter.h)
void GLITTER_init(void);

// uc_orig: GLITTER_create (fallen/Headers/glitter.h)
// Allocates a new glitter anchored at (map_x, map_z); returns its index or NULL on failure.
UBYTE GLITTER_create(UBYTE flag, UBYTE map_x, UBYTE map_z, ULONG colour);

// uc_orig: GLITTER_destroy (fallen/Headers/glitter.h)
// Marks the glitter for destruction; it is freed once all its sparks die.
void GLITTER_destroy(UBYTE glitter);

// uc_orig: GLITTER_add (fallen/Headers/glitter.h)
// Adds a spark at world position (x, y, z) to the given glitter.
void GLITTER_add(UBYTE glitter, SLONG x, SLONG y, SLONG z);

// uc_orig: GLITTER_process (fallen/Headers/glitter.h)
void GLITTER_process(void);

// uc_orig: GLITTER_get_start (fallen/Headers/glitter.h)
void GLITTER_get_start(UBYTE xmin, UBYTE xmax, UBYTE z);

// uc_orig: GLITTER_get_next (fallen/Headers/glitter.h)
// Returns the next line segment, or NULL when done.
GLITTER_Info* GLITTER_get_next(void);

#endif // EFFECTS_COMBAT_GLITTER_H
