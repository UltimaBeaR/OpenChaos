#ifndef WORLD_ENVIRONMENT_PUDDLE_H
#define WORLD_ENVIRONMENT_PUDDLE_H

#include "engine/core/types.h"

// uc_orig: PUDDLE_init (fallen/Headers/puddle.h)
void PUDDLE_init(void);

// uc_orig: PUDDLE_create (fallen/Headers/puddle.h)
// Creates a whole puddle centered at (x,y,z).
void PUDDLE_create(UWORD x, SWORD y, UWORD z);

// uc_orig: PUDDLE_precalculate (fallen/Headers/puddle.h)
// Places puddles along building edges, road edges, and curb corners.
void PUDDLE_precalculate(void);

// uc_orig: PUDDLE_splash (fallen/Headers/puddle.h)
// Triggers a ripple effect if (x,y,z) is inside a puddle.
// (x>>8, z>>8) are map-square coordinates.
void PUDDLE_splash(SLONG x, SLONG y, SLONG z);

// uc_orig: PUDDLE_in (fallen/Headers/puddle.h)
// Returns UC_TRUE if (x,z) is inside any puddle or water square.
SLONG PUDDLE_in(SLONG x, SLONG z);

// uc_orig: PUDDLE_process (fallen/Headers/puddle.h)
// Decays ripple animations each frame.
void PUDDLE_process(void);

// uc_orig: PUDDLE_Info (fallen/Headers/puddle.h)
// Rendering info for a single puddle quad, returned by PUDDLE_get_next.
typedef struct
{
    SLONG x1;
    SLONG z1;
    SLONG u1; // 0 to 256
    SLONG v1; // 0 to 256

    SLONG x2;
    SLONG z2;
    SLONG u2;
    SLONG v2;

    SLONG y;

    UBYTE puddle_y1;
    UBYTE puddle_y2;
    UBYTE puddle_g1;
    UBYTE puddle_g2;
    UBYTE puddle_s1;
    UBYTE puddle_s2;
    UWORD rotate_uvs; // Rotate UVs relative to XZ coordinates

} PUDDLE_Info;

// uc_orig: PUDDLE_get_start (fallen/Headers/puddle.h)
void PUDDLE_get_start(UBYTE z_map, UBYTE x_map_min, UBYTE x_map_max);

// uc_orig: PUDDLE_get_next (fallen/Headers/puddle.h)
// Returns the next puddle on the current z-line within the x range, or NULL.
PUDDLE_Info* PUDDLE_get_next(void);

#endif // WORLD_ENVIRONMENT_PUDDLE_H
