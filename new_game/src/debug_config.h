#pragma once

// Compile-time debug toggles. Sibling to config.h — kept separate so the
// "main" config stays focused on user-facing tuning (resolution, vsync,
// FOV, render scale) while developer debug flags don't pollute it.
// Always keep these `false` in shipping builds.

// Paint the composition layer's outer pillar/letterbox bars dark red
// instead of black so the FBO boundary is visible during layout
// debugging.
#define OC_DEBUG_COMPOSITION_BARS_RED false

// Strongly tint + blur the scene during composition so the UI/scene
// split is visible at a glance — anything touched by the composition
// pass (= the 3D scene and any UI that's still living in the scene FBO)
// comes out magenta and blurry; UI that's drawn in the post-composition
// pass stays sharp and original colours. Used to audit which call sites
// still need to migrate.
#define OC_DEBUG_HIGHLIGHT_NON_UI false

// Silence all sound/music by forcing the OpenAL listener gain to 0 right
// after init. OpenAL itself stays fully operational (skipping init
// caused busy AL_INVALID_OPERATION spin in update paths and tanked FPS)
// — this is the cheap mute knob.
#define OC_DEBUG_SOUND_DISABLED false

// Physics / render timing debug overlay and hotkeys.
//
// When true, enables:
//   Overlay (top-left, yellow text, always visible during gameplay):
//     "phys: <hz>  lock: <fps|unlim>  IP: <on|off>  fps: <N.N>"
//     "ticks/frame: <N>"  — shown in orange/red only when > 1 tick/frame
//
//   Hotkeys (no bangunsnotgames required, gameplay only):
//     1              — toggle physics Hz between 20 Hz (design) and 5 Hz (slow-mo debug)
//     9 / 0          — fine-tune physics Hz ±1 (range 1..20)
//     2              — toggle render FPS cap: unlimited ↔ 30 fps
//     DualSense touchpad click — same as key 2 (render cap toggle)
//     3              — toggle render interpolation on/off
//
// When false: no overlay, no hotkeys, no runtime state changes possible.
// Always keep false in shipping builds.
#define OC_DEBUG_PHYSICS_TIMING false

// Built-in performance diagnostics (engine/debug/perf_diag). When false,
// PERF_SCOPE / PERF_COUNT / perf_frame_* / perf_diag_draw become
// compile-time no-ops (arguments not evaluated, no symbols emitted) — zero
// cost even with call sites left in the code. When true: per-stage
// wall-clock timers + value counters accumulate every frame, a left-side
// panel (dark backdrop + text, mirrors dbglog but on the left, covers the
// HUD). Fixed top block = FPS + averaging window + the general "frame.*"
// stats. Below it the per-stage metrics, grouped by first-level name with
// blank lines between groups and the group "*.total" pinned at the
// bottom of its group; each row underlined to tie the far-apart name and
// value together. The value column shows % of frame time, colour-graded
// white→red (nonlinear) by that %. Hotkeys (gameplay, no
// bangunsnotgames): KKEY_4 toggle panel, KKEY_5 cycle averaging window, KKEY_6
// reset history. The MASTER switch for the perf-diag panel and
// instrumentation. A dev-only build flag; keep false in shipping builds.
//
// ⚠️ To understand what each line measures, the hotkeys, the colours and
// the file log — READ new_game_devlog/perf_diag/panel_guide.md (usage
// manual). Architecture/history: new_game_devlog/perf_diag/design.md.
#define OC_DEBUG_PERF false

// Generic on-screen debug log (engine/debug/dbglog). When false, DBGLOG /
// DBGLOG_draw / DBGLOG_clear become compile-time no-ops (arguments are
// not even evaluated) — zero cost, nothing drawn, even if call sites
// remain in the code. When true the log writes and draws unconditionally
// — this flag is the ONLY gate (no bangunsnotgames / runtime toggle).
// A dev-only build flag; keep false in shipping builds.
//
// This is the MASTER switch for the shared log infrastructure. The log
// itself is generic and meant to be reused by many subsystems. To avoid
// flooding the panel with every instrumented subsystem at once, each
// subsystem ALSO has its own compile-time gate below: a call site is
// compiled in only when BOTH the master flag and that subsystem's flag
// are true. So while debugging one system you turn on just its flag and
// everything else stays a stripped no-op. Add a new OC_DEBUG_LOG_<SYS>
// flag here per subsystem as it gets instrumented.
#define OC_DEBUG_LOG false

// Input-prompt glyph catalog: shows the "INPUT TEST" item in the pause-menu
// "Controls" list — an auto-generated, device-aware list of every mapped
// button glyph (input_prompt_map.cpp). For visually checking the glyph map when
// bindings/atlases change. When false the item is hidden from the list (players
// never see it); the page itself stays in the build. Dev-only; keep false in
// shipping builds.
#define OC_DEBUG_INPUT_PROMPT_CATALOG false

// --- Per-subsystem debug-log gates (see note above) ---------------------

// Melee combat tuning log: combo proc/pity rolls, the per-action
// cooldowns (arrest/grapple/block/roll), block arm-latch, grapple
// throw-to-ground roll, behind-stun gate. Turn on ONLY while debugging
// combat; off by default so unrelated debugging sessions aren't flooded
// with combat lines. (Combat work is done — leave false until needed.)
#define OC_DEBUG_LOG_COMBAT false
