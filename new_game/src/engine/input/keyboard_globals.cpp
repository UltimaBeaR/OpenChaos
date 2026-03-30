#include "engine/input/keyboard_globals.h"

// uc_orig: AltFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
volatile UBYTE AltFlag,
// uc_orig: ControlFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
    ControlFlag,
// uc_orig: ShiftFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
    ShiftFlag;
// uc_orig: Keys (fallen/DDLibrary/Source/GKeyboard.cpp)
volatile UBYTE Keys[256],
// uc_orig: LastKey (fallen/DDLibrary/Source/GKeyboard.cpp)
    LastKey;

// uc_orig: key_turn (fallen/DDLibrary/Source/GKeyboard.cpp)
UBYTE key_turn[256];

// uc_orig: Released (fallen/DDLibrary/Source/GKeyboard.cpp)
UBYTE Released[MAX_RELEASE];
// uc_orig: release_count (fallen/DDLibrary/Source/GKeyboard.cpp)
SWORD release_count = 0;
// uc_orig: game_turn (fallen/DDLibrary/Source/GKeyboard.cpp)
UBYTE game_turn = 0;
