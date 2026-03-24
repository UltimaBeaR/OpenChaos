#include "engine/platform/uc_common.h"
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/text/font2d_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "assets/tga.h"

// 256×256 array of TGA pixels — the actual type behind the void* font2d_data global.
// uc_orig: MyArrayType (fallen/DDEngine/Source/font2d.cpp)
typedef TGA_Pixel FontAtlasPixels[256][256];

// Character atlas is 256x256 pixels with a 16px row height.
// uc_orig: FONT2D_LETTER_HEIGHT (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_LETTER_HEIGHT 16

// Character-set layout constants — offsets into FONT2D_letter[].
// uc_orig: FONT2D_LOWERCASE (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_LOWERCASE 0
// uc_orig: FONT2D_UPPERCASE (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_UPPERCASE 26
// uc_orig: FONT2D_NUMBERS (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_NUMBERS 52
// uc_orig: FONT2D_PUNCT_PLING (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_PUNCT_PLING 62
// Punctuation block index constants (same names as original).
// uc_orig: FONT2D_PUNCT_DQUOTE..FONT2D_PUNCT_COMMA (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_PUNCT_DQUOTE 63
#define FONT2D_PUNCT_POUND 64
#define FONT2D_PUNCT_DOLLAR 65
#define FONT2D_PUNCT_PERCENT 66
#define FONT2D_PUNCT_POWER 67
#define FONT2D_PUNCT_AMPERSAND 68
#define FONT2D_PUNCT_ASTERISK 69
#define FONT2D_PUNCT_OPEN 70
#define FONT2D_PUNCT_CLOSE 71
#define FONT2D_PUNCT_COPEN 72
#define FONT2D_PUNCT_CCLOSE 73
#define FONT2D_PUNCT_SOPEN 74
#define FONT2D_PUNCT_SCLOSE 75
#define FONT2D_PUNCT_LT 76
#define FONT2D_PUNCT_GT 77
#define FONT2D_PUNCT_BSLASH 78
#define FONT2D_PUNCT_FSLASH 79
#define FONT2D_PUNCT_COLON 80
#define FONT2D_PUNCT_SEMICOLON 81
#define FONT2D_PUNCT_QUOTE 82
#define FONT2D_PUNCT_AT 83
#define FONT2D_PUNCT_HASH 84
#define FONT2D_PUNCT_TILDE 85
#define FONT2D_PUNCT_QMARK 86
#define FONT2D_PUNCT_MINUS 87
#define FONT2D_PUNCT_EQUALS 88
#define FONT2D_PUNCT_PLUS 89
#define FONT2D_PUNCT_DOT 90
#define FONT2D_PUNCT_COMMA 91
// Extra localisation character counts.
// uc_orig: FONT2D_GERMAN_CHARS (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_GERMAN_CHARS 10
// uc_orig: FONT2D_FRENCH_CHARS (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_FRENCH_CHARS 14
// uc_orig: FONT2D_SPANISH_CHARS (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_SPANISH_CHARS 11
// uc_orig: FONT2D_ITALIAN_CHARS (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_ITALIAN_CHARS 12
// uc_orig: FONT2D_NUM_LETTERS (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_NUM_LETTERS (92 + FONT2D_GERMAN_CHARS + FONT2D_FRENCH_CHARS + FONT2D_SPANISH_CHARS + FONT2D_ITALIAN_CHARS)

// Forward declarations for panel helpers (panel.cpp not yet migrated).
// uc_orig: PANEL_draw_quad (fallen/DDEngine/Source/panel.cpp)
extern void PANEL_draw_quad(float left, float top, float right, float bottom,
    SLONG page, ULONG colour, float u1, float v1, float u2, float v2);
// uc_orig: PANEL_GetNextDepthBodge (fallen/DDEngine/Source/panel.cpp)
extern float PANEL_GetNextDepthBodge(void);

// Convenience cast macro — font2d_data is void* in globals to avoid tga.h dependency in engine layer.
// The actual dereferenced type is TGA_Pixel[256][256].
// uc_orig: FONT2D_data (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_DATA ((FontAtlasPixels*)font2d_data)

// Returns true if any pixel in the column at x within a 17-row window around y has alpha.
// Used to detect glyph boundaries during atlas scanning.
// uc_orig: FONT2D_found_data (fallen/DDEngine/Source/font2d.cpp)
static SLONG FONT2D_found_data(SLONG x, SLONG y)
{
    ASSERT(font2d_data != NULL);
    ASSERT(WITHIN(x, 0, 255));

    for (SLONG dy = -14; dy <= 3; dy++) {
        SLONG py = y + dy;
        if (WITHIN(py, 0, 255)) {
            if (((*FONT2D_DATA)[py][x]).alpha) {
                return UC_TRUE;
            }
        }
    }
    return UC_FALSE;
}

// uc_orig: FONT2D_init (fallen/DDEngine/Headers/font2d.h)
void FONT2D_init(SLONG font_id)
{
    TGA_Info ti;

// uc_orig: TEXTURE_EXTRA_DIR (fallen/DDEngine/Source/font2d.cpp)
#define TEXTURE_EXTRA_DIR "server\\textures\\extras\\"

    CBYTE fname[256];
    font2d_data = MemAlloc(sizeof(TGA_Pixel) * 256 * 256);
    ASSERT(font2d_data != NULL);

    sprintf(fname, "%s%s", TEXTURE_EXTRA_DIR, "multifontPC.tga");

    ti = TGA_load(fname, 256, 256, &((*FONT2D_DATA)[0][0]), font_id, UC_FALSE);

    ASSERT(ti.valid);
    ASSERT(ti.width == 256);
    ASSERT(ti.height == 256);

// uc_orig: FONT2D_NUM_BASELINES (fallen/DDEngine/Source/font2d.cpp)
#define FONT2D_NUM_BASELINES 8

    SLONG baseline[FONT2D_NUM_BASELINES] = { 17, 37, 57, 77, 97, 117, 137, 157 };

    SLONG x = 0;
    SLONG y = baseline[0];
    SLONG line = 0;

    for (SLONG i = 0; i < FONT2D_NUM_LETTERS; i++) {
        FONT2D_Letter* fl = &FONT2D_letter[i];

        // Scan right to find where this glyph starts.
        while (!FONT2D_found_data(x, y)) {
            x += 1;
            if (x >= 256) {
                x = 0;
                line += 1;
                if (!WITHIN(line, 0, FONT2D_NUM_BASELINES - 1)) {
                    return;
                }
                y = baseline[line];
            }
        }

        fl->u = (float)x;
        fl->v = (float)y;

        // Scan right to find where this glyph ends.
        x += 1;
        while (FONT2D_found_data(x, y)) {
            x += 1;
        }

        fl->width = x + 1.0F - fl->u;

        // Convert pixel coords to normalised UV [0..1].
        fl->u *= 1.0F / 256.0F;
        fl->v *= 1.0F / 256.0F;
        fl->v -= 14.0F / 256.0F;
    }

    MemFree(font2d_data);
    font2d_data = NULL;
}

// Returns the FONT2D_letter[] index for the given character code.
// uc_orig: FONT2D_GetIndex (fallen/DDEngine/Source/font2d.cpp)
static SLONG FONT2D_GetIndex(CBYTE chr)
{
    SLONG letter;

    // Remap a Windows-1252 smart-quote to a standard apostrophe.
    if (chr == '\xc9')
        chr = 'E';

    if (WITHIN(chr, 'a', 'z')) {
        letter = FONT2D_LOWERCASE + chr - 'a';
    } else if (WITHIN(chr, 'A', 'Z')) {
        letter = FONT2D_UPPERCASE + chr - 'A';
    } else if (WITHIN(chr, '0', '9')) {
        letter = FONT2D_NUMBERS + chr - '0';
    } else {
        letter = FONT2D_PUNCT_PLING;
        for (CBYTE* ch = FONT2D_punct; *ch && *ch != chr; ch++, letter++)
            ;
    }

    if (!WITHIN(letter, 0, FONT2D_NUM_LETTERS - 1)) {
        letter = FONT2D_PUNCT_QMARK;
    }

    return letter;
}

// uc_orig: FONT2D_GetLetterWidth (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_GetLetterWidth(CBYTE chr)
{
    // Space and non-breaking space (0xAC) are always 8px wide.
    if ((chr == ' ') || (chr == '\xac')) {
        return 8;
    }

    SLONG letter = FONT2D_GetIndex(chr);
    ASSERT(WITHIN(letter, 0, FONT2D_NUM_LETTERS - 1));
    return FONT2D_letter[letter].width + 1;
}

// uc_orig: FONT2D_DrawLetter (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawLetter(CBYTE chr, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade)
{
    // Windows-1252 left single quotation mark → ASCII apostrophe.
    if (chr == -110) {
        chr = 39;
    }

    // Whitespace characters advance without drawing.
    if (chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t' || chr == '\xac') {
        return 8 * scale >> 8;
    }

    // 'y' has a descender that extends left into the previous character's space.
    if (chr == 'y') {
        x -= 2 * scale >> 8;
    }

    SLONG letter = FONT2D_GetIndex(chr);
    ASSERT(WITHIN(letter, 0, FONT2D_NUM_LETTERS - 1));

    FONT2D_Letter* fl = &FONT2D_letter[letter];

    SATURATE(fade, 0, 255);

    PANEL_draw_quad(
        (float)x,
        (float)(y - 3),
        (float)(x + (fl->width * scale >> 8)),
        (float)(y + (15 * scale >> 8)),
        page,
        (rgb & 0xffffff) | ((255 - fade) << 24),
        fl->u,
        fl->v,
        fl->u + float(fl->width) * (1.0F / 256.0F),
        fl->v + 18.0F * (1.0F / 256.0F));

    if (chr == 'y') {
        return ((fl->width + 1) * scale >> 8) - (2 * scale >> 8);
    } else {
        return (fl->width + 1) * scale >> 8;
    }
}

// uc_orig: FONT2D_DrawString (fallen/DDEngine/Headers/font2d.h)
void FONT2D_DrawString(CBYTE* str, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade)
{
    if (str == NULL) {
        str = "Null string";
    }

    FONT2D_rightmost_x = x;

    for (UBYTE i = 0; i < strlen(str); i++) {
        if (str[i] == '\t') {
            x &= ~127;
            x += 128;
        } else {
            x += FONT2D_DrawLetter(str[i], x, y, rgb, scale, page, fade);
        }
        if (x > FONT2D_rightmost_x) {
            FONT2D_rightmost_x = x;
        }
    }
}

// Like FONT2D_DrawString but always uses the bitmap path (no TrueType).
// Only called internally from FONT2D_DrawString_3d.
// uc_orig: FONT2D_DrawString_NoTrueType (fallen/DDEngine/Source/font2d.cpp)
static void FONT2D_DrawString_NoTrueType(CBYTE* str, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade)
{
    if (str == NULL) {
        str = "Null string";
    }

    for (UBYTE i = 0; i < strlen(str); i++) {
        if (str[i] == '\t') {
            x &= ~127;
            x += 128;
        } else {
            x += FONT2D_DrawLetter(str[i], x, y, rgb, scale, page, fade);
        }
    }
}

// uc_orig: FONT2D_DrawStringWrap (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawStringWrap(CBYTE* str, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade)
{
    if (str == NULL) {
        str = "Null string";
    }

    SLONG xbase = x;
    SLONG len = strlen(str);
    FONT2D_rightmost_x = x + 8;

    for (SLONG i = 0; i < len; i++) {
        if (str[i] == ' ') {
            x += FONT2D_GetLetterWidth(' ');

            // Look ahead to see if the next word fits before the screen edge (~600px).
            CBYTE* ch = &str[i + 1];
            SLONG xlook = x;
            while (*ch && *ch != ' ') {
                xlook += FONT2D_GetLetterWidth(*ch);
                ch += 1;
            }

            if (xlook >= 600) {
                x = xbase;
                y += FONT2D_LETTER_HEIGHT - 1;
            }
        } else {
            x += FONT2D_DrawLetter(str[i], x, y, rgb, scale, page, fade);
            if (x > FONT2D_rightmost_x) {
                FONT2D_rightmost_x = x;
            }
        }
    }

    return y;
}

// uc_orig: FONT2D_DrawStringWrapTo (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawStringWrapTo(CBYTE* str, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade, SWORD span)
{
    if (str == NULL) {
        str = "Null string";
    }

    SLONG xbase = x;
    SLONG len = strlen(str);
    FONT2D_rightmost_x = x + 8;

    for (SLONG i = 0; i < len; i++) {
        if (str[i] == ' ') {
            x += FONT2D_GetLetterWidth(' ');

            CBYTE* ch = &str[i + 1];
            SLONG xlook = x;
            while (*ch && *ch != ' ') {
                xlook += FONT2D_GetLetterWidth(*ch);
                ch += 1;
            }

            if (xlook >= span) {
                x = xbase;
                y += FONT2D_LETTER_HEIGHT - 1;
            }
        } else {
            if (str[i] == 13) {
                // Carriage return forces a new line.
                x = xbase;
                y += FONT2D_LETTER_HEIGHT - 1;
            } else {
                x += FONT2D_DrawLetter(str[i], x, y, rgb, scale, page, fade);
            }
            if (x > FONT2D_rightmost_x) {
                FONT2D_rightmost_x = x;
            }
        }
    }

    return y;
}

// uc_orig: FONT2D_DrawStringRightJustify (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawStringRightJustify(CBYTE* str, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade, bool bDontDraw)
{
    if (str == NULL) {
        str = "Null string";
    }

    // xbase is the right margin minus one character width of padding.
    SLONG xbase = x - ((scale >> 4) - 7);
    SLONG len = strlen(str);
    SLONG drawn_upto = 0;

    x = xbase;
    FONT2D_leftmost_x = x;

    for (SLONG i = 0; i <= len; i++) {
        if (str[i] == ' ' || str[i] == '\000') {
            SLONG xlook;
            if (str[i] == '\000') {
                // End of string — always draw this last line.
                xlook = (SLONG)-UC_INFINITY;
            } else {
                // Look ahead to check if the next word fits.
                CBYTE* ch = &str[i + 1];
                xlook = x;
                while (*ch && *ch != ' ') {
                    xlook -= FONT2D_GetLetterWidth(*ch);
                    ch += 1;
                }
            }

            if (xlook < 10) {
                CBYTE backup = str[i];
                str[i] = '\000';

                if (!bDontDraw) {
                    FONT2D_DrawString(&str[drawn_upto], x, y, rgb, scale, page, fade);
                }

                str[i] = backup;
                drawn_upto = i + 1;
                y += FONT2D_LETTER_HEIGHT - 1;
                x = xbase + FONT2D_GetLetterWidth(str[i]);
            }
        }

        x -= FONT2D_GetLetterWidth(str[i]);

        if (x < FONT2D_leftmost_x) {
            FONT2D_leftmost_x = x;
        }
    }

    return y;
}

// uc_orig: FONT2D_DrawStringRightJustifyNoWrap (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_DrawStringRightJustifyNoWrap(CBYTE* str, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade)
{
    if (str == NULL) {
        str = "Null string";
    }

    // Walk backward through the string to find the starting x.
    for (CBYTE* ch = str; *ch; ch++) {
        x -= FONT2D_GetLetterWidth(*ch);
    }

    FONT2D_DrawString(str, x, y, rgb, scale, page, fade);
    return 0;
}

// uc_orig: FONT2D_DrawStringCentred (fallen/DDEngine/Headers/font2d.h)
void FONT2D_DrawStringCentred(CBYTE* chr, SLONG x, SLONG y, ULONG rgb, SLONG scale, SLONG page, SWORD fade)
{
    SLONG length = 0;
    for (CBYTE* ch = chr; *ch; ch++) {
        length += FONT2D_GetLetterWidth(*ch) * scale >> 8;
    }
    x -= length / 2;
    FONT2D_DrawString(chr, x, y, rgb, scale, page, fade);
}

// Draws the string at a 3D world coordinate, projected onto the screen.
// Scale is derived from the projected Z depth.
// uc_orig: FONT2D_DrawString_3d (fallen/DDEngine/Source/font2d.cpp)
void FONT2D_DrawString_3d(CBYTE* str, ULONG world_x, ULONG world_y, ULONG world_z, ULONG rgb, SLONG text_size, SWORD fade)
{
    if (str == NULL) {
        str = "Null string";
    }

    POLY_Point mid;
    POLY_transform(world_x, world_y, world_z, &mid);

    if (mid.IsValid()) {
        float x = mid.X;
        float y = mid.Y;
        SLONG scale = SLONG(mid.Z * 6 * 256);

        // Compute total pixel width and centre horizontally.
        SLONG len = 0;
        for (CBYTE* ch = str; *ch; ch++) {
            len += FONT2D_GetLetterWidth(*ch) * scale >> 8;
        }

        x -= len / 2;

        FONT2D_DrawString_NoTrueType(str, (SLONG)x, (SLONG)y, 0xFFFFFF, scale, POLY_PAGE_FONT2D, 0);
    }
}

// uc_orig: FONT2D_DrawStrikethrough (fallen/DDEngine/Headers/font2d.h)
void FONT2D_DrawStrikethrough(SLONG x1, SLONG x2, SLONG y, ULONG rgb, SLONG scale, SLONG page, SLONG fade, bool bUseLastOffset)
{
    SLONG letter = FONT2D_GetIndex('-');
    FONT2D_Letter* fl = &FONT2D_letter[letter];

    SLONG offset1;
    SLONG offset2;

    if (bUseLastOffset) {
        offset1 = DST_offset1;
        offset2 = DST_offset2;
    } else {
        // Pseudo-random wobble based on screen position.
        offset1 = (y / 5) % 9;
        offset2 = offset1 * (y / 20);
        offset1 >>= 3;
        offset2 %= 11;

        DST_offset1 = offset1;
        DST_offset2 = offset2;
    }

    POLY_Point pp[4];
    POLY_Point* quad[4];

    float fWDepthBodge = PANEL_GetNextDepthBodge();
    float fZDepthBodge = 1.0f - fWDepthBodge;

    pp[0].X = (float)x1;
    pp[0].Y = (float)(y - 3 + offset1 - 1);
    pp[0].z = fZDepthBodge;
    pp[0].Z = fWDepthBodge;
    pp[0].u = fl->u;
    pp[0].v = fl->v;
    pp[0].colour = rgb | ((255 - fade) << 24);
    pp[0].specular = 0xff000000;

    pp[1].X = (float)x2;
    pp[1].Y = (float)(y - 3 + offset2 - 3);
    pp[1].z = fZDepthBodge;
    pp[1].Z = fWDepthBodge;
    pp[1].u = fl->u + float(fl->width) * (1.0F / 256.0F);
    pp[1].v = fl->v;
    pp[1].colour = rgb | ((255 - fade) << 24);
    pp[1].specular = 0xff000000;

    pp[2].X = (float)x1;
    pp[2].Y = (float)(y + (15 * scale >> 8) + offset1 - 1);
    pp[2].z = fZDepthBodge;
    pp[2].Z = fWDepthBodge;
    pp[2].u = fl->u;
    pp[2].v = fl->v + 18.0F * (1.0F / 256.0F);
    pp[2].colour = rgb | ((255 - fade) << 24);
    pp[2].specular = 0xff000000;

    pp[3].X = (float)x2;
    pp[3].Y = (float)(y + (15 * scale >> 8) + offset2 - 3);
    pp[3].z = fZDepthBodge;
    pp[3].Z = fWDepthBodge;
    pp[3].u = fl->u + float(fl->width) * (1.0F / 256.0F);
    pp[3].v = fl->v + 18.0F * (1.0F / 256.0F);
    pp[3].colour = rgb | ((255 - fade) << 24);
    pp[3].specular = 0xff000000;

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
}
