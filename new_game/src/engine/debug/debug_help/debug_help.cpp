// Debug hotkey legend overlay — see debug_help.h.

#include "engine/debug/debug_help/debug_help.h"

#include "engine/graphics/text/font.h"
#include "engine/input/keyboard.h" // KKEY_F1
#include "engine/input/input_frame.h"
#include "game/action_map/act_bangunsnotgames.h" // ACT_BANG_SHOW_LEGEND_KKEY

#include <chrono>

extern BOOL allow_debug_keys;

namespace {

std::chrono::steady_clock::time_point s_hide_at;
bool s_visible = false;

// One row per line. First column = key, second = description.
//
// F1-modifier scheme: ALL keys below are read as "F1 + key" — hold F1 to
// enable debug input AND simultaneously suppress gameplay-input (WASD,
// jump, etc.) so the two don't fight. F1 alone (no other key) shows this
// help for 5 seconds; as soon as F1 + another key is pressed the overlay
// auto-hides and the action fires. F9 (open console) is the only debug-
// adjacent key that works without F1.
struct Row {
    const char* key;
    const char* desc;
};
const Row s_rows[] = {
    // Meta — F1 alone shows this help (5s). F1 is also the global debug
    // modifier (held while pressing any debug key below — the overlay
    // auto-hides on the first F1+key combo). F9 (open console) isn't a
    // debug-mode hotkey — omitted here intentionally.
    { "F1", "hold = debug modifier; alone = show this help (5s)" },
    // Function keys (under F2)
    { "F1+F3", "load game   (Shift+F3 save, poo.sav)" },
    { "F1+F4", "toggle clouds" },
    { "F1+F2", "toggle CRT scanline shader" },
    { "F1+F8", "toggle single-step mode" },
    { "F1+F10", "farfacet debug split view" },
    { "F1+F11", "input debug panel" },
    { "F1+F12", "spawn weapons around player + heal" },
    { "F1+Shift+F12", "cheat toggle (prints FPS)" },
    { "F1+Insert", "step once (while single-step)" },
    { "F1+Ctrl", "lock debug overlay (force-visible)" },
    { "F1+Ctrl+L", "toggle outside / ambient lighting" },
    { "F1+Ctrl+Q", "quit" },
    // Symbols
    { "F1+~", "toggle detail level (low / high)" },
    { "F1+;", "toggle slow-motion" },
    { "F1+=", "combat test: +1 enemy (on)" },
    { "F1+-", "combat test: -1 enemy (0 = off)" },
    { "F1+\\", "combat test: cycle armament tier" },
    { "F1+/", "toggle stealth debug" },
    // Player / world
    { "F1+B", "toggle skeleton overlay (bone lines + balls)" },
    { "F1+L", "toggle dynamic light at player" },
    { "F1+N", "cycle Darci's visual model (15 person types)" },
    { "F1+T", "warehouse debug (doors, MAV arrows)" },
    { "F1+V", "show version string" },
    { "F1+P", "save game to save.me (+ camera-2 follow toggle)" },
    { "F1+S", "screenshot (Shift+S toggles record)" },
    { "F1+D", "room-behind-check (Shift+D: debugger trap)" },
    // Camera 2 (splitscreen follow) — bracket keys spelled out because
    // the 5x7 pixel font has no glyphs for '[' and ']' (renders as '?').
    { "F1+Lbrack", "camera-2: focus previous person" },
    { "F1+Rbrack", "camera-2: focus next person" },
    // Spawning (no Shift)
    { "F1+O", "spawn random object in front of Darci" },
    { "F1+E", "spawn vehicle (cycles VEH types)" },
    { "F1+G", "teleport Darci to mouse cursor" },
    // Spawning / cheats (Shift)
    { "F1+Shift+A", "spawn fight-test thug (cycles skill)" },
    { "F1+Shift+I", "spawn bodyguard following Darci" },
    { "F1+Shift+J", "Darci: random dance + sit on bench" },
    { "F1+Shift+G", "give Darci a gun (FLAGS_HAS_GUN)" },
    { "F1+Shift+H", "save top-down map shot (plan_view TGA)" },
    { "F1+Shift+O", "spawn civilian chopper above Darci" },
    { "F1+Shift+M", "spawn mine at mouse cursor" },
    // Continuous-while-held overlays / effects
    { "F1+J  (hold)", "MAV navigation overlay around Darci" },
    { "F1+I  (hold)", "WAND navigation overlay around Darci" },
    { "F1+W  (hold)", "spawn water particles in front of Darci" },
    { "F1+.  (hold)", "spawn smoke particles above Darci" },
    { "F1+U  (hold)", "barrel-hit sphere damage at Darci" },
    { "F1+Q  (hold)", "ROAD_debug overlay" },
    { "F1+Shift+Y (h)", "fastnav debug overlay at Darci" },
    // Pyro test (moved off numpad — perf-debug compile-flag owns numpad)
    { "F1+End", "cycle pyro type for spawn key" },
    { "F1+K", "spawn current pyro near Darci" },
    { "F1+Delete", "immolate chopper 1" },
    { "F1+R", "place / connect fire-pool line marker" },
    // Message buffer scroll (moved off numpad)
    { "F1+PageUp", "msg-buffer scroll up" },
    { "F1+PageDown", "msg-buffer scroll down" },
    { "F1+Home", "msg-buffer scroll reset" },
};
constexpr int s_row_count = sizeof(s_rows) / sizeof(s_rows[0]);

} // namespace

void debug_help_show(float seconds)
{
    const auto ms = std::chrono::milliseconds(static_cast<long long>(seconds * 1000.0f));
    s_hide_at = std::chrono::steady_clock::now() + ms;
    s_visible = true;
}

void debug_help_notify_bangunsnotgames_on()
{
    debug_help_show(5.0f);
}

void debug_help_tick()
{
    // F1 is the universal "remind me what keys exist" binding AND the
    // global debug modifier (see input_frame::input_debug_modifier_active).
    // Gated behind allow_debug_keys so the legend — and therefore the
    // existence of the debug mode — stays hidden from regular players.
    if (allow_debug_keys && input_key_just_pressed(ACT_BANG_SHOW_LEGEND_KKEY)) {
        debug_help_show(5.0f);
    }

    // Auto-hide the legend immediately once the dev presses a non-F1 key
    // while F1 is held — they're using F1 as a modifier now, no need for
    // the help overlay to stick around. Uses s_last_key (most recent
    // scancode pressed in this frame); if it differs from F1, something
    // else fired and the dev has moved past discovery.
    if (s_visible && input_key_held(ACT_BANG_SHOW_LEGEND_KKEY)) {
        const UBYTE last = input_last_key();
        if (last != 0 && last != ACT_BANG_SHOW_LEGEND_KKEY) {
            s_visible = false;
        }
    }

    if (s_visible && std::chrono::steady_clock::now() >= s_hide_at) {
        s_visible = false;
    }
}

void debug_help_render()
{
    if (!s_visible)
        return;

    // Draw in literal window pixels via FONT_buffer_add. The legend
    // lives in the top-left corner where pixel-coord text works fine —
    // it's always in the same place regardless of window size. Logical
    // 640×480 scaling would drift the legend across wider windows.
    constexpr int line_h = 10;
    constexpr int header_x = 10;
    constexpr int header_y = 40; // below the tab bar if input panel opens later
    constexpr int rows_x = 10;
    constexpr int rows_y = header_y + line_h * 2;
    constexpr int col_split = 90; // key column width in pixels

    FONT_buffer_add(header_x, header_y, 255, 255, 0, 1,
        (CBYTE*)"DEBUG MODE  (bangunsnotgames)");

    for (int i = 0; i < s_row_count; ++i) {
        const int y = rows_y + i * line_h;
        FONT_buffer_add(rows_x, y, 120, 220, 255, 1, (CBYTE*)"%s", s_rows[i].key);
        FONT_buffer_add(rows_x + col_split, y, 220, 220, 220, 1, (CBYTE*)"%s", s_rows[i].desc);
    }
}
