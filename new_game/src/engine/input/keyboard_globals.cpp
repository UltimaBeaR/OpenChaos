#include "engine/input/keyboard_globals.h"

// uc_orig: ControlFlag / ShiftFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
// (uc_orig AltFlag removed — it was set from the Alt keys but never read.)
volatile UBYTE ControlFlag,
    ShiftFlag;

bool debug_overlay_locked_on = false;
