#ifndef EFFECTS_MIST_H
#define EFFECTS_MIST_H

#include "core/types.h"

// Quad-patch mist/fog overlay effect.
// Up to 8 simultaneous mist layers, each a detail x detail grid of MIST_Point entries.
// Points animate via gust() calls and self-damping spring integration.
// The renderer uses MIST_get_start / MIST_get_detail / MIST_get_point / MIST_get_texture.

// uc_orig: MIST_MAX_POINTS (fallen/Source/mist.cpp)
#define MIST_MAX_POINTS 4096

// uc_orig: MIST_MAX_MIST (fallen/Source/mist.cpp)
#define MIST_MAX_MIST 8

// uc_orig: MIST_MAX_STRENGTH (fallen/Source/mist.cpp)
#define MIST_MAX_STRENGTH 48

// Per-grid-point UV displacement (animated by gusts, self-damping spring).
// uc_orig: MIST_Point (fallen/Source/mist.cpp)
typedef struct
{
    float u;  // UV offset from base u
    float v;  // UV offset from base v
    float du; // velocity along u (applied each tick)
    float dv; // velocity along v (applied each tick)
} MIST_Point;

// One layer of mist over a rectangular world region.
// uc_orig: MIST_Mist (fallen/Source/mist.cpp)
typedef struct
{
    UBYTE type;    // scroll pattern variant (0..3), cycles on creation
    UBYTE detail;  // grid resolution (quads-per-row, clamped 3..255)
    UWORD p_index; // first index into the shared MIST_point[] pool
    SLONG height;  // world Y offset above the ground
    SLONG x1, z1;  // world min corner
    SLONG x2, z2;  // world max corner
} MIST_Mist;

// uc_orig: MIST_init (fallen/Headers/mist.h)
void MIST_init(void);

// uc_orig: MIST_create (fallen/Headers/mist.h)
// Creates a new mist layer over the given world rectangle.
void MIST_create(SLONG detail, SLONG height, SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: MIST_gust (fallen/Headers/mist.h)
// Applies a wind gust along the vector (x1,z1)->(x2,z2) to nearby mist points.
void MIST_gust(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: MIST_process (fallen/Headers/mist.h)
// Advances all mist point positions and applies damping each tick.
void MIST_process(void);

// uc_orig: MIST_get_start (fallen/Headers/mist.h)
void MIST_get_start(void);

// uc_orig: MIST_get_detail (fallen/Headers/mist.h)
// Returns the detail of the next mist layer, or NULL when done.
SLONG MIST_get_detail(void);

// uc_orig: MIST_get_point (fallen/Headers/mist.h)
void MIST_get_point(SLONG px, SLONG pz, SLONG* x, SLONG* y, SLONG* z);

// uc_orig: MIST_get_texture (fallen/Headers/mist.h)
void MIST_get_texture(SLONG px, SLONG pz, float* u, float* v);

#endif // EFFECTS_MIST_H
