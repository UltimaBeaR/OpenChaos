#ifndef UI_CONTROLS_GLOBALS_H
#define UI_CONTROLS_GLOBALS_H

#include "engine/platform/platform.h"              // SLONG, UBYTE, UWORD, CBYTE, BOOL base types
#include "engine/lighting/light.h"

// uc_orig: NIGHT_specular_enable (fallen/Source/Controls.cpp)
extern SLONG NIGHT_specular_enable;

// uc_orig: draw_3d (fallen/Source/Controls.cpp)
extern SLONG draw_3d;

// uc_orig: amb_choice (fallen/Source/Controls.cpp)
extern LIGHT_Colour amb_choice[3];

// uc_orig: amb_colour (fallen/Source/Controls.cpp)
extern LIGHT_Colour amb_colour;

// uc_orig: amb_choice_cur (fallen/Source/Controls.cpp)
extern SLONG amb_choice_cur;

// uc_orig: controls (fallen/Source/Controls.cpp)
extern UWORD controls;

// uc_orig: stealth_debug (fallen/Source/Controls.cpp)
// If set, Darci is undetected by enemies (testing stealth).
extern UBYTE stealth_debug;

// uc_orig: CONTROLS_inventory_mode (fallen/Source/Controls.cpp)
// Countdown timer (in ticks) for the inventory panel fade; 0 = hidden.
extern SWORD CONTROLS_inventory_mode;

// uc_orig: InkeyToAscii (fallen/Source/Controls.cpp)
// DirectInput scan-code to ASCII table (unshifted).
extern UBYTE InkeyToAscii[128];

// uc_orig: InkeyToAsciiShift (fallen/Source/Controls.cpp)
// DirectInput scan-code to ASCII table (shifted).
extern UBYTE InkeyToAsciiShift[128];

// uc_orig: cmd_list (fallen/Source/Controls.cpp)
extern CBYTE* cmd_list[];

// uc_orig: allow_debug_keys (fallen/Source/Controls.cpp)
// Global flag: debug keyboard shortcuts are active.
extern BOOL allow_debug_keys;

// uc_orig: dkeys_have_been_used (fallen/Source/Controls.cpp)
// Set to UC_TRUE once debug keys are toggled via the console.
extern BOOL dkeys_have_been_used;

// uc_orig: yomp_speed (fallen/Source/Controls.cpp)
// Darci walk speed (debug-adjustable via Ctrl+Shift+1-4).
extern SLONG yomp_speed;

// uc_orig: sprint_speed (fallen/Source/Controls.cpp)
// Darci sprint speed (debug-adjustable).
extern SLONG sprint_speed;

#endif // UI_CONTROLS_GLOBALS_H
