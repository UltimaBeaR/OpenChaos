// Player input processing: hardware input reading, action dispatch, movement.
// Translates keyboard/gamepad state into game actions for the player character.

#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "game/action_map/act_foot.h" // ACT_FOOT_*
#include "game/action_map/act_car.h" // ACT_CAR_*
#include "engine/debug/input_debug/input_debug.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/io/env.h"
#include "things/core/thing.h"
#include "things/core/player.h"
#include "things/core/interact.h"
#include "things/core/state.h"
#include "things/characters/person.h"
#include "things/items/special.h"
#include "things/items/hook.h"
#include "things/vehicles/vehicle.h"
#include "map/ob.h"
#include "map/pap.h"
#include "buildings/ware.h"
#include "ai/mav.h"
#include "ai/pcom.h"
#include "missions/eway.h"
#include "missions/eway_globals.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "camera/look_sensitivity.h" // zoom look scales (track sensitivity, ZOOM_LOOK_RATIO of orbit)
#include "world_objects/dirt.h"

#include "map/supermap.h"
#include "map/level_pools.h"
// actors/characters/person.h already included above (line 14)

#include "things/characters/anim_ids.h"
#include "things/core/statedef.h"
#include "engine/physics/collide.h"
#include "combat/combat.h"
#include "combat/combat_cooldown.h" // OpenChaos: player arrest cooldown
#include "engine/input/gamepad_globals.h" // active_input_device
#include "engine/input/weapon_feel.h" // weapon_feel_evaluate_fire
// Engine.h removed: SIN/COS/QDIST2 come transitively via MFStdLib→StdMaths→core/math.h.
#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"
#include "assets/xlat_str.h" // OpenChaos: X_FAILED for arrest-on-cooldown message
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/input_frame.h"
#include "engine/input/mouse_capture.h" // mouse_capture_is_active (aim mouse-look)
#include "game/game_globals.h"
#include "engine/graphics/pipeline/aeng.h" // MSG_add
#include "engine/console/console.h" // CONSOLE_text_at

// Forward declaration for gang-attack reset (defined in pcom.cpp, not yet in pcom.h).
// uc_orig: reset_gang_attack (fallen/Source/pcom.cpp)
extern void reset_gang_attack(Thing* p_target);

extern SLONG set_person_search(Thing* p_person, SLONG ob_index, SLONG ox, SLONG oy, SLONG oz);
extern SLONG set_person_search_corpse(Thing* p_person, Thing* p_personb);
extern SLONG find_searchable_person(Thing* p_person);

// Analog stick X / Y are packed into bits 18-31 of the input ULONG (range
// -128..+126, even). They are read back via input_virtual_axis(input, VAXIS_*)
// — see input_actions.h. (Replaces the old GET_JOYX / GET_JOYY macros, which
// had no source-name argument; behaviour is byte-identical.)

// Minimum analog stick velocity below which movement is ignored (game logic deadzone).
// PC: 8/128. The hardware-level deadzone is NOISE_TOLERANCE in get_hardware_input().
// uc_orig: ANALOGUE_MIN_VELOCITY (fallen/Source/interfac.cpp)
#define ANALOGUE_MIN_VELOCITY 8

// OpenChaos: stick "pure axis" cone — the dominant axis must be at least
// STICK_AXIS_DOMINANCE_RATIO times bigger than the minor axis for the stick
// to count as "pure" (horizontal or vertical). 2 = ~27° cone around the
// dominant axis. Used in several places to discriminate intentional axis-
// dominant input from diagonal mix-ins:
//   - L2 forward↔back zone tracker (PURE_FORWARD / PURE_BACK / SIDE_ISH)
//   - hug-wall stick handling (pure-side → step along, else → break out)
//   - dangling stick handling (pure-vertical → pull up / drop, else → side
//     traverse)
//
// The ratio is paired with NOISE_TOLERANCE (the in-game stick deadzone, read
// from config in get_hardware_input; default 8192 raw) for the "stick past
// deadzone" test.
#define STICK_AXIS_DOMINANCE_RATIO 2

// OpenChaos: rotate a camera-relative stick vector (raw_dx, raw_dy) into the
// character-relative frame.
//
// Camera and character angles are in PSX game-units (2048 = 360°). Output
// convention matches the input convention used everywhere else in this file:
//   - dy_out < 0  → stick points in character's forward direction
//   - dy_out > 0  → stick points in character's backward direction
//   - dx_out > 0  → stick points to character's right
//   - dx_out < 0  → stick points to character's left
// Magnitude is preserved (modulo integer truncation).
//
// Source-agnostic by design: ANY input source that produces (dx, dy) in
// camera frame feeds this helper. A future keyboard WASD path is by
// construction already in character frame and should NOT go through this
// rotation — feed (dx, dy) downstream directly (or pass cam_angle ==
// char_angle to get the identity transform).
//
// Derivation: the screen-frame stick angle relates to its world direction
// by `world = screen + cam_angle`; the char-frame angle is `char_rel =
// world - char_angle = screen + (cam - char)`. So we rotate the (dx, dy)
// vector by `delta = cam_angle - char_angle`. Sign of the rotation matches
// the Arctan(-dx, dy) convention used by player_turn_left_right_analogue
// and the snap-angle formulae in apply_button_input — verified by checking
// that delta == 1024 (character facing 180° from camera) maps stick "up"
// to stick "down" in the char frame, which is what tank-relative controls
// require.
//
// 64-bit intermediates: raw_dx / raw_dy can reach ±32768 and SIN/COS are
// 16-bit signed fixed-point (65536 = 1.0). Each product fits in 32 bits
// only just (2^15 × 2^16 = 2^31), and the sum of two such terms can
// overflow a signed 32-bit accumulator.
static inline void stick_camera_to_char_frame(
    SLONG raw_dx, SLONG raw_dy, SLONG cam_angle, SLONG char_angle,
    SLONG* out_dx, SLONG* out_dy)
{
    const SLONG delta = (cam_angle - char_angle) & 2047;
    const SLONG s = SIN(delta);
    const SLONG c = COS(delta);
    *out_dx = (SLONG)(((int64_t)raw_dx * c + (int64_t)raw_dy * s) >> 16);
    *out_dy = (SLONG)((-(int64_t)raw_dx * s + (int64_t)raw_dy * c) >> 16);
}

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

// OpenChaos: "player just transitioned into MOVEING from non-MOVEING" counter
// (ticks). Armed via player_arm_run_entry_snap() from set_person_running when
// prior State was not MOVEING; decremented once at the top of apply_button_input.
//
// Used by ACTION_RUNNING_JUMP to apply the same stick+camera angle snap that
// ACTION_STANDING_JUMP already performs. Race-prone scenarios:
//  - Stand + (W slightly before Space): on the W tick action tree fires no
//    action; player_interface_move runs the rotation pass with State=IDLE
//    max_angle=1024 and snaps Angle — but if the camera is STILL rotating
//    when W lands, the snap goes to an intermediate camera angle. Then
//    process_analogue_movement transitions to MOVEING. On the next tick(s)
//    rotation max_angle drops to 192/(velocity/8) ≈ 38/tick and the character
//    lags far behind the (still-moving) camera. Space arrives → action_run →
//    ACTION_RUNNING_JUMP. Without snap, set_person_running_jump bakes the
//    half-rotated facing.
//  - Landing on a ledge with W held: SUB_STATE_DROP_DOWN_LAND end →
//    set_person_idle → continue_moveing fallback → set_person_running. Same
//    half-rotated Angle problem on next-tick ACTION_RUNNING_JUMP.
//
// Counter value bounds the snap window: small enough that "running steady +
// camera flip + jump" (the user-requested behaviour where the character does
// NOT spin to face the camera mid-flight) is preserved — that scenario starts
// with counter == 0 because the character has been MOVEING for many ticks.
// Large enough to cover the typical 1-3 tick window between transition into
// MOVEING and the JUMP press.
//
// 5 ticks @ 50 Hz design rate ≈ 100 ms — within "almost simultaneously" but
// outside any deliberate run-then-jump-later input pattern.
#define PLAYER_RUN_ENTRY_SNAP_TICKS 5
static SLONG g_player_run_entry_ticks = 0;

// Called from set_person_running (person.cpp) whenever the player transitions
// from a non-MOVEING state into MOVEING — regardless of the path that led
// there (process_analogue_movement IDLE→MOVEING, set_person_idle landing-then-
// continue-moveing fallback, dangling drop, etc.). Centralising the arm here
// keeps the magic constant and global private to this translation unit.
void player_arm_run_entry_snap()
{
    g_player_run_entry_ticks = PLAYER_RUN_ENTRY_SNAP_TICKS;
}

// Action tables: each array lists valid transitions from a specific player action/state.
// Indexed via action_tree[] below.
// uc_orig: action_idle (fallen/Source/interfac.cpp)
// OpenChaos: removed priority-entries that routed JUMP+LEFT/RIGHT directly to
// ACTION_FLIP_LEFT/RIGHT (original entries 0-1). All JUMP presses now route
// through ACTION_STANDING_JUMP, whose body picks roll-vs-jump per analogue flag
// (see body for details). D-pad / keyboard path still produces rolls; stick
// path no longer does (stick rolls will return as an L2 chord later).
struct ActionInfo action_idle[] = {
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
// OpenChaos: removed mid-jump switch to FLIP_LEFT/RIGHT (original entries 0-1).
// A short side-tap during a jump used to instantly switch the action to a roll.
// Body now produces only forward / vertical jumps for stick; for D-pad /
// keyboard the body still triggers rolls on side+jump at press time. The
// mid-jump-tap switch path is gone for all inputs (minor original-behavior
// loss; cleaner UX), all roll triggering happens at the entry of
// ACTION_STANDING_JUMP. Will be replaced by L2-chord scheme later.
struct ActionInfo action_standing_jump[] = {
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

// uc_orig: action_dead (fallen/Source/interfac.cpp)
struct ActionInfo action_dead[] = {
    { ACTION_BLOCK_FLAG, 0, INPUT_MASK_ACTION },
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
    0, // Gun Away
    0, // Respawn
    action_dead, // Dead
    0, // 27
    0, // flip right
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
    0, // Enter vehicle
    0, // Inside vehicle
    action_sit, // Sit bench
    action_hug_wall, // hug wall
    0, // hug left
    0, // hug right
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

// If the player is holding a grenade or explosives, activates/primes it.
// Returns 1 if any in-hand item action was performed.
// uc_orig: player_activate_in_hand (fallen/Source/interfac.cpp)
SLONG player_activate_in_hand(Thing* p_person)
{
    // While the throw/release animation is already playing, swallow the attack
    // button entirely: don't restart the animation (mashing used to reset it to
    // frame 0, so the actual throw — fired at frame 3 — kept getting pushed
    // back) and don't prime the NEXT grenade in the stack. The button does
    // nothing until the current throw animation finishes.
    if (p_person->SubState == SUB_STATE_CANNING_RELEASE) {
        return 1; // consumed — ignore the press
    }

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

                    // Phase-1 feedback: play the pistol-style burst so the
                    // prime press has a tactile "click". On DualSense the
                    // continuous Weapon25 effect (enabled via weapon_ready
                    // for an un-primed grenade) supplies the trigger click;
                    // this adds the same rumble/haptic the pistol gives on
                    // a shot. On Xbox (no adaptive trigger) this short
                    // rumble IS the click simulation. Player only.
                    if (p_person->Genus.Person->PlayerID) {
                        weapon_feel_on_shot_fired(SPECIAL_GRENADE, 0);
                    }
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
    // OpenChaos: shooting during walk_back is fully blocked. Several
    // attempts to "exit walk_back then shoot cleanly" produced
    // anim glitches (frozen walk-back pose with fire effect, no shoot
    // anim playing for pistol, single shot then stuck for AK / shotgun).
    // Walk_back + shoot anim/state combinations are messy across weapons —
    // simpler to disallow firing entirely while backing up. Player
    // releases the back stick to shoot. Applies to all call sites
    // (apply_button_input ACTION_SHOOT body, PUNCH-while-gun-out path,
    // aim-mode first-person shoot path).
    if (p_person->SubState == SUB_STATE_WALKING_BACKWARDS)
        return;

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
    if (p_vehicle->Genus.Vehicle->Driver
        || (p_vehicle->Genus.Vehicle->Flags & FLAG_VEH_ANIMATING)) {
        //
        // This vehicle is already occupied, or someone is mid enter/exit
        // animation (Driver is only set once the enter animation finishes, so
        // FLAG_VEH_ANIMATING covers the in-between window).
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

        // Max speed (signed Velocity units) at which the driver may bail out.
        // The original effectively required a dead stop; OpenChaos lets you step
        // out while still moving slowly. Tunable — raise for an easier exit.
        constexpr SLONG CAR_EXIT_MAX_SPEED = 250;

        if (abs(p_vehicle->Velocity) >= CAR_EXIT_MAX_SPEED) {
            return 0; // moving too fast to step out
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

                // OpenChaos: player arrest cooldown. PlayerID 0 is never
                // gated, so the cop AI arrest path (pcom.cpp) is
                // unaffected.
                if (p_thing->Genus.Person->PersonType == PERSON_DARCI && (index = find_arrestee(p_thing))) {
                    if (combat_cooldown_ready(p_thing->Genus.Person->PlayerID, COOLDOWN_ARREST)) {
                        combat_cooldown_note(p_thing->Genus.Person->PlayerID, COOLDOWN_ARREST, "ARREST", true);
                        combat_cooldown_arm(p_thing->Genus.Person->PlayerID, COOLDOWN_ARREST);
                        set_person_arrest(p_thing, index);
                    } else {
                        // Still on cooldown: refuse, but tell the player
                        // via the same HUD info message used for "the
                        // car is locked". Reuses the existing X_FAILED
                        // string -- no new localised text needed.
                        combat_cooldown_note(p_thing->Genus.Person->PlayerID, COOLDOWN_ARREST, "ARREST", false);
                        PANEL_new_info_message(XLAT_str(X_FAILED));
                    }

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

            // OpenChaos: player arrest cooldown (see note above).
            if (p_thing->Genus.Person->PersonType == PERSON_DARCI && (index = find_arrestee(p_thing))) {
                if (combat_cooldown_ready(p_thing->Genus.Person->PlayerID, COOLDOWN_ARREST)) {
                    combat_cooldown_note(p_thing->Genus.Person->PlayerID, COOLDOWN_ARREST, "ARREST", true);
                    combat_cooldown_arm(p_thing->Genus.Person->PlayerID, COOLDOWN_ARREST);
                    set_person_arrest(p_thing, index);
                } else {
                    combat_cooldown_note(p_thing->Genus.Person->PlayerID, COOLDOWN_ARREST, "ARREST", false);
                    PANEL_new_info_message(XLAT_str(X_FAILED));
                }

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

        if (p_thing->SubState != SUB_STATE_IDLE_CROUTCH && p_thing->SubState != SUB_STATE_IDLE_CROUTCHING)
            if (angle = can_i_hug_wall(p_thing)) {
                p_thing->Draw.Tweened->Angle = angle - 1;
                set_person_turn_to_hug_wall(p_thing);
                return (INPUT_MASK_ACTION);
            }

        //
        // Near a coke can and idle. Only pick up with truly EMPTY hands: the
        // PersonID weapon-mesh bits cover guns / melee, but a grenade or C4 in
        // hand sets no mesh bits (see set_persons_personid) — so also require no
        // special in use, otherwise you could grab a can while holding a grenade.
        // Also skip if a can is ALREADY held: otherwise the bend-down pickup
        // animation plays pointlessly (the second can is never actually grabbed —
        // fn_person_can guards that — so the anim would just be wasted).
        //
        if ((p_thing->Draw.Tweened->PersonID >> 5) == 0
            && p_thing->Genus.Person->SpecialUse == 0
            && !(p_thing->Genus.Person->Flags & FLAG_PERSON_CANNING)) {
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

    // OpenChaos: the old sprint (moving → PERSON_MODE_SPRINT) and stealth-crouch
    // (idle/gun → set_person_croutch) fallbacks used to live here, so the single
    // ACTION button did all three. They were split onto their own buttons and now
    // live in the central input dispatch (see INPUT_MASK_SPRINT / INPUT_MASK_-
    // STEALTH). do_an_action is now purely the interaction multitool: if none of
    // the interactions above matched, it does nothing.
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

// OpenChaos: returns true if the movement input points in the character's
// own forward direction. Used by sit-stand transitions so the stand-up
// trigger respects character orientation rather than camera/screen
// orientation — e.g. if the character is facing the camera after backing
// into a bench, stick UP on the controller is char-back (don't stand up),
// and stick DOWN is char-forward (stand up).
//
// Analog stick: rotate raw stick (dx, dy) into the character frame and
// check the Y component. Digital input (stick in deadzone, e.g. keyboard
// W) falls back to INPUT_MASK_FORWARDS — keyboard WASD will get its own
// char-rel translation in the keyboard scheme work.
static bool input_is_char_rel_forward(Thing* p_person, ULONG input)
{
    const SLONG raw_dx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
    const SLONG raw_dy = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);
    if ((raw_dx != 0 || raw_dy != 0) && p_person->Draw.Tweened) {
        SLONG dx_p, dy_p;
        stick_camera_to_char_frame(raw_dx, raw_dy,
            get_camera_angle(),
            p_person->Draw.Tweened->Angle,
            &dx_p, &dy_p);
        constexpr SLONG kPackedThreshold = 16;
        return dy_p < -kPackedThreshold;
    }
    return (input & INPUT_MASK_FORWARDS) != 0;
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
    // Default UC_FALSE: radar rotation follows the camera, not Darci's facing.
    PANEL_scanner_poo = ENV_get_value_number("scanner_follows", UC_FALSE, "Game");
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

    dx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
    dz = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);

    angle = Arctan(-dx, dz);

    if (flags & JOY_REL_CAMERA) {
        SLONG ca;

        ca = get_camera_angle();

        angle += ca;
        angle = (angle + 2048) & 2047;
    }
    return (angle);
}

// Back-walk regime — written in get_hardware_input (the STICKY_BACK state of the
// back-walk machine, which lives on the ZOOM/BACK-WALK modifier — gamepad L1 /
// keyboard E), read by player_turn_left_right_analogue to force walk_back
// entry/exit. Only NONE / ACTIVE are used.
enum {
    BACKWALK_REGIME_NONE = 0,
    BACKWALK_REGIME_ACTIVE,
};
static int g_backwalk_regime = BACKWALK_REGIME_NONE;

// L2/Ctrl tap-window counter. Set to L2_ROLL_WINDOW_TICKS when the modifier
// transitions from not-held → held. Decremented each tick in get_hardware_input.
// It marks the "quick tap" interval: if the modifier is RELEASED while it is
// still > 0 with the stick deflected, a ROLL fires (the machine posts
// g_l2_roll_request, which the roll dispatch fires as a flip — no ✕, no jump).
// If the modifier is instead HELD past the window, no roll fires and the hold
// SLOW-WALK runs as usual. Reset to 0 on release.
//
// L2_ROLL_WINDOW_TICKS = 10 ticks at 20 Hz ≈ 500 ms — the longest a press can
// last and still count as a tap; a longer press is treated as a hold.
static constexpr SLONG L2_ROLL_WINDOW_TICKS = 10; // ~500 ms at 20 Hz
static SLONG g_l2_engage_window_ticks = 0;

// L2 ROLL request. Set by the L2 machine (get_hardware_input) on the L2 release
// edge when it was a quick tap with the stick deflected; consumed by the roll
// dispatch (apply_button_input / _fight) the same tick.
//   g_l2_roll_request    : 0 = none, 1 = roll requested.
//   g_l2_roll_target_angle: WORLD direction the roll should travel toward
//                           (0..2047) — the stick direction at release, camera-
//                           considered, independent of how the character rotated.
//   g_l2_roll_flip_dir    : which flip animation (0 = left, 1 = right). The flip
//                           moves perpendicular to facing (facing+512 for left,
//                           facing-512 for right; see SUB_STATE_FLIPING), so the
//                           dispatch snaps facing to target∓512 to send the
//                           motion toward target. The two candidates are 180°
//                           apart; the side is chosen to keep the character's
//                           toward/away-from-camera orientation, falling back to
//                           the last movement near the camera axis (see
//                           compute_l2_roll_dir).
// Deliberately NOT routed through INPUT_MASK_JUMP: the request only ever fires a
// flip in a jump-capable state and is dropped otherwise, so L2 can never produce
// an actual jump. Reset to 0 every tick at the top of the L2 machine.
static SLONG g_l2_roll_request = 0;
static SLONG g_l2_roll_target_angle = 0;
static SLONG g_l2_roll_flip_dir = 0;

// Last steering side: -1 = left, +1 = right, 0 = none. Two sources feed the same
// sign so keyboard and mouse stay consistent (read by compute_l2_roll_dir to
// break the near-camera-axis tie):
//   1. Lateral stick component (A/D, W+D, gamepad tilt) — direct intent, holds
//      through release. It also SUPPRESSES source 2 for a short while, so the
//      "return to straight" turn after releasing a key (the character rotating
//      back to forward) doesn't flip the sign the wrong way.
//   2. The character's actual TURN (world-frame facing rotation) — captures
//      camera/mouse steering while running pure-forward (the character turns to
//      follow the camera even though the stick stays pure-forward). Used only
//      when there's been no recent lateral input.
static SLONG g_l2_last_side_sign = 0;
static constexpr SLONG L2_LEAN_MIN_RAW = 8192; // clear lateral lean (raw camera-frame X, ±32768)
// Net accumulated facing change (2048 = 360°) that registers a turn — accumulating
// per-tick deltas catches slow, gradual mouse steering that never spikes in one tick.
static constexpr SLONG L2_TURN_ACCUM_THRESH = 32; // ≈ 5.6° net turn
static constexpr SLONG L2_LATERAL_HOLD_TICKS = 15; // suppress the turn source this long after a lateral input

// Back-walk-modifier held flag (gamepad L1 / keyboard E). Set by the zoom/
// back-walk machine (process_zoomwalk) each tick. Read by other modules that
// need to know the player is in the back-walk modifier — the camera (fc.cpp:
// suppress auto get-behind so it doesn't swing behind while retreating) and
// collision (collide.cpp: lenient bump-into-bench sit while walking backward).
// Accessed externally via input_backwalk_held().
static bool g_backwalk_held = false;

// OpenChaos: see input_actions.h for purpose.
bool input_backwalk_held(void)
{
    return g_backwalk_held;
}

// Zoom / back-walk modifier mode — written once per tick by process_zoomwalk
// (called from get_hardware_input for whichever input device is active), read by
// apply_button_input_first_person to decide whether to run the zoom/look pose.
// The modifier (bound via ACT_FOOT_AIM_* — gamepad L1 / keyboard E) is a HOLD
// with two mutually exclusive modes for the duration of one hold:
//   ZOOM — entered only from a standstill with the stick/WASD untouched: the
//          aim/look pose (camera pulls close, character pivots in place via mouse
//          / right stick). The instant the stick/WASD is touched it latches to
//          WALK for the rest of the hold; it never returns to zoom until the
//          modifier is released and re-pressed.
//   WALK — entered when the modifier is pressed while already moving, or when the
//          stick is touched during zoom. Runs the BACK-WALK machine: a deflection
//          into the rear semicircle walks the character backward (camera-relative,
//          tank-turn); any other deflection is normal movement; re-centring
//          re-arms. No zoom, no in-place crank.
enum {
    ZOOMWALK_OFF = 0,
    ZOOMWALK_ZOOM,
    ZOOMWALK_WALK,
};
static int g_zoomwalk_mode = ZOOMWALK_OFF;

// Shared zoom / back-walk machine: ZOOM (standstill look pose) ↔ WALK (back-walk
// machine). Called once per tick from get_hardware_input by whichever input
// device is active (gamepad L1 / keyboard E). Updates g_zoomwalk_mode, sets
// g_backwalk_held, and drives g_backwalk_regime + m_bForceWalk while walking
// backward. Camera-frame stick offset: dx_c = right+, dy_c = back+;
// stick_in_deadzone = stick centred. p_pc = the player character.
static void process_zoomwalk(bool engaged, SLONG dx_c, SLONG dy_c,
    bool stick_in_deadzone, Thing* p_pc)
{
    static bool prev_engaged = false;
    const bool was_engaged = prev_engaged;
    prev_engaged = engaged;

    // BACK-WALK sticky sub-state (used only in WALK):
    //  - ARMED (stick centred) → await a deflection.
    //  - ARMED + deflection into the rear semicircle (char-frame dy_p > 0)
    //    → STICKY_BACK (camera-relative back-walk).
    //  - ARMED + any other deflection → STICKY_PASS (normal movement).
    //  - either sticky mode re-arms when the stick re-centres.
    // Reset to ARMED on the engage edge so a fresh hold always starts disarmed
    // (a previous hold that ended in STICKY_BACK must not carry over).
    enum { BW_ARMED,
        BW_STICKY_BACK,
        BW_STICKY_PASS };
    static int bw_state = BW_ARMED;
    if (!was_engaged)
        bw_state = BW_ARMED;

    g_backwalk_held = engaged;

    if (!engaged) {
        g_zoomwalk_mode = ZOOMWALK_OFF;
        g_backwalk_regime = BACKWALK_REGIME_NONE;
        return;
    }

    const bool stick_active = !stick_in_deadzone;

    if (g_zoomwalk_mode == ZOOMWALK_OFF) {
        // First held tick: decide the mode for this whole hold. Touching the
        // stick or already being on the move means the player wants to walk,
        // not to zoom; a clean standstill with a neutral stick enters zoom.
        const bool moving = p_pc && (p_pc->State == STATE_MOVEING);
        g_zoomwalk_mode = (stick_active || moving) ? ZOOMWALK_WALK : ZOOMWALK_ZOOM;
    } else if (g_zoomwalk_mode == ZOOMWALK_ZOOM) {
        // The moment the stick is touched in zoom, latch to walk for the rest of
        // the hold (zoom never auto-resumes — needs a fresh press).
        if (stick_active)
            g_zoomwalk_mode = ZOOMWALK_WALK;
    }
    // WALK stays latched until the modifier is released (handled above). Outside
    // WALK (zoom or just-pressed-standstill) there's no back-walk: re-arm and
    // clear the regime.
    if (g_zoomwalk_mode != ZOOMWALK_WALK) {
        bw_state = BW_ARMED;
        g_backwalk_regime = BACKWALK_REGIME_NONE;
        return;
    }

    // Char-frame stick (camera-considered) for the rear-zone test.
    SLONG dx_p = dx_c, dy_p = dy_c;
    if (p_pc && p_pc->Draw.Tweened)
        stick_camera_to_char_frame(dx_c, dy_c, get_camera_angle(),
            p_pc->Draw.Tweened->Angle, &dx_p, &dy_p);
    const bool stick_in_back_cone = (dy_p > 0);

    if (stick_in_deadzone)
        bw_state = BW_ARMED;
    else if (bw_state == BW_ARMED)
        bw_state = stick_in_back_cone ? BW_STICKY_BACK : BW_STICKY_PASS;

    if (bw_state == BW_STICKY_BACK) {
        g_backwalk_regime = BACKWALK_REGIME_ACTIVE;
        m_bForceWalk = UC_TRUE;
    } else {
        g_backwalk_regime = BACKWALK_REGIME_NONE;
    }
}

// Absolute angular distance between two game angles (0..2047, 2048 = 360°).
// Result in 0..1024.
static SLONG l2_angle_abs_diff(SLONG a, SLONG b)
{
    SLONG d = (a - b) & 2047;
    if (d > 1024)
        d = 2048 - d;
    return d;
}

// Compute a roll toward the camera-frame stick vector (sx = right+, sy = back+,
// any scale). Fills *out_target with the WORLD direction the roll should travel
// (stick angle + camera yaw) and *out_flip with the flip animation side
// (0 = left, 1 = right). The flip moves perpendicular to facing, so the caller
// snaps facing to out_target∓512; the two candidate facings are 180° apart.
//
// Flip-side choice: normally pick it to KEEP p_person's toward/away-from-camera
// orientation across the roll. BUT when the roll travels nearly straight
// toward/away from the camera (target within ROLL_FLIP_CAM_AXIS_BUFFER of the
// camera view axis), both candidate facings sit symmetrically on the screen-left
// / screen-right sides — toward/away can't disambiguate, and a fixed pick looks
// wrong. In that buffer zone fall back to the last left/right movement
// (g_l2_last_side_sign): left input → left flip, right input → right flip.
//
// Returns false if the stick is centred (no direction).
static bool compute_l2_roll_dir(Thing* p_person, SLONG sx, SLONG sy,
    SLONG* out_target, SLONG* out_flip)
{
    if (sx == 0 && sy == 0)
        return false;
    const SLONG cam = get_camera_angle() & 2047;
    const SLONG target = (Arctan(-sx, sy) + cam + 2048) & 2047;
    SLONG char_face = cam;
    if (p_person && p_person->Draw.Tweened)
        char_face = p_person->Draw.Tweened->Angle & 2047;
    const bool faces_away = l2_angle_abs_diff(char_face, cam) < 512;
    const SLONG face_left = (target - 512 + 2048) & 2047; // LEFT flip facing
    const SLONG face_right = (target + 512 + 2048) & 2047; // RIGHT flip facing
    const bool left_is_away = l2_angle_abs_diff(face_left, cam) < l2_angle_abs_diff(face_right, cam);
    *out_target = target;
    // XNOR: keep away when facing away, keep toward when facing toward.
    SLONG flip = (faces_away == left_is_away) ? 0 : 1;

    // Buffer zone around the camera view axis (roll ~straight toward/away from
    // the camera): the away/toward test is ambiguous there, so disambiguate by
    // the last left/right movement instead. ~20° (2048 = 360°).
    constexpr SLONG ROLL_FLIP_CAM_AXIS_BUFFER = 114; // ≈ 20°
    const SLONG roll_vs_cam = l2_angle_abs_diff(target, cam); // 0 = away, 1024 = toward
    const bool near_cam_axis = (roll_vs_cam < ROLL_FLIP_CAM_AXIS_BUFFER)
        || (roll_vs_cam > 1024 - ROLL_FLIP_CAM_AXIS_BUFFER);
    if (near_cam_axis && g_l2_last_side_sign != 0)
        flip = (g_l2_last_side_sign < 0) ? 1 : 0; // inverted inside the buffer (by feel)

    *out_flip = flip;
    return true;
}

// Shared L2 / Ctrl tactical-mode machine: TAP-RELEASE ROLL + HOLD BACK-WALK.
// Driven from a single trigger-engaged flag plus the camera-frame stick offset
// (dx_c = right+, dy_c = back+; stick_in_deadzone = stick centred). Called once
// per physics tick by whichever input device is active — gamepad (L2 + analog
// stick) or keyboard (Ctrl + WASD-emulated stick) — so both share identical
// behaviour and state. p_pc = the player character (NET_PERSON(0)).
//
// Outputs (file-scope): g_l2_roll_request/_target_angle/_flip_dir (the roll,
// fired by the dispatch in apply_button_input / _fight), m_bForceWalk (the slow
// walk, applied in the normal movement path) and the tap window. Back-walk is
// NOT here — it lives on the separate zoom/back-walk modifier (process_zoomwalk).
static void process_l2_tactical(bool engaged, SLONG dx_c, SLONG dy_c,
    bool stick_in_deadzone, Thing* p_pc)
{
    static bool prev_engaged = false;
    const bool was_engaged = prev_engaged;
    prev_engaged = engaged;

    // One-shot roll request: cleared every tick; set below only on the exact
    // release tick of a quick tap with the stick deflected.
    g_l2_roll_request = 0;

    // Track the last steering side (g_l2_last_side_sign) from two sources — see
    // that global's comment for the full rationale. Source 1 is the direct
    // lateral stick input; source 2 is the character's actual turn (camera/mouse
    // steering), skipped briefly after lateral input and while flipping so the
    // return-to-straight turn / the roll's own facing snap don't corrupt it.
    // (+angle = char's left, see SUB_STATE_FLIPING in person.cpp.)
    {
        static SLONG prev_face = -1;
        static SLONG turn_accum = 0;
        static SLONG lateral_hold = 0;
        if (stick_in_deadzone) {
            // Not moving → forget everything.
            g_l2_last_side_sign = 0;
            turn_accum = 0;
            lateral_hold = 0;
            prev_face = -1;
        } else {
            // Source 1: lateral stick component (direct, holds through release).
            if (abs(dx_c) > L2_LEAN_MIN_RAW) {
                g_l2_last_side_sign = (dx_c < 0) ? -1 : +1;
                lateral_hold = L2_LATERAL_HOLD_TICKS;
            } else if (lateral_hold > 0) {
                --lateral_hold;
            }

            // Source 2: the character's actual turn (camera/mouse steering),
            // only when no recent lateral input (so the post-release return turn
            // doesn't flip the sign).
            if (lateral_hold == 0 && p_pc && p_pc->Draw.Tweened
                && p_pc->SubState != SUB_STATE_FLIPING) {
                const SLONG face = p_pc->Draw.Tweened->Angle & 2047;
                if (prev_face >= 0) {
                    SLONG d = (face - prev_face) & 2047;
                    if (d > 1024)
                        d -= 2048; // signed turn this tick, [-1024, 1024]
                    turn_accum += d;
                    // Register a side once the NET turn passes the threshold, then
                    // reset (gradual steering accumulates; a reversal cancels).
                    if (turn_accum > L2_TURN_ACCUM_THRESH) {
                        g_l2_last_side_sign = -1; // net turn left
                        turn_accum = 0;
                    } else if (turn_accum < -L2_TURN_ACCUM_THRESH) {
                        g_l2_last_side_sign = +1; // net turn right
                        turn_accum = 0;
                    }
                }
                prev_face = face;
            } else {
                prev_face = -1; // re-baseline (skip during the lateral-hold / flip)
                turn_accum = 0;
            }
        }
    }

    // Tap window. Opens on the engage edge, closes on release, ticks down
    // otherwise. Capture its pre-update value first — the update zeroes it on
    // release, and the tap-vs-hold test below needs to know it was still open at
    // release.
    const SLONG window_before = g_l2_engage_window_ticks;
    if (!was_engaged && engaged) {
        g_l2_engage_window_ticks = L2_ROLL_WINDOW_TICKS;
    } else if (!engaged) {
        g_l2_engage_window_ticks = 0;
    } else if (g_l2_engage_window_ticks > 0) {
        --g_l2_engage_window_ticks;
    }

    // TAP-RELEASE ROLL: on the release edge, if it was a quick tap (window still
    // open) and the stick is deflected, roll toward the stick's world direction.
    if (was_engaged && !engaged && window_before > 0 && !stick_in_deadzone) {
        SLONG t, f;
        if (compute_l2_roll_dir(p_pc, dx_c, dy_c, &t, &f)) {
            g_l2_roll_target_angle = t;
            g_l2_roll_flip_dir = f;
            g_l2_roll_request = 1;
        }
    }

    // HOLD SLOW-WALK: while the modifier is HELD with the stick deflected, force
    // a slow camera-relative walk (m_bForceWalk caps the run to walk speed +
    // plays the step anim; direction comes from the normal movement path). The
    // tap-release roll above is independent — a quick tap rolls instead. Any
    // stick direction slow-walks that way (back-walk is a different modifier).
    if (engaged && !stick_in_deadzone)
        m_bForceWalk = UC_TRUE;
}

// Post-climb jump suppression counter. Set when pull_up animation ends (in
// person.cpp), decremented in apply_button_input. While > 0, INPUT_MASK_JUMP
// is cleared so no jump action fires.
//
// Purpose: without this, JUMP pressed during a climb (commonly spammed
// because player wants to immediately leap off the ledge) would fire as
// ACTION_STANDING_JUMP the instant pull_up animation ends — character is
// still facing the climb direction (analog rotation toward stick hasn't
// caught up yet) and standing-jump direction logic misses for stick
// deflections below the digital threshold, so the jump goes forward (climb
// direction) instead of where the player is holding the stick. By delaying
// JUMP a few ticks the character has time to (a) enter STATE_MOVEING via
// process_analogue_movement and (b) rotate toward the stick direction. When
// JUMP unblocks, ACTION_RUNNING_JUMP fires from MOVEING state and jumps in
// the (now-aligned) facing direction. Held JUMP press just feels slightly
// delayed; player doesn't perceive a lost input.
//
// Exposed via `extern SLONG g_post_climb_jump_block_ticks` from person.cpp
// (no shared header — minimal touching).
SLONG g_post_climb_jump_block_ticks = 0;

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

    // L2 BACKWARD regime: explicit walk_back entry / exit. Backing is
    // CAMERA-RELATIVE (player_relative stays 0): the walk_back target is in
    // world space, so the character converges toward a fixed backing direction,
    // and rotating the stick around the rim steers where it backs. We force-
    // enter walk_back here when the regime becomes ACTIVE (the sticky back-
    // walk latch in get_hardware_input), and force-exit on regime change (stick
    // returned to centre / modifier released) so the character doesn't get stuck.
    if (g_backwalk_regime == BACKWALK_REGIME_ACTIVE) {
        // Allow force-entry from any normal locomotion state. STATE_IDLE
        // alone misses the deceleration window (State still MOVEING when
        // player engages L2+back fast after a run). STATE_GUN must also be
        // included — character with a firearm out (pistol / AK / shotgun)
        // sits in STATE_GUN at rest; without this entry, BACKWARD regime
        // forces m_bForceWalk + player_relative=0 but walk_back is never
        // entered, so process_analogue_movement falls through to
        // set_person_running and the character spins to face the camera
        // (back-stick in camera-relative analog = "go toward camera") before
        // actually moving. Matches the keyboard-back handler in
        // apply_button_input which also includes STATE_GUN as a walk_back
        // entry point. STATE_HIT_RECOIL covered for symmetry with that
        // handler's set_person_running default.
        if ((p_thing->State == STATE_IDLE
                || p_thing->State == STATE_MOVEING
                || p_thing->State == STATE_GUN
                || p_thing->State == STATE_HIT_RECOIL)
            && p_thing->SubState != SUB_STATE_WALKING_BACKWARDS
            && p_thing->Genus.Person->Action != ACTION_SIT_BENCH) {
            // Don't yank the character out of the sit-down animation:
            // set_person_sit_down sets Action=ACTION_SIT_BENCH and
            // SubState=SIMPLE_ANIM, but L2 is still held in regime
            // BACKWARD, and SIMPLE_ANIM ≠ WALKING_BACKWARDS, so without
            // this gate the next tick would call set_person_walk_backwards
            // again, cancelling the sit. The result was a rapid sit→
            // walk-back→sit loop visible to the player as "stuck next
            // to the bench, never sitting."
            set_person_walk_backwards(p_thing);
            return (0);
        }
    } else if (p_thing->SubState == SUB_STATE_WALKING_BACKWARDS) {
        // Regime no longer BACKWARD → exit walk_back. Safe because under
        // the current scheme the only entry into walk_back for the player
        // is via this regime (tank-mode auto-entry is gated on
        // player_relative=1 which is no longer set anywhere).
        set_person_running(p_thing);
        return (0);
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

        dx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
        dz = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);

        velocity = QDIST2(abs(dx), abs(dz));

        if (velocity > ANALOGUE_MIN_VELOCITY)
            angle = Arctan(-dx, dz);

        if (p_thing->Velocity > 10 && p_thing->SubState == SUB_STATE_RUNNING) {
            p_thing->Draw.Tweened->Roll = ((((p_thing->Velocity - 9) * dx)) >> 5);
            p_thing->Draw.Tweened->DRoll = 0;
        }

        if (angle >= 0) {
            SLONG ca;
            // Turn-rate caps in angle-units/tick at 20 Hz design rate.
            // Values scaled ×1.5 vs original (128/16/64) so that 20 Hz × cap
            // = 3840/480/1920 units/sec — presumably matching the original feel
            // (original source has 128/16/64; at what Hz it was tuned is unknown).
            static const SLONG TURN_CAP_RUN = 192; // 192×20 = 3840 = 128×30
            static const SLONG TURN_CAP_JUMP = 24; //  24×20 =  480 =  16×30
            static const SLONG TURN_CAP_CRAWL = 96; //  96×20 = 1920 =  64×30

            SLONG max_angle = TURN_CAP_RUN;
            SLONG dangle;

            ca = get_camera_angle();

            if (player_relative) {
                ca = p_thing->Draw.Tweened->Angle;
            }

            angle += ca;
            // OpenChaos: +1024 offset for backflip too. Backflip enters with
            // a snap to (stick + cam + 1024) (see ACTION_STANDING_JUMP body,
            // BACKWARD branch); during the backflip flight player_turn_left_
            // right_analogue still runs and rotates toward target. Without
            // the +1024 here, target = stick + cam → 180° away from snap →
            // capped rotation (TURN_CAP_JUMP=24/tick × ~10 ticks of backflip)
            // curves the trajectory off the intended direction.
            if (p_thing->SubState == SUB_STATE_WALKING_BACKWARDS
                || p_thing->SubState == SUB_STATE_STANDING_JUMP_BACKWARDS) {
                angle += 1024;
            }
            angle = (angle + 2048) & 2047;

            if (p_thing->State == STATE_JUMPING || p_thing->SubState == SUB_STATE_RUNNING_SKID_STOP)
                max_angle = TURN_CAP_JUMP;
            if (p_thing->SubState == SUB_STATE_CRAWLING)
                max_angle = TURN_CAP_CRAWL;

            if (player_relative) {
                max_angle >>= 1;
            }

            if (!player_relative) {
                if (p_thing->State == STATE_IDLE || (p_thing->SubState == SUB_STATE_AIM_GUN))
                    max_angle = 1024;
            }

            // Scale turn-rate cap from ticks to seconds so angular velocity
            // stays constant if physics Hz is changed from the 20 Hz design rate.
            max_angle = (max_angle * TICK_RATIO) >> TICK_SHIFT;

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
                        // Back-walk regime: keep character in walk_back even when
                        // stick angle pulls dangle below the exit threshold.
                        // Without this gate, rotating the stick from straight back
                        // toward forward (e.g. full circle) drops |dangle| below
                        // 600 and the engine switches to forward running mid-back-
                        // walk — user perceives "stick forward → suddenly walking
                        // forward" mid-backing.
                        if (g_backwalk_regime != BACKWALK_REGIME_ACTIVE && abs(dangle) > 600) {
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

    dx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
    dz = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);

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
        case STATE_HUG_WALL:
            // OpenChaos: side-dominant analog stick stays in the hug and
            // lets ACTION_HUG_LEFT/RIGHT (from action_hug_wall, fed by
            // INPUT_MASK_LEFT/RIGHT) step along the wall. Forward/back-
            // dominant stick breaks out of the hug into normal movement —
            // matches the D-pad / keyboard behaviour where forward/back
            // arrows leave the hug and only side arrows step along it.
            // (Original behaviour broke out on ANY stick input, including
            // pure side, which made the hug-step impossible on the stick.)
            //
            // We call set_person_running directly rather than fall through
            // to the default path because the case-fallthrough chain below
            // hits STATE_CARRY which has its own `if (SubState != STAND_CARRY)
            // return;` gate — hug-wall's SubState is never STAND_CARRY so
            // that path swallows the input.
            //
            // HUG_WALL_TURN sub-state is the turn-into-wall animation; never
            // interrupt it.
            if (p_thing->SubState == SUB_STATE_HUG_WALL_TURN)
                return;
            // Pure-side cone (|dx| > ratio*|dz|) → stay in hug. Outside the
            // cone (diagonals, pure forward, pure back) → break out into
            // running.
            if (abs(dx) > STICK_AXIS_DOMINANCE_RATIO * abs(dz))
                return;
            set_person_running(p_thing); // forward/back/diagonal-dominant → break out
            return;
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
                set_person_running(p_thing); // arms run-entry snap via player_arm_run_entry_snap()
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
                        // OpenChaos: was scaled by stick magnitude
                        // (`(velocity * 60) >> 8`), which gave slow running
                        // for partial stick deflection. With slow-walk by
                        // magnitude removed and slow walk now exclusively
                        // L2-gated, partial stick should give full-speed
                        // running. Constant matches max of the old formula
                        // (full diagonal: QDIST2(128,128) * 60 / 256 ≈ 42).
                        // L2 path still scales below via the walking branch.
                        p_thing->Velocity = 42;
                    } else {
                        p_thing->Velocity = (velocity * 51) >> 8;
                    }

                    // OpenChaos: removed Velocity < 20 from the running →
                    // walking transition. Slow walking is now only triggered
                    // by the L2 modifier (m_bForceWalk), not by weak stick
                    // deflection.
                    if (m_bForceWalk && p_thing->Genus.Person->AnimType != ANIM_TYPE_ROPER) {
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
                // OpenChaos: removed Velocity > 24 from the walking → running
                // transition. Walking is now exclusively an L2-modifier mode,
                // so as soon as L2 is released we want to leave walking
                // regardless of stick magnitude — otherwise releasing L2 with
                // a weakly-deflected stick would leave the character stuck
                // walking until the stick passed the run threshold.
                if (p_thing->Genus.Person->AnimType == ANIM_TYPE_ROPER || !m_bForceWalk) {
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

    SWORD wJoyX = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
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
    // Entering fight stance drops a held can — can't square up with a can in hand.
    person_drop_held_can(p_person);

    p_person->Genus.Person->Mode = PERSON_MODE_FIGHT;
}

// uc_orig: apply_button_input (fallen/Source/interfac.cpp)
// Main normal-mode input handler: dispatches actions, movement, and fight/jump/skid from input mask.
// Returns the bitmask of inputs consumed/processed this frame.
ULONG apply_button_input(Thing* p_player, Thing* p_person, ULONG input)
{
    // Decrement the post-IDLE→MOVEING snap window once per physics tick.
    // See PLAYER_RUN_ENTRY_SNAP_TICKS comment for full rationale. Decrement
    // happens BEFORE the action tree dispatch so the counter reflects the
    // window state at jump-decision time.
    if (g_player_run_entry_ticks > 0)
        --g_player_run_entry_ticks;

    // Post-climb jump suppression — see g_post_climb_jump_block_ticks comment.
    // Suppress JUMP for a few ticks after climb-up so the buffered jump press
    // fires from running state (RUNNING_JUMP, facing-aligned) rather than the
    // standing-jump path immediately on climb-end (still facing climb direction).
    if (g_post_climb_jump_block_ticks > 0) {
        input &= ~INPUT_MASK_JUMP;
        --g_post_climb_jump_block_ticks;
    }

    // Dangling pre-filter: when the character is hanging on a ledge before a
    // climb-up, action_dangling priority is { PULL_UP on FWD, DROP on BACK,
    // TRAVERSE_LEFT, TRAVERSE_RIGHT }. With raw INPUT_MASK_* from the stick
    // any slight forward tilt sets FORWARDS → PULL_UP wins and the player
    // can't side-traverse when they want to. Filter the input here to a
    // single dominant axis using the L2 pure-axis cone
    // (STICK_AXIS_DOMINANCE_RATIO):
    //
    //   stick pure-vertical (|dy| > ratio*|dx|) → keep FWD/BACK, clear LEFT/RIGHT
    //   else (diagonal or pure-side past deadzone) → keep LEFT/RIGHT, clear FWD/BACK/MOVE
    //
    // Gated on stick being past the deadzone so keyboard input still flows
    // through unchanged (when stick is centred, dx_c/dy_c are ~0 and the
    // dominance check would mis-classify centred-stick as side-ish).
    if (p_person->State == STATE_DANGLING && input_gamepad_connected()) {
        const SLONG axis_x = input_stick_x_axis_raw(ACT_FOOT_MOVE_GAXIS);
        const SLONG axis_y = input_stick_y_axis_raw(ACT_FOOT_MOVE_GAXIS);
        const SLONG dx_c = axis_x - 32768;
        const SLONG dy_c = axis_y - 32768;
        const bool stick_active = (abs(dx_c) > 8192) || (abs(dy_c) > 8192);
        if (stick_active) {
            const bool dy_dominates_dx = abs(dy_c) > STICK_AXIS_DOMINANCE_RATIO * abs(dx_c);
            if (dy_dominates_dx) {
                input &= ~(INPUT_MASK_LEFT | INPUT_MASK_RIGHT);
            } else {
                input &= ~(INPUT_MASK_FORWARDS | INPUT_MASK_BACKWARDS | INPUT_MASK_MOVE);
            }
        }
    }

    ULONG input_used = 0;
    ULONG processed = 0;

    // L2 SIDE ROLL (non-fight). Fired from g_l2_roll_request (set in
    // get_hardware_input on a quick L2 tap released with the stick to a side).
    // It must produce a roll or NOTHING — never a jump — so the action tree is
    // only PROBED (with INPUT_MASK_JUMP) to confirm the character is in a
    // jump-capable state (the same gating the old ✕ roll relied on); the flip is
    // then triggered directly. No jump is ever dispatched.
    if (g_l2_roll_request && should_i_jump(p_person)) {
        ULONG probe_used;
        const SLONG probe = find_best_action_from_tree(
            p_person->Genus.Person->Action, INPUT_MASK_JUMP, &probe_used);
        if (probe == ACTION_STANDING_JUMP || probe == ACTION_RUNNING_JUMP) {
            // Snap facing so the flip's perpendicular motion travels toward the
            // requested world direction (left flip → facing+512, right → -512;
            // see SUB_STATE_FLIPING in person.cpp).
            const SLONG facing = (g_l2_roll_flip_dir == 0)
                ? (g_l2_roll_target_angle - 512)
                : (g_l2_roll_target_angle + 512);
            p_person->Draw.Tweened->Angle = (facing + 2048) & 2047;
            set_person_flip(p_person, g_l2_roll_flip_dir);
            g_l2_roll_request = 0;
            return processed | input_used; // roll committed — nothing else this tick
        }
    }

    // Min stamina (Stamina units) to START a sprint — original threshold.
    constexpr SLONG PERSON_SPRINT_MIN_STAMINA = 3;

    // USE — interaction multitool (Square / F). The old sprint and crouch
    // fallbacks that used to live at the tail of do_an_action are split out onto
    // their own buttons below, so do_an_action is now pure "interact".
    if (input & INPUT_MASK_ACTION) {
        processed |= do_an_action(p_person, input);
        input &= ~processed;
    }

    // SPRINT (Circle / Shift) — hold while running to sprint; release drops back
    // to a normal run.
    if (input & INPUT_MASK_SPRINT) {
        if (p_person->State == STATE_MOVEING) {
            if (p_person->SubState == SUB_STATE_RUNNING) {
                if (p_person->Genus.Person->Stamina > PERSON_SPRINT_MIN_STAMINA
                    && p_person->Genus.Person->PersonType != PERSON_ROPER)
                    p_person->Genus.Person->Mode = PERSON_MODE_SPRINT;
            } else if (p_person->SubState == SUB_STATE_WALKING) {
                p_person->Genus.Person->Mode = PERSON_MODE_RUN;
            }
        }
    } else {
        if (p_person->Genus.Person->Mode == PERSON_MODE_SPRINT)
            p_person->Genus.Person->Mode = PERSON_MODE_RUN;
    }

    // STEALTH (Triangle / C) — hold to crouch (stealth pose); release to stand.
    // Pure crouch: arrest stays on the USE button (do_an_action), so crouching
    // next to a suspect no longer arrests them.
    if (input & INPUT_MASK_STEALTH) {
        if (p_person->State == STATE_IDLE && p_person->SubState == 0)
            set_person_croutch(p_person);
        else if (p_person->State == STATE_GUN)
            set_person_croutch(p_person);
    } else {
        if (p_person->SubState == SUB_STATE_IDLE_CROUTCHING) {
            MSG_add("no stealth so stand up");
            set_person_idle_uncroutch(p_person);
        }
    }

    // SPRINT/STEALTH were handled directly above — drop their bits so the action
    // tree below (find_best_action_from_tree) never sees them.
    input &= ~(INPUT_MASK_SPRINT | INPUT_MASK_STEALTH);

    if (p_person->State == STATE_CARRY) {
        if (p_person->SubState == SUB_STATE_PICKUP_CARRY || p_person->SubState == SUB_STATE_DROP_CARRY) {
            return processed | input_used;
        }
    }

    if (input && (p_person->State != STATE_CARRY)) {
        SLONG new_action;

        new_action = find_best_action_from_tree(p_person->Genus.Person->Action, input, &input_used);

        // OpenChaos: char-relative stand-up override. While sitting,
        // the player's stand-up intent is "stick in the direction the
        // character is facing", not "stick UP in camera frame". While the
        // back-walk modifier is held the entire stand-up path is blocked
        // (you backed into the bench and are still holding it). Otherwise:
        //   - char-rel forward + raw FORWARDS not set (e.g. char faces
        //     camera so player pushes stick DOWN = char-forward):
        //     action_sit table didn't return UNSIT, so force it here.
        //   - char-rel not-forward but action tree returned UNSIT
        //     (raw FORWARDS held but stick is actually char-back):
        //     suppress the unsit.
        if (p_person->Genus.Person->Action == ACTION_SIT_BENCH
            && !input_backwalk_held()) {
            const bool char_fwd = input_is_char_rel_forward(p_person, input);
            if (char_fwd) {
                new_action = ACTION_UNSIT;
                input_used = INPUT_MASK_FORWARDS | INPUT_MASK_MOVE;
            } else if (new_action == ACTION_UNSIT) {
                new_action = 0;
                input_used = 0;
            }
        }

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
            // OpenChaos: while the back-walk modifier is held the same
            // stick they used to back into the chair is typically still
            // held, which the action_sit table would otherwise read as a
            // stand-up command. Just block unsit until the modifier is
            // released; nothing else is affected because it isn't held
            // during normal play.
            if (input_backwalk_held()) {
                input_used = 0;
                break;
            }
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
                // ✕ during BACK-WALK → backflip toward the live stick direction.
                // The player commits to walk-back (zoom/back-walk modifier — L1 /
                // E) and flips out of it with jump. Side rolls do NOT come through
                // here (posted via g_l2_roll_request, never a jump). Standard
                // +1024 facing snap.
                if (should_i_jump(p_person) && g_backwalk_regime == BACKWALK_REGIME_ACTIVE) {
                    if (should_person_backflip(p_person)) {
                        const SLONG raw_dx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
                        const SLONG raw_dy = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);
                        SLONG target_angle = Arctan(-raw_dx, raw_dy) + get_camera_angle() + 1024;
                        target_angle = (target_angle + 2048) & 2047;
                        p_person->Draw.Tweened->Angle = target_angle;
                        set_person_standing_jump_backwards(p_person);
                    } else {
                        set_person_standing_jump(p_person); // blocked → vertical jump
                    }
                    break;
                }

                // Source-aware fallback (not back-walking).
                //
                // Analog stick (analogue == 1): light side / back stick taps produced
                // unwanted rolls / backflips — including right after a climb (player
                // mashes side + jump while still in pull-up, gets a roll and falls off
                // the ledge). Stick path now produces only forward-jump (on FORWARDS)
                // or snap-and-forward (any movement direction).
                //
                // D-pad / keyboard arrows (analogue == 0): legacy auto-roll /
                // auto-backflip on side / back + jump is kept intact until the L2
                // modifier scheme replaces it across all input sources too. The
                // action priority entries in action_idle / action_standing_jump
                // tables were removed so all routes (stick and digital) flow
                // through this body with a single analogue gate.
                //
                // can_jump (should_i_jump): false in warehouse-doorway-like tiles
                // and right after climb — for stick that means do nothing (avoid
                // climb-edge rolls); for D-pad/keyboard, legacy roll-anyway path
                // is preserved.
                if (should_i_jump(p_person)) {
                    if (input & INPUT_MASK_FORWARDS) {
                        // OpenChaos: snap facing to stick+camera direction
                        // before jumping. apply_button_input dispatches the
                        // action tree BEFORE player_interface_move runs the
                        // rotation pass, so on the first tick of (idle +
                        // forward + jump) the character's Tweened->Angle is
                        // still the pre-rotation value. Without the snap,
                        // set_person_standing_jump_forwards bakes that stale
                        // angle and the jump flies in the OLD facing instead
                        // of the camera-relative direction the player aimed
                        // at. Race-prone: if forward and jump arrive on
                        // separate ticks the character has already rotated
                        // (1-tick max_angle=1024 snap from STATE_IDLE in
                        // player_turn_left_right_analogue), so the bug only
                        // shows when both inputs land on the same physics
                        // tick → user perceives "sometimes works, sometimes
                        // jumps the wrong way" on stand → forward + jump.
                        //
                        // Same snap formula as the side/back-stick branch
                        // below and the L2-tactical branches above. Guarded
                        // on (dx|dz) != 0 to skip the digital-pure-arrow
                        // case where bits 18-31 carry no stick value
                        // (Arctan(0,0) is undefined) — there the legacy
                        // non-analog rotation in player_turn_left_right
                        // path handles facing.
                        const SLONG dx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
                        const SLONG dz = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);
                        if (dx != 0 || dz != 0) {
                            SLONG target_angle = Arctan(-dx, dz) + get_camera_angle();
                            target_angle = (target_angle + 2048) & 2047;
                            p_person->Draw.Tweened->Angle = target_angle;
                        }
                        set_person_standing_jump_forwards(p_person);
                    } else if (analogue && (input & (INPUT_MASK_LEFT | INPUT_MASK_RIGHT | INPUT_MASK_BACKWARDS))) {
                        // Stick: any movement direction → forward jump in stick
                        // direction (same end-call as running jump). Without
                        // this, slow-walk side/back + jump produced a feel-wrong
                        // vertical jump on spot, while slow-walk forward + jump
                        // already did a directional jump.
                        //
                        // Snap the character facing to stick direction first.
                        // In most cases analog rotation has already aligned them
                        // (running and slow-walk both continuously rotate the
                        // character to face the stick), so the snap is a no-op.
                        // But in transitional states where rotation was suppressed
                        // — e.g. character just finished a climb-up, was locked
                        // facing forward throughout the pull-up anim — the
                        // character is still facing the old direction at the
                        // instant the buffered jump fires. Without snap,
                        // running_jump propels them in that old direction
                        // instead of where the player is holding the stick.
                        //
                        // Same stick → world angle formula as
                        // process_analogue_movement.
                        const SLONG dx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
                        const SLONG dz = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);
                        SLONG target_angle = Arctan(-dx, dz) + get_camera_angle();
                        target_angle = (target_angle + 2048) & 2047;
                        p_person->Draw.Tweened->Angle = target_angle;
                        set_person_standing_jump_forwards(p_person);
                    } else if (!analogue && (input & INPUT_MASK_BACKWARDS) && should_person_backflip(p_person))
                        set_person_standing_jump_backwards(p_person);
                    else if (!analogue && (input & INPUT_MASK_LEFT))
                        set_person_flip(p_person, 0);
                    else if (!analogue && (input & INPUT_MASK_RIGHT))
                        set_person_flip(p_person, 1);
                    else
                        set_person_standing_jump(p_person);
                } else if (!analogue) {
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
                // No ✕ backflip here: back-walk caps velocity to walk speed, so a
                // jump while backing resolves to ACTION_STANDING_JUMP (handled
                // there). Side rolls never come through here either (posted via
                // g_l2_roll_request, never a jump).
                if (should_i_jump(p_person)) {
                    // OpenChaos: snap facing to stick+camera direction when
                    // the character JUST transitioned from IDLE → MOVEING
                    // (g_player_run_entry_ticks > 0). Mirrors the snap in
                    // ACTION_STANDING_JUMP and covers the residual 5% of the
                    // "stand → W+Space" race where W landed one tick earlier
                    // (state switched to MOVEING + Angle partially rotated
                    // toward camera). Without this, ACTION_RUNNING_JUMP fires
                    // through the action_run table on the Space tick and
                    // set_person_running_jump bakes the half-rotated facing
                    // → jump goes in an in-between direction.
                    //
                    // Window-gated (counter expires after a few ticks), so a
                    // deliberate "running steady + camera flip + jump" keeps
                    // its existing behaviour (no snap, jump in current facing).
                    if (g_player_run_entry_ticks > 0) {
                        const SLONG dx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
                        const SLONG dz = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);
                        if (dx != 0 || dz != 0) {
                            SLONG target_angle = Arctan(-dx, dz) + get_camera_angle();
                            target_angle = (target_angle + 2048) & 2047;
                            p_person->Draw.Tweened->Angle = target_angle;
                        }
                    }
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
                    // OpenChaos: shooting blocked during walk_back (see
                    // set_player_shoot for rationale). action_walk_back
                    // maps PUNCH to ACTION_FIGHT_PUNCH which calls
                    // set_person_shoot directly here, bypassing
                    // set_player_shoot — so the gate has to repeat here.
                    if (p_person->SubState == SUB_STATE_WALKING_BACKWARDS)
                        break;
                    set_person_shoot(p_person, 0);
                } else if (!player_activate_in_hand(p_person))
                    if (turn_to_target_and_punch(p_person)) {
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
    if (input & INPUT_MOVEMENT_MASK) {
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
                // OpenChaos: stand-up via this path uses the same gates
                // as the action_sit / ACTION_UNSIT dispatch above — the
                // back-walk modifier hold blocks stand-up entirely;
                // otherwise the input must be CHAR-RELATIVE forward, not
                // raw camera-frame forward. Without char-rel the raw stick
                // UP held to back into the bench (= char-back when char
                // faces the camera) would yank her out of the sit anim one
                // tick after set_person_sit_down.
                if (!input_backwalk_held()
                    && input_is_char_rel_forward(p_person, input)) {
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

// OpenChaos: true while Darci is mid a GRAPPLE/throw/escape. A new
// grapple (forward+punch hook) or a target switch (circle) must NOT be
// started while one of these is running -- re-grappling there
// chain-teleports her between victims and corrupts the animation; she
// has to finish the grapple first. Deliberately does NOT include
// PUNCH/KICK/STEP_FORWARD: the grab is MEANT to interrupt a punch/kick
// combo (that's how "forward+punch" grabs at all), and punch/kick
// already set FLAG_PERSON_NON_INT_M which the target-switch gate checks
// separately. Values from statedef.h.
static inline bool oc_combat_busy(SLONG sub)
{
    return sub == SUB_STATE_GRAPPLE
        || sub == SUB_STATE_GRAPPLE_HOLD || sub == SUB_STATE_GRAPPLE_HELD
        || sub == SUB_STATE_ESCAPE || sub == SUB_STATE_GRAPPLE_ATTACK;
}

// uc_orig: apply_button_input_fight (fallen/Source/interfac.cpp)
// Input handler for PERSON_MODE_FIGHT. Handles punch/kick direction combos, target selection,
// fight stepping, flip, and exit from fight mode (MOVE without FORWARDS).
ULONG apply_button_input_fight(Thing* p_player, Thing* p_person, ULONG input)
{
    Player* pl = p_player->Genus.Player;

    if (p_person->SubState == SUB_STATE_IDLE_CROUTCH_ARREST)
        return (0);

    // OpenChaos: allow switching the combat target in two cases the
    // normal target-switch path below cannot handle:
    //   1. Hit-recoil (taking damage) -- that path's state gate only
    //      accepts IDLE / FIGHTING / RUN_STOP, never STATE_HIT_RECOIL.
    //   2. A fight-stance STEP (moving in any direction, all
    //      SUB_STATE_STEP_FORWARD) -- the directional step handler
    //      further down does `return(0)` whenever a move direction is
    //      held, swallowing the ACTION press before the target-switch
    //      block is ever reached.
    // Both are brief, safe moves where re-targeting must stay live, so
    // handle them up here, before the step handler. Scoped to the
    // target switch only: arrest / item pickup keep their original
    // state requirements. oc_combat_busy stays respected so a grapple/
    // throw still can't be interrupted (a recoil/step never overlaps
    // one, but keep the guard for safety). Flips/rolls are a different
    // substate and are deliberately NOT included.
    if ((pl->Pressed & INPUT_MASK_ACTION) && p_person->Genus.Person->PlayerID
        && (p_person->State == STATE_HIT_RECOIL
            || p_person->SubState == SUB_STATE_STEP_FORWARD)
        && !oc_combat_busy(p_person->SubState)) {
        person_pick_best_target(p_person, 1);
        pl->DoneSomething = UC_TRUE;
        return INPUT_MASK_ACTION;
    }

    if (pl->Pressed & INPUT_MASK_PUNCH)
        if (person_has_gun_out(p_person)) {
            set_player_shoot(p_person, 0);
            return (INPUT_MASK_PUNCH);
        }

    // OpenChaos: forward + punch = GRAPPLE attempt, intercepted right
    // here at the input level -- BEFORE the combo system swallows the
    // punch press (the trace showed in-fight punches never reach
    // set_person_punch, so a grapple check buried there can never fire).
    // Works regardless of substate / combo, in any input device:
    // keyboard & D-pad set the digital INPUT_MASK_FORWARDS bit, the
    // analog stick packs its deflection into the high Input bits so we
    // also decode stick Y (negative = pushed forward). find_best_grapple
    // self-gates on the grapple cooldown (while on CD it returns 0 and
    // we just fall through to the normal punch/combo) and finds its own
    // target; if nothing grabs we also fall through. Player only; this
    // handler is the player's fight input path.
    if ((pl->Pressed & INPUT_MASK_PUNCH) && p_person->Genus.Person->PlayerID
        && !oc_combat_busy(p_person->SubState)) {
        ULONG pin = pl->Input;
        const SLONG STICK_DEADZONE = 8; // ANALOGUE_MIN_VELOCITY
        SLONG sy = input_virtual_axis(pin, ACT_FOOT_MOVE_Y_VAXIS); // forward/back intent (<0 = fwd)
        bool fwd = (pin & INPUT_MASK_FORWARDS) || (sy < -STICK_DEADZONE);
        if (fwd && find_best_grapple(p_person)) {
            pl->DoneSomething = UC_TRUE;
            return (INPUT_MASK_PUNCH);
        }
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

    if (input) {
        SLONG new_action;
        ULONG input_used;

        new_action = find_best_action_from_tree(p_person->Genus.Person->Action, input, &input_used);
        switch (new_action) {
        case ACTION_RESPAWN:
            break;
        }
    }

    // OpenChaos: combat directional ROLL — ✕ and L2 are full equivalents. A
    // press of ✕ with the stick held in a direction, OR a quick L2 tap
    // (g_l2_roll_request), rolls toward the stick's WORLD direction (same as the
    // on-foot roll) — any direction, not just left/right. Runs before the
    // fight-step / step-✕ handlers below so the chord rolls instead of stepping.
    // Allowed from the idle stance or a fight step, and not mid punch/kick.
    {
        SLONG roll_target = 0, roll_flip = 0;
        bool do_roll = false;
        if (g_l2_roll_request) {
            roll_target = g_l2_roll_target_angle;
            roll_flip = g_l2_roll_flip_dir;
            g_l2_roll_request = 0;
            do_roll = true;
        } else if (pl->Pressed & INPUT_MASK_JUMP) {
            const SLONG sx = input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS);
            const SLONG sy = input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS);
            do_roll = compute_l2_roll_dir(p_person, sx, sy, &roll_target, &roll_flip);
        }
        if (do_roll
            && (pl->Pressed & INPUT_MASK_PUNCH) == 0 && (pl->Pressed & INPUT_MASK_KICK) == 0
            && (p_person->State == STATE_IDLE || p_person->SubState == SUB_STATE_STEP_FORWARD)) {
            const SLONG facing = (roll_flip == 0) ? (roll_target - 512) : (roll_target + 512);
            p_person->Draw.Tweened->Angle = (facing + 2048) & 2047;
            set_person_flip(p_person, roll_flip);
            return (0);
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

    // OpenChaos: a fresh "back" press arms exactly one defensive block
    // for this hold (consumed in set_person_block). This does NOT touch
    // the back-step movement -- it still works exactly as before; only
    // the block/duck engage is gated. Player only.
    if (p_person->Genus.Person->PlayerID && (pl->Pressed & INPUT_MASK_BACKWARDS))
        player_block_arm(p_person);

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

                // OpenChaos: player arrest cooldown (see note in do_an_action).
                if (p_person->Genus.Person->PersonType == PERSON_DARCI && (index = find_arrestee(p_person))) {
                    if (combat_cooldown_ready(p_person->Genus.Person->PlayerID, COOLDOWN_ARREST)) {
                        combat_cooldown_note(p_person->Genus.Person->PlayerID, COOLDOWN_ARREST, "ARREST", true);
                        combat_cooldown_arm(p_person->Genus.Person->PlayerID, COOLDOWN_ARREST);
                        set_person_arrest(p_person, index);
                    } else {
                        combat_cooldown_note(p_person->Genus.Person->PlayerID, COOLDOWN_ARREST, "ARREST", false);
                        PANEL_new_info_message(XLAT_str(X_FAILED));
                    }
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

                // OpenChaos: only let the ACTION button switch the
                // combat target while Darci is at rest -- NOT mid
                // punch/kick/grapple/throw/arrest. Switching the target
                // while a committed action animation is running corrupts
                // that animation and (combined with the grapple) makes
                // her dart between targets. FLAG_PERSON_NON_INT_M is set
                // by every committed action and cleared by fight-idle,
                // so it's exactly "busy vs ready". When busy we still
                // consume the press (no fall-through) but don't reselect.
                // (Hit-recoil and fight-stance steps are handled in an
                // earlier block, before this state-gated path is even
                // reached.)
                if (!(p_person->Genus.Person->Flags & FLAG_PERSON_NON_INT_M)
                    && !oc_combat_busy(p_person->SubState)) {
                    person_pick_best_target(p_person, 1);
                }
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
                // OpenChaos: before a forward punch fires, re-target onto
                // whatever the player is actually looking at (closest
                // enemy inside a narrow cone around the camera ray). The
                // original game only re-aims through the body-cone search
                // once the attack runs, so the locked target stuck around
                // even after the player had turned the camera onto a
                // different enemy. snap_body == 1 also points Darci at
                // the new target so the punch animation starts already
                // facing it -- consistent with how the forward-kick path
                // (turn_to_target_and_kick) snaps via find_attack_stance.
                // No-op when nothing is in the cone -- existing flow
                // resumes unchanged. Same busy-gate as the Circle target
                // switch above: mid-grapple/throw/arrest we leave the
                // locked target alone so the committed action plays out.
                if (!(p_person->Genus.Person->Flags & FLAG_PERSON_NON_INT_M)
                    && !oc_combat_busy(p_person->SubState)) {
                    player_pick_attack_target_by_camera(p_person, 1 /*snap_body*/);
                }
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
                // OpenChaos: see the matching note in the forward-punch
                // branch above. Camera-cone re-target also runs before
                // the forward kick. Directional kicks (left/right/back)
                // intentionally skip this -- those are an explicit
                // off-camera input and should not be hijacked back
                // toward whatever the camera is on.
                if (!(p_person->Genus.Person->Flags & FLAG_PERSON_NON_INT_M)
                    && !oc_combat_busy(p_person->SubState)) {
                    player_pick_attack_target_by_camera(p_person, 1 /*snap_body*/);
                }
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

    } else {
        // Combo queuing: set REQUEST flags when buttons pressed during a fighting animation.
        if (pl->Pressed & INPUT_MASK_PUNCH) {
            if (!(p_person->Genus.Person->Flags & FLAG_PERSON_REQUEST_PUNCH)) {
                // OpenChaos: re-target on every queued punch in a combo,
                // same camera-cone logic as the fresh-attack branches.
                // snap_body == 0: Darci is already mid-animation, so the
                // next combo node's aim_at_victim handles the smooth
                // turn toward the new target. A snap here would visibly
                // pop her mid-anim. Same busy-gate as Circle / fresh
                // attacks: grapple/throw/arrest keep the locked target.
                if (!(p_person->Genus.Person->Flags & FLAG_PERSON_NON_INT_M)
                    && !oc_combat_busy(p_person->SubState)) {
                    player_pick_attack_target_by_camera(p_person, 0 /*no snap*/);
                }
                p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_PUNCH;
            }
        }

        if (pl->Pressed & INPUT_MASK_KICK) {
            if (!(p_person->Genus.Person->Flags & FLAG_PERSON_REQUEST_KICK)) {
                // OpenChaos: same camera-cone re-target for queued kicks.
                // See the note in the matching punch branch above.
                if (!(p_person->Genus.Person->Flags & FLAG_PERSON_NON_INT_M)
                    && !oc_combat_busy(p_person->SubState)) {
                    player_pick_attack_target_by_camera(p_person, 0 /*no snap*/);
                }
                p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_KICK;
            }
        }
    }

    return 0;
}

// uc_orig: apply_button_input_car (fallen/Source/interfac.cpp)
// Translates the input bitmask into steering/throttle/brake for the vehicle the player is driving.
// Steering: always the damped analogue path — the wheel accumulator eases toward
// the joypad/WASD X axis at a constant rate (STEERING_MAX_DELTA per tick).
ULONG apply_button_input_car(Thing* p_furn, ULONG input)
{
    ULONG processed_input = 0;

    ASSERT(p_furn->Class == CLASS_VEHICLE);

    Vehicle* veh = p_furn->Genus.Vehicle;

    // Player car steering is ALWAYS the analogue damper path — there is no
    // digital fallback. This keeps the wheel centring at one consistent rate
    // whether gassing or coasting (the old digital ">>1" path only ran while
    // coasting, so the wheel lingered far longer on a coast-release than on a
    // gas-release — felt like the car kept turning after letting go). KBM packs
    // a centred stick baseline when no WASD is held, so GET_JOYX reads 0 there;
    // the gamepad reads its stick; AI steers via pcom, not through here.
    //
    // The accumulator eases toward the input at a constant rate (the original
    // damping). Speed-sensitive steering caps/rates were tried here but made the
    // feel unstable near the skid threshold — reverted to this simple, stable form.
    static SWORD wCurrentSteering = 0;
#define STEERING_MAX_DELTA 30

    SWORD wSteering = input_virtual_axis(input, ACT_CAR_STEER_VAXIS);
    if ((input & (INPUT_MASK_LEFT | INPUT_MASK_RIGHT)) == 0) {
        wSteering = 0;
    }

    if (wCurrentSteering > wSteering) {
        wCurrentSteering -= STEERING_MAX_DELTA;
        if (wCurrentSteering < wSteering)
            wCurrentSteering = wSteering;
    } else {
        wCurrentSteering += STEERING_MAX_DELTA;
        if (wCurrentSteering > wSteering)
            wCurrentSteering = wSteering;
    }
    veh->Steering = wCurrentSteering;
    veh->IsAnalog = 1;

    veh->DControl = 0;

    // Three independent in-car controls — gas, brake, reverse:
    //   Keyboard: W=gas, Space=brake, S=reverse, A/D=steer, E=siren.
    //   Gamepad:  R2=gas, L1=brake, L2=reverse, stick X=steer (Y ignored),
    //             Triangle=siren.
    //
    // VEH_FASTER: always-on, unconditionally, no input gating. The flag
    // historically had keyboard and gamepad binding chords to toggle it,
    // but in manual testing no perceptible vehicle behavior change was
    // observed. To avoid wasting a chord (and to free the chord component
    // keys for the new WASD car layout), the flag is just always set
    // every tick. If a future audit of vehicle.cpp shows the flag actually
    // does something undesirable when always-on, gate it here.
    veh->DControl |= VEH_FASTER;

    bool ctl_accel;
    bool ctl_brake;
    bool ctl_reverse;
    if (active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE) {
        // Read W / S / Space DIRECTLY as independent gas / reverse / brake
        // keys. We must NOT go through the WASD analog-stick bits here: W and
        // S are opposite ends of the same stick Y axis, so holding BOTH cancels
        // to centre (neither FORWARDS nor BACKWARDS) — which made W+S do
        // nothing instead of reversing. Steering (A/D) still rides the stick X
        // axis read at the top of this function. Gated by input_gameplay_enabled
        // so the F1 debug modifier suppresses driving, same as the stick path.
        const bool gameplay = input_gameplay_enabled();
        ctl_accel = gameplay && input_key_held(ACT_CAR_ACCEL_KKEY); // W
        ctl_reverse = gameplay && input_key_held(ACT_CAR_REVERSE_KKEY); // S
        ctl_brake = gameplay && input_key_held(ACT_CAR_BRAKE_KKEY); // Space
    } else {
        // Triggers/bumpers read as digital bits here. Analog trigger position
        // (for proportional gas) is read separately in vehicle.cpp.
        // Left stick is STEERING ONLY (X axis); stick Y is intentionally
        // ignored for gas/brake — the previous stick-Y mapping auto-braked
        // mid-corner on slight pull-back / drift ("car brakes by itself").
        ctl_accel = input_btn_held(ACT_CAR_ACCEL_GBTN); // R2
        ctl_brake = input_btn_held(ACT_CAR_BRAKE_GBTN); // L1
        ctl_reverse = input_btn_held(ACT_CAR_REVERSE_GBTN); // L2
    }

    // Set every held control; pedals() resolves the priority brake > reverse
    // > gas. Setting all bits (rather than an else-if chain) is what makes
    // simultaneous presses behave: gas+brake -> brake wins, gas+reverse ->
    // reverse wins (the old else-if let gas mask the brake — BUG-6).
    if (ctl_accel)
        veh->DControl |= VEH_ACCEL;
    if (ctl_reverse)
        veh->DControl |= VEH_REVERSE;
    if (ctl_brake)
        veh->DControl |= VEH_DECEL;

    // Siren toggle: edge-triggered via input_frame's sticky press_pending.
    // do_car_input reads VEH_SIREN and calls siren(veh, !veh->Siren) — without
    // edge-detect, holding the button toggles the siren every physics tick,
    // giving unpredictable on/off depending on hold duration × physics Hz
    // (issue #13 in fps_unlock_issues_resolved).
    //
    // press_pending (rather than just_pressed or a local level-edge tracker)
    // is required because this code runs on physics tick, not render frame.
    // At low physics Hz a tap can fit entirely between two ticks — the level
    // signal sampled at each tick shows "not held" both times → tap missed.
    // press_pending is latched on rising edge by every render-frame snapshot
    // and survives until consumed → physics tick reliably sees the press
    // even if its sampling window doesn't overlap the held window.
    //
    // Drain rule: the call site (`do_packets` else-branch when not driving)
    // also consumes both pendings every physics tick to prevent stale flags
    // from outside-car presses (Triangle pressed on foot, SPACE = jump)
    // leaking into the first car-entry tick as a spurious toggle. Triangle
    // on foot is currently unbound, but the press_pending flag is still set
    // by the input layer whenever it's physically pressed.
    {
        // Suppress KB siren read when F1 debug modifier is held (gameplay-
        // input gate). Gamepad siren read stays unconditional — F1 is a
        // keyboard concept.
        const bool kb_siren = input_gameplay_enabled() && input_key_press_pending(ACT_CAR_SIREN_KKEY);
        const bool pad_siren = input_btn_press_pending(ACT_CAR_SIREN_GBTN);
        input_key_consume(ACT_CAR_SIREN_KKEY);
        input_btn_consume(ACT_CAR_SIREN_GBTN);
        if (kb_siren || pad_siren) {
            veh->DControl |= VEH_SIREN;
        }
    }

    return (processed_input);
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
    if (input_debug_is_active())
        return 0;

    ULONG input = 0;

    // On-foot modifier flags are re-evaluated from scratch every tick.
    // L2-held on the gamepad (below) may set m_bForceWalk (in any L2 regime
    // except NONE). player_relative is currently never set to 1 by anything —
    // it's reset here to keep the orphan global at a known value. BACKWARD
    // regime uses player_relative=0 (camera-relative) plus explicit walk_back
    // entry/exit in player_turn_left_right_analogue, not tank-mode auto-entry.
    m_bForceWalk = UC_FALSE;
    player_relative = 0;

    static bool bLastInputWasntAnInputCozThereWasNoController = UC_TRUE;

    // Deadzone for the analogue stick: raw axis 0..65535, centre 32768. The
    // deadzone width (NOISE_TOLERANCE) is now read from config at startup
    // (gamepad.gameplay_stick_deadzone, a 0..1 fraction) instead of a hardcoded
    // 8192 — input_gameplay_deadzone_raw() returns the raw value (default 0.25 =
    // 8192, unchanged). Menu navigation has its own, separately-configured
    // threshold (gamepad.menu_stick_deadzone) so the two can be tuned apart.
    // AXIS_MIN/MAX bracket the centre by the deadzone.
#define AXIS_CENTRE 32768
    const SLONG NOISE_TOLERANCE = input_gameplay_deadzone_raw();
    const SLONG AXIS_MIN = AXIS_CENTRE - NOISE_TOLERANCE;
    const SLONG AXIS_MAX = AXIS_CENTRE + NOISE_TOLERANCE;

    // gamepad_state is filled by input_frame_update() → gamepad_poll() each
    // render frame; we read it through the input_frame API below.

    DWORD dwCurrentTime = 0;

    if (type & INPUT_TYPE_JOY) {
        // Skip the entire gamepad block when KBM is the active input device.
        // Without this, gamepad data leaks into the input mask (stick packing
        // into bits 18-31, button bits, etc.) and tints the player input even
        // though the user is on keyboard. Specifically: the gamepad block
        // unconditionally packs CENTER stick into bits 18-31, which the
        // keyboard path then relies on as the "neutral stick" baseline — so
        // disconnecting the gamepad made GET_JOYX/Y return -128 (raw zero
        // bits decode as full-deflection), the character was forced into
        // STATE_MOVEING, action_run mapped PUNCH → ACTION_SHOOT, no gun →
        // silent no-op → "click doesn't punch" bug.
        //
        // active_input_device is auto-switched by the input_frame layer when
        // it sees any gamepad input, so the gate doesn't lock the user into
        // KBM — the first gamepad input flips active_input_device to PAD and
        // this block resumes running.
        if (input_gamepad_connected() && active_input_device != INPUT_DEVICE_KEYBOARD_MOUSE) {
            {
                ULONG ulAxisMax = AXIS_MAX;
                ULONG ulAxisMin = AXIS_MIN;

                // OpenChaos: D-Pad no longer drives on-foot movement. Use the
                // raw (pre-D-Pad-override) stick axes — D-Pad presses don't
                // mirror into the stick reading, so the D-Pad alone produces
                // no character movement here. D-Pad buttons themselves are
                // still readable via input_btn_held(11..14) for menu nav and
                // any other consumer that wants them.
                // While R3 (inventory) is held the left stick is repurposed as
                // the weapon-popup scroll (read separately via the stick virtual-
                // direction in process_controls), so suppress its on-foot
                // movement here — otherwise the character would run while the
                // player browses weapons. R3 isn't reachable in the car.
                const bool r3_inventory_held = input_btn_held(ACT_FOOT_INVENTORY_GBTN);
                const SLONG axis_x = r3_inventory_held ? (SLONG)AXIS_CENTRE : input_stick_x_axis_raw(ACT_FOOT_MOVE_GAXIS);
                const SLONG axis_y = r3_inventory_held ? (SLONG)AXIS_CENTRE : input_stick_y_axis_raw(ACT_FOOT_MOVE_GAXIS);

                if (input_gamepad_connected()) {
                    // Analog stick mode: pack position into bits 18-31.
                    // process_analogue_movement() uses these for proportional speed.
                    // Digital direction flags set only for menus (not movement).
                    analogue = 1;
                    // Apply deadzone before packing — stick drift within NOISE_TOLERANCE
                    // must pack as centre, otherwise continue_moveing() sees phantom input.
                    SLONG pack_x = axis_x;
                    SLONG pack_y = axis_y;
                    if (pack_x >= (SLONG)ulAxisMin && pack_x <= (SLONG)ulAxisMax)
                        pack_x = AXIS_CENTRE;
                    if (pack_y >= (SLONG)ulAxisMin && pack_y <= (SLONG)ulAxisMax)
                        pack_y = AXIS_CENTRE;
                    input |= ((pack_x >> 9) + 0) << 18;
                    input |= ((pack_y >> 9) + 0) << 25;
                } else {
                    // No gamepad: digital mode (full speed). Keyboard branch
                    // below also sets analogue=0 if movement keys are pressed.
                    analogue = 0;
                }

                // Always set digital direction flags from axes (needed for menus).
                if (axis_x > (SLONG)ulAxisMax) {
                    g_dwLastInputChangeTime = dwCurrentTime;
                    input |= INPUT_MASK_RIGHT;
                } else if (axis_x < (SLONG)ulAxisMin) {
                    g_dwLastInputChangeTime = dwCurrentTime;
                    input |= INPUT_MASK_LEFT;
                }

                if (axis_y > (SLONG)ulAxisMax) {
                    g_dwLastInputChangeTime = dwCurrentTime;
                    input |= INPUT_MASK_BACKWARDS;
                } else if (axis_y < (SLONG)ulAxisMin) {
                    input |= INPUT_MASK_FORWARDS;
                    input |= INPUT_MASK_MOVE;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // L2 on-foot tactical modifier: ROLL (quick tap) + SLOW-WALK
                // (hold).
                //
                // ROLL: a quick L2 TAP (press + release within the window) with
                // the stick deflected in ANY direction at release → a roll toward
                // the stick's WORLD direction (its camera-frame angle + camera
                // yaw), independent of how the character rotated to follow the
                // stick. So "stick points there → roll there", not a fixed
                // left/right relative to the current facing. The flip animation
                // only moves perpendicular to facing, so the dispatch snaps facing
                // to send the motion the right way; the left/right flip side is
                // chosen to KEEP the character's toward/away-from-camera
                // orientation across the roll. The machine posts g_l2_roll_request
                // on the release edge; the roll dispatch fires the flip — never a
                // jump.
                //
                // SLOW-WALK (L2 HELD past the window with the stick deflected):
                // m_bForceWalk caps the run to walk speed + plays the step anim;
                // direction comes from the normal camera-relative movement path.
                // Any stick direction slow-walks that way. (Back-walk is NOT here —
                // it's on the separate zoom/back-walk modifier below.)
                //
                // Hysteresis on the L2 trigger value engages near the BOTTOM of
                // travel (~88 %) and releases on a short ease-up (~78 %) — see the
                // threshold constants below. l2_engaged is driven to false when
                // input_trigger_raw returns 0 (no pad).
                {
                    // L2 analog trigger → engaged flag with hysteresis: engage at
                    // the BOTTOM of travel (~88%), release on a short ease-up
                    // (~78%) — same feel as the R2 bare-hand punch (weapon_feel
                    // melee fire 225 / reset 200, raw 0..255). The shared
                    // process_l2_tactical machine then does the rest, identically
                    // to the keyboard Ctrl path. Operators mirror weapon_feel
                    // (> fire / <= reset).
                    static bool l2_engaged = false;
                    const SLONG l2_raw = input_trigger_raw(ACT_FOOT_TACTICAL_MODE_GTRIG);
                    constexpr SLONG L2_ENGAGE_THRESHOLD = 225; // ~88% — near end of travel
                    constexpr SLONG L2_RELEASE_THRESHOLD = 200; // ~78% — short release stroke
                    if (!l2_engaged && l2_raw > L2_ENGAGE_THRESHOLD)
                        l2_engaged = true;
                    else if (l2_engaged && l2_raw <= L2_RELEASE_THRESHOLD)
                        l2_engaged = false;

                    // Camera-frame stick offset for the shared machine.
                    const SLONG dx_c = axis_x - AXIS_CENTRE;
                    const SLONG dy_c = axis_y - AXIS_CENTRE;
                    const bool stick_in_deadzone = (abs(dx_c) <= (SLONG)NOISE_TOLERANCE) && (abs(dy_c) <= (SLONG)NOISE_TOLERANCE);

                    process_l2_tactical(l2_engaged, dx_c, dy_c, stick_in_deadzone, NET_PERSON(0));

                    // Zoom / back-walk modifier (L1): held from a standstill =
                    // zoom; held + stick = back-walk (rear stick → walk backward).
                    // See process_zoomwalk.
                    process_zoomwalk(input_btn_held(ACT_FOOT_AIM_GBTN), dx_c, dy_c, stick_in_deadzone, NET_PERSON(0));
                }

                if (input_btn_held(ACT_FOOT_JUMP_GBTN)) {
                    input |= INPUT_MASK_JUMP;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // Triangle / Y — driving siren toggle (in-car only). The kick
                // action lives on R1 (button 10) and no longer fires from
                // Triangle. INPUT_CAR_PAD_SIREN == INPUT_MASK_KICK, so while
                // driving the triangle press emits INPUT_MASK_KICK to flip
                // the siren / lights. INPUT_MASK_CANCEL (widget-UI back-out)
                // now comes from Circle (see ACT_FOOT_SPRINT_GBTN block below)
                // — modern PlayStation convention: Cross confirms, Circle cancels.
                if (input_btn_held(ACT_CAR_SIREN_GBTN)) {
                    Thing* p_darci = NET_PERSON(0);
                    const bool driving = p_darci && p_darci->Genus.Person && (p_darci->Genus.Person->Flags & FLAG_PERSON_DRIVING);
                    if (driving) {
                        input |= INPUT_MASK_KICK;
                        g_dwLastInputChangeTime = dwCurrentTime;
                    }
                }

                // R1 / RB — on-foot kick (button 10). Suppressed
                // while driving because the player has no kick action in a
                // vehicle (and the in-car CAN_PAD_SIREN bit is already covered
                // by the Triangle path above; firing it again from R1 would
                // toggle siren on every R1 press, which the player does not
                // expect in-car).
                if (input_btn_held(ACT_FOOT_KICK_GBTN)) {
                    Thing* p_darci = NET_PERSON(0);
                    const bool driving = p_darci && p_darci->Genus.Person && (p_darci->Genus.Person->Flags & FLAG_PERSON_DRIVING);
                    if (!driving) {
                        input |= INPUT_MASK_KICK;
                        g_dwLastInputChangeTime = dwCurrentTime;
                    }
                }

                // Square/X previously mapped to PUNCH/shoot; removed so that
                // punch & shoot live only on R2. Keep the button unbound in
                // gameplay — nothing to do here.

                if (input_btn_held(ACT_FOOT_START_GBTN)) {
                    input |= INPUT_MASK_START;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // Weapon cycle / inventory rotation moved from Share/Back
                // (button 4) to R3 (right stick click). Index 8 in rgbButtons
                // matches SDL3_BTN_RIGHT_STICK and the DualSense R3 mapping.
                if (input_btn_held(ACT_FOOT_INVENTORY_GBTN)) {
                    input |= INPUT_MASK_SELECT;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // This button doubles as camera-type toggle / force-behind, but
                // ONLY when the zoom/back-walk modifier enters the ZOOM mode
                // (standstill). Gating on g_zoomwalk_mode keeps the camera
                // snap-to-back exclusive to zoom entry — pressing it on the move
                // enters back-walk (process_zoomwalk) and must NOT jerk the
                // camera behind the character. In the car this button is the
                // BRAKE, so the driving guard suppresses it there too.
                // g_zoomwalk_mode is computed just above (machine runs before
                // this block), and ZOOM implies the button is held.
                {
                    Thing* p_drv = NET_PERSON(0);
                    const bool drv_driving = p_drv && p_drv->Genus.Person
                        && (p_drv->Genus.Person->Flags & FLAG_PERSON_DRIVING);
                    if (!drv_driving && g_zoomwalk_mode == ZOOMWALK_ZOOM) {
                        input |= INPUT_MASK_CAMERA;
                        g_dwLastInputChangeTime = dwCurrentTime;
                    }
                }

                // L2/R2 no longer map to camera rotation on gamepad — right stick handles camera.
                // On foot: R2 = alternative PUNCH (shoot/melee), L2 = alternative KICK.
                // In car: gas/brake handled in apply_button_input_car() via rgbButtons directly.
                // Keyboard CAM_LEFT/CAM_RIGHT mapping is preserved below.
                {
                    Thing* p_darci = NET_PERSON(0);
                    bool driving = p_darci && p_darci->Genus.Person && (p_darci->Genus.Person->Flags & FLAG_PERSON_DRIVING);
                    if (!driving) {
                        // Fire detection (rising-edge vs held-down + thresholds)
                        // is fully encapsulated in the weapon_feel module. The
                        // active WeaponFeelProfile drives both the R2/L2 firing
                        // thresholds and whether auto-fire is allowed, and it's
                        // the same profile the adaptive-trigger state machine
                        // consults — so shot and click can't desync.
                        int32_t current_weapon = SPECIAL_NONE;
                        bool weapon_drawn = false;
                        if (p_darci && p_darci->Genus.Person) {
                            const bool has_pistol_out = (p_darci->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) != 0;
                            bool has_heavy_out = false;
                            if (p_darci->Genus.Person->SpecialUse) {
                                Thing* p_special = TO_THING(p_darci->Genus.Person->SpecialUse);
                                if (p_special) {
                                    current_weapon = p_special->Genus.Special->SpecialType;
                                    has_heavy_out = (current_weapon == SPECIAL_AK47 || current_weapon == SPECIAL_SHOTGUN);
                                }
                            }
                            // Pistol sits in FLAG_PERSON_GUN_OUT with no Thing*;
                            // AK47 / shotgun sit in SpecialUse with the flag
                            // cleared (see set_person_draw_item). Either counts
                            // as "weapon drawn" for fire detection.
                            weapon_drawn = has_pistol_out || has_heavy_out;
                        }

                        // weapon_drawn disambiguates bare-hand punch (no gun)
                        // from pistol (SpecialUse==null + FLAG_PERSON_GUN_OUT):
                        // both hit SPECIAL_NONE in the profile registry, so
                        // the profile alone can't tell them apart. Without
                        // this flag the act-bit fire path waits for a
                        // hardware click that never comes (mode=NONE when
                        // gun isn't drawn), and R2 punch stops working.
                        // mag_empty: drives reload-press-mode in evaluate_fire —
                        // AK47 with empty clip behaves like a single-shot
                        // pistol so the reload PUNCH synchronises with the
                        // Weapon25 click point (reload_click_end_zone).
                        // Includes the reload gate to keep the mode stable
                        // across the reload press (gate stays set until the
                        // next fire or R2 release).
                        bool mag_empty = false;
                        if (p_darci && p_darci->Genus.Person) {
                            if (p_darci->Genus.Person->SpecialUse) {
                                Thing* p_su3 = TO_THING(p_darci->Genus.Person->SpecialUse);
                                if (p_su3 && p_su3->Genus.Special->ammo == 0)
                                    mag_empty = true;
                            } else if ((p_darci->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)
                                && p_darci->Genus.Person->Ammo == 0) {
                                mag_empty = true;
                            }
                            if (input_actions_ak47_reload_gate_set())
                                mag_empty = true;
                        }
                        WeaponFireDecision fd = weapon_feel_evaluate_fire(
                            current_weapon, input_trigger_raw(ACT_FOOT_PUNCH_GTRIG),
                            input_trigger_raw(ACT_FOOT_TACTICAL_MODE_GTRIG), weapon_drawn, mag_empty);
                        if (fd.shoot) {
                            input |= INPUT_MASK_PUNCH;
                            g_dwLastInputChangeTime = dwCurrentTime;
                        }
                        // fd.kick (analog L2 trigger) intentionally NOT applied —
                        // kick moved from L2 to R1 (digital, see the Triangle/R1
                        // block above). L2 is the ACTION button on foot now and
                        // the brake while driving.
                    }
                }

                // Square / X — USE (the interaction multitool: get-in-or-out of
                // car, pick up, search, arrest, levers, ...). Fires both on foot
                // and while driving (the get-out-of-car flow consumes it).
                if (input_btn_held(ACT_FOOT_USE_GBTN)) {
                    input |= INPUT_MASK_ACTION;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // Circle / B — SPRINT (hold while running) AND widget-UI CANCEL
                // (back-out of in-game dialogs like the form_leave_map "Leave
                // area?" prompt). CANCEL stays on Circle (the PlayStation "back"
                // button); INPUT_MASK_CANCEL is read by widget.cpp::FORM_Process
                // to dismiss in-game forms and harmlessly coexists with SPRINT on
                // foot (no widget dialog → mask is ignored).
                if (input_btn_held(ACT_FOOT_SPRINT_GBTN)) {
                    input |= INPUT_MASK_SPRINT | INPUT_MASK_CANCEL;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // Triangle / Y — STEALTH (hold to crouch). On-foot only behaviour;
                // while driving Triangle is the car siren (separate act_car map),
                // and the dispatch ignores STEALTH unless idle/gun, so setting the
                // bit here is harmless in the car.
                if (input_btn_held(ACT_FOOT_STEALTH_GBTN)) {
                    input |= INPUT_MASK_STEALTH;
                    g_dwLastInputChangeTime = dwCurrentTime;
                }

                // Gamepad cheats: Select + L1 + L2 + D-pad direction.
                // Ported from Dreamcast cheats (fallen/Source/interfac.cpp).
                // Works on DualSense (Share+L1+L2) and Xbox (Back+LB+LT).
                {
                    static bool bCheatLastFrame = false;
                    bool cheat_combo = input_btn_held(ACT_FOOT_CHEAT_MOD_SELECT_GBTN)
                        && input_btn_held(ACT_FOOT_CHEAT_MOD_L1_GBTN)
                        && input_btn_held(ACT_FOOT_CHEAT_MOD_L2_BTN_GBTN);

                    if (cheat_combo) {
                        if (input_btn_held(ACT_FOOT_CHEAT_IMMORTAL_GBTN)) {
                            if (!bCheatLastFrame) {
                                bCheatLastFrame = true;
                                cheat_apply_immortal_toggle();
                            }
                        } else if (input_btn_held(ACT_FOOT_CHEAT_FULL_HEALTH_GBTN)) {
                            if (!bCheatLastFrame) {
                                bCheatLastFrame = true;
                                cheat_apply_full_health();
                            }
                        } else if (input_btn_held(ACT_FOOT_CHEAT_SPAWN_WEAPONS_GBTN)) {
                            if (!bCheatLastFrame) {
                                bCheatLastFrame = true;
                                cheat_apply_spawn_weapons();
                            }
                        } else if (input_btn_held(ACT_FOOT_CHEAT_MAX_AMMO_GBTN)) {
                            if (!bCheatLastFrame) {
                                bCheatLastFrame = true;
                                cheat_apply_max_ammo();
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

    if ((type & INPUT_TYPE_KEY) && input_gameplay_enabled()) {

        // Movement / action buttons: level reads through input_key_held
        // (continuous "while held" semantic — these drive INPUT_MASK_* bits
        // that downstream consumers sample as level state per physics tick).
        //
        // The whole block is gated by input_gameplay_enabled() — when the
        // F1 debug modifier is held (or any other "suppress gameplay"
        // condition fires in the future), keyboard input is dropped so a
        // debug-key press doesn't ALSO drive the character.

        // WASD analog-stick emulation. Each held key contributes a unit
        // signed component (W=-Y, S=+Y, A=-X, D=+X). When two perpendicular
        // keys are held (e.g. W+D), the deflection vector is clamped to the
        // unit circle — same magnitude as a real stick at 45° (≈0.7071 per
        // axis, not 1.0+1.0 = √2). Result is packed into bits 18-31 of the
        // input mask using the same encoding as the gamepad stick path
        // (lines ~3778-3779 in this file), so downstream consumers
        // (process_analogue_movement, etc.) see WASD identically to a stick.
        //
        // Runs AFTER the gamepad stick branch above, so if both gamepad
        // stick and WASD are active, WASD takes precedence (overwrites the
        // analog bits). Pragmatic: a player using both at once isn't a
        // realistic case to optimize for; the simpler "last writer wins"
        // rule avoids cross-source blending edge cases.
        const bool kb_fwd = input_key_held(ACT_FOOT_MOVE_FORWARD_KKEY);
        const bool kb_back = input_key_held(ACT_FOOT_MOVE_BACKWARD_KKEY);
        const bool kb_left = input_key_held(ACT_FOOT_MOVE_LEFT_KKEY);
        const bool kb_right = input_key_held(ACT_FOOT_MOVE_RIGHT_KKEY);

        if (kb_fwd || kb_back || kb_left || kb_right) {
            // Signed direction along each axis.
            const SLONG vx_sign = (kb_right ? 1 : 0) - (kb_left ? 1 : 0);
            const SLONG vy_sign = (kb_back ? 1 : 0) - (kb_fwd ? 1 : 0);

            // Deflection magnitude per axis:
            //   1 axis active  → full deflection  AXIS_CENTRE = 32768
            //   2 axes active  → diagonal — 32768 / sqrt(2) ≈ 23170 per
            //                   axis, so the (vx, vy) magnitude stays at
            //                   32768 just like a stick on its rim.
            constexpr SLONG WASD_FULL_DEFLECTION_PER_AXIS = 32768;
            constexpr SLONG WASD_DIAG_DEFLECTION_PER_AXIS = 23170;
            const SLONG mag = (vx_sign != 0 && vy_sign != 0)
                ? WASD_DIAG_DEFLECTION_PER_AXIS
                : WASD_FULL_DEFLECTION_PER_AXIS;

            SLONG axis_x = AXIS_CENTRE + vx_sign * mag;
            SLONG axis_y = AXIS_CENTRE + vy_sign * mag;
            if (axis_x < 0)
                axis_x = 0;
            if (axis_x > 65535)
                axis_x = 65535;
            if (axis_y < 0)
                axis_y = 0;
            if (axis_y > 65535)
                axis_y = 65535;

            // Clear any previous bits-18-31 packing from the JOY branch
            // before writing WASD's, otherwise OR'ing in WASD on top of a
            // partially-set gamepad packing would garble the magnitude.
            input &= 0x0003FFFF;
            analogue = 1;
            input |= ((axis_x >> 9) + 0) << 18;
            input |= ((axis_y >> 9) + 0) << 25;

            // Digital direction flags — same thresholds as gamepad stick path.
            if (axis_x > (SLONG)AXIS_MAX)
                input |= INPUT_MASK_RIGHT;
            else if (axis_x < (SLONG)AXIS_MIN)
                input |= INPUT_MASK_LEFT;

            if (axis_y > (SLONG)AXIS_MAX) {
                input |= INPUT_MASK_BACKWARDS;
            } else if (axis_y < (SLONG)AXIS_MIN) {
                input |= INPUT_MASK_FORWARDS;
                input |= INPUT_MASK_MOVE;
            }

            g_dwLastInputChangeTime = dwCurrentTime;
        } else if (active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE) {
            // No WASD held AND KBM is the active device. Pack CENTER stick
            // (bits 18-31) so GET_JOYX/Y returns 0 — without this baseline,
            // raw zero bits in 18-31 decode as -128 (full deflection), which
            // process_analogue_movement interprets as "stick fully zenged" →
            // character forced into STATE_MOVEING / ACTION_RUN → action_run
            // table maps PUNCH → ACTION_SHOOT → no gun → silent no-op
            // (the click-doesn't-punch bug).
            //
            // Previously the gamepad block above provided this baseline
            // (always packing CENTER even when stick was at rest), but with
            // gamepad now gated by active_input_device != KBM, that path no
            // longer fires when KBM is active — so KBM must provide its own
            // neutral baseline. Also covers the gamepad-disconnected case
            // identically: bits 18-31 always have a valid encoding.
            //
            // Clear bits 18-31 first to drop any stale gamepad packing from
            // a previous tick (in case active device was just switched).
            constexpr SLONG WASD_AXIS_CENTRE_PACKED = 64; // AXIS_CENTRE >> 9
            input &= 0x0003FFFF;
            input |= WASD_AXIS_CENTRE_PACKED << 18;
            input |= WASD_AXIS_CENTRE_PACKED << 25;
            analogue = 0;
        }

        // Keyboard L2 equivalent — Ctrl + WASD drive the SAME tactical-mode
        // machine as the gamepad's L2 + stick: tap Ctrl → roll toward the WASD
        // direction, hold Ctrl → slow-walk. Gated on KBM being the active device
        // so it never fights the gamepad path. WASD maps to a camera-frame stick
        // offset; only the 8-way direction and "anything held" matter to the
        // machine, so a fixed full deflection per held axis is fine.
        if (active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE) {
            const bool ctrl_engaged = input_key_held(ACT_FOOT_TACTICAL_MODE_KKEY);
            const SLONG kb_vx = (kb_right ? 1 : 0) - (kb_left ? 1 : 0);
            const SLONG kb_vy = (kb_back ? 1 : 0) - (kb_fwd ? 1 : 0);
            constexpr SLONG KB_DEFLECTION = 32768; // full per-axis (well past deadzone)
            const SLONG dx_c = kb_vx * KB_DEFLECTION;
            const SLONG dy_c = kb_vy * KB_DEFLECTION;
            const bool stick_in_deadzone = (kb_vx == 0 && kb_vy == 0);
            process_l2_tactical(ctrl_engaged, dx_c, dy_c, stick_in_deadzone, NET_PERSON(0));

            // Zoom / back-walk modifier on KBM = E key (ACT_FOOT_AIM_KKEY): held
            // from a standstill = zoom (mouse rotates), held + WASD = back-walk.
            // The keyboard counterpart of the gamepad's L1. Gated by
            // input_gameplay_enabled() so F1-debug suppresses it (matches the
            // first-person zoom pose gate).
            const bool e_held = input_gameplay_enabled() && input_key_held(ACT_FOOT_AIM_KKEY);
            process_zoomwalk(e_held, dx_c, dy_c, stick_in_deadzone, NET_PERSON(0));
        }

        // Tanky-arrow reference implementation — kept here commented in
        // case anyone wants to bring back the old PC control scheme as an
        // opt-in setting. Not on the roadmap. Original Shift+arrow behavior
        // was strafe (STEP_LEFT/RIGHT) vs unmodified arrow = tank turn
        // (LEFT/RIGHT).
        //
        // const bool kb_tank_fwd   = input_key_held(ACT_FOOT_MOVE_TANK_FORWARD_KKEY);
        // const bool kb_tank_back  = input_key_held(ACT_FOOT_MOVE_TANK_BACKWARD_KKEY);
        // const bool kb_tank_left  = input_key_held(ACT_FOOT_MOVE_TANK_TURN_LEFT_KKEY);
        // const bool kb_tank_right = input_key_held(ACT_FOOT_MOVE_TANK_TURN_RIGHT_KKEY);
        //
        // if (kb_tank_fwd || kb_tank_back || kb_tank_left || kb_tank_right) {
        //     analogue = 0;
        //     input &= 0x0003FFFF;
        // }
        //
        // if (kb_tank_fwd)  input |= INPUT_MASK_FORWARDS;
        // if (kb_tank_back) input |= INPUT_MASK_BACKWARDS;
        // if (kb_tank_left) {
        //     if (ShiftFlag) input |= INPUT_MASK_STEP_LEFT;
        //     else           input |= INPUT_MASK_LEFT;
        // }
        // if (kb_tank_right) {
        //     if (ShiftFlag) input |= INPUT_MASK_STEP_RIGHT;
        //     else           input |= INPUT_MASK_RIGHT;
        // }

        if (input_key_held(ACT_FOOT_INVENTORY_KKEY))
            input |= INPUT_MASK_SELECT;

        if (input_key_held(ACT_FOOT_JUMP_KKEY))
            input |= INPUT_MASK_JUMP;

        // LMB = punch (R2-trigger equivalent), RMB = kick (R1 equivalent).
        // Digital reads: trigger-pull strength is not modeled on mouse —
        // weapon_feel_evaluate_fire is bypassed on this path (same as the
        // historical keyboard Z punch behavior). For melee that's fine;
        // for gun fire the weapon's own cyclic-rate gating applies.
        if (input_mouse_btn_held(ACT_FOOT_PUNCH_MBTN))
            input |= INPUT_MASK_PUNCH;
        if (input_mouse_btn_held(ACT_FOOT_KICK_MBTN))
            input |= INPUT_MASK_KICK;

        // F = USE (interaction multitool), Shift = SPRINT (hold while running),
        // C = STEALTH (hold to crouch). See act_foot.h.
        if (input_key_held(ACT_FOOT_USE_KKEY))
            input |= INPUT_MASK_ACTION;
        if (input_key_held(ACT_FOOT_SPRINT_KKEY))
            input |= INPUT_MASK_SPRINT;
        if (input_key_held(ACT_FOOT_STEALTH_KKEY))
            input |= INPUT_MASK_STEALTH;
    }

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

// uc_orig: pre_process_input (fallen/Source/interfac.cpp)
// Hook for remapping input before dispatching. Currently a pass-through (all remapping
// code is commented out in the original).
ULONG pre_process_input(SLONG mode, ULONG input)
{
    return (input);
}

// uc_orig: apply_button_input_first_person (fallen/Source/interfac.cpp)
// Handles first-person look-around mode when the 1st-person hotkey is held
// (KKEY_A on keyboard / L1 on gamepad).
// Rotates the camera pitch and character angle without consuming movement input.
// Returns non-zero if first-person mode is active this frame.
ULONG apply_button_input_first_person(Thing* p_player, Thing* p_person, ULONG input, ULONG* processed)
{
    static SLONG look_ami = UC_FALSE;
    SLONG fpm = UC_FALSE;

    *processed = 0;

    // Zoom/look pose is active only while the zoom-walk modifier is in ZOOM mode
    // (see process_zoomwalk, driven from get_hardware_input). The machine enters
    // ZOOM only from a standstill with a neutral stick; the instant the stick is
    // touched it latches to SLOWWALK, fpm goes false here, and the normal
    // movement dispatcher runs the slow camera-relative step instead. The held
    // state of the modifier (incl. the input_gameplay_enabled gate for MMB) is
    // already folded into g_zoomwalk_mode by the machine.
    if (g_zoomwalk_mode == ZOOMWALK_ZOOM) {
        fpm = UC_TRUE;
    }

    if (p_person->State != STATE_IDLE && p_person->State != STATE_GUN && p_person->State != STATE_NORMAL && p_person->State != STATE_HIT_RECOIL) {
        fpm = UC_FALSE;
    }

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

        // Zoom-mode look is driven by the RIGHT stick (gamepad) and the mouse
        // (KBM) only. The LEFT stick / WASD does NOT look here: touching it
        // leaves zoom for the slow-walk mode (process_zoomwalk), so zoom is
        // always a rooted pose with a neutral movement stick. (The
        // INPUT_MASK_MOVE clear further down keeps the pose rooted for the tick
        // the transition is detected.)
        //
        // Vertical inverted to match the non-aim camera convention: in non-
        // aim mode right-stick UP raises the orbital camera (so the view
        // ends up looking DOWN at the character). Here right-stick UP likewise
        // pitches the view DOWN. Same intuition across both modes — toggling
        // aim on/off doesn't flip the up/down feel.
        {
            constexpr SLONG STICK_DEAD = 8000;
            // Zoom-look stick rates (per-frame at full deflection): track the
            // gamepad sensitivity and run at ZOOM_LOOK_RATIO of the normal
            // orbit speed. Defined in camera/look_sensitivity.h.
            const SLONG STICK_PITCH_MAX = zoom_stick_pitch();
            const SLONG STICK_YAW_MAX = zoom_stick_yaw();

            // Gamepad sticks — only the RIGHT stick drives yaw + pitch in the
            // zoom/look pose. The LEFT stick is reserved for movement: touching
            // it leaves zoom for the slow-walk mode (process_zoomwalk), so
            // it must NOT also crank the look here — that was the duplicate
            // rotation we removed. (Pre-rework the left stick was a second look
            // source via a per-axis fallback; that is gone now.)
            //
            // Only consume the stick while a controller is the active input
            // device, so an idle stick on a connected controller doesn't fight
            // keyboard input.
            if (active_input_device != INPUT_DEVICE_KEYBOARD_MOUSE && input_gamepad_connected()) {
                const SLONG sx = (SLONG)input_stick_x_axis(ACT_FOOT_AIM_LOOK_GAXIS) - 32768;
                const SLONG sy = (SLONG)input_stick_y_axis(ACT_FOOT_AIM_LOOK_GAXIS) - 32768;

                if (abs(sy) > STICK_DEAD) {
                    // sy > 0 (stick down) → look UP   (look_pitch += step)
                    // sy < 0 (stick up)   → look DOWN (look_pitch -= step)
                    // Vertical invert from config (gamepad camera_orbit_invert_y).
                    look_pitch += ((CAM_gamepad_invert_y() ? -sy : sy) * STICK_PITCH_MAX) / 32767;
                }
                if (!CONTROLS_inventory_mode && abs(sx) > STICK_DEAD) {
                    // sx > 0 (stick right) → turn character right (angle -=)
                    // sx < 0 (stick left)  → turn character left  (angle +=)
                    SLONG ang_step = (sx * STICK_YAW_MAX) / 32767;
                    p_person->Draw.Tweened->Angle = (p_person->Draw.Tweened->Angle - ang_step) & 2047;
                }
            }

            // Mouse-look in aim (KBM): mouse motion rotates the character
            // (yaw) and pitches the look — the mouse counterpart of the right
            // stick above. CONSUMING the mouse here also drains the relative-
            // motion accumulator so it can't pile up while aiming and burst out
            // as a camera jerk the moment aim is released (the non-aim camera
            // mouse-orbit in fc.cpp is gated off while toonear). The camera
            // then follows the new facing via FC_position_for_lookaround + the
            // camera-mode angle smoothing — snappy in MANUAL, rubberness-
            // smoothed in AUTO — so it matches normal mouse-look.
            if (active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE
                && input_gameplay_enabled() && mouse_capture_is_active()) {
                SLONG mdx = 0, mdy = 0;
                input_mouse_consume_rel(&mdx, &mdy);
                // Zoom-look mouse rates (Q8 game-units per pixel): track the
                // mouse sensitivity and run at ZOOM_LOOK_RATIO of the normal
                // orbit speed. Defined in camera/look_sensitivity.h.
                const SLONG MOUSE_AIM_YAW_Q8 = zoom_mouse_yaw_q8();
                const SLONG MOUSE_AIM_PITCH_Q8 = zoom_mouse_pitch_q8();
                // Yaw: mouse right → turn right (Angle decreases) — same sign
                // as the right stick above.
                p_person->Draw.Tweened->Angle = (p_person->Draw.Tweened->Angle - (mdx * MOUSE_AIM_YAW_Q8 / 256)) & 2047;
                // Pitch: vertical invert from config (mouse camera_orbit_invert_y),
                // matching the non-aim mouse-orbit convention.
                look_pitch += ((CAM_mouse_invert_y() ? -mdy : mdy) * MOUSE_AIM_PITCH_Q8 / 256);
            }
        }

        // Aim mode is a rooted pose — character pivots in place but doesn't
        // walk. Both LEFT-stick forward and the Up arrow set INPUT_MASK_MOVE
        // upstream in get_hardware_input; without this clear, holding either
        // would kick set_person_running below and the player would jog
        // forward while trying to aim. Original game behaviour was rooted
        // (the pre-rework code did the same MASK clear, but only on the
        // FORWARDS branch — moving the clear out makes it independent of
        // which look-input source is active).
        input &= ~INPUT_MASK_MOVE;

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

                } else {
                    set_player_shoot(p_person, 0);
                    *processed |= INPUT_MASK_PUNCH;
                }
            }
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
    // Modal input debug panel — drive the dispatcher with input=0 so the
    // state machine transitions the character into idle (continue_moveing
    // returns 0, jump→fall→land terminates as idle, etc.) instead of
    // freezing whatever bits the player was holding before the panel
    // opened. Then bail out before the rest of the per-tick logic so
    // the panel stays modal (no first-person aim, no camera switching,
    // no inventory / weapon hotkeys process from this dispatcher).
    if (input_debug_is_active()) {
        if (p_player && p_player->Genus.Player) {
            Thing* p_person = p_player->Genus.Player->PlayerPerson;
            if (p_person && p_person->Genus.Person) {
                apply_button_input(p_player, p_person, 0);
            } else {
                p_player->Genus.Player->Input = 0;
            }
        }
        return;
    }

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
        // Weapon switching (keyboard 1-4 + Tab melee, MMB disarm, mouse wheel
        // scroll, gamepad D-pad / R3) is handled centrally in
        // game_tick.cpp::process_controls — not here.

        if (!apply_button_input_first_person(p_player, p_person, input, &processed)) {

            if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                ASSERT(p_person->Genus.Person->InCar);

                processed = apply_button_input_car(TO_THING(p_person->Genus.Person->InCar), input);
                processed |= apply_button_input(p_player, p_person, 0);
            } else {
                // Not driving — drain the car-siren press_pending flags every
                // physics tick so that a press of these buttons outside the
                // car (Triangle on foot, SPACE = jump) doesn't leak into the
                // first car-entry tick as a spurious siren toggle. SPACE is
                // dual-use (jump on foot), Triangle on foot is currently
                // unbound but the input layer still sets press_pending on
                // every physical press. apply_button_input_car is the only
                // consumer of these flags — without this drain, the flag
                // would sit set indefinitely from any outside press.
                input_key_consume(ACT_CAR_SIREN_KKEY);
                input_btn_consume(ACT_CAR_SIREN_GBTN);
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

// Reload gate: set when AK47 magazine auto-swaps on an empty-clip press
// (HAD_TO_CHANGE_CLIP). While set, the player's auto-fire re-entry paths
// (SUB_STATE_AIM_GUN, SUB_STATE_SHOOT_GUN re-fire, fn_person_moveing)
// will not fire the next shot even if PUNCH is still held — enforces
// "release R2 and press again" between reload and the next shot.
// Cleared when (a) a real shot actually fires — via clear from the
// person.cpp fire paths after actually_fire_gun, or (b) the player
// physically releases R2 (r2 <= reset_threshold) — via clear from
// weapon_feel_evaluate_fire. We do NOT clear on PUNCH bit low: the
// act-bit reload-press path in weapon_feel only raises PUNCH on the
// rising-edge tick, so a low-PUNCH check the very next tick while the
// trigger is still held would wrongly clear the gate and let auto-
// fire kick in.
static bool s_ak47_reload_gate = false;

void input_actions_mark_ak47_reload_gate() { s_ak47_reload_gate = true; }
void input_actions_clear_ak47_reload_gate() { s_ak47_reload_gate = false; }
bool input_actions_ak47_reload_gate_set() { return s_ak47_reload_gate; }

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
            if (s_ak47_reload_gate)
                return UC_FALSE;
            return UC_TRUE;
        } else {
            // PUNCH bit low — just report "not firing". We do NOT clear
            // the reload gate here: in the act-bit reload-press path the
            // PUNCH bit is edge-triggered (only set on the rising-edge
            // tick), so a low PUNCH the very next tick while the trigger
            // is still physically held would wrongly clear the gate and
            // let auto-fire kick in. Gate is cleared from evaluate_fire
            // on physical R2 release (r2 <= reset_threshold) instead.
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

            dx = abs((SLONG)input_virtual_axis(input, ACT_FOOT_MOVE_X_VAXIS));
            dy = abs((SLONG)input_virtual_axis(input, ACT_FOOT_MOVE_Y_VAXIS));

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

// ---- Cheat helpers ----------------------------------------------------------
// Ported from Dreamcast cheats (fallen/Source/interfac.cpp). Each helper
// performs the gameplay effect AND prints the corresponding on-screen
// message — same behavior whether triggered by the gamepad combo
// (Select+L1+L2+DPad) or by the keyboard F9-console command.

void cheat_apply_immortal_toggle()
{
    Thing* p_darci = NET_PERSON(0);
    if (!p_darci || !p_darci->Genus.Person)
        return;
    p_darci->Genus.Person->Flags2 ^= FLAG2_PERSON_INVULNERABLE;
    if (p_darci->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE)
        CONSOLE_text_en((CBYTE*)"I am immortal, I have inside me blood of kings.");
    else
        CONSOLE_text_en((CBYTE*)"I'm just a mortal after all.");
}

void cheat_apply_full_health()
{
    Thing* p_darci = NET_PERSON(0);
    if (!p_darci || !p_darci->Genus.Person)
        return;
    // 1000 HP — same value as the original Dreamcast cheat. Darci's normal
    // max is 100 (gameplay HP scale) so 1000 is effectively "you can't die
    // from anything that hits you in one shot".
    constexpr SLONG CHEAT_FULL_HEALTH_HP = 1000;
    p_darci->Genus.Person->Health = CHEAT_FULL_HEALTH_HP;
    CONSOLE_text_en((CBYTE*)"My wings are as a shield of steel.");
}

void cheat_apply_spawn_weapons()
{
    Thing* p_darci = NET_PERSON(0);
    if (!p_darci)
        return;
    // Spawn ring of weapons around the player: AK47, shotgun, pistol, 3
    // grenades. Each weapon offsets by CHEAT_RING_SIZE world-units from
    // Darci's position so the player doesn't auto-pick them up immediately
    // and can choose what to grab.
    constexpr SLONG CHEAT_RING_SIZE = 128;
    const SLONG px = p_darci->WorldPos.X >> 8;
    const SLONG py = p_darci->WorldPos.Y >> 8;
    const SLONG pz = p_darci->WorldPos.Z >> 8;
    alloc_special(SPECIAL_AK47, SPECIAL_SUBSTATE_NONE, px + CHEAT_RING_SIZE, py, pz + CHEAT_RING_SIZE, 0);
    alloc_special(SPECIAL_SHOTGUN, SPECIAL_SUBSTATE_NONE, px - CHEAT_RING_SIZE, py, pz - CHEAT_RING_SIZE, 0);
    alloc_special(SPECIAL_GUN, SPECIAL_SUBSTATE_NONE, px - CHEAT_RING_SIZE, py, pz + CHEAT_RING_SIZE, 0);
    alloc_special(SPECIAL_GRENADE, SPECIAL_SUBSTATE_NONE, px + CHEAT_RING_SIZE - 32, py, pz - CHEAT_RING_SIZE - 32, 0);
    alloc_special(SPECIAL_GRENADE, SPECIAL_SUBSTATE_NONE, px + CHEAT_RING_SIZE, py, pz - CHEAT_RING_SIZE, 0);
    alloc_special(SPECIAL_GRENADE, SPECIAL_SUBSTATE_NONE, px + CHEAT_RING_SIZE + 32, py, pz - CHEAT_RING_SIZE + 32, 0);
    CONSOLE_text_en((CBYTE*)"We need guns. Lots of guns.");
}

void cheat_apply_max_ammo()
{
    Thing* p_darci = NET_PERSON(0);
    if (!p_darci || !p_darci->Genus.Person)
        return;
    // 240 ammo packs each — original Dreamcast cheat value, well past any
    // realistic mission requirement.
    constexpr SLONG CHEAT_MAX_AMMO = 240;
    p_darci->Genus.Person->ammo_packs_pistol = CHEAT_MAX_AMMO;
    p_darci->Genus.Person->ammo_packs_shotgun = CHEAT_MAX_AMMO;
    p_darci->Genus.Person->ammo_packs_ak47 = CHEAT_MAX_AMMO;
    CONSOLE_text_en((CBYTE*)"Well, to tell you the truth, in all this excitement, I've kinda lost track myself.");
}
