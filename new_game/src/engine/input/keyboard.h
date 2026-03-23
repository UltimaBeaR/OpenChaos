#ifndef ENGINE_INPUT_KEYBOARD_H
#define ENGINE_INPUT_KEYBOARD_H

#include "core/types.h"

// DirectInput keyboard scancodes.
// Extended keys (right ctrl, arrows, etc.) have 0x80 added.

// uc_orig: KB_ESC (MFStdLib/Headers/StdKeybd.h)
#define KB_ESC 0x01
// uc_orig: KB_1 (MFStdLib/Headers/StdKeybd.h)
#define KB_1 0x02
// uc_orig: KB_2 (MFStdLib/Headers/StdKeybd.h)
#define KB_2 0x03
// uc_orig: KB_3 (MFStdLib/Headers/StdKeybd.h)
#define KB_3 0x04
// uc_orig: KB_4 (MFStdLib/Headers/StdKeybd.h)
#define KB_4 0x05
// uc_orig: KB_5 (MFStdLib/Headers/StdKeybd.h)
#define KB_5 0x06
// uc_orig: KB_6 (MFStdLib/Headers/StdKeybd.h)
#define KB_6 0x07
// uc_orig: KB_7 (MFStdLib/Headers/StdKeybd.h)
#define KB_7 0x08
// uc_orig: KB_8 (MFStdLib/Headers/StdKeybd.h)
#define KB_8 0x09
// uc_orig: KB_9 (MFStdLib/Headers/StdKeybd.h)
#define KB_9 0x0a
// uc_orig: KB_0 (MFStdLib/Headers/StdKeybd.h)
#define KB_0 0x0b
// uc_orig: KB_MINUS (MFStdLib/Headers/StdKeybd.h)
#define KB_MINUS 0x0c
// uc_orig: KB_PLUS (MFStdLib/Headers/StdKeybd.h)
#define KB_PLUS 0x0d
// uc_orig: KB_BS (MFStdLib/Headers/StdKeybd.h)
#define KB_BS 0x0e

// uc_orig: KB_TAB (MFStdLib/Headers/StdKeybd.h)
#define KB_TAB 0x0f
// uc_orig: KB_Q (MFStdLib/Headers/StdKeybd.h)
#define KB_Q 0x10
// uc_orig: KB_W (MFStdLib/Headers/StdKeybd.h)
#define KB_W 0x11
// uc_orig: KB_E (MFStdLib/Headers/StdKeybd.h)
#define KB_E 0x12
// uc_orig: KB_R (MFStdLib/Headers/StdKeybd.h)
#define KB_R 0x13
// uc_orig: KB_T (MFStdLib/Headers/StdKeybd.h)
#define KB_T 0x14
// uc_orig: KB_Y (MFStdLib/Headers/StdKeybd.h)
#define KB_Y 0x15
// uc_orig: KB_U (MFStdLib/Headers/StdKeybd.h)
#define KB_U 0x16
// uc_orig: KB_I (MFStdLib/Headers/StdKeybd.h)
#define KB_I 0x17
// uc_orig: KB_O (MFStdLib/Headers/StdKeybd.h)
#define KB_O 0x18
// uc_orig: KB_P (MFStdLib/Headers/StdKeybd.h)
#define KB_P 0x19
// uc_orig: KB_LBRACE (MFStdLib/Headers/StdKeybd.h)
#define KB_LBRACE 0x1a
// uc_orig: KB_RBRACE (MFStdLib/Headers/StdKeybd.h)
#define KB_RBRACE 0x1b
// uc_orig: KB_ENTER (MFStdLib/Headers/StdKeybd.h)
#define KB_ENTER 0x1c

// uc_orig: KB_LCONTROL (MFStdLib/Headers/StdKeybd.h)
#define KB_LCONTROL 0x1d
// uc_orig: KB_CAPSLOCK (MFStdLib/Headers/StdKeybd.h)
#define KB_CAPSLOCK 0x3a
// uc_orig: KB_A (MFStdLib/Headers/StdKeybd.h)
#define KB_A 0x1e
// uc_orig: KB_S (MFStdLib/Headers/StdKeybd.h)
#define KB_S 0x1f
// uc_orig: KB_D (MFStdLib/Headers/StdKeybd.h)
#define KB_D 0x20
// uc_orig: KB_F (MFStdLib/Headers/StdKeybd.h)
#define KB_F 0x21
// uc_orig: KB_G (MFStdLib/Headers/StdKeybd.h)
#define KB_G 0x22
// uc_orig: KB_H (MFStdLib/Headers/StdKeybd.h)
#define KB_H 0x23
// uc_orig: KB_J (MFStdLib/Headers/StdKeybd.h)
#define KB_J 0x24
// uc_orig: KB_K (MFStdLib/Headers/StdKeybd.h)
#define KB_K 0x25
// uc_orig: KB_L (MFStdLib/Headers/StdKeybd.h)
#define KB_L 0x26
// uc_orig: KB_COLON (MFStdLib/Headers/StdKeybd.h)
#define KB_COLON 0x27
// uc_orig: KB_QUOTE (MFStdLib/Headers/StdKeybd.h)
#define KB_QUOTE 0x28
// uc_orig: KB_TILD (MFStdLib/Headers/StdKeybd.h)
#define KB_TILD 0x29

// uc_orig: KB_LSHIFT (MFStdLib/Headers/StdKeybd.h)
#define KB_LSHIFT 0x2a
// uc_orig: KB_BACKSLASH (MFStdLib/Headers/StdKeybd.h)
#define KB_BACKSLASH 0x2b
// uc_orig: KB_Z (MFStdLib/Headers/StdKeybd.h)
#define KB_Z 0x2c
// uc_orig: KB_X (MFStdLib/Headers/StdKeybd.h)
#define KB_X 0x2d
// uc_orig: KB_C (MFStdLib/Headers/StdKeybd.h)
#define KB_C 0x2e
// uc_orig: KB_V (MFStdLib/Headers/StdKeybd.h)
#define KB_V 0x2f
// uc_orig: KB_B (MFStdLib/Headers/StdKeybd.h)
#define KB_B 0x30
// uc_orig: KB_N (MFStdLib/Headers/StdKeybd.h)
#define KB_N 0x31
// uc_orig: KB_M (MFStdLib/Headers/StdKeybd.h)
#define KB_M 0x32
// uc_orig: KB_COMMA (MFStdLib/Headers/StdKeybd.h)
#define KB_COMMA 0x33
// uc_orig: KB_POINT (MFStdLib/Headers/StdKeybd.h)
#define KB_POINT 0x34
// uc_orig: KB_FORESLASH (MFStdLib/Headers/StdKeybd.h)
#define KB_FORESLASH 0x35
// uc_orig: KB_RSHIFT (MFStdLib/Headers/StdKeybd.h)
#define KB_RSHIFT 0x36
// uc_orig: KB_LALT (MFStdLib/Headers/StdKeybd.h)
#define KB_LALT 0x38
// uc_orig: KB_SPACE (MFStdLib/Headers/StdKeybd.h)
#define KB_SPACE 0x39
// uc_orig: KB_RALT (MFStdLib/Headers/StdKeybd.h)
#define KB_RALT (0x38 + 0x80)
// uc_orig: KB_RCONTROL (MFStdLib/Headers/StdKeybd.h)
#define KB_RCONTROL (0x1d + 0x80)

// uc_orig: KB_F1 (MFStdLib/Headers/StdKeybd.h)
#define KB_F1 0x3b
// uc_orig: KB_F2 (MFStdLib/Headers/StdKeybd.h)
#define KB_F2 0x3c
// uc_orig: KB_F3 (MFStdLib/Headers/StdKeybd.h)
#define KB_F3 0x3d
// uc_orig: KB_F4 (MFStdLib/Headers/StdKeybd.h)
#define KB_F4 0x3e
// uc_orig: KB_F5 (MFStdLib/Headers/StdKeybd.h)
#define KB_F5 0x3f
// uc_orig: KB_F6 (MFStdLib/Headers/StdKeybd.h)
#define KB_F6 0x40
// uc_orig: KB_F7 (MFStdLib/Headers/StdKeybd.h)
#define KB_F7 0x41
// uc_orig: KB_F8 (MFStdLib/Headers/StdKeybd.h)
#define KB_F8 0x42
// uc_orig: KB_F9 (MFStdLib/Headers/StdKeybd.h)
#define KB_F9 0x43
// uc_orig: KB_F10 (MFStdLib/Headers/StdKeybd.h)
#define KB_F10 0x44
// uc_orig: KB_F11 (MFStdLib/Headers/StdKeybd.h)
#define KB_F11 0x57
// uc_orig: KB_F12 (MFStdLib/Headers/StdKeybd.h)
#define KB_F12 0x58

// uc_orig: KB_PRTSC (MFStdLib/Headers/StdKeybd.h)
#define KB_PRTSC (0x37 + 0x80)
// uc_orig: KB_SCROLLLOCK (MFStdLib/Headers/StdKeybd.h)
#define KB_SCROLLLOCK 0x46

// Edit pad
// uc_orig: KB_INS (MFStdLib/Headers/StdKeybd.h)
#define KB_INS (0x52 + 0x80)
// uc_orig: KB_HOME (MFStdLib/Headers/StdKeybd.h)
#define KB_HOME (0x47 + 0x80)
// uc_orig: KB_PGUP (MFStdLib/Headers/StdKeybd.h)
#define KB_PGUP (0x49 + 0x80)
// uc_orig: KB_DEL (MFStdLib/Headers/StdKeybd.h)
#define KB_DEL (0x53 + 0x80)
// uc_orig: KB_END (MFStdLib/Headers/StdKeybd.h)
#define KB_END (0x4f + 0x80)
// uc_orig: KB_PGDN (MFStdLib/Headers/StdKeybd.h)
#define KB_PGDN (0x51 + 0x80)

// Cursor pad
// uc_orig: KB_LEFT (MFStdLib/Headers/StdKeybd.h)
#define KB_LEFT (0x4b + 0x80)
// uc_orig: KB_UP (MFStdLib/Headers/StdKeybd.h)
#define KB_UP (0x48 + 0x80)
// uc_orig: KB_RIGHT (MFStdLib/Headers/StdKeybd.h)
#define KB_RIGHT (0x4d + 0x80)
// uc_orig: KB_DOWN (MFStdLib/Headers/StdKeybd.h)
#define KB_DOWN (0x50 + 0x80)

// Numpad
// uc_orig: KB_NUMLOCK (MFStdLib/Headers/StdKeybd.h)
#define KB_NUMLOCK 0x45
// uc_orig: KB_PSLASH (MFStdLib/Headers/StdKeybd.h)
#define KB_PSLASH (0x35 + 0x80)
// uc_orig: KB_ASTERISK (MFStdLib/Headers/StdKeybd.h)
#define KB_ASTERISK 0x37
// uc_orig: KB_PMINUS (MFStdLib/Headers/StdKeybd.h)
#define KB_PMINUS 0x4a
// uc_orig: KB_P7 (MFStdLib/Headers/StdKeybd.h)
#define KB_P7 0x47
// uc_orig: KB_P8 (MFStdLib/Headers/StdKeybd.h)
#define KB_P8 0x48
// uc_orig: KB_P9 (MFStdLib/Headers/StdKeybd.h)
#define KB_P9 0x49
// uc_orig: KB_PPLUS (MFStdLib/Headers/StdKeybd.h)
#define KB_PPLUS 0x4e
// uc_orig: KB_P4 (MFStdLib/Headers/StdKeybd.h)
#define KB_P4 0x4b
// uc_orig: KB_P5 (MFStdLib/Headers/StdKeybd.h)
#define KB_P5 0x4c
// uc_orig: KB_P6 (MFStdLib/Headers/StdKeybd.h)
#define KB_P6 0x4d
// uc_orig: KB_P1 (MFStdLib/Headers/StdKeybd.h)
#define KB_P1 0x4f
// uc_orig: KB_P2 (MFStdLib/Headers/StdKeybd.h)
#define KB_P2 0x50
// uc_orig: KB_P3 (MFStdLib/Headers/StdKeybd.h)
#define KB_P3 0x51
// uc_orig: KB_PENTER (MFStdLib/Headers/StdKeybd.h)
#define KB_PENTER (0x1c + 0x80)
// uc_orig: KB_P0 (MFStdLib/Headers/StdKeybd.h)
#define KB_P0 0x52
// uc_orig: KB_PPOINT (MFStdLib/Headers/StdKeybd.h)
#define KB_PPOINT 0x53

#include "engine/input/keyboard_globals.h"

// uc_orig: SetupKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
BOOL SetupKeyboard(void);
// uc_orig: ResetKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
void ResetKeyboard(void);
// uc_orig: ClearLatchedKeys (fallen/DDLibrary/Source/GKeyboard.cpp)
void ClearLatchedKeys(void);

#endif // ENGINE_INPUT_KEYBOARD_H
