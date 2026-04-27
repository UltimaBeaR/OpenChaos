#include "engine/graphics/text/font.h"
#include "engine/graphics/text/font_atlas.h"
#include "engine/graphics/text/font_globals.h"

#include "engine/graphics/graphics_engine/game_graphics_engine.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "engine/graphics/pipeline/polypage.h"   // PolyPage::s_XScale / s_YScale (virtual → pixel scale)

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
        FONT_atlas_begin_batch();
        FONT_atlas_draw_glyph(sx, sy, red, green, blue, ch);
        FONT_atlas_end_batch();
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

    FONT_atlas_begin_batch();
    for (ch = message; *ch; ch++) {
        if (*ch == '\t') {
            x += FONT_TAB;
            x &= ~(FONT_TAB - 1);
        } else {
            x += FONT_draw_coloured_char(x, y, red, green, blue, *ch) + 1;
        }
    }
    FONT_atlas_end_batch();

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

    FONT_atlas_begin_batch();
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
    FONT_atlas_end_batch();

    return x - xstart;
}

// Shared queue helper — both FONT_buffer_add variants funnel through here.
// `scale_x` / `scale_y` are stored on the message; FONT_buffer_draw
// resolves them as `(x * scale_x, y * scale_y)` at flush time. Pass
// (0, 0) for the literal-pixel path. See FONT_Message in font_globals.h.
static void font_buffer_add_impl(
    SLONG x, SLONG y,
    UBYTE r, UBYTE g, UBYTE b, UBYTE s,
    float scale_x, float scale_y,
    const CBYTE* message)
{
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

    FONT_Message* fm = &FONT_message[FONT_message_upto++];
    fm->x = x;
    fm->y = y;
    fm->r = r;
    fm->g = g;
    fm->b = b;
    fm->s = s;
    fm->scale_x = scale_x;
    fm->scale_y = scale_y;
    fm->m = FONT_buffer_upto;

    FONT_buffer_upto += strlen(message) + 1;
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
    CBYTE message[FONT_MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    // scale 0/0 → literal-pixel path; FONT_buffer_draw will use (x, y) as-is.
    font_buffer_add_impl(x, y, r, g, b, s, 0.0f, 0.0f, message);
}

void FONT_buffer_add_virtual(
    SLONG x,
    SLONG y,
    UBYTE r,
    UBYTE g,
    UBYTE b,
    UBYTE s,
    CBYTE* fmt, ...)
{
    CBYTE message[FONT_MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    // Snapshot the live PolyPage scale at SUBMIT time — by the time
    // FONT_buffer_draw flushes, any FullscreenUIModeScope / push_ui_mode
    // the caller may have used will already have been popped, so reading
    // PolyPage::s_X/YScale at flush would pick up the wrong scope.
    font_buffer_add_impl(x, y, r, g, b, s,
                         PolyPage::s_XScale, PolyPage::s_YScale,
                         message);
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

    // FONT_draw_coloured_char now emits textured quads through the regular TL
    // pipeline — no lock/unlock of the backbuffer. The per-frame readback
    // pattern the legacy path relied on is exactly what triggers the WDDM
    // throttle investigated in new_game_devlog/startup_hang_investigation.
    FONT_atlas_begin_batch();
    for (i = 0; i < FONT_message_upto; i++) {
        fm = &FONT_message[i];

        // Resolve virtual-coords messages to live FB pixels using the scale
        // captured at SUBMIT time (see FONT_Message in font_globals.h). A
        // zero scale means the caller passed literal FB pixels via
        // FONT_buffer_add — leave (x, y) as-is.
        SLONG line_start_x;
        if (fm->scale_x != 0.0f) {
            line_start_x = SLONG(float(fm->x) * fm->scale_x);
            y = SLONG(float(fm->y) * fm->scale_y);
        } else {
            line_start_x = fm->x;
            y = fm->y;
        }
        x = line_start_x;

        for (ch = fm->m; *ch; ch++) {
            if (*ch == '\t') {
                x += FONT_TAB;
                x &= ~(FONT_TAB - 1);
            } else if (*ch == '\n') {
                // Reset to the resolved line-start x — for virtual_coords
                // messages this is the SCALED x. Earlier code used fm->x
                // directly here, which on virtual messages was the raw
                // unscaled value, so multi-line AENG_world_text labels had
                // line 1 at the right place but lines 2+ snapped to the
                // upper-left of the framebuffer.
                x = line_start_x;
                // Line height stays unscaled — glyphs render at fixed
                // pixel size (5×9), so a non-uniform percentage scope
                // mustn't stretch the gap between lines independently of
                // the glyph height.
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
    FONT_atlas_end_batch();

    FONT_buffer_upto = &FONT_buffer[0];
    FONT_message_upto = 0;
}

