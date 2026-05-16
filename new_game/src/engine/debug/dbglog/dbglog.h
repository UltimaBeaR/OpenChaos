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

#if OC_DEBUG_LOG

void DBGLOG(const char* color, const char* fmt, ...);
void DBGLOG_clear(void);
// Render-pass draw (FONT2D). Call from inside a POLY/PANEL frame
// (OVERLAY_handle), NOT from the game tick.
void DBGLOG_draw(void);

#else

// Compile-time no-ops. Args are discarded unevaluated.
#define DBGLOG(...)    ((void)0)
#define DBGLOG_clear() ((void)0)
#define DBGLOG_draw()  ((void)0)

#endif // OC_DEBUG_LOG

#endif // ENGINE_DEBUG_DBGLOG_DBGLOG_H
