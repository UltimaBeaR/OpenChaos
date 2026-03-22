#ifndef UI_INTERFAC_H
#define UI_INTERFAC_H

// uc_orig: interfac.h (fallen/Headers/interfac.h)
// Player input system: hardware input reading, action dispatch, movement processing.
// Translates keyboard/gamepad state into game actions (INPUT_MASK_* / ACTION_*).

#include "core/types.h"

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
// Covers only the 18 digital button bits (0-17); bits 18-31 hold analog axis data.
// uc_orig: INPUT_MASK_ALL_BUTTONS (fallen/Headers/interfac.h)
#define INPUT_MASK_ALL_BUTTONS (0x3ffff)

// uc_orig: INPUT_CAR_ACCELERATE (fallen/Headers/interfac.h)
#define INPUT_CAR_ACCELERATE (INPUT_MASK_FORWARDS | INPUT_MASK_MOVE | INPUT_MASK_PUNCH)
// uc_orig: INPUT_CAR_DECELERATE (fallen/Headers/interfac.h)
#define INPUT_CAR_DECELERATE (INPUT_MASK_BACKWARDS | INPUT_MASK_KICK)
// uc_orig: INPUT_CAR_GOFASTER (fallen/Headers/interfac.h)
#define INPUT_CAR_GOFASTER (INPUT_CAR_ACCELERATE | INPUT_CAR_DECELERATE)
// uc_orig: INPUT_CAR_SIREN (fallen/Headers/interfac.h)
#define INPUT_CAR_SIREN (INPUT_MASK_JUMP)

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

// Logical button index into joypad_button_use[]/keybrd_button_use[].
// uc_orig: JOYPAD_BUTTON_KICK (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_KICK 0
// uc_orig: JOYPAD_BUTTON_PUNCH (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_PUNCH 1
// uc_orig: JOYPAD_BUTTON_JUMP (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_JUMP 2
// uc_orig: JOYPAD_BUTTON_ACTION (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_ACTION 3
// uc_orig: JOYPAD_BUTTON_MOVE (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_MOVE 4
// uc_orig: JOYPAD_BUTTON_START (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_START 5
// uc_orig: JOYPAD_BUTTON_SELECT (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_SELECT 6
// uc_orig: JOYPAD_BUTTON_CAMERA (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_CAMERA 7
// uc_orig: JOYPAD_BUTTON_CAM_LEFT (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_CAM_LEFT 8
// uc_orig: JOYPAD_BUTTON_CAM_RIGHT (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_CAM_RIGHT 9
// uc_orig: JOYPAD_BUTTON_1STPERSON (fallen/Headers/interfac.h)
#define JOYPAD_BUTTON_1STPERSON 10

// uc_orig: KEYBRD_BUTTON_LEFT (fallen/Headers/interfac.h)
#define KEYBRD_BUTTON_LEFT 11
// uc_orig: KEYBRD_BUTTON_RIGHT (fallen/Headers/interfac.h)
#define KEYBRD_BUTTON_RIGHT 12
// uc_orig: KEYBRD_BUTTON_FORWARDS (fallen/Headers/interfac.h)
#define KEYBRD_BUTTON_FORWARDS 13
// uc_orig: KEYBRD_BUTTON_BACK (fallen/Headers/interfac.h)
#define KEYBRD_BUTTON_BACK 14

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
// uc_orig: person_get_in_car (fallen/Headers/interfac.h)
extern SLONG person_get_in_car(Thing* p_person);
// uc_orig: person_get_in_specific_car (fallen/Headers/interfac.h)
extern SLONG person_get_in_specific_car(Thing* p_person, Thing* p_vehicle);
// uc_orig: get_hardware_input (fallen/Headers/interfac.h)
extern ULONG get_hardware_input(UWORD type);
// uc_orig: get_last_input (fallen/Headers/interfac.h)
extern ULONG get_last_input(UWORD type);
// uc_orig: allow_input_autorepeat (fallen/Headers/interfac.h)
extern void allow_input_autorepeat(void);

// Functions declared/used in interfac.cpp chunk 1 (migrated to new/ui/interfac.cpp).
// uc_orig: init_joypad_config (fallen/Source/interfac.cpp)
extern void init_joypad_config(void);
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
// uc_orig: player_stop_move (fallen/Source/interfac.cpp)
extern void player_stop_move(Thing* p_thing, ULONG input);
// uc_orig: get_analogue_dxdz (fallen/Source/interfac.cpp)
extern void get_analogue_dxdz(SLONG in_dx, SLONG in_dz, SLONG* dx, SLONG* dz);
// uc_orig: player_interface_move (fallen/Source/interfac.cpp)
extern void player_interface_move(Thing* p_thing, ULONG input);
// uc_orig: lock_to_compass (fallen/Source/interfac.cpp)
extern void lock_to_compass(Thing* p_thing);
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
// uc_orig: player_apply_move_analgue (fallen/Source/interfac.cpp)
extern void player_apply_move_analgue(Thing* p_thing, ULONG input);
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
// uc_orig: set_look_pitch (fallen/Source/interfac.cpp)
void set_look_pitch(SLONG p);
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
// uc_orig: remove_action_used (fallen/Source/interfac.cpp)
void remove_action_used(Thing* p_person);

#endif // UI_INTERFAC_H
