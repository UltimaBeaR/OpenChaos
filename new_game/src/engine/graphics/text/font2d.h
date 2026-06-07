#ifndef ENGINE_GRAPHICS_TEXT_FONT2D_H
#define ENGINE_GRAPHICS_TEXT_FONT2D_H

#include "engine/core/types.h"
#include "engine/graphics/pipeline/poly.h"

// Character atlas is 256x256 pixels with a 16px row height. The drawn letter
// cell height in pixels at scale 256 (1.0). Exported so callers laying text out
// alongside other elements (e.g. inline input glyphs) can size lines to match.
// uc_orig: FONT2D_LETTER_HEIGHT (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_LETTER_HEIGHT 16

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

// OpenChaos addition: builds a license-clean ENGLISH replacement atlas into the
// given texture slot, with glyphs rasterised (from the embedded public-domain
// font8x8) into the SAME per-letter cells FONT2D_init scanned. Lets our own
// always-English text (drawn via FONT2D_DrawLetter with the alternate POLY page)
// survive when the game's font atlas is overwritten by an unofficial
// localisation. Must be called AFTER FONT2D_init (needs the scanned FONT2D_letter
// UVs). See poly.h POLY_PAGE_FONT2D_ALT.
void FONT2D_build_alt_atlas(SLONG alt_texture_slot);

// Returns the pixel width of the given character.
// uc_orig: FONT2D_GetLetterWidth (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_GetLetterWidth(CBYTE chr);

// Pixel width of a character in the alternate (English replacement) font — used
// to lay out text drawn through POLY_PAGE_FONT2D_ALT so advance/wrap match.
// OpenChaos addition (see FONT2D_build_alt_atlas).
SLONG FONT2D_GetLetterWidthAlt(CBYTE chr);

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

// x-coordinate of the rightmost character drawn by the last DrawString call.
// uc_orig: FONT2D_rightmost_x (fallen/DDEngine/Headers/font2d.h)
extern SLONG FONT2D_rightmost_x;

// x-coordinate of the leftmost character drawn during right-justify layout.
// uc_orig: FONT2D_leftmost_x (fallen/DDEngine/Headers/font2d.h)
extern SLONG FONT2D_leftmost_x;

#endif // ENGINE_GRAPHICS_TEXT_FONT2D_H
