#ifndef ENGINE_DEBUG_DBGLOG_DBGLOG_H
#define ENGINE_DEBUG_DBGLOG_DBGLOG_H

#include "debug_config.h" // OC_DEBUG_LOG

// Generic on-screen debug log (OpenChaos addition — not in the original).
//
// Append lines from anywhere in game code; they render as a scrolling
// panel on the RIGHT of the screen (the left side is the HUD), top to
// bottom. Reusable for any subsystem debugging — currently used by the
// melee combat tuning, but intentionally generic.
//
// Gating: ONLY the compile-time flag OC_DEBUG_LOG (debug_config.h).
// When false the three entry points below are preprocessor no-ops —
// arguments are not even evaluated, no symbols emitted, zero cost; safe
// to leave DBGLOG(...) calls sprinkled in shipping code. When true the
// log writes and draws unconditionally (no bangunsnotgames / no toggle
// key). It's a dev-only build flag, kept false in shipping builds.
//
// color: "red" "green" "white" "yellow" "grey" "cyan" "orange"
//        (unknown name -> white). printf-style: DBGLOG(color, fmt, ...).
//
// Multi-colour line: one visible line can be split into several
// independently-coloured pieces laid out in fixed columns:
//
//     DBGLOG_begin();
//     DBGLOG_seg(DBGLOG_color("white"), "%s", name);
//     DBGLOG_seg(my_rgb,                "%d%%", pct);
//     DBGLOG_seg(DBGLOG_color("green"), "SUCCESS");
//     DBGLOG_commit();
//
// Segments flow consecutively (each starts where the previous ended) —
// the log adds NO spacing of its own. Do column alignment yourself with
// spaces or '\t' in the segment text (tabs snap to a fixed grid, so
// padding the end of a field with a tab lines columns up across lines,
// terminal-style). DBGLOG_seg takes a packed 0xRRGGBB value;
// DBGLOG_color(name) converts a colour name to that value (so callers
// can also pass a computed gradient colour). DBGLOG(...) is unchanged
// and is just a one-segment line.

#if OC_DEBUG_LOG

void DBGLOG(const char* color, const char* fmt, ...);

// 0xRRGGBB for a colour name (unknown -> white). Usable as a DBGLOG_seg
// colour argument alongside caller-computed RGB values.
unsigned long DBGLOG_color(const char* name);

// Build a multi-segment line: begin, one or more seg, then commit.
void DBGLOG_begin(void);
void DBGLOG_seg(unsigned long rgb, const char* fmt, ...);
void DBGLOG_commit(void);

void DBGLOG_clear(void);
// Render-pass draw (FONT2D). Call from inside a POLY/PANEL frame
// (OVERLAY_handle), NOT from the game tick.
void DBGLOG_draw(void);

#else

// Compile-time no-ops. Args are discarded unevaluated.
#define DBGLOG(...)      ((void)0)
#define DBGLOG_color(...) (0ul)
#define DBGLOG_begin()   ((void)0)
#define DBGLOG_seg(...)  ((void)0)
#define DBGLOG_commit()  ((void)0)
#define DBGLOG_clear()   ((void)0)
#define DBGLOG_draw()    ((void)0)

#endif // OC_DEBUG_LOG

#endif // ENGINE_DEBUG_DBGLOG_DBGLOG_H
