// Global variables for the player input system.
// All globals from interfac.cpp moved here per Stage 4 rules.

#include "game/input_actions_globals.h"

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
// Analog stick mode: smooth movement speed based on stick deflection.
// Dynamically set to 1 when gamepad is active, 0 for keyboard.
SLONG analogue = 0;

// uc_orig: g_bEngineVibrations (fallen/Source/interfac.cpp)
// Whether gamepad force feedback (vibration) is enabled.
bool g_bEngineVibrations = UC_TRUE;

// uc_orig: m_bForceWalk (fallen/Source/interfac.cpp)
bool m_bForceWalk = UC_FALSE;

// uc_orig: joypad_button_use (fallen/Source/interfac.cpp)
UBYTE joypad_button_use[16];

// uc_orig: keybrd_button_use (fallen/Source/interfac.cpp)
UBYTE keybrd_button_use[16];

// Current pitch angle for first-person look mode (0..2047).
// uc_orig: look_pitch (fallen/Source/interfac.cpp)
SLONG look_pitch = 0;

// Last polled input state and edge-detect buffers (used by get_last_input).
// uc_orig: m_CurrentInput (fallen/Source/interfac.cpp)
ULONG m_CurrentInput = 0;
// uc_orig: m_PreviousInput (fallen/Source/interfac.cpp)
ULONG m_PreviousInput = 0;
// uc_orig: m_CurrentGoneDownInput (fallen/Source/interfac.cpp)
ULONG m_CurrentGoneDownInput = 0;

// Timestamp (sdl3_get_ticks ms) of the last input state change; used to detect idle/controller removal.
// uc_orig: g_dwLastInputChangeTime (fallen/Source/interfac.cpp)
// BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
uint64_t g_dwLastInputChangeTime = 0;

// Non-zero while the player is in first-person aim mode; read by the renderer (aeng.cpp).
// uc_orig: FirstPersonMode (fallen/Source/interfac.cpp)
SLONG FirstPersonMode = UC_FALSE;

// Last camera type selected by the player (persisted across network desync recovery).
// uc_orig: g_iPlayerCameraMode (fallen/Source/interfac.cpp)
int g_iPlayerCameraMode = 0;

// Lookup table: 4-bit direction bitmask (bits: 3=right, 2=left, 1=back, 0=forward)
// -> 256-unit angle base for fight-step direction (shifted left 3 to get 2048-range angle).
// uc_orig: input_to_angle (fallen/Source/interfac.cpp)
UBYTE input_to_angle[] = {
    0,       // 0000 - no direction
    0,       // 0001 - forward
    128,     // 0010 - back
    0,       // 0011 - forward+back (cancel)
    192,     // 0100 - left
    192+32,  // 0101 - forward+left
    192-32,  // 0110 - back+left
    0,       // 0111 - (unused)
    64,      // 1000 - right
    32,      // 1001 - forward+right
    96,      // 1010 - back+right
    0,       // 1011 - (unused)
    0,       // 1100 - (unused)
    0,       // 1101 - (unused)
};
