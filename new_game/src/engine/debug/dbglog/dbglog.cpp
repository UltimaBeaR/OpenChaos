#include "engine/debug/dbglog/dbglog.h"

#if OC_DEBUG_LOG

#include "engine/platform/uc_common.h" // SLONG / ULONG / CBYTE
#include "engine/graphics/text/font.h" // FONT_buffer_add (literal pixels)
#include "engine/graphics/pipeline/aeng.h" // AENG_draw_rect
#include "engine/graphics/pipeline/poly.h" // POLY_PAGE_*, POLY_screen_*
#include "ui/hud/panel.h" // PANEL_start / PANEL_finish (own batch)
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // ge_get_screen_*
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks (message timestamps)
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

// Ring buffer of the most recent log lines. Must exceed the largest
// MAX (screen_h / line height) we'd ever hit, otherwise the buffer — not
// the screen — caps the visible count and the bottom of a tall window
// stays empty while the log scrolls. 256 covers ~3300 px of height at
// the 13 px line step (well past 4K). Fine as static.
#define DBGLOG_LINES 256

// A line is up to DBGLOG_MAX_SEGS independently-coloured pieces. Each
// piece is laid out in its own fixed-width column so columns align
// across lines. Single-colour DBGLOG() lines are just one segment.
#define DBGLOG_MAX_SEGS 6
#define DBGLOG_SEGLEN 40

struct DbgSeg {
    char txt[DBGLOG_SEGLEN];
    ULONG rgb;
};

static DbgSeg s_seg[DBGLOG_LINES][DBGLOG_MAX_SEGS];
static UBYTE s_nseg[DBGLOG_LINES];
static uint64_t s_ms[DBGLOG_LINES]; // sdl3_get_ticks() at write time
static SLONG s_count = 0; // valid entries (0..DBGLOG_LINES)
static SLONG s_pos = 0; // next write slot

// Line currently being assembled by DBGLOG_begin/seg/commit.
static DbgSeg s_build[DBGLOG_MAX_SEGS];
static SLONG s_build_n = 0;

// First-ever log time — message timestamps are offsets from this, so the
// clock reads ~from game start. Deliberately NOT reset by DBGLOG_clear
// (per-level clears keep the same game-start origin).
static uint64_t s_base_ms = 0;
static bool s_base_set = false;

// Panel geometry in FIXED FRAMEBUFFER PIXELS (ge_get_screen_* space — the
// same space FONT_buffer_add literal coords use). Nothing here scales with
// resolution: the panel is always the same pixel width, the line gap is
// always the same pixel height. A bigger window just fits more lines.
#define DBGLOG_PANEL_W_PX 630 // wide panel (1.5x of the previous 420)
#define DBGLOG_PAD_PX 6 // left text inset inside the panel
#define DBGLOG_TOP_PX 8 // top/bottom margin
#define DBGLOG_LINE_PX 13 // fixed gap between lines (px)
#define DBGLOG_TS_W_PX 64 // fixed pixel width of the "mm:ss:mmm " stamp

unsigned long DBGLOG_color(const char* name)
{
    if (!name) return 0xffffff;
    if (strcmp(name, "red") == 0) return 0xff3030;
    if (strcmp(name, "green") == 0) return 0x30ff30;
    if (strcmp(name, "yellow") == 0) return 0xffe030;
    if (strcmp(name, "grey") == 0 || strcmp(name, "gray") == 0) return 0xa0a0a0;
    if (strcmp(name, "cyan") == 0) return 0x30ffff;
    if (strcmp(name, "orange") == 0) return 0xff9030;
    return 0xffffff; // "white" / unknown
}

void DBGLOG_begin(void)
{
    s_build_n = 0;
}

void DBGLOG_seg(unsigned long rgb, const char* fmt, ...)
{
    if (s_build_n >= DBGLOG_MAX_SEGS)
        return;

    char* slot = s_build[s_build_n].txt;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(slot, DBGLOG_SEGLEN, fmt, ap);
    va_end(ap);

    // Uppercase for a consistent debug-readout look (matches CONSOLE).
    for (char* c = slot; *c; ++c)
        *c = (char)toupper((unsigned char)*c);

    s_build[s_build_n].rgb = (ULONG)rgb;
    s_build_n++;
}

void DBGLOG_commit(void)
{
    if (s_build_n <= 0)
        return;

    const uint64_t now = sdl3_get_ticks();
    if (!s_base_set) {
        s_base_ms = now;
        s_base_set = true;
    }

    for (SLONG i = 0; i < s_build_n; i++)
        s_seg[s_pos][i] = s_build[i];
    s_nseg[s_pos] = (UBYTE)s_build_n;
    s_ms[s_pos] = now;

    s_pos = (s_pos + 1) % DBGLOG_LINES;
    if (s_count < DBGLOG_LINES)
        s_count++;
    s_build_n = 0;
}

void DBGLOG(const char* color, const char* fmt, ...)
{
    // One-segment line built on the segment path.
    char buf[DBGLOG_SEGLEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    DBGLOG_begin();
    DBGLOG_seg(DBGLOG_color(color), "%s", buf);
    DBGLOG_commit();
}

void DBGLOG_clear(void)
{
    s_count = 0;
    s_pos = 0;
    s_build_n = 0;
}

void DBGLOG_draw(void)
{
    // Called from the top UI layer (ui_render, after input_debug_render)
    // so it sits above the HUD / enemy HP bars. Not inside an existing
    // batch there, so we open our own PANEL_start/PANEL_finish around the
    // backdrop rect (same as input_debug_render). Gated purely by the
    // OC_DEBUG_LOG compile flag (no bangunsnotgames).
    //
    // Everything is computed in FIXED framebuffer pixels (ge_get_screen_*
    // space). Text uses FONT_buffer_add (literal, scale 0 -> as-is) so the
    // 5x9 glyphs and the line gap never scale with resolution. The
    // backdrop is AENG_draw_rect which works in logical 640x480 and the
    // pipeline scales it by screen/640 — so to get a FIXED pixel rect we
    // pass logical = px * POLY_screen_width / screen_px (the inverse of
    // that scale). FONT_buffer_add is flushed after all poly pages, so
    // text always lands on top of the alpha backdrop.

    const SLONG screen_w = (SLONG)ge_get_screen_width();
    const SLONG screen_h = (SLONG)ge_get_screen_height();
    if (screen_w <= 0 || screen_h <= 0)
        return;

    // Right-aligned panel, fixed pixel width, full screen height.
    const SLONG panel_x_px = screen_w - DBGLOG_PANEL_W_PX;
    const SLONG text_x_px = panel_x_px + DBGLOG_PAD_PX;

    // px -> logical (inverse of the pipeline's logical->px scale) so the
    // rect ends up exactly DBGLOG_PANEL_W_PX pixels wide on any window.
    const float lx = (float)POLY_screen_width / (float)screen_w;

    // Vertically: the text uses the literal-pixel path (GEVertexTL,
    // normalised by the UI dst-rect viewport) while AENG_draw_rect goes
    // through PolyPage scaling + letterbox offsets — during in-game
    // cutscenes those two vertical mappings diverge, so a logical 0..480
    // rect no longer covers the full text column. Rather than reproduce
    // the pipeline's offset math, overshoot far past the top and bottom
    // in logical space; the viewport clips it, so the panel is a full
    // column in every mode (normal play and cutscene).
    const SLONG over = (SLONG)POLY_screen_height;
    PANEL_start();
    AENG_draw_rect(
        (SLONG)(panel_x_px * lx), -over,
        (SLONG)(DBGLOG_PANEL_W_PX * lx), (SLONG)POLY_screen_height + 2 * over,
        (SLONG)0xE6000000u, 8, POLY_PAGE_COLOUR_ALPHA); // ~90% black
    PANEL_finish();

    // How many lines fit on this screen (more on bigger windows).
    SLONG max_lines = (screen_h - 2 * DBGLOG_TOP_PX) / DBGLOG_LINE_PX;
    if (max_lines < 1)
        max_lines = 1;

    // Bottom-anchored: the NEWEST message always sits on the bottom-most
    // line and history grows upward. Before the screen fills, the empty
    // space is at the TOP (not the bottom) — so the latest line is always
    // at the same place (the bottom), consistent whether full or not.
    SLONG show = s_count < max_lines ? s_count : max_lines;
    const SLONG newest = (s_pos - 1 + DBGLOG_LINES) % DBGLOG_LINES;
    const SLONG bottom_y = DBGLOG_TOP_PX + (max_lines - 1) * DBGLOG_LINE_PX;

    for (SLONG j = 0; j < show; j++) { // j = 0 -> newest (bottom)
        const SLONG idx = (newest - j + DBGLOG_LINES) % DBGLOG_LINES;
        const SLONG y = bottom_y - j * DBGLOG_LINE_PX;

        const uint64_t t = s_ms[idx] - s_base_ms;
        const unsigned long long mm = (unsigned long long)(t / 60000ull);
        const unsigned long long ss = (unsigned long long)((t / 1000ull) % 60ull);
        const unsigned long long ms = (unsigned long long)(t % 1000ull);

        // Grey timestamp prefix (computed here, never part of the message).
        FONT_buffer_add(text_x_px, y, 150, 150, 150, 1,
            (CBYTE*)"%02llu:%02llu:%03llu", mm, ss, ms);

        // Segments flow consecutively on one line; each starts exactly
        // where the previous ended (its measured pixel width). The log
        // adds NO implicit spacing — callers do column alignment with
        // spaces/tabs in the segment text (lx is the line-local cursor
        // so '\t' tab stops snap on a consistent grid).
        const SLONG seg_x0 = text_x_px + DBGLOG_TS_W_PX;
        SLONG lx = 0;
        for (SLONG k = 0; k < (SLONG)s_nseg[idx]; k++) {
            const ULONG rgb = s_seg[idx][k].rgb;
            char* txt = s_seg[idx][k].txt;
            FONT_buffer_add(seg_x0 + lx, y,
                (UBYTE)((rgb >> 16) & 0xff), (UBYTE)((rgb >> 8) & 0xff),
                (UBYTE)(rgb & 0xff), 1, (CBYTE*)"%s", txt);
            lx += FONT_string_width(lx, (CBYTE*)txt);
        }
    }
}

#endif // OC_DEBUG_LOG
