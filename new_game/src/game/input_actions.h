#ifndef GAME_INPUT_ACTIONS_H
#define GAME_INPUT_ACTIONS_H

// uc_orig: interfac.h (fallen/Headers/interfac.h)
// Player input system: hardware input reading, action dispatch, movement processing.
// Translates keyboard/gamepad state into game actions (INPUT_MASK_* / ACTION_*).

#include "engine/core/types.h"
#include "game/action_map/input_codes.h" // VAXIS_X / VAXIS_Y for input_virtual_axis

// Forward declarations
struct Thing;

// uc_orig: INPUT_FORWARDS (fallen/Headers/interfac.h)
#define INPUT_FORWARDS 0
// uc_orig: INPUT_BACKWARDS (fallen/Headers/interfac.h)
#define INPUT_BACKWARDS 1
// uc_orig: INPUT_LEFT (fallen/Headers/interfac.h)
#define INPUT_LEFT 2
// uc_orig: INPUT_RIGHT (fallen/Headers/interfac.h)
#define INPUT_RIGHT 3
// uc_orig: INPUT_START (fallen/Headers/interfac.h)
#define INPUT_START 4
// uc_orig: INPUT_CANCEL (fallen/Headers/interfac.h)
#define INPUT_CANCEL 5
// uc_orig: INPUT_PUNCH (fallen/Headers/interfac.h)
#define INPUT_PUNCH 6
// uc_orig: INPUT_KICK (fallen/Headers/interfac.h)
#define INPUT_KICK 7
// uc_orig: INPUT_ACTION (fallen/Headers/interfac.h)
#define INPUT_ACTION 8
// uc_orig: INPUT_JUMP (fallen/Headers/interfac.h)
#define INPUT_JUMP 9
// uc_orig: INPUT_CAMERA (fallen/Headers/interfac.h)
#define INPUT_CAMERA 10
// uc_orig: INPUT_CAM_LEFT (fallen/Headers/interfac.h)
#define INPUT_CAM_LEFT 11
// uc_orig: INPUT_CAM_RIGHT (fallen/Headers/interfac.h)
#define INPUT_CAM_RIGHT 12
// uc_orig: INPUT_CAM_BEHIND (fallen/Headers/interfac.h)
#define INPUT_CAM_BEHIND 13
// uc_orig: INPUT_MOVE (fallen/Headers/interfac.h)
#define INPUT_MOVE 14
// uc_orig: INPUT_SELECT (fallen/Headers/interfac.h)
#define INPUT_SELECT 15
// uc_orig: INPUT_STEP_LEFT (fallen/Headers/interfac.h)
#define INPUT_STEP_LEFT 16
// uc_orig: INPUT_STEP_RIGHT (fallen/Headers/interfac.h)
#define INPUT_STEP_RIGHT 17

// Bitmask versions of INPUT_* constants. Bits 18-31 are reserved for analog stick values.
// uc_orig: INPUT_MASK_FORWARDS (fallen/Headers/interfac.h)
#define INPUT_MASK_FORWARDS (1 << INPUT_FORWARDS)
// uc_orig: INPUT_MASK_BACKWARDS (fallen/Headers/interfac.h)
#define INPUT_MASK_BACKWARDS (1 << INPUT_BACKWARDS)
// uc_orig: INPUT_MASK_LEFT (fallen/Headers/interfac.h)
#define INPUT_MASK_LEFT (1 << INPUT_LEFT)
// uc_orig: INPUT_MASK_START (fallen/Headers/interfac.h)
#define INPUT_MASK_START (1 << INPUT_START)
// uc_orig: INPUT_MASK_CANCEL (fallen/Headers/interfac.h)
#define INPUT_MASK_CANCEL (1 << INPUT_CANCEL)
// uc_orig: INPUT_MASK_RIGHT (fallen/Headers/interfac.h)
#define INPUT_MASK_RIGHT (1 << INPUT_RIGHT)
// uc_orig: INPUT_MASK_PUNCH (fallen/Headers/interfac.h)
#define INPUT_MASK_PUNCH (1 << INPUT_PUNCH)
// uc_orig: INPUT_MASK_DOMENU (fallen/Headers/interfac.h)
#define INPUT_MASK_DOMENU (INPUT_MASK_PUNCH)
// uc_orig: INPUT_MASK_KICK (fallen/Headers/interfac.h)
#define INPUT_MASK_KICK (1 << INPUT_KICK)
// uc_orig: INPUT_MASK_ACTION (fallen/Headers/interfac.h)
#define INPUT_MASK_ACTION (1 << INPUT_ACTION)
// uc_orig: INPUT_MASK_JUMP (fallen/Headers/interfac.h)
#define INPUT_MASK_JUMP (1 << INPUT_JUMP)
// uc_orig: INPUT_MASK_CAMERA (fallen/Headers/interfac.h)
#define INPUT_MASK_CAMERA (1 << INPUT_CAMERA)
// uc_orig: INPUT_MASK_CAM_LEFT (fallen/Headers/interfac.h)
#define INPUT_MASK_CAM_LEFT (1 << INPUT_CAM_LEFT)
// uc_orig: INPUT_MASK_CAM_RIGHT (fallen/Headers/interfac.h)
#define INPUT_MASK_CAM_RIGHT (1 << INPUT_CAM_RIGHT)
// uc_orig: INPUT_MASK_CAM_BEHIND (fallen/Headers/interfac.h)
#define INPUT_MASK_CAM_BEHIND (1 << INPUT_CAM_BEHIND)
// uc_orig: INPUT_MASK_MOVE (fallen/Headers/interfac.h)
#define INPUT_MASK_MOVE (1 << INPUT_MOVE)
// uc_orig: INPUT_MASK_SELECT (fallen/Headers/interfac.h)
#define INPUT_MASK_SELECT (1 << INPUT_SELECT)
// uc_orig: INPUT_MASK_STEP_LEFT (fallen/Headers/interfac.h)
#define INPUT_MASK_STEP_LEFT (1 << INPUT_STEP_LEFT)
// uc_orig: INPUT_MASK_STEP_RIGHT (fallen/Headers/interfac.h)
#define INPUT_MASK_STEP_RIGHT (1 << INPUT_STEP_RIGHT)

// OpenChaos: SPRINT and STEALTH split off the old single ACTION button onto
// their own buttons (see act_foot.h). They need their own bits in the per-tick
// Input word (so the action dispatch and replays stay deterministic), but bits
// 18-31 are the packed analog stick and there are no spare button bits — except
// STEP_LEFT/STEP_RIGHT (the sidestep inputs), which are dead in the WASD+mouse
// scheme (only commented-out references remain). Reuse those two bits.
#define INPUT_SPRINT       INPUT_STEP_LEFT
#define INPUT_STEALTH      INPUT_STEP_RIGHT
#define INPUT_MASK_SPRINT  INPUT_MASK_STEP_LEFT
#define INPUT_MASK_STEALTH INPUT_MASK_STEP_RIGHT
// Covers only the 18 digital button bits (0-17); bits 18-31 hold analog axis data.
// uc_orig: INPUT_MASK_ALL_BUTTONS (fallen/Headers/interfac.h)
#define INPUT_MASK_ALL_BUTTONS (0x3ffff)

// Unpacks one axis of the virtual movement-intent stick (VAXIS_X / VAXIS_Y,
// see input_codes.h) from the packed gameplay input word. Bits 18-31 are
// filled by get_hardware_input from either the gamepad left stick or the
// keyboard WASD emulation, so this is the device-agnostic "movement intent"
// read used by foot turning / movement, car steering and grapple-forward
// detection. Reaction sites pass an ACT_*_VAXIS constant naming what they use
// it for (ACT_FOOT_MOVE_X_VAXIS, ACT_FOOT_MOVE_Y_VAXIS, ACT_CAR_STEER_VAXIS).
//
// Behaviour is byte-identical to the GET_JOYX / GET_JOYY macros it replaces:
// same bit layout (X at >>17, Y at >>24), same signed range (even values,
// -128..+126; the low bit is masked off by & 0xfe).
// uc_orig: GET_JOYX / GET_JOYY (fallen/Source/interfac.cpp)
inline SLONG input_virtual_axis(ULONG input, SLONG vaxis)
{
    const SLONG shift = (vaxis == VAXIS_X) ? 17 : 24;
    return (SLONG)(((input >> shift) & 0xfe) - 128);
}

// Vehicle button mapping — two variants from the original (fallen/Headers/interfac.h).
// Keyboard uses the PC variant; gamepad uses the PSX variant (matches PS1 Config 0).
// apply_button_input_car() picks the right set based on active_input_device.
//
// Keyboard (new layout): W=gas, Space=brake, S=reverse, A/D=steer, E=siren.
// Gas/brake/reverse are read DIRECTLY from their keys via input_key_held in
// apply_button_input_car (W and S are opposite ends of the same WASD stick
// axis, so reading them as mask bits would cancel when both are held). So no
// KB accelerate/brake/reverse masks are needed here. Only the siren cancel
// sentinel below is mask-based.
// "Siren cancel" sentinel for vehicle.cpp::pedals InputDone tagging. Siren
// itself is read via input_key_press_pending(ACT_CAR_SIREN_KKEY) — direct
// E-key read, NOT via this mask. The mask bit only serves as a unique
// identifier so do_car_input's `move_cancel == INPUT_CAR_KB_SIREN` check
// can distinguish "siren just fired" from any other move_cancel value.
// INPUT_MASK_SELECT (Tab inventory cycle) is unused in-car (can't cycle
// weapons while driving) so using it here has no observable side effect.
#define INPUT_CAR_KB_SIREN (INPUT_MASK_SELECT)
//
// Gamepad gas/brake/reverse are read directly via input_btn_held (R2 / L1 /
// L2) in apply_button_input_car — they don't go through input-mask bits, so
// no PAD accelerate/brake/reverse masks are needed. Only the siren cancel
// sentinel below is mask-based (same role as INPUT_CAR_KB_SIREN).
// uc_orig: INPUT_CAR_SIREN (fallen/Headers/interfac.h, PSX variant)
#define INPUT_CAR_PAD_SIREN (INPUT_MASK_KICK)

// uc_orig: INPUT_MASKM_CAM_TYPE (fallen/Headers/interfac.h)
#define INPUT_MASKM_CAM_TYPE (INPUT_MASK_CAM_LEFT | INPUT_MASK_CAM_RIGHT)
// uc_orig: INPUT_MASKM_CAM_SHIFT (fallen/Headers/interfac.h)
#define INPUT_MASKM_CAM_SHIFT (INPUT_CAM_LEFT)
// uc_orig: INPUT_MASKM_CAM1 (fallen/Headers/interfac.h)
#define INPUT_MASKM_CAM1 (0)
// uc_orig: INPUT_MASKM_CAM2 (fallen/Headers/interfac.h)
#define INPUT_MASKM_CAM2 (INPUT_MASK_CAM_LEFT)
// uc_orig: INPUT_MASKM_CAM3 (fallen/Headers/interfac.h)
#define INPUT_MASKM_CAM3 (INPUT_MASK_CAM_RIGHT)
// uc_orig: INPUT_MASKM_CAM4 (fallen/Headers/interfac.h)
#define INPUT_MASKM_CAM4 (INPUT_MASK_CAM_LEFT | INPUT_MASK_CAM_RIGHT)

// uc_orig: INPUT_MASK_DIR (fallen/Headers/interfac.h)
#define INPUT_MASK_DIR (INPUT_MASK_FORWARDS | INPUT_MASK_BACKWARDS | INPUT_MASK_LEFT | INPUT_MASK_RIGHT)

// State-transition table entry for player action dispatch.
// Logic == 0: any bit in Input triggers the action.
// Logic == 1: all bits in Input must be set.
// uc_orig: ActionInfo (fallen/Source/interfac.cpp)
struct ActionInfo {
    UBYTE Action;
    UBYTE Logic;
    ULONG Input;
};

// Dispatch table indexed by ACTION_* constant.
// uc_orig: action_tree (fallen/Source/interfac.cpp)
extern struct ActionInfo* action_tree[];

// Input type flags for get_hardware_input().
// uc_orig: INPUT_TYPE_KEY (fallen/Headers/interfac.h)
#define INPUT_TYPE_KEY (1 << 0)
// uc_orig: INPUT_TYPE_JOY (fallen/Headers/interfac.h)
#define INPUT_TYPE_JOY (1 << 1)
// When combined: only return buttons that went down since the last poll (for menus).
// uc_orig: INPUT_TYPE_GONEDOWN (fallen/Headers/interfac.h)
#define INPUT_TYPE_GONEDOWN (1 << 6)
// uc_orig: INPUT_TYPE_ALL (fallen/Headers/interfac.h)
#define INPUT_TYPE_ALL (INPUT_TYPE_KEY | INPUT_TYPE_JOY)

// NOTE: The original interfac.h declared apply_button_input with 2 args (Thing*, SLONG input),
// but the actual definition in interfac.cpp has 3 args (player, person, ULONG input).
// The 3-arg declaration below matches the definition.
// uc_orig: apply_button_input (fallen/Headers/interfac.h)
extern ULONG apply_button_input(struct Thing* p_player, struct Thing* p_person, ULONG input);
// uc_orig: process_hardware_level_input_for_player (fallen/Headers/interfac.h)
extern void process_hardware_level_input_for_player(Thing* p_thing);
// uc_orig: init_user_interface (fallen/Headers/interfac.h)
extern void init_user_interface(void);
// uc_orig: continue_action (fallen/Headers/interfac.h)
extern SLONG continue_action(Thing* p_person);
// uc_orig: continue_moveing (fallen/Headers/interfac.h)
extern SLONG continue_moveing(Thing* p_person);
// uc_orig: continue_firing (fallen/Headers/interfac.h)
extern SLONG continue_firing(Thing* p_person);
// AK47 reload gate: see input_actions.cpp s_ak47_reload_gate for details.
// mark: called from person.cpp fire paths on HAD_TO_CHANGE_CLIP.
// clear: called from person.cpp after a real shot, and from
//        weapon_feel_evaluate_fire on physical R2 release.
// set: game.cpp queries to extend mag_empty through the reload-press
//      window so the Weapon25 click effect stays active until release.
extern void input_actions_mark_ak47_reload_gate();
extern void input_actions_clear_ak47_reload_gate();
extern bool input_actions_ak47_reload_gate_set();
// uc_orig: person_get_in_car (fallen/Headers/interfac.h)
extern SLONG person_get_in_car(Thing* p_person);
// uc_orig: person_get_in_specific_car (fallen/Headers/interfac.h)
extern SLONG person_get_in_specific_car(Thing* p_person, Thing* p_vehicle);
// uc_orig: get_hardware_input (fallen/Headers/interfac.h)
extern ULONG get_hardware_input(UWORD type);
// Functions declared/used in interfac.cpp chunk 1 (migrated to new/ui/interfac.cpp).
// uc_orig: do_an_action (fallen/Source/interfac.cpp)
extern ULONG do_an_action(Thing* p_thing, ULONG input);
// uc_orig: player_activate_in_hand (fallen/Source/interfac.cpp)
extern SLONG player_activate_in_hand(Thing* p_person);
// uc_orig: set_player_shoot (fallen/Source/interfac.cpp)
extern void set_player_shoot(Thing* p_person, SLONG param);
// uc_orig: set_player_punch (fallen/Source/interfac.cpp)
extern void set_player_punch(Thing* p_person);
// uc_orig: should_i_jump (fallen/Source/interfac.cpp)
extern SLONG should_i_jump(Thing* darci);
// uc_orig: should_person_backflip (fallen/Source/interfac.cpp)
extern SLONG should_person_backflip(Thing* darci);
// uc_orig: bad_place_for_car (fallen/Source/interfac.cpp)
extern SLONG bad_place_for_car(Thing* p_person, Thing* p_vehicle);
// uc_orig: get_car_enter_xz (fallen/Source/interfac.cpp)
extern void get_car_enter_xz(Thing* p_vehicle, SLONG door, SLONG* cx, SLONG* cz);
// uc_orig: in_right_place_for_car (fallen/Source/interfac.cpp)
extern SLONG in_right_place_for_car(Thing* p_person, Thing* p_vehicle, SLONG* door);
// uc_orig: find_best_action_from_tree (fallen/Source/interfac.cpp)
extern SLONG find_best_action_from_tree(SLONG action, ULONG input, ULONG* input_used);
// uc_orig: get_camera_angle (fallen/Source/interfac.cpp)
extern SLONG get_camera_angle(void);

// OpenChaos: returns true while the gamepad L2 trigger is held past the
// engagement hysteresis (~10 % press, ~5 % release — see get_hardware_input).
// Read by external systems that need to suppress auto behaviour while the
// player holds the BACK-WALK modifier (gamepad L1 / keyboard E) — the camera
// get-behind block (fc.cpp) freezes auto-positioning so it doesn't swing behind
// while retreating, and the bench-sit collision (collide.cpp) is lenient about
// backing into a bench.
extern bool input_backwalk_held(void);
// uc_orig: player_stop_move (fallen/Source/interfac.cpp)
extern void player_stop_move(Thing* p_thing, ULONG input);
// uc_orig: player_interface_move (fallen/Source/interfac.cpp)
extern void player_interface_move(Thing* p_thing, ULONG input);

// Flag for get_joy_angle: adjust the returned angle relative to the camera yaw.
// uc_orig: JOY_REL_CAMERA (fallen/Source/interfac.cpp)
#define JOY_REL_CAMERA (1 << 0)
// uc_orig: get_joy_angle (fallen/Source/interfac.cpp)
extern SLONG get_joy_angle(ULONG input, UWORD flags);
// uc_orig: player_turn_left_right_analogue (fallen/Source/interfac.cpp)
extern SLONG player_turn_left_right_analogue(Thing* p_thing, SLONG input);
// uc_orig: process_analogue_movement (fallen/Source/interfac.cpp)
extern void process_analogue_movement(Thing* p_thing, SLONG input);
// uc_orig: player_turn_left_right (fallen/Source/interfac.cpp)
extern SLONG player_turn_left_right(Thing* p_thing, SLONG input);
// uc_orig: player_apply_move (fallen/Source/interfac.cpp)
extern void player_apply_move(Thing* p_thing, ULONG input);
// uc_orig: person_enter_fight_mode (fallen/Source/interfac.cpp)
extern void person_enter_fight_mode(Thing* p_person);

// The two-argument form declared in the original interfac.h is different from the
// actual three-argument definition in interfac.cpp; the declaration below matches the definition.
// uc_orig: apply_button_input (fallen/Source/interfac.cpp)
ULONG apply_button_input(Thing* p_player, Thing* p_person, ULONG input);
// uc_orig: apply_button_input_fight (fallen/Source/interfac.cpp)
ULONG apply_button_input_fight(Thing* p_player, Thing* p_person, ULONG input);
// uc_orig: apply_button_input_car (fallen/Source/interfac.cpp)
ULONG apply_button_input_car(Thing* p_furn, ULONG input);
// uc_orig: apply_button_input_first_person (fallen/Source/interfac.cpp)
ULONG apply_button_input_first_person(Thing* p_player, Thing* p_person, ULONG input, ULONG* processed);
// uc_orig: can_darci_change_weapon (fallen/Source/interfac.cpp)
SLONG can_darci_change_weapon(Thing* p_person);
// uc_orig: pre_process_input (fallen/Source/interfac.cpp)
ULONG pre_process_input(SLONG mode, ULONG input);
// uc_orig: continue_pressing_action (fallen/Source/interfac.cpp)
SLONG continue_pressing_action(Thing* p_person);
// uc_orig: set_action_used (fallen/Source/interfac.cpp)
void set_action_used(Thing* p_person);
// uc_orig: continue_dir (fallen/Source/interfac.cpp)
SLONG continue_dir(Thing* p_person, SLONG dir);
// uc_orig: continue_firing (fallen/Source/interfac.cpp)
SLONG continue_firing(Thing* p_person);
// uc_orig: continue_moveing (fallen/Source/interfac.cpp)
SLONG continue_moveing(Thing* p_person);
// uc_orig: continue_blocking (fallen/Source/interfac.cpp)
SLONG continue_blocking(Thing* p_person);

// ---- Cheat helpers ---------------------------------------------------------
// Apply one of the four port-from-Dreamcast cheats. Used by the gamepad
// Select+L1+L2+DPad combo (input_actions.cpp::get_hardware_input) AND by the
// keyboard F9-console commands (game_tick.cpp::parse_console). Each function
// performs the same gameplay effect AND prints the corresponding on-screen
// message — see GAMEPLAY_CHANGES.md "Геймпад-читы" for the message text and
// new_game_devlog/input_system/keyboard_mouse_layout.md for the keyboard
// command names (bloodofkings / shieldofsteel / weneedguns / losttrack).
void cheat_apply_immortal_toggle();
void cheat_apply_full_health();
void cheat_apply_spawn_weapons();
void cheat_apply_max_ammo();
#endif // GAME_INPUT_ACTIONS_H
