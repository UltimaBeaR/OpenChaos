#ifndef ENGINE_DEBUG_PERF_DIAG_PERF_DIAG_H
#define ENGINE_DEBUG_PERF_DIAG_PERF_DIAG_H

#include "debug_config.h" // OC_DEBUG_PERF / OC_DEBUG_PERF_LOG

// Built-in performance diagnostics (OpenChaos addition — not in the
// original). Per-stage wall-clock timers and value counters, windowed
// averaging, a left-side on-screen panel, sharp-change colour
// highlighting + peak-hold, and an optional averaged CSV file log.
//
// Goal: catch FLOATING perf regressions that the FPS number alone hides —
// one stage doubling its time while FPS barely dips. Many granular
// metrics + red-tinting of whatever deviated sharply makes the culprit
// pop out of the corner of your eye.
//
// Gating: the instrumentation/macros are compiled in when EITHER
// OC_DEBUG_PERF or OC_DEBUG_PERF_LOG is true (logging needs the same
// probes; the log flag alone exists to capture the load phase from
// process start). The panel + hotkeys are OC_DEBUG_PERF only; the file
// log is OC_DEBUG_PERF_LOG only. When both flags are false EVERY entry
// point below is a preprocessor no-op — arguments are not evaluated, no
// symbols emitted, zero cost; safe to leave PERF_* calls in shipping
// code. Dev-only build flags, kept false in shipping builds. Design:
// new_game_devlog/perf_diag/design.md.
//
// Adding a metric is one line anywhere — the name auto-registers a slot
// on first execution (string path, dotted for grouping):
//
//   void render_shadows() {
//       PERF_SCOPE("render.shadows");   // RAII timer, summed per frame
//       ...
//   }
//   PERF_COUNT("render.drawcalls", n);  // value sample for this frame
//
// PERF_SCOPE accumulates elapsed time of its block into the named slot
// (a slot hit several times in a frame sums). PERF_COUNT records a value
// for the slot this frame (repeated calls in a frame sum, e.g. counting).

#define OC_PERF_ACTIVE (OC_DEBUG_PERF || OC_DEBUG_PERF_LOG)

#if OC_PERF_ACTIVE

namespace perf {

enum SlotKind { KIND_TIME = 0, KIND_VALUE = 1 };

struct Slot; // opaque

// Find-or-create a slot by name (linear, but a macro caches the pointer
// in a static local so this runs once per call site).
Slot* get_slot(const char* name, SlotKind kind);

// Add to a slot's current-frame accumulator (ms for KIND_TIME via the
// scope timer; raw value for KIND_VALUE).
void add_value(Slot* s, double v);

// RAII wall-clock timer; adds its lifetime (ms) to the slot on scope
// exit. Uses the high-res performance counter.
struct ScopeTimer {
    Slot* slot;
    unsigned long long t0;
    explicit ScopeTimer(Slot* s);
    ~ScopeTimer();
    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;
};

// Frame boundary. begin: mark frame start. end: close "frame.total",
// roll every slot's accumulator into its history ring, recompute
// short/long averages + colour/peak-hold, emit a CSV row if due, reset
// accumulators. Both no-op-safe to call even before any slot exists.
void frame_begin();
void frame_end();

// Hotkey handling (OC_DEBUG_PERF only): KB_4 toggle panel, KB_5 cycle
// averaging window, KB_6 reset history/baseline. No-op when only the log
// flag is set.
void handle_keys();

// Render the panel (OC_DEBUG_PERF only). Call from the top UI layer
// (ui_render, next to DBGLOG_draw) — own PANEL batch, left-aligned.
void draw();

} // namespace perf

#define OC_PERF_CONCAT_(a, b) a##b
#define OC_PERF_CONCAT(a, b) OC_PERF_CONCAT_(a, b)

#define PERF_SCOPE(name)                                                       \
    static ::perf::Slot* OC_PERF_CONCAT(perf_slot_, __LINE__) =                 \
        ::perf::get_slot((name), ::perf::KIND_TIME);                            \
    ::perf::ScopeTimer OC_PERF_CONCAT(perf_timer_, __LINE__)(                   \
        OC_PERF_CONCAT(perf_slot_, __LINE__))

#define PERF_COUNT(name, val)                                                  \
    do {                                                                       \
        static ::perf::Slot* OC_PERF_CONCAT(perf_cslot_, __LINE__) =            \
            ::perf::get_slot((name), ::perf::KIND_VALUE);                       \
        ::perf::add_value(OC_PERF_CONCAT(perf_cslot_, __LINE__),                \
                          (double)(val));                                      \
    } while (0)

#define PERF_FRAME_BEGIN() ::perf::frame_begin()
#define PERF_FRAME_END()   ::perf::frame_end()
#define PERF_HANDLE_KEYS() ::perf::handle_keys()
#define PERF_DRAW()        ::perf::draw()

#else // !OC_PERF_ACTIVE — compile-time no-ops (args unevaluated)

#define PERF_SCOPE(name)      ((void)0)
#define PERF_COUNT(name, val) ((void)0)
#define PERF_FRAME_BEGIN()    ((void)0)
#define PERF_FRAME_END()      ((void)0)
#define PERF_HANDLE_KEYS()    ((void)0)
#define PERF_DRAW()           ((void)0)

#endif // OC_PERF_ACTIVE

#endif // ENGINE_DEBUG_PERF_DIAG_PERF_DIAG_H
