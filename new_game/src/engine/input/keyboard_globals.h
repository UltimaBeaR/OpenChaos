#ifndef ENGINE_INPUT_KEYBOARD_GLOBALS_H
#define ENGINE_INPUT_KEYBOARD_GLOBALS_H

#include "engine/core/types.h"

// Public keyboard state — the legacy Keys[256] array and LastKey have been
// removed. Use input_frame.h API:
//   input_key_held / input_key_just_pressed / input_key_press_pending
//   input_key_consume / input_key_force_release
//   input_last_key / input_last_key_consume
// Modifier flags below are kept as convenience shortcuts (mirrored from
// input_frame's event-tracked state on every keyboard event in keyboard.cpp).

// uc_orig: AltFlag (MFStdLib/Headers/StdKeybd.h)
extern volatile UBYTE AltFlag;
// uc_orig: ControlFlag (MFStdLib/Headers/StdKeybd.h)
extern volatile UBYTE ControlFlag;
// When true, ControlFlag is forced to 1 every frame (debug overlay toggle).
extern bool debug_overlay_locked_on;
// uc_orig: ShiftFlag (MFStdLib/Headers/StdKeybd.h)
extern volatile UBYTE ShiftFlag;

#endif // ENGINE_INPUT_KEYBOARD_GLOBALS_H
