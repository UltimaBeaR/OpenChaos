#ifndef WORLD_ENVIRONMENT_BANG_H
#define WORLD_ENVIRONMENT_BANG_H

#include "core/types.h"

// Explosion effect system. Handles big bang visual effects (explosion spheres).
// Bangs are semi-sphere primitives with additive blending; center has the given
// colour and the edge fades to black.

// uc_orig: BANG_BIG (fallen/Headers/bang.h)
#define BANG_BIG 0

// uc_orig: BANG_Info (fallen/Headers/bang.h)
// Describes a single explosion sphere for rendering.
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    SLONG dx; // direction of semi-sphere normal, normalised to 256
    SLONG dy;
    SLONG dz;
    SLONG radius;
    UBYTE red;
    UBYTE green;
    UBYTE blue;
    UBYTE frame;

} BANG_Info;

// uc_orig: BANG_init (fallen/Headers/bang.h)
void BANG_init(void);

// uc_orig: BANG_process (fallen/Headers/bang.h)
void BANG_process(void);

// uc_orig: BANG_create (fallen/Headers/bang.h)
void BANG_create(
    SLONG type,
    SLONG x,
    SLONG y,
    SLONG z);

// Iteration helpers for rendering: set up iteration over bangs in a map column range,
// then call BANG_get_next() until it returns NULL.
// uc_orig: BANG_get_start (fallen/Headers/bang.h)
void BANG_get_start(UBYTE xmin, UBYTE xmax, UBYTE z);
// uc_orig: BANG_get_next (fallen/Headers/bang.h)
BANG_Info* BANG_get_next(void); // NULL => no more bangs

#endif // WORLD_ENVIRONMENT_BANG_H
