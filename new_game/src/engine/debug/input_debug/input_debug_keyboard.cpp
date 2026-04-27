// Input debug panel — Keyboard page.
//
// Lists currently-held keys by human-readable name (with scancode in
// parens for completeness). A richer keymap picture with lit-up keys
// is a future iteration.

#include "engine/debug/input_debug/input_debug.h"

#include "engine/input/keyboard.h"

namespace {

constexpr SLONG CONTENT_Y = 48;
constexpr SLONG LINE_H    = 12;
constexpr SLONG COL_X     = 20;
constexpr SLONG COL_W     = 110;
constexpr int   MAX_ROWS  = 20;

// Friendly name for a scancode. Mirrors the KB_* defines in keyboard.h.
// Extended keys live at scancode + 0x80 (cursor pad, numpad slash/enter,
// right-side modifiers) — handled by cases that explicitly add 0x80.
// Returns nullptr when no known name exists; caller falls back to the
// bare hex code.
const char* key_name(UBYTE sc)
{
    switch (sc) {
    case KB_ESC:        return "Esc";
    case KB_1:          return "1";
    case KB_2:          return "2";
    case KB_3:          return "3";
    case KB_4:          return "4";
    case KB_5:          return "5";
    case KB_6:          return "6";
    case KB_7:          return "7";
    case KB_8:          return "8";
    case KB_9:          return "9";
    case KB_0:          return "0";
    case KB_MINUS:      return "-";
    case KB_PLUS:       return "=";
    case KB_BS:         return "Bksp";

    case KB_TAB:        return "Tab";
    case KB_Q:          return "Q";
    case KB_W:          return "W";
    case KB_E:          return "E";
    case KB_R:          return "R";
    case KB_T:          return "T";
    case KB_Y:          return "Y";
    case KB_U:          return "U";
    case KB_I:          return "I";
    case KB_O:          return "O";
    case KB_P:          return "P";
    case KB_LBRACE:     return "[";
    case KB_RBRACE:     return "]";
    case KB_ENTER:      return "Enter";

    case KB_LCONTROL:   return "L-Ctrl";
    case KB_A:          return "A";
    case KB_S:          return "S";
    case KB_D:          return "D";
    case KB_F:          return "F";
    case KB_G:          return "G";
    case KB_H:          return "H";
    case KB_J:          return "J";
    case KB_K:          return "K";
    case KB_L:          return "L";
    case KB_COLON:      return ";";
    case KB_QUOTE:      return "'";
    case KB_TILD:       return "`";

    case KB_LSHIFT:     return "L-Shift";
    case KB_BACKSLASH:  return "\\";
    case KB_Z:          return "Z";
    case KB_X:          return "X";
    case KB_C:          return "C";
    case KB_V:          return "V";
    case KB_B:          return "B";
    case KB_N:          return "N";
    case KB_M:          return "M";
    case KB_COMMA:      return ",";
    case KB_POINT:      return ".";
    case KB_FORESLASH:  return "/";
    case KB_RSHIFT:     return "R-Shift";
    case KB_ASTERISK:   return "P*";
    case KB_LALT:       return "L-Alt";
    case KB_SPACE:      return "Space";
    case KB_CAPSLOCK:   return "Caps";

    case KB_F1:         return "F1";
    case KB_F2:         return "F2";
    case KB_F3:         return "F3";
    case KB_F4:         return "F4";
    case KB_F5:         return "F5";
    case KB_F6:         return "F6";
    case KB_F7:         return "F7";
    case KB_F8:         return "F8";
    case KB_F9:         return "F9";
    case KB_F10:        return "F10";
    case KB_F11:        return "F11";
    case KB_F12:        return "F12";

    case KB_NUMLOCK:    return "NumLk";
    case KB_SCROLLLOCK: return "ScrLk";

    case KB_P7:         return "P7";
    case KB_P8:         return "P8";
    case KB_P9:         return "P9";
    case KB_PMINUS:     return "P-";
    case KB_P4:         return "P4";
    case KB_P5:         return "P5";
    case KB_P6:         return "P6";
    case KB_PPLUS:      return "P+";
    case KB_P1:         return "P1";
    case KB_P2:         return "P2";
    case KB_P3:         return "P3";
    case KB_P0:         return "P0";
    case KB_PPOINT:     return "P.";

    // Extended keys (scancode + 0x80).
    case KB_RCONTROL:   return "R-Ctrl";
    case KB_RALT:       return "R-Alt";
    case KB_PRTSC:      return "PrtSc";
    case KB_PSLASH:     return "P/";
    case KB_PENTER:     return "P-Enter";

    case KB_INS:        return "Ins";
    case KB_HOME:       return "Home";
    case KB_PGUP:       return "PgUp";
    case KB_DEL:        return "Del";
    case KB_END:        return "End";
    case KB_PGDN:       return "PgDn";

    case KB_LEFT:       return "Left";
    case KB_UP:         return "Up";
    case KB_RIGHT:      return "Right";
    case KB_DOWN:       return "Down";

    default:            return nullptr;
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
        if (!input_debug_key_held((UBYTE)sc)) continue;
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
        if (row >= MAX_ROWS) { row = 0; col++; }
    }

    if (count == 0) {
        input_debug_text(COL_X, CONTENT_Y + LINE_H * 2, 120, 120, 120, 1,
            "(no keys held)");
    }
}
