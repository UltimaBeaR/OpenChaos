#ifndef EFFECTS_DRIP_H
#define EFFECTS_DRIP_H

#include "engine/core/types.h"

// Drip effect: expanding, fading ripple circles on water surfaces.
// Created when objects fall into puddles or water tiles.
// Renderer iterates all active drips via DRIP_get_start / DRIP_get_next.

// uc_orig: DRIP_FLAG_PUDDLES_ONLY (fallen/Headers/drip.h)
#define DRIP_FLAG_PUDDLES_ONLY (1)

// Renderer output: one visible drip circle at a world position.
// uc_orig: DRIP_Info (fallen/Headers/drip.h)
typedef struct
{
    UWORD x;
    SWORD y;
    UWORD z;
    UBYTE size;
    UBYTE fade; // 255 = opaque, 0 = transparent (drip is dead when fade == 0)
    UBYTE flags;
} DRIP_Info;

// uc_orig: DRIP_init (fallen/Headers/drip.h)
void DRIP_init(void);

// uc_orig: DRIP_create (fallen/Headers/drip.h)
// Creates a new drip ripple at (x, y, z).
void DRIP_create(UWORD x, SWORD y, UWORD z, UBYTE flags);

// uc_orig: DRIP_create_if_in_puddle (fallen/Headers/drip.h)
// Creates a drip only if (x, y, z) is over a reflective or water tile and inside a puddle.
// Also spawns a PARTICLE splash sprite.
void DRIP_create_if_in_puddle(UWORD x, SWORD y, UWORD z);

// uc_orig: DRIP_process (fallen/Headers/drip.h)
// Expands drips and fades them out each tick.
void DRIP_process(void);

// uc_orig: DRIP_get_start (fallen/Headers/drip.h)
void DRIP_get_start(void);

// uc_orig: DRIP_get_next (fallen/Headers/drip.h)
// Returns the next active drip, or NULL when done.
DRIP_Info* DRIP_get_next(void);

#endif // EFFECTS_DRIP_H
