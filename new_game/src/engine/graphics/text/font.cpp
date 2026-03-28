#include "engine/graphics/text/font.h"
#include "engine/graphics/text/font_globals.h"

#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

#include "engine/core/macros.h"
#include <math.h>

// uc_orig: FONT_TAB (fallen/DDEngine/Source/Font.cpp)
#define FONT_TAB 16

// uc_orig: FONT_draw_coloured_char (fallen/DDEngine/Source/Font.cpp)
SLONG FONT_draw_coloured_char(
    SLONG sx,
    SLONG sy,
    UBYTE red,
    UBYTE green,
    UBYTE blue,
    CBYTE ch)
{
    SLONG b;
    SLONG x;
    SLONG y;

    FONT_Char* fc;

    if (ch == ' ') {
        return 3;
    }
    if (ch >= 'A' && ch <= 'Z') {
        fc = &FONT_upper[ch - 'A'];
    } else if (ch >= 'a' && ch <= 'z') {
        fc = &FONT_lower[ch - 'a'];
    } else if (ch >= '0' && ch <= '9') {
        fc = &FONT_number[ch - '0'];
    } else {
        switch (ch) {
        case '.':   fc = &FONT_punct[FONT_PUNCT_DOT];    break;
        case ',':   fc = &FONT_punct[FONT_PUNCT_COMMA];  break;
        case '?':   fc = &FONT_punct[FONT_PUNCT_QMARK];  break;
        case '!':   fc = &FONT_punct[FONT_PUNCT_PLING];  break;
        case '"':   fc = &FONT_punct[FONT_PUNCT_QUOTES]; break;
        case '(':   fc = &FONT_punct[FONT_PUNCT_OPEN];   break;
        case ')':   fc = &FONT_punct[FONT_PUNCT_CLOSE];  break;
        case '+':   fc = &FONT_punct[FONT_PUNCT_PLUS];   break;
        case '-':   fc = &FONT_punct[FONT_PUNCT_MINUS];  break;
        case '=':   fc = &FONT_punct[FONT_PUNCT_EQUAL];  break;
        case '#':   fc = &FONT_punct[FONT_PUNCT_HASH];   break;
        case '%':   fc = &FONT_punct[FONT_PUNCT_PCENT];  break;
        case '*':   fc = &FONT_punct[FONT_PUNCT_STAR];   break;
        case '\\':  fc = &FONT_punct[FONT_PUNCT_BSLASH]; break;
        case '/':   fc = &FONT_punct[FONT_PUNCT_FSLASH]; break;
        case ':':   fc = &FONT_punct[FONT_PUNCT_COLON];  break;
        case ';':   fc = &FONT_punct[FONT_PUNCT_SCOLON]; break;
        case '\'':  fc = &FONT_punct[FONT_PUNCT_APOST];  break;
        case '&':   fc = &FONT_punct[FONT_PUNCT_AMPER];  break;
        case '\xa3': fc = &FONT_punct[FONT_PUNCT_POUND]; break;
        case '$':   fc = &FONT_punct[FONT_PUNCT_DOLLAR]; break;
        case '<':   fc = &FONT_punct[FONT_PUNCT_LT];     break;
        case '>':   fc = &FONT_punct[FONT_PUNCT_GT];     break;
        case '@':   fc = &FONT_punct[FONT_PUNCT_AT];     break;
        case '_':   fc = &FONT_punct[FONT_PUNCT_UNDER];  break;
        default:    fc = &FONT_punct[FONT_PUNCT_QMARK];  break;
        }
    }

    if (sy < -FONT_HEIGHT || sy >= ge_get_screen_height() || sx < -FONT_WIDTH || sx >= ge_get_screen_width()) {
        // Off-screen — skip drawing but still return width.
    } else {
        for (y = 0; y < FONT_HEIGHT; y++) {
            for (b = 0x10, x = 0; x < 5; x++, b >>= 1) {
                if (fc->bit[y] & b) {
                    ge_plot_pixel(sx + x, sy + y, red, green, blue);
                }
            }
        }
    }

    return fc->width;
}

// uc_orig: FONT_draw_coloured_text (fallen/DDEngine/Source/Font.cpp)
SLONG FONT_draw_coloured_text(
    SLONG x,
    SLONG y,
    UBYTE red,
    UBYTE green,
    UBYTE blue,
    CBYTE* fmt, ...)
{
    CBYTE message[FONT_MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    CBYTE* ch;
    SLONG xstart = x;

    for (ch = message; *ch; ch++) {
        if (*ch == '\t') {
            x += FONT_TAB;
            x &= ~(FONT_TAB - 1);
        } else {
            x += FONT_draw_coloured_char(x, y, red, green, blue, *ch) + 1;
        }
    }

    return x - xstart;
}

// uc_orig: FONT_draw (fallen/DDEngine/Source/Font.cpp)
SLONG FONT_draw(SLONG x, SLONG y, CBYTE* fmt, ...)
{
    CBYTE message[FONT_MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    CBYTE* ch;
    SLONG xstart = x;

    for (ch = message; *ch; ch++) {
        if (*ch == '\t') {
            x += FONT_TAB;
            x &= ~(FONT_TAB - 1);
        } else {
            // Yellow text with red shadow at +1,+1.
            FONT_draw_coloured_char(x + 1, y + 1, 255, 0, 0, *ch);
            x += FONT_draw_coloured_char(x + 0, y + 0, 255, 255, 0, *ch) + 1;
        }
    }

    return x - xstart;
}

// uc_orig: FONT_buffer_add (fallen/DDEngine/Source/Font.cpp)
void FONT_buffer_add(
    SLONG x,
    SLONG y,
    UBYTE r,
    UBYTE g,
    UBYTE b,
    UBYTE s,
    CBYTE* fmt, ...)
{
    FONT_Message* fm;

    CBYTE message[FONT_MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    // Lazy initialisation — avoids needing an explicit init call.
    if (FONT_buffer_upto == NULL) {
        FONT_buffer_upto = &FONT_buffer[0];
    }

    if (!WITHIN(FONT_message_upto, 0, FONT_MAX_MESSAGES - 1)) {
        return;
    }

    if (FONT_buffer_upto + strlen(message) + 1 > &FONT_buffer[FONT_BUFFER_SIZE]) {
        return;
    }

    strcpy(FONT_buffer_upto, message);

    fm = &FONT_message[FONT_message_upto++];
    fm->x = x;
    fm->y = y;
    fm->r = r;
    fm->g = g;
    fm->b = b;
    fm->s = s;
    fm->m = FONT_buffer_upto;

    FONT_buffer_upto += strlen(message) + 1;
}

// uc_orig: FONT_buffer_draw (fallen/DDEngine/Source/Font.cpp)
void FONT_buffer_draw()
{
    SLONG i;
    SLONG x;
    SLONG y;
    CBYTE* ch;
    FONT_Message* fm;

    if (FONT_message_upto == 0) {
        return;
    }

    if (ge_lock_screen()) {
        for (i = 0; i < FONT_message_upto; i++) {
            fm = &FONT_message[i];
            x = fm->x;
            y = fm->y;

            for (ch = fm->m; *ch; ch++) {
                if (*ch == '\t') {
                    x += FONT_TAB;
                    x &= ~(FONT_TAB - 1);
                } else if (*ch == '\n') {
                    x = fm->x;
                    y += 10;
                } else {
                    if (fm->s) {
                        // Shadowed: draw darker copy at +1,+1 first.
                        FONT_draw_coloured_char(x + 1, y + 1, fm->r >> 1, fm->g >> 1, fm->b >> 1, *ch);
                        x += FONT_draw_coloured_char(x + 0, y + 0, fm->r, fm->g, fm->b, *ch) + 1;
                    } else {
                        x += FONT_draw_coloured_char(x + 0, y + 0, fm->r, fm->g, fm->b, *ch) + 1;
                    }
                }
            }
        }

        ge_unlock_screen();
    }

    FONT_buffer_upto = &FONT_buffer[0];
    FONT_message_upto = 0;
}

// uc_orig: FONT_draw_speech_bubble_text (fallen/DDEngine/Source/Font.cpp)
void FONT_draw_speech_bubble_text(
    SLONG x,
    SLONG y,
    UBYTE red,
    UBYTE green,
    UBYTE blue,
    CBYTE* fmt, ...)
{
    CBYTE message[FONT_MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    // Measure total pixel length to calculate a roughly-square wrapping width.
    SLONG length = FONT_draw_coloured_text(-100, -100, 0, 0, 0, message);
    SLONG width = SLONG(sqrt(float(length) * 13.0F));

    // First pass: count lines at the chosen width.
    CBYTE* ch;
    SLONG cw;
    SLONG w = 0;
    SLONG lines = 0;

    for (ch = message; *ch; ch++) {
        w += FONT_draw_coloured_char(-100, -100, 0, 0, 0, *ch) + 1;
        if (w > width) {
            w = 0;
            lines += 1;
        }
    }

    // Start position — text grows upward from (x,y).
    SLONG xstart = x;
    SLONG ystart = y - lines * 10;

    // Second pass: actually draw.
    x = xstart;
    y = ystart;

    for (ch = message; *ch; ch++) {
        cw = FONT_draw_coloured_char(x, y, red, green, blue, *ch) + 1;
        w += cw;
        x += cw;
        if (w > width) {
            w = 0;
            x = xstart;
            y += 10;
        }
    }
}
