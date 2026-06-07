#ifndef ENGINE_GRAPHICS_TEXT_FONT8X8_H
#define ENGINE_GRAPHICS_TEXT_FONT8X8_H

// Public-domain 8x8 bitmap font (font8x8_basic by Daniel Hepper, based on the
// public-domain IBM PC OEM VGA fonts). See THIRD_PARTY_LICENSES.md.
//
// One glyph per printable ASCII code 0x20..0x7E; 8 bytes = 8 rows top-to-bottom;
// within a row, bit N (value 1<<N) is the pixel in column N counted from the LEFT.
//
// The glyph data is defined in game/missing_resources.cpp (where it is also used
// for the self-contained "files not found" screen). Exposed here so the FONT2D
// alternate-atlas generator can rasterise license-clean English glyphs into a
// replacement font texture (see font2d.cpp FONT2D_build_alt_atlas).

#define FONT8X8_FIRST_CH 0x20
#define FONT8X8_LAST_CH 0x7E
#define FONT8X8_GLYPH_PX 8

extern const unsigned char g_font8x8_basic[FONT8X8_LAST_CH - FONT8X8_FIRST_CH + 1][FONT8X8_GLYPH_PX];

#endif // ENGINE_GRAPHICS_TEXT_FONT8X8_H
