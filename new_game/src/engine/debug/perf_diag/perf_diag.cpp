#include "engine/debug/perf_diag/perf_diag.h"

#if OC_PERF_ACTIVE

#include "engine/platform/uc_common.h"            // SLONG / ULONG / UBYTE / CBYTE
#include "engine/platform/sdl3_bridge.h"          // perf counter / get_ticks
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // ge_gpu_finish / ge_draw_call_*
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#if OC_DEBUG_PERF
#include "engine/graphics/text/font.h"            // FONT_buffer_add_scaled
#include "engine/graphics/text/font_atlas.h"      // FONT_atlas_fill_* (literal-px rects)
#include "engine/debug/debug_panel_text_scale.h"  // DBG_PANEL_TEXT_SCALE
#include "engine/input/input_frame.h"             // input_key_just_pressed
#include "engine/input/keyboard.h"                // KB_4 / KB_5
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
    // CPU / value channel.
    double   accum;             // current-frame accumulator
    double   ring[RING];
    int      head;              // next write
    int      count;             // valid entries (≤ RING)
    double   short_avg;
    double   long_avg;
    // GPU channel — real GPU time of this stage (ScopeTimer dtor
    // glFinishes on exit and times the drain).
    double   gpu_accum;
    double   gpu_ring[RING];
    int      gpu_head;
    int      gpu_count;
    double   gpu_short_avg;
    // Graphics-API CPU channel (CPU wall spent inside ge_* draws/
    // composition/swap — driver/submit/state-changes). Own column.
    double   ge_accum;
    double   ge_ring[RING];
    int      ge_head;
    int      ge_count;
    double   ge_short_avg;
    // Display order + liveness. seq = order this slot was first entered
    // THIS frame (so rows list in real frame call order). seen_frame =
    // last frame the slot was hit (stale → hidden, so menu/gameplay don't
    // leave each other's dead rows hanging).
    int      seq;
    uint64_t seen_frame;
};

static Slot s_slot[MAX_SLOTS];
static int  s_slot_count = 0;

static Slot* s_frame_total = nullptr; // builtin: whole-frame ms (→ FPS)

static uint64_t s_freq        = 0;    // perf counter frequency (Hz)
static uint64_t s_frame_t0    = 0;    // perf counter at frame_begin
static uint64_t s_frame_idx   = 0;
static bool     s_frame_began = false; // a frame_begin has run at least once
static uint32_t s_seq_counter = 0;    // reset each frame_begin; ++ per first slot hit

// Graphics-API CPU accumulator (perf-counter ticks), depth-counted so
// nested ge_* calls don't double-count. Monotonic; scopes snapshot it.
static uint64_t s_ge_accum = 0;
static int      s_ge_depth = 0;
static uint64_t s_ge_t0    = 0;

// glFinish GPU measurement is ALWAYS on while perf is compiled in (no
// toggle): each scope glFinishes on exit and the CPU clock measures that
// stage's real GPU drain. This serialises CPU/GPU so the displayed FPS
// is synthetically lower than a normal run — accepted on purpose: a
// debug build that clearly shows what is CPU vs GPU is worth more than a
// realistic FPS here. Fully compiled out when perf is off (zero cost).

// Global monotonic accumulator (ticks) of time spent blocked in
// ge_gpu_finish(). Every ScopeTimer subtracts the finish-wait that
// elapsed inside it from its own wall, so all CPU/wall figures (incl.
// wrapper ".other") stay PURE — free of the synthetic glFinish cost.
// The total is shown as one separate "glfinish" row (ignore it; it's the
// price of the measurement, not the game).
static uint64_t s_gfinish_accum = 0;
static uint64_t s_frame_gf0     = 0; // snapshot at frame_begin

// The single "all CPU-side GPU-wait" row (per-stage glFinish drains +
// pre-swap drain). CPU blocked draining the GPU — the measurement cost.
// Kept out of s_ord/roots and out of EVERY column's 100%; drawn as its
// own muted ms-only line AFTER the total so the percentages reflect
// only the real work.
static const char* const PERF_GPU_WAIT_NAME = "GPU wait (glFinish)";

// Global monotonic accumulator (ticks) of time spent inside
// PERF_OVERHEAD_SCOPE — the cost of drawing the perf panel itself
// (glyph queue + flush). Treated exactly like s_gfinish_accum: every
// ScopeTimer subtracts the overhead that elapsed inside it from its own
// wall, so EVERY stage / wrapper ".other" / percentage stays pure (the
// diagnostics cost never pollutes the game's numbers). The per-frame
// total is booked into the slot below and drawn as one standalone line
// next to "GPU wait", outside every 100%. The queue site and the flush
// site both add here → they SUM (no overwrite).
static uint64_t s_overhead_accum = 0;
static uint64_t s_frame_ov0      = 0; // snapshot at frame_begin

// Companion accumulator: graphics-API CPU (s_ge_accum ticks) spent INSIDE
// PERF_OVERHEAD_SCOPE. The panel's glyph flush issues real ge_* draws, so
// without this their ge_* CPU would leak into the enclosing scope's ge_*
// channel (render.ui.other) and inflate the "ge_* CPU" column with
// diagnostics. ScopeTimer subtracts this from its ge attribution exactly
// as it does for the wall, keeping that column pure too.
static uint64_t s_overhead_ge_accum = 0;

// Companion accumulator: GPU drain (ge_gpu_finish ticks) done INSIDE
// PERF_OVERHEAD_SCOPE — the panel's own GPU work. Display-only: lets the
// "perfpanel" row split honestly into ge_* / non-ge / GPU. NOT subtracted
// from enclosing scopes' GPU channel (the panel drains its own GPU here,
// so a parent's glFinish already reads ~0 for it — no leak to strip).
static uint64_t s_overhead_gpu_accum = 0;

// Frame-start snapshots for the per-frame perfpanel split (mirrors
// s_frame_ov0 for the wall, added so ge_* / GPU parts are per-frame too).
static uint64_t s_frame_ovge0  = 0;
static uint64_t s_frame_ovgpu0 = 0;

// The single "perf panel draw cost" row. Kept out of s_ord/roots and out
// of every column total; drawn as its own muted ms-only line by the
// total, like GPU-wait. Shown short ("perfpanel") so it reads as a
// thing apart, not as render.ui internals (even though that's physically
// where it runs).
static const char* const PERF_PERFPANEL_NAME = "perfpanel";

// Stamp a slot's frame call order + liveness on its first hit this frame.
static void touch_slot(Slot* s)
{
    if (!s || s->seen_frame == s_frame_idx)
        return;
    s->seq = (int)s_seq_counter++;
    s->seen_frame = s_frame_idx;
}

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
    if (s) { touch_slot(s); s->accum += v; }
}

ScopeTimer::ScopeTimer(Slot* s)
    : slot(s), t0((ensure_freq(), sdl3_get_performance_counter())),
      ge0(s_ge_accum), gf0(s_gfinish_accum), ov0(s_overhead_accum),
      ovge0(s_overhead_ge_accum)
{
    touch_slot(s);
}

ScopeTimer::~ScopeTimer()
{
    if (!slot || s_freq == 0)
        return;
    const uint64_t now = sdl3_get_performance_counter();
    // Wall = entry→here, MINUS any glFinish-wait that elapsed inside this
    // scope (descendants' finishes ran on the CPU thread during it). So
    // every CPU/wall figure — incl. wrapper ".other" — stays pure, free
    // of the synthetic glFinish cost (which is shown as its own row).
    double wall = (double)(now - t0);
    wall -= (double)(s_gfinish_accum - gf0);
    // Also strip the perf-panel overhead that ran inside this scope, so
    // every stage / wrapper ".other" stays free of the diagnostics cost
    // (same contract as the glFinish subtraction above).
    wall -= (double)(s_overhead_accum - ov0);
    if (wall < 0.0) wall = 0.0;
    slot->accum += wall * 1000.0 / (double)s_freq;
    // ge_* spent inside this scope, MINUS the perf-panel's own ge_* that
    // ran within it (same isolation as the wall above) so the "ge_* CPU"
    // column never includes the diagnostics flush.
    double ge_t = (double)(s_ge_accum - ge0)
                - (double)(s_overhead_ge_accum - ovge0);
    if (ge_t < 0.0) ge_t = 0.0;
    slot->ge_accum += ge_t * 1000.0 / (double)s_freq;

    // Drain the GPU now. The previous (sequential) scope already drained
    // its work, so this wait = THIS scope's real GPU time. Wrappers
    // naturally read ~0 (children already drained). The wait is added to
    // the global glFinish accumulator AFTER measuring it, so the
    // enclosing (parent) scope subtracts it from its own wall too.
    ge_gpu_finish();
    const uint64_t after = sdl3_get_performance_counter();
    const uint64_t fwait = after - now;
    slot->gpu_accum += (double)fwait * 1000.0 / (double)s_freq;
    s_gfinish_accum += fwait;
}

// ---- Graphics-API CPU bracket -------------------------------------------

GeCallTimer::GeCallTimer()
{
    ensure_freq();
    if (s_ge_depth++ == 0)               // outermost of a nested group
        s_ge_t0 = sdl3_get_performance_counter();
}

GeCallTimer::~GeCallTimer()
{
    if (--s_ge_depth == 0)
        s_ge_accum += sdl3_get_performance_counter() - s_ge_t0;
}

// ---- Perf-panel overhead bracket ----------------------------------------

OverheadTimer::OverheadTimer()
    : t0((ensure_freq(), sdl3_get_performance_counter())), ge0(s_ge_accum)
{
}

OverheadTimer::~OverheadTimer()
{
    if (s_freq == 0)
        return;
    // Drain the GPU so the panel's own draw cost (incl. its GPU work) is
    // fully attributed here, not leaked into a later stage's glFinish.
    // Deliberately NOT added to s_gfinish_accum — this is perf-panel
    // overhead, booked to the "perfpanel" line, never the "GPU wait" one.
    // Time the drain on its own so the perfpanel row can split into
    // ge_* / non-ge / GPU.
    const uint64_t pre_finish = sdl3_get_performance_counter();
    ge_gpu_finish();
    const uint64_t after = sdl3_get_performance_counter();
    s_overhead_gpu_accum += after - pre_finish;
    // Book this instance's ge_* (the flush's real draws) so every
    // enclosing ScopeTimer strips it from its ge_* channel, then its
    // wall (incl. the drain just above) so they strip that too. ge first
    // — the wall add must come last so a parent's snapshot diff is exact.
    s_overhead_ge_accum += s_ge_accum - ge0;
    s_overhead_accum += after - t0;
}

// ---- Sequential phase timer ---------------------------------------------

// One active phase at a time (not nestable). Snapshots mirror ScopeTimer:
// the close path is byte-for-byte the same wall / ge_* / GPU contract.
static Slot*    s_phase_slot  = nullptr;
static uint64_t s_phase_t0    = 0;
static uint64_t s_phase_ge0   = 0;
static uint64_t s_phase_gf0   = 0;
static uint64_t s_phase_ov0   = 0;
static uint64_t s_phase_ovge0 = 0;

// Close the open phase, booking it exactly as ScopeTimer::~ScopeTimer
// does (strip glFinish-wait + perf-panel overhead from wall and ge_*,
// then drain the GPU and book this phase's real GPU time, feeding the
// drain back into s_gfinish_accum so the enclosing wrapper stays pure).
static void phase_close()
{
    Slot* slot = s_phase_slot;
    s_phase_slot = nullptr;
    if (!slot || s_freq == 0)
        return;
    const uint64_t now = sdl3_get_performance_counter();
    double wall = (double)(now - s_phase_t0);
    wall -= (double)(s_gfinish_accum  - s_phase_gf0);
    wall -= (double)(s_overhead_accum - s_phase_ov0);
    if (wall < 0.0) wall = 0.0;
    slot->accum += wall * 1000.0 / (double)s_freq;
    double ge_t = (double)(s_ge_accum         - s_phase_ge0)
                - (double)(s_overhead_ge_accum - s_phase_ovge0);
    if (ge_t < 0.0) ge_t = 0.0;
    slot->ge_accum += ge_t * 1000.0 / (double)s_freq;
    ge_gpu_finish();
    const uint64_t after = sdl3_get_performance_counter();
    const uint64_t fwait = after - now;
    slot->gpu_accum += (double)fwait * 1000.0 / (double)s_freq;
    s_gfinish_accum += fwait;
}

void phase_begin(Slot* s)
{
    phase_close();              // close the previous phase, if any
    ensure_freq();
    s_phase_slot  = s;
    s_phase_t0    = sdl3_get_performance_counter();
    s_phase_ge0   = s_ge_accum;
    s_phase_gf0   = s_gfinish_accum;
    s_phase_ov0   = s_overhead_accum;
    s_phase_ovge0 = s_overhead_ge_accum;
    touch_slot(s);
}

void phase_end()
{
    phase_close();
}

// Drain the GPU right before the buffer swap and book the wait into the
// "GPU wait" bucket — so ALL real GPU-wait is captured here regardless
// of which stage ran last, and the swap then waits ~0. The drain is
// inside the render.present scope, so the existing s_gfinish_accum
// subtraction keeps CPU/ge_*/render.other pure.
void pre_swap_gpu_drain()
{
    if (s_freq == 0)
        return;
    const uint64_t t = sdl3_get_performance_counter();
    ge_gpu_finish();
    s_gfinish_accum += sdl3_get_performance_counter() - t;
}

// ---- Averaging ----------------------------------------------------------

static double avg_ring(const double* ring, int head, int count, int n)
{
    int m = count < n ? count : n;
    if (m <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < m; i++) {
        int idx = (head - 1 - i + RING) % RING;
        sum += ring[idx];
    }
    return sum / (double)m;
}

static double avg_last(const Slot* s, int n)
{
    return avg_ring(s->ring, s->head, s->count, n);
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
    // Drop a phase still open from the previous frame (e.g. the F10
    // far-facet-debug early return inside AENG_draw_city skips
    // PERF_PHASE_END). Its partial, frame-crossing time is intentionally
    // NOT booked — kept out of every figure rather than corrupting one.
    s_phase_slot = nullptr;
    s_seq_counter = 0; // restart per-frame call-order numbering
    s_frame_gf0 = s_gfinish_accum;
    s_frame_ov0    = s_overhead_accum;
    s_frame_ovge0  = s_overhead_ge_accum;
    s_frame_ovgpu0 = s_overhead_gpu_accum;
    s_frame_t0 = sdl3_get_performance_counter();
}

void frame_end()
{
    if (!s_frame_began)
        return; // frame_begin never ran (partial frame) — skip

    const uint64_t now   = sdl3_get_performance_counter();
    const double   total = (double)(now - s_frame_t0) * 1000.0 / (double)s_freq;
    if (s_frame_total) s_frame_total->accum = total;

    // GPU draw-call count for the frame just finished (a value metric —
    // shows under "render" as a counter; not a time, so not part of the
    // CPU 100% sum).
    static Slot* s_drawcalls = nullptr;
    if (!s_drawcalls)
        s_drawcalls = get_slot("render.drawcalls", KIND_VALUE);
    if (s_drawcalls) {
        touch_slot(s_drawcalls); // order it right after the render scopes
        s_drawcalls->accum += (double)ge_draw_call_count();
    }
    ge_draw_call_count_reset();

    // ALL CPU-side GPU-wait in ONE row: per-stage glFinish drains +
    // pre-swap drain. It's "CPU stuck waiting on the GPU" — graphics-side
    // CPU wall, shown separately so it can be discounted. Routed here &
    // subtracted from every stage's wall, so CPU/ge_*/render.other stay
    // pure.
    {
        static Slot* s_gf = nullptr;
        if (!s_gf)
            s_gf = get_slot(PERF_GPU_WAIT_NAME, KIND_TIME);
        if (s_gf) {
            touch_slot(s_gf);
            s_gf->accum += (double)(s_gfinish_accum - s_frame_gf0)
                * 1000.0 / (double)s_freq;
        }
    }

    // Perf-panel draw cost for the frame just finished (queue + flush
    // sites summed via s_overhead_accum). One standalone "perfpanel"
    // row, like GPU-wait: out of s_ord and every 100%, drawn by the
    // footer. Already subtracted from every stage's wall by ScopeTimer.
    {
        static Slot* s_ovh = nullptr;
        if (!s_ovh)
            s_ovh = get_slot(PERF_PERFPANEL_NAME, KIND_TIME);
        if (s_ovh) {
            touch_slot(s_ovh);
            const double k = 1000.0 / (double)s_freq;
            // wall total (= ge_* submit + non-ge build + GPU drain),
            // plus the ge_* and GPU parts so the row splits honestly.
            // The generic roll below averages all three channels.
            s_ovh->accum     += (double)(s_overhead_accum    - s_frame_ov0)    * k;
            s_ovh->ge_accum  += (double)(s_overhead_ge_accum  - s_frame_ovge0) * k;
            s_ovh->gpu_accum += (double)(s_overhead_gpu_accum - s_frame_ovgpu0)* k;
        }
    }

    for (int i = 0; i < s_slot_count; i++) {
        Slot* s = &s_slot[i];
        const double sample = s->accum;

        s->ring[s->head] = sample;
        s->head = (s->head + 1) % RING;
        if (s->count < RING) s->count++;

        s->short_avg = avg_last(s, short_w());
        s->long_avg  = avg_last(s, LONG_W);
        s->accum = 0.0;

        // GPU channel (per-scope glFinish drain = real GPU time).
        s->gpu_ring[s->gpu_head] = s->gpu_accum;
        s->gpu_head = (s->gpu_head + 1) % RING;
        if (s->gpu_count < RING) s->gpu_count++;
        s->gpu_short_avg =
            avg_ring(s->gpu_ring, s->gpu_head, s->gpu_count, short_w());
        s->gpu_accum = 0.0;

        // Graphics-API CPU channel (rolled like the others).
        s->ge_ring[s->ge_head] = s->ge_accum;
        s->ge_head = (s->ge_head + 1) % RING;
        if (s->ge_count < RING) s->ge_count++;
        s->ge_short_avg = avg_ring(s->ge_ring, s->ge_head, s->ge_count, short_w());
        s->ge_accum = 0.0;
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
static constexpr SLONG PERF_COL_GAP_PX  = 70 * PERF_TEXT_SCALE; // name→CPU% gap (tunable)
static constexpr SLONG PERF_COL_GAP2_PX = 24 * PERF_TEXT_SCALE; // CPU%→GPU ms gap (tunable)

// Row-underline colour (AARRGGBB, alpha-blended) — a thin grey rule under
// every row so the eye doesn't mis-pair a name with a far value. No
// "total" rows any more (they confused more than helped), so a single
// uniform rule — no special bright/thick line.
static constexpr SLONG PERF_RULE_NORMAL = (SLONG)0x80909090u;

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

// ---- Hierarchy helpers --------------------------------------------------
// Metric names are dot-paths ("render.scene"). The panel is a tree:
//  - a LEAF (no measured time child) prints its own row;
//  - a CONTAINER (has measured time children) does NOT print itself —
//    it prints its children then a synthetic "<name>.other" row =
//    (container measured) − Σ(direct time children). So children + .other
//    always reconstruct the container exactly → the whole table's CPU%
//    sums to exactly 100% (top "(other)" = frame − Σ roots).
// Rows appear in FRAME CALL order (per-frame seq stamped on first hit),
// so the list reads top-to-bottom exactly as the frame runs. Stale slots
// (a stage not run for a while — e.g. gameplay metrics after returning to
// the menu) are hidden. Level-1 groups (first dotted token) are separated
// by a blank line; deeper levels are not.

// Parent path = name minus its last ".segment" ("" if no dot).
static void parent_path(const char* name, char* buf, int bufsz)
{
    int n = (int)strlen(name), cut = -1;
    for (int i = n - 1; i >= 0; i--)
        if (name[i] == '.') { cut = i; break; }
    int len = cut < 0 ? 0 : cut;
    if (len > bufsz - 1) len = bufsz - 1;
    memcpy(buf, name, (size_t)len);
    buf[len] = '\0';
}

// Frames-of-inactivity after which a slot is considered stale and hidden
// (set to the averaging window in draw()): once a stage stops running —
// e.g. gameplay scopes after returning to the menu — its averaged value
// is all-zero anyway, so don't leave a dead empty row hanging.
static uint64_t s_stale_win = 30;

static bool slot_stale(int i)
{
    return (s_frame_idx - s_slot[i].seen_frame) > s_stale_win;
}

// Sorted-by-frame-call-order view of the live, non-"frame" slots, rebuilt
// each draw(). Iterating this (instead of registration order) makes rows
// list exactly in the order the stages run in a frame, and drops stale
// rows.
static int s_ord[MAX_SLOTS];
static int s_ordn = 0;

static int find_time_slot(const char* name)
{
    for (int i = 0; i < s_slot_count; i++)
        if (s_slot[i].kind == KIND_TIME && !slot_stale(i)
            && strcmp(s_slot[i].name, name) == 0)
            return i;
    return -1;
}

static bool is_frame_group(const char* name)
{
    char g[16];
    return strcmp(group_of(name, g, sizeof(g)), "frame") == 0;
}

// A root = a time metric whose parent path is not itself a measured time
// metric (so it's a top-level frame stage: physics, render, idle, …).
static bool is_root(int idx)
{
    if (s_slot[idx].kind != KIND_TIME || is_frame_group(s_slot[idx].name))
        return false;
    char pp[64];
    parent_path(s_slot[idx].name, pp, sizeof(pp));
    return pp[0] == '\0' || find_time_slot(pp) < 0;
}


static bool has_time_children(int idx)
{
    char pp[64];
    for (int i = 0; i < s_slot_count; i++) {
        if (s_slot[i].kind != KIND_TIME || slot_stale(i)) continue;
        parent_path(s_slot[i].name, pp, sizeof(pp));
        if (strcmp(pp, s_slot[idx].name) == 0) return true;
    }
    return false;
}

struct DrawCtx {
    // Each column is normalised by its OWN total → every column sums to
    // 100% on its own (no "add two columns" needed): cpu% = stage pure
    // CPU / total CPU; ge% = stage ge_* / total ge_*; gpu% = stage GPU /
    // total GPU. frame_ms kept only for the header split.
    double frame_ms, cpu_total, ge_total, gpu_total;
    SLONG  cpu_col_x, ge_col_x, gpu_col_x, max_y, y;
    SLONG* uy;
    int*   un;
    int    ucap;
};

// Total-row highlight colour (bright amber) — set apart from the
// load-graded data cells and the cyan section headings.
static const UBYTE PERF_TOTAL_R = 255, PERF_TOTAL_G = 215, PERF_TOTAL_B = 70;

// One value cell: "PCT% (MSms)". In the total row the colour is the
// fixed highlight; otherwise it's load-graded by the percentage.
static void emit_cell(SLONG x, SLONG yy, double pct, double ms,
                      bool is_total)
{
    UBYTE r, g, b;
    if (is_total) { r = PERF_TOTAL_R; g = PERF_TOTAL_G; b = PERF_TOTAL_B; }
    else          pct_color(pct, r, g, b);
    FONT_buffer_add_scaled(x, yy, r, g, b, 1, PERF_TEXT_SCALE,
        (CBYTE*)"%05.2f%% (%05.2fms)", pct, ms);
}

// One row. Counter → name + raw value (cpu column, neutral). Time row →
// name + per-column "pct% (msms)". is_total → whole row in the fixed
// highlight colour (the bottom "total" row, always 100% per column).
static void emit_row(DrawCtx& c, const char* nm, bool is_counter,
                     double counter_val,
                     double cpu_pct, double ge_pct, double gpu_pct,
                     double cpu_ms, double ge_ms, double gpu_ms,
                     bool is_total = false)
{
    if (c.y > c.max_y)
        return;
    const UBYTE nr = is_total ? PERF_TOTAL_R : 200;
    const UBYTE ng = is_total ? PERF_TOTAL_G : 200;
    const UBYTE nb = is_total ? PERF_TOTAL_B : 200;
    FONT_buffer_add_scaled(PERF_PAD_PX, c.y, nr, ng, nb, 1,
        PERF_TEXT_SCALE, (CBYTE*)"%s", nm);
    if (is_counter) {
        // Counters are window-averaged: a small value (e.g. physics.ticks
        // decoupled from render → ~0.2/frame) needs decimals or it rounds
        // to 0. Big counts (drawcalls, thousands) stay integer. Threshold
        // 1000 separates "rate-ish small" from "bulk count".
        const char* cfmt = (counter_val < 1000.0 && counter_val > -1000.0)
            ? "%.2f" : "%.0f";
        FONT_buffer_add_scaled(c.cpu_col_x, c.y, 200, 200, 200, 1,
            PERF_TEXT_SCALE, (CBYTE*)cfmt, counter_val);
    } else {
        emit_cell(c.ge_col_x,  c.y, ge_pct,  ge_ms,  is_total);
        emit_cell(c.cpu_col_x, c.y, cpu_pct, cpu_ms, is_total);
        emit_cell(c.gpu_col_x, c.y, gpu_pct, gpu_ms, is_total);
    }
    if (*c.un < c.ucap)
        c.uy[(*c.un)++] = c.y + PERF_GLYPH_H_PX;
    c.y += PERF_LINE_PX;
}

// Print a node's subtree (children in frame call order, then the node's
// ".other" if it's a container). Leaves print themselves.
static void emit_node(DrawCtx& c, int idx)
{
    const Slot* s = &s_slot[idx];
    char pp[64];

    if (!has_time_children(idx)) {
        double ge   = s->ge_short_avg;
        double pure = s->short_avg - ge;        // wall − graphics-API CPU
        if (pure < 0.0) pure = 0.0;
        const double cpu_pct = c.cpu_total > 1e-6 ? pure / c.cpu_total * 100.0 : 0.0;
        const double ge_pct  = c.ge_total  > 1e-6 ? ge   / c.ge_total  * 100.0 : 0.0;
        const double gpu_pct = c.gpu_total > 1e-6
            ? s->gpu_short_avg / c.gpu_total * 100.0 : 0.0;
        emit_row(c, s->name, false, 0.0, cpu_pct, ge_pct, gpu_pct,
                 pure, ge, s->gpu_short_avg);
        // Counters (KIND_VALUE) go in their own list at the bottom.
        return;
    }

    // Children in frame call order (s_ord = seq-sorted, stale dropped).
    // Only KIND_TIME children — counters are listed separately below.
    double child_cpu = 0.0, child_ge = 0.0, child_gpu = 0.0;
    for (int o = 0; o < s_ordn; o++) {
        int i = s_ord[o];
        if (s_slot[i].kind != KIND_TIME)
            continue;
        parent_path(s_slot[i].name, pp, sizeof(pp));
        if (strcmp(pp, s->name) != 0)
            continue;
        child_cpu += s_slot[i].short_avg;
        child_ge  += s_slot[i].ge_short_avg;
        child_gpu += s_slot[i].gpu_short_avg;
        emit_node(c, i);
    }
    char on[80];
    snprintf(on, sizeof(on), "%s.other", s->name);
    double o_wall = s->short_avg     - child_cpu; if (o_wall < 0.0) o_wall = 0.0;
    double o_ge   = s->ge_short_avg  - child_ge;  if (o_ge   < 0.0) o_ge   = 0.0;
    double o_gpu  = s->gpu_short_avg - child_gpu; if (o_gpu < 0.0) o_gpu = 0.0;
    double o_pure = o_wall - o_ge; if (o_pure < 0.0) o_pure = 0.0;
    const double cpu_pct = c.cpu_total > 1e-6 ? o_pure / c.cpu_total * 100.0 : 0.0;
    const double ge_pct  = c.ge_total  > 1e-6 ? o_ge   / c.ge_total  * 100.0 : 0.0;
    const double gpu_pct = c.gpu_total > 1e-6
        ? o_gpu / c.gpu_total * 100.0 : 0.0;
    emit_row(c, on, false, 0.0, cpu_pct, ge_pct, gpu_pct,
             o_pure, o_ge, o_gpu);
}

void draw()
{
    if (!s_panel_visible)
        return;

    const SLONG screen_w = (SLONG)ge_get_screen_width();
    const SLONG screen_h = (SLONG)ge_get_screen_height();
    if (screen_w <= 0 || screen_h <= 0)
        return;

    // Route this panel's glyphs into the dedicated PERF queue so its
    // rasterisation is flushed (and timed) on its own — separate from
    // game/dbglog text. Restored to MAIN before returning. No early
    // returns between here and the end of the function (the ctx.y guards
    // only skip emission). The FONT_atlas_fill_* backdrop path is
    // immediate, not queued, so the selector does not affect it.
    FONT_buffer_select(FONT_QUEUE_PERF);

    const double frame_ms = s_frame_total ? s_frame_total->short_avg : 0.0;
    const SLONG  max_y = screen_h - PERF_LINE_PX;
    const double fps = frame_ms > 1e-6 ? 1000.0 / frame_ms : 0.0;

    // Slot is stale after the averaging window of inactivity (its average
    // is all-zero by then anyway). Build the ordered live view: non-frame,
    // non-stale slots sorted by frame-call seq.
    // The GPU-wait row is kept OUT of s_ord (not a stage/root) and out
    // of every column total — drawn as its own ms-only line after the
    // total. Captured here for that line, the header, and to subtract
    // from cpu_total/(other).
    double gpu_wait_ms = 0.0;
    for (int i = 0; i < s_slot_count; i++)
        if (strcmp(s_slot[i].name, PERF_GPU_WAIT_NAME) == 0) {
            gpu_wait_ms = s_slot[i].short_avg;
            break;
        }

    // Perf-panel draw cost — same treatment as GPU-wait: captured for
    // its own line + subtracted from cpu_total/(other) so it stays out
    // of every 100%.
    double perfpanel_ms = 0.0;   // wall total (removed from the 100%)
    double perfpanel_ge = 0.0;   // ge_* part   (flush draws)
    double perfpanel_gpu = 0.0;  // GPU part    (its glFinish drain)
    for (int i = 0; i < s_slot_count; i++)
        if (strcmp(s_slot[i].name, PERF_PERFPANEL_NAME) == 0) {
            perfpanel_ms  = s_slot[i].short_avg;
            perfpanel_ge  = s_slot[i].ge_short_avg;
            perfpanel_gpu = s_slot[i].gpu_short_avg;
            break;
        }
    // non-ge CPU = wall − ge_* − GPU drain (the CPU glyph build / queue).
    double perfpanel_nonge = perfpanel_ms - perfpanel_ge - perfpanel_gpu;
    if (perfpanel_nonge < 0.0) perfpanel_nonge = 0.0;

    s_stale_win = (uint64_t)short_w();
    s_ordn = 0;
    for (int i = 0; i < s_slot_count; i++) {
        // Skip the whole-frame total, stale slots, internal scopes (name
        // starts with '_'), and the GPU-wait row (drawn separately
        // after the total — must not also be a stage/root here).
        if (is_frame_group(s_slot[i].name) || slot_stale(i)
            || s_slot[i].name[0] == '_'
            || strcmp(s_slot[i].name, PERF_GPU_WAIT_NAME) == 0
            || strcmp(s_slot[i].name, PERF_PERFPANEL_NAME) == 0)
            continue;
        s_ord[s_ordn++] = i;
    }
    for (int a = 0; a < s_ordn; a++) // small N, simple insertion sort by seq
        for (int b = a + 1; b < s_ordn; b++)
            if (s_slot[s_ord[b]].seq < s_slot[s_ord[a]].seq) {
                int t = s_ord[a]; s_ord[a] = s_ord[b]; s_ord[b] = t;
            }

    // GPU "pie" total = Σ of EVERY time stage's GPU, wrappers included.
    // GPU is not span-measured: each scope glFinishes on exit and books
    // only the drain of its OWN as-yet-undrained submissions — children
    // already drained at their own exits, so a wrapper's gpu_accum holds
    // just its direct residual (exactly its ".other"). The slots'
    // GPU therefore partition the frame with no overlap, and summing all
    // of them (leaves + wrappers) double-counts nothing. The old
    // leaf-only sum DROPPED every wrapper residual: when GPU work is done
    // directly inside a wrapper past its named sub-scopes (e.g. the whole
    // menu/console draw lands in render.ui.other), `total` under-counted
    // and that ".other" row blew past 100%. GPU-wait / perfpanel are
    // already kept out of s_ord, so they stay out of this sum.
    double gpu_total = 0.0;
    for (int o = 0; o < s_ordn; o++) {
        int i = s_ord[o];
        if (s_slot[i].kind == KIND_TIME)
            gpu_total += s_slot[i].gpu_short_avg;
    }

    // Per-column totals so EACH column sums to 100% on its own — and
    // GPU-wait is NOT in any of them. The frame partitions into: ge_*
    // CPU (real submit) + pure non-ge CPU + GPU-wait (its own line after
    // the total, no %). Excluding GPU-wait from the 100% makes the
    // percentages reflect ONLY the real work:
    //   ge_total  = Σ ge_* over roots (real submit, NO GPU-wait);
    //   cpu_total = frame − ge_total − GPU-wait (pure non-ge incl
    //               "(other)").
    double ge_total = 0.0;
    for (int o = 0; o < s_ordn; o++) {
        int i = s_ord[o];
        if (is_root(i))
            ge_total += s_slot[i].ge_short_avg;
    }
    // perfpanel subtracted too: ScopeTimer already strips it from every
    // stage's wall, but frame.total is raw (incl. it), so the non-ge
    // denominator must drop it as well or the rows wouldn't sum to 100%.
    double cpu_total = frame_ms - ge_total - gpu_wait_ms - perfpanel_ms;
    if (cpu_total < 1e-6) cpu_total = 1e-6;

    // Header is just the FPS line now (the CPU/GPU breakdown moved into
    // the "split" footer row of the table).
    char hdr[64];
    snprintf(hdr, sizeof(hdr), "FPS %3.0f (%d last frames)", fps, short_w());
    // Width from a FIXED template (digits as constants) so neither the
    // FPS value nor the averaging-window value (key 5 cycles 10/30/60/120
    // — different digit counts) changes the measured width.
    char hdr_tmpl[64];
    snprintf(hdr_tmpl, sizeof(hdr_tmpl), "FPS 000 (000 last frames)");

    // ---- Measure layout (auto-hug): name | CPU% | GPU% ---------------
    SLONG name_w = 0;
    for (int o = 0; o < s_ordn; o++) {
        int i = s_ord[o];
        SLONG w = FONT_string_width(0, (CBYTE*)s_slot[i].name, PERF_TEXT_SCALE);
        if (w > name_w) name_w = w;
        if (s_slot[i].kind == KIND_TIME && has_time_children(i)) {
            char on[80];
            snprintf(on, sizeof(on), "%s.other", s_slot[i].name);
            w = FONT_string_width(0, (CBYTE*)on, PERF_TEXT_SCALE);
            if (w > name_w) name_w = w;
        }
    }
    {
        SLONG w = FONT_string_width(0, (CBYTE*)"(other)", PERF_TEXT_SCALE);
        if (w > name_w) name_w = w;
    }
    // Three value columns: CPU | ge_* | GPU. Width auto-hugs (panel
    // widens to fit the extra column).
    // Order: ge_* | CPU | GPU (CPU and GPU adjacent for easy compare).
    // Column stride must fit BOTH the widest value cell
    // ("100.00% (0000.00ms)") and the widest column title
    // ("non-ge_* CPU") — otherwise text bleeds into the next column.
    SLONG colw = FONT_string_width(0, (CBYTE*)"100.00% (0000.00ms)",
                                   PERF_TEXT_SCALE);
    {
        static const char* const col_titles[] = {
            "ge_* CPU", "non-ge_* CPU", "GPU"
        };
        for (const char* t : col_titles) {
            SLONG w = FONT_string_width(0, (CBYTE*)t, PERF_TEXT_SCALE);
            if (w > colw) colw = w;
        }
    }
    const SLONG ge_col_x  = PERF_PAD_PX + name_w + PERF_COL_GAP_PX;
    const SLONG cpu_col_x = ge_col_x  + colw + PERF_COL_GAP2_PX;
    const SLONG gpu_col_x = cpu_col_x + colw + PERF_COL_GAP2_PX;
    SLONG panel_w = gpu_col_x + colw + PERF_PAD_PX;
    {
        const SLONG hw = PERF_PAD_PX
            + FONT_string_width(0, (CBYTE*)hdr_tmpl, PERF_TEXT_SCALE) + PERF_PAD_PX;
        if (hw > panel_w) panel_w = hw;
    }

    SLONG underline_y[MAX_SLOTS * 2];
    int   underline_n = 0;
    SLONG y = PERF_TOP_PX;

    // Header: FPS line only (cyan). The CPU/GPU breakdown lives in the
    // "split" footer row of the table now.
    FONT_buffer_add_scaled(PERF_PAD_PX, y, 90, 230, 230, 1, PERF_TEXT_SCALE,
        (CBYTE*)"%s", hdr);
    y += PERF_LINE_PX;

    // Hotkey legend (these are the perf-panel keys; see handle_keys).
    FONT_buffer_add_scaled(PERF_PAD_PX, y, 120, 170, 170, 1, PERF_TEXT_SCALE,
        (CBYTE*)"keys: 4 show/hide  5 avg window");
    y += PERF_LINE_PX;
    y += PERF_LINE_PX; // blank line under the legend

    // Column titles. "ge_* CPU" = CPU wall spent INSIDE the graphics API
    // (driver/submit; never GPU execution, never GPU-wait — that's the
    // separate line after the total). "non-ge_* CPU" = CPU outside the
    // API (game logic + engine render-prep). "GPU" = real GPU execution
    // (per-scope glFinish). Each column normalised to its own 100%.
    FONT_buffer_add_scaled(PERF_PAD_PX, y, 150, 200, 230, 1, PERF_TEXT_SCALE,
        (CBYTE*)"stages");
    FONT_buffer_add_scaled(ge_col_x, y, 150, 200, 230, 1, PERF_TEXT_SCALE,
        (CBYTE*)"ge_* CPU");
    FONT_buffer_add_scaled(cpu_col_x, y, 150, 200, 230, 1, PERF_TEXT_SCALE,
        (CBYTE*)"non-ge_* CPU");
    FONT_buffer_add_scaled(gpu_col_x, y, 150, 200, 230, 1, PERF_TEXT_SCALE,
        (CBYTE*)"GPU");
    if (underline_n < (int)(sizeof(underline_y) / sizeof(underline_y[0])))
        underline_y[underline_n++] = y + PERF_GLYPH_H_PX;
    y += PERF_LINE_PX; // no blank under titles — header glued to its rows

    DrawCtx ctx;
    ctx.frame_ms  = frame_ms;
    ctx.cpu_total = cpu_total;
    ctx.ge_total  = ge_total;
    ctx.gpu_total = gpu_total;
    ctx.cpu_col_x = cpu_col_x;
    ctx.ge_col_x  = ge_col_x;
    ctx.gpu_col_x = gpu_col_x;
    ctx.max_y     = max_y;
    ctx.y         = y;
    ctx.uy        = underline_y;
    ctx.un        = &underline_n;
    ctx.ucap      = (int)(sizeof(underline_y) / sizeof(underline_y[0]));

    // Roots in frame call order — contiguous, NO blank lines between
    // level-1 groups (only ~2 levels; the spacing was clutter and made
    // the counters list blend in).
    double root_cpu_sum = 0.0;
    for (int o = 0; o < s_ordn; o++) {
        int i = s_ord[o];
        if (!is_root(i))
            continue;
        root_cpu_sum += s_slot[i].short_avg;
        emit_node(ctx, i);
    }

    // Top "(other)" = frame − Σ roots wall − GPU-wait − perfpanel:
    // unmeasured gaps (input, sfx, …) — all pure CPU. GPU-wait and the
    // perf-panel cost are subtracted here too (neither is in s_ord/roots,
    // so they'd otherwise leak into this remainder). Normalised by
    // cpu_total so the column sums to 100%.
    double other_ms = frame_ms - root_cpu_sum - gpu_wait_ms - perfpanel_ms;
    if (other_ms < 0.0) other_ms = 0.0;
    const double other_pct = other_ms / cpu_total * 100.0;
    emit_row(ctx, "(other)", false, 0.0, other_pct, 0.0, 0.0,
             other_ms, 0.0, 0.0);

    // Bottom "total" row: each column is its own 100% by construction —
    // show the absolute ms each column sums to (ge_* CPU real submit,
    // non-ge_* CPU, GPU) — GPU-wait NOT included. Highlighted colour so
    // it reads as a footer, not a stage.
    emit_row(ctx, "total", false, 0.0,
             cpu_total > 1e-6 ? 100.0 : 0.0,
             ge_total  > 1e-6 ? 100.0 : 0.0,
             gpu_total > 1e-6 ? 100.0 : 0.0,
             cpu_total, ge_total, gpu_total, /*is_total=*/true);

    // "split" row — AFTER total, name yellow like total. Distributes the
    // measured frame across the 3 columns: each column's total ms as a
    // share of (ge_total + cpu_total + gpu_total). Left→right the three
    // sum to 100%. Percentages load-graded red/white like the cells.
    if (ctx.y <= max_y) {
        const double split_sum = ge_total + cpu_total + gpu_total;
        const double inv = split_sum > 1e-6 ? 100.0 / split_sum : 0.0;
        const double ge_sp  = ge_total  * inv;
        const double cpu_sp = cpu_total * inv;
        const double gpu_sp = gpu_total * inv;
        UBYTE r, g, b;
        FONT_buffer_add_scaled(PERF_PAD_PX, ctx.y, PERF_TOTAL_R,
            PERF_TOTAL_G, PERF_TOTAL_B, 1, PERF_TEXT_SCALE,
            (CBYTE*)"split");
        pct_color(ge_sp, r, g, b);
        FONT_buffer_add_scaled(ctx.ge_col_x, ctx.y, r, g, b, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2f%%", ge_sp);
        pct_color(cpu_sp, r, g, b);
        FONT_buffer_add_scaled(ctx.cpu_col_x, ctx.y, r, g, b, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2f%%", cpu_sp);
        pct_color(gpu_sp, r, g, b);
        FONT_buffer_add_scaled(ctx.gpu_col_x, ctx.y, r, g, b, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2f%%", gpu_sp);
        if (underline_n < (int)(sizeof(underline_y) / sizeof(underline_y[0])))
            underline_y[underline_n++] = ctx.y + PERF_GLYPH_H_PX;
        ctx.y += PERF_LINE_PX;
    }

    // perf-panel cost line — AFTER total/split, next to GPU-wait. Pure
    // diagnostics overhead (this panel's own glyph queue + flush), kept
    // OUTSIDE every column's 100% (already subtracted from every stage's
    // wall + from cpu_total/(other)). ms-only, muted slate so it reads
    // as "not the game" and is distinct from the amber GPU-wait line.
    if (ctx.y <= max_y) {
        const UBYTE pr = 150, pg = 160, pb = 185; // muted slate
        FONT_buffer_add_scaled(PERF_PAD_PX, ctx.y, pr, pg, pb, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%s", PERF_PERFPANEL_NAME);
        // Honest 3-way split (ge_* draws / CPU glyph build / GPU drain),
        // ms only — no %, there's nothing to take a share OF (it's
        // outside every 100%). Σ of the three = the wall on the line.
        FONT_buffer_add_scaled(ctx.ge_col_x, ctx.y, pr, pg, pb, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2fms", perfpanel_ge);
        FONT_buffer_add_scaled(ctx.cpu_col_x, ctx.y, pr, pg, pb, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2fms", perfpanel_nonge);
        FONT_buffer_add_scaled(ctx.gpu_col_x, ctx.y, pr, pg, pb, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2fms", perfpanel_gpu);
        if (underline_n < (int)(sizeof(underline_y) / sizeof(underline_y[0])))
            underline_y[underline_n++] = ctx.y + PERF_GLYPH_H_PX;
        ctx.y += PERF_LINE_PX;
    }

    // glFinish-wait line — AFTER total/split, deliberately OUTSIDE every
    // column's 100% and with NO percentage (ms only, no parens). It's
    // the CPU blocked draining the GPU (graphics-side measurement cost);
    // kept off the percentages so they show only the real work. Muted
    // amber. Context for "why this FPS", not a thing to optimise.
    if (ctx.y <= max_y) {
        const UBYTE wr = 185, wg = 150, wb = 80; // muted amber
        FONT_buffer_add_scaled(PERF_PAD_PX, ctx.y, wr, wg, wb, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%s", PERF_GPU_WAIT_NAME);
        // It's a glFinish (a graphics-API call) → the real number lives
        // in the ge_* column. non-ge / GPU have no meaningful split here;
        // shown as artificial 00.00ms just so all 3 columns line up.
        FONT_buffer_add_scaled(ctx.ge_col_x, ctx.y, wr, wg, wb, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2fms", gpu_wait_ms);
        FONT_buffer_add_scaled(ctx.cpu_col_x, ctx.y, wr, wg, wb, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2fms", 0.0);
        FONT_buffer_add_scaled(ctx.gpu_col_x, ctx.y, wr, wg, wb, 1,
            PERF_TEXT_SCALE, (CBYTE*)"%05.2fms", 0.0);
        if (underline_n < (int)(sizeof(underline_y) / sizeof(underline_y[0])))
            underline_y[underline_n++] = ctx.y + PERF_GLYPH_H_PX;
        ctx.y += PERF_LINE_PX;
    }

    // ---- Counters: separate list (different shape — name + raw value,
    // no %, no CPU/GPU columns). drawcalls / physics.ticks / … live here.
    bool any_counter = false;
    for (int o = 0; o < s_ordn; o++) {
        const Slot* s = &s_slot[s_ord[o]];
        if (s->kind != KIND_VALUE)
            continue;
        if (!any_counter) {
            ctx.y += PERF_LINE_PX; // one blank line; heading makes it clear
            FONT_buffer_add_scaled(PERF_PAD_PX, ctx.y, 150, 200, 230, 1,
                PERF_TEXT_SCALE, (CBYTE*)"counters");
            if (underline_n < (int)(sizeof(underline_y) / sizeof(underline_y[0])))
                underline_y[underline_n++] = ctx.y + PERF_GLYPH_H_PX;
            ctx.y += PERF_LINE_PX; // no blank — heading glued to its rows
            any_counter = true;
        }
        emit_row(ctx, s->name, true, s->short_avg, 0.0, 0.0, 0.0,
                 0.0, 0.0, 0.0);
    }

    // ---- Pass 2: backdrop + uniform thin row underlines --------------
    FONT_atlas_fill_begin();
    FONT_atlas_fill_rect(0, 0, panel_w, screen_h, 0xE6000000u); // ~90% black
    for (int i = 0; i < underline_n; i++)
        FONT_atlas_fill_rect(0, underline_y[i], panel_w,
            PERF_TEXT_SCALE + 1, (ULONG)PERF_RULE_NORMAL);
    FONT_atlas_fill_end();

    FONT_buffer_select(FONT_QUEUE_MAIN);
}

#else // !OC_DEBUG_PERF — log-only build: panel is a no-op

void draw() {}

#endif // OC_DEBUG_PERF

} // namespace perf

#endif // OC_PERF_ACTIVE
