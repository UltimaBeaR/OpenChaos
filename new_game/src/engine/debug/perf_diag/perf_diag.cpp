#include "engine/debug/perf_diag/perf_diag.h"

#if OC_PERF_ACTIVE

#include "engine/platform/uc_common.h"            // SLONG / ULONG / UBYTE / CBYTE
#include "engine/platform/sdl3_bridge.h"          // perf counter / get_ticks
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#if OC_DEBUG_PERF
#include "engine/graphics/text/font.h"            // FONT_buffer_add_scaled
#include "engine/graphics/text/font_atlas.h"      // FONT_atlas_fill_* (literal-px rects)
#include "engine/debug/debug_panel_text_scale.h"  // DBG_PANEL_TEXT_SCALE
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // ge_get_screen_*
#include "engine/input/input_frame.h"             // input_key_just_pressed
#include "engine/input/keyboard.h"                // KB_4 / KB_5 / KB_6
#endif

namespace perf {

// ---- Tunables -----------------------------------------------------------

// Max distinct instrumented metrics. Static table — generous headroom for
// deep instrumentation without re-tuning. Overflow just drops new slots.
static constexpr int MAX_SLOTS = 128;

// Per-slot history ring (one entry = one render frame's accumulated
// value). Big enough to hold the longest averaging window plus headroom.
static constexpr int RING = 256;

// Long/baseline window (frames) the short average is compared against to
// detect a sharp change. ~Several seconds at typical FPS — stable enough
// to be a "what's normal here" reference without manual calibration.
static constexpr int LONG_W = 240;

// Short averaging window options (frames) cycled by KB_5. Short = reacts
// fast / jitters; long = smooth / inert.
static const int SHORT_W_OPTS[] = { 10, 30, 60, 120 };
static constexpr int SHORT_W_OPT_COUNT =
    (int)(sizeof(SHORT_W_OPTS) / sizeof(SHORT_W_OPTS[0]));

// File-log averaging window (frames per CSV row). Default; when
// OC_DEBUG_PERF is also on, KB_5 sets this to the chosen short window.
static int s_log_avg_n = 60;

// ---- Slot ---------------------------------------------------------------

struct Slot {
    char     name[64];
    SlotKind kind;
    double   accum;             // current-frame accumulator
    double   ring[RING];
    int      head;              // next write
    int      count;             // valid entries (≤ RING)
    double   short_avg;
    double   long_avg;
};

static Slot s_slot[MAX_SLOTS];
static int  s_slot_count = 0;

static Slot* s_frame_total = nullptr; // builtin: whole-frame ms (→ FPS)

static uint64_t s_freq        = 0;    // perf counter frequency (Hz)
static uint64_t s_frame_t0    = 0;    // perf counter at frame_begin
static uint64_t s_frame_idx   = 0;
static bool     s_frame_began = false; // a frame_begin has run at least once

static void ensure_freq()
{
    if (s_freq == 0)
        s_freq = sdl3_get_performance_frequency();
}

#if OC_DEBUG_PERF
static bool s_panel_visible   = true;
static int  s_short_w_idx     = 1;    // -> SHORT_W_OPTS[1] = 30
#else
static int  s_short_w_idx     = 1;
#endif

static int short_w() { return SHORT_W_OPTS[s_short_w_idx]; }

// ---- Registry -----------------------------------------------------------

Slot* get_slot(const char* name, SlotKind kind)
{
    ensure_freq(); // probes can run before the first frame_begin (level load)
    for (int i = 0; i < s_slot_count; i++)
        if (strcmp(s_slot[i].name, name) == 0)
            return &s_slot[i];

    if (s_slot_count >= MAX_SLOTS)
        return nullptr; // table full — drop silently

    Slot* s = &s_slot[s_slot_count++];
    memset(s, 0, sizeof(*s));
    snprintf(s->name, sizeof(s->name), "%s", name);
    s->kind = kind;
    return s;
}

void add_value(Slot* s, double v)
{
    if (s) s->accum += v;
}

ScopeTimer::ScopeTimer(Slot* s)
    : slot(s), t0((ensure_freq(), sdl3_get_performance_counter()))
{}

ScopeTimer::~ScopeTimer()
{
    if (!slot || s_freq == 0)
        return;
    const uint64_t now = sdl3_get_performance_counter();
    slot->accum += (double)(now - t0) * 1000.0 / (double)s_freq;
}

// ---- Averaging ----------------------------------------------------------

static double avg_last(const Slot* s, int n)
{
    int m = s->count < n ? s->count : n;
    if (m <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < m; i++) {
        int idx = (s->head - 1 - i + RING) % RING;
        sum += s->ring[idx];
    }
    return sum / (double)m;
}

// ---- File log -----------------------------------------------------------

#if OC_DEBUG_PERF_LOG
static FILE* s_log     = nullptr;
static int   s_log_cols = 0;       // slot count covered by the last header

static void log_write_header()
{
    fprintf(s_log, "#frame,time_ms");
    for (int i = 0; i < s_slot_count; i++)
        fprintf(s_log, ",%s", s_slot[i].name);
    fprintf(s_log, "\n");
    s_log_cols = s_slot_count;
}

static void log_row()
{
    if (!s_log) {
        s_log = fopen("perf_log.csv", "w"); // next to the exe, like crash_log.txt
        if (!s_log) return;
    }
    // Re-emit the header whenever new slots appeared (lazy registration).
    if (s_slot_count != s_log_cols)
        log_write_header();

    fprintf(s_log, "%llu,%llu",
        (unsigned long long)s_frame_idx,
        (unsigned long long)sdl3_get_ticks());
    for (int i = 0; i < s_slot_count; i++)
        fprintf(s_log, ",%.4f", avg_last(&s_slot[i], s_log_avg_n));
    fprintf(s_log, "\n");
    fflush(s_log);
}
#endif // OC_DEBUG_PERF_LOG

// ---- Frame boundary -----------------------------------------------------

void frame_begin()
{
    ensure_freq();
    if (!s_frame_total)
        s_frame_total = get_slot("frame.total", KIND_TIME);
    s_frame_began = true;
    s_frame_t0 = sdl3_get_performance_counter();
}

void frame_end()
{
    if (!s_frame_began)
        return; // frame_begin never ran (partial frame) — skip

    const uint64_t now   = sdl3_get_performance_counter();
    const double   total = (double)(now - s_frame_t0) * 1000.0 / (double)s_freq;
    if (s_frame_total) s_frame_total->accum = total;

    for (int i = 0; i < s_slot_count; i++) {
        Slot* s = &s_slot[i];
        const double sample = s->accum;

        s->ring[s->head] = sample;
        s->head = (s->head + 1) % RING;
        if (s->count < RING) s->count++;

        s->short_avg = avg_last(s, short_w());
        s->long_avg  = avg_last(s, LONG_W);
        s->accum = 0.0;
    }

    s_frame_idx++;

#if OC_DEBUG_PERF_LOG
    if (s_log_avg_n > 0 && (s_frame_idx % (uint64_t)s_log_avg_n) == 0)
        log_row();
#endif
}

// ---- Hotkeys ------------------------------------------------------------

void handle_keys()
{
#if OC_DEBUG_PERF
    if (input_key_just_pressed(KB_4))
        s_panel_visible = !s_panel_visible;

    if (input_key_just_pressed(KB_5)) {
        s_short_w_idx = (s_short_w_idx + 1) % SHORT_W_OPT_COUNT;
        s_log_avg_n = short_w(); // file-log window follows the panel window
    }

    if (input_key_just_pressed(KB_6)) {
        for (int i = 0; i < s_slot_count; i++) {
            s_slot[i].count = 0;
            s_slot[i].head = 0;
            s_slot[i].short_avg = 0.0;
            s_slot[i].long_avg = 0.0;
        }
    }
#endif
}

// ---- Panel --------------------------------------------------------------

#if OC_DEBUG_PERF

// Chunky integer text size — shared knob in debug_config.h so this panel
// and dbglog never drift apart. All geometry below multiplies by it so
// the backdrop / columns / line-step track the glyph size.
static constexpr UBYTE PERF_TEXT_SCALE = DBG_PANEL_TEXT_SCALE;

// Panel geometry in FIXED framebuffer pixels (same space FONT_buffer_add
// literal coords use) — nothing scales with resolution; a bigger window
// just fits more lines. Left-aligned (dbglog owns the right side). The
// value column x and the panel width are NOT fixed — they are measured
// from the actual text each frame (longest name + a small gap), so the
// panel always hugs its content with no dead slack. Only padding / line
// step / glyph height are constants.
static constexpr SLONG PERF_PAD_PX     = 6;
static constexpr SLONG PERF_TOP_PX     = 8;
static constexpr SLONG PERF_LINE_PX    = 13 * PERF_TEXT_SCALE;
static constexpr SLONG PERF_GLYPH_H_PX = 9  * PERF_TEXT_SCALE;  // FONT_HEIGHT * scale
static constexpr SLONG PERF_COL_GAP_PX = 70 * PERF_TEXT_SCALE;  // name→value gap (tunable)

// Row-underline colours (AARRGGBB, alpha-blended). Normal rows get a
// clearly-visible grey; a "*.total" row gets a brighter near-white line
// so at a glance you know that single line summarises the whole group —
// only dive into the group's members when the total looks off.
static constexpr SLONG PERF_RULE_NORMAL = (SLONG)0x80909090u;
static constexpr SLONG PERF_RULE_TOTAL  = (SLONG)0xC8F0F0F0u;

// First-level group of a dotted metric name ("render.shadows" → "render").
// Writes into buf; returns it. No '.' → whole name is the group.
static const char* group_of(const char* name, char* buf, int bufsz)
{
    int i = 0;
    for (; name[i] && name[i] != '.' && i < bufsz - 1; i++)
        buf[i] = name[i];
    buf[i] = '\0';
    return buf;
}

static bool is_total(const char* name)
{
    const char* dot = strchr(name, '.');
    return dot && strcmp(dot + 1, "total") == 0;
}

// % of frame time → text colour. White-ish at ~0, red at 100%. Nonlinear
// (pow < 1) so even ~10% is already clearly tinted while 50% vs 80% still
// differ visibly. Replaces the old spike/peak-hold mechanism — the colour
// now simply encodes "how much of the frame this stage eats".
static void pct_color(double pct, UBYTE& r, UBYTE& g, UBYTE& b)
{
    double t = pct * 0.01;
    if (t < 0.0) t = 0.0; else if (t > 1.0) t = 1.0;
    t = pow(t, 0.45); // knee: low % already visible, high % compressed
    r = (UBYTE)(210 + t * (255 - 210));
    g = (UBYTE)(210 + t * (40  - 210));
    b = (UBYTE)(210 + t * (40  - 210));
}

// A "countable" leaf = a time metric that is part of the disjoint
// partition of the frame: has a group.sub name, isn't a "*.total"
// summary, and isn't an info sub-metric (sub starting with '_', e.g.
// render._perf_panel which is nested inside render.overlay and would
// double-count). Group totals are built from these so all group totals
// + "(other)" sum to exactly 100% of the frame.
static bool is_countable(const Slot* s)
{
    if (s->kind != KIND_TIME)
        return false;
    const char* d = strchr(s->name, '.');
    if (!d)
        return false;
    const char* sub = d + 1;
    return strcmp(sub, "total") != 0 && sub[0] != '_';
}

// Group's % of the frame: an explicit "<group>.total" probe if one was
// registered (e.g. physics.total — a measured block), otherwise the sum
// of the group's countable leaves (e.g. render = draw_screen + overlay +
// present).
static double group_total_pct(const char* grp, double frame_ms)
{
    if (frame_ms <= 1e-6)
        return 0.0;
    for (int i = 0; i < s_slot_count; i++) {
        char gb[16];
        if (strcmp(group_of(s_slot[i].name, gb, sizeof(gb)), grp) != 0)
            continue;
        if (s_slot[i].kind == KIND_TIME && is_total(s_slot[i].name))
            return s_slot[i].short_avg / frame_ms * 100.0;
    }
    double sum = 0.0;
    for (int i = 0; i < s_slot_count; i++) {
        char gb[16];
        if (strcmp(group_of(s_slot[i].name, gb, sizeof(gb)), grp) != 0)
            continue;
        if (is_countable(&s_slot[i]))
            sum += s_slot[i].short_avg / frame_ms * 100.0;
    }
    return sum;
}

void draw()
{
    if (!s_panel_visible)
        return;

    const SLONG screen_w = (SLONG)ge_get_screen_width();
    const SLONG screen_h = (SLONG)ge_get_screen_height();
    if (screen_w <= 0 || screen_h <= 0)
        return;

    const double frame_ms = s_frame_total ? s_frame_total->short_avg : 0.0;
    const SLONG max_y = screen_h - PERF_LINE_PX;
    const double fps = frame_ms > 1e-6 ? 1000.0 / frame_ms : 0.0;

    char hdr[48];
    snprintf(hdr, sizeof(hdr), "FPS %3.0f (%d last frames)", fps, short_w());
    // Width is measured from a FIXED template (FPS digits as a constant
    // "000"), not the live value — otherwise the changing FPS number
    // makes the measured header width (and thus the panel) jitter by a
    // pixel every frame. short_w() only changes on a KB_5 toggle.
    char hdr_tmpl[48];
    snprintf(hdr_tmpl, sizeof(hdr_tmpl), "FPS 000 (%d last frames)", short_w());

    // ---- Measure layout from the actual text (auto-hug, no slack) -----
    // value column x = pad + longest displayed name + small gap; panel
    // width = column + value field + pad (or the header, whichever is
    // wider). So the panel always tightly fits its content regardless of
    // which metrics are registered.
    SLONG name_w = 0;
    for (int i = 0; i < s_slot_count; i++) {
        char gb[16];
        group_of(s_slot[i].name, gb, sizeof(gb));
        if (!(strcmp(gb, "frame") == 0 && is_total(s_slot[i].name))) {
            SLONG w = FONT_string_width(0, (CBYTE*)s_slot[i].name, PERF_TEXT_SCALE);
            if (w > name_w) name_w = w;
        }
        if (strcmp(gb, "frame") != 0) {
            char tl[32];
            snprintf(tl, sizeof(tl), "%s .total", gb);
            SLONG w = FONT_string_width(0, (CBYTE*)tl, PERF_TEXT_SCALE);
            if (w > name_w) name_w = w;
        }
    }
    {
        SLONG w = FONT_string_width(0, (CBYTE*)"(other) .total", PERF_TEXT_SCALE);
        if (w > name_w) name_w = w;
    }
    const SLONG val_w     = FONT_string_width(0, (CBYTE*)"100.00%", PERF_TEXT_SCALE);
    const SLONG val_col_x = PERF_PAD_PX + name_w + PERF_COL_GAP_PX;
    SLONG panel_w = val_col_x + val_w + PERF_PAD_PX;
    {
        const SLONG hw = PERF_PAD_PX
            + FONT_string_width(0, (CBYTE*)hdr_tmpl, PERF_TEXT_SCALE) + PERF_PAD_PX;
        if (hw > panel_w) panel_w = hw;
    }

    // ---- Pass 1: queue all text, remember row-underline Y positions ----
    // Everything is in literal framebuffer pixels. Text is queued now and
    // flushed by FONT_buffer_draw later (after our rects) so it lands on
    // top of the backdrop/underlines. Both use the same literal-pixel TL
    // path → aligned in every render mode.
    SLONG underline_y[MAX_SLOTS * 2];
    bool  underline_tot[MAX_SLOTS * 2];
    int   underline_n = 0;

    SLONG y = PERF_TOP_PX;

    // Fixed top block: FPS + averaging window, then the "frame.*" group
    // (general per-frame stuff) — never part of the dynamic list.
    FONT_buffer_add_scaled(PERF_PAD_PX, y, 90, 230, 230, 1, PERF_TEXT_SCALE,
        (CBYTE*)"%s", hdr);
    y += PERF_LINE_PX;

    for (int i = 0; i < s_slot_count; i++) {
        const Slot* s = &s_slot[i];
        char gb[16];
        if (strcmp(group_of(s->name, gb, sizeof(gb)), "frame") != 0)
            continue;
        if (is_total(s->name))
            continue; // frame.total == whole frame == FPS above
        const double pct = (s->kind == KIND_TIME && frame_ms > 1e-6)
            ? s->short_avg / frame_ms * 100.0 : 0.0;
        UBYTE r, g, b;
        pct_color(pct, r, g, b);
        FONT_buffer_add_scaled(PERF_PAD_PX, y, 200, 200, 200, 1, PERF_TEXT_SCALE,
            (CBYTE*)"%s", s->name);
        FONT_buffer_add_scaled(val_col_x, y, r, g, b, 1, PERF_TEXT_SCALE,
            (CBYTE*)"%05.2f%%", pct);
        y += PERF_LINE_PX;
    }
    y += PERF_LINE_PX; // blank line under the fixed block

    // ---- Dynamic groups (everything except "frame") -------------------
    // First-seen group order. Per group: members listed, then a synthetic
    // "<group> .total" row (bright underline) — its % is the group's
    // share of the frame. All group totals + a final "(other)" remainder
    // sum to exactly 100%, so reading just the .total lines gives the
    // whole-frame picture; dive into a group only if its total looks off.
    char   seen[MAX_SLOTS][16];
    int    seen_n = 0;
    double sum_groups_pct = 0.0;

    for (int gi = 0; gi < s_slot_count; gi++) {
        char gb[16];
        group_of(s_slot[gi].name, gb, sizeof(gb));
        if (strcmp(gb, "frame") == 0)
            continue;

        bool already = false;
        for (int k = 0; k < seen_n; k++)
            if (strcmp(seen[k], gb) == 0) { already = true; break; }
        if (already)
            continue;
        snprintf(seen[seen_n++], 16, "%s", gb);

        // Member rows (skip the explicit "<group>.total" — it's the
        // synthetic total row below).
        for (int i = 0; i < s_slot_count; i++) {
            const Slot* s = &s_slot[i];
            char ig[16];
            if (strcmp(group_of(s->name, ig, sizeof(ig)), gb) != 0)
                continue;
            if (s->kind == KIND_TIME && is_total(s->name))
                continue;
            if (y > max_y)
                break;

            if (s->kind == KIND_TIME) {
                const double pct = frame_ms > 1e-6
                    ? s->short_avg / frame_ms * 100.0 : 0.0;
                UBYTE r, g, b;
                pct_color(pct, r, g, b);
                FONT_buffer_add_scaled(PERF_PAD_PX, y, 200, 200, 200, 1,
                    PERF_TEXT_SCALE, (CBYTE*)"%s", s->name);
                FONT_buffer_add_scaled(val_col_x, y, r, g, b, 1,
                    PERF_TEXT_SCALE, (CBYTE*)"%05.2f%%", pct);
            } else {
                // Counter (e.g. physics.ticks) — raw value, neutral.
                FONT_buffer_add_scaled(PERF_PAD_PX, y, 200, 200, 200, 1,
                    PERF_TEXT_SCALE, (CBYTE*)"%s", s->name);
                FONT_buffer_add_scaled(val_col_x, y, 200, 200, 200, 1,
                    PERF_TEXT_SCALE, (CBYTE*)"%.0f", s->short_avg);
            }
            if (underline_n < (int)(sizeof(underline_y) / sizeof(underline_y[0]))) {
                underline_y[underline_n]   = y + PERF_GLYPH_H_PX;
                underline_tot[underline_n] = false;
                underline_n++;
            }
            y += PERF_LINE_PX;
        }

        // Synthetic group total row.
        if (y <= max_y) {
            const double gt = group_total_pct(gb, frame_ms);
            sum_groups_pct += gt;
            char tl[32];
            snprintf(tl, sizeof(tl), "%s .total", gb);
            // Same colours as a normal row (name neutral, % graded by
            // load). The ONLY thing that marks a total is the thicker
            // brighter underline (pass 2).
            UBYTE r, g, b;
            pct_color(gt, r, g, b);
            FONT_buffer_add_scaled(PERF_PAD_PX, y, 200, 200, 200, 1,
                PERF_TEXT_SCALE, (CBYTE*)"%s", tl);
            FONT_buffer_add_scaled(val_col_x, y, r, g, b, 1,
                PERF_TEXT_SCALE, (CBYTE*)"%05.2f%%", gt);
            if (underline_n < (int)(sizeof(underline_y) / sizeof(underline_y[0]))) {
                underline_y[underline_n]   = y + PERF_GLYPH_H_PX;
                underline_tot[underline_n] = true;
                underline_n++;
            }
            y += PERF_LINE_PX;
        }
        y += PERF_LINE_PX; // blank line between groups
    }

    // "(other)" = unmeasured remainder so every .total above + this one
    // add up to exactly 100% of the frame (gaps between instrumented
    // blocks: input, sfx, gamepad output, etc.).
    if (y <= max_y) {
        double other = 100.0 - sum_groups_pct;
        if (other < 0.0) other = 0.0;
        else if (other > 100.0) other = 100.0;
        UBYTE r, g, b;
        pct_color(other, r, g, b);
        FONT_buffer_add_scaled(PERF_PAD_PX, y, 200, 200, 200, 1,
            PERF_TEXT_SCALE, (CBYTE*)"(other) .total");
        FONT_buffer_add_scaled(val_col_x, y, r, g, b, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2f%%", other);
        if (underline_n < (int)(sizeof(underline_y) / sizeof(underline_y[0]))) {
            underline_y[underline_n]   = y + PERF_GLYPH_H_PX;
            underline_tot[underline_n] = true;
            underline_n++;
        }
        y += PERF_LINE_PX;
    }

    // ---- Pass 2: backdrop + row underlines (literal-pixel layer) ------
    // Same coordinate space as the text → aligned in gameplay, menu and
    // cutscenes alike (no AENG_draw_rect / POLY logical-letterbox path).
    FONT_atlas_fill_begin();
    FONT_atlas_fill_rect(0, 0, panel_w, screen_h, 0xE6000000u); // ~90% black
    for (int i = 0; i < underline_n; i++) {
        // Total rows: a thicker, brighter rule so the group summary line
        // stands out from its members.
        const ULONG col = (ULONG)(underline_tot[i] ? PERF_RULE_TOTAL : PERF_RULE_NORMAL);
        const SLONG th  = underline_tot[i] ? (2 * PERF_TEXT_SCALE + 1)
                                           : (PERF_TEXT_SCALE + 1);
        FONT_atlas_fill_rect(0, underline_y[i], panel_w, th, col);
    }
    FONT_atlas_fill_end();
}

#else // !OC_DEBUG_PERF — log-only build: panel is a no-op

void draw() {}

#endif // OC_DEBUG_PERF

} // namespace perf

#endif // OC_PERF_ACTIVE
