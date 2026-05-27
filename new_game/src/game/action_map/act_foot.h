#ifndef GAME_ACTION_MAP_ACT_FOOT_H
#define GAME_ACTION_MAP_ACT_FOOT_H

// =============================================================================
// ACT_FOOT_* — action constants for gameplay on foot: movement, attacks, jump,
// camera, inventory, weapon switching, first-person aim, global gameplay
// hotkeys (F9 → open dev console).
//
// ESC → open pause menu lives in act_menu.h (ACT_MENU_TOGGLE_PAUSE_*) — it's
// a menu-context action triggered from gameplay, the menu owns the binding.
//
// Analog inputs (left stick for movement, right stick for camera, mouse
// motion, L2 tactical engagement) are read directly through the input_frame
// API (input_stick_x_axis / input_stick_y_axis / input_mouse_consume_rel /
// input_trigger_raw). These are NOT wrapped in ACT_* constants — they're
// streams of float / int data, not discrete bindings. Where the act-map view
// matters (e.g. "mouse X = camera yaw"), it's documented in a
// _MAXIS / _GAXIS constant below as a semantic tag.
//
// Naming rules, prefix table, suffix table → see
//   new_game_devlog/input_system/action_map/rules.md
// Step plan (this header is populated in step 3c.5) → see
//   new_game_devlog/input_system/action_map/plan.md
//
// Device codes used as values (KKEY_*, GBTN_*, GTRIG_*, MAXIS_*, ...) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// ---- Movement (keyboard digital; gamepad is analog left stick) -------------
// Arrow keys for movement on foot. Gamepad movement comes from the left stick
// (input_stick_x_axis(GAXIS_LEFT) / _y_axis(...)), packed into the
// input_mask analog bits — no GBTN constants for that path.

constexpr int ACT_FOOT_MOVE_FORWARD_KKEY  = KKEY_UP;
constexpr int ACT_FOOT_MOVE_BACKWARD_KKEY = KKEY_DOWN;
constexpr int ACT_FOOT_MOVE_LEFT_KKEY     = KKEY_LEFT;
constexpr int ACT_FOOT_MOVE_RIGHT_KKEY    = KKEY_RIGHT;

// ---- Combat: punch / kick / action / jump ----------------------------------
// PUNCH: keyboard Z (level read) + analog R2 trigger via weapon_feel.
// KICK: keyboard X + gamepad R1.
// ACTION (interact / get in or out of vehicle / pick up): keyboard C + Circle/B.
// JUMP: keyboard SPACE + Cross/A.

constexpr int ACT_FOOT_PUNCH_KKEY  = KKEY_Z;
constexpr int ACT_FOOT_KICK_KKEY   = KKEY_X;
constexpr int ACT_FOOT_ACTION_KKEY = KKEY_C;
constexpr int ACT_FOOT_JUMP_KKEY   = KKEY_SPACE;

constexpr int ACT_FOOT_JUMP_GBTN   = GBTN_SOUTH; // DS: Cross, Xbox: A
constexpr int ACT_FOOT_ACTION_GBTN = GBTN_EAST;  // DS: Circle, Xbox: B
constexpr int ACT_FOOT_KICK_GBTN   = GBTN_R1;    // DS: R1, Xbox: RB

// Analog R2 trigger drives the punch / shoot path (weapon_feel). Read as
// input_trigger_raw(GTRIG_R2) inside get_hardware_input.
constexpr int ACT_FOOT_PUNCH_GTRIG = GTRIG_R2;

// ---- L2 tactical mode (slow-walk + rolls) ----------------------------------
// L2 analog trigger engages tactical mode (slow walk + ✕ does rolls / backflips
// inside a 600 ms tap window). Read as input_trigger_raw(GTRIG_L2) with
// ~10% engage / ~5% release hysteresis.

constexpr int ACT_FOOT_TACTICAL_MODE_GTRIG = GTRIG_L2;

// ---- Inventory / weapon cycle (R3 / Enter) ---------------------------------
// Opens / advances the inventory wheel popup. Read in
// game_tick.cpp::process_controls AND in input_actions.cpp::get_hardware_input
// (the latter sets INPUT_MASK_SELECT from R3 hold).

constexpr int ACT_FOOT_INVENTORY_KKEY = KKEY_ENTER;
constexpr int ACT_FOOT_INVENTORY_GBTN = GBTN_R3; // DS: R3, Xbox: RSB

// ---- Weapon select: keyboard digit hotkeys ---------------------------------
// Direct weapon switch. KKEY_1 holsters (gun away), KKEY_2 draws pistol, the
// rest pick a specific SpecialUse weapon. Read in
// input_actions.cpp::apply_button_input.

constexpr int ACT_FOOT_WEAPON_HOLSTER_KKEY     = KKEY_1;
constexpr int ACT_FOOT_WEAPON_PISTOL_KKEY      = KKEY_2;
constexpr int ACT_FOOT_WEAPON_SHOTGUN_KKEY     = KKEY_3;
constexpr int ACT_FOOT_WEAPON_AK47_KKEY        = KKEY_4;
constexpr int ACT_FOOT_WEAPON_GRENADE_KKEY     = KKEY_5;
constexpr int ACT_FOOT_WEAPON_EXPLOSIVES_KKEY  = KKEY_6;
constexpr int ACT_FOOT_WEAPON_KNIFE_KKEY       = KKEY_7;
constexpr int ACT_FOOT_WEAPON_BAT_KKEY         = KKEY_8;

// ---- Weapon select: D-pad on gamepad ---------------------------------------
// Direct weapon select via D-pad. Read in game_tick.cpp::process_controls,
// gated by !cheat_combo_active so the cheat-direction selector (Select + L1
// + L2 + DPad) doesn't double-fire weapon switch.

constexpr int ACT_FOOT_WEAPON_PISTOL_GBTN       = GBTN_DPAD_UP;
constexpr int ACT_FOOT_WEAPON_AK47_GBTN         = GBTN_DPAD_LEFT;
constexpr int ACT_FOOT_WEAPON_SHOTGUN_GBTN      = GBTN_DPAD_RIGHT;
constexpr int ACT_FOOT_WEAPON_MELEE_CYCLE_GBTN  = GBTN_DPAD_DOWN;

// ---- Camera toggle (gamepad-only) ------------------------------------------
// Gamepad L1 sets INPUT_MASK_CAMERA (cycle next camera mode). Keyboard
// equivalents were removed when the new WASD + mouse layout took over —
// mouse handles look, so keyboard camera rotation / snap-behind bindings
// (F5/F6/F7/End/Del/PgDn) are gone. See
// new_game_devlog/input_system/keyboard_mouse_layout.md.

constexpr int ACT_FOOT_CAM_TOGGLE_GBTN = GBTN_L1; // DS: L1, Xbox: LB

// ---- Camera look: right stick + mouse (analog) -----------------------------
// Right stick X / Y and mouse relative motion drive free camera yaw / pitch.
// Read via input_stick_x_axis(GAXIS_RIGHT) and input_mouse_consume_rel.
// Constants here are SEMANTIC TAGS — no API uses them directly; they document
// which physical axis maps to which camera axis.

constexpr int ACT_FOOT_CAMERA_YAW_MAXIS   = MAXIS_X;
constexpr int ACT_FOOT_CAMERA_PITCH_MAXIS = MAXIS_Y;

// ---- First-person aim (L1 / KKEY_A held) -----------------------------------
// Holding L1 on gamepad or A on keyboard enters first-person aim mode.
// Same GBTN as ACT_FOOT_CAM_TOGGLE_GBTN — L1 doubles as camera-toggle press
// (edge) and aim modifier (hold). Two distinct ACT constants for the two
// semantics on the same button.

constexpr int ACT_FOOT_AIM_KKEY = KKEY_A;
constexpr int ACT_FOOT_AIM_GBTN = GBTN_L1;

// ---- First-person look (arrow keys while aiming) ---------------------------
// While in first-person aim, the arrow keys steer pitch / yaw of the look
// view independently of character movement. Same scancodes as ACT_FOOT_MOVE_*
// but different semantic — distinct ACT constants per rules.md.

constexpr int ACT_FOOT_AIM_LOOK_UP_KKEY    = KKEY_UP;
constexpr int ACT_FOOT_AIM_LOOK_DOWN_KKEY  = KKEY_DOWN;
constexpr int ACT_FOOT_AIM_LOOK_LEFT_KKEY  = KKEY_LEFT;
constexpr int ACT_FOOT_AIM_LOOK_RIGHT_KKEY = KKEY_RIGHT;

// ---- Start button (INPUT_MASK_START in gameplay) ---------------------------
// Used as a generic "menu / pause" gameplay-side flag in get_hardware_input.
// The actual pause-menu open lives in act_menu.h (ACT_MENU_TOGGLE_PAUSE_GBTN);
// this one just sets the INPUT_MASK_START bit so legacy gameplay logic that
// reads the mask sees the press.

constexpr int ACT_FOOT_START_GBTN = GBTN_START;

// ---- Open dev console (gameplay-level hotkey) ------------------------------
// F9 opens the dev console from gameplay. The console-internal text-input
// keys live in act_dev_console.h. Same KKEY appears in act_car.h —
// a separate constant per rules.md (different contexts).

constexpr int ACT_FOOT_OPEN_DEV_CONSOLE_KKEY = KKEY_F9;

// ---- Cheat combo (Select + L1 + L2 + D-pad direction) ----------------------
// Three-button hold gate (Select + L1 + L2_BTN) that, when active, lets the
// D-pad direction trigger a cheat. Modifier buttons + each D-pad direction
// have their own ACT names so the four cheats are explicit. D-pad here uses
// the same physical buttons as weapon select above — the cheat handler runs
// when the modifier gate is held, the weapon handler when it isn't (cheat
// combo is checked first).

constexpr int ACT_FOOT_CHEAT_MOD_SELECT_GBTN = GBTN_SELECT;
constexpr int ACT_FOOT_CHEAT_MOD_L1_GBTN     = GBTN_L1;
constexpr int ACT_FOOT_CHEAT_MOD_L2_BTN_GBTN = GBTN_L2_DIGITAL;

// Cheats (held with modifier gate above):
constexpr int ACT_FOOT_CHEAT_IMMORTAL_GBTN       = GBTN_DPAD_UP;
constexpr int ACT_FOOT_CHEAT_FULL_HEALTH_GBTN    = GBTN_DPAD_DOWN;
constexpr int ACT_FOOT_CHEAT_SPAWN_WEAPONS_GBTN  = GBTN_DPAD_LEFT;
constexpr int ACT_FOOT_CHEAT_MAX_AMMO_GBTN       = GBTN_DPAD_RIGHT;

#endif // GAME_ACTION_MAP_ACT_FOOT_H
