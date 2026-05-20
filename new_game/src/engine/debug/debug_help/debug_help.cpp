// Debug hotkey legend overlay — see debug_help.h.

#include "engine/debug/debug_help/debug_help.h"

#include "engine/graphics/text/font.h"
#include "engine/input/keyboard.h" // KB_F1
#include "engine/input/input_frame.h"

#include <chrono>

extern BOOL allow_debug_keys;

namespace {

std::chrono::steady_clock::time_point s_hide_at;
bool s_visible = false;

// One row per line. First column = key, second = description.
struct Row {
    const char* key;
    const char* desc;
};
const Row s_rows[] = {
    // Function keys & misc
    { "F1", "show this help (5s)" },
    { "F2", "toggle CRT scanline shader" },
    { "F3", "load game   (Shift+F3 save, poo.sav)" },
    { "F4", "toggle clouds" },
    { "F8", "toggle single-step mode" },
    { "F9", "console (type bangunsnotgames to exit)" },
    { "F10", "farfacet debug split view" },
    { "F11", "input debug panel" },
    { "F12", "spawn weapons around player + heal" },
    { "Shift+F12", "cheat toggle (prints FPS)" },
    { "Insert", "step once (while single-step)" },
    { "Ctrl", "lock debug overlay (force-visible)" },
    { "Ctrl+L", "toggle outside / ambient lighting" },
    { "Ctrl+Q", "quit" },
    { "~", "toggle detail level (low / high)" },
    { ";", "toggle slow-motion" },
    // Combat test (combat_test_mode.cpp)
    { "=", "combat test: +1 enemy (on)" },
    { "-", "combat test: -1 enemy (0 = off)" },
    { "\\", "combat test: cycle armament tier" },
    // Player / world
    { "B", "toggle skeleton overlay (bone lines + balls)" },
    { "L", "toggle dynamic light at player" },
    { "N", "cycle Darci's visual model (15 person types)" },
    { "T", "warehouse debug (doors, MAV arrows)" },
    { "V", "show version string" },
    { "P", "save game to save.me (+ also: camera-2 follow toggle)" },
    { "/", "toggle stealth debug" },
    // Camera 2 (splitscreen follow) — bracket keys spelled out because
    // the 5x7 pixel font has no glyphs for '[' and ']' (renders as '?').
    { "Lbrack", "camera-2: focus previous person" },
    { "Rbrack", "camera-2: focus next person" },
    // Spawning (no Shift)
    { "O", "spawn random object in front of Darci" },
    { "E", "spawn vehicle (cycles VEH types)" },
    { "G", "teleport Darci to mouse cursor" },
    // Spawning / cheats (Shift)
    { "Shift+A", "spawn fight-test thug (cycles skill)" },
    { "Shift+I", "spawn bodyguard following Darci" },
    { "Shift+J", "Darci: random dance + sit on bench" },
    { "Shift+G", "give Darci a gun (FLAGS_HAS_GUN)" },
    { "Shift+H", "save top-down map shot (plan_view TGA)" },
    { "Shift+O", "spawn civilian chopper above Darci" },
    { "Shift+M", "spawn mine at mouse cursor" },
    { "Shift+D", "debugger trap (ASSERT 2+2==5)" },
    // Continuous-while-held overlays / effects
    { "J  (hold)", "MAV navigation overlay around Darci" },
    { "I  (hold)", "WAND navigation overlay around Darci" },
    { "W  (hold)", "spawn water particles in front of Darci" },
    { ".  (hold)", "spawn smoke particles above Darci" },
    { "U  (hold)", "barrel-hit sphere damage at Darci" },
    { "Q  (hold)", "ROAD_debug overlay" },
    { "Shift+Y (h)", "fastnav debug overlay at Darci" },
    // Pyro test (numpad)
    { "Num7", "cycle pyro type for Num5" },
    { "Num5", "spawn current pyro near Darci" },
    { "Num2", "immolate chopper 1" },
    { "Num3", "place / connect fire-pool line marker" },
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
    // F1 is the universal "remind me what keys exist" binding. Gated
    // behind allow_debug_keys so the legend — and therefore the
    // existence of the debug mode — stays hidden from regular players.
    if (allow_debug_keys && input_key_just_pressed(KB_F1)) {
        debug_help_show(5.0f);
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
