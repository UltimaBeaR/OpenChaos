#ifndef UI_INTERFAC_GLOBALS_H
#define UI_INTERFAC_GLOBALS_H

// Global variables for the player input system (interfac.cpp).

#include "core/types.h"
#include <windows.h>  // DWORD for g_dwLastInputChangeTime

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

// Dead camera position deltas from "look around" mode (not used in current PC code path).
// uc_orig: last_camera_dx (fallen/Source/interfac.cpp)
extern SLONG last_camera_dx;
// uc_orig: last_camera_dy (fallen/Source/interfac.cpp)
extern SLONG last_camera_dy;
// uc_orig: last_camera_dz (fallen/Source/interfac.cpp)
extern SLONG last_camera_dz;
// uc_orig: last_camera_yaw (fallen/Source/interfac.cpp)
extern SLONG last_camera_yaw;
// uc_orig: last_camera_pitch (fallen/Source/interfac.cpp)
extern SLONG last_camera_pitch;

// Current pitch angle for first-person look mode (0..2047).
// uc_orig: look_pitch (fallen/Source/interfac.cpp)
extern SLONG look_pitch;

// Raw hardware input captured last frame (debug / replay use).
// uc_orig: debug_input (fallen/Source/interfac.cpp)
extern ULONG debug_input;

// Last polled input state and edge-detect buffers (used by get_last_input).
// uc_orig: m_CurrentInput (fallen/Source/interfac.cpp)
extern ULONG m_CurrentInput;
// uc_orig: m_PreviousInput (fallen/Source/interfac.cpp)
extern ULONG m_PreviousInput;
// uc_orig: m_CurrentGoneDownInput (fallen/Source/interfac.cpp)
extern ULONG m_CurrentGoneDownInput;

// Timestamp (GetTickCount ms) of the last input state change; used to detect idle/controller removal.
// uc_orig: g_dwLastInputChangeTime (fallen/Source/interfac.cpp)
extern DWORD g_dwLastInputChangeTime;

// Non-zero while the player is in first-person aim mode; read by the renderer (aeng.cpp).
// uc_orig: FirstPersonMode (fallen/Source/interfac.cpp)
extern SLONG FirstPersonMode;

// Last camera type selected by the player (persisted across network desync recovery).
// uc_orig: g_iPlayerCameraMode (fallen/Source/interfac.cpp)
extern int g_iPlayerCameraMode;

// Lookup table: 4-bit direction bitmask (r/l/b/f) -> 2048-unit angle for fight-step direction.
// uc_orig: input_to_angle (fallen/Source/interfac.cpp)
extern UBYTE input_to_angle[];

#endif // UI_INTERFAC_GLOBALS_H
