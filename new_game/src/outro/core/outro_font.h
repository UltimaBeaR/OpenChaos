#ifndef OUTRO_CORE_OUTRO_FONT_H
#define OUTRO_CORE_OUTRO_FONT_H

#include "outro/core/outro_always.h"

// uc_orig: FONT_FLAG_JUSTIFY_LEFT (fallen/outro/font.h)
#define FONT_FLAG_JUSTIFY_LEFT 0

// uc_orig: FONT_FLAG_JUSTIFY_CENTRE (fallen/outro/font.h)
#define FONT_FLAG_JUSTIFY_CENTRE (1 << 0)

// uc_orig: FONT_FLAG_JUSTIFY_RIGHT (fallen/outro/font.h)
#define FONT_FLAG_JUSTIFY_RIGHT (1 << 1)

// uc_orig: FONT_FLAG_ITALIC (fallen/outro/font.h)
#define FONT_FLAG_ITALIC (1 << 2)

// uc_orig: FONT_init (fallen/outro/font.h)
// Loads the font texture and calculates UV coordinates of each glyph.
void FONT_init(void);

// uc_orig: FONT_draw (fallen/outro/font.h)
// Draws formatted text at (start_x, start_y) in normalised screen coords.
// shimmer: 0.0F = solid, 1.0F = fully shimmered/transparent.
// cursor: draw an insertion cursor after the cursor-th character (negative = no cursor).
void FONT_draw(
    SLONG flag,
    float start_x,
    float start_y,
    ULONG colour,
    float scale,
    SLONG cursor,
    float shimmer,
    CBYTE* fmt, ...);

#endif // OUTRO_CORE_OUTRO_FONT_H
