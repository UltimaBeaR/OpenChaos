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
    // Active / inactive indication is already on the tab up top; page
    // body just focuses on its content. Live key list below gates on
    // input_debug_key_held which returns false when keyboard is not the
    // currently-active input source.
    input_debug_text(COL_X, CONTENT_Y, 255, 255, 255, 1,
        "Keyboard — currently-held keys (scancode)");

    input_debug_text(COL_X, CONTENT_Y + LINE_H, 140, 140, 140, 1,
        "A richer keymap view (names, remaps) comes in a later iteration.");

    int col = 0;
    int row = 0;
    int count = 0;
    for (int sc = 0; sc < 256; sc++) {
        if (!input_debug_key_held((UBYTE)sc)) continue;
        const SLONG x = COL_X + col * COL_W;
        const SLONG y = CONTENT_Y + LINE_H * (3 + row);
        input_debug_text(x, y, 0, 255, 0, 1, "0x%02X (%d)", sc, sc);
        count++;
        row++;
        if (row >= MAX_ROWS) { row = 0; col++; }
    }

    if (count == 0) {
        input_debug_text(COL_X, CONTENT_Y + LINE_H * 3, 120, 120, 120, 1,
            "(no keys held)");
    }
}
