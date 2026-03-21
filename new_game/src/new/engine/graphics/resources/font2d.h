#ifndef ENGINE_GRAPHICS_RESOURCES_FONT2D_H
#define ENGINE_GRAPHICS_RESOURCES_FONT2D_H

#include "core/types.h"
#include "engine/graphics/pipeline/poly.h"

// Per-character UV coordinates and pixel width for the 2D font atlas.
// uc_orig: FONT2D_Letter (fallen/DDEngine/Headers/font2d.h)
typedef struct
{
    float u;
    float v;
    SLONG width; // Width in pixels. Divide by 256.0F to get UV width.
} FONT2D_Letter;

// Initialises the 2D font by scanning the font atlas TGA and computing letter UVs.
// font_id is the texture page index to load the atlas into.
// uc_orig: FONT2D_init (fallen/DDEngine/Headers/font2d.h)
void FONT2D_init(SLONG font_id);

// Returns the pixel width of the given character.
// uc_orig: FONT2D_GetLetterWidth (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_GetLetterWidth(CBYTE chr);

// Draws a single character at (x,y). Returns pixel advance width.
// uc_orig: FONT2D_DrawLetter (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawLetter(CBYTE chr, SLONG x, SLONG y, ULONG rgb = 0xffffff, SLONG scale = 256, SLONG page = POLY_PAGE_FONT2D, SWORD fade = 0);

// Draws a string at (x,y).
// uc_orig: FONT2D_DrawString (fallen/DDEngine/Headers/font2d.h)
void FONT2D_DrawString(CBYTE* chr, SLONG x, SLONG y, ULONG rgb = 0xffffff, SLONG scale = 256, SLONG page = POLY_PAGE_FONT2D, SWORD fade = 0);

// Draws a string with word-wrapping at screen edge (~600px). Returns final y.
// uc_orig: FONT2D_DrawStringWrap (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawStringWrap(CBYTE* chr, SLONG x, SLONG y, ULONG rgb = 0xffffff, SLONG scale = 256, SLONG page = POLY_PAGE_FONT2D, SWORD fade = 0);

// Draws a string with word-wrapping within a given pixel span. Returns final y.
// uc_orig: FONT2D_DrawStringWrapTo (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawStringWrapTo(CBYTE* str, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade, SWORD span);

// Draws a right-justified, word-wrapped string. Returns final y.
// bDontDraw=true calculates layout without rendering.
// uc_orig: FONT2D_DrawStringRightJustify (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawStringRightJustify(CBYTE* chr, SLONG x, SLONG y, ULONG rgb = 0xffffff, SLONG scale = 256, SLONG page = POLY_PAGE_FONT2D, SWORD fade = 0, bool bDontDraw = false);

// Draws a right-justified string on a single line. Returns 0.
// uc_orig: FONT2D_DrawStringRightJustifyNoWrap (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawStringRightJustifyNoWrap(CBYTE* chr, SLONG x, SLONG y, ULONG rgb = 0xffffff, SLONG scale = 256, SLONG page = POLY_PAGE_FONT2D, SWORD fade = 0);

// Draws a centre-justified string at (x,y).
// uc_orig: FONT2D_DrawStringCentred (fallen/DDEngine/Headers/font2d.h)
void FONT2D_DrawStringCentred(CBYTE* chr, SLONG x, SLONG y, ULONG rgb = 0xffffff, SLONG scale = 256, SLONG page = POLY_PAGE_FONT2D, SWORD fade = 0);

// Draws a decorative strikethrough line. bUseLastOffset reuses the previous frame's offset.
// uc_orig: FONT2D_DrawStrikethrough (fallen/DDEngine/Headers/font2d.h)
void FONT2D_DrawStrikethrough(SLONG x1, SLONG x2, SLONG y, ULONG rgb, SLONG scale, SLONG page, SLONG fade, bool bUseLastOffset = false);

// Draws a string at a 3D world position, projected to screen.
// Not declared in original font2d.h; used via extern in facet.cpp and guns.cpp.
// uc_orig: FONT2D_DrawString_3d (fallen/DDEngine/Source/font2d.cpp)
void FONT2D_DrawString_3d(CBYTE* str, ULONG world_x, ULONG world_y, ULONG world_z, ULONG rgb, SLONG text_size, SWORD fade);

// x-coordinate of the rightmost character drawn by the last DrawString call.
// uc_orig: FONT2D_rightmost_x (fallen/DDEngine/Headers/font2d.h)
extern SLONG FONT2D_rightmost_x;

// x-coordinate of the leftmost character drawn during right-justify layout.
// uc_orig: FONT2D_leftmost_x (fallen/DDEngine/Headers/font2d.h)
extern SLONG FONT2D_leftmost_x;

#endif // ENGINE_GRAPHICS_RESOURCES_FONT2D_H
