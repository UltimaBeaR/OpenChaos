#ifndef UI_CUTSCENES_OUTRO_OUTRO_FONT_GLOBALS_H
#define UI_CUTSCENES_OUTRO_OUTRO_FONT_GLOBALS_H

#include "fallen/outro/always.h" // Temporary: outro uses its own type definitions
#include "fallen/outro/Tga.h"    // Temporary: OUTRO_TGA_Pixel
#include "fallen/outro/os.h"     // Temporary: OS_Texture

// uc_orig: FONT_Letter (fallen/outro/outroFont.cpp)
// UV and width data for each letter in the font texture.
struct FONT_Letter
{
    float u;
    float v;
    float uwidth;
};

// uc_orig: FONT_NUM_FOREIGN (fallen/outro/outroFont.cpp)
#define FONT_NUM_FOREIGN 66
// uc_orig: FONT_NUM_LETTERS (fallen/outro/outroFont.cpp)
#define FONT_NUM_LETTERS (92 + FONT_NUM_FOREIGN)

// uc_orig: FONT_letter (fallen/outro/outroFont.cpp)
extern FONT_Letter FONT_letter[FONT_NUM_LETTERS];

// uc_orig: FONT_punct (fallen/outro/outroFont.cpp)
// Lookup table mapping punctuation ASCII chars to font letter indices.
extern CBYTE FONT_punct[];

// uc_orig: FONT_ot (fallen/outro/outroFont.cpp)
// The font texture used for rendering glyphs.
extern OS_Texture* FONT_ot;

// uc_orig: FONT_data (fallen/outro/outroFont.cpp)
// In-memory copy of the font TGA (256x256 RGBA), used to locate letter boundaries.
extern OUTRO_TGA_Pixel FONT_data[256][256];

// uc_orig: FONT_end_x (fallen/outro/outroFont.cpp)
// X position where the next character would be drawn after FONT_draw returns.
extern float FONT_end_x;

// uc_orig: FONT_end_y (fallen/outro/outroFont.cpp)
// Y position where the next character would be drawn after FONT_draw returns.
extern float FONT_end_y;

#endif // UI_CUTSCENES_OUTRO_OUTRO_FONT_GLOBALS_H
