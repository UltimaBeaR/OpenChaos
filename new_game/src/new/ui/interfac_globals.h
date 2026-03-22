#ifndef UI_INTERFAC_GLOBALS_H
#define UI_INTERFAC_GLOBALS_H

// Global variables for the player input system (interfac.cpp).

#include "core/types.h"

// uc_orig: player_relative (fallen/Source/interfac.cpp)
extern UBYTE player_relative;
// uc_orig: cheat (fallen/Source/interfac.cpp)
extern UBYTE cheat;
// uc_orig: g_bPunishMePleaseICheatedOnThisLevel (fallen/Source/interfac.cpp)
extern bool g_bPunishMePleaseICheatedOnThisLevel;
// uc_orig: input_mode (fallen/Source/interfac.cpp)
extern SLONG input_mode;
// uc_orig: mouse_input (fallen/Source/interfac.cpp)
extern SLONG mouse_input;
// uc_orig: analogue (fallen/Source/interfac.cpp)
extern SLONG analogue;
// uc_orig: g_bEngineVibrations (fallen/Source/interfac.cpp)
extern bool g_bEngineVibrations;
// uc_orig: m_bForceWalk (fallen/Source/interfac.cpp)
extern bool m_bForceWalk;
// uc_orig: g_iCheatNumber (fallen/Source/interfac.cpp)
extern int g_iCheatNumber;
// Maps logical button functions (JOYPAD_BUTTON_*) to physical DirectInput button indices.
// uc_orig: joypad_button_use (fallen/Source/interfac.cpp)
extern UBYTE joypad_button_use[16];
// Maps logical button functions (KEYBRD_BUTTON_*) to keyboard scan codes.
// uc_orig: keybrd_button_use (fallen/Source/interfac.cpp)
extern UBYTE keybrd_button_use[16];

#endif // UI_INTERFAC_GLOBALS_H
