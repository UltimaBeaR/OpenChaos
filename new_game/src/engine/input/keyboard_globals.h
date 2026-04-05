#ifndef ENGINE_INPUT_KEYBOARD_GLOBALS_H
#define ENGINE_INPUT_KEYBOARD_GLOBALS_H

#include "engine/core/types.h"

// Public keyboard state.

// uc_orig: AltFlag (MFStdLib/Headers/StdKeybd.h)
extern volatile UBYTE AltFlag;
// uc_orig: ControlFlag (MFStdLib/Headers/StdKeybd.h)
extern volatile UBYTE ControlFlag;
// When true, ControlFlag is forced to 1 every frame (debug overlay toggle).
extern bool debug_overlay_locked_on;
// uc_orig: ShiftFlag (MFStdLib/Headers/StdKeybd.h)
extern volatile UBYTE ShiftFlag;
// uc_orig: Keys (MFStdLib/Headers/StdKeybd.h)
extern volatile UBYTE Keys[256];
// uc_orig: LastKey (MFStdLib/Headers/StdKeybd.h)
extern volatile UBYTE LastKey;

// Internal keyboard state (used by keyboard.cpp, exposed for host.cpp access).

// uc_orig: key_turn (fallen/DDLibrary/Source/GKeyboard.cpp)
extern UBYTE key_turn[256];
// uc_orig: MAX_RELEASE (fallen/DDLibrary/Source/GKeyboard.cpp)
#define MAX_RELEASE 10
// uc_orig: Released (fallen/DDLibrary/Source/GKeyboard.cpp)
extern UBYTE Released[MAX_RELEASE];
// uc_orig: release_count (fallen/DDLibrary/Source/GKeyboard.cpp)
extern SWORD release_count;
// uc_orig: game_turn (fallen/DDLibrary/Source/GKeyboard.cpp)
extern UBYTE game_turn;

#endif // ENGINE_INPUT_KEYBOARD_GLOBALS_H
