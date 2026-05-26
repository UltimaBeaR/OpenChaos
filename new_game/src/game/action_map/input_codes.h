#ifndef GAME_ACTION_MAP_INPUT_CODES_H
#define GAME_ACTION_MAP_INPUT_CODES_H

// =============================================================================
// Device-level input codes — values for action constants in act_*.h files.
//
// Naming rules, structure of action constants, suffix table → see
//   new_game_devlog/input_system/action_map/rules.md
//
// This file holds ONLY device codes (KKEY_*, MBTN_*, MAXIS_*, GBTN_*, DBTN_*,
// GTRIG_*). Action constants (ACT_<context>_<purpose>_<TYPE>) live in
// new_game/src/game/action_map/act_*.h and reference these device codes as
// their values.
//
// GAXIS_* and GDIR_DIR_* mirror the InputStickId / InputStickDir enums in
// engine/input/input_frame.h so they can be passed directly to the two-arg
// input_stick_*() API via implicit int → enum conversion.
// =============================================================================

// =============================================================================
// KKEY_* — Keyboard scancodes (DirectInput Set 1; extended keys = base + 0x80).
// =============================================================================

// uc_orig: KB_ESC (MFStdLib/Headers/StdKeybd.h)
#define KKEY_ESC 0x01
// uc_orig: KB_1 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_1 0x02
// uc_orig: KB_2 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_2 0x03
// uc_orig: KB_3 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_3 0x04
// uc_orig: KB_4 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_4 0x05
// uc_orig: KB_5 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_5 0x06
// uc_orig: KB_6 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_6 0x07
// uc_orig: KB_7 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_7 0x08
// uc_orig: KB_8 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_8 0x09
// uc_orig: KB_9 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_9 0x0a
// uc_orig: KB_0 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_0 0x0b
// uc_orig: KB_MINUS (MFStdLib/Headers/StdKeybd.h)
#define KKEY_MINUS 0x0c
// uc_orig: KB_PLUS (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PLUS 0x0d
// uc_orig: KB_BS (MFStdLib/Headers/StdKeybd.h)
#define KKEY_BS 0x0e

// uc_orig: KB_TAB (MFStdLib/Headers/StdKeybd.h)
#define KKEY_TAB 0x0f
// uc_orig: KB_Q (MFStdLib/Headers/StdKeybd.h)
#define KKEY_Q 0x10
// uc_orig: KB_W (MFStdLib/Headers/StdKeybd.h)
#define KKEY_W 0x11
// uc_orig: KB_E (MFStdLib/Headers/StdKeybd.h)
#define KKEY_E 0x12
// uc_orig: KB_R (MFStdLib/Headers/StdKeybd.h)
#define KKEY_R 0x13
// uc_orig: KB_T (MFStdLib/Headers/StdKeybd.h)
#define KKEY_T 0x14
// uc_orig: KB_Y (MFStdLib/Headers/StdKeybd.h)
#define KKEY_Y 0x15
// uc_orig: KB_U (MFStdLib/Headers/StdKeybd.h)
#define KKEY_U 0x16
// uc_orig: KB_I (MFStdLib/Headers/StdKeybd.h)
#define KKEY_I 0x17
// uc_orig: KB_O (MFStdLib/Headers/StdKeybd.h)
#define KKEY_O 0x18
// uc_orig: KB_P (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P 0x19
// uc_orig: KB_LBRACE (MFStdLib/Headers/StdKeybd.h)
#define KKEY_LBRACE 0x1a
// uc_orig: KB_RBRACE (MFStdLib/Headers/StdKeybd.h)
#define KKEY_RBRACE 0x1b
// uc_orig: KB_ENTER (MFStdLib/Headers/StdKeybd.h)
#define KKEY_ENTER 0x1c

// uc_orig: KB_LCONTROL (MFStdLib/Headers/StdKeybd.h)
#define KKEY_LCONTROL 0x1d
// uc_orig: KB_CAPSLOCK (MFStdLib/Headers/StdKeybd.h)
#define KKEY_CAPSLOCK 0x3a
// uc_orig: KB_A (MFStdLib/Headers/StdKeybd.h)
#define KKEY_A 0x1e
// uc_orig: KB_S (MFStdLib/Headers/StdKeybd.h)
#define KKEY_S 0x1f
// uc_orig: KB_D (MFStdLib/Headers/StdKeybd.h)
#define KKEY_D 0x20
// uc_orig: KB_F (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F 0x21
// uc_orig: KB_G (MFStdLib/Headers/StdKeybd.h)
#define KKEY_G 0x22
// uc_orig: KB_H (MFStdLib/Headers/StdKeybd.h)
#define KKEY_H 0x23
// uc_orig: KB_J (MFStdLib/Headers/StdKeybd.h)
#define KKEY_J 0x24
// uc_orig: KB_K (MFStdLib/Headers/StdKeybd.h)
#define KKEY_K 0x25
// uc_orig: KB_L (MFStdLib/Headers/StdKeybd.h)
#define KKEY_L 0x26
// uc_orig: KB_COLON (MFStdLib/Headers/StdKeybd.h)
#define KKEY_COLON 0x27
// uc_orig: KB_QUOTE (MFStdLib/Headers/StdKeybd.h)
#define KKEY_QUOTE 0x28
// uc_orig: KB_TILD (MFStdLib/Headers/StdKeybd.h)
#define KKEY_TILD 0x29

// uc_orig: KB_LSHIFT (MFStdLib/Headers/StdKeybd.h)
#define KKEY_LSHIFT 0x2a
// uc_orig: KB_BACKSLASH (MFStdLib/Headers/StdKeybd.h)
#define KKEY_BACKSLASH 0x2b
// uc_orig: KB_Z (MFStdLib/Headers/StdKeybd.h)
#define KKEY_Z 0x2c
// uc_orig: KB_X (MFStdLib/Headers/StdKeybd.h)
#define KKEY_X 0x2d
// uc_orig: KB_C (MFStdLib/Headers/StdKeybd.h)
#define KKEY_C 0x2e
// uc_orig: KB_V (MFStdLib/Headers/StdKeybd.h)
#define KKEY_V 0x2f
// uc_orig: KB_B (MFStdLib/Headers/StdKeybd.h)
#define KKEY_B 0x30
// uc_orig: KB_N (MFStdLib/Headers/StdKeybd.h)
#define KKEY_N 0x31
// uc_orig: KB_M (MFStdLib/Headers/StdKeybd.h)
#define KKEY_M 0x32
// uc_orig: KB_COMMA (MFStdLib/Headers/StdKeybd.h)
#define KKEY_COMMA 0x33
// uc_orig: KB_POINT (MFStdLib/Headers/StdKeybd.h)
#define KKEY_POINT 0x34
// uc_orig: KB_FORESLASH (MFStdLib/Headers/StdKeybd.h)
#define KKEY_FORESLASH 0x35
// uc_orig: KB_RSHIFT (MFStdLib/Headers/StdKeybd.h)
#define KKEY_RSHIFT 0x36
// uc_orig: KB_LALT (MFStdLib/Headers/StdKeybd.h)
#define KKEY_LALT 0x38
// uc_orig: KB_SPACE (MFStdLib/Headers/StdKeybd.h)
#define KKEY_SPACE 0x39
// uc_orig: KB_RALT (MFStdLib/Headers/StdKeybd.h)
#define KKEY_RALT (0x38 + 0x80)
// uc_orig: KB_RCONTROL (MFStdLib/Headers/StdKeybd.h)
#define KKEY_RCONTROL (0x1d + 0x80)

// uc_orig: KB_F1 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F1 0x3b
// uc_orig: KB_F2 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F2 0x3c
// uc_orig: KB_F3 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F3 0x3d
// uc_orig: KB_F4 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F4 0x3e
// uc_orig: KB_F5 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F5 0x3f
// uc_orig: KB_F6 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F6 0x40
// uc_orig: KB_F7 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F7 0x41
// uc_orig: KB_F8 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F8 0x42
// uc_orig: KB_F9 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F9 0x43
// uc_orig: KB_F10 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F10 0x44
// uc_orig: KB_F11 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F11 0x57
// uc_orig: KB_F12 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_F12 0x58

// uc_orig: KB_PRTSC (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PRTSC (0x37 + 0x80)
// uc_orig: KB_SCROLLLOCK (MFStdLib/Headers/StdKeybd.h)
#define KKEY_SCROLLLOCK 0x46

// Edit pad
// uc_orig: KB_INS (MFStdLib/Headers/StdKeybd.h)
#define KKEY_INS (0x52 + 0x80)
// uc_orig: KB_HOME (MFStdLib/Headers/StdKeybd.h)
#define KKEY_HOME (0x47 + 0x80)
// uc_orig: KB_PGUP (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PGUP (0x49 + 0x80)
// uc_orig: KB_DEL (MFStdLib/Headers/StdKeybd.h)
#define KKEY_DEL (0x53 + 0x80)
// uc_orig: KB_END (MFStdLib/Headers/StdKeybd.h)
#define KKEY_END (0x4f + 0x80)
// uc_orig: KB_PGDN (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PGDN (0x51 + 0x80)

// Cursor pad
// uc_orig: KB_LEFT (MFStdLib/Headers/StdKeybd.h)
#define KKEY_LEFT (0x4b + 0x80)
// uc_orig: KB_UP (MFStdLib/Headers/StdKeybd.h)
#define KKEY_UP (0x48 + 0x80)
// uc_orig: KB_RIGHT (MFStdLib/Headers/StdKeybd.h)
#define KKEY_RIGHT (0x4d + 0x80)
// uc_orig: KB_DOWN (MFStdLib/Headers/StdKeybd.h)
#define KKEY_DOWN (0x50 + 0x80)

// Numpad
// uc_orig: KB_NUMLOCK (MFStdLib/Headers/StdKeybd.h)
#define KKEY_NUMLOCK 0x45
// uc_orig: KB_PSLASH (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PSLASH (0x35 + 0x80)
// uc_orig: KB_ASTERISK (MFStdLib/Headers/StdKeybd.h)
#define KKEY_ASTERISK 0x37
// uc_orig: KB_PMINUS (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PMINUS 0x4a
// uc_orig: KB_P7 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P7 0x47
// uc_orig: KB_P8 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P8 0x48
// uc_orig: KB_P9 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P9 0x49
// uc_orig: KB_PPLUS (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PPLUS 0x4e
// uc_orig: KB_P4 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P4 0x4b
// uc_orig: KB_P5 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P5 0x4c
// uc_orig: KB_P6 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P6 0x4d
// uc_orig: KB_P1 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P1 0x4f
// uc_orig: KB_P2 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P2 0x50
// uc_orig: KB_P3 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P3 0x51
// uc_orig: KB_PENTER (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PENTER (0x1c + 0x80)
// uc_orig: KB_P0 (MFStdLib/Headers/StdKeybd.h)
#define KKEY_P0 0x52
// uc_orig: KB_PPOINT (MFStdLib/Headers/StdKeybd.h)
#define KKEY_PPOINT 0x53

// =============================================================================
// MBTN_* — Mouse buttons (SDL3 button indices, passed to mouse_capture_on_button).
// =============================================================================

#define MBTN_LEFT 1
#define MBTN_MIDDLE 2
#define MBTN_RIGHT 3

// =============================================================================
// MAXIS_* — Mouse motion axes. Symbolic markers for the two axes returned by
// input_mouse_consume_rel(&dx, &dy) — MAXIS_X corresponds to dx, MAXIS_Y to dy.
// Constants here let action-map files (act_*.h) document which axis a given
// action reads, even though the API itself reads both axes in one call.
// =============================================================================

#define MAXIS_X 0
#define MAXIS_Y 1

// =============================================================================
// GBTN_* — Gamepad buttons (common Xbox / DualSense — indices into
// rgbButtons[0..16]). Names follow SDL3 positional convention; the DualSense
// and Xbox labels of each button are in the comment over the constant.
// =============================================================================

// DS: Cross, Xbox: A
#define GBTN_SOUTH 0
// DS: Circle, Xbox: B
#define GBTN_EAST 1
// DS: Square, Xbox: X
#define GBTN_WEST 2
// DS: Triangle, Xbox: Y
#define GBTN_NORTH 3
// DS: Create/Share, Xbox: Back/View
#define GBTN_SELECT 4
// DS: PS, Xbox: Guide
#define GBTN_GUIDE 5
// DS: Options, Xbox: Start/Menu
#define GBTN_START 6
// Left stick click. DS: L3, Xbox: LSB
#define GBTN_L3 7
// Right stick click. DS: R3, Xbox: RSB
#define GBTN_R3 8
// DS: L1, Xbox: LB
#define GBTN_L1 9
// DS: R1, Xbox: RB
#define GBTN_R1 10
#define GBTN_DPAD_UP    11
#define GBTN_DPAD_DOWN  12
#define GBTN_DPAD_LEFT  13
#define GBTN_DPAD_RIGHT 14
// L2 as digital button (threshold). DS: L2, Xbox: LT.
// For analog value see GTRIG_L2.
#define GBTN_L2_BTN 15
// R2 as digital button. DS: R2, Xbox: RT.
// For analog value see GTRIG_R2.
#define GBTN_R2_BTN 16

// =============================================================================
// DBTN_* — DualSense unique buttons (no Xbox equivalent).
// =============================================================================

// DualSense touchpad click.
#define DBTN_TOUCHPAD 17
// DualSense mute button.
#define DBTN_MUTE     18

// =============================================================================
// GAXIS_* — Gamepad stick analog axis ID. First arg to input_stick_x_axis() /
// input_stick_y_axis() (the X-or-Y choice is in the function name itself).
// Values mirror InputStickId in input_frame.h (implicit int → enum cast).
// =============================================================================

#define GAXIS_LEFT  0
#define GAXIS_RIGHT 1

// =============================================================================
// GDIR_DIR_* — Gamepad stick discrete direction. Second arg to
// input_stick_held() / input_stick_just_pressed() / etc. (first arg is a
// GAXIS_* stick id). Values mirror InputStickDir in input_frame.h.
// =============================================================================

#define GDIR_DIR_UP    0
#define GDIR_DIR_DOWN  1
#define GDIR_DIR_LEFT  2
#define GDIR_DIR_RIGHT 3

// =============================================================================
// GTRIG_* — Gamepad triggers analog (float 0.0 .. 1.0 via input_trigger(),
// byte 0..255 via input_trigger_raw()). Numeric value is the rgbButtons index
// of the corresponding digital threshold — same as GBTN_L2_BTN / GBTN_R2_BTN.
// =============================================================================

// L2 trigger analog. DS: L2, Xbox: LT.
#define GTRIG_L2 15
// R2 trigger analog. DS: R2, Xbox: RT.
#define GTRIG_R2 16

#endif // GAME_ACTION_MAP_INPUT_CODES_H
