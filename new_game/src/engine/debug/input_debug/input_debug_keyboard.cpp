// Input debug panel — Keyboard page.
//
// Lists currently-held keys by human-readable name (with scancode in
// parens for completeness). A richer keymap picture with lit-up keys
// is a future iteration.

#include "engine/debug/input_debug/input_debug.h"

#include "engine/input/keyboard.h"

namespace {

constexpr SLONG CONTENT_Y = 48;
constexpr SLONG LINE_H = 12;
constexpr SLONG COL_X = 20;
constexpr SLONG COL_W = 110;
constexpr int MAX_ROWS = 20;

// Friendly name for a scancode. Mirrors the KB_* defines in keyboard.h.
// Extended keys live at scancode + 0x80 (cursor pad, numpad slash/enter,
// right-side modifiers) — handled by cases that explicitly add 0x80.
// Returns nullptr when no known name exists; caller falls back to the
// bare hex code.
const char* key_name(UBYTE sc)
{
    switch (sc) {
    case KKEY_ESCAPE:
        return "Esc";
    case KKEY_1:
        return "1";
    case KKEY_2:
        return "2";
    case KKEY_3:
        return "3";
    case KKEY_4:
        return "4";
    case KKEY_5:
        return "5";
    case KKEY_6:
        return "6";
    case KKEY_7:
        return "7";
    case KKEY_8:
        return "8";
    case KKEY_9:
        return "9";
    case KKEY_0:
        return "0";
    case KKEY_MINUS:
        return "-";
    case KKEY_EQUALS:
        return "=";
    case KKEY_BACKSPACE:
        return "Bksp";

    case KKEY_TAB:
        return "Tab";
    case KKEY_Q:
        return "Q";
    case KKEY_W:
        return "W";
    case KKEY_E:
        return "E";
    case KKEY_R:
        return "R";
    case KKEY_T:
        return "T";
    case KKEY_Y:
        return "Y";
    case KKEY_U:
        return "U";
    case KKEY_I:
        return "I";
    case KKEY_O:
        return "O";
    case KKEY_P:
        return "P";
    case KKEY_LEFT_BRACKET:
        return "[";
    case KKEY_RIGHT_BRACKET:
        return "]";
    case KKEY_ENTER:
        return "Enter";

    case KKEY_LEFT_CONTROL:
        return "L-Ctrl";
    case KKEY_A:
        return "A";
    case KKEY_S:
        return "S";
    case KKEY_D:
        return "D";
    case KKEY_F:
        return "F";
    case KKEY_G:
        return "G";
    case KKEY_H:
        return "H";
    case KKEY_J:
        return "J";
    case KKEY_K:
        return "K";
    case KKEY_L:
        return "L";
    case KKEY_SEMICOLON:
        return ";";
    case KKEY_APOSTROPHE:
        return "'";
    case KKEY_GRAVE:
        return "`";

    case KKEY_LEFT_SHIFT:
        return "L-Shift";
    case KKEY_BACKSLASH:
        return "\\";
    case KKEY_Z:
        return "Z";
    case KKEY_X:
        return "X";
    case KKEY_C:
        return "C";
    case KKEY_V:
        return "V";
    case KKEY_B:
        return "B";
    case KKEY_N:
        return "N";
    case KKEY_M:
        return "M";
    case KKEY_COMMA:
        return ",";
    case KKEY_PERIOD:
        return ".";
    case KKEY_SLASH:
        return "/";
    case KKEY_RIGHT_SHIFT:
        return "R-Shift";
    case KKEY_NUMPAD_ASTERISK:
        return "P*";
    case KKEY_LEFT_ALT:
        return "L-Alt";
    case KKEY_SPACE:
        return "Space";
    case KKEY_CAPS_LOCK:
        return "Caps";

    case KKEY_F1:
        return "F1";
    case KKEY_F2:
        return "F2";
    case KKEY_F3:
        return "F3";
    case KKEY_F4:
        return "F4";
    case KKEY_F5:
        return "F5";
    case KKEY_F6:
        return "F6";
    case KKEY_F7:
        return "F7";
    case KKEY_F8:
        return "F8";
    case KKEY_F9:
        return "F9";
    case KKEY_F10:
        return "F10";
    case KKEY_F11:
        return "F11";
    case KKEY_F12:
        return "F12";

    case KKEY_NUM_LOCK:
        return "NumLk";
    case KKEY_SCROLL_LOCK:
        return "ScrLk";

    case KKEY_NUMPAD_7:
        return "P7";
    case KKEY_NUMPAD_8:
        return "P8";
    case KKEY_NUMPAD_9:
        return "P9";
    case KKEY_NUMPAD_MINUS:
        return "P-";
    case KKEY_NUMPAD_4:
        return "P4";
    case KKEY_NUMPAD_5:
        return "P5";
    case KKEY_NUMPAD_6:
        return "P6";
    case KKEY_NUMPAD_PLUS:
        return "P+";
    case KKEY_NUMPAD_1:
        return "P1";
    case KKEY_NUMPAD_2:
        return "P2";
    case KKEY_NUMPAD_3:
        return "P3";
    case KKEY_NUMPAD_0:
        return "P0";
    case KKEY_NUMPAD_PERIOD:
        return "P.";

    // Extended keys (scancode + 0x80).
    case KKEY_RIGHT_CONTROL:
        return "R-Ctrl";
    case KKEY_RIGHT_ALT:
        return "R-Alt";
    case KKEY_PRINT_SCREEN:
        return "PrtSc";
    case KKEY_NUMPAD_SLASH:
        return "P/";
    case KKEY_NUMPAD_ENTER:
        return "P-Enter";

    case KKEY_INSERT:
        return "Ins";
    case KKEY_HOME:
        return "Home";
    case KKEY_PAGE_UP:
        return "PgUp";
    case KKEY_DELETE:
        return "Del";
    case KKEY_END:
        return "End";
    case KKEY_PAGE_DOWN:
        return "PgDn";

    case KKEY_LEFT:
        return "Left";
    case KKEY_UP:
        return "Up";
    case KKEY_RIGHT:
        return "Right";
    case KKEY_DOWN:
        return "Down";

    default:
        return nullptr;
    }
}

} // namespace

void input_debug_render_keyboard_page()
{
    // Active / inactive indication is already on the tab up top; page
    // body just focuses on its content. Live key list below gates on
    // input_debug_key_held which returns false when keyboard is not the
    // currently-active input source.
    input_debug_text(COL_X, CONTENT_Y, 255, 255, 255, 1,
        "Keyboard — currently-held keys");

    int col = 0;
    int row = 0;
    int count = 0;
    for (int sc = 0; sc < 256; sc++) {
        if (!input_debug_key_held((UBYTE)sc))
            continue;
        const SLONG x = COL_X + col * COL_W;
        const SLONG y = CONTENT_Y + LINE_H * (2 + row);
        const char* name = key_name((UBYTE)sc);
        if (name) {
            input_debug_text(x, y, 0, 255, 0, 1, "%-8s 0x%02X", name, sc);
        } else {
            input_debug_text(x, y, 0, 255, 0, 1, "?        0x%02X", sc);
        }
        count++;
        row++;
        if (row >= MAX_ROWS) {
            row = 0;
            col++;
        }
    }

    if (count == 0) {
        input_debug_text(COL_X, CONTENT_Y + LINE_H * 2, 120, 120, 120, 1,
            "(no keys held)");
    }
}
