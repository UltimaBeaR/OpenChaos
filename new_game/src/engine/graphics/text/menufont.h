#ifndef ENGINE_GRAPHICS_TEXT_MENUFONT_H
#define ENGINE_GRAPHICS_TEXT_MENUFONT_H

#include "engine/platform/uc_common.h"

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

// OpenChaos addition: builds a license-clean ENGLISH replacement atlas for the
// menu font into the given texture slot, with glyphs rasterised from the
// embedded public-domain font8x8 in our own grid. Lets our always-English menu
// text (the help screens) survive when the game's menu font atlas (olyfont2.tga)
// is overwritten by an unofficial localisation. alt_page is the POLY page that
// binds that texture (POLY_PAGE_MENUFONT_ALT). Renders all-caps (lowercase maps
// to the uppercase glyph) to match the game menu font's look. Call once at
// startup; activate per-draw via MENUFONT_AltScope.
void MENUFONT_build_alt_atlas(SLONG alt_texture_slot, SLONG alt_page);

// RAII: temporarily redirect the live menu font (FontInfo glyph table + FontPage)
// to the license-clean English replacement atlas built by MENUFONT_build_alt_atlas,
// so the existing MENUFONT draw path renders our text from that atlas instead of
// the game's. Restores the game font on scope exit. Wrap the help-screen MENUFONT
// calls in one of these. (Mirrors the FONT2D alt-page swap, but the menu font
// draws through globals rather than a page parameter, so we swap the globals.)
// OpenChaos addition.
struct MENUFONT_AltScope {
    MENUFONT_AltScope();
    ~MENUFONT_AltScope();
    MENUFONT_AltScope(const MENUFONT_AltScope&) = delete;
    MENUFONT_AltScope& operator=(const MENUFONT_AltScope&) = delete;

private:
    CharData saved_info_[256];
    SLONG saved_page_;
};

// Draws a string using the menu font with the given flags, scale, and colour.
// uc_orig: MENUFONT_Draw (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Draw(SWORD x, SWORD y, UWORD scale, CBYTE* msg, SLONG rgb, UWORD flags, SWORD max = -1);

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

#endif // ENGINE_GRAPHICS_TEXT_MENUFONT_H
