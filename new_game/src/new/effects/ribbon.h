#ifndef EFFECTS_RIBBON_H
#define EFFECTS_RIBBON_H

#include "core/types.h"
#include "core/vector.h"

// Ribbon system: circular-buffer trail renderer for fire, smoke, and other streaking effects.
// Each ribbon is a sequence of 3D points rendered as a textured triangle strip.
// Ribbons are 1-indexed (0 = invalid/no ribbon).

// uc_orig: MAX_RIBBONS (fallen/Headers/ribbon.h)
#define MAX_RIBBONS 64

// uc_orig: MAX_RIBBON_SIZE (fallen/Headers/ribbon.h)
#define MAX_RIBBON_SIZE 32

// uc_orig: RIBBON_FLAG_USED (fallen/Headers/ribbon.h)
#define RIBBON_FLAG_USED 1
// uc_orig: RIBBON_FLAG_FADE (fallen/Headers/ribbon.h)
#define RIBBON_FLAG_FADE 2
// uc_orig: RIBBON_FLAG_SLIDE (fallen/Headers/ribbon.h)
#define RIBBON_FLAG_SLIDE 4
// uc_orig: RIBBON_FLAG_DOUBLED (fallen/Headers/ribbon.h)
#define RIBBON_FLAG_DOUBLED 8
// uc_orig: RIBBON_FLAG_CONVECT (fallen/Headers/ribbon.h)
#define RIBBON_FLAG_CONVECT 16
// uc_orig: RIBBON_FLAG_IALPHA (fallen/Headers/ribbon.h)
#define RIBBON_FLAG_IALPHA 32

// One ribbon slot. The point buffer is a circular queue between Tail and Head.
// uc_orig: Ribbon (fallen/Headers/ribbon.h)
struct Ribbon {
    SLONG Flags;
    SLONG Page;       // POLY_PAGE texture page index
    SLONG Life;       // ticks remaining; -1 = infinite
    SLONG RGB;        // colour
    UBYTE Size;       // max points allowed
    UBYTE Head;       // next write position
    UBYTE Tail;       // oldest point
    UBYTE Scroll;     // texture offset for looped textures
    UBYTE FadePoint;  // number of steps over which alpha fades out
    UBYTE SlideSpeed; // how much Scroll advances per tick
    UBYTE TextureU;   // texture repeat count along width
    UBYTE TextureV;   // texture steps per loop
    GameCoord Points[MAX_RIBBON_SIZE];
};

// Renders a single ribbon's point buffer as a textured triangle strip.
// uc_orig: RIBBON_draw_ribbon (fallen/DDEngine/Source/drawxtra.cpp)
void RIBBON_draw_ribbon(Ribbon* ribbon);

// uc_orig: RIBBON_init (fallen/Headers/ribbon.h)
void RIBBON_init();

// uc_orig: RIBBON_draw (fallen/Headers/ribbon.h)
void RIBBON_draw();

// uc_orig: RIBBON_process (fallen/Headers/ribbon.h)
void RIBBON_process();

// uc_orig: RIBBON_alloc (fallen/Headers/ribbon.h)
// Allocates a ribbon slot; returns 1-based index, or 0 on failure.
SLONG RIBBON_alloc(SLONG flags, UBYTE max_segments, SLONG page, SLONG life = -1,
    UBYTE fade = 0, UBYTE scroll = 0, UBYTE u = 1, UBYTE v = 0, SLONG rgb = 0xFFFFFF);

// uc_orig: RIBBON_free (fallen/Headers/ribbon.h)
void RIBBON_free(SLONG ribbon);

// uc_orig: RIBBON_extend (fallen/Headers/ribbon.h)
// Appends a point to the ribbon's circular buffer. No-op when paused (GAMEMENU active).
void RIBBON_extend(SLONG ribbon, SLONG x, SLONG y, SLONG z);

// uc_orig: RIBBON_length (fallen/Headers/ribbon.h)
SLONG RIBBON_length(SLONG ribbon);

// uc_orig: RIBBON_life (fallen/Headers/ribbon.h)
void RIBBON_life(SLONG ribbon, SLONG life);

#endif // EFFECTS_RIBBON_H
