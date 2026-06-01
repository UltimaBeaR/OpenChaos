// Keyboard input handling — thin SDL event hook layer that forwards
// presses/releases into input_frame and refreshes the convenience
// modifier flags (Shift/Ctrl/Alt). The legacy Keys[256] / LastKey /
// same-turn release latching machinery has been removed: input_frame is
// the single source of truth for keyboard state.

#include "engine/input/keyboard.h"
#include "engine/input/input_frame.h"
#include "game/action_map/act_modifiers.h" // ACT_MOD_*

// Modifier flags are convenience shortcuts for the held state of Shift /
// Ctrl — recomputed from input_frame's live event-tracked state on every
// keyboard event so the just-applied event is visible immediately
// (input_key_held would return the previous snapshot until the next
// input_frame_update). (Alt is no longer tracked — nothing read AltFlag.)
static void update_modifier_flags()
{
    ControlFlag = input_key_event_held(ACT_MOD_CTRL_L_KKEY) || debug_overlay_locked_on;
    ShiftFlag = input_key_event_held(ACT_MOD_SHIFT_L_KKEY) || input_key_event_held(ACT_MOD_SHIFT_R_KKEY);
}

// uc_orig: SetupKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
BOOL SetupKeyboard(void)
{
    ControlFlag = 0;
    ShiftFlag = 0;
    return UC_TRUE;
}

// uc_orig: ResetKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
void ResetKeyboard(void)
{
    // No-op: Win32 keyboard hook removed.
}

// Called from SDL3 event loop when a key is pressed.
// scancode is a KB_* scancode (Set 1, extended keys have 0x80 added).
// uc_orig: derived from KeyboardProc (fallen/DDLibrary/Source/GKeyboard.cpp)
void keyboard_key_down(UBYTE scancode)
{
    input_frame_on_key_down(scancode);
    update_modifier_flags();
}

// Called from SDL3 event loop when a key is released.
// uc_orig: derived from KeyboardProc (fallen/DDLibrary/Source/GKeyboard.cpp)
void keyboard_key_up(UBYTE scancode)
{
    input_frame_on_key_up(scancode);
    update_modifier_flags();
}
