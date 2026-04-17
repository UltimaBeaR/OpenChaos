// Player input processing: hardware input reading, action dispatch, movement.
// Translates keyboard/gamepad state into game actions for the player character.

#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "engine/debug/input_debug/input_debug.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/io/env.h"
#include "things/core/thing.h"
#include "things/core/thing_globals.h"
#include "things/core/player.h"
#include "things/core/player_globals.h"
#include "things/core/interact.h"
#include "things/core/state.h"
#include "things/core/hierarchy.h"
#include "things/characters/person.h"
#include "things/items/special.h"
#include "things/items/hook.h"
#include "things/vehicles/vehicle.h"
#include "map/ob.h"
#include "map/pap.h"
#include "buildings/ware.h"
#include "buildings/id.h"
#include "ai/mav.h"
#include "ai/pcom.h"
#include "missions/eway.h"
#include "missions/eway_globals.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "world_objects/dirt.h"

#include "map/supermap.h"
#include "map/level_pools.h"
// actors/characters/person.h already included above (line 14)

#include "things/characters/anim_ids.h"
#include "things/core/statedef.h"
#include "engine/physics/collide.h"
#include "combat/combat.h"
#include "engine/input/joystick.h"             // ReadInputDevice
#include "engine/input/joystick_globals.h"  // the_state (GamepadState)
#include "game/game_tick_globals.h"         // allow_debug_keys, dkeys_have_been_used
#include "engine/input/gamepad_globals.h"   // active_input_device
#include "engine/input/weapon_feel.h"       // weapon_feel_evaluate_fire
// Engine.h removed: SIN/COS/QDIST2 come transitively via MFStdLib→StdMaths→core/math.h.
#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"
#include "camera/cam.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/mouse_globals.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "engine/graphics/pipeline/aeng.h"       // MSG_add
#include "engine/console/console.h"   // CONSOLE_text_at

// Forward declaration for gang-attack reset (defined in pcom.cpp, not yet in pcom.h).
// uc_orig: reset_gang_attack (fallen/Source/pcom.cpp)
extern void reset_gang_attack(Thing* p_target);

extern SLONG set_person_search(Thing* p_person, SLONG ob_index, SLONG ox, SLONG oy, SLONG oz);
extern SLONG set_person_search_corpse(Thing* p_person, Thing* p_personb);
extern SLONG find_searchable_person(Thing* p_person);

// Analog stick X axis: packed into bits 18-24 of the input ULONG, range -128..+127.
// uc_orig: GET_JOYX (fallen/Source/interfac.cpp)
#define GET_JOYX(input) (((input >> 17) & 0xfe) - 128)
// Analog stick Y axis: packed into bits 25-31 of the input ULONG, range -128..+127.
// uc_orig: GET_JOYY (fallen/Source/interfac.cpp)
#define GET_JOYY(input) (((input >> 24) & 0xfe) - 128)

// Minimum analog stick velocity below which movement is ignored (game logic deadzone).
// PC: 8/128. The hardware-level deadzone is NOISE_TOLERANCE in get_hardware_input().
// uc_orig: ANALOGUE_MIN_VELOCITY (fallen/Source/interfac.cpp)
#define ANALOGUE_MIN_VELOCITY 8

// uc_orig: INPUT_KEYS (fallen/Source/interfac.cpp)
#define INPUT_KEYS 0
// uc_orig: INPUT_JOYPAD (fallen/Source/interfac.cpp)
#define INPUT_JOYPAD 1

// On PC, inventory mode is purely graphical (no effect on movement). Always 0.
// uc_orig: CONTROLS_inventory_mode (fallen/Source/interfac.cpp)
#define CONTROLS_inventory_mode 0

// Internal masks separating movement bits (0-5 + MOVE) from action bits.
// uc_orig: INPUT_MOVEMENT_MASK (fallen/Source/interfac.cpp)
#define INPUT_MOVEMENT_MASK ((0x3f) | INPUT_MASK_MOVE)
// uc_orig: INPUT_ACTION_MASK (fallen/Source/interfac.cpp)
#define INPUT_ACTION_MASK (~0x3f)

// Action tables: each array lists valid transitions from a specific player action/state.
// Indexed via action_tree[] below.
// uc_orig: action_idle (fallen/Source/interfac.cpp)
struct ActionInfo action_idle[] = {
    { ACTION_FLIP_LEFT, 1, INPUT_MASK_JUMP | INPUT_MASK_LEFT },
    { ACTION_FLIP_RIGHT, 1, INPUT_MASK_JUMP | INPUT_MASK_RIGHT },
    { ACTION_STANDING_JUMP, 0, INPUT_MASK_JUMP },
    { ACTION_FIGHT_PUNCH, 0, INPUT_MASK_PUNCH },
    { ACTION_FIGHT_KICK, 0, INPUT_MASK_KICK },
    { 0 }
};

// uc_orig: action_walk (fallen/Source/interfac.cpp)
struct ActionInfo action_walk[] = {
    { ACTION_RUN, 0, 0 },
    { ACTION_FIGHT_PUNCH, 0, INPUT_MASK_PUNCH },
    { ACTION_FIGHT_KICK, 0, INPUT_MASK_KICK },
    { ACTION_STANDING_JUMP, 0, INPUT_MASK_JUMP },
    { 0 }
};

// uc_orig: action_walk_back (fallen/Source/interfac.cpp)
struct ActionInfo action_walk_back[] = {
    { ACTION_FIGHT_PUNCH, 0, INPUT_MASK_PUNCH },
    { ACTION_FIGHT_KICK, 0, INPUT_MASK_KICK },
    { ACTION_STANDING_JUMP, 0, INPUT_MASK_JUMP },
    { 0 }
};

// uc_orig: action_run (fallen/Source/interfac.cpp)
struct ActionInfo action_run[] = {
    { ACTION_WALK, 0, 0 },
    { ACTION_RUNNING_JUMP, 0, INPUT_MASK_JUMP },
    { ACTION_SKID, 0, INPUT_MASK_KICK },
    { ACTION_SHOOT, 0, INPUT_MASK_PUNCH },
    { 0 }
};

// uc_orig: action_standing_jump (fallen/Source/interfac.cpp)
struct ActionInfo action_standing_jump[] = {
    { ACTION_FLIP_LEFT, 1, INPUT_MASK_LEFT },
    { ACTION_FLIP_RIGHT, 1, INPUT_MASK_RIGHT },
    { ACTION_FIGHT_KICK, 0, INPUT_MASK_KICK },
    { 0, 0, 0 }
};

// uc_orig: action_standing_jump_grab (fallen/Source/interfac.cpp)
struct ActionInfo action_standing_jump_grab[] = {
    { 0, 0, 0 }
};

// uc_orig: action_running_jump (fallen/Source/interfac.cpp)
struct ActionInfo action_running_jump[] = {
    { 0, 0, 0 }
};

// uc_orig: action_dangling (fallen/Source/interfac.cpp)
struct ActionInfo action_dangling[] = {
    { ACTION_PULL_UP, 0, INPUT_MASK_FORWARDS },
    { ACTION_PULL_UP, 0, INPUT_MASK_MOVE },
    { ACTION_DROP_DOWN, 0, INPUT_MASK_BACKWARDS },
    { ACTION_TRAVERSE_LEFT, 0, INPUT_MASK_LEFT },
    { ACTION_TRAVERSE_RIGHT, 0, INPUT_MASK_RIGHT },
    { 0, 0, 0 }
};

// uc_orig: action_climbing (fallen/Source/interfac.cpp)
struct ActionInfo action_climbing[] = {
    { ACTION_DROP_DOWN, 0, INPUT_MASK_JUMP },
    { 0, 0, 0 }
};

// uc_orig: action_cable (fallen/Source/interfac.cpp)
struct ActionInfo action_cable[] = {
    { ACTION_DROP_DOWN, 0, INPUT_MASK_JUMP },
    { 0, 0, 0 }
};

// uc_orig: action_aim_gun (fallen/Source/interfac.cpp)
struct ActionInfo action_aim_gun[] = {
    { ACTION_SHOOT, 0, INPUT_MASK_PUNCH },
    { ACTION_STANDING_JUMP, 0, INPUT_MASK_JUMP },
    { 0, 0, 0 }
};

// uc_orig: action_shoot (fallen/Source/interfac.cpp)
struct ActionInfo action_shoot[] = {
    { ACTION_SHOOT, 0, INPUT_MASK_PUNCH },
    { 0, 0, 0 }
};

// uc_orig: action_pull_up (fallen/Source/interfac.cpp)
struct ActionInfo action_pull_up[] = {
    { 0, 0, 0 }
};

// uc_orig: action_grabbing_ledge (fallen/Source/interfac.cpp)
struct ActionInfo action_grabbing_ledge[] = {
    { 0, 0, 0 }
};

// uc_orig: action_death_slide (fallen/Source/interfac.cpp)
struct ActionInfo action_death_slide[] = {
    { ACTION_DROP_DOWN, 0, INPUT_MASK_JUMP },
    { 0, 0, 0 }
};

// uc_orig: action_run_jump (fallen/Source/interfac.cpp)
struct ActionInfo action_run_jump[] = {
    { ACTION_KICK_FLAG, 0, INPUT_MASK_KICK },
    { 0, 0, 0 }
};

// uc_orig: action_fight (fallen/Source/interfac.cpp)
struct ActionInfo action_fight[] = {
    { ACTION_KICK_FLAG, 0, INPUT_MASK_KICK },
    { ACTION_PUNCH_FLAG, 0, INPUT_MASK_PUNCH },
    { ACTION_BLOCK_FLAG, 0, INPUT_MASK_ACTION },
    { 0, 0, 0 }
};

// uc_orig: action_dying (fallen/Source/interfac.cpp)
// ASan fix: original was missing {0,0,0} terminator — find_best_action_from_tree() reads
// past array end. Original bug in MuckyFoot code, worked on x86 by luck (padding zeros).
struct ActionInfo action_dying[] = {
    { ACTION_BLOCK_FLAG, 0, INPUT_MASK_ACTION },
    { 0, 0, 0 }
};

// uc_orig: action_grapple (fallen/Source/interfac.cpp)
struct ActionInfo action_grapple[] = {
    { ACTION_KICK_FLAG, 0, INPUT_MASK_KICK },
    { ACTION_PUNCH_FLAG, 0, INPUT_MASK_PUNCH },
    { 0, 0, 0 }
};

// uc_orig: action_grapplee (fallen/Source/interfac.cpp)
struct ActionInfo action_grapplee[] = {
    { ACTION_BLOCK_FLAG, 0, INPUT_MASK_ACTION },
    { 0, 0, 0 }
};

// uc_orig: action_stand_relax (fallen/Source/interfac.cpp)
struct ActionInfo action_stand_relax[] = {
    { ACTION_STANDING_JUMP, 0, INPUT_MASK_JUMP },
    { ACTION_STANDING_JUMP_GRAB, 0, INPUT_MASK_JUMP },
    { ACTION_WALK_BACK, 0, INPUT_MASK_BACKWARDS },
    { 0, 0, 0 }
};

// uc_orig: action_ladder (fallen/Source/interfac.cpp)
struct ActionInfo action_ladder[] = {
    { ACTION_DROP_DOWN, 0, INPUT_MASK_JUMP },
    { 0, 0, 0 }
};

// uc_orig: action_dead (fallen/Source/interfac.cpp)
struct ActionInfo action_dead[] = {
    { ACTION_BLOCK_FLAG, 0, INPUT_MASK_ACTION },
    { 0, 0, 0 }
};

// uc_orig: action_flip_left (fallen/Source/interfac.cpp)
struct ActionInfo action_flip_left[] = {
    { ACTION_FLIP_LEFT, 1, INPUT_MASK_LEFT },
    { 0, 0, 0 }
};

// uc_orig: action_flip_right (fallen/Source/interfac.cpp)
struct ActionInfo action_flip_right[] = {
    { ACTION_FLIP_RIGHT, 1, INPUT_MASK_RIGHT },
    { 0, 0, 0 }
};

// uc_orig: action_hug_wall (fallen/Source/interfac.cpp)
struct ActionInfo action_hug_wall[] = {
    { ACTION_HUG_RIGHT, 1, INPUT_MASK_RIGHT },
    { ACTION_HUG_LEFT, 1, INPUT_MASK_LEFT },
    { 0, 0, 0 }
};

// uc_orig: action_sit (fallen/Source/interfac.cpp)
struct ActionInfo action_sit[] = {
    { ACTION_UNSIT, 0, INPUT_MASK_FORWARDS },
    { ACTION_UNSIT, 0, INPUT_MASK_MOVE },
    { 0, 0, 0 }
};

// Dispatch table: indexed by ACTION_* constant, each entry points to the valid
// transitions for that state. NULL = no player input accepted in this state.
// uc_orig: action_tree (fallen/Source/interfac.cpp)
struct ActionInfo* action_tree[] = {
    action_idle,
    action_walk,
    action_run,
    action_standing_jump,
    action_standing_jump_grab,
    action_running_jump,
    action_dangling,
    action_pull_up,
    action_stand_relax,
    action_grabbing_ledge,
    0,
    0,
    0,
    action_climbing, // 13
    action_fight,
    action_fight,
    action_idle, // 16 action_idle_fight
    action_cable, // 17 action cable
    0,
    0,
    action_dying, // dying
    0,
    action_aim_gun,
    action_shoot, // Shoot gun
    0,  // Gun Away
    0,  // Respawn
    action_dead, // Dead
    0,  // 27
    0,  // flip right
    action_idle,
    0,
    action_run_jump,
    0,
    0,
    action_walk_back,
    action_death_slide,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    action_grapple,
    action_grapplee,
    0,  // Enter vehicle
    0,  // Inside vehicle
    action_sit, // Sit bench
    action_hug_wall, // hug wall
    0,  // hug left
    0,  // hug right
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

// Reads joypad and keyboard button mappings from config.ini via ENV_get_value_number.
// Fills joypad_button_use[] and keybrd_button_use[] with physical button indices
// for each logical function (JOYPAD_BUTTON_*).
// uc_orig: init_joypad_config (fallen/Source/interfac.cpp)
void init_joypad_config(void)
{
    /*
            //was
            joypad_button_use[JOYPAD_BUTTON_KICK]		= ENV_get_value_number("joypad_kick",		2, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_PUNCH]		= ENV_get_value_number("joypad_punch",		3, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_JUMP]		= ENV_get_value_number("joypad_jump",		0, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_ACTION]		= ENV_get_value_number("joypad_action",		1, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_MOVE]		= ENV_get_value_number("joypad_move",		5, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_START]		= ENV_get_value_number("joypad_start",		9, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_SELECT]		= ENV_get_value_number("joypad_select",		10, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_CAMERA]		= ENV_get_value_number("joypad_camera",		6, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_CAM_LEFT]	= ENV_get_value_number("joypad_cam_left",	7, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_CAM_RIGHT]	= ENV_get_value_number("joypad_cam_right",	8, "Joypad");
            joypad_button_use[JOYPAD_BUTTON_1STPERSON]	= ENV_get_value_number("joypad_1stperson",	4, "Joypad");
    */

    // PS1 Config 0 (Classic) adapted for Xbox/DualSense layout.
    // SDL3 button indices: 0=South(A/Cross), 1=East(B/Circle), 2=West(X/Square),
    // 3=North(Y/Triangle), 4=Back, 6=Start, 7=L3, 8=R3, 9=LB/L1, 10=RB/R1,
    // 15=LT/L2(digital), 16=RT/R2(digital).
    // Defaults come from the hardcoded config in env.cpp (see env_default_config).
    joypad_button_use[JOYPAD_BUTTON_JUMP]       = ENV_get_value_number("joypad_jump",       0, "Joypad");  // A / Cross
    joypad_button_use[JOYPAD_BUTTON_ACTION]     = ENV_get_value_number("joypad_action",     1, "Joypad");  // B / Circle
    joypad_button_use[JOYPAD_BUTTON_PUNCH]      = ENV_get_value_number("joypad_punch",      2, "Joypad");  // X / Square
    joypad_button_use[JOYPAD_BUTTON_KICK]       = ENV_get_value_number("joypad_kick",       3, "Joypad");  // Y / Triangle
    joypad_button_use[JOYPAD_BUTTON_SELECT]     = ENV_get_value_number("joypad_select",     4, "Joypad");  // Back / Select
    joypad_button_use[JOYPAD_BUTTON_START]      = ENV_get_value_number("joypad_start",      6, "Joypad");  // Start
    joypad_button_use[JOYPAD_BUTTON_CAMERA]     = ENV_get_value_number("joypad_camera",     9, "Joypad");  // LB / L1
    joypad_button_use[JOYPAD_BUTTON_1STPERSON]  = ENV_get_value_number("joypad_1stperson",  9, "Joypad");  // LB / L1 (same as PS1)
    joypad_button_use[JOYPAD_BUTTON_MOVE]       = ENV_get_value_number("joypad_move",      10, "Joypad");  // RB / R1 (step/strafe)
    joypad_button_use[JOYPAD_BUTTON_CAM_LEFT]   = ENV_get_value_number("joypad_cam_left",  15, "Joypad");  // LT / L2
    joypad_button_use[JOYPAD_BUTTON_CAM_RIGHT]  = ENV_get_value_number("joypad_cam_right", 16, "Joypad");  // RT / R2

    keybrd_button_use[KEYBRD_BUTTON_LEFT] = ENV_get_value_number("keyboard_left", 203, "Keyboard");
    keybrd_button_use[KEYBRD_BUTTON_RIGHT] = ENV_get_value_number("keyboard_right", 205, "Keyboard");
    keybrd_button_use[KEYBRD_BUTTON_FORWARDS] = ENV_get_value_number("keyboard_forward", 200, "Keyboard");
    keybrd_button_use[KEYBRD_BUTTON_BACK] = ENV_get_value_number("keyboard_back", 208, "Keyboard");

    keybrd_button_use[JOYPAD_BUTTON_PUNCH] = ENV_get_value_number("keyboard_punch", 44, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_KICK] = ENV_get_value_number("keyboard_kick", 45, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_ACTION] = ENV_get_value_number("keyboard_action", 46, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_MOVE] = ENV_get_value_number("keyboard_run", 47, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_JUMP] = ENV_get_value_number("keyboard_jump", 57, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_START] = ENV_get_value_number("keyboard_start", 15, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_SELECT] = ENV_get_value_number("keyboard_select", 28, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_CAMERA] = ENV_get_value_number("keyboard_camera", 207, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_CAM_LEFT] = ENV_get_value_number("keyboard_cam_left", 211, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_CAM_RIGHT] = ENV_get_value_number("keyboard_cam_right", 209, "Keyboard");
    keybrd_button_use[JOYPAD_BUTTON_1STPERSON] = ENV_get_value_number("keyboard_1stperson", 30, "Keyboard");
}

// If the player is holding a grenade or explosives, activates/primes it.
// Returns 1 if any in-hand item action was performed.
// uc_orig: player_activate_in_hand (fallen/Source/interfac.cpp)
SLONG player_activate_in_hand(Thing* p_person)
{
    if (p_person->Genus.Person->Flags & FLAG_PERSON_CANNING) {
        //
        // Release the coke can.
        //

        set_person_can_release(p_person, 128);

        return (1);
    }

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        if (p_person->Genus.Person->Ware) {
            //
            // Can't throw a grenade inside a warehouse!
            //
        } else {
            if (p_special->Genus.Special->SpecialType == SPECIAL_GRENADE) {
                if (p_special->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
                    //
                    // The person should throw the grenade.
                    //

                    set_person_can_release(p_person, 128);
                } else {
                    //
                    // Prime the grenade.
                    //

                    SPECIAL_prime_grenade(p_special);
                }

                return (1);
            }
        }

        if (p_special->Genus.Special->SpecialType == SPECIAL_EXPLOSIVES) {
            //
            // Put down and prime the explosives.
            //

            SPECIAL_set_explosives(p_person);

            return (1);
        }
    }
    return (0);
}

// If the player has a gun drawn, shoots. Otherwise tries to activate in-hand item.
// uc_orig: set_player_shoot (fallen/Source/interfac.cpp)
void set_player_shoot(Thing* p_person, SLONG param)
{
    if (person_has_gun_out(p_person)) {
        set_person_shoot(p_person, param);
        return;
    }
    player_activate_in_hand(p_person);
}

// Punch button handler: fight mode punches; otherwise activates in-hand item or punches.
// uc_orig: set_player_punch (fallen/Source/interfac.cpp)
void set_player_punch(Thing* p_person)
{
    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        set_person_punch(p_person);
        return;
    }
    if (!player_activate_in_hand(p_person)) {
        if (p_person->SubState == SUB_STATE_WALKING_BACKWARDS && person_has_gun_out(p_person)) {
            set_person_shoot(p_person, 0);
            return;
        }

        set_person_punch(p_person);
    }
}

// Returns UC_TRUE if Darci can safely jump (not standing in a warehouse doorway).
// Compares four probe points around the character in the warehouse map.
// If all four map to the same tile, jumping is safe.
// Note: dx/dz are hardcoded (0x10000>>11, 0) — not angle-dependent (pre-release simplification).
// uc_orig: should_i_jump (fallen/Source/interfac.cpp)
SLONG should_i_jump(Thing* darci)
{
    // SLONG dx = SIN(darci->Draw.Tweened->Angle) >> 11;
    // SLONG dz = COS(darci->Draw.Tweened->Angle) >> 11;

    const SLONG dx = 0x10000 >> 11;
    const SLONG dz = 0x00000 >> 11;

    SLONG x1 = (darci->WorldPos.X >> 8);
    SLONG z1 = (darci->WorldPos.Z >> 8);

    SLONG x2 = x1 + dx;
    SLONG z2 = z1 + dz;

    SLONG x3 = x1 - dz;
    SLONG z3 = z1 + dx;

    SLONG x4 = x1 + dz;
    SLONG z4 = z1 - dx;

    x1 -= dx;
    z1 -= dz;

    x1 >>= 8;
    z1 >>= 8;

    x2 >>= 8;
    z2 >>= 8;

    x3 >>= 8;
    z3 >>= 8;

    x4 >>= 8;
    z4 >>= 8;

    if (x1 == x2 && z1 == z2 && x1 == x3 && z1 == z3 && x1 == x4 && z1 == z4) {
        return UC_TRUE;
    } else {
        return WARE_which_contains(x1, z1) == WARE_which_contains(x2, z2) && WARE_which_contains(x3, z3) == WARE_which_contains(x4, z4);
    }
}

// Returns UC_TRUE if Darci can safely backflip (checks LOS ahead and warehouse roof clearance).
// uc_orig: should_person_backflip (fallen/Source/interfac.cpp)
SLONG should_person_backflip(Thing* darci)
{
    if (darci->Genus.Person->Ware) {
        SLONG px, py, pz, wy;
        //
        // slide along warehouse roof
        //

        calc_sub_objects_position(darci, darci->Draw.Tweened->AnimTween, SUB_OBJECT_HEAD, &px, &py, &pz);
        px += (darci->WorldPos.X) >> 8;
        py += (darci->WorldPos.Y) >> 8;
        pz += (darci->WorldPos.Z) >> 8;

        wy = MAVHEIGHT(px >> 8, pz >> 8) << 6;
        if (wy - py < 340) {
            return (0);
        }
    }

    SLONG dx = SIN(darci->Draw.Tweened->Angle) >> 8;
    SLONG dz = COS(darci->Draw.Tweened->Angle) >> 8;

    SLONG x1 = (darci->WorldPos.X >> 8);
    SLONG y1 = (darci->WorldPos.Y >> 8) + 0x60;
    SLONG z1 = (darci->WorldPos.Z >> 8);

    SLONG x2 = x1 + dx;
    SLONG y2 = y1;
    SLONG z2 = z1 + dz;

    return there_is_a_los(
        x1, y1, z1,
        x2, y2, z2,
        LOS_FLAG_IGNORE_UNDERGROUND_CHECK);
}

// Returns 1 if p_person is not near the side door of p_vehicle (wrong approach angle).
// uc_orig: bad_place_for_car (fallen/Source/interfac.cpp)
SLONG bad_place_for_car(Thing* p_person, Thing* p_vehicle)
{
    SLONG dx;
    SLONG dz;
    SLONG ix, jx;
    SLONG iz, jz;

    SLONG dist, vx, vz, on;

    //
    // Are we near it's door?
    //

    dx = SIN(p_vehicle->Genus.Vehicle->Angle);
    dz = COS(p_vehicle->Genus.Vehicle->Angle);

    ix = p_vehicle->WorldPos.X >> 8;
    iz = p_vehicle->WorldPos.Z >> 8;

    jx = ix;
    jz = iz;

    ix += (dx * 512) >> 16;
    iz += (dz * 512) >> 16;

    jx -= (dx * 512) >> 16;
    jz -= (dz * 512) >> 16;

    signed_dist_to_line_with_normal(ix, iz, jx, jz,
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Z >> 8,
        &dist, &vx, &vz, &on);

    //
    // 120 for vans 85 for cars!
    //

    if (abs(dist) < 100 || abs(dist > 200))
        return (1);

    return (0);
}

// Forward declaration (get_car_door_offsets is in vehicle.cpp — not yet migrated in full).
extern void get_car_door_offsets(SLONG type, SLONG door, SLONG* dx, SLONG* dz);

// Computes the world-space XZ position of a specific car door.
// door: 0 = driver side, 1 = passenger side.
// uc_orig: get_car_enter_xz (fallen/Source/interfac.cpp)
void get_car_enter_xz(Thing* p_vehicle, SLONG door, SLONG* cx, SLONG* cz)
{
    SLONG dx;
    SLONG dz;
    SLONG ix;
    SLONG iz;
    SLONG width, length;

    ASSERT(door == 0 || door == 1);

    get_car_door_offsets(p_vehicle->Genus.Vehicle->Type, door, &width, &length);

    //
    // Are we near it's door?
    //

    dx = -SIN(p_vehicle->Genus.Vehicle->Angle);
    dz = -COS(p_vehicle->Genus.Vehicle->Angle);

    ix = p_vehicle->WorldPos.X >> 8;
    iz = p_vehicle->WorldPos.Z >> 8;

    ix += (dx * length) >> 16;
    iz += (dz * length) >> 16;

    ix += (dz * width) >> 16;
    iz -= (dx * width) >> 16;

    //
    // (ix,iz) is now the position of the door.
    //

    *cx = ix;
    *cz = iz;
}

// Returns UC_TRUE if p_person is close enough to and aligned with p_vehicle's door.
// Sets *door to 0 (driver) or 1 (passenger) if in range.
// uc_orig: in_right_place_for_car (fallen/Source/interfac.cpp)
SLONG in_right_place_for_car(Thing* p_person, Thing* p_vehicle, SLONG* door)
{
    SLONG i;

    SLONG ix, iz;
    SLONG dx, dy;
    SLONG dz;
    SLONG dist;

    ASSERT(p_person->Class == CLASS_PERSON);
    ASSERT(p_vehicle->Class == CLASS_VEHICLE);

    if (bad_place_for_car(p_person, p_vehicle)) {
        return (0);
    }

    for (i = 0; i <= 1; i++) {
        extern UBYTE sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim;

        get_car_enter_xz(p_vehicle, i, &ix, &iz);

        //
        // (ix,iz) is now the position of the door.
        //

        dx = (p_person->WorldPos.X >> 8) - ix;
        dy = (p_person->WorldPos.Y >> 8) - ((p_vehicle->WorldPos.Y >> 8) - 90);
        dz = (p_person->WorldPos.Z >> 8) - iz;

        dist = QDIST2(abs(dx), abs(dz));

        if (dist < 0xb0 && abs(dy) < 64) {
            dy = PAP_calc_map_height_at(ix, iz) - (p_person->WorldPos.Y >> 8);
            if (abs(dy) < 64) {
                *door = i;

                ASSERT(*door == 0 || *door == 1);

                return UC_TRUE;
            }
        }
    }

    return UC_FALSE;
}

// Returns UC_TRUE if the person can enter p_vehicle (unoccupied, not destroyed, in range).
// Sets p_person->InCar to the vehicle index if successful.
// uc_orig: person_get_in_specific_car (fallen/Source/interfac.cpp)
SLONG person_get_in_specific_car(Thing* p_person, Thing* p_vehicle, SLONG* door)
{
    if (p_vehicle->Genus.Vehicle->Driver) {
        //
        // This vehicle is already occupied.
        //

        return UC_FALSE;
    }

    if (p_vehicle->State == STATE_DEAD) {
        //
        // Broken car!
        //

        return UC_FALSE;
    }

    //
    // Are we near it's door?
    //

    return in_right_place_for_car(p_person, p_vehicle, door);
}

// Scans nearby vehicles to find one the person can enter.
// Returns UC_TRUE if found; sets p_person->InCar and *door.
// uc_orig: person_get_in_car (fallen/Source/interfac.cpp)
SLONG person_get_in_car(Thing* p_thing, SLONG* door)
{
    SLONG i;

    Thing* col_thing;

#define MAX_COL_WITH 16

    THING_INDEX col_with[MAX_COL_WITH];
    SLONG col_with_upto;

    col_with_upto = THING_find_sphere(
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Y >> 8,
        p_thing->WorldPos.Z >> 8,
        0x180, // Some things can be quite big...
        col_with,
        MAX_COL_WITH,
        1 << CLASS_VEHICLE);

    ASSERT(col_with_upto <= MAX_COL_WITH);

    for (i = 0; i < col_with_upto; i++) {
        col_thing = TO_THING(col_with[i]);

        if (person_get_in_specific_car(p_thing, col_thing, door)) {
            p_thing->Genus.Person->InCar = THING_NUMBER(col_thing);

            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// Context-sensitive action handler for the ACTION button.
// Priority order: exit vehicle → arrest → climb down ladder → enter vehicle →
//   pick up hook → press switch → arrest (2nd pass) → pick up special → EWAY magic
//   radius → talk to NPC → search/carry → pick up can → crouch.
// Returns INPUT_MASK_ACTION if the action was consumed, 0 otherwise.
// uc_orig: do_an_action (fallen/Source/interfac.cpp)
ULONG do_an_action(Thing* p_thing, ULONG input)
{
    Thing* special_thing;
    THING_INDEX special_index;
    SLONG dist;
    SLONG door;

    if (p_thing->SubState == SUB_STATE_GRAPPLE_HELD || p_thing->SubState == SUB_STATE_GRAPPLE_HOLD) {
        return (0);
    }
    if (p_thing->State == SUB_STATE_IDLE_CROUTCH_ARREST) {
        return (0);
    }

    if (p_thing->State == STATE_GUN) {
        if (p_thing->SubState == SUB_STATE_DRAW_GUN || p_thing->SubState == SUB_STATE_DRAW_ITEM) {
            //
            // Wait 'till you've finished drawing your gun!
            //

            return 0;
        }
    }

    extern SLONG can_i_hug_wall(Thing * p_person);
    extern void set_person_turn_to_hug_wall(Thing * p_person);

    if (p_thing->State == STATE_CARRY) {
        if (p_thing->SubState == SUB_STATE_PICKUP_CARRY || p_thing->SubState == SUB_STATE_DROP_CARRY) {
            //
            // Don't do anything while picking somebody up or down...
            //

            return INPUT_MASK_ACTION;
        }

        extern SLONG is_there_room_in_front_of_me(Thing * p_person, SLONG how_much_room);

        if (!is_there_room_in_front_of_me(p_thing, 192)) {
            /*
                                    add_damage_text(
                                            p_thing->WorldPos.X          >> 8,
                                            p_thing->WorldPos.Y + 0x6000 >> 8,
                                            p_thing->WorldPos.Z          >> 8,
                                            "No room!");
            */
        } else {
            extern void set_person_uncarry(Thing * p_person);

            set_person_uncarry(p_thing);
        }

        return (INPUT_MASK_ACTION);
    }
    if (p_thing->SubState == SUB_STATE_HUG_WALL_LOOK_L) {
        set_person_hug_wall_leap_out(p_thing, 0);
        return (INPUT_MASK_ACTION);
    }
    if (p_thing->SubState == SUB_STATE_HUG_WALL_LOOK_R) {
        set_person_hug_wall_leap_out(p_thing, 1);
        return (INPUT_MASK_ACTION);
    }

    if (p_thing->State == STATE_DEAD || p_thing->State == STATE_DYING || p_thing->State == STATE_SEARCH) {
        return (0);
    }

    if (p_thing->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
        Thing* p_vehicle = TO_THING(p_thing->Genus.Person->InCar);

        if (p_vehicle->Genus.Vehicle->GrabAction && (p_vehicle->Velocity > 5)) {
            return 0; // button grabbed by car
        }

        if (abs(p_vehicle->Velocity) >= 50) {
            return 0; // moving too fast
        }

        if (p_vehicle->Genus.Vehicle->Skid >= 3) // SKID_START defined in vehicle.cpp!
        {
            SLONG dist;

            dist = QDIST2(abs(p_vehicle->Genus.Vehicle->VelX), abs(p_vehicle->Genus.Vehicle->VelZ));

            if (abs(dist) > 500)
                return 0;
        }

        if (p_vehicle->Genus.Vehicle->Flags & FLAG_VEH_IN_AIR) {
            return 0;
        }

        set_person_exit_vehicle(p_thing);
        return (INPUT_MASK_ACTION);
    }

    if (p_thing->Genus.Person->Flags & FLAG_PERSON_GRAPPLING) {
        //
        // Release the grappling hook.
        //

        set_person_grappling_hook_release(p_thing);

        return INPUT_MASK_ACTION;
    }
    if (p_thing->State != STATE_MOVEING) {
        if ((p_thing->State == STATE_IDLE || (p_thing->State == STATE_GUN && p_thing->SubState == SUB_STATE_AIM_GUN)) && p_thing->SubState != SUB_STATE_IDLE_CROUTCH && p_thing->SubState != SUB_STATE_IDLE_CROUTCHING) {
            //
            // Find someone to arrest?
            //

            {
                UWORD index;

                extern UWORD find_arrestee(Thing * p_person);

                if (p_thing->Genus.Person->PersonType == PERSON_DARCI && (index = find_arrestee(p_thing))) {
                    set_person_arrest(p_thing, index);

                    return INPUT_MASK_ACTION;
                }
            }

            //
            // climb down a ladder?
            //
            {
                SLONG ladder_col;
                struct DFacet* p_facet;
                ladder_col = find_nearby_ladder_colvect(p_thing);

                SLONG set_person_climb_down_onto_ladder(Thing * p_person, SLONG colvect);

                if (ladder_col) {
                    SLONG top;
                    //
                    // Mount the ladder.  This could call set_person_climb_ladder(),
                    // but then it would need the storey instead of the col_vect.
                    //
                    p_facet = &dfacets[ladder_col];

                    top = p_facet->Y[0] + p_facet->Height * 64;

                    if (abs(top - (p_thing->WorldPos.Y >> 8)) < 64) {
                        if (set_person_climb_down_onto_ladder(p_thing, ladder_col)) {
                            return (INPUT_MASK_ACTION);
                        }
                    }
                }
            }
        }

        extern SLONG person_on_floor(Thing * p_person);
        if (person_on_floor(p_thing))
            if (person_get_in_car(p_thing, &door)) {
                if (TO_THING(p_thing->Genus.Person->InCar)->Class == CLASS_VEHICLE) {
                    p_thing->Genus.Person->SpecialUse = NULL;
                    p_thing->Draw.Tweened->PersonID &= ~0xe0;
                    p_thing->Genus.Person->Flags &= ~FLAG_PERSON_GUN_OUT;

                    set_person_enter_vehicle(p_thing, TO_THING(p_thing->Genus.Person->InCar), door);

                    if (p_thing->SubState == SUB_STATE_ENTERING_VEHICLE)
                        return (INPUT_MASK_ACTION);
                }
            }
    }

    //
    // How far is Darci from the grappling hook?
    //
    if (p_thing->State != STATE_MOVEING) {
        SLONG hook_x;
        SLONG hook_y;
        SLONG hook_z;
        SLONG hook_yaw;
        SLONG hook_pitch;
        SLONG hook_roll;

        HOOK_pos_grapple(
            &hook_x,
            &hook_y,
            &hook_z,
            &hook_yaw,
            &hook_pitch,
            &hook_roll);

        SLONG dx = p_thing->WorldPos.X - hook_x >> 8;
        SLONG dy = p_thing->WorldPos.Y - hook_y >> 8;
        SLONG dz = p_thing->WorldPos.Z - hook_z >> 8;

        if (abs(dy) < 0x20) {
            dist = abs(dx) + abs(dz);

            if (dist < 0x80) {
                //
                // Near enough, so bend down to pickup the grappling hook.
                //

                set_person_grappling_hook_pickup(p_thing);

                return INPUT_MASK_ACTION;
            }
        }
    }

    if ((p_thing->State == STATE_IDLE || (p_thing->State == STATE_GUN && p_thing->SubState == SUB_STATE_AIM_GUN)) && p_thing->SubState != SUB_STATE_IDLE_CROUTCH && p_thing->SubState != SUB_STATE_IDLE_CROUTCHING) {

        /* moved up a bit
                        //
                        // Find someone to arrest?
                        //

                        {
                                UWORD index;

                                extern UWORD find_arrestee(Thing *p_person);

                                if (p_thing->Genus.Person->PersonType == PERSON_DARCI && (index = find_arrestee(p_thing)))
                                {
                                        set_person_arrest(p_thing, index);

                                        return INPUT_MASK_ACTION;
                                }
                        }
        */

        //
        // Near a switch or a valve?
        //

        {
            SLONG ob_x;
            SLONG ob_y;
            SLONG ob_z;
            SLONG ob_yaw;
            SLONG ob_prim;
            SLONG ob_index;

            if (OB_find_type(
                    p_thing->WorldPos.X >> 8,
                    p_thing->WorldPos.Y >> 8,
                    p_thing->WorldPos.Z >> 8,
                    0x180,
                    FIND_OB_SWITCH_OR_VALVE,
                    &ob_x,
                    &ob_y,
                    &ob_z,
                    &ob_yaw,
                    &ob_prim,
                    &ob_index)) {
                //
                // In the correctish place?
                //

                SLONG anim;

                SLONG want_x;
                SLONG want_z;

                if (ob_prim == PRIM_OBJ_SWITCH_OFF) {
                    anim = ANIM_BUTTON;
                    want_x = ob_x + (SIN(ob_yaw) >> 10);
                    want_z = ob_z + (COS(ob_yaw) >> 10);
                } else {
                    anim = ANIM_VALVE_LOOP;
                    want_x = ob_x + (SIN(ob_yaw) >> 8) - (SIN(ob_yaw) >> 10);
                    want_z = ob_z + (COS(ob_yaw) >> 8) - (COS(ob_yaw) >> 10);
                }

                SLONG dx = abs(want_x - (p_thing->WorldPos.X >> 8));
                SLONG dz = abs(want_z - (p_thing->WorldPos.Z >> 8));

                SLONG dist = QDIST2(dx, dz);

                if (dist < 0x40) {
                    //
                    // Facing in the right direction?
                    //

                    SLONG dangle;

                    dangle = p_thing->Draw.Tweened->Angle - ob_yaw;
                    dangle &= 2047;

                    if (dangle > 1024) {
                        dangle -= 2048;
                    }

                    if (abs(dangle) < 200) {
                        //
                        // Move to the right place?
                        //

                        GameCoord newpos;

                        newpos.X = want_x << 8;
                        newpos.Y = p_thing->WorldPos.Y;
                        newpos.Z = want_z << 8;

                        move_thing_on_map(p_thing, &newpos);

                        p_thing->Draw.Tweened->Angle = ob_yaw;

                        //
                        // Press the switch.
                        //

                        p_thing->Genus.Person->Timer1 = 2;
                        set_person_do_a_simple_anim(p_thing, anim);

                        //
                        // Tell the waypoint system.
                        //

                        EWAY_prim_activated(ob_index);

                        //
                        // Turn the switch green!
                        //

                        if (OB_ob[ob_index].prim == PRIM_OBJ_SWITCH_OFF) {
                            OB_ob[ob_index].prim = PRIM_OBJ_SWITCH_ON;
                        }

                        return INPUT_MASK_ACTION;
                    }
                }
            }
        }

        //
        // Find someone to arrest? just to make sure
        //

        {
            UWORD index;

            extern UWORD find_arrestee(Thing * p_person);

            if (p_thing->Genus.Person->PersonType == PERSON_DARCI && (index = find_arrestee(p_thing))) {
                set_person_arrest(p_thing, index);

                return INPUT_MASK_ACTION;
            }
        }

        //
        // Shall we bend down to pick up a special?
        //

        special_index = THING_find_nearest(
            p_thing->WorldPos.X >> 8,
            p_thing->WorldPos.Y >> 8,
            p_thing->WorldPos.Z >> 8,
            0xa0, // made same as canning_get_special
            1 << CLASS_SPECIAL);

        if (special_index) {
            special_thing = TO_THING(special_index);

            if (should_person_get_item(p_thing, special_thing)) {
                //
                // Bend down to pick up the special.
                //

                set_person_special_pickup(p_thing);

                return (INPUT_MASK_ACTION);
            }
        }

        //
        // Player hits 'use' in a radius
        //

        if (EWAY_magic_radius_flag) {
            EWAY_Way* test = EWAY_magic_radius_flag;
            EWAY_magic_radius_flag = 0;
            EWAY_evaluate_condition(test, &test->ec);
            if (EWAY_magic_radius_flag) {
                EWAY_set_active(EWAY_magic_radius_flag);
                EWAY_magic_radius_flag = 0;
                return INPUT_MASK_ACTION;
            }
        }

        EWAY_magic_radius_flag = 0;

        //
        // Using a person (only if you're on the mapwho)
        //

        if (p_thing->Flags & FLAGS_ON_MAPWHO) {

            // #ifndef	PSX
            if (p_thing->Genus.Person->PersonType == PERSON_ROPER) {
                SLONG index;
                if ((index = find_corpse(p_thing))) {
                    set_person_carry(p_thing, index);
                    return INPUT_MASK_ACTION;
                }
            }
            // #endif
            {
                OB_Info* oi;

                extern SLONG OB_find_type(SLONG mid_x, SLONG mid_y, SLONG mid_z, SLONG max_range, ULONG prim_flags, SLONG * ob_x, SLONG * ob_y, SLONG * ob_z, SLONG * ob_yaw, SLONG * ob_prim, SLONG * ob_index);

                if (oi = OB_find_index(p_thing->WorldPos.X >> 8, p_thing->WorldPos.Y >> 8, p_thing->WorldPos.Z >> 8, 256, UC_TRUE)) {
                    if (oi)
                        if (set_person_search(p_thing, oi->index, oi->x, oi->y, oi->z)) {
                            return 0;
                        }
                }

                {
                    SLONG index;
                    index = find_searchable_person(p_thing);
                    if (index) {
                        set_person_search_corpse(p_thing, TO_THING(index));
                        return 0;
                    }
                }
            }

            //
            // Find our nearest person not including ourselves.
            //

            remove_thing_from_map(p_thing);

            ULONG use = THING_find_nearest(
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y >> 8,
                p_thing->WorldPos.Z >> 8,
                0xa0,
                1 << CLASS_PERSON);

            add_thing_to_map(p_thing);

            //
            // If this person is useable...
            //

            extern UBYTE is_semtex; // defined in frontend.cpp

            if (is_semtex && (use == 193 || use == 195)) {
                // skip the wetback line in stern revenge, (skymiss2)  PC && DREAMCAST
            } else if (use) {
                Thing* p_person = TO_THING(use);

                ASSERT(p_person->Class == CLASS_PERSON);

                if (p_person->Genus.Person->Flags & FLAG_PERSON_USEABLE) {
                    //
                    // Is this person doing something that can be interrupted?
                    //

                    if (PCOM_person_doing_nothing_important(p_person)) {
                        //
                        // Pressed action near a useable person! Alert the waypoint system.
                        //

                        if (EWAY_used_person(use)) {
                            //
                            // The waypoint system is triggering off a message. Make the
                            // person talk to darci.
                            //

                            PCOM_make_people_talk_to_eachother(
                                TO_THING(use),
                                p_thing,
                                UC_FALSE,
                                UC_FALSE);
                            return INPUT_MASK_ACTION;
                        }
                    }

                    return INPUT_MASK_ACTION;
                } else if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) {
                    //
                    // Is this person doing something that can be interrupted?
                    //

                    if (NET_PERSON(0)->Genus.Person->PersonType == PERSON_DARCI)
                        if (PCOM_person_doing_nothing_important(p_person)) {
                            SLONG message_type = EWAY_FAKE_MESSAGE_NORMAL;

                            //
                            // Pick a random sentence for this person to say.
                            //
                            PCOM_make_people_talk_to_eachother(
                                TO_THING(use),
                                p_thing,
                                UC_FALSE,
                                UC_FALSE);

                            if (TO_THING(use)->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY) {
                                message_type = EWAY_FAKE_MESSAGE_GUILTY;
                            }
                            if (TO_THING(use)->Genus.Person->pcom_ai_memory) {
                                message_type = EWAY_FAKE_MESSAGE_ANNOYED;
                            }

                            CBYTE* response = EWAY_get_fake_wander_message(message_type);

                            if (response) {
                                PANEL_new_text(
                                    TO_THING(use),
                                    4000,
                                    response);

                                PCOM_make_people_talk_to_eachother(
                                    TO_THING(use),
                                    p_thing,
                                    UC_FALSE,
                                    UC_FALSE);
                            }
                        }
                }
            }
        }
    }

    if (p_thing->State == STATE_IDLE || (p_thing->State == STATE_GUN && p_thing->SubState == SUB_STATE_AIM_GUN)) {

        SLONG angle;

        /*
        if (THING_find_nearest(
                        p_thing->WorldPos.X >> 8,
                        p_thing->WorldPos.Y >> 8,
                        p_thing->WorldPos.Z >> 8,
                        0x80,
                        (1 << CLASS_BARREL)))
        {
                set_person_barrel_pickup(p_thing);

                return INPUT_MASK_ACTION;
        }
        */
        if (p_thing->SubState != SUB_STATE_IDLE_CROUTCH && p_thing->SubState != SUB_STATE_IDLE_CROUTCHING)
            if (angle = can_i_hug_wall(p_thing)) {
                p_thing->Draw.Tweened->Angle = angle - 1;
                set_person_turn_to_hug_wall(p_thing);
                return (INPUT_MASK_ACTION);
            }

        //
        // Near a coke can and idle
        //
        if ((p_thing->Draw.Tweened->PersonID >> 5) == 0) {
            dist = DIRT_get_nearest_can_or_head_dist(
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y >> 8,
                p_thing->WorldPos.Z >> 8);

            if (dist < 0x80) {
                //
                // Near enough. Bend down to pick up the coke can.
                //

                set_person_can_pickup(p_thing);

                return INPUT_MASK_ACTION;
            }
        }
    }

    if (p_thing->State == STATE_IDLE || p_thing->State == STATE_MOVEING) {
        /*
                // now auto climb ladders
                        //
                        // Can she mount a ladder? ...so to speak!
                        //

                        ladder_col = find_nearby_ladder_colvect(p_thing);

                        if (ladder_col)
                        {
                                //
                                // Mount the ladder.  This could call set_person_climb_ladder(),
                                // but then it would need the storey instead of the col_vect.
                                //

                                if(mount_ladder(p_thing, ladder_col))
                                        return (INPUT_MASK_ACTION);
                        }
        */

        if (p_thing->State == STATE_IDLE || p_thing->State == STATE_GUN) {
            //
            // failed to do any of the other stuff (pickup/flick switch/enter car/...), so croutch
            //
            switch (p_thing->SubState) {
            case 0:
                set_person_croutch(p_thing);
                return 0; //  we need to hold this down so dont use up the action button //INPUT_MASK_ACTION;
                break;
            case SUB_STATE_IDLE_CROUTCH:
            case SUB_STATE_IDLE_CROUTCHING:
                return (0);
                break;
            }
        }

        if (p_thing->State == STATE_MOVEING) {
            //
            // failed to do any of the other stuff (pickup/flick switch/enter car/...), so croutch
            //
            switch (p_thing->SubState) {
            case SUB_STATE_RUNNING:
                if ((p_thing->Genus.Person->Stamina > 3) && (p_thing->Genus.Person->PersonType != PERSON_ROPER)) {
                    p_thing->Genus.Person->Mode = PERSON_MODE_SPRINT;
                    return (0);
                }
                break;
            case SUB_STATE_WALKING:
                p_thing->Genus.Person->Mode = PERSON_MODE_RUN;
                break;
            }
        }
    } else if (p_thing->State == STATE_GUN) {
        set_person_croutch(p_thing);
        return (0);
    }

    return (0); // INPUT_MASK_ACTION);
}

// ============================================================
// Chunk 2: find_best_action_from_tree, movement system
// ============================================================

// uc_orig: find_best_action_from_tree (fallen/Source/interfac.cpp)
// Searches action_tree[action] for the first entry whose Input matches the current input.
// Logic==0: any bit set triggers; Logic!=0: all bits must be set.
SLONG find_best_action_from_tree(SLONG action, ULONG input, ULONG* input_used)
{
    struct ActionInfo* action_options;

    action_options = action_tree[action];

    if (action_options) {
        while (action_options->Action) {
            if (action_options->Input) {
                if (action_options->Logic == 0) {
                    if (input & action_options->Input) {
                        *input_used = action_options->Input;
                        return (action_options->Action);
                    }
                } else {
                    if ((input & action_options->Input) == action_options->Input) {
                        *input_used = action_options->Input;
                        return (action_options->Action);
                    }
                }
            }
            action_options++;
        }
    }
    return (0);
}

// uc_orig: get_camera_angle (fallen/Source/interfac.cpp)
// Returns the current camera yaw angle (0-2047). Uses EWAY cutscene camera if active,
// otherwise reads FC_cam[0].yaw.
SLONG get_camera_angle(void)
{
    SLONG ca;
    SLONG cam_x, cam_y, cam_z, cam_yaw, cam_pitch, cam_roll, cam_lens;

    if (EWAY_grab_camera(&cam_x, &cam_y, &cam_z, &cam_yaw, &cam_pitch, &cam_roll, &cam_lens)) {
        ca = cam_yaw >> 8;
    } else {
        ca = FC_cam[0].yaw >> 8;
    }

    return (ca);
}

// uc_orig: player_stop_move (fallen/Source/interfac.cpp)
// Transitions the player from a moving state into the stopping sub-state.
// Has no effect when already in non-moving states (fighting, grappling, etc.).
void player_stop_move(Thing* p_thing, ULONG input)
{
    if (p_thing->State == STATE_GRAPPLING || p_thing->State == STATE_CANNING || p_thing->State == STATE_FIGHTING) {
        return;
    }
    if (p_thing->SubState == SUB_STATE_RUNNING_SKID_STOP)
        return;

    if (p_thing->SubState == SUB_STATE_BLOCK)
        ASSERT(0);
    if (p_thing->SubState != SUB_STATE_STOPPING && p_thing->SubState != SUB_STATE_STOPPING_DEAD && p_thing->SubState != SUB_STATE_RUN_STOP && p_thing->SubState != SUB_STATE_STOPPING_OT && p_thing->SubState != SUB_STATE_SLIPPING && p_thing->SubState != SUB_STATE_SLIPPING_END && p_thing->SubState != SUB_STATE_RUNNING_VAULT && p_thing->SubState != SUB_STATE_RUNNING_HIT_WALL) {

        p_thing->SubState = SUB_STATE_STOPPING;
    }
}

// uc_orig: player_interface_move (fallen/Source/interfac.cpp)
// Dispatches to the active movement handler. USER_INTERFACE==0 is the only active path.
// Filters simultaneous left+right input (clears both bits).
void player_interface_move(Thing* p_thing, ULONG input)
{
    if ((input & (INPUT_MASK_LEFT | INPUT_MASK_RIGHT)) == (INPUT_MASK_LEFT | INPUT_MASK_RIGHT))
        input &= ~(INPUT_MASK_LEFT | INPUT_MASK_RIGHT);

    switch (USER_INTERFACE) {
    case 0:
        if (!CONTROLS_inventory_mode)
            player_apply_move(p_thing, input);

        break;
    case 1:
        // Old test mode for movement in pressed direction — unused.
        // player_apply_move_analogue(p_thing, input);
        break;
    }
}

// uc_orig: init_user_interface (fallen/Source/interfac.cpp)
// Initialises the input system: sets USER_INTERFACE mode and reads scanner config.
void init_user_interface(void)
{
    USER_INTERFACE = 0;
    PANEL_scanner_poo = ENV_get_value_number("scanner_follows", UC_TRUE, "Game");
}

// Snapping threshold for lock_to_compass: angles within ±DLOCK of a cardinal direction snap to it.
// uc_orig: DLOCK (fallen/Source/interfac.cpp)
#define DLOCK 32

// uc_orig: lock_to_compass (fallen/Source/interfac.cpp)
// Snaps the thing's facing angle to the nearest 90-degree compass direction
// if it is within DLOCK units (32) of that direction.
void lock_to_compass(Thing* p_thing)
{
    SLONG angle;

    angle = p_thing->Draw.Tweened->Angle;

    if (angle < DLOCK || angle > 2048 - DLOCK) {
        angle = 0;
    } else if (angle > 512 - DLOCK && angle < 512 + DLOCK) {
        angle = 512;
    } else if (angle > 1024 - DLOCK && angle < 1024 + DLOCK) {
        angle = 1024;
    } else if (angle > 1536 - DLOCK && angle < 1536 + DLOCK) {
        angle = 1536;
    }
    p_thing->Draw.Tweened->Angle = angle;
}

// Maximum turn speed per frame when rotating (controls animation frame advance rate).
// uc_orig: TURN_TIMER (fallen/Source/interfac.cpp)
#define TURN_TIMER 512

// uc_orig: get_joy_angle (fallen/Source/interfac.cpp)
// Returns the world-space angle indicated by the analog stick.
// If JOY_REL_CAMERA is set in flags, the angle is adjusted by the current camera yaw.
SLONG get_joy_angle(ULONG input, UWORD flags)
{
    SLONG dx = 0, dz = 0;
    SLONG angle = -1;

    dx = GET_JOYX(input);
    dz = GET_JOYY(input);

    angle = Arctan(-dx, dz);

    if (flags & JOY_REL_CAMERA) {
        SLONG ca;

        ca = get_camera_angle();

        angle += ca;
        angle = (angle + 2048) & 2047;
    }
    return (angle);
}

// uc_orig: player_turn_left_right_analogue (fallen/Source/interfac.cpp)
// Legacy analog-stick rotation system (used on PSX/DC). On PC, player_turn_left_right() is used instead.
// Reads GET_JOYX/GET_JOYY from input, computes angle via Arctan(-dx, dz) relative to camera.
// Applies visual Roll lean proportional to velocity * stick X deflection.
// Suppresses turning for 8 frames after an EWAY camera jump (cutscene transition).
SLONG player_turn_left_right_analogue(Thing* p_thing, SLONG input)
{
    static UBYTE reduce_turn = 0;

    if (EWAY_cam_jumped > 0) {
        EWAY_cam_jumped -= 1;
    }

    if (EWAY_cam_jumped) {
        reduce_turn = 8;
    }
    if (reduce_turn) {
        --reduce_turn;
    }

    if (p_thing->State == STATE_SEARCH) {
    } else if (p_thing->Genus.Person->Action == ACTION_SIDE_STEP) {
        return 1;
    } else if (p_thing->SubState == SUB_STATE_RUNNING_HIT_WALL) {
    } else if (p_thing->SubState == SUB_STATE_SIDLE) {
        if (input & INPUT_MASK_LEFT) {
            set_person_step_right(p_thing);
            return 1;
        } else if (input & INPUT_MASK_RIGHT) {
            set_person_step_left(p_thing);
            return 1;
        }

    } else {
        SLONG dx = 0, dz = 0;
        SLONG angle = -1;
        SLONG velocity;

        dx = GET_JOYX(input);
        dz = GET_JOYY(input);

        velocity = QDIST2(abs(dx), abs(dz));

        if (velocity > ANALOGUE_MIN_VELOCITY)
            angle = Arctan(-dx, dz);

        if (p_thing->Velocity > 10 && p_thing->SubState == SUB_STATE_RUNNING) {
            p_thing->Draw.Tweened->Roll = ((((p_thing->Velocity - 9) * dx)) >> 5);
            p_thing->Draw.Tweened->DRoll = 0;
        }

        if (angle >= 0) {
            SLONG ca;
            SLONG max_angle = 128;
            SLONG dangle;

            ca = get_camera_angle();

            if (player_relative) {
                ca = p_thing->Draw.Tweened->Angle;
            }

            angle += ca;
            if (p_thing->SubState == SUB_STATE_WALKING_BACKWARDS) {
                angle += 1024;
            }
            angle = (angle + 2048) & 2047;

            if (p_thing->State == STATE_JUMPING || p_thing->SubState == SUB_STATE_RUNNING_SKID_STOP)
                max_angle = 16;
            if (p_thing->SubState == SUB_STATE_CRAWLING)
                max_angle = 64;

            if (player_relative) {
                max_angle >>= 1;
            }

            if (!player_relative) {
                if (p_thing->State == STATE_IDLE || (p_thing->SubState == SUB_STATE_AIM_GUN))
                    max_angle = 1024;
            }

            if (reduce_turn) {
                max_angle -= (reduce_turn << 5);
                if (max_angle <= 0)
                    return (0);
            }

            {
                dangle = -p_thing->Draw.Tweened->Angle + angle;
                dangle &= 2047;
                if (dangle >= 1024)
                    dangle = -(2048 - dangle);

                if (p_thing->Velocity > 8) {

                    dangle /= (p_thing->Velocity) >> 3;
                }

                if (player_relative) {
                    if (p_thing->State == STATE_IDLE) {
                        if (abs(dangle) > 1024 - 400) {
                            set_person_walk_backwards(p_thing);
                            return (0);
                        }

                    } else if (p_thing->SubState == SUB_STATE_WALKING_BACKWARDS) {
                        if (abs(dangle) > 600) {
                            set_person_running(p_thing);
                            return (0);
                        }
                    }
                }

                if (dangle < -max_angle)
                    dangle = -max_angle;
                else if (dangle > max_angle)
                    dangle = max_angle;

                p_thing->Draw.Tweened->Angle += dangle;

                p_thing->Draw.Tweened->Angle &= 2047;
            }
        }
    }
    return (0);
}

// uc_orig: process_analogue_movement (fallen/Source/interfac.cpp)
// Analog-stick movement handler: moves the character in the camera-relative direction
// indicated by the stick. Handles all movement states (idle, running, climbing, dangling...).
void process_analogue_movement(Thing* p_thing, SLONG input)
{
    SLONG dx, dz, velocity;
    SLONG angle = -1;
    SLONG ca;

    dx = GET_JOYX(input);
    dz = GET_JOYY(input);

    velocity = QDIST2(abs(dx), abs(dz));

    angle = Arctan(-dx, dz);
    ca = get_camera_angle();

    angle += ca;
    angle = (angle + 2048) & 2047;

    SLONG dangle = (ca & 2047) - (p_thing->Draw.Tweened->Angle & 20247);

    dangle &= 2047;

    if (dangle > 1024) {
        dangle -= 2048;
    }
    if (dangle < -1024) {
        dangle += 2048;
    }

    SLONG facing_camera;

    if (abs(dangle) > 512) {
        facing_camera = UC_TRUE;
    } else {
        facing_camera = UC_FALSE;
    }

    if (velocity > ANALOGUE_MIN_VELOCITY) {
        switch (p_thing->State) {
        case STATE_CARRY:
            if (p_thing->SubState != SUB_STATE_STAND_CARRY)
                return;
        case STATE_IDLE:
        case STATE_HUG_WALL:
            if (p_thing->SubState == SUB_STATE_HUG_WALL_TURN)
                return;

        case STATE_HIT_RECOIL:
        case STATE_GUN:
            switch (p_thing->SubState) {
            case 0:
            case SUB_STATE_SIDLE:
            default:
                set_person_running(p_thing);
                break;
            case SUB_STATE_IDLE_CROUTCHING:
                set_person_crawling(p_thing);
                break;
            case SUB_STATE_IDLE_CROUTCH:
                if (p_thing->Draw.Tweened->FrameIndex > 3)
                    return;
                else
                    set_person_running(p_thing);
                break;
            }
            break;

            break;

        case STATE_MOVEING:
            switch (p_thing->SubState) {
            case SUB_STATE_RUNNING:
                if (p_thing->Genus.Person->Mode != PERSON_MODE_SPRINT) {
                    if (p_thing->Genus.Person->AnimType != ANIM_TYPE_ROPER) {
                        p_thing->Velocity = (velocity * 60) >> 8;
                    } else {
                        p_thing->Velocity = (velocity * 51) >> 8;
                    }

                    if ((p_thing->Velocity < 20 || m_bForceWalk) && p_thing->Genus.Person->AnimType != ANIM_TYPE_ROPER) {
                        if (!(p_thing->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING)) {
                            if (p_thing->Velocity > 20)
                                p_thing->Velocity = 20;

                            set_person_walking(p_thing);
                            p_thing->Genus.Person->Mode = PERSON_MODE_WALK;
                        }
                    }
                }
                break;
            case SUB_STATE_STOP_CRAWL:
                p_thing->SubState = SUB_STATE_CRAWLING;
                break;
            case SUB_STATE_CRAWLING:
                break;
            case SUB_STATE_WALKING:
                if (p_thing->Genus.Person->AnimType != ANIM_TYPE_ROPER) {
                    p_thing->Velocity = (velocity * 60) >> 8;
                } else {
                    p_thing->Velocity = (velocity * 51) >> 8;
                }
                if ((p_thing->Velocity > 24 || p_thing->Genus.Person->AnimType == ANIM_TYPE_ROPER) && !m_bForceWalk) {
                    set_person_running(p_thing);
                    p_thing->Genus.Person->Mode = PERSON_MODE_RUN;
                } else {
                    if (p_thing->Velocity > 24)
                        p_thing->Velocity = 24;
                }
                break;
            case SUB_STATE_STEP_LEFT:
            case SUB_STATE_STEP_RIGHT:
                set_person_running(p_thing);
                break;
            }
            break;
        case STATE_CLIMB_LADDER:
            if (dz < -MAX(abs(dx), 8)) {
                switch (p_thing->SubState) {
                case SUB_STATE_MOUNT_LADDER:
                    break;
                case SUB_STATE_STOPPING:
                case SUB_STATE_ON_LADDER:
                    p_thing->SubState = SUB_STATE_CLIMB_UP_LADDER;
                    break;
                }
            }
            break;
        case STATE_CLIMBING:
            if (dz < 0 - MAX(abs(dx), 8)) {
                switch (p_thing->SubState) {
                case SUB_STATE_STOPPING:
                case SUB_STATE_CLIMB_AROUND_WALL:

                    if (facing_camera) {
                        p_thing->SubState = SUB_STATE_CLIMB_DOWN_WALL;
                    } else {
                        p_thing->SubState = SUB_STATE_CLIMB_UP_WALL;
                    }

                    break;
                }
            }
            break;
        case STATE_DANGLING:
            if (dz < -MAX(abs(dx), 8)) {
                switch (p_thing->SubState) {
                case SUB_STATE_STOPPING:
                case SUB_STATE_DANGLING_CABLE_FORWARD:
                case SUB_STATE_DANGLING_CABLE_BACKWARD:
                case SUB_STATE_DANGLING_CABLE:
                    if (p_thing->Flags & FLAG_PERSON_ON_CABLE)
                        p_thing->SubState = SUB_STATE_DANGLING_CABLE_FORWARD;
                    else
                        ASSERT(0);
                    break;
                }
            }
            break;

        case STATE_CANNING:
            break;
        }
    } else {
        switch (p_thing->State) {

        case STATE_MOVEING:
            switch (p_thing->SubState) {
            case SUB_STATE_CRAWLING:
                set_person_idle_croutch(p_thing);
                break;
            case SUB_STATE_RUNNING:
            case SUB_STATE_WALKING:
            case SUB_STATE_SIDLE:
                player_stop_move(p_thing, input);
                break;
            }
            break;
        }
    }

    if (dz > MAX(dx, 8)) {
        switch (p_thing->State) {

        case STATE_HUG_WALL:
            if (p_thing->SubState == SUB_STATE_HUG_WALL_TURN)
                return;
        case STATE_IDLE:
        case STATE_GUN:
            break;
        case STATE_MOVEING:
            break;
        case STATE_CLIMB_LADDER:
            switch (p_thing->SubState) {
            case SUB_STATE_MOUNT_LADDER:
                break;
            case SUB_STATE_STOPPING:
            case SUB_STATE_ON_LADDER:
                p_thing->SubState = SUB_STATE_CLIMB_DOWN_LADDER;
                break;
            }
            break;
        case STATE_CLIMBING:
            switch (p_thing->SubState) {
            case SUB_STATE_STOPPING:
            case SUB_STATE_CLIMB_AROUND_WALL:

                if (facing_camera) {
                    p_thing->SubState = SUB_STATE_CLIMB_UP_WALL;
                } else {
                    p_thing->SubState = SUB_STATE_CLIMB_DOWN_WALL;
                }
                break;
            }
        case STATE_DANGLING:
            switch (p_thing->SubState) {
            case SUB_STATE_STOPPING:
            case SUB_STATE_DANGLING_CABLE_FORWARD:
            case SUB_STATE_DANGLING_CABLE_BACKWARD:
            case SUB_STATE_DANGLING_CABLE:
                if (p_thing->Flags & FLAG_PERSON_ON_CABLE)
                    p_thing->SubState = SUB_STATE_DANGLING_CABLE_FORWARD;
                else
                    ASSERT(0);
                break;
            }
            break;

        case STATE_CANNING:
            break;
        }
    }
}

// Maximum lean (roll) change per frame for the visual tilt during turning.
// uc_orig: MAX_LEAN_DELTA (fallen/Source/interfac.cpp)
#define MAX_LEAN_DELTA 50

// uc_orig: player_turn_left_right (fallen/Source/interfac.cpp)
// Primary (PC) rotation handler. Turns the player based on keyboard or analog input.
// Keyboard: cumulative turn speed builds up 16 units/frame to a max of ±128.
// Joystick: turn is proportional to wJoyX * wMaxTurn.
// Turn speed is capped and reduced while running fast, jumping, or skidding.
// Also animates lean (Roll) for visual effect when turning at speed.
SLONG player_turn_left_right(Thing* p_thing, SLONG input)
{
    if (p_thing->SubState == SUB_STATE_RUNNING_HIT_WALL) {
        return (1);
    }

    if (p_thing->Genus.Person->Action == ACTION_SIDE_STEP) {
        return 1;
    }

    if (p_thing->SubState == SUB_STATE_SIDLE) {
        if (input & INPUT_MASK_LEFT) {
            set_person_step_right(p_thing);
        } else if (input & INPUT_MASK_RIGHT) {
            set_person_step_left(p_thing);
        }
        return 1;
    }

    SWORD wMaxTurn = 94;

    if (p_thing->State == STATE_JUMPING) {
        wMaxTurn = 12;
    }

    if (p_thing->State == STATE_SEARCH) {
        wMaxTurn = 0;
    }

    if ((p_thing->SubState == SUB_STATE_RUNNING) || (p_thing->SubState == SUB_STATE_RUNNING_SKID_STOP)) {
        SWORD wRunningMaxTurn = 70 - (p_thing->Velocity);
        ASSERT(wRunningMaxTurn > 0);
        if (wMaxTurn > wRunningMaxTurn) {
            wMaxTurn = wRunningMaxTurn;
        }
    }
    if (p_thing->SubState == SUB_STATE_CRAWLING || p_thing->SubState == SUB_STATE_RUNNING_SKID_STOP) {
        wMaxTurn >>= 1;
    }

    SWORD wJoyX = GET_JOYX(input);
    SWORD wTurn;
    if ((input & INPUT_MASK_LEFT) || (input & INPUT_MASK_RIGHT)) {
        // Keyboard detection: if the analog bits (18-31) are all zero while a direction bit is set,
        // input is from keyboard, so use cumulative turning.
        if ((input & 0xfffc0000) == 0) {
            static SWORD wLastTurn = 0;
            if (input & INPUT_MASK_LEFT) {
                if (wLastTurn > 0) {
                    wLastTurn = 0;
                }
                wLastTurn -= 16;
                if (wLastTurn < -128) {
                    wLastTurn = -128;
                }
            } else {
                ASSERT(input & INPUT_MASK_RIGHT);
                if (wLastTurn < 0) {
                    wLastTurn = 0;
                }
                wLastTurn += 16;
                if (wLastTurn > 128) {
                    wLastTurn = 128;
                }
            }

            wTurn = wLastTurn;

            SATURATE(wTurn, -wMaxTurn, +wMaxTurn);
        } else {
            wTurn = (wJoyX * wMaxTurn) >> 7;
        }
    } else {
        wTurn = 0;
    }

    SWORD wFrameTurn = (wTurn * TICK_RATIO) >> TICK_SHIFT;

    p_thing->Draw.Tweened->Angle = (p_thing->Draw.Tweened->Angle - wFrameTurn) & 2047;

    SWORD wDesiredRoll = (wTurn * p_thing->Velocity) >> 2;

    if ((p_thing->SubState == SUB_STATE_WALKING_BACKWARDS)
        || (p_thing->Velocity <= 12)
        || (p_thing->State == STATE_JUMPING)) {
        wDesiredRoll = 0;
    }

    SWORD wCurrentRoll = p_thing->Draw.Tweened->Roll;
    if (wCurrentRoll > wDesiredRoll) {
        wCurrentRoll -= MAX_LEAN_DELTA;
        if (wCurrentRoll < wDesiredRoll) {
            wCurrentRoll = wDesiredRoll;
        }
    } else {
        wCurrentRoll += MAX_LEAN_DELTA;
        if (wCurrentRoll > wDesiredRoll) {
            wCurrentRoll = wDesiredRoll;
        }
    }
    p_thing->Draw.Tweened->Roll = wCurrentRoll;

    if (p_thing->State == STATE_IDLE && !is_person_crouching(p_thing)) {
        p_thing->Draw.Tweened->DRoll = -1;
        if (p_thing->Genus.Person->AnimType != ANIM_TYPE_ROPER) {
            if ((input & INPUT_MASK_LEFT) != 0) {
                if (p_thing->Draw.Tweened->CurrentAnim != ANIM_ROTATE_L) {
                    set_anim(p_thing, ANIM_ROTATE_L);
                    p_thing->Genus.Person->pcom_ai_counter = 0;
                } else {
                    ASSERT(wFrameTurn <= 0);
                    p_thing->Genus.Person->pcom_ai_counter -= wFrameTurn;
                    if (p_thing->Genus.Person->pcom_ai_counter > TURN_TIMER) {
                        p_thing->Draw.Tweened->DRoll = 0;
                    }
                }
            } else if ((input & INPUT_MASK_RIGHT) != 0) {
                if (p_thing->Draw.Tweened->CurrentAnim != ANIM_ROTATE_R) {
                    set_anim(p_thing, ANIM_ROTATE_R);
                    p_thing->Genus.Person->pcom_ai_counter = 0;
                } else {
                    ASSERT(wFrameTurn >= 0);
                    p_thing->Genus.Person->pcom_ai_counter += wFrameTurn;
                    if (p_thing->Genus.Person->pcom_ai_counter > TURN_TIMER) {
                        p_thing->Draw.Tweened->DRoll = 0;
                    }
                }
            } else {
                p_thing->Genus.Person->pcom_ai_counter = 0;
                p_thing->Draw.Tweened->DRoll = 0;
            }
        }
    }

    return 0;
}

// uc_orig: player_apply_move (fallen/Source/interfac.cpp)
// Core movement state machine: routes movement input to the correct handler based on
// the player's current state (idle, running, climbing, dangling, etc.).
// Dispatches to player_turn_left_right[_analogue] for rotation, then processes
// MOVE/FORWARDS/BACKWARDS inputs to set SubState transitions.
void player_apply_move(Thing* p_thing, ULONG input)
{
    switch (p_thing->State) {
    case STATE_CLIMBING:
        switch (p_thing->SubState) {
        case SUB_STATE_STOPPING:
            void locked_anim_change(Thing * p_person, UWORD locked_object, UWORD anim, SLONG dangle = 0);
            break;
        }
        break;

    case STATE_CARRY:
        if (p_thing->SubState != SUB_STATE_STAND_CARRY)
            return;

    case STATE_IDLE:

    case STATE_JUMPING:
    case STATE_MOVEING:
    case STATE_GUN:
    case STATE_GRAPPLING:

        if (analogue) {
            if (player_turn_left_right_analogue(p_thing, input))
                return;
        } else {
            if (player_turn_left_right(p_thing, input))
                return;
        }

        break;

        break;
    }

    switch (p_thing->State) {
    case STATE_JUMPING:
        return;
    }

    if (analogue) {
        process_analogue_movement(p_thing, input);
    } else {
        if (input & INPUT_MASK_MOVE) {
            switch (p_thing->State) {

            case STATE_HUG_WALL:
                if (p_thing->SubState == SUB_STATE_HUG_WALL_TURN)
                    return;
                p_thing->Velocity = 40;
                person_normal_move(p_thing);

                if (p_thing->SubState != SUB_STATE_HUG_WALL_TURN)
                    set_person_running(p_thing);
                break;
            case STATE_CARRY:
                if (p_thing->SubState != SUB_STATE_STAND_CARRY)
                    return;
            case STATE_IDLE:
            case STATE_HIT_RECOIL:

            case STATE_GUN:
                switch (p_thing->SubState) {
                case 0:
                case SUB_STATE_SIDLE:
                default:
                    set_person_running(p_thing);
                    break;
                case SUB_STATE_IDLE_CROUTCHING:
                    set_person_crawling(p_thing);
                    break;
                case SUB_STATE_IDLE_CROUTCH:
                    if (p_thing->Draw.Tweened->FrameIndex > 3)
                        return;
                    else
                        set_person_running(p_thing);
                    break;
                }
                break;

                break;

            case STATE_MOVEING:
                switch (p_thing->SubState) {
                case SUB_STATE_RUNNING:
                    break;
                case SUB_STATE_STOP_CRAWL:
                    p_thing->SubState = SUB_STATE_CRAWLING;
                    break;
                case SUB_STATE_CRAWLING:
                    break;
                case SUB_STATE_WALKING_BACKWARDS:
                    player_stop_move(p_thing, input);

                    break;
                case SUB_STATE_WALKING:
                    set_person_running(p_thing);
                    break;
                case SUB_STATE_STEP_LEFT:
                case SUB_STATE_STEP_RIGHT:
                    set_person_running(p_thing);
                    break;
                }
                break;
            case STATE_CLIMB_LADDER:
                switch (p_thing->SubState) {
                case SUB_STATE_MOUNT_LADDER:
                    break;
                case SUB_STATE_STOPPING:
                case SUB_STATE_ON_LADDER:
                    p_thing->SubState = SUB_STATE_CLIMB_UP_LADDER;
                    break;
                }
                break;
            case STATE_CLIMBING:
                switch (p_thing->SubState) {
                case SUB_STATE_STOPPING:
                case SUB_STATE_CLIMB_AROUND_WALL:
                    p_thing->SubState = SUB_STATE_CLIMB_UP_WALL;
                    break;
                }
                break;
            case STATE_DANGLING:
                switch (p_thing->SubState) {
                case SUB_STATE_STOPPING:
                case SUB_STATE_DANGLING_CABLE_FORWARD:
                case SUB_STATE_DANGLING_CABLE_BACKWARD:
                case SUB_STATE_DANGLING_CABLE:
                    if (p_thing->Flags & FLAG_PERSON_ON_CABLE)
                        p_thing->SubState = SUB_STATE_DANGLING_CABLE_FORWARD;
                    else
                        ASSERT(0);
                    break;
                }
                break;

            case STATE_GRAPPLING:
                if (p_thing->SubState == SUB_STATE_GRAPPLING_WINDUP) {
                    set_person_running(p_thing);
                }

                break;

            case STATE_CANNING:
                break;
            }
        } else {
            switch (p_thing->State) {

            case STATE_MOVEING:
                switch (p_thing->SubState) {
                case SUB_STATE_CRAWLING:
                    set_person_idle_croutch(p_thing);
                    break;
                case SUB_STATE_WALKING:
                    if (!(input & INPUT_MASK_FORWARDS))
                        player_stop_move(p_thing, input);
                    break;

                case SUB_STATE_RUNNING:
                    if (input & INPUT_MASK_FORWARDS) {
                        set_person_walking(p_thing);
                    } else {
                        player_stop_move(p_thing, input);
                    }

                    break;

                case SUB_STATE_SIDLE:
                    player_stop_move(p_thing, input);
                    break;
                }
                break;
            }
        }

        if ((input & INPUT_MASK_FORWARDS) && !(input & INPUT_MASK_MOVE)) {
            switch (p_thing->State) {

            case STATE_IDLE:
            case STATE_GUN:
                set_person_walking(p_thing);
                break;
            }
        }

        if (input & INPUT_MASK_BACKWARDS) {
            switch (p_thing->State) {

            case STATE_HUG_WALL:
                if (p_thing->SubState == SUB_STATE_HUG_WALL_TURN)
                    break;
            case STATE_IDLE:
            case STATE_GUN:
                set_person_walk_backwards(p_thing);
                break;
            case STATE_MOVEING:
                break;
            case STATE_CLIMB_LADDER:
                switch (p_thing->SubState) {
                case SUB_STATE_MOUNT_LADDER:
                    break;
                case SUB_STATE_STOPPING:
                case SUB_STATE_ON_LADDER:
                    p_thing->SubState = SUB_STATE_CLIMB_DOWN_LADDER;
                    break;
                }
                break;
            case STATE_CLIMBING:
                switch (p_thing->SubState) {
                case SUB_STATE_STOPPING:
                case SUB_STATE_CLIMB_AROUND_WALL:
                    p_thing->SubState = SUB_STATE_CLIMB_DOWN_WALL;
                    break;
                }
            case STATE_DANGLING:
                switch (p_thing->SubState) {
                case SUB_STATE_STOPPING:
                case SUB_STATE_DANGLING_CABLE_FORWARD:
                case SUB_STATE_DANGLING_CABLE_BACKWARD:
                case SUB_STATE_DANGLING_CABLE:
                    if (p_thing->Flags & FLAG_PERSON_ON_CABLE)
                        p_thing->SubState = SUB_STATE_DANGLING_CABLE_FORWARD;
                    else
                        ASSERT(0);
                    break;
                }
                break;

            case STATE_GRAPPLING:
                if (p_thing->SubState == SUB_STATE_GRAPPLING_WINDUP) {
                    set_person_walk_backwards(p_thing);
                }

                break;

            case STATE_CANNING:
                break;
            }
        }
    }
}

// uc_orig: person_enter_fight_mode (fallen/Source/interfac.cpp)
// Sets the person to fight mode.
void person_enter_fight_mode(Thing* p_person)
{
    p_person->Genus.Person->Mode = PERSON_MODE_FIGHT;
}

// uc_orig: apply_button_input (fallen/Source/interfac.cpp)
// Main normal-mode input handler: dispatches actions, movement, and fight/jump/skid from input mask.
// Returns the bitmask of inputs consumed/processed this frame.
ULONG apply_button_input(Thing* p_player, Thing* p_person, ULONG input)
{
    ULONG input_used = 0;
    ULONG processed = 0;

    /*

    if (p_person->Genus.Person->UnderAttack && p_person->Genus.Person->Mode != PERSON_MODE_FIGHT)
    {
            //
            // Lets the player know that pressing 'MODE' with put him into
            // fight mode.
            //
    }

    */

    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        CONSOLE_text_at(400, 400, 50, "Fight mode");
    }
    /*
            if(input&INPUT_MASK_MODE_CHANGE)
            {
                    person_change_mode(p_person);

                    processed |= INPUT_MASK_MODE_CHANGE;
            }
    */

    if (input & INPUT_MASK_ACTION) {
        processed |= do_an_action(p_person, input);
        input &= ~processed;

    } else {
        /*
                        if (p_person->Genus.Person->AnimType==ANIM_TYPE_ROPER)
                        {
                                if (p_person->Genus.Person->Mode==PERSON_MODE_RUN)
                                        p_person->Genus.Person->Mode=PERSON_MODE_SPRINT;
                        }
                        else
        */
        {
            if (p_person->Genus.Person->Mode == PERSON_MODE_SPRINT)
                p_person->Genus.Person->Mode = PERSON_MODE_RUN;
        }
        /*
                        if(p_person->Genus.Person->InsideIndex)
                        {
                                //
                                // your inside so walk unless actioning
                                //
                                if(p_person->Genus.Person->Mode==PERSON_MODE_RUN)
                                        p_person->Genus.Person->Mode=PERSON_MODE_WALK;
                        }
        */
        if (p_person->SubState == SUB_STATE_IDLE_CROUTCHING) {
            MSG_add("no action so stand up");
            set_person_idle_uncroutch(p_person);
        }
    }

    if (p_person->State == STATE_CARRY) {
        if (p_person->SubState == SUB_STATE_PICKUP_CARRY || p_person->SubState == SUB_STATE_DROP_CARRY) {
            return processed | input_used;
        }
    }

    if (input && (p_person->State != STATE_CARRY)) {
        SLONG new_action;

        new_action = find_best_action_from_tree(p_person->Genus.Person->Action, input, &input_used);

        switch (new_action) {
        case ACTION_KICK_FLAG:
            p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_KICK;
            break;
        case ACTION_PUNCH_FLAG:
            p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_PUNCH;
            break;
        case ACTION_BLOCK_FLAG:
            p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_BLOCK;
            break;
        case ACTION_JUMP_FLAG:
            p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_JUMP;
            break;
        case ACTION_HUG_LEFT:
            if (p_person->SubState != SUB_STATE_HUG_WALL_STEP_RIGHT && p_person->SubState != SUB_STATE_HUG_WALL_LOOK_L && p_person->SubState != SUB_STATE_HUG_WALL_TURN && p_person->SubState != SUB_STATE_HUG_WALL_LEAP_OUT)
                set_person_hug_wall_dir(p_person, 0);
            input_used = 0;
            break;
        case ACTION_HUG_RIGHT:
            if (p_person->SubState != SUB_STATE_HUG_WALL_STEP_LEFT && p_person->SubState != SUB_STATE_HUG_WALL_LOOK_R && p_person->SubState != SUB_STATE_HUG_WALL_TURN && p_person->SubState != SUB_STATE_HUG_WALL_LEAP_OUT)
                set_person_hug_wall_dir(p_person, 1);
            input_used = 0;
            break;
        case ACTION_UNSIT:
            void set_person_unsit(Thing * p_person);
            set_person_unsit(p_person);
            break;

        case ACTION_FLIP_LEFT:
            if (p_person->Genus.Person->Action == ACTION_STANDING_JUMP) {
                if (p_person->Draw.Tweened->FrameIndex < 3)
                    set_person_flip(p_person, 0);
            } else
                set_person_flip(p_person, 0);
            input_used = 0;
            break;
        case ACTION_FLIP_RIGHT:
            if (p_person->Genus.Person->Action == ACTION_STANDING_JUMP) {
                if (p_person->Draw.Tweened->FrameIndex < 3)
                    set_person_flip(p_person, 1);
            } else
                set_person_flip(p_person, 1);
            input_used = 0;
            break;
        case ACTION_SHOOT:
            if (p_person->Genus.Person->Action == ACTION_SHOOT) {
                /*

                //
                // Only the pistol animation can be interrupted so you
                // can shoot again.
                //

                if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)
                {
                        set_player_shoot(p_person,0);
                        processed|=input_used; //needs a clear click
                }

                */
            } else {
                {
                    set_player_shoot(p_person, 0);

                    processed |= input_used;
                }
            }
            break;
        case ACTION_GUN_AWAY:
            set_person_gun_away(p_person);
            processed |= input_used;
            break;
        case ACTION_RESPAWN:
            processed |= input_used;
            break;
        case ACTION_DROP_DOWN:
            if (p_person->Genus.Person->Action == ACTION_DEATH_SLIDE) {
                MFX_stop(THING_NUMBER(p_person), S_ZIPWIRE);
                set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
                p_person->Velocity = 0;
            } else {
                set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
            }
            break;
        }

        if (!(p_person->Genus.Person->Flags & (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C))) {
            switch (new_action) {

            case ACTION_SKID:
                if (p_person->SubState != SUB_STATE_RUNNING_SKID_STOP)
                    if (p_person->SubState != SUB_STATE_RUNNING_JUMP_LAND_FAST)
                        if (p_person->Velocity > 25) {
                            set_anim(p_person, ANIM_SLIDER_START);
                            p_person->SubState = SUB_STATE_RUNNING_SKID_STOP;
                            if (!(p_person->Genus.Person->Flags & FLAG_PERSON_SLIDING)) {
                                MFX_play_thing(THING_NUMBER(p_person), S_SLIDE_START, MFX_LOOPED, p_person);
                                p_person->Genus.Person->Flags |= FLAG_PERSON_SLIDING;
                            }
                        }

                break;
            case ACTION_STANDING_JUMP:

                if (should_i_jump(p_person)) {
                    if (input & INPUT_MASK_FORWARDS)
                        set_person_standing_jump_forwards(p_person);
                    else if ((input & INPUT_MASK_BACKWARDS) && should_person_backflip(p_person))
                        set_person_standing_jump_backwards(p_person);
                    else if (input & INPUT_MASK_LEFT)
                        set_person_flip(p_person, 0);
                    else if (input & INPUT_MASK_RIGHT)
                        set_person_flip(p_person, 1);
                    else
                        set_person_standing_jump(p_person);
                } else {
                    if (input & INPUT_MASK_LEFT)
                        set_person_flip(p_person, 0);
                    else if (input & INPUT_MASK_RIGHT)
                        set_person_flip(p_person, 1);
                }

                break;
            case ACTION_RUNNING_JUMP:
                if (p_person->SubState == SUB_STATE_RUNNING_SKID_STOP) {
                    p_person->Genus.Person->Flags &= ~FLAG_PERSON_SLIDING;
                    MFX_stop(THING_NUMBER(p_person), S_SLIDE_START);
                }
                if (should_i_jump(p_person)) {
                    set_person_running_jump(p_person);
                }
                break;
            case ACTION_TRAVERSE_LEFT:
                set_person_traverse(p_person, 0);
                break;
            case ACTION_TRAVERSE_RIGHT:
                set_person_traverse(p_person, 1);
                break;
            case ACTION_PULL_UP:
                set_person_pulling_up(p_person);
                input_used = 0;
                break;
            case ACTION_FIGHT_KICK:
                if (input & INPUT_MASK_BACKWARDS) {
                    if (p_person->State != STATE_JUMPING) {
                        turn_to_target(
                            p_person,
                            FIND_DIR_BACK | FIND_DIR_DONT_TURN);

                        p_person->Genus.Person->Timer1 = 6;

                        set_person_kick_dir(p_person, 2);

                        person_enter_fight_mode(p_person);

                        processed |= INPUT_MASK_KICK | INPUT_MASK_BACKWARDS;
                    }
                } else {
                    if (p_person->State != STATE_JUMPING) {
                        if (turn_to_target_and_kick(p_person)) {
                            person_enter_fight_mode(p_person);
                        }
                    }
                }

                break;
            case ACTION_FIGHT_PUNCH:
                if (person_has_gun_out(p_person)) {
                    set_person_shoot(p_person, 0);
                } else if (!player_activate_in_hand(p_person))
                    if (turn_to_target_and_punch(p_person))
                    {
                        person_enter_fight_mode(p_person);
                    }

                break;
            case ACTION_DRAW_SPECIAL:
                set_person_draw_special(p_person);
                processed |= input_used;
                break;
            }
        }
    }
    if ((input & INPUT_MOVEMENT_MASK) || (mouse_input && MouseDX)) {
        if (!(p_person->Genus.Person->Flags & FLAG_PERSON_NON_INT_M)) {
            switch (p_person->State) {
            case STATE_HIT_RECOIL:
            case STATE_CARRY:
            case STATE_HUG_WALL:
            case STATE_IDLE:
            case STATE_MOVEING:
            case STATE_GUN:
            case STATE_CLIMB_LADDER:
            case STATE_CLIMBING:
            case STATE_DANGLING:
            case STATE_JUMPING:
                player_interface_move(p_person, input);
                break;

            case STATE_GRAPPLING:

                if (p_person->SubState == SUB_STATE_GRAPPLING_WINDUP) {
                    player_interface_move(p_person, input);
                }

                break;
            default:
                break;
            }
        } else {
            if (p_person->Genus.Person->Action == ACTION_SIT_BENCH) {
                if (input & (INPUT_MASK_FORWARDS | INPUT_MASK_MOVE)) {
                    input_used |= INPUT_MASK_FORWARDS | INPUT_MASK_MOVE;

                    set_person_idle(p_person);
                }
            }
        }

    } else {
        if (!(p_person->Genus.Person->Flags & FLAG_PERSON_NON_INT_M))
            switch (p_person->State) {
            case STATE_IDLE:
                player_turn_left_right(p_person, 0);
                break;
            case STATE_MOVEING:
                switch (p_person->SubState) {
                case SUB_STATE_CRAWLING:
                    set_person_idle_croutch(p_person);
                    break;
                default:
                    player_stop_move(p_person, input);
                }

                break;
                /*
                                        case	STATE_CLIMB_LADDER:
                                                switch(p_person->SubState)
                                                {
                                                        case	SUB_STATE_MOUNT_LADDER:
                                                                break;
                                                        case	SUB_STATE_STOPPING:
                                                        case	SUB_STATE_ON_LADDER:
                                                        case	SUB_STATE_CLIMB_UP_LADDER:
                                                        case	SUB_STATE_CLIMB_DOWN_LADDER:
                                                                player_stop_move(p_person,input);
                                                                break;
                                                }
                */
                /*
                                                case	STATE_CLIMBING:
                                                switch(p_person->SubState)
                                                {
                                                        case	SUB_STATE_STOPPING:
                                                        case	SUB_STATE_CLIMB_UP_WALL:
                                                        case	SUB_STATE_CLIMB_AROUND_WALL:
                                                        case	SUB_STATE_CLIMB_DOWN_WALL:
                                                                player_stop_move(p_person,input);
                                                                break;
                                                }
                                                break;
                */
            case STATE_DANGLING:
                switch (p_person->SubState) {
                case SUB_STATE_STOPPING:
                case SUB_STATE_DANGLING_CABLE_FORWARD:
                case SUB_STATE_DANGLING_CABLE_BACKWARD:
                case SUB_STATE_DANGLING_CABLE:
                    player_stop_move(p_person, input);
                    break;
                }
                break;
            default:
                p_player->Genus.Player->Input = input;
                break;
            }
    }
    return (processed | input_used);
}

// Forward declaration used by apply_button_input_fight.
// uc_orig: count_gang (fallen/Source/pcom.cpp)
extern UWORD count_gang(Thing* p_target);

// uc_orig: apply_button_input_fight (fallen/Source/interfac.cpp)
// Input handler for PERSON_MODE_FIGHT. Handles punch/kick direction combos, target selection,
// fight stepping, flip, and exit from fight mode (MOVE without FORWARDS).
ULONG apply_button_input_fight(Thing* p_player, Thing* p_person, ULONG input)
{
    Player* pl = p_player->Genus.Player;

    if (p_person->SubState == SUB_STATE_IDLE_CROUTCH_ARREST)
        return (0);

    if (pl->Pressed & INPUT_MASK_PUNCH)
        if (person_has_gun_out(p_person)) {
            set_player_shoot(p_person, 0);
            return (INPUT_MASK_PUNCH);
        }

    // Move button (without FORWARDS) exits combat mode.
    if (!analogue) {
        if (p_player->Genus.Player->Pressed & INPUT_MASK_MOVE && !(p_player->Genus.Player->Pressed & INPUT_MASK_FORWARDS)) {
            if (p_person->Genus.Person->Flags & (FLAG_PERSON_HELPLESS | FLAG_PERSON_KO)) {
                // In no fit state to run away.
            } else {
                p_person->Genus.Person->Mode = PERSON_MODE_RUN;
                set_person_running(p_person);
                return (0);
            }
        }
    }

    /*
           #ifdef MUST_DOUBLE_CLICK_FORWARDS_TO_GET_OUT_OF_FIGHT_MODE
           ...
           #endif
    */

    if (input) {
        SLONG new_action;
        ULONG input_used;

        new_action = find_best_action_from_tree(p_person->Genus.Person->Action, input, &input_used);
        switch (new_action) {
        case ACTION_RESPAWN:
            break;
        }
    }

    if (p_person->SubState == SUB_STATE_STEP_FORWARD) {
        if (p_player->Genus.Player->Pressed & INPUT_MASK_JUMP) {
            switch (p_person->Draw.Tweened->CurrentAnim) {
            case ANIM_FIGHT_STEP_E:
                set_person_flip(p_person, 1);
                return INPUT_MASK_JUMP;
                break;
            case ANIM_FIGHT_STEP_W:
                set_person_flip(p_person, 0);
                return INPUT_MASK_JUMP;
                break;
            }
        }
    }

    extern SLONG get_combat_type_for_node(UBYTE current_node);

    if ((pl->Pressed & INPUT_MASK_PUNCH) == 0 && (pl->Pressed & INPUT_MASK_KICK) == 0)
        if (p_person->State == STATE_IDLE || (p_person->State == STATE_HIT_RECOIL && p_person->Draw.Tweened->FrameIndex > 2) || ((p_person->SubState == SUB_STATE_STEP_FORWARD || p_person->SubState == SUB_STATE_PUNCH || p_person->SubState == SUB_STATE_KICK)))
            if (p_person->Draw.Tweened->CurrentAnim != ANIM_KICK_NAD)
                if (p_person->Draw.Tweened->CurrentAnim != ANIM_LEG_SWEEP)
                    if (p_person->Draw.Tweened->CurrentAnim != ANIM_KICK_LEFT)
                        if (p_person->Draw.Tweened->CurrentAnim != ANIM_KICK_RIGHT)
                            if (p_person->Draw.Tweened->CurrentAnim != ANIM_KICK_NS)
                                if (p_person->Draw.Tweened->CurrentAnim != ANIM_KICK_BEHIND) {
                                    if (input & INPUT_MASK_BACKWARDS) {

                                        set_person_fight_step(p_person, 2);
                                        return (0);
                                    }
                                    if (input & INPUT_MASK_FORWARDS) {
                                        set_person_fight_step(p_person, 0);
                                        return (0);
                                    }
                                    if (input & INPUT_MASK_RIGHT) {
                                        set_person_fight_step(p_person, 1);
                                        return (0);
                                    }
                                    if (input & INPUT_MASK_LEFT) {
                                        set_person_fight_step(p_person, 3);
                                        return (0);
                                    }
                                }
    if ((pl->Pressed & INPUT_MASK_PUNCH) == 0 && (pl->Pressed & INPUT_MASK_KICK) == 0)
        if (p_player->Genus.Player->Pressed & (INPUT_MASK_LEFT | INPUT_MASK_RIGHT))
            if (p_person->State == STATE_FIGHTING) {
                if (p_person->SubState == SUB_STATE_KICK) {
                    if (p_person->Draw.Tweened->CurrentAnim == ANIM_KICK_NS) {
                        if (p_person->Draw.Tweened->FrameIndex < 2) {
                            if (p_player->Genus.Player->Pressed & INPUT_MASK_LEFT) {
                                set_person_flip(p_person, 0);
                            } else if (p_player->Genus.Player->Pressed & INPUT_MASK_RIGHT) {
                                set_person_flip(p_person, 1);
                            }
                        }
                    }
                }
            }

    if ((pl->Pressed & INPUT_MASK_PUNCH) == 0 && (pl->Pressed & INPUT_MASK_KICK) == 0)
        if (p_person->State == STATE_IDLE) {
            SLONG flags = 0;
            if (p_player->Genus.Player->Pressed & INPUT_MASK_JUMP) {
                if (p_player->Genus.Player->Pressed & INPUT_MASK_LEFT) {
                    set_person_flip(p_person, 0);
                } else if (p_player->Genus.Player->Pressed & INPUT_MASK_RIGHT) {
                    set_person_flip(p_person, 1);
                } else {
                    set_person_fight_anim(p_person, ANIM_KICK_NS);
                }

                return (0);
            }

            if (flags) {
                SLONG angle = input_to_angle[flags];
                angle <<= 3;
                angle += get_camera_angle();

                angle &= 2047;

                angle -= p_person->Draw.Tweened->Angle;
                if (angle > 1024)
                    angle -= 2048;
                if (angle < -1024)
                    angle += 2048;
                angle >>= 1;

                if (abs(angle) < 64) {
                    void set_person_fight_step_forward(Thing * p_person);
                    set_person_fight_step_forward(p_person);
                }

                p_person->Draw.Tweened->Angle += angle;
                p_person->Draw.Tweened->Angle &= 2047;

                turn_to_target(
                    p_person,
                    FIND_DIR_FRONT);
            }
        }

        else if (p_person->State == STATE_FIGHTING) {
            /*
            ...combo/direction interrupt code (commented out in original)...
            */

            if (p_person->SubState == SUB_STATE_STEP_FORWARD) {
                if (p_player->Genus.Player->Pressed & INPUT_MASK_JUMP) {
                    switch (p_person->Draw.Tweened->CurrentAnim) {
                    case ANIM_FIGHT_STEP_E:
                        set_person_flip(p_person, 1);
                        return INPUT_MASK_JUMP;
                        break;
                    case ANIM_FIGHT_STEP_W:
                        set_person_flip(p_person, 0);
                        return INPUT_MASK_JUMP;
                        break;
                    }
                }

                if (p_player->Genus.Player->Pressed & INPUT_MASK_BACKWARDS) {

                    if (p_person->Draw.Tweened->CurrentAnim == ANIM_FIGHT_STEP_S) {
                        if (p_person->Genus.Person->Timer1 < 4) {
                            reset_gang_attack(p_person);
                            turn_to_direction_and_find_target(p_person, FIND_DIR_BACK);

                            set_person_fight_idle(p_person);
                            return (INPUT_MASK_BACKWARDS);
                        }
                    }
                }
                if (p_player->Genus.Player->Pressed & INPUT_MASK_RIGHT) {
                    if (p_person->Draw.Tweened->CurrentAnim == ANIM_FIGHT_STEP_E) {
                        if (p_person->Genus.Person->Timer1 < 4) {
                            reset_gang_attack(p_person);
                            turn_to_direction_and_find_target(p_person, FIND_DIR_RIGHT);
                            set_person_fight_idle(p_person);
                            return (INPUT_MASK_RIGHT);
                        }
                    }
                }
                if (p_player->Genus.Player->Pressed & INPUT_MASK_LEFT) {
                    if (p_person->Draw.Tweened->CurrentAnim == ANIM_FIGHT_STEP_W) {
                        if (p_person->Genus.Person->Timer1 < 4) {
                            reset_gang_attack(p_person);
                            turn_to_direction_and_find_target(p_person, FIND_DIR_LEFT);
                            set_person_fight_idle(p_person);
                            return (INPUT_MASK_LEFT);
                        }
                    }
                }
            }
        }

    // Can only attack while idle or not mid-recoil.

    if (p_person->SubState == SUB_STATE_GRAPPLE_HELD || is_person_ko(p_person)) {
        if (pl->Pressed & (INPUT_MASK_ACTION | INPUT_MASK_PUNCH | INPUT_MASK_KICK | INPUT_MASK_JUMP)) {
            p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_BLOCK;
            pl->DoneSomething = UC_TRUE;
            return (pl->Pressed & (INPUT_MASK_ACTION | INPUT_MASK_PUNCH | INPUT_MASK_KICK | INPUT_MASK_JUMP));
        }
        return (0);
    }

    if (pl->Pressed & INPUT_MASK_ACTION) {
        extern UWORD find_arrestee(Thing * p_person);
        if (p_person->SubState != SUB_STATE_GRAPPLE_HELD && p_person->SubState != SUB_STATE_GRAPPLE_HOLD)
            if (p_person->State == STATE_IDLE || p_person->State == STATE_FIGHTING || p_person->SubState == SUB_STATE_RUN_STOP) {
                UWORD index;

                if (p_person->Genus.Person->PersonType == PERSON_DARCI && (index = find_arrestee(p_person))) {
                    set_person_arrest(p_person, index);
                    pl->DoneSomething = UC_TRUE;
                    return INPUT_MASK_ACTION;
                }

                index = THING_find_nearest(
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y >> 8,
                    p_person->WorldPos.Z >> 8,
                    0xa0,
                    1 << CLASS_SPECIAL);

                if (index) {
                    Thing* special_thing = TO_THING(index);

                    if (should_person_get_item(p_person, special_thing)) {
                        set_person_special_pickup(p_person);

                        return (INPUT_MASK_ACTION);
                    }
                }

                person_pick_best_target(p_person, 1);
                pl->DoneSomething = UC_TRUE;
                return INPUT_MASK_ACTION;
            }
    }

    if (p_person->State == STATE_IDLE || p_person->SubState == SUB_STATE_STEP_FORWARD) {
        SLONG dir = 0;
        if (p_person->SubState == SUB_STATE_STEP_FORWARD) {
            switch (p_person->Draw.Tweened->CurrentAnim) {
            case ANIM_FIGHT_STEP_N:
            case ANIM_FIGHT_STEP_N_BAT:
                dir = 1;
                break;
            case ANIM_FIGHT_STEP_E:
            case ANIM_FIGHT_STEP_E_BAT:
                dir = 2;
                break;
            case ANIM_FIGHT_STEP_S:
            case ANIM_FIGHT_STEP_S_BAT:
                dir = 3;
                break;
            case ANIM_FIGHT_STEP_W:
            case ANIM_FIGHT_STEP_W_BAT:
                dir = 4;
                break;
            }
        }
        if (pl->Pressed & INPUT_MASK_PUNCH) {
            /*
            ...directional punch code (commented out in original)...
            */
            {
                // Forward punch.
                set_player_punch(p_person);
            }

            pl->DoneSomething = UC_TRUE;
        } else if (pl->Pressed & INPUT_MASK_KICK) {
            if (p_person->Genus.Person->Timer1 < 5 || (input & (INPUT_MASK_DIR))) {
                if ((input & INPUT_MASK_LEFT) || dir == 4) {
                    if (p_person->State != STATE_JUMPING) {
                        turn_to_target(
                            p_person,
                            FIND_DIR_LEFT | FIND_DIR_DONT_TURN);

                        set_person_kick_dir(p_person, 3);

                        MSG_add("Kick left");
                        return (INPUT_MASK_KICK | INPUT_MASK_LEFT);
                    }
                } else if ((input & INPUT_MASK_RIGHT) || dir == 2) {
                    if (p_person->State != STATE_JUMPING) {
                        turn_to_target(
                            p_person,
                            FIND_DIR_RIGHT | FIND_DIR_DONT_TURN);

                        set_person_kick_dir(p_person, 1);

                        MSG_add("Kick right");
                        return (INPUT_MASK_KICK | INPUT_MASK_RIGHT);
                    }
                } else if ((input & INPUT_MASK_BACKWARDS) || dir == 3) {
                    if (p_person->State != STATE_JUMPING) {
                        turn_to_target(
                            p_person,
                            FIND_DIR_BACK | FIND_DIR_DONT_TURN);

                        p_person->Genus.Person->Timer1 = 6;

                        set_person_kick_dir(p_person, 2);

                        MSG_add("Kick back");
                        return (INPUT_MASK_KICK | INPUT_MASK_BACKWARDS);
                    }
                }
            }

            if (p_person->State != STATE_JUMPING) {
                // Forward kick.
                turn_to_target_and_kick(p_person);
            }

            pl->DoneSomething = UC_TRUE;
        } else if (pl->Pressed & INPUT_MASK_ACTION) {
            // ACTION in fight mode exits to idle.
            p_person->Genus.Person->Mode = PERSON_MODE_RUN;
            set_person_idle(p_person);

            pl->DoneSomething = UC_TRUE;
            return (INPUT_MASK_ACTION);
        }
        /*
                        if ((pl->Released & INPUT_MASK_LEFT) && !pl->DoneSomething)
                        {
                                turn_to_target(p_person, FIND_DIR_TURN_LEFT);
                        }

                        if ((pl->Released & INPUT_MASK_RIGHT) && !pl->DoneSomething)
                        {
                                turn_to_target(p_person, FIND_DIR_TURN_RIGHT);
                        }
        */
    } else {
        // Combo queuing: set REQUEST flags when buttons pressed during a fighting animation.
        if (pl->Pressed & INPUT_MASK_PUNCH) {
            if (!(p_person->Genus.Person->Flags & FLAG_PERSON_REQUEST_PUNCH)) {
                p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_PUNCH;

                // Timer used to check if the gap between combo stages is short enough.
                p_person->Genus.Person->pcom_ai_counter = 0;
            }
        }

        if (pl->Pressed & INPUT_MASK_KICK) {
            if (!(p_person->Genus.Person->Flags & FLAG_PERSON_REQUEST_KICK)) {
                p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_KICK;
                p_person->Genus.Person->pcom_ai_counter = 0;
            }
        }
    }

    return 0;
}

// uc_orig: apply_button_input_car (fallen/Source/interfac.cpp)
// Translates the input bitmask into steering/throttle/brake for the vehicle the player is driving.
// Analogue: uses joypad X axis with damping (STEERING_MAX_DELTA per frame).
// Digital: steering = -1/0/+1.
ULONG apply_button_input_car(Thing* p_furn, ULONG input)
{
    ULONG processed_input = 0;

    ASSERT(p_furn->Class == CLASS_VEHICLE);

    Vehicle* veh = p_furn->Genus.Vehicle;

    if (analogue) {
        /*
                        SLONG	dx,vx;

                        dx = GET_JOYX(input);	// -128 to 127
                        vx = p_furn->Velocity-1000;
                        SATURATE(vx,0,1000);

                        vx = 64000-(vx*48);

                        veh->IsAnalog = 1;

                        dx =(((dx+31)&0xffffffe0) * vx) >> 17;	// now -64 to +63, * (290/256)
                        SATURATE(dx,-32,32);

                        veh->Steering = dx;
        */

        // Damped analogue steering.
        static SWORD wCurrentSteering = 0;
#define STEERING_MAX_DELTA 30
        SWORD wSteering = (GET_JOYX(input));
        if ((input & (INPUT_MASK_LEFT | INPUT_MASK_RIGHT)) == 0) {
            wSteering = 0;
        }

        if (wCurrentSteering > wSteering) {
            wCurrentSteering -= STEERING_MAX_DELTA;
            if (wCurrentSteering < wSteering) {
                wCurrentSteering = wSteering;
            }
        } else {
            wCurrentSteering += STEERING_MAX_DELTA;
            if (wCurrentSteering > wSteering) {
                wCurrentSteering = wSteering;
            }
        }
        veh->Steering = wCurrentSteering;

        veh->IsAnalog = 1;

    } else {
        veh->IsAnalog = 0;
        veh->Steering = 0;

        if (input & INPUT_MASK_LEFT)
            veh->Steering--;
        if (input & INPUT_MASK_RIGHT)
            veh->Steering++;
    }

    veh->DControl = 0;

    // Keyboard uses PC button→action mapping (Z=accel, X=brake, Space=siren).
    // Gamepad uses PSX mapping (Cross=accel, Square=brake, Triangle=siren).
    if (active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE) {
        if (input & INPUT_CAR_KB_ACCELERATE)
            veh->DControl |= VEH_ACCEL;
        else if (input & INPUT_CAR_KB_DECELERATE)
            veh->DControl |= VEH_DECEL;
        if (input & INPUT_CAR_KB_GOFASTER)
            veh->DControl |= VEH_FASTER;
        if (input & INPUT_CAR_KB_SIREN)
            veh->DControl |= VEH_SIREN;
    } else {
        // R2 = alternative accelerate, L2 = alternative brake (in addition to Cross/Square).
        bool r2 = the_state.rgbButtons[16] != 0;
        bool l2 = the_state.rgbButtons[15] != 0;

        if ((input & INPUT_CAR_PAD_ACCELERATE) || r2)
            veh->DControl |= VEH_ACCEL;
        else if ((input & INPUT_CAR_PAD_DECELERATE) || l2)
            veh->DControl |= VEH_DECEL;
        if ((input & INPUT_CAR_PAD_GOFASTER) || r2 || l2)
            veh->DControl |= VEH_FASTER;
        if (input & INPUT_CAR_PAD_SIREN)
            veh->DControl |= VEH_SIREN;
    }

    return (processed_input);
}

// uc_orig: set_look_pitch (fallen/Source/interfac.cpp)
// Forces the first-person look pitch to a specific value (called from cutscene/EWAY code).
void set_look_pitch(SLONG p)
{
    look_pitch = p;
}

// uc_orig: get_last_input (fallen/Source/interfac.cpp)
// Returns the last polled hardware input. Pass INPUT_TYPE_GONEDOWN to get only newly-pressed buttons.
ULONG get_last_input(UWORD type)
{
    if (type == INPUT_TYPE_GONEDOWN) {
        return (m_CurrentGoneDownInput);
    } else {
        ASSERT(type == 0);
        return (m_CurrentInput);
    }
}

// uc_orig: get_hardware_input (fallen/Source/interfac.cpp)
// Polls the DirectInput device and keyboard, packs all button states and analog axes into a ULONG.
// Analog X axis goes to bits 18-24, Y axis to bits 25-31 (GET_JOYX/GET_JOYY unpack them).
// Pass INPUT_TYPE_KEY | INPUT_TYPE_JOY for full input; add INPUT_TYPE_GONEDOWN for edge-only.
ULONG get_hardware_input(UWORD type)
{
    // Modal input debug panel consumes all input — return no-input so
    // do_packets (called from process_things) doesn't drive the player
    // character while the panel is open.
    if (input_debug_is_active()) return 0;

    ULONG input = 0;

    static bool bLastInputWasntAnInputCozThereWasNoController = UC_TRUE;

#define BUTTON_IS_PRESSED(value) (value)

    // Deadzone constants for the DirectInput analogue stick (PC path: 0..65535, centre 32768).
    // Replace with SDL_GetGamepadAxis deadzone equivalent when porting to SDL3.
#define AXIS_CENTRE 32768
#define NOISE_TOLERANCE 8192
#define AXIS_MIN (AXIS_CENTRE - NOISE_TOLERANCE)
#define AXIS_MAX (AXIS_CENTRE + NOISE_TOLERANCE)

    // the_state (GamepadState) is filled by ReadInputDevice() → gamepad_poll() each frame.

    DWORD dwCurrentTime = 0;

    if (type & INPUT_TYPE_JOY) {
        if (ReadInputDevice()) {
            {
                ULONG ulAxisMax = AXIS_MAX;
                ULONG ulAxisMin = AXIS_MIN;

                if (the_state.connected && !the_state.dpad_active) {
                    // Analog stick mode: pack position into bits 18-31.
                    // process_analogue_movement() uses these for proportional speed.
                    // Digital direction flags set only for menus (not movement).
                    analogue = 1;
                    // Apply deadzone before packing — stick drift within NOISE_TOLERANCE
                    // must pack as centre, otherwise continue_moveing() sees phantom input.
                    SLONG pack_x = the_state.lX;
                    SLONG pack_y = the_state.lY;
                    if (pack_x >= (SLONG)ulAxisMin && pack_x <= (SLONG)ulAxisMax) pack_x = AXIS_CENTRE;
                    if (pack_y >= (SLONG)ulAxisMin && pack_y <= (SLONG)ulAxisMax) pack_y = AXIS_CENTRE;
                    input |= ((pack_x >> 9) + 0) << 18;
                    input |= ((pack_y >> 9) + 0) << 25;

                } else {
                    // D-Pad or no gamepad: digital mode (full speed).
                    analogue = 0;
                }

                // Always set digital direction flags from axes (needed for menus, D-Pad).
                if (the_state.lX > ulAxisMax) {
                    g_dwLastInputChangeTime = dwCurrentTime;
                    input |= INPUT_MASK_RIGHT;
                } else if (the_state.lX < ulAxisMin) {
                    g_dwLastInputChangeTime = dwCurrentTime;
                    input |= INPUT_MASK_LEFT;
                }

                if (the_state.lY > ulAxisMax) {
                    g_dwLastInputChangeTime = dwCurrentTime;
                    input |= INPUT_MASK_BACKWARDS;
                } else if (the_state.lY < ulAxisMin) {
                    input |= INPUT_MASK_FORWARDS;
                    input |= INPUT_MASK_MOVE;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                if (BUTTON_IS_PRESSED(the_state.rgbButtons[joypad_button_use[JOYPAD_BUTTON_MOVE]])) {
                    m_bForceWalk = UC_TRUE;
                } else {
                    m_bForceWalk = UC_FALSE;
                }

                if (BUTTON_IS_PRESSED(the_state.rgbButtons[joypad_button_use[JOYPAD_BUTTON_JUMP]])) {
                    input |= INPUT_MASK_JUMP;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                if (BUTTON_IS_PRESSED(the_state.rgbButtons[joypad_button_use[JOYPAD_BUTTON_KICK]])) {
                    // Triangle/Y stays as menu CANCEL (PS1 behavior); on foot
                    // KICK lives on L2 only — see the analog-trigger block below.
                    // When driving, L2/R2 serve as gas/brake (handled in
                    // apply_button_input_car), so the analog trigger kick path
                    // never fires. The in-car siren/light toggle is bound to
                    // INPUT_CAR_PAD_SIREN == INPUT_MASK_KICK, so while driving
                    // the triangle button must also emit INPUT_MASK_KICK, or
                    // nothing will toggle the siren.
                    input |= INPUT_MASK_CANCEL;
                    {
                        Thing* p_darci = NET_PERSON(0);
                        const bool driving = p_darci && p_darci->Genus.Person &&
                                             (p_darci->Genus.Person->Flags & FLAG_PERSON_DRIVING);
                        if (driving) input |= INPUT_MASK_KICK;
                    }
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // Square/X previously mapped to PUNCH/shoot; removed so that
                // punch & shoot live only on R2. Keep the button unbound in
                // gameplay — nothing to do here.

                if (BUTTON_IS_PRESSED(the_state.rgbButtons[joypad_button_use[JOYPAD_BUTTON_START]])) {
                    input |= INPUT_MASK_START;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // Weapon cycle / inventory rotation moved from Share/Back
                // (JOYPAD_BUTTON_SELECT) to R3 (right stick click). Index 8
                // in rgbButtons matches SDL3_BTN_RIGHT_STICK and the DualSense
                // R3 mapping.
                if (BUTTON_IS_PRESSED(the_state.rgbButtons[8])) {
                    input |= INPUT_MASK_SELECT;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                if (BUTTON_IS_PRESSED(the_state.rgbButtons[joypad_button_use[JOYPAD_BUTTON_CAMERA]])) {
                    input |= INPUT_MASK_CAMERA;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // L2/R2 no longer map to camera rotation on gamepad — right stick handles camera.
                // On foot: R2 = alternative PUNCH (shoot/melee), L2 = alternative KICK.
                // In car: gas/brake handled in apply_button_input_car() via rgbButtons directly.
                // Keyboard CAM_LEFT/CAM_RIGHT mapping is preserved below.
                {
                    Thing* p_darci = NET_PERSON(0);
                    bool driving = p_darci && p_darci->Genus.Person &&
                                   (p_darci->Genus.Person->Flags & FLAG_PERSON_DRIVING);
                    if (!driving) {
                        // Fire detection (rising-edge vs held-down + thresholds)
                        // is fully encapsulated in the weapon_feel module. The
                        // active WeaponFeelProfile drives both the R2/L2 firing
                        // thresholds and whether auto-fire is allowed, and it's
                        // the same profile the adaptive-trigger state machine
                        // consults — so shot and click can't desync.
                        int32_t current_weapon = SPECIAL_NONE;
                        if (p_darci && p_darci->Genus.Person && p_darci->Genus.Person->SpecialUse) {
                            Thing* p_special = TO_THING(p_darci->Genus.Person->SpecialUse);
                            if (p_special) current_weapon = p_special->Genus.Special->SpecialType;
                        }

                        WeaponFireDecision fd = weapon_feel_evaluate_fire(
                            current_weapon, the_state.trigger_right, the_state.trigger_left);
                        if (fd.shoot) {
                            input |= INPUT_MASK_PUNCH;
                            g_dwLastInputChangeTime = dwCurrentTime;
                        }
                        if (fd.kick) {
                            input |= INPUT_MASK_KICK;
                            g_dwLastInputChangeTime = dwCurrentTime;
                        }
                    }
                }

                if (BUTTON_IS_PRESSED(the_state.rgbButtons[joypad_button_use[JOYPAD_BUTTON_ACTION]])) {
                    MSG_add(" action pressed \n");
                    input |= INPUT_MASK_ACTION;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // Gamepad cheats: Select + L1 + L2 + D-pad direction.
                // Ported from Dreamcast cheats (fallen/Source/interfac.cpp).
                // Works on DualSense (Share+L1+L2) and Xbox (Back+LB+LT).
                {
                    static bool bCheatLastFrame = false;
                    bool cheat_combo = BUTTON_IS_PRESSED(the_state.rgbButtons[4])   // Select/Back
                                    && BUTTON_IS_PRESSED(the_state.rgbButtons[9])   // L1/LB
                                    && BUTTON_IS_PRESSED(the_state.rgbButtons[15]);  // L2/LT (digital)

                    if (cheat_combo) {
                        if (BUTTON_IS_PRESSED(the_state.rgbButtons[11])) {
                            // D-pad Up: toggle immortality (invulnerability flag on player).
                            if (!bCheatLastFrame) {
                                bCheatLastFrame = true;
                                NET_PERSON(0)->Genus.Person->Flags2 ^= FLAG2_PERSON_INVULNERABLE;
                                if (NET_PERSON(0)->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE)
                                    CONSOLE_text((CBYTE*)"I am immortal, I have inside me blood of kings.");
                                else
                                    CONSOLE_text((CBYTE*)"I'm just a mortal after all.");
                            }
                        } else if (BUTTON_IS_PRESSED(the_state.rgbButtons[12])) {
                            // D-pad Down: full health.
                            if (!bCheatLastFrame) {
                                bCheatLastFrame = true;
                                NET_PERSON(0)->Genus.Person->Health = 1000;
                                CONSOLE_text((CBYTE*)"My wings are as a shield of steel.");
                            }
                        } else if (BUTTON_IS_PRESSED(the_state.rgbButtons[13])) {
                            // D-pad Left: spawn weapons around player (AK47, shotgun, pistol, 3 grenades).
                            if (!bCheatLastFrame) {
                                bCheatLastFrame = true;
                                #define CHEAT_RING_SIZE 128
                                alloc_special(SPECIAL_AK47,    SPECIAL_SUBSTATE_NONE, (NET_PERSON(0)->WorldPos.X >> 8) + CHEAT_RING_SIZE,  NET_PERSON(0)->WorldPos.Y >> 8, (NET_PERSON(0)->WorldPos.Z >> 8) + CHEAT_RING_SIZE, 0);
                                alloc_special(SPECIAL_SHOTGUN, SPECIAL_SUBSTATE_NONE, (NET_PERSON(0)->WorldPos.X >> 8) - CHEAT_RING_SIZE,  NET_PERSON(0)->WorldPos.Y >> 8, (NET_PERSON(0)->WorldPos.Z >> 8) - CHEAT_RING_SIZE, 0);
                                alloc_special(SPECIAL_GUN,     SPECIAL_SUBSTATE_NONE, (NET_PERSON(0)->WorldPos.X >> 8) - CHEAT_RING_SIZE,  NET_PERSON(0)->WorldPos.Y >> 8, (NET_PERSON(0)->WorldPos.Z >> 8) + CHEAT_RING_SIZE, 0);
                                alloc_special(SPECIAL_GRENADE, SPECIAL_SUBSTATE_NONE, (NET_PERSON(0)->WorldPos.X >> 8) + CHEAT_RING_SIZE - 32, NET_PERSON(0)->WorldPos.Y >> 8, (NET_PERSON(0)->WorldPos.Z >> 8) - CHEAT_RING_SIZE - 32, 0);
                                alloc_special(SPECIAL_GRENADE, SPECIAL_SUBSTATE_NONE, (NET_PERSON(0)->WorldPos.X >> 8) + CHEAT_RING_SIZE,      NET_PERSON(0)->WorldPos.Y >> 8, (NET_PERSON(0)->WorldPos.Z >> 8) - CHEAT_RING_SIZE, 0);
                                alloc_special(SPECIAL_GRENADE, SPECIAL_SUBSTATE_NONE, (NET_PERSON(0)->WorldPos.X >> 8) + CHEAT_RING_SIZE + 32, NET_PERSON(0)->WorldPos.Y >> 8, (NET_PERSON(0)->WorldPos.Z >> 8) - CHEAT_RING_SIZE + 32, 0);
                                #undef CHEAT_RING_SIZE
                                CONSOLE_text((CBYTE*)"We need guns. Lots of guns.");
                            }
                        } else if (BUTTON_IS_PRESSED(the_state.rgbButtons[14])) {
                            // D-pad Right: max ammo for all weapon types.
                            if (!bCheatLastFrame) {
                                bCheatLastFrame = true;
                                NET_PERSON(0)->Genus.Person->ammo_packs_pistol = 240;
                                NET_PERSON(0)->Genus.Person->ammo_packs_shotgun = 240;
                                NET_PERSON(0)->Genus.Person->ammo_packs_ak47 = 240;
                                CONSOLE_text((CBYTE*)"Well, to tell you the truth, in all this excitement, I've kinda lost track myself.");
                            }
                        } else {
                            bCheatLastFrame = false;
                        }

                        // Suppress normal button input while cheat combo is active.
                        input &= ~INPUT_MASK_ALL_BUTTONS;
                    } else {
                        bCheatLastFrame = false;
                    }
                }
            }

            if (input) {
                input_mode = INPUT_JOYPAD;
            }
        } else {
            bLastInputWasntAnInputCozThereWasNoController = UC_TRUE;
        }
    }

    // When gamepad is connected, disable analog mode if keyboard provides input
    // (analog mode is re-enabled next frame if gamepad has stick input).

    if (type & INPUT_TYPE_KEY) {

        // If any movement key is pressed, switch to digital mode
        // (analog mode only makes sense with a stick).
        // Also clear analog bits 18-31 — otherwise player_turn_left_right sees
        // non-zero upper bits and treats centered stick (value 0) as analog input.
        if (Keys[keybrd_button_use[KEYBRD_BUTTON_FORWARDS]] ||
            Keys[keybrd_button_use[KEYBRD_BUTTON_BACK]] ||
            Keys[keybrd_button_use[KEYBRD_BUTTON_LEFT]] ||
            Keys[keybrd_button_use[KEYBRD_BUTTON_RIGHT]]) {
            analogue = 0;
            input &= 0x0003FFFF;
        }

        if (Keys[keybrd_button_use[KEYBRD_BUTTON_FORWARDS]]) {
            input |= INPUT_MASK_FORWARDS;
        }

        if (Keys[keybrd_button_use[KEYBRD_BUTTON_BACK]])
            input |= INPUT_MASK_BACKWARDS;

        if (Keys[keybrd_button_use[KEYBRD_BUTTON_LEFT]]) {
            if (ShiftFlag)
                input |= INPUT_MASK_STEP_LEFT;
            else
                input |= INPUT_MASK_LEFT;
        }

        if (Keys[keybrd_button_use[KEYBRD_BUTTON_RIGHT]]) {
            if (ShiftFlag)
                input |= INPUT_MASK_STEP_RIGHT;
            else
                input |= INPUT_MASK_RIGHT;
        }

        if (Keys[keybrd_button_use[JOYPAD_BUTTON_SELECT]])
            input |= INPUT_MASK_SELECT;

        if (Keys[KB_F5]) {
            input |= INPUT_MASK_CAMERA;
            input &= ~INPUT_MASKM_CAM_TYPE;
            input |= INPUT_MASKM_CAM1;
            Keys[KB_F5] = 0;
        }
        if (Keys[KB_F6]) {
            input |= INPUT_MASK_CAMERA;
            input &= ~INPUT_MASKM_CAM_TYPE;
            input |= INPUT_MASKM_CAM2;
            Keys[KB_F6] = 0;
        }
        if (Keys[KB_F7]) {
            input |= INPUT_MASK_CAMERA;
            input &= ~INPUT_MASKM_CAM_TYPE;
            input |= INPUT_MASKM_CAM3;
            Keys[KB_F7] = 0;
        }

        /*
        if(Keys[KB_F8])
        {
                input|=INPUT_MASK_CAMERA;
                input&=~INPUT_MASKM_CAM_TYPE;
                input|=INPUT_MASKM_CAM4;
                Keys[KB_F8]=0;
        }
        */

        if (Keys[keybrd_button_use[JOYPAD_BUTTON_CAMERA]]) {
            Keys[keybrd_button_use[JOYPAD_BUTTON_CAMERA]] = 0;
            input |= INPUT_MASK_CAM_BEHIND;
        }

        if (Keys[keybrd_button_use[JOYPAD_BUTTON_CAM_LEFT]]) {
            Keys[JOYPAD_BUTTON_CAM_LEFT] = 0;
            input |= INPUT_MASK_CAM_LEFT;
        }
        if (Keys[keybrd_button_use[JOYPAD_BUTTON_CAM_RIGHT]]) {
            Keys[keybrd_button_use[JOYPAD_BUTTON_CAM_RIGHT]] = 0;
            input |= INPUT_MASK_CAM_RIGHT;
        }

        if (Keys[keybrd_button_use[JOYPAD_BUTTON_JUMP]])
            input |= INPUT_MASK_JUMP;

        if (Keys[keybrd_button_use[JOYPAD_BUTTON_PUNCH]]) {
            input |= INPUT_MASK_PUNCH;
        }
        if (Keys[keybrd_button_use[JOYPAD_BUTTON_KICK]]) {
            MSG_add(" HARDWARE KICK");
            input |= INPUT_MASK_KICK;
        }

        if (Keys[keybrd_button_use[JOYPAD_BUTTON_ACTION]]) {
            input |= INPUT_MASK_ACTION;
        }

        /*

        // Take out the V key?!

        if(Keys[keybrd_button_use[JOYPAD_BUTTON_MOVE]])
        {
                input|=INPUT_MASK_MOVE;
        }
        */

        if (Keys[keybrd_button_use[KEYBRD_BUTTON_FORWARDS]]) {
            input |= INPUT_MASK_MOVE;
        }
    }

    /*

    if (EWAY_stop_player_moving())
    {
        input&=INPUT_MASK_JUMP;
    }

    */

    /*
            if (SNIPE_on)
            {
                    ...sniper mode input (all commented out in original)...
            }
    */

    if (input) {
        input_mode = INPUT_KEYS;
    }

    m_CurrentInput = input;
    m_CurrentGoneDownInput = (m_CurrentInput & ~(m_PreviousInput)) & INPUT_MASK_ALL_BUTTONS;
    m_PreviousInput = m_CurrentInput;
    if (type & INPUT_TYPE_GONEDOWN) {
        return (m_CurrentGoneDownInput);
    } else {
        return (m_CurrentInput);
    }
}

// uc_orig: allow_input_autorepeat (fallen/Source/interfac.cpp)
// Clears the previous-input state so the next get_hardware_input(GONEDOWN) will return all
// currently-held buttons as "newly pressed" (used in menus to allow held navigation).
void allow_input_autorepeat(void)
{
    m_PreviousInput = 0;
}

// uc_orig: pre_process_input (fallen/Source/interfac.cpp)
// Hook for remapping input before dispatching. Currently a pass-through (all remapping
// code is commented out in the original).
ULONG pre_process_input(SLONG mode, ULONG input)
{
    return (input);
    /*
            UWORD	output;

            output=input;

            switch(mode)
            {
                    case	PERSON_MODE_FIGHT:
                            break;
                    case	PERSON_MODE_RUN:
                            if(input&(INPUT_MASK_SPRINT|INPUT_MASK_YOMP))
                            {
                                    output|=INPUT_MASK_FORWARDS;
                            }
                            break;
            }
            return(output);
    */
}

// uc_orig: apply_button_input_first_person (fallen/Source/interfac.cpp)
// Handles first-person look-around mode when JOYPAD_BUTTON_1STPERSON is held.
// Rotates the camera pitch and character angle without consuming movement input.
// Returns non-zero if first-person mode is active this frame.
ULONG apply_button_input_first_person(Thing* p_player, Thing* p_person, ULONG input, ULONG* processed)
{
    static SLONG look_ami = UC_FALSE;
    SLONG fpm = UC_FALSE;

    *processed = 0;

    // the_state (GamepadState) for the 1st-person button check.

    if ((Keys[keybrd_button_use[JOYPAD_BUTTON_1STPERSON]]) || the_state.rgbButtons[joypad_button_use[JOYPAD_BUTTON_1STPERSON]]) {
        fpm = UC_TRUE;
    }

    if (p_person->State != STATE_IDLE && p_person->State != STATE_GUN && p_person->State != STATE_NORMAL && p_person->State != STATE_HIT_RECOIL) {
        fpm = UC_FALSE;
    }

    /*
    else
    if (p_person->Genus.Person->Action == ACTION_AIM_GUN || ...)
    {
        ...draw-gun / special aim cases (commented out in original)...
    }
    */

    if (fpm) {

        if (!look_ami) {
            p_person->Genus.Person->Flags2 |= FLAG2_PERSON_LOOK;
            look_ami = UC_TRUE;
            look_pitch = -FC_cam[p_person->Genus.Person->PlayerID - 1].pitch >> 8;
            look_pitch &= 2047;

            if (look_pitch > 1024) {
                look_pitch -= 2048;
            }

            look_pitch >>= 4;
            look_pitch &= 2047;
        }

        if (mouse_input) {
            if (MouseDY) {
                look_pitch -= MouseDY;
            }
            if (MouseDX) {
                p_person->Draw.Tweened->Angle = (p_person->Draw.Tweened->Angle - MouseDX) & 2047;
            }
        }

        if (input & INPUT_MASK_FORWARDS) {
            look_pitch += 13;
            input &= ~INPUT_MASK_MOVE;
        }
        if (input & INPUT_MASK_BACKWARDS) {
            look_pitch -= 13;
        }

        if (!CONTROLS_inventory_mode) {
            if (input & INPUT_MASK_LEFT) {
                p_person->Draw.Tweened->Angle = (p_person->Draw.Tweened->Angle + 32) & 2047;
            }

            if (input & INPUT_MASK_RIGHT) {
                p_person->Draw.Tweened->Angle = (p_person->Draw.Tweened->Angle - 32) & 2047;
            }
        }

        if (input & INPUT_MASK_MOVE) {
            set_person_running(p_person);
        }

        if (look_pitch > 256 && look_pitch <= 1024) {
            look_pitch = 256;
        }
        if (look_pitch < 1850 && look_pitch >= 1024) {
            look_pitch = 1850;
        }

        look_pitch &= 2047;

        FC_position_for_lookaround(p_person->Genus.Person->PlayerID - 1, look_pitch);

        if (person_has_gun_out(p_person)) {
            if (input & INPUT_MASK_PUNCH) {
                if (p_person->Genus.Person->Action == ACTION_SHOOT) {
                    /*

                    //
                    // Only the pistol animation can be interrupted so you
                    // can shoot again.
                    //

                    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)
                    {
                            set_player_shoot(p_person,0);
                            *processed|=INPUT_MASK_PUNCH; //needs a clear click
                    }

                    */
                } else {
                    set_player_shoot(p_person, 0);
                    *processed |= INPUT_MASK_PUNCH;
                }
            }

            /*
            if(input&INPUT_MASK_SELECT)
            {
                    set_person_gun_away(p_person);
                    *processed|=INPUT_MASK_SELECT;

            }
            */
        }
    } else {
        if (look_ami) {
            p_person->Genus.Person->Flags2 &= ~FLAG2_PERSON_LOOK;

            if (CNET_network_game) {
                if (p_person->Genus.Person->PlayerID - 1 == PLAYER_ID) {
                    FC_force_camera_behind(0);
                }
            } else
                FC_force_camera_behind(p_person->Genus.Person->PlayerID - 1);

            look_ami = UC_FALSE;
            look_pitch = 0;
        }
    }

    FirstPersonMode = fpm;
    return (fpm);
}

// uc_orig: can_darci_change_weapon (fallen/Source/interfac.cpp)
// Returns true if the player character is in a state that allows weapon switching.
// Blocks during reload animations, grabs, death slides, etc.
SLONG can_darci_change_weapon(Thing* p_person)
{
    if (EWAY_stop_player_moving()) {
        return UC_FALSE;
    }

    if (p_person->State == STATE_IDLE) {
        return UC_TRUE;
    }

    if (p_person->State == STATE_MOVEING) {
        if (p_person->SubState == SUB_STATE_RUNNING || p_person->SubState == SUB_STATE_WALKING) {
            return UC_TRUE;
        }
    }

    if (p_person->State == STATE_GUN) {
        if (p_person->SubState == SUB_STATE_AIM_GUN) {
            return UC_TRUE;
        }
    }
    if (p_person->SubState == SUB_STATE_ITEM_AWAY)
        return UC_TRUE;

    if (p_person->SubState == SUB_STATE_DRAW_ITEM)
        return UC_TRUE;

    if (p_person->SubState == SUB_STATE_DRAW_GUN)
        return UC_TRUE;

    return UC_FALSE;
}

// uc_orig: process_hardware_level_input_for_player (fallen/Source/interfac.cpp)
// Per-frame entry point for player input. Reads the packet for this player, handles camera
// switching, applies InputDone masking, and dispatches to the appropriate input handler
// (driving / fight / normal).
void process_hardware_level_input_for_player(Thing* p_player)
{
    // Modal input debug panel — bail out entirely so neither the packet
    // path nor the direct Keys[]/rgbButtons[] reads inside this dispatcher
    // (e.g. apply_button_input_first_person) can drive the player.
    if (input_debug_is_active()) return;

    SLONG i;

    ULONG input;
    ULONG processed = 0;
    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
    uint64_t tick = sdl3_get_ticks();

    Thing* p_person;
    p_person = p_player->Genus.Player->PlayerPerson;

    input = PACKET_DATA(p_player->Genus.Player->PlayerID);

    Player* pl = p_player->Genus.Player;

    // Camera controls are processed before InputDone / cutscene blocking,
    // so the player can still switch cameras during cutscenes.
    if (pl->Pressed & INPUT_MASK_CAMERA) {
        g_iPlayerCameraMode = input & INPUT_MASKM_CAM_TYPE;
        g_iPlayerCameraMode >>= INPUT_MASKM_CAM_SHIFT;
        input &= ~INPUT_MASKM_CAM_TYPE;
        if (CNET_network_game) {
            if (p_person->Genus.Person->PlayerID - 1 == PLAYER_ID) {
                FC_change_camera_type(0, g_iPlayerCameraMode);
                FC_force_camera_behind(0);
            }
        } else {

            FC_change_camera_type(p_person->Genus.Person->PlayerID - 1, g_iPlayerCameraMode);
            FC_force_camera_behind(p_person->Genus.Person->PlayerID - 1);
        }
    }

    if (input & INPUT_MASK_CAM_BEHIND) {

        if (CNET_network_game) {
            if (p_person->Genus.Person->PlayerID - 1 == PLAYER_ID) {
                FC_force_camera_behind(0);
            }
        } else {
            FC_force_camera_behind(p_person->Genus.Person->PlayerID - 1);
        }
    }

    if (input & INPUT_MASK_CAM_LEFT) {
        if (CNET_network_game) {
            if (p_person->Genus.Person->PlayerID - 1 == PLAYER_ID) {
                FC_rotate_left(p_person->Genus.Person->PlayerID - 1);
            }
        } else
            FC_rotate_left(p_person->Genus.Person->PlayerID - 1);
    }

    if (input & INPUT_MASK_CAM_RIGHT) {
        if (CNET_network_game) {
            if (p_person->Genus.Person->PlayerID - 1 == PLAYER_ID) {
                FC_rotate_left(0);
            }
        } else
            FC_rotate_right(p_person->Genus.Person->PlayerID - 1);
    }

    // Compute per-frame pressed/released edge states.
    pl->LastInput = pl->ThisInput;
    pl->ThisInput = input;

    pl->Pressed = pl->ThisInput & ~pl->LastInput;
    pl->Released = ~pl->ThisInput & pl->LastInput;

    /*

    if (pl->Pressed & INPUT_MASK_JUMP)
    {
            CBYTE str[100];

            sprintf(str, "Pressed jump turn %d\n", GAME_TURN);

    }

    */

    if (pl->Pressed) {
        pl->DoneSomething = UC_FALSE;
    }

    // Double-click detection: DoubleClick[i] counts consecutive presses of button i within 200ms.
    for (i = 0; i < 16; i++) {
        if (pl->Pressed & (1 << i)) {
            if (pl->LastReleased[i] >= tick - 200) {
                pl->DoubleClick[i] += 1;
            } else {
                pl->DoubleClick[i] = 1;
            }
        }
    }

    for (i = 0; i < 16; i++) {
        if (pl->Released & (1 << i)) {
            pl->LastReleased[i] = tick;
        }
    }

    if (input & INPUT_MASK_ACTION) {
        MSG_add(" still action");
    }

    SLONG no_control = UC_FALSE;

    if (EWAY_stop_player_moving()) {
        // Mid-cutscene: suppress all player input and reset map-leave timer.
        input = 0;
        form_left_map = 15;
    }

    if (form_leave_map) {
        input = 0;
    }

    if (draw_map_screen) {
        if ((input & INPUT_MASK_SELECT) && (input & INPUT_MASK_MOVE)) {
            // SELECT + MOVE on the map screen quits the current session.
            GAME_STATE = 0;
        }

        input = 0;
    }

    if (form_left_map) {
        input = 0;
        form_left_map -= 1;
    }

    // InputDone: bitmask of buttons that were already processed in a previous frame and are
    // still held. Clear bits that have been released, then mask them out of this frame's input.
    p_player->Genus.Player->InputDone &= (input & 0x3ffff);
    p_player->Genus.Player->Input = input;

    input &= ~(p_player->Genus.Player->InputDone);

    if (!no_control) {
        if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
            // Can't draw weapons while driving.
        } else {
            // Keyboard weapon hotkeys (PC only).
            if (can_darci_change_weapon(p_person)) {
                if (Keys[KB_1]) {
                    Keys[KB_1] = 0;

                    if ((p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) || (p_person->Genus.Person->SpecialUse)) {
                        set_person_gun_away(p_person);
                    }
                }

                if (Keys[KB_2]) {
                    Keys[KB_2] = 0;

                    if (!(p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)) {
                        if (p_person->Flags & FLAGS_HAS_GUN) {
                            if (p_person->Genus.Person->SpecialUse) {
                                set_person_item_away(p_person);
                            }

                            set_person_draw_gun(p_person);
                        } else {
                            /*
                                                                                    add_damage_text(
                                                                                            p_person->WorldPos.X          >> 8,
                                                                                            p_person->WorldPos.Y + 0x6000 >> 8,
                                                                                            p_person->WorldPos.Z          >> 8,
                                                                                            "No Gun");
                            */
                        }
                    }
                }

                SLONG special_type = SPECIAL_NONE;

                if (Keys[KB_3]) {
                    Keys[KB_3] = 0;
                    special_type = SPECIAL_SHOTGUN;
                }
                if (Keys[KB_4]) {
                    Keys[KB_4] = 0;
                    special_type = SPECIAL_AK47;
                }
                if (Keys[KB_5]) {
                    Keys[KB_5] = 0;
                    special_type = SPECIAL_GRENADE;
                }
                if (Keys[KB_6]) {
                    Keys[KB_6] = 0;
                    special_type = SPECIAL_EXPLOSIVES;
                }
                if (Keys[KB_7]) {
                    Keys[KB_7] = 0;
                    special_type = SPECIAL_KNIFE;
                }
                if (Keys[KB_8]) {
                    Keys[KB_8] = 0;
                    special_type = SPECIAL_BASEBALLBAT;
                }

                if (special_type) {
                    if (person_has_special(p_person, special_type)) {
                        if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
                            set_person_gun_away(p_person);
                        }

                        set_person_draw_item(p_person, special_type);
                    } else {
                        /*
                                                                        CBYTE str[40];

                                                                        sprintf(str, "No %s", SPECIAL_info[special_type].name);

                                                                        add_damage_text(
                                                                                p_person->WorldPos.X >> 8,
                                                                                p_person->WorldPos.Y + 0x6000 >> 8,
                                                                                p_person->WorldPos.Z          >> 8,
                                                                                str);
                        */
                    }
                }
            }
        }

        if (!apply_button_input_first_person(p_player, p_person, input, &processed)) {

            if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                ASSERT(p_person->Genus.Person->InCar);

                processed = apply_button_input_car(TO_THING(p_person->Genus.Person->InCar), input);
                processed |= apply_button_input(p_player, p_person, 0);
            }
            {
                // Main input dispatcher: fight mode vs normal run mode.
                // pre_process_input() can remap buttons between modes (currently pass-through).
                if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
                    input = pre_process_input(PERSON_MODE_FIGHT, input);
                    processed = apply_button_input_fight(p_player, p_person, input);
                } else {
                    input = pre_process_input(PERSON_MODE_RUN, input);
                    processed = apply_button_input(p_player, p_person, input);
                }
            }
        }
    }

    p_player->Genus.Player->InputDone |= processed;
}

// uc_orig: continue_action (fallen/Source/interfac.cpp)
// Returns true if the player is still holding the input that triggered the current flip action.
SLONG continue_action(Thing* p_person)
{
    Thing* p_player;
    ULONG input, input_used, new_action;
    if (p_person->Genus.Person->PlayerID) {
        p_player = NET_PLAYER(p_person->Genus.Person->PlayerID - 1);
        input = p_player->Genus.Player->Input;
        MSG_add(" continue action %d  input %x \n", p_person->Genus.Person->Action, input);
        switch (p_person->Genus.Person->Action) {
        case ACTION_FLIP_LEFT:
            new_action = find_best_action_from_tree(ACTION_IDLE, input, &input_used);
            if (new_action == ACTION_FLIP_LEFT)
                return (1);
            break;
        case ACTION_FLIP_RIGHT:
            new_action = find_best_action_from_tree(ACTION_IDLE, input, &input_used);
            if (new_action == ACTION_FLIP_RIGHT)
                return (1);
            break;
        }
    }
    return (0);
}

// uc_orig: continue_pressing_action (fallen/Source/interfac.cpp)
// Returns true if the player is still holding the ACTION button.
SLONG continue_pressing_action(Thing* p_person)
{
    Thing* p_player;
    ULONG input;
    if (p_person->Genus.Person->PlayerID) {
        p_player = NET_PLAYER(p_person->Genus.Person->PlayerID - 1);
        input = p_player->Genus.Player->Input;
        if (input & INPUT_MASK_ACTION)
            return (1);
        else
            return (0);
    }
    return (1);
}

// uc_orig: set_action_used (fallen/Source/interfac.cpp)
// Marks the ACTION button as consumed for this press so it is not re-processed next frame.
void set_action_used(Thing* p_person)
{
    Thing* p_player;
    if (p_person->Genus.Person->PlayerID) {
        p_player = NET_PLAYER(p_person->Genus.Person->PlayerID - 1);
        p_player->Genus.Player->InputDone |= INPUT_MASK_ACTION;
    }
}

// uc_orig: continue_dir (fallen/Source/interfac.cpp)
// Returns true if the player is holding the LEFT (dir==0) or RIGHT (dir==1) input.
SLONG continue_dir(Thing* p_person, SLONG dir)
{
    Thing* p_player;
    ULONG input;
    if (p_person->Genus.Person->PlayerID) {
        p_player = NET_PLAYER(p_person->Genus.Person->PlayerID - 1);
        input = p_player->Genus.Player->Input;
        if (dir == 1) {
            if (input & INPUT_MASK_RIGHT)
                return (1);
        }

        if (dir == 0) {
            if (input & INPUT_MASK_LEFT)
                return (1);
        }
    }

    return (0);
}

// uc_orig: continue_firing (fallen/Source/interfac.cpp)
// Returns true if the player/NPC should keep firing (PUNCH held, AK47 has ammo, target alive).
SLONG continue_firing(Thing* p_person)
{
    Thing* p_player;
    ULONG input;

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        if (p_special->Genus.Special->SpecialType == SPECIAL_AK47) {
            if (p_special->Genus.Special->ammo == 0) {
                return UC_FALSE;
            }
        }
    }

    if (p_person->Genus.Person->PlayerID) {
        p_player = NET_PLAYER(p_person->Genus.Person->PlayerID - 1);
        input = PACKET_DATA(p_player->Genus.Player->PlayerID);

        if (input & INPUT_MASK_PUNCH) {
            return UC_TRUE;
        } else {
            return UC_FALSE;
        }
    } else {
        Thing* p_target;
        UWORD i_target;

        // NPC: fire until the target is dead.
        i_target = PCOM_person_wants_to_kill(p_person);

        if (i_target) {
            p_target = TO_THING(i_target);

            if (p_target->State == STATE_DEAD) {
                return UC_FALSE;
            } else {
                return UC_TRUE;
            }
        }

        return UC_FALSE;
    }
}

// uc_orig: continue_moveing (fallen/Source/interfac.cpp)
// Returns true if the player/NPC should keep moving. For player: checks analog stick magnitude
// and direction vs current facing; for NPC: delegates to PCOM navigation.
SLONG continue_moveing(Thing* p_person)
{
    Thing* p_player;
    ULONG input;
    if (p_person->Genus.Person->PlayerID) {
        p_player = NET_PLAYER(p_person->Genus.Person->PlayerID - 1);
        input = p_player->Genus.Player->Input;
        if (analogue) {
            SLONG angle, dx, dy;

            dx = abs((SLONG)GET_JOYX(input));
            dy = abs((SLONG)GET_JOYY(input));

            if (QDIST2(dx, dy) < ANALOGUE_MIN_VELOCITY) {
                return (0);
            }

            if (p_person->State == STATE_JUMPING)
                return (1);

            angle = get_joy_angle(input, JOY_REL_CAMERA);

            angle -= p_person->Draw.Tweened->Angle;

            if (abs(angle) < 512 || angle > 2048 - 512) {
                return (1);
            } else {
                return (0);
            }

        } else {
            if (input & (INPUT_MASK_MOVE | INPUT_MASK_FORWARDS))
                return (1);
            else
                return (0);
        }
    } else {
        return PCOM_jumping_navigating_person_continue_moving(p_person);
    }
}

// uc_orig: continue_blocking (fallen/Source/interfac.cpp)
// Returns true if the player is still holding BACKWARDS (block direction) input.
SLONG continue_blocking(Thing* p_person)
{
    Thing* p_player;
    ULONG input;
    if (p_person->Genus.Person->PlayerID) {
        p_player = NET_PLAYER(p_person->Genus.Person->PlayerID - 1);
        input = p_player->Genus.Player->Input;
        if (input & (INPUT_MASK_BACKWARDS))
            return (1);
        else {
            return (0);
        }
    } else {
        return 0;
    }
}
