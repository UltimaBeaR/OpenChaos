// Input debug panel — Keyboard page (stub).
//
// Placeholder: lists currently-held keys by scancode (hex + decimal).
// A richer layout — keymap picture with lit-up keys, device info line,
// remap inspector — comes in a later iteration.

#include "engine/debug/input_debug/input_debug.h"

#include "engine/graphics/text/font.h"
#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"
#include "engine/input/keyboard_globals.h"

#include <cstdio>

namespace {

constexpr SLONG CONTENT_Y = 48;
constexpr SLONG LINE_H    = 12;
constexpr SLONG COL_X     = 20;
constexpr SLONG COL_W     = 90;
constexpr int   MAX_ROWS  = 20;

} // namespace

void input_debug_render_keyboard_page()
{
    // Only the currently-active input source renders live state — prevents
    // confusion when presses on an inactive device show up on its page.
    if (active_input_device != INPUT_DEVICE_KEYBOARD_MOUSE) {
        FONT_buffer_add(COL_X, CONTENT_Y, 200, 140, 140, 1,
            (CBYTE*)"Keyboard not active.");
        FONT_buffer_add(COL_X, CONTENT_Y + LINE_H, 150, 150, 150, 1,
            (CBYTE*)"Press any key to activate — live state will appear here.");
        return;
    }

    FONT_buffer_add(COL_X, CONTENT_Y, 255, 255, 255, 1,
        (CBYTE*)"Keyboard — currently-held keys (scancode)");

    FONT_buffer_add(COL_X, CONTENT_Y + LINE_H, 140, 140, 140, 1,
        (CBYTE*)"A richer keymap view (names, remaps) comes in a later iteration.");

    int col = 0;
    int row = 0;
    int count = 0;
    for (int sc = 0; sc < 256; sc++) {
        if (!Keys[sc]) continue;
        const SLONG x = COL_X + col * COL_W;
        const SLONG y = CONTENT_Y + LINE_H * (3 + row);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "0x%02X (%d)", sc, sc);
        FONT_buffer_add(x, y, 0, 255, 0, 1, (CBYTE*)"%s", buf);
        count++;
        row++;
        if (row >= MAX_ROWS) { row = 0; col++; }
    }

    if (count == 0) {
        FONT_buffer_add(COL_X, CONTENT_Y + LINE_H * 3, 120, 120, 120, 1,
            (CBYTE*)"(no keys held)");
    }
}
