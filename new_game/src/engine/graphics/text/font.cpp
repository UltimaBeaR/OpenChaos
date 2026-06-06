#include "engine/graphics/text/font.h"
#include "engine/graphics/text/font_atlas.h"
#include "engine/graphics/text/font_globals.h"

#include "engine/graphics/graphics_engine/game_graphics_engine.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "engine/graphics/pipeline/polypage.h" // PolyPage::s_XScale / s_YScale (virtual → pixel scale)

#include "engine/core/macros.h"
#include "debug_config.h" // OC_DEBUG_PERF — gates the perf-panel text queue
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
    CBYTE ch,
    SLONG scale)
{
    FONT_Char* fc;

    if (scale < 1)
        scale = 1;

    if (ch == ' ') {
        return 3 * scale;
    }
    if (ch >= 'A' && ch <= 'Z') {
        fc = &FONT_upper[ch - 'A'];
    } else if (ch >= 'a' && ch <= 'z') {
        fc = &FONT_lower[ch - 'a'];
    } else if (ch >= '0' && ch <= '9') {
        fc = &FONT_number[ch - '0'];
    } else {
        switch (ch) {
        case '.':
            fc = &FONT_punct[FONT_PUNCT_DOT];
            break;
        case ',':
            fc = &FONT_punct[FONT_PUNCT_COMMA];
            break;
        case '?':
            fc = &FONT_punct[FONT_PUNCT_QMARK];
            break;
        case '!':
            fc = &FONT_punct[FONT_PUNCT_PLING];
            break;
        case '"':
            fc = &FONT_punct[FONT_PUNCT_QUOTES];
            break;
        case '(':
            fc = &FONT_punct[FONT_PUNCT_OPEN];
            break;
        case ')':
            fc = &FONT_punct[FONT_PUNCT_CLOSE];
            break;
        case '+':
            fc = &FONT_punct[FONT_PUNCT_PLUS];
            break;
        case '-':
            fc = &FONT_punct[FONT_PUNCT_MINUS];
            break;
        case '=':
            fc = &FONT_punct[FONT_PUNCT_EQUAL];
            break;
        case '#':
            fc = &FONT_punct[FONT_PUNCT_HASH];
            break;
        case '%':
            fc = &FONT_punct[FONT_PUNCT_PCENT];
            break;
        case '*':
            fc = &FONT_punct[FONT_PUNCT_STAR];
            break;
        case '\\':
            fc = &FONT_punct[FONT_PUNCT_BSLASH];
            break;
        case '/':
            fc = &FONT_punct[FONT_PUNCT_FSLASH];
            break;
        case ':':
            fc = &FONT_punct[FONT_PUNCT_COLON];
            break;
        case ';':
            fc = &FONT_punct[FONT_PUNCT_SCOLON];
            break;
        case '\'':
            fc = &FONT_punct[FONT_PUNCT_APOST];
            break;
        case '&':
            fc = &FONT_punct[FONT_PUNCT_AMPER];
            break;
        case '\xa3':
            fc = &FONT_punct[FONT_PUNCT_POUND];
            break;
        case '$':
            fc = &FONT_punct[FONT_PUNCT_DOLLAR];
            break;
        case '<':
            fc = &FONT_punct[FONT_PUNCT_LT];
            break;
        case '>':
            fc = &FONT_punct[FONT_PUNCT_GT];
            break;
        case '@':
            fc = &FONT_punct[FONT_PUNCT_AT];
            break;
        case '_':
            fc = &FONT_punct[FONT_PUNCT_UNDER];
            break;
        default:
            fc = &FONT_punct[FONT_PUNCT_QMARK];
            break;
        }
    }

    if (sy < -FONT_HEIGHT * scale || sy >= ge_get_screen_height() || sx < -FONT_WIDTH * scale || sx >= ge_get_screen_width()) {
        // Off-screen — skip drawing but still return width.
    } else {
        FONT_atlas_begin_batch();
        FONT_atlas_draw_glyph(sx, sy, red, green, blue, ch, scale);
        FONT_atlas_end_batch();
    }

    return fc->width * scale;
}

// OpenChaos addition: measure-only counterpart of FONT_draw_coloured_text.
// Uses the same advance rule (glyph width + 1, '\t' -> tab grid). Width
// comes from FONT_draw_coloured_char called far off-screen: it skips the
// actual draw (and the atlas batch) for off-screen glyphs yet still
// returns the glyph width, so there are no side effects.
SLONG FONT_string_width(SLONG x, CBYTE* str, SLONG scale)
{
    if (scale < 1)
        scale = 1;
    SLONG xstart = x;
    for (CBYTE* ch = str; *ch; ch++) {
        if (*ch == '\t') {
            if (scale == 1) {
                x += FONT_TAB;
                x &= ~(FONT_TAB - 1);
            } else {
                // Scaled tab grid (the &~ snap only works for the
                // pow-of-two FONT_TAB); must match the flush loop.
                const SLONG tab = FONT_TAB * scale;
                x += tab;
                x -= x % tab;
            }
        } else {
            x += FONT_draw_coloured_char(-100000, -100000, 0, 0, 0, *ch, scale) + scale;
        }
    }
    return x - xstart;
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

// --- Deferred-text queues -------------------------------------------------
//
// Queue 0 (MAIN) reuses the original globals (FONT_buffer / FONT_message
// / *_upto) so its behaviour is byte-identical to before this split.
// Queue 1 (PERF) exists ONLY when the perf-diag panel is compiled in
// (OC_DEBUG_PERF) — used solely by that panel so its glyph rasterisation
// is timed separately. When OC_DEBUG_PERF is off the PERF queue and its
// backing storage are NOT compiled in at all (no static RAM spent on a
// feature that can't run): only the MAIN queue exists, select() is a
// no-op, and FONT_buffer_draw_perf() does nothing. See font.h.
namespace {

struct FontQueue {
    CBYTE* buf_base; // backing string storage
    SLONG buf_size;
    CBYTE** buf_upto; // -> this queue's write cursor variable
    FONT_Message* msgs; // message records
    SLONG max_msgs;
    SLONG* msg_upto; // -> this queue's message-count variable
};

#if OC_DEBUG_PERF

// Perf-panel queue storage. Sized for the panel's worst case (~stage
// rows × up to 4 cells + header/legend/counters) with headroom — much
// smaller than the shared main queue (the panel is a few dozen short
// lines, nothing like a full dbglog). Static RAM only, and only when
// the panel is compiled in.
constexpr SLONG PERF_FONT_BUFFER_SIZE = 1024 * 8;
constexpr SLONG PERF_FONT_MAX_MESSAGES = 384;
CBYTE s_perf_font_buffer[PERF_FONT_BUFFER_SIZE];
CBYTE* s_perf_font_buffer_upto = nullptr;
FONT_Message s_perf_font_message[PERF_FONT_MAX_MESSAGES];
SLONG s_perf_font_message_upto = 0;

constexpr int FONT_QUEUE_N = 2;

#else

constexpr int FONT_QUEUE_N = 1; // PERF queue not built — MAIN only

#endif // OC_DEBUG_PERF

FontQueue s_queues[FONT_QUEUE_N] = {
    { &FONT_buffer[0], FONT_BUFFER_SIZE, &FONT_buffer_upto,
        &FONT_message[0], FONT_MAX_MESSAGES, &FONT_message_upto },
#if OC_DEBUG_PERF
    { &s_perf_font_buffer[0], PERF_FONT_BUFFER_SIZE, &s_perf_font_buffer_upto,
        &s_perf_font_message[0], PERF_FONT_MAX_MESSAGES,
        &s_perf_font_message_upto },
#endif
};

int s_active_queue = FONT_QUEUE_MAIN;

} // namespace

void FONT_buffer_select(int queue)
{
    // Only the PERF queue is ever selected by callers (the panel); when
    // it isn't compiled in, stay on MAIN. Bounds-checked against the
    // queues that actually exist.
    if (queue >= 0 && queue < FONT_QUEUE_N)
        s_active_queue = queue;
}

// Shared queue helper — both FONT_buffer_add variants funnel through here.
// `scale_x` / `scale_y` are stored on the message; FONT_buffer_draw
// resolves them as `(x * scale_x, y * scale_y)` at flush time. Pass
// (0, 0) for the literal-pixel path. See FONT_Message in font_globals.h.
// Routes to whichever queue FONT_buffer_select last picked.
static void font_buffer_add_impl(
    SLONG x, SLONG y,
    UBYTE r, UBYTE g, UBYTE b, UBYTE s,
    UBYTE gs,
    float scale_x, float scale_y,
    const CBYTE* message)
{
    FontQueue& q = s_queues[s_active_queue];

    // Lazy initialisation — avoids needing an explicit init call.
    if (*q.buf_upto == NULL) {
        *q.buf_upto = q.buf_base;
    }

    if (!WITHIN(*q.msg_upto, 0, q.max_msgs - 1)) {
        return;
    }

    if (*q.buf_upto + strlen(message) + 1 > q.buf_base + q.buf_size) {
        return;
    }

    strcpy(*q.buf_upto, message);

    FONT_Message* fm = &q.msgs[(*q.msg_upto)++];
    fm->x = x;
    fm->y = y;
    fm->r = r;
    fm->g = g;
    fm->b = b;
    fm->s = s;
    fm->gs = (gs < 1) ? 1 : gs;
    fm->scale_x = scale_x;
    fm->scale_y = scale_y;
    fm->m = *q.buf_upto;

    *q.buf_upto += strlen(message) + 1;
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
    font_buffer_add_impl(x, y, r, g, b, s, 1, 0.0f, 0.0f, message);
}

void FONT_buffer_add_scaled(
    SLONG x,
    SLONG y,
    UBYTE r,
    UBYTE g,
    UBYTE b,
    UBYTE s,
    UBYTE glyph_scale,
    CBYTE* fmt, ...)
{
    CBYTE message[FONT_MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    // Literal-pixel path with a glyph-size multiplier.
    font_buffer_add_impl(x, y, r, g, b, s, glyph_scale, 0.0f, 0.0f, message);
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
    font_buffer_add_impl(x, y, r, g, b, s, 1,
        PolyPage::s_XScale, PolyPage::s_YScale,
        message);
}

// Render every queued message in `q`, then clear the queue. Shared by
// the MAIN (FONT_buffer_draw) and PERF (FONT_buffer_draw_perf) flushes.
static void font_flush_queue(FontQueue& q)
{
    SLONG i;
    SLONG x;
    SLONG y;
    CBYTE* ch;
    FONT_Message* fm;

    const SLONG count = *q.msg_upto;
    if (count == 0) {
        return;
    }

    // FONT_draw_coloured_char now emits textured quads through the regular TL
    // pipeline — no lock/unlock of the backbuffer. The per-frame readback
    // pattern the legacy path relied on is exactly what triggers the WDDM
    // throttle investigated in new_game_devlog/startup_hang_investigation.
    FONT_atlas_begin_batch();
    for (i = 0; i < count; i++) {
        fm = &q.msgs[i];

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

        // Integer glyph-size multiplier for this message (1 = original).
        const SLONG gs = (fm->gs < 1) ? 1 : fm->gs;

        for (ch = fm->m; *ch; ch++) {
            if (*ch == '\t') {
                if (gs == 1) {
                    x += FONT_TAB;
                    x &= ~(FONT_TAB - 1);
                } else {
                    const SLONG tab = FONT_TAB * gs;
                    x += tab;
                    x -= x % tab;
                }
            } else if (*ch == '\n') {
                // Reset to the resolved line-start x — for virtual_coords
                // messages this is the SCALED x. Earlier code used fm->x
                // directly here, which on virtual messages was the raw
                // unscaled value, so multi-line AENG_world_text labels had
                // line 1 at the right place but lines 2+ snapped to the
                // upper-left of the framebuffer.
                x = line_start_x;
                // Line height: base 10 px (one more than the 9 px glyph)
                // multiplied by the glyph scale so multi-line scaled text
                // doesn't overlap. gs==1 → 10, identical to before.
                y += 10 * gs;
            } else {
                if (fm->s) {
                    // Shadowed: draw darker copy offset by the scale so the
                    // shadow tracks the bigger glyph (gs==1 → +1,+1).
                    FONT_draw_coloured_char(x + gs, y + gs, fm->r >> 1, fm->g >> 1, fm->b >> 1, *ch, gs);
                    x += FONT_draw_coloured_char(x + 0, y + 0, fm->r, fm->g, fm->b, *ch, gs) + gs;
                } else {
                    x += FONT_draw_coloured_char(x + 0, y + 0, fm->r, fm->g, fm->b, *ch, gs) + gs;
                }
            }
        }
    }
    FONT_atlas_end_batch();

    *q.buf_upto = q.buf_base;
    *q.msg_upto = 0;
}

// uc_orig: FONT_buffer_draw (fallen/DDEngine/Source/Font.cpp)
void FONT_buffer_draw()
{
    font_flush_queue(s_queues[FONT_QUEUE_MAIN]);
}

void FONT_buffer_draw_perf()
{
#if OC_DEBUG_PERF
    font_flush_queue(s_queues[FONT_QUEUE_PERF]);
#endif
    // Else: the PERF queue does not exist — nothing queued, nothing to do.
}
