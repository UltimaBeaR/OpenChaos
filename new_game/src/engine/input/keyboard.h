#ifndef ENGINE_INPUT_KEYBOARD_H
#define ENGINE_INPUT_KEYBOARD_H

#include "engine/core/types.h"

// Keyboard scancode constants (DirectInput Set 1; extended keys = base + 0x80)
// live in game/action_map/input_codes.h as KKEY_* — included here so existing
// consumers of keyboard.h continue to compile.
//
// uc_orig: keyboard scancodes (MFStdLib/Headers/StdKeybd.h)
#include "game/action_map/input_codes.h"

#include "engine/input/keyboard_globals.h"

// uc_orig: SetupKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
BOOL SetupKeyboard(void);
// uc_orig: ResetKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
void ResetKeyboard(void);

// Direct input callbacks (called from SDL3 event loop).
// scancode is a KKEY_* scancode (Set 1, extended keys have 0x80 added).
void keyboard_key_down(UBYTE scancode);
void keyboard_key_up(UBYTE scancode);

#endif // ENGINE_INPUT_KEYBOARD_H
