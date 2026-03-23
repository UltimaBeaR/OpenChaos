#ifndef ENGINE_GRAPHICS_RESOURCES_MENUFONT_H
#define ENGINE_GRAPHICS_RESOURCES_MENUFONT_H

#include <platform.h>

// uc_orig: MENUFONT_FUTZING (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_FUTZING (1)
// uc_orig: MENUFONT_HALOED (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_HALOED (2)
// uc_orig: MENUFONT_GLIMMER (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_GLIMMER (4)
// uc_orig: MENUFONT_CENTRED (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_CENTRED (8)
// uc_orig: MENUFONT_FLANGED (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_FLANGED (16)
// uc_orig: MENUFONT_SINED (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_SINED (32)
// uc_orig: MENUFONT_ONLY (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_ONLY (64)
// uc_orig: MENUFONT_RIGHTALIGN (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_RIGHTALIGN (128)
// uc_orig: MENUFONT_HSCALEONLY (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_HSCALEONLY (256)
// uc_orig: MENUFONT_SUPER_YCTR (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_SUPER_YCTR (512)
// uc_orig: MENUFONT_SHAKE (fallen/DDEngine/Headers/menufont.h)
#define MENUFONT_SHAKE (1024)

// Per-character glyph layout info loaded from the font TGA.
// uc_orig: CharData (fallen/DDEngine/Headers/menufont.h)
struct CharData {
    float x, y, ox, oy; // fractional texture coordinates
    UBYTE width, height;
    UBYTE xofs, yofs;
};

// uc_orig: FontInfo (fallen/DDEngine/Source/menufont.cpp)
extern CharData FontInfo[256];

// Loads and parses the font TGA, filling FontInfo for each character in fontlist.
// uc_orig: MENUFONT_Load (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Load(CBYTE* fn, SLONG page, CBYTE* fontlist);

// Sets which poly page font quads are submitted to.
// uc_orig: MENUFONT_Page (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Page(SLONG page);

// Draws a string using the menu font with the given flags, scale, and colour.
// uc_orig: MENUFONT_Draw (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Draw(SWORD x, SWORD y, UWORD scale, CBYTE* msg, SLONG rgb, UWORD flags, SWORD max = -1);

// Draws a string with floating-point position.
// uc_orig: MENUFONT_Draw_floats (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Draw_floats(float x, float y, UWORD scale, CBYTE* msg, SLONG rgb, UWORD flags);

// Frees the font (clears the font name so the next MENUFONT_Load reloads).
// uc_orig: MENUFONT_Free (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Free();

// Returns rendered pixel width and height of a string at the given scale.
// uc_orig: MENUFONT_Dimensions (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Dimensions(CBYTE* fn, SLONG& x, SLONG& y, SWORD max = -1, SWORD scale = 256);

// Returns how many characters of the string fit within x pixels at the given scale.
// uc_orig: MENUFONT_CharFit (fallen/DDEngine/Source/menufont.cpp)
SLONG MENUFONT_CharFit(CBYTE* fn, SLONG x, UWORD scale = 256);

// Returns pixel width of a single character at the given scale.
// uc_orig: MENUFONT_CharWidth (fallen/DDEngine/Source/menufont.cpp)
SLONG MENUFONT_CharWidth(CBYTE fn, UWORD scale = 256);

// Copies uppercase glyph data into the lowercase slots so lowercase renders correctly.
// Not declared in the original header but called externally via forward declaration.
// uc_orig: MENUFONT_MergeLower (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_MergeLower(void);

// =========================================================================
// Line fade-in text effect.
// =========================================================================

// Resets the fadein state (sets the reveal line to the far right).
// uc_orig: MENUFONT_fadein_init (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_fadein_init(void);

// Sets the x position of the reveal line (8-bit fixed point).
// uc_orig: MENUFONT_fadein_line (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_fadein_line(SLONG x);

// Draws centred text with a horizontal reveal effect.
// uc_orig: MENUFONT_fadein_draw (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_fadein_draw(SLONG x, SLONG y, UBYTE fade, CBYTE* msg);

#endif // ENGINE_GRAPHICS_RESOURCES_MENUFONT_H
