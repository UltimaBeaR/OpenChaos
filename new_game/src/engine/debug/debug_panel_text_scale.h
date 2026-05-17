#ifndef ENGINE_DEBUG_DEBUG_PANEL_TEXT_SCALE_H
#define ENGINE_DEBUG_DEBUG_PANEL_TEXT_SCALE_H

// Fixed code constant — integer pixel multiplier for the on-screen debug
// panels (perf_diag + dbglog). It lives here, shared, only so the two
// panels render at the SAME size and can't drift apart. This is NOT a
// dev/user toggle (that's why it is not in debug_config.h) — it's a
// hardcoded layout choice; change the number here only if the debug
// panels ever need a different font size. 1 = original tiny 5×9 font,
// 2 = chunky 2× (readable on a Steam Deck / from a couch).
static constexpr int DBG_PANEL_TEXT_SCALE = 2;

#endif // ENGINE_DEBUG_DEBUG_PANEL_TEXT_SCALE_H
