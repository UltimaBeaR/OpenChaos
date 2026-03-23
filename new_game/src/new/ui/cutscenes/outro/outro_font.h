#ifndef UI_CUTSCENES_OUTRO_OUTRO_FONT_H
#define UI_CUTSCENES_OUTRO_OUTRO_FONT_H

#include "fallen/outro/always.h" // Temporary: outro uses its own type definitions

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

// uc_orig: FONT_char_is_valid (fallen/outro/font.h)
// Returns UC_TRUE if the font module can render the given ASCII character.
SLONG FONT_char_is_valid(CBYTE ch);

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

#endif // UI_CUTSCENES_OUTRO_OUTRO_FONT_H
