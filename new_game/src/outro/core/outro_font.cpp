#include "outro/core/outro_font.h"
#include "outro/core/outro_font_globals.h"
#include "outro/core/outro_tga.h"
#include "outro/core/outro_os.h"
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

// uc_orig: FONT_LETTER_HEIGHT (fallen/outro/outroFont.cpp)
#define FONT_LETTER_HEIGHT 21

// uc_orig: FONT_LOWERCASE (fallen/outro/outroFont.cpp)
#define FONT_LOWERCASE 0
// uc_orig: FONT_UPPERCASE (fallen/outro/outroFont.cpp)
#define FONT_UPPERCASE 26
// uc_orig: FONT_NUMBERS (fallen/outro/outroFont.cpp)
#define FONT_NUMBERS 52
// uc_orig: FONT_PUNCT_PLING (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_PLING 62
// uc_orig: FONT_PUNCT_DQUOTE (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_DQUOTE 63
// uc_orig: FONT_PUNCT_POUND (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_POUND 64
// uc_orig: FONT_PUNCT_DOLLAR (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_DOLLAR 65
// uc_orig: FONT_PUNCT_PERCENT (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_PERCENT 66
// uc_orig: FONT_PUNCT_POWER (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_POWER 67
// uc_orig: FONT_PUNCT_AMPERSAND (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_AMPERSAND 68
// uc_orig: FONT_PUNCT_ASTERISK (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_ASTERISK 69
// uc_orig: FONT_PUNCT_OPEN (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_OPEN 70
// uc_orig: FONT_PUNCT_CLOSE (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_CLOSE 71
// uc_orig: FONT_PUNCT_COPEN (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_COPEN 72
// uc_orig: FONT_PUNCT_CCLOSE (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_CCLOSE 73
// uc_orig: FONT_PUNCT_SOPEN (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_SOPEN 74
// uc_orig: FONT_PUNCT_SCLOSE (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_SCLOSE 75
// uc_orig: FONT_PUNCT_LT (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_LT 76
// uc_orig: FONT_PUNCT_GT (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_GT 77
// uc_orig: FONT_PUNCT_BSLASH (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_BSLASH 78
// uc_orig: FONT_PUNCT_FSLASH (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_FSLASH 79
// uc_orig: FONT_PUNCT_COLON (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_COLON 80
// uc_orig: FONT_PUNCT_SEMICOLON (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_SEMICOLON 81
// uc_orig: FONT_PUNCT_QUOTE (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_QUOTE 82
// uc_orig: FONT_PUNCT_AT (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_AT 83
// uc_orig: FONT_PUNCT_HASH (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_HASH 84
// uc_orig: FONT_PUNCT_TILDE (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_TILDE 85
// uc_orig: FONT_PUNCT_QMARK (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_QMARK 86
// uc_orig: FONT_PUNCT_MINUS (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_MINUS 87
// uc_orig: FONT_PUNCT_EQUALS (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_EQUALS 88
// uc_orig: FONT_PUNCT_PLUS (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_PLUS 89
// uc_orig: FONT_PUNCT_DOT (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_DOT 90
// uc_orig: FONT_PUNCT_COMMA (fallen/outro/outroFont.cpp)
#define FONT_PUNCT_COMMA 91

// uc_orig: FONT_found_data (fallen/outro/outroFont.cpp)
// Returns UC_TRUE if any pixel in a vertical strip at column x has non-zero alpha.
// Used to locate left/right edges of each glyph in the font texture.
static SLONG FONT_found_data(SLONG x, SLONG y)
{
    SLONG dy;
    SLONG px;
    SLONG py;

    ASSERT(WITHIN(x, 0, 255));

    for (dy = -16; dy <= 4; dy++) {
        px = x;
        py = y + dy;

        if (WITHIN(py, 0, 255)) {
            if (FONT_data[255 - py][px].alpha > 32) {
                return UC_TRUE;
            }
        }
    }

    return UC_FALSE;
}

// uc_orig: FONT_init (fallen/outro/outroFont.cpp)
void FONT_init()
{
    SLONG i;
    SLONG y;
    SLONG x;

    FONT_Letter* fl;

    FONT_ot = OS_texture_create("font.tga");

    OUTRO_TGA_Info ti;

    ti = OUTRO_TGA_load(
        "Textures/font.tga",
        256,
        256,
        &FONT_data[0][0]);

    ASSERT(ti.valid);
    ASSERT(ti.width == 256);
    ASSERT(ti.height == 256);

    x = 0;
    y = 19;

    for (i = 0; i < FONT_NUM_LETTERS; i++) {
        fl = &FONT_letter[i];

        while (!FONT_found_data(x, y)) {
            x += 1;

            if (x >= 256) {
                x = 0;
                y += 22;

                if (y > 256) {
                    return;
                }
            }
        }

        fl->u = float(x);
        fl->v = float(y);

        x += 3;

        while (FONT_found_data(x, y)) {
            x += 1;
        }

        fl->uwidth = (x - fl->u) * (1.0F / 256.0F);

        fl->u *= 1.0F / 256.0F;
        fl->v *= 1.0F / 256.0F;

        fl->v -= 16.0F / 256.0F;
    }
}

// uc_orig: FONT_get_index (fallen/outro/outroFont.cpp)
// Returns the font letter array index for the given character.
static SLONG FONT_get_index(CBYTE chr)
{
    SLONG letter;

    if (WITHIN(chr, 'a', 'z')) {
        letter = FONT_LOWERCASE + chr - 'a';
    } else if (WITHIN(chr, 'A', 'Z')) {
        letter = FONT_UPPERCASE + chr - 'A';
    } else if (WITHIN(chr, '0', '9')) {
        letter = FONT_NUMBERS + chr - '0';
    } else {
        letter = FONT_PUNCT_PLING;

        for (CBYTE* ch = OUTRO_FONT_punct; *ch && *ch != chr; ch++, letter++)
            ;
    }

    if (!WITHIN(letter, 0, FONT_NUM_LETTERS - 1)) {
        letter = FONT_PUNCT_QMARK;
    }

    return letter;
}

// uc_orig: FONT_char_is_valid (fallen/outro/outroFont.cpp)
SLONG FONT_char_is_valid(CBYTE ch)
{
    if (FONT_get_index(ch) == FONT_PUNCT_QMARK && ch != '?') {
        return UC_FALSE;
    } else {
        return UC_TRUE;
    }
}

// uc_orig: FONT_get_letter_width (fallen/outro/outroFont.cpp)
// Returns normalised width of a single character (space = 8/256).
static float FONT_get_letter_width(CBYTE chr)
{
    SLONG letter;

    if (chr == ' ') {
        return 8.0F / 256.0F;
    }

    letter = FONT_get_index(chr);

    ASSERT(WITHIN(letter, 0, FONT_NUM_LETTERS - 1));

    return FONT_letter[letter].uwidth + (1.0F / 256.0F);
}

// uc_orig: FONT_draw_letter (fallen/outro/outroFont.cpp)
// Adds a single glyph sprite to the buffer. Returns the advance width.
// shimmer != 0 draws the glyph in horizontal strips with a sine-wave wobble.
static float FONT_draw_letter(
    OS_Buffer* ob,
    CBYTE chr,
    float x,
    float y,
    ULONG colour = 0xffffffff,
    float scale = 1.0F,
    float shimmer = 0.0F,
    SLONG italic = UC_FALSE)
{
    SLONG letter;
    float width;
    float lean;

    FONT_Letter* fl;

    lean = (italic) ? scale * 0.02F : 0.0F;

    if (chr == ' ') {
        width = (10.0F / 256.0F) * scale;
    } else {
        letter = FONT_get_index(chr);

        ASSERT(WITHIN(letter, 0, FONT_NUM_LETTERS - 1));

        fl = &FONT_letter[letter];

        width = fl->uwidth;

        if (shimmer == 0.0F) {
            /*

            OS_buffer_add_sprite(
                    ob,
                    x,
                    y,
                    x + fl->uwidth * scale,
                    y + (FONT_LETTER_HEIGHT * 1.33F / 256.0F) * scale,
                    fl->u,
                    1.0F - (fl->v),
                    fl->u + fl->uwidth,
                    1.0F - (fl->v + (FONT_LETTER_HEIGHT / 256.0F)),
                    0.0F,
                    colour);
            */

            OS_buffer_add_sprite_arbitrary(
                ob,
                x + lean, y,
                x + lean + fl->uwidth * scale, y,
                x,
                y + (FONT_LETTER_HEIGHT * 1.33F / 256.0F) * scale,
                x + fl->uwidth * scale,
                y + (FONT_LETTER_HEIGHT * 1.33F / 256.0F) * scale,
                fl->u,
                1.0F - (fl->v),
                fl->u + fl->uwidth,
                1.0F - (fl->v),
                fl->u,
                1.0F - (fl->v + (FONT_LETTER_HEIGHT / 256.0F)),
                fl->u + fl->uwidth,
                1.0F - (fl->v + (FONT_LETTER_HEIGHT / 256.0F)),
                0.0F,
                colour);
        } else {
            SLONG i;

// uc_orig: FONT_SHIMMER_SEGS (fallen/outro/outroFont.cpp)
#define FONT_SHIMMER_SEGS 12
// uc_orig: FONT_SHIMMER_DANGLE (fallen/outro/outroFont.cpp)
#define FONT_SHIMMER_DANGLE 0.8F
// uc_orig: FONT_SHIMMER_AMOUNT (fallen/outro/outroFont.cpp)
#define FONT_SHIMMER_AMOUNT 0.01F

            float dx_last;
            float dx_now;
            float v = fl->v;
            float dy = (FONT_LETTER_HEIGHT * 1.33F / 256.0F) * scale / FONT_SHIMMER_SEGS;
            float dv = (FONT_LETTER_HEIGHT * 1.00F / 256.0F) / FONT_SHIMMER_SEGS;
            float angle = OS_ticks() * 0.004F;
            float dlean = lean * (1.0F / FONT_SHIMMER_SEGS);

            shimmer *= scale;
            shimmer *= FONT_SHIMMER_AMOUNT;

            dx_last = sin(angle - FONT_SHIMMER_DANGLE) * shimmer + lean;

            for (i = 0; i < FONT_SHIMMER_SEGS; i++) {
                lean -= dlean;
                dx_now = sin(angle) * shimmer + lean;

                OS_buffer_add_sprite_arbitrary(
                    ob,
                    x + dx_last, y, x + fl->uwidth * scale + dx_last, y,
                    x + dx_now, y + dy, x + fl->uwidth * scale + dx_now, y + dy,
                    fl->u, 1.0F - v,
                    fl->u + fl->uwidth, 1.0F - v,
                    fl->u, 1.0F - (v + dv),
                    fl->u + fl->uwidth, 1.0F - (v + dv),
                    0.0F,
                    colour);

                y += dy;
                v += dv;
                dx_last = dx_now;
                angle += FONT_SHIMMER_DANGLE;
            }
        }
    }

    return (width + 1.0F / 256.0F) * scale;
}

// uc_orig: FONT_get_width (fallen/outro/outroFont.cpp)
// Returns the total width of a string in normalised screen coordinates.
static float FONT_get_width(CBYTE* str, float scale)
{
    float ans = 0.0F;

    for (CBYTE* ch = str; *ch; ch++) {
        ans += FONT_get_letter_width(*ch) * scale;
    }

    return ans;
}

// uc_orig: FONT_draw (fallen/outro/outroFont.cpp)
void FONT_draw(SLONG flag, float start_x, float start_y, ULONG colour, float scale, SLONG cursor, float shimmer, CBYTE* fmt, ...)
{
    CBYTE message[4096];
    va_list ap;

    if (fmt == NULL) {
        sprintf(message, "<NULL>");
    } else {
        va_start(ap, fmt);
        vsprintf(message, fmt, ap);
        va_end(ap);
    }

    // A scale of 1.0F is normal size.
    scale *= 0.5F;

    OS_Buffer* ob = OS_buffer_new();

    SLONG alpha;

    SATURATE(shimmer, 0.0F, 1.0F);

    alpha = ftol(255.0F * (1.0F - shimmer));
    colour |= alpha << 24;

    float x = start_x;
    float y = start_y;

    if (flag & FONT_FLAG_JUSTIFY_CENTRE) {
        x -= FONT_get_width(message, scale) * 0.5F;
    } else if (flag & FONT_FLAG_JUSTIFY_RIGHT) {
        x -= FONT_get_width(message, scale);
    }

    CBYTE* ch = message;

    while (*ch) {
        if (*ch == '\n') {
            x = start_x;
            y += (FONT_LETTER_HEIGHT + 1.0F) * scale;
        } else {
            if (cursor-- == 0) {
                {
                    OS_Buffer* ob = OS_buffer_new();

                    OS_buffer_add_sprite(
                        ob,
                        x, y, x + 0.01F * scale, y + (FONT_LETTER_HEIGHT * 1.33F / 256.0F) * scale,
                        0.0F, 0.0F,
                        1.0F, 1.0F,
                        0.0F,
                        0xeeeeeff);

                    OS_buffer_draw(ob, NULL, NULL);
                }
            }

            x += FONT_draw_letter(ob, *ch, x, y, colour, scale, shimmer, flag & FONT_FLAG_ITALIC);
        }

        ch += 1;
    }

    if (cursor-- == 0) {
        {
            OS_Buffer* ob = OS_buffer_new();

            OS_buffer_add_sprite(
                ob,
                x, y, x + 0.01F * scale, y + (FONT_LETTER_HEIGHT * 1.33F / 256.0F) * scale,
                0.0F, 0.0F,
                1.0F, 1.0F,
                0.0F,
                0xeeeeeff);

            OS_buffer_draw(ob, NULL, NULL);
        }
    }

    OS_buffer_draw(ob, FONT_ot, NULL, OS_DRAW_DOUBLESIDED | OS_DRAW_ZALWAYS | OS_DRAW_NOZWRITE | OS_DRAW_ALPHABLEND);

    FONT_end_x = x;
    FONT_end_y = y;
}
