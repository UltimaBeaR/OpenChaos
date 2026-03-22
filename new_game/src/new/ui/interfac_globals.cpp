// Global variables for the player input system.
// All globals from interfac.cpp moved here per Stage 4 rules.

#include "ui/interfac_globals.h"

// uc_orig: player_relative (fallen/Source/interfac.cpp)
UBYTE player_relative;

// uc_orig: cheat (fallen/Source/interfac.cpp)
UBYTE cheat = 0;

// uc_orig: g_bPunishMePleaseICheatedOnThisLevel (fallen/Source/interfac.cpp)
bool g_bPunishMePleaseICheatedOnThisLevel = UC_FALSE;

// uc_orig: input_mode (fallen/Source/interfac.cpp)
SLONG input_mode = 0;

// uc_orig: mouse_input (fallen/Source/interfac.cpp)
// Set to 1 when mouse input is active (PC only).
SLONG mouse_input = 0;

// uc_orig: analogue (fallen/Source/interfac.cpp)
// Set to 1 when analog stick input is in use.
SLONG analogue = 0;

// uc_orig: g_bEngineVibrations (fallen/Source/interfac.cpp)
// Whether gamepad force feedback (vibration) is enabled.
bool g_bEngineVibrations = UC_TRUE;

// uc_orig: m_bForceWalk (fallen/Source/interfac.cpp)
bool m_bForceWalk = UC_FALSE;

// uc_orig: g_iCheatNumber (fallen/Source/interfac.cpp)
int g_iCheatNumber = -1;

// uc_orig: joypad_button_use (fallen/Source/interfac.cpp)
UBYTE joypad_button_use[16];

// uc_orig: keybrd_button_use (fallen/Source/interfac.cpp)
UBYTE keybrd_button_use[16];
