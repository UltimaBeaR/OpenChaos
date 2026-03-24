#ifndef OUTRO_CORE_OUTRO_KEY_H
#define OUTRO_CORE_OUTRO_KEY_H

// Keyboard state and scancode constants for the outro/credits sequence.
// The outro has its own keyboard handling that does not share the main
// game's input system.

#include "outro/core/outro_always.h"

// Keyboard state is defined in outro_os.cpp and declared in outro_os_globals.h:
//   KEY_on[256], KEY_inkey, KEY_shift
// Include outro_os.h if you need both state and scancodes.

// Bitmask values for KEY_shift.
// uc_orig: KEY_SHIFT (fallen/outro/key.h)
#define KEY_SHIFT   (1 << 0)
// uc_orig: KEY_ALT (fallen/outro/key.h)
#define KEY_ALT     (1 << 1)
// uc_orig: KEY_CONTROL (fallen/outro/key.h)
#define KEY_CONTROL (1 << 2)

// Keyboard scancodes.
// uc_orig: KEY_ESCAPE (fallen/outro/key.h)
#define KEY_ESCAPE    0x01
// uc_orig: KEY_1 (fallen/outro/key.h)
#define KEY_1         0x02
// uc_orig: KEY_2 (fallen/outro/key.h)
#define KEY_2         0x03
// uc_orig: KEY_3 (fallen/outro/key.h)
#define KEY_3         0x04
// uc_orig: KEY_4 (fallen/outro/key.h)
#define KEY_4         0x05
// uc_orig: KEY_5 (fallen/outro/key.h)
#define KEY_5         0x06
// uc_orig: KEY_6 (fallen/outro/key.h)
#define KEY_6         0x07
// uc_orig: KEY_7 (fallen/outro/key.h)
#define KEY_7         0x08
// uc_orig: KEY_8 (fallen/outro/key.h)
#define KEY_8         0x09
// uc_orig: KEY_9 (fallen/outro/key.h)
#define KEY_9         0x0a
// uc_orig: KEY_0 (fallen/outro/key.h)
#define KEY_0         0x0b
// uc_orig: KEY_MINUS (fallen/outro/key.h)
#define KEY_MINUS     0x0c
// uc_orig: KEY_EQUAL (fallen/outro/key.h)
#define KEY_EQUAL     0x0d
// uc_orig: KEY_BACKSPACE (fallen/outro/key.h)
#define KEY_BACKSPACE 0x0e
// uc_orig: KEY_TAB (fallen/outro/key.h)
#define KEY_TAB       0x0f
// uc_orig: KEY_Q (fallen/outro/key.h)
#define KEY_Q         0x010
// uc_orig: KEY_W (fallen/outro/key.h)
#define KEY_W         0x011
// uc_orig: KEY_E (fallen/outro/key.h)
#define KEY_E         0x012
// uc_orig: KEY_R (fallen/outro/key.h)
#define KEY_R         0x013
// uc_orig: KEY_T (fallen/outro/key.h)
#define KEY_T         0x014
// uc_orig: KEY_Y (fallen/outro/key.h)
#define KEY_Y         0x015
// uc_orig: KEY_U (fallen/outro/key.h)
#define KEY_U         0x016
// uc_orig: KEY_I (fallen/outro/key.h)
#define KEY_I         0x017
// uc_orig: KEY_O (fallen/outro/key.h)
#define KEY_O         0x018
// uc_orig: KEY_P (fallen/outro/key.h)
#define KEY_P         0x019
// uc_orig: KEY_LSBRACKET (fallen/outro/key.h)
#define KEY_LSBRACKET 0x01a
// uc_orig: KEY_RSBRACKET (fallen/outro/key.h)
#define KEY_RSBRACKET 0x01b
// uc_orig: KEY_RETURN (fallen/outro/key.h)
#define KEY_RETURN    0x01c
// uc_orig: KEY_LCONTROL (fallen/outro/key.h)
#define KEY_LCONTROL  0x01d
// uc_orig: KEY_RCONTROL (fallen/outro/key.h)
#define KEY_RCONTROL  (0x01d + 0x80)
// uc_orig: KEY_A (fallen/outro/key.h)
#define KEY_A         0x01e
// uc_orig: KEY_S (fallen/outro/key.h)
#define KEY_S         0x01f
// uc_orig: KEY_D (fallen/outro/key.h)
#define KEY_D         0x020
// uc_orig: KEY_F (fallen/outro/key.h)
#define KEY_F         0x021
// uc_orig: KEY_G (fallen/outro/key.h)
#define KEY_G         0x022
// uc_orig: KEY_H (fallen/outro/key.h)
#define KEY_H         0x023
// uc_orig: KEY_J (fallen/outro/key.h)
#define KEY_J         0x024
// uc_orig: KEY_K (fallen/outro/key.h)
#define KEY_K         0x025
// uc_orig: KEY_L (fallen/outro/key.h)
#define KEY_L         0x026
// uc_orig: KEY_COLON (fallen/outro/key.h)
#define KEY_COLON     0x027
// uc_orig: KEY_QUOTE (fallen/outro/key.h)
#define KEY_QUOTE     0x028
// uc_orig: KEY_QUOTE2 (fallen/outro/key.h)
#define KEY_QUOTE2    0x029
// uc_orig: KEY_LSHIFT (fallen/outro/key.h)
#define KEY_LSHIFT    0x02a
// uc_orig: KEY_HASH (fallen/outro/key.h)
#define KEY_HASH      0x02b
// uc_orig: KEY_BACKSLASH (fallen/outro/key.h)
#define KEY_BACKSLASH 0x056
// uc_orig: KEY_Z (fallen/outro/key.h)
#define KEY_Z         0x02c
// uc_orig: KEY_X (fallen/outro/key.h)
#define KEY_X         0x02d
// uc_orig: KEY_C (fallen/outro/key.h)
#define KEY_C         0x02e
// uc_orig: KEY_V (fallen/outro/key.h)
#define KEY_V         0x02f
// uc_orig: KEY_B (fallen/outro/key.h)
#define KEY_B         0x030
// uc_orig: KEY_N (fallen/outro/key.h)
#define KEY_N         0x031
// uc_orig: KEY_M (fallen/outro/key.h)
#define KEY_M         0x032
// uc_orig: KEY_COMMA (fallen/outro/key.h)
#define KEY_COMMA     0x033
// uc_orig: KEY_POINT (fallen/outro/key.h)
#define KEY_POINT     0x034
// uc_orig: KEY_SLASH (fallen/outro/key.h)
#define KEY_SLASH     0x035
// uc_orig: KEY_RSHIFT (fallen/outro/key.h)
#define KEY_RSHIFT    0x036
// uc_orig: KEY_LALT (fallen/outro/key.h)
#define KEY_LALT      0x038
// uc_orig: KEY_RALT (fallen/outro/key.h)
#define KEY_RALT      (0x038 + 0x80)
// uc_orig: KEY_SPACE (fallen/outro/key.h)
#define KEY_SPACE     0x039
// uc_orig: KEY_CAPS (fallen/outro/key.h)
#define KEY_CAPS      0x03a
// uc_orig: KEY_F1 (fallen/outro/key.h)
#define KEY_F1        0x03b
// uc_orig: KEY_F2 (fallen/outro/key.h)
#define KEY_F2        0x03c
// uc_orig: KEY_F3 (fallen/outro/key.h)
#define KEY_F3        0x03d
// uc_orig: KEY_F4 (fallen/outro/key.h)
#define KEY_F4        0x03e
// uc_orig: KEY_F5 (fallen/outro/key.h)
#define KEY_F5        0x03f
// uc_orig: KEY_F6 (fallen/outro/key.h)
#define KEY_F6        0x040
// uc_orig: KEY_F7 (fallen/outro/key.h)
#define KEY_F7        0x041
// uc_orig: KEY_F8 (fallen/outro/key.h)
#define KEY_F8        0x042
// uc_orig: KEY_F9 (fallen/outro/key.h)
#define KEY_F9        0x043
// uc_orig: KEY_F10 (fallen/outro/key.h)
#define KEY_F10       0x044
// uc_orig: KEY_F11 (fallen/outro/key.h)
#define KEY_F11       0x057
// uc_orig: KEY_F12 (fallen/outro/key.h)
#define KEY_F12       0x058

// uc_orig: KEY_HOME (fallen/outro/key.h)
#define KEY_HOME     (0x47 + 0x80)
// uc_orig: KEY_UP (fallen/outro/key.h)
#define KEY_UP       (0x48 + 0x80)
// uc_orig: KEY_PAGEUP (fallen/outro/key.h)
#define KEY_PAGEUP   (0x49 + 0x80)
// uc_orig: KEY_LEFT (fallen/outro/key.h)
#define KEY_LEFT     (0x4b + 0x80)
// uc_orig: KEY_RIGHT (fallen/outro/key.h)
#define KEY_RIGHT    (0x4d + 0x80)
// uc_orig: KEY_END (fallen/outro/key.h)
#define KEY_END      (0x4f + 0x80)
// uc_orig: KEY_DOWN (fallen/outro/key.h)
#define KEY_DOWN     (0x50 + 0x80)
// uc_orig: KEY_PAGEDOWN (fallen/outro/key.h)
#define KEY_PAGEDOWN (0x51 + 0x80)
// uc_orig: KEY_INSERT (fallen/outro/key.h)
#define KEY_INSERT   (0x52 + 0x80)
// uc_orig: KEY_DELETE (fallen/outro/key.h)
#define KEY_DELETE   (0x53 + 0x80)

// uc_orig: KEY_NUM_LOCK (fallen/outro/key.h)
#define KEY_NUM_LOCK 0x045
// uc_orig: KEY_PMINUS (fallen/outro/key.h)
#define KEY_PMINUS   0x04a
// uc_orig: KEY_PADD (fallen/outro/key.h)
#define KEY_PADD     0x04e
// uc_orig: KEY_PSLASH (fallen/outro/key.h)
#define KEY_PSLASH   (0x035 + 0x80)
// uc_orig: KEY_ASTERISK (fallen/outro/key.h)
#define KEY_ASTERISK 0x037
// uc_orig: KEY_PDOT (fallen/outro/key.h)
#define KEY_PDOT     0x053
// uc_orig: KEY_ENTER (fallen/outro/key.h)
#define KEY_ENTER    (0x01c + 0x80)

// Numpad keys.
// uc_orig: KEY_P7 (fallen/outro/key.h)
#define KEY_P7 0x047
// uc_orig: KEY_P8 (fallen/outro/key.h)
#define KEY_P8 0x048
// uc_orig: KEY_P9 (fallen/outro/key.h)
#define KEY_P9 0x049
// uc_orig: KEY_P4 (fallen/outro/key.h)
#define KEY_P4 0x04b
// uc_orig: KEY_P5 (fallen/outro/key.h)
#define KEY_P5 0x04c
// uc_orig: KEY_P6 (fallen/outro/key.h)
#define KEY_P6 0x04d
// uc_orig: KEY_P1 (fallen/outro/key.h)
#define KEY_P1 0x04f
// uc_orig: KEY_P2 (fallen/outro/key.h)
#define KEY_P2 0x050
// uc_orig: KEY_P3 (fallen/outro/key.h)
#define KEY_P3 0x051
// uc_orig: KEY_P0 (fallen/outro/key.h)
#define KEY_P0 0x052

#endif // OUTRO_CORE_OUTRO_KEY_H
