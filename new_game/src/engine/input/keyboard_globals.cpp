#include "engine/input/keyboard_globals.h"

// uc_orig: AltFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
volatile UBYTE AltFlag,
    // uc_orig: ControlFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
    ControlFlag,
    // uc_orig: ShiftFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
    ShiftFlag;

bool debug_overlay_locked_on = false;
