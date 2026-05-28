// Camera and player control logic.
#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/characters/anim_ids.h"
#include "engine/core/fmatrix.h"
#include "buildings/ware.h"
#include "buildings/ware_globals.h"
#include "things/core/statedef.h"
#include "missions/eway.h"
#include "map/level_pools.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "camera/camera_mode.h" // get_active_camera_mode, keep_for_rubberness, camera_is_*
#include "camera/cam_trig.h"    // cam_arctan_psx_fp8 — high-precision atan for FC_look_at_focus
#include "engine/input/gamepad.h" // gamepad_set_shock
#include "engine/input/gamepad_globals.h" // active_input_device
#include "engine/input/input_frame.h" // input_gamepad_connected, input_stick_*_axis, input_mouse_consume_rel
#include "engine/input/mouse_capture.h" // mouse_capture_is_active
#include "game/input_actions.h" // input_l2_is_held (suppress get-behind while L2 held)
#include "assets/formats/anim_globals.h" // next_prim_face4 (for ASSERTs)
#include "navigation/wmove.h" // WMOVE_face for rigid platform inheritance

// Cutscene gates — these globals live in other subsystems and we only read
// them. Local extern declarations avoid pulling EWAY / PLAYCUTS headers
// into the camera layer for two flags. Used by FC_process to freeze the
// camera state at the script-set position while a cut-scene is playing,
// so EWAY's separate framing (EWAY_cam_*) owns the visible camera and
// FC_cam is restored unchanged when gameplay resumes.
extern SLONG EWAY_cam_active;
extern BOOL PLAYCUTS_playing;

// CAM_MORE_IN: PC camera is 25% closer to the player than the PSX version.
// Applied to cam_dist and camera height offsets.
// uc_orig: CAM_MORE_IN (fallen/Source/fc.cpp)
#define CAM_MORE_IN (0.75F)

// Mouse-driven camera orbit sensitivity (OpenChaos -- no original).
//
// MOUSE_SENSITIVITY: 0..100 (= 0.0..1.0 user slider, just in percent
//   so it stays integer). Maps linearly between two compile-time
//   bounds expressed in Q8 fixed-point game-angle units per pixel:
//     0   -> MOUSE_SENS_MIN_Q8 (slow / comfortable feel)
//     100 -> MOUSE_SENS_MAX_Q8 (fast / original default)
//   When the settings UI lands, this becomes a runtime variable;
//   the conversion stays the same (slider * 100 -> this value).
// MOUSE_HEIGHT_SCALE (per-pixel height delta) is fully derived below
//   from MOUSE_YAW_SCALE_Q8 using the right stick's height-to-yaw
//   ratio, so vertical and horizontal mouse motion feel proportionally
//   consistent across sensitivity values, and the proportion itself
//   matches the right stick.
// MOUSE_INVERT_Y: 1 = mouse-up lowers the camera (FPS-style "inverted
//   look"), 0 = mouse-up raises the camera. Mouse only -- the right
//   stick keeps its own convention.
// MOUSE_ORBIT_LAG: residual yaw lag (game-angle units) retained for the
//   "rubber band" smoothing tail. Capped here because mouse input is
//   unbounded -- values above ~60 start to expose chord shrinkage in
//   the position smoothing for sustained fast flicks.
// STICK_ORBIT_LAG: same idea for the right stick. Stick input is
//   bounded to ~61 game-angle units per tick at full deflection, so
//   setting this to 61 means the rotation path is identical to the
//   legacy gamepad feel (no sync, full smoothing tail) while still
//   using exact rotation math (no distance inflation).
#define MOUSE_SENS_MIN_Q8   51     // ~0.2 game-units/px
#define MOUSE_SENS_MAX_Q8   768    // ~3.0 game-units/px
#define MOUSE_SENSITIVITY   22     // 0..100, see comment above.
#define MOUSE_INVERT_Y      1
// Residual angular lag the rotation path leaves unsynced each tick,
// closed afterwards by the standard position+angle smoothing. Higher =
// more "rubber band" tail; lower = snappier.
//
// These constants are the SHIPPING DEFAULTS — i.e. the residual at
// rubberness 0.5 (preserves the legacy gamepad rubber feel). At runtime
// the actual residual is scaled by `orbit_lag_for_rubberness(default)`:
//   rubberness 0.0  → 0   (snap, no residual)
//   rubberness 0.5  → 61  (shipping)
//   rubberness 1.0  → 244 (4× residual — distinctly laggier)
//
// Only used in AUTO camera mode. MANUAL mode does full snap on both
// want and fc (no residual) — see the mdx block in FC_process and the
// stick-X block below for the gated paths.
//
// 61 game-angle units ≈ stick deflection at full per-tick, so at the
// shipping default the stick effectively contributes 0 "sync" while
// leaving the entire motion to be smoothed by the position/angle pass.
// Mouse uses the same default value so AUTO + mouse and AUTO + gamepad
// feel identical at rubberness 0.5.
#define MOUSE_ORBIT_LAG     61
#define STICK_ORBIT_LAG     61

// Camera Y range, relative to focus_y (the character's vertical position).
// Same numbers for mouse and gamepad — the camera-elevation range must
// not depend on which input device is active. Lower bound is kept well
// above the character's feet so the camera doesn't dip into ground
// geometry and trigger the wall-collision system on the floor (whose
// EXIT fade then produces a visible jerk on liftoff).
#define FC_CAM_Y_MIN_ABOVE_FOCUS  0x6000   // ~3/4 m above feet
#define FC_CAM_Y_MAX_ABOVE_FOCUS  0x28000  // ~5 m above feet

// Compile-time resolved scales. When MOUSE_SENSITIVITY becomes a
// runtime variable, lift these inline into the call site.
#define MOUSE_YAW_SCALE_Q8 \
    (MOUSE_SENS_MIN_Q8 + (MOUSE_SENS_MAX_Q8 - MOUSE_SENS_MIN_Q8) * MOUSE_SENSITIVITY / 100)

// Height scale derived from yaw scale using the right stick's own
// height/yaw ratio: STICK_HEIGHT_MAX (0x3100, height-units per tick at
// full stick deflection) / STICK_YAW_MAX (61, game-angle units per
// tick at full stick deflection) ~ 205.6 height-units per game-unit.
// Multiplied by per-pixel yaw scale in game-units (= MOUSE_YAW_SCALE_Q8
// / 256) gives per-pixel height scale that mirrors the stick feel at
// any sensitivity. Result for sens=100: ~617 (vs the previous 1024,
// which made vertical feel too aggressive relative to horizontal).
#define MOUSE_HEIGHT_SCALE \
    (MOUSE_YAW_SCALE_Q8 * 0x3100 / (61 * 256))

// OpenChaos: master switch for the vehicle enter/exit camera "scene override"
// (programmatic lock-rotation behind the car + manual-input block during the
// enter/exit animations). TEMPORARILY false so the animations can be watched
// with a free camera while tuning the exit reverse-play. Set back to true to
// re-enable the lock for BOTH entering and exiting.
// uc_orig: n/a (OpenChaos addition)
static constexpr bool CAR_ENTER_EXIT_CAM_LOCK = false;

// uc_orig: CAM_AT_HEAD (fallen/Source/fc.cpp)
#define CAM_AT_HEAD 1
// uc_orig: CAM_AT_WORLD_POS (fallen/Source/fc.cpp)
#define CAM_AT_WORLD_POS 2
// uc_orig: CAM_AT_FEET (fallen/Source/fc.cpp)
#define CAM_AT_FEET 3

// uc_orig: FC_CSP_HEIGHT (fallen/Source/fc.cpp)
#define FC_CSP_HEIGHT 0xa0
// uc_orig: FC_CSP_RADIUS (fallen/Source/fc.cpp)
#define FC_CSP_RADIUS 0x30

// uc_orig: FC_DIST_MIN (fallen/Source/fc.cpp)
#define FC_DIST_MIN ((fc->cam_dist * offset_dist))
// uc_orig: FC_DIST_MAX (fallen/Source/fc.cpp)
#define FC_DIST_MAX ((fc->cam_dist * offset_dist))

extern UBYTE GAME_cut_scene;
extern SLONG analogue;

// uc_orig: person_has_gun_out (fallen/Source/fc.cpp)
extern SLONG person_has_gun_out(Thing* p_person);

// uc_orig: calc_sub_objects_position (fallen/Source/fc.cpp)
// Not declared in any header — forward declaration needed.
extern void calc_sub_objects_position(
    Thing* p_mthing,
    SLONG tween,
    UWORD object,
    SLONG* x,
    SLONG* y,
    SLONG* z);

// uc_orig: EWAY_cam_jumped (fallen/Source/fc.cpp)
extern SLONG EWAY_cam_jumped;
// Note: EWAY_grab_camera declared in fallen/Headers/eway.h (returns SLONG)

// Adjusts the camera height offset (dheight) and distance (ddist) based on the
// focus character's current state: gun-out, driving a vehicle, or idle walking.
// Gun-out: camera moves closer and lower (dheight=0, ddist=200).
// In a car: dheight=0, ddist=356. Otherwise: ddist=256 unless idle.
// uc_orig: FC_alter_for_pos (fallen/Source/fc.cpp)
SLONG FC_alter_for_pos(FC_Cam* fc, SLONG* dheight, SLONG* ddist)
{
    if (person_has_gun_out(fc->focus) && !(fc->focus->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING))) {
        *dheight = 0;
        *ddist = 200;
        return 0;
    }
    if (fc->focus->Genus.Person->InCar) {
        switch (TO_THING(fc->focus->Genus.Person->InCar)->Genus.Vehicle->Type) {
        case 0:
        case 4:
        case 5:
        case 6:
        case 8:
            *dheight = 0;
            *ddist = 356;
            break;
        default:
            *dheight = 0;
            *ddist = 356;
            break;
        }
        return (0);
    }

    if (fc->focus->State != STATE_IDLE) {
        *dheight = 0;
        *ddist = 256;
        return (0);
    }

    *dheight = 0;
    *ddist = 256;

    return (0);
}

// uc_orig: FC_init (fallen/Source/fc.cpp)
// OpenChaos: a fight-mode target change triggers ONE smooth get-behind
// swing that stays active this many GAME_TURN ticks -- enough for the
// existing get-behind smoothing to complete even a full 180, after
// which the fight camera goes static again until the next change.
#define FC_FIGHT_BEHIND_TURNS (UC_PHYSICS_DESIGN_HZ * 3 / 2) // ~1.5s
// Manual camera rotation locks BOTH triggers (target change and the
// periodic inactivity one) out for this long, so the camera doesn't
// fight the player right after they moved it by hand.
#define FC_FIGHT_BEHIND_LOCK_TURNS (UC_PHYSICS_DESIGN_HZ * 3) // 3s
// While fighting (and outside the lockout), re-arm the get-behind
// swing this often even without a target change -> the camera keeps
// drifting back behind Darci ~this fast when the enemy circles around.
#define FC_FIGHT_AUTO_PERIOD UC_PHYSICS_DESIGN_HZ // 1s

void FC_init(void)
{
    SLONG i;

    memset(FC_cam, 0, sizeof(FC_Cam) * FC_MAX_CAMS);

    for (i = 0; i < FC_MAX_CAMS; i++) {
        FC_cam[i].lens = 0x24000;
        FC_cam[i].cam_dist = 0x280 * CAM_MORE_IN;
        FC_cam[i].cam_height = 0x16000;
        // Start with the manual-rotation lockout long expired so the
        // very first fight at level start can still trigger.
        FC_cam[i].last_manual_cam_turn = -FC_FIGHT_BEHIND_LOCK_TURNS - 1;
    }

    GAME_cut_scene = 0;
}

// Switch between 4 camera distance/height presets.
// Type 0 (default): dist=0x280*0.75, height=0x16000.
// Type 1: wider view. Type 2: even wider. Type 3: low cinematic angle.
// uc_orig: FC_change_camera_type (fallen/Source/fc.cpp)
void FC_change_camera_type(SLONG cam, SLONG cam_type)
{
    switch (cam_type) {
    case 0:
        FC_cam[cam].cam_dist = 0x280 * CAM_MORE_IN;
        FC_cam[cam].cam_height = 0x16000;
        break;
    case 1:
        FC_cam[cam].cam_dist = 0x280;
        FC_cam[cam].cam_height = 0x20000;
        break;
    case 2:
        FC_cam[cam].cam_dist = 0x380;
        FC_cam[cam].cam_height = 0x25000;
        break;
    case 3:
        FC_cam[cam].cam_dist = 0x300;
        FC_cam[cam].cam_height = 0x8000;
        break;
    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: FC_look_at (fallen/Source/fc.cpp)
void FC_look_at(SLONG cam, UWORD thing_index)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    fc->focus = TO_THING(thing_index);
    GAME_cut_scene = 0;
}

// Returns the desired height of the camera above the focus point.
// Adjusts based on: gun-out (lower), vehicle type, warehouse (adds 50%), split-screen (subtracts).
// uc_orig: FC_focus_above (fallen/Source/fc.cpp)
SLONG FC_focus_above(FC_Cam* fc)
{
    SLONG above;
    SLONG lower = 0;

    if (fc->focus->Class == CLASS_PERSON && person_has_gun_out(fc->focus)) {
        lower = 0xa000;
    }

    if (fc->focus->Genus.Person->InCar) {
        switch (TO_THING(fc->focus->Genus.Person->InCar)->Genus.Vehicle->Type) {
        case 4:
            lower = -(0x3000) * CAM_MORE_IN;
            break;
        case 0:
        case 5:
        case 6:
        case 8:
            lower = -(0x1000) * CAM_MORE_IN;
            break;
        default:
            lower = 0xa000 * CAM_MORE_IN;
            break;
        }
    }

    if (fc->focus->Class == CLASS_PERSON && fc->focus->Genus.Person->InsideIndex) {
        above = fc->cam_height + (fc->cam_height >> 1);
    } else {
        above = fc->cam_height;

        if (FC_cam[1].focus) {
            // Split-screen: camera sits lower because the viewport is narrower.
            above -= 0x4000;
            if (above < 0x1000)
                above = 0x1000;
        }
        above -= lower;
    }

    return above;
}

// Returns which body part of a person the camera should target.
// Dangling/climbing → feet; entering vehicle/dead → head; default → world position.
// uc_orig: FC_get_person_body_part_target (fallen/Source/fc.cpp)
SLONG FC_get_person_body_part_target(Thing* p_thing)
{
    switch (p_thing->Genus.Person->Action) {
    case ACTION_DANGLING:
    case ACTION_PULL_UP:
    case ACTION_CLIMBING:
        return (CAM_AT_FEET);

    case ACTION_HOP_BACK:
    case ACTION_SIDE_STEP:
    case ACTION_FLIP_LEFT:
    case ACTION_FLIP_RIGHT:
    case ACTION_DEAD:
    case ACTION_DYING:
    case ACTION_ENTER_VEHICLE:
        return (CAM_AT_HEAD);

    default:
        switch (p_thing->SubState) {
        case SUB_STATE_RUNNING_VAULT:
        case SUB_STATE_RUNNING_HALF_BLOCK:
            return (CAM_AT_FEET);
            break;
        default:
            return (CAM_AT_WORLD_POS);
        }
        break;
    }
    return (CAM_AT_WORLD_POS);
}

// Calculates the world position (focus_x/y/z) and yaw (focus_yaw) that the camera looks at.
// For persons: adjusts yaw based on driving/entering vehicle/hugging wall.
// For vehicles: smoothly tracks vehicle yaw.
// focus_x/y/z is set from head, feet, or world position depending on action.
// uc_orig: FC_calc_focus (fallen/Source/fc.cpp)
void FC_calc_focus(FC_Cam* fc)
{
    SLONG body_part = 0;

    fc->focus_in_warehouse = NULL;

    switch (fc->focus->Class) {
    case CLASS_PERSON:

        if (fc->focus->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
            Thing* p_vehicle = TO_THING(fc->focus->Genus.Person->InCar);

            SLONG yaw_car;

            yaw_car = p_vehicle->Genus.Vehicle->Angle;
            yaw_car -= p_vehicle->Genus.Vehicle->WheelAngle * p_vehicle->Velocity >> 10;

            if (p_vehicle->Velocity < 0) {
                yaw_car += 1024;
            }

            fc->focus_yaw = yaw_car & 2047;
            fc->platform_thing = 0;
        } else {
            fc->focus_yaw = fc->focus->Draw.Tweened->Angle;

            // Detect "standing on a moving vehicle" so FC_process can
            // rigidly inherit the vehicle's motion onto the camera (keeps
            // the camera glued to the vehicle's reference frame).
            UBYTE on_vehicle_platform = UC_FALSE;
            if (fc->focus->OnFace > 0) {
                ASSERT(WITHIN(fc->focus->OnFace, 1, next_prim_face4 - 1));
                PrimFace4* f4 = &prim_faces4[fc->focus->OnFace];
                if (f4->FaceFlags & FACE_FLAG_WMOVE) {
                    SLONG wmove_index = f4->ThingIndex;
                    ASSERT(WITHIN(wmove_index, 1, WMOVE_face_upto - 1));
                    WMOVE_Face* wf = &WMOVE_face[wmove_index];
                    if (wf->thing && TO_THING(wf->thing)->Class == CLASS_VEHICLE) {
                        Thing* p_vehicle = TO_THING(wf->thing);
                        if (fc->platform_thing != wf->thing) {
                            // Just boarded — snapshot vehicle pose; first
                            // tick produces zero delta in FC_process.
                            fc->platform_thing = wf->thing;
                            fc->platform_last_x = p_vehicle->WorldPos.X;
                            fc->platform_last_z = p_vehicle->WorldPos.Z;
                            fc->platform_last_yaw = p_vehicle->Genus.Vehicle->Angle;
                        }
                        on_vehicle_platform = UC_TRUE;
                    }
                }
            }
            if (!on_vehicle_platform) {
                fc->platform_thing = 0;
            }

            if (fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE) {
                if (fc->focus->Draw.Tweened->CurrentAnim == ANIM_ENTER_TAXI) {
                    fc->focus_yaw -= 712;
                    fc->focus_yaw &= 2047;
                } else {
                    fc->focus_yaw += 712;
                    fc->focus_yaw &= 2047;
                }
            } else if (fc->focus->Genus.Person->Action == ACTION_SIDE_STEP || fc->focus->Genus.Person->Action == ACTION_SIT_BENCH || fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE || fc->focus->Genus.Person->Action == ACTION_HUG_WALL) {
                fc->focus_yaw += 1024;
                fc->focus_yaw &= 2047;
            }
        }

        if (fc->focus->Genus.Person->Flags & FLAG_PERSON_WAREHOUSE) {
            fc->focus_in_warehouse = fc->focus->Genus.Person->Ware;
        }

        if (GAME_FLAGS & GF_SIDE_ON_COMBAT) {
            fc->focus_yaw -= 512;
        }

        if (fc->focus->State == STATE_DANGLING) {
            if (fc->focus->SubState == SUB_STATE_DANGLING_CABLE_FORWARD || fc->focus->SubState == SUB_STATE_DANGLING_CABLE_BACKWARD || fc->focus->SubState == SUB_STATE_DEATH_SLIDE || fc->focus->SubState == SUB_STATE_TRAVERSE_LEFT || fc->focus->SubState == SUB_STATE_TRAVERSE_RIGHT || fc->focus->Genus.Person->Mode == PERSON_MODE_FIGHT || fc->focus->SubState == SUB_STATE_DANGLING_CABLE) {
                // Look from the side when dangling on cable.
                SLONG a1;
                SLONG a2;
                SLONG dangle;

                a1 = fc->focus_yaw;
                a2 = fc->yaw >> 8;

                dangle = a2 - a1;
                dangle &= 2047;

                if (dangle > 1024) {
                    dangle -= 2048;
                }

                if (dangle < 0) {
                    fc->focus_yaw -= 550;
                } else {
                    fc->focus_yaw += 550;
                }

                fc->focus_yaw &= 2047;
            }
        }

        break;

    case CLASS_VEHICLE: {
        SLONG dyaw;
        SLONG yaw_car;
        SLONG yaw_cam;

        yaw_car = fc->focus->Genus.Vehicle->Angle;
        yaw_cam = fc->yaw >> 8;

        dyaw = yaw_car - yaw_cam;

        if (dyaw > +1024) {
            dyaw -= 2048;
        }
        if (dyaw < -1024) {
            dyaw += 2048;
        }

        fc->focus_yaw = yaw_car + dyaw * 4;
        fc->focus_yaw &= 2047;
        fc->platform_thing = 0;
    } break;

    default:
        ASSERT(0);
        break;
    }

    switch (fc->focus->Class) {
    case CLASS_PERSON:

        body_part = FC_get_person_body_part_target(fc->focus);

        switch (body_part) {
        case CAM_AT_HEAD:
            calc_sub_objects_position(
                fc->focus,
                fc->focus->Draw.Tweened->AnimTween,
                SUB_OBJECT_HEAD,
                &fc->focus_x,
                &fc->focus_y,
                &fc->focus_z);

            fc->focus_x <<= 8;
            fc->focus_z <<= 8;

            fc->focus_x += fc->focus->WorldPos.X;
            fc->focus_y = fc->focus->WorldPos.Y;
            fc->focus_z += fc->focus->WorldPos.Z;
            break;

        case CAM_AT_FEET: {
            SLONG lfx, lfy, lfz;
            SLONG rfx, rfy, rfz;

            calc_sub_objects_position(
                fc->focus,
                fc->focus->Draw.Tweened->AnimTween,
                SUB_OBJECT_LEFT_FOOT,
                &lfx, &lfy, &lfz);

            calc_sub_objects_position(
                fc->focus,
                fc->focus->Draw.Tweened->AnimTween,
                SUB_OBJECT_RIGHT_FOOT,
                &rfx, &rfy, &rfz);

            fc->focus_x = lfx + rfx << 7;
            fc->focus_y = lfy + rfy << 7;
            fc->focus_z = lfz + rfz << 7;

            fc->focus_x += fc->focus->WorldPos.X;
            fc->focus_y += fc->focus->WorldPos.Y;
            fc->focus_z += fc->focus->WorldPos.Z;
        } break;

        case CAM_AT_WORLD_POS:
            fc->focus_x = fc->focus->WorldPos.X;
            fc->focus_y = fc->focus->WorldPos.Y;
            fc->focus_z = fc->focus->WorldPos.Z;
            break;
        }
        break;

    default:
        fc->focus_x = fc->focus->WorldPos.X;
        fc->focus_y = fc->focus->WorldPos.Y;
        fc->focus_z = fc->focus->WorldPos.Z;
        break;
    }

    // Peek left/right wall-hug states: shift focus_x/z in the look direction.
    if (fc->focus->SubState == SUB_STATE_HUG_WALL_LOOK_L) {
        SLONG angle, dx, dz;
        SLONG velocity;
        angle = fc->focus->Draw.Tweened->Angle - 512;
        velocity = fc->focus->Genus.Person->InsideRoom;
        angle &= 2047;
        dx = -(SIN(angle) * velocity) >> 4;
        dz = -(COS(angle) * velocity) >> 4;
        fc->focus_x += dx;
        fc->focus_z += dz;
    }
    if (fc->focus->SubState == SUB_STATE_HUG_WALL_LOOK_R) {
        SLONG angle, dx, dz;
        SLONG velocity;
        angle = fc->focus->Draw.Tweened->Angle + 512;
        velocity = fc->focus->Genus.Person->InsideRoom;
        angle &= 2047;
        dx = -(SIN(angle) * velocity) >> 4;
        dz = -(COS(angle) * velocity) >> 4;
        fc->focus_x += dx;
        fc->focus_z += dz;
    }
}

// Computes want_yaw/pitch/roll to aim the camera at the current focus point.
// In toonear mode: adjusts aim angles from the saved pre-collision position.
// When driving: adds vehicle roll based on wheel angle and velocity.
// uc_orig: FC_look_at_focus (fallen/Source/fc.cpp)
void FC_look_at_focus(FC_Cam* fc)
{
    SLONG dx = fc->focus_x - fc->want_x >> 8;
    SLONG dy = fc->focus_y + fc->lookabove - fc->want_y >> 8;
    SLONG dz = fc->focus_z - fc->want_z >> 8;

    if (fc->toonear && fc->toonear_dist != 0x90000) {
        SLONG dangle;
        SLONG angle;

        dangle = fc->focus_yaw - fc->toonear_focus_yaw;

        if (dangle > +1024) {
            dangle -= 2048;
        }
        if (dangle < -1024) {
            dangle += 2048;
        }
        if (dangle < -512) {
            dangle = -512 + (-512 - dangle);
        }
        if (dangle > 512) {
            dangle = 512 - (dangle - 512);
        }

        angle = fc->toonear_focus_yaw + dangle;
        angle &= 2047;

        dx -= SIN(angle) >> 9;
        dz -= COS(angle) >> 8;
    }

    // OpenChaos: was QDIST2 (fast |a|+|b|/2 approximation), which
    // has 3-11% error depending on the dx/dz ratio. As the camera
    // orbits around the focus, dx and dz swap dominance each tick,
    // so the QDIST2 result oscillates by ~40 units at constant true
    // distance. Through Arctan(dy, dxz) that translates into a
    // per-tick pitch swing of ~0.4 degrees -- the source of the
    // "camera trembles vertically while rotating" feel. Using a
    // real square-root removes the oscillation. Tiny mean pitch
    // shift (a few degrees more down-tilt) is the unavoidable
    // side-effect: QDIST2 systematically over-estimated by ~6%, so
    // pitch was systematically biased toward horizontal.
    SLONG dxz = Root(dx * dx + dz * dz);

    // Compute yaw/pitch directly in the 11.8 fixed-point format used by
    // FC_Cam yaw/pitch/roll (256 == 1 PSX game-angle unit). Avoids the
    // legacy round-to-integer-PSX step that loses the bottom 8 bits of
    // precision and produces ±1 PSX unit per-tick jitter on steady
    // rotation (~the micro-jerks visible at high frame rates with
    // smoothing snap modes).
    constexpr SLONG YAW_FP8_RANGE = 2048 * 256;
    fc->want_yaw   = ((-cam_arctan_psx_fp8(dx, dz)) % YAW_FP8_RANGE + YAW_FP8_RANGE) % YAW_FP8_RANGE;
    fc->want_pitch = cam_arctan_psx_fp8(dy, dxz);
    fc->want_roll  = 1024 << 8;

    if (fc->focus->Class == CLASS_PERSON && (fc->focus->Genus.Person->Flags & FLAG_PERSON_DRIVING)) {
        Thing* p_vehicle = TO_THING(fc->focus->Genus.Person->InCar);

        fc->want_roll = 1024 - (p_vehicle->Genus.Vehicle->WheelAngle * p_vehicle->Velocity >> 15);
        fc->want_roll &= 2047;
        fc->want_roll <<= 8;
    }
}

// Immediately snaps want_x/y/z to behind the focus character.
// uc_orig: FC_force_camera_behind (fallen/Source/fc.cpp)
void FC_force_camera_behind(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG gox;
    SLONG goy;
    SLONG goz;

    FC_calc_focus(fc);

    gox = fc->focus_x + (SIN(fc->focus_yaw) * fc->cam_dist >> 8);
    goz = fc->focus_z + (COS(fc->focus_yaw) * fc->cam_dist >> 8);
    goy = fc->focus_y + FC_focus_above(fc);

    fc->toonear = UC_FALSE;

    fc->want_x = gox;
    fc->want_y = goy;
    fc->want_z = goz;
}

// uc_orig: FC_setup_initial_camera (fallen/Source/fc.cpp)
void FC_setup_initial_camera(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG dx;
    SLONG dz;

    FC_calc_focus(fc);

    dx = -SIN(fc->focus_yaw);
    dz = -COS(fc->focus_yaw);

    fc->x = fc->want_x = fc->focus_x + dx + dx + dx;
    fc->y = fc->want_y = fc->focus_y + 0xa000;
    fc->z = fc->want_z = fc->focus_z + dz + dz + dz;

    FC_look_at_focus(fc);

    fc->yaw = fc->want_yaw;
    fc->pitch = fc->want_pitch;
    fc->roll = fc->want_roll;

    fc->toonear = UC_FALSE;
}

// uc_orig: FC_rotate_left (fallen/Source/fc.cpp)
void FC_rotate_left(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_cam[cam].rotate = +0x600;
}

// uc_orig: FC_rotate_right (fallen/Source/fc.cpp)
void FC_rotate_right(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_cam[cam].rotate = -0x600;
}

// Positions the camera just inside the warehouse, in front of the door closest to the player.
// Called when the player first enters a warehouse to give a good initial inside view.
// uc_orig: FC_setup_camera_for_warehouse (fallen/Source/fc.cpp)
void FC_setup_camera_for_warehouse(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG i;
    SLONG dx, dz;
    SLONG dist;
    SLONG best_dist = UC_INFINITY;
    SLONG best_door = -1;

    WARE_Ware* ww;

    ASSERT(fc->focus->Class == CLASS_PERSON);
    ASSERT(WITHIN(fc->focus->Genus.Person->Ware, 1, WARE_ware_upto - 1));

    ww = &WARE_ware[fc->focus->Genus.Person->Ware];

    for (i = 0; i < ww->door_upto; i++) {
        dx = ww->door[i].in_x - (fc->focus->WorldPos.X >> 16);
        dz = ww->door[i].in_z - (fc->focus->WorldPos.Z >> 16);
        dist = abs(dx) + abs(dz);
        if (best_dist > dist) {
            best_dist = dist;
            best_door = i;
        }
    }

    ASSERT(WITHIN(best_door, 0, ww->door_upto - 1));

    dx = ww->door[best_door].in_x - ww->door[best_door].out_x;
    dz = ww->door[best_door].in_z - ww->door[best_door].out_z;

    dx = SIGN(dx);
    dz = SIGN(dz);

    dx <<= 18;
    dz <<= 18;

    fc->want_x = fc->x = fc->focus->WorldPos.X + dx;
    fc->want_z = fc->z = fc->focus->WorldPos.Z + dz;
    fc->want_y = fc->y = fc->focus->WorldPos.Y + 0xa000;

    FC_look_at_focus(fc);

    fc->yaw = fc->want_yaw;
    fc->pitch = fc->want_pitch;
    fc->roll = fc->want_roll;

    fc->toonear = UC_FALSE;
}

// Apply a Y delta to (px, py, pz) point relative to fc->focus AND adjust
// the XZ position so the 3D distance to focus is preserved. Geometrically
// equivalent to rotating the point around focus in its meridional plane
// (the vertical plane containing focus, the point, and the world Y axis).
//
// Used by the mouse vertical orbit: mouse-Y motion ARCS the camera up/down
// around the focus character (camera rises and moves closer in XZ when
// looking up, lowers and moves farther in XZ when looking down) — same
// philosophy as horizontal mouse, which orbits in the XZ plane around
// focus. The plain Y-translation that this replaces produced a visible
// "camera lifts straight up while view tilts to keep aiming at focus"
// effect that felt off once vertical smoothing was tightened to match
// horizontal snappiness.
//
// The Y value is clamped UNCONDITIONALLY to the same focus-relative range
// the gamepad stick path uses (FC_CAM_Y_MIN/MAX_ABOVE_FOCUS). An earlier
// "only clamp if THIS call crosses the boundary" version let overshoot
// accumulate (e.g. focus_y moves DOWN, max_y drops, want_y now above new
// max_y — but `old_py <= max_y` condition then failed and the clamp
// silently stopped working, so further mouse-up pushed want_y past the
// nominal limit without bound).
// Pitch angle limits in game-angle units (2048 = 360°). Matched to the
// gamepad's effective pitch range at default cam_dist:
//   MIN: atan(FC_CAM_Y_MIN_ABOVE_FOCUS / target_xz) ≈ 11° = ~64 units
//   MAX: atan(FC_CAM_Y_MAX_ABOVE_FOCUS / target_xz) ≈ 53° = ~302 units
// The clamp is on the ANGLE itself, not on Y position — so the camera
// never pitches steeper than ~53° (no bird's-eye view) regardless of
// dist3d. Earlier Y-position-based clamping allowed pitch to approach
// 90° when dist3d was small (camera straight above focus, looking down).
static const SLONG MOUSE_PITCH_MIN = 64;
static const SLONG MOUSE_PITCH_MAX = 302;

static void apply_pitch_y_delta(FC_Cam* fc, SLONG* px, SLONG* py, SLONG* pz, SLONG y_delta)
{
    if (y_delta == 0)
        return;

    const int64_t old_dx = (int64_t)(*px) - fc->focus_x;
    const int64_t old_dz = (int64_t)(*pz) - fc->focus_z;
    const int64_t old_off_y = (int64_t)(*py) - fc->focus_y;
    const int64_t old_xz_sq = old_dx * old_dx + old_dz * old_dz;
    const int64_t dist3d_sq = old_xz_sq + old_off_y * old_off_y;

    // dist3d (fc-units). Pre-shift by 16 to keep Root() input within SLONG.
    const SLONG dist3d = (SLONG)((int64_t)Root((SLONG)(dist3d_sq >> 16)) << 8);

    // Convert pitch limits to Y offset using off_y = dist3d * sin(pitch).
    // SIN/COS in this engine return 16-bit fixed point ([-65536, +65536]),
    // so shift the product right by 16 to get fc-units.
    const int64_t pitch_max_off_y = ((int64_t)dist3d * SIN(MOUSE_PITCH_MAX)) >> 16;
    const int64_t pitch_min_off_y = ((int64_t)dist3d * SIN(MOUSE_PITCH_MIN)) >> 16;

    // Combine with gamepad's absolute Y range. Take the TIGHTER of the
    // two per side so that:
    //   - max never exceeds gamepad MAX (no extra-high camera at large dist3d).
    //   - min never goes below gamepad MIN (no extra-low camera at small dist3d
    //     — without this, pitch=11° on a shrunk dist3d gives a Y offset less
    //     than the gamepad floor, which the user perceived as mouse allowing
    //     the camera lower than the gamepad does).
    int64_t max_off_y = (pitch_max_off_y < FC_CAM_Y_MAX_ABOVE_FOCUS)
                        ? pitch_max_off_y
                        : (int64_t)FC_CAM_Y_MAX_ABOVE_FOCUS;
    int64_t min_off_y = (pitch_min_off_y > FC_CAM_Y_MIN_ABOVE_FOCUS)
                        ? pitch_min_off_y
                        : (int64_t)FC_CAM_Y_MIN_ABOVE_FOCUS;
    if (min_off_y > max_off_y) min_off_y = max_off_y;

    // Early-exit when user input pushes IN THE DIRECTION the position is
    // already past a limit. See block comment at the top of this section.
    if (y_delta > 0 && old_off_y > max_off_y) return;
    if (y_delta < 0 && old_off_y < min_off_y) return;

    int64_t new_off_y = old_off_y + y_delta;
    if (new_off_y > max_off_y) new_off_y = max_off_y;
    if (new_off_y < min_off_y) new_off_y = min_off_y;

    const SLONG new_py = (SLONG)(new_off_y + fc->focus_y);
    if (new_py == *py)
        return;

    // New XZ derived from preserved 3D distance.
    int64_t new_xz_sq = dist3d_sq - new_off_y * new_off_y;
    if (new_xz_sq < 0) new_xz_sq = 0;

    *py = new_py;

    if (old_xz_sq > 0) {
        const SLONG old_xz_root = Root((SLONG)(old_xz_sq >> 16));
        const SLONG new_xz_root = Root((SLONG)(new_xz_sq >> 16));
        if (old_xz_root > 0) {
            *px = fc->focus_x + (SLONG)(old_dx * new_xz_root / old_xz_root);
            *pz = fc->focus_z + (SLONG)(old_dz * new_xz_root / old_xz_root);
        }
    } else if (new_xz_sq > 0) {
        // Degenerate recovery: old XZ was 0 (legacy state from before
        // pitch was angle-clamped). Place point behind the character
        // per fc->focus_yaw — same direction as FC_setup_initial_camera.
        const SLONG new_xz = (SLONG)((int64_t)Root((SLONG)(new_xz_sq >> 16)) << 8);
        const SLONG fyaw = fc->focus_yaw & 2047;
        *px = fc->focus_x - (SLONG)(((int64_t)SIN(fyaw) * new_xz) >> 16);
        *pz = fc->focus_z - (SLONG)(((int64_t)COS(fyaw) * new_xz) >> 16);
    }
}

// Main camera update called once per frame. Processes each active camera in FC_cam[].
// Steps per camera:
//   1. Skip if focus == NULL (inactive).
//   2. FC_alter_for_pos: get height/distance offsets from character state.
//   3. FC_calc_focus: compute the target look-at position.
//   4. Warehouse transition: snap camera on enter/exit.
//   5. lookabove: 0xa000 normally, decays on death.
//   6. Rotate: L1/R1 camera orbit; nobehind suppresses get-behind logic.
//   7. toonear cancel: exit first-person mode if angle diverges > 200.
//   8. Get-behind: push camera behind Darci.
//   9. Y position: track FC_focus_above with clamped delta.
//   10. Distance clamp: keep camera between FC_DIST_MIN and FC_DIST_MAX.
//   11. Position smoothing: want → actual (x/z >>2, y >>3).
//   12. Angle smoothing: want_yaw/pitch/roll → actual >>2.
//   13. Lens: fixed FOV = 0x28000 * CAM_MORE_IN.
//   14. Shake: random offset decays each frame.
// uc_orig: FC_process (fallen/Source/fc.cpp)
void FC_process()
{
    SLONG dx, dy, dz;
    SLONG cam;
    SLONG dist;
    SLONG ddist;
    SLONG behind_x, behind_z;

    UBYTE used_to_be_in_warehouse;
    SLONG offset_dist, offset_height;

    FC_Cam* fc;

    // Previous-tick focus snapshot used by the XZ translation-tracking
    // block and the Y delta-tracking block. Lifted here (out of those
    // blocks) so the cut-scene short-circuit below can keep them in sync
    // with the current focus while we skip the trackers — otherwise the
    // first post-cutscene tick would see a stale delta and jolt the
    // camera. Per-cam to support split-screen.
    static SLONG s_prev_focus_x[FC_MAX_CAMS] = {};
    static SLONG s_prev_focus_y[FC_MAX_CAMS] = {};
    static SLONG s_prev_focus_z[FC_MAX_CAMS] = {};

    const bool cutscene_active = (EWAY_cam_active != 0) || PLAYCUTS_playing;

    for (cam = 0; cam < FC_MAX_CAMS; cam++) {
        fc = &FC_cam[cam];

        if (fc->focus == NULL) {
            continue;
        }

        FC_alter_for_pos(fc, &offset_height, &offset_dist);

        used_to_be_in_warehouse = fc->focus_in_warehouse;

        FC_calc_focus(fc);

        // "Manual Y input is active this tick" — set by the mouse and
        // stick input blocks below when a non-zero vertical input arrived.
        // Consumed by the Y-tracking block further down to decide between
        // AUTO-mode default-height convergence (no input) and AUTO-mode
        // delta-tracking (input active). MANUAL mode ignores this and
        // always delta-tracks. Default false so a tick with no input
        // block running counts as idle. Declared at cam-loop scope
        // because the input blocks and the Y-tracking block live in
        // separate `if (!fc->toonear)` sub-scopes within the loop.
        bool manual_y_input_this_tick = false;

        // Rigidly inherit vehicle motion when the focus person stands on
        // a moving vehicle: translate camera by vehicle's per-tick world
        // delta and rotate it around the vehicle's pivot by the per-tick
        // yaw delta. Result: camera is glued to the vehicle's reference
        // frame, so standing still on a moving/turning vehicle gives the
        // exact same camera behaviour as standing still on the ground.
        if (fc->platform_thing) {
            Thing* p_vehicle = TO_THING(fc->platform_thing);
            SLONG cur_x = p_vehicle->WorldPos.X;
            SLONG cur_z = p_vehicle->WorldPos.Z;
            SLONG cur_yaw = p_vehicle->Genus.Vehicle->Angle;

            SLONG dx_plat = cur_x - fc->platform_last_x;
            SLONG dz_plat = cur_z - fc->platform_last_z;
            SLONG dyaw = (cur_yaw - fc->platform_last_yaw) & 2047;
            SLONG dyaw_signed = (dyaw > 1024) ? dyaw - 2048 : dyaw;

            if (dx_plat || dz_plat || dyaw_signed) {
                SLONG s = SIN(dyaw);
                SLONG c = COS(dyaw);

                // Rotate (px, pz) around (last_x, last_z) by dyaw, then
                // translate to new pivot. Game yaw is clockwise-positive
                // (matching `behind_x = focus_x + SIN(yaw)*dist`), so the
                // rotation matrix is the CW form: new_x = rx*c + rz*s,
                // new_z = rz*c - rx*s. Pre-shift by 4 to keep the
                // intermediate product within SLONG range (max camera
                // offset ~120000; 120000>>4 * 65536 = 4.9e8 < 2^31).
                SLONG rx, rz;
#define INHERIT_PT(px, pz) \
    rx = (px) - fc->platform_last_x; \
    rz = (pz) - fc->platform_last_z; \
    (px) = cur_x + (((rx >> 4) * c + (rz >> 4) * s) >> 12); \
    (pz) = cur_z + (((rz >> 4) * c - (rx >> 4) * s) >> 12)

                INHERIT_PT(fc->x, fc->z);
                INHERIT_PT(fc->want_x, fc->want_z);
#undef INHERIT_PT

                // Yaw stored as angle << 8.
                SLONG yaw_mask = (2048 << 8) - 1;
                fc->yaw = (fc->yaw + (dyaw_signed << 8)) & yaw_mask;
                fc->want_yaw = (fc->want_yaw + (dyaw_signed << 8)) & yaw_mask;
            }

            fc->platform_last_x = cur_x;
            fc->platform_last_z = cur_z;
            fc->platform_last_yaw = cur_yaw;
        }

        // Warehouse transition: snap camera on entry/exit.
        if (used_to_be_in_warehouse != fc->focus_in_warehouse) {
            EWAY_cam_jumped = 10;
            if (fc->focus_in_warehouse) {
                FC_setup_camera_for_warehouse(cam);
            } else {
                FC_setup_initial_camera(cam);
            }
            // FC_cam was just hard-snapped to a new position — collapse
            // render-interp prev=curr so the renderer doesn't lerp across
            // the warehouse boundary jump.
            extern void render_interp_mark_camera_teleport(FC_Cam*);
            render_interp_mark_camera_teleport(fc);
        }

        // lookabove: height the camera looks above the focus.
        // Slowly lowers after the focus person dies.
        if (fc->focus->Class == CLASS_PERSON && (fc->focus->State == STATE_DEAD || fc->focus->State == STATE_DYING)) {
            if (fc->lookabove > 0) {
                fc->lookabove -= 0x80 * TICK_RATIO >> TICK_SHIFT;
            }
        } else {
            fc->lookabove = 0xa000;
        }

        // Cut-scene freeze.
        //
        // While EWAY / PLAYCUTS plays a cinematic, the visible camera
        // is driven from EWAY_cam_* (see vis_cam.cpp: VC_state is set
        // to OFF and render-interp pulls EWAY_cam_*). FC_cam is hidden
        // for the duration. We must NOT keep mutating fc->want_* /
        // fc->yaw / pitch / shake here — otherwise auto-corrections
        // (get-behind, Y-convergence, translation tracking, mouse /
        // stick orbit if the player nudges input during a cinematic)
        // accumulate while the cinematic plays, and when it ends the
        // gameplay camera snaps to whatever auto-corrected state we
        // built up instead of the script-set position. Symptom: the
        // first frame of gameplay shows the camera in a different
        // place each time depending on cut-scene length / mouse
        // motion / focus-character motion during the cinematic.
        //
        // Things still done here:
        //   • Drain the mouse-relative accumulator so any cursor
        //     motion during the cinematic doesn't bleed into the
        //     first gameplay tick.
        //   • Sync the previous-focus snapshots used by the XZ /
        //     Y delta trackers so they compute zero delta on the
        //     first post-cinematic tick (focus may have moved a
        //     lot during the cinematic — e.g. character respawn,
        //     warehouse transition).
        //
        // Things deliberately NOT done here: cut-scene skip (a
        // button press routed through input_actions / EWAY), pause
        // menu entry (GAMEMENU layer), shake decay (cut-scene runs
        // briefly enough that residual shake is invisible anyway and
        // FC_cam is hidden during the cinematic regardless).
        //
        // Platform inheritance and warehouse-transition snap above
        // ARE allowed to run — they keep fc anchored to the right
        // reference frame if the focus character is on a moving
        // vehicle or crosses a warehouse boundary mid-cinematic.
        if (cutscene_active) {
            SLONG drain_x = 0, drain_y = 0;
            input_mouse_consume_rel(&drain_x, &drain_y);
            s_prev_focus_x[cam] = fc->focus_x;
            s_prev_focus_y[cam] = fc->focus_y;
            s_prev_focus_z[cam] = fc->focus_z;
            continue;
        }

        if (!fc->toonear) {
            if (fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE) {
                fc->rotate = 0;
                fc->nobehind = 0;
            }

            if (fc->rotate != 0) {
                if (fc->rotate > 0) {
                    fc->rotate -= 0x80 * TICK_RATIO >> TICK_SHIFT;
                }
                if (fc->rotate < 0) {
                    fc->rotate += 0x80 * TICK_RATIO >> TICK_SHIFT;
                }

                if (abs(fc->rotate) < 0x100) {
                    fc->rotate = 0;
                } else {
                    dx = fc->focus_x - fc->want_x >> 3;
                    dz = fc->focus_z - fc->want_z >> 3;

                    fc->want_x -= dz * (fc->rotate >> 4) * TICK_RATIO >> (TICK_SHIFT + 6);
                    fc->want_z += dx * (fc->rotate >> 4) * TICK_RATIO >> (TICK_SHIFT + 6);
                }

                fc->nobehind = 0x2000;
                fc->last_manual_cam_turn = (SLONG)GAME_TURN; // hand rotate -> lock fight trigger
            }

            // Right stick: continuous camera orbit and height adjustment.
            // X axis: horizontal orbit (same math as L2/R2 rotate, proportional to deflection).
            // Y axis: camera height offset (stick up = higher, stick down = lower).
            //
            // Disabled during the vehicle-entry animation: that anim auto-rotates
            // the camera so the (visually closed) door isn't seen from a bad
            // angle; letting the player override with the right stick exposes
            // the trick. Released as soon as SubState transitions out of
            // ENTERING_VEHICLE (anim end).
            const bool entering_vehicle = CAR_ENTER_EXIT_CAM_LOCK
                && fc->focus
                && fc->focus->Class == CLASS_PERSON
                && (fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE
                    || fc->focus->SubState == SUB_STATE_EXITING_VEHICLE);

            // Mouse-driven orbital camera. Mirrors the right-stick block
            // below — same orbit-around-focus math, same height-delta
            // adjustment — but the input is mouse-relative-motion in
            // window pixels instead of stick deflection. Two differences:
            //
            //   1. No TICK_RATIO scaling. Mouse delta is already per-
            //      tick motion (events accumulate between consumes), so
            //      the value is naturally frame-rate independent. The
            //      stick path scales by TICK_RATIO because stick
            //      deflection is constant — without the scale the rotation
            //      rate would track the render frame rate.
            //
            //   2. Gated on mouse_capture_is_active() — that's the
            //      single source of truth for "mouse is gameplay input
            //      right now". Capture engages on click in the window
            //      during active gameplay; see engine/input/mouse_capture.h.
            //
            // Tunables (top of file): MOUSE_YAW_SCALE, MOUSE_HEIGHT_SCALE,
            // MOUSE_ORBIT_LAG. See those for what each knob does.
            //
            // TODO (post-1.0):
            //   - Sensitivity option in the menu.
            //   - Y-axis inversion option.
            //   - Mouse-driven aim mode.
            //   - Per-axis dead zone if micro-tremor turns out to matter.
            // input_gameplay_enabled() gate: while F1 debug modifier is
            // held, mouse motion is dropped so the camera doesn't swing
            // around while the dev is using debug hotkeys. We DRAIN the
            // accumulated motion (consume + discard) rather than just
            // skipping, otherwise motion would burst all at once on F1
            // release.
            if (!entering_vehicle && mouse_capture_is_active() && !input_gameplay_enabled()) {
                input_mouse_drain_rel();
            }
            if (!entering_vehicle && mouse_capture_is_active() && input_gameplay_enabled()) {
                SLONG mdx = 0, mdy = 0;
                input_mouse_consume_rel(&mdx, &mdy);

                if (mdx != 0) {
                    // Horizontal mouse orbit: rotate camera position
                    // around focus in XZ by `angle`.
                    //
                    // MANUAL mode: full snap on both want AND fc (no
                    // residual). Rotation is intentionally instant; the
                    // angle smoothing block at the end of the pipeline
                    // also snaps fc->yaw = want_yaw in MANUAL.
                    //
                    // AUTO mode: orbit want by full angle, orbit fc by
                    // (angle - MOUSE_ORBIT_LAG) so a small residual
                    // remains for position+angle smoothing to close
                    // over the next few ticks. Mirrors the gamepad
                    // stick's sync-with-residual approach so the two
                    // input devices have a consistent rubber-band feel
                    // in AUTO.
                    //
                    // CCW rotation (matches the original Euler-step
                    // direction for positive mdx): new_rx = rx*c - rz*s,
                    // new_rz = rx*s + rz*c. 64-bit math to avoid overflow
                    // at large camera offsets.
                    SLONG angle = mdx * MOUSE_YAW_SCALE_Q8 / 256;
                    SLONG s_full = SIN(angle & 2047);
                    SLONG c_full = COS(angle & 2047);
                    int64_t rx, rz;
#define ORBIT_PT(px, pz, s, c) \
    rx = (int64_t)(px) - fc->focus_x; \
    rz = (int64_t)(pz) - fc->focus_z; \
    (px) = fc->focus_x + (SLONG)((rx * (c) - rz * (s)) >> 16); \
    (pz) = fc->focus_z + (SLONG)((rx * (s) + rz * (c)) >> 16)

                    ORBIT_PT(fc->want_x, fc->want_z, s_full, c_full);

                    if (camera_is_manual()) {
                        // Snap fc — no residual lag.
                        ORBIT_PT(fc->x, fc->z, s_full, c_full);
                    } else {
                        // AUTO: sync fc by (angle - lag), leaving the
                        // rubberness-scaled residual to be smoothed.
                        const SLONG lag = (SLONG)orbit_lag_for_rubberness((float)MOUSE_ORBIT_LAG);
                        SLONG sync_angle;
                        if      (angle >  lag) sync_angle = angle - lag;
                        else if (angle < -lag) sync_angle = angle + lag;
                        else                   sync_angle = 0;
                        if (sync_angle != 0) {
                            SLONG s_sync = SIN(sync_angle & 2047);
                            SLONG c_sync = COS(sync_angle & 2047);
                            ORBIT_PT(fc->x, fc->z, s_sync, c_sync);
                            SLONG yaw_mask = (2048 << 8) - 1;
                            fc->yaw      = (fc->yaw      - (sync_angle << 8)) & yaw_mask;
                            fc->want_yaw = (fc->want_yaw - (sync_angle << 8)) & yaw_mask;
                        }
                    }
#undef ORBIT_PT

                    fc->nobehind = 0x2000;
                    fc->last_manual_cam_turn = (SLONG)GAME_TURN;
                }

                if (mdy != 0) {
                    manual_y_input_this_tick = true;
                    // Vertical mouse = orbital pitch around focus.
                    // Camera arcs up/down on a sphere around the focus
                    // character — looking up lifts the camera AND moves
                    // it closer in XZ; looking down lowers it AND moves
                    // it farther in XZ. 3D distance to focus is held
                    // constant by the helper.
                    //
                    // Same MOUSE_INVERT_Y semantics as before: =1 means
                    // mouse-up lowers the camera (FPS-style inverted),
                    // =0 means mouse-up raises it. mdy is positive when
                    // mouse moves down (SDL convention).
                    //
                    // Full snap on both want AND fc — no rotation
                    // rubber-band. fc->pitch is NOT explicitly updated
                    // here: FC_look_at_focus (later in the pipeline)
                    // recomputes want_pitch from the new want geometry,
                    // and the KBM-gated angle smoothing block snaps
                    // fc->pitch = want_pitch.
                    //
                    // No want_y clamp here — clamping is done inside the
                    // helper, only when THIS call crosses the boundary,
                    // so the focus-tracking delta below doesn't see a
                    // double clamp (which historically produced a -2Δ
                    // jerk when want_y was at the clamp AND focus_y
                    // moved at the same time).
                    SLONG height_delta = (MOUSE_INVERT_Y ? mdy : -mdy) * MOUSE_HEIGHT_SCALE;

                    // Apply pitch helper ONLY to want, then mirror the
                    // resulting world-space delta onto fc.
                    //
                    // Calling the helper independently for want and fc was
                    // broken: each call computes pitch_max_off_y from its
                    // OWN dist3d, and want vs fc diverge over time
                    // (position smoothing lag + want-only translation /
                    // Y-focus tracking). At the upper limit, want correctly
                    // saw eff_dy=0, but fc — with a larger dist3d — saw
                    // a higher pitch_max and continued moving up by
                    // thousands of fc-units per tick. Position smoothing
                    // then dragged fc back down toward want each tick,
                    // visible as drift at the limit on continued mouse-up.
                    //
                    // Mirroring the delta keeps want/fc in lockstep on
                    // mouse motion (same as ORBIT_PT on both for mdx).
                    // Other sources of want/fc divergence (translation,
                    // Y-focus tracking, smoothing) keep working as before.
                    const SLONG want_x_pre = fc->want_x;
                    const SLONG want_y_pre = fc->want_y;
                    const SLONG want_z_pre = fc->want_z;
                    apply_pitch_y_delta(fc, &fc->want_x, &fc->want_y, &fc->want_z, height_delta);
                    if (camera_is_manual()) {
                        // MANUAL: mirror the world-space delta onto fc
                        // so vertical rotation is instant (no rubber).
                        fc->x += (fc->want_x - want_x_pre);
                        fc->y += (fc->want_y - want_y_pre);
                        fc->z += (fc->want_z - want_z_pre);
                    }
                    // AUTO: leave fc alone — position smoothing pulls it
                    // toward want over the next few ticks (rubber tail).

                    fc->nobehind = 0x2000;
                }
            }

            if (!entering_vehicle && active_input_device != INPUT_DEVICE_KEYBOARD_MOUSE && input_gamepad_connected()) {
                SLONG stick_x = input_stick_x_axis(GAXIS_RIGHT) - 32768; // signed, -32768..+32767
                SLONG stick_y = input_stick_y_axis(GAXIS_RIGHT) - 32768;

                if (abs(stick_x) > 8000) {
                    // True rotation with constant-residual lag -- see
                    // the comment in the mouse block for the full
                    // rationale. Scale: original Euler effective theta
                    // at full deflection ~ 0.187 rad = ~61 game units;
                    // we preserve that magnitude. TICK_RATIO scaling
                    // kept so per-tick angle is framerate-independent.
                    SLONG angle = (stick_x * 61) / 32767;
                    angle = angle * TICK_RATIO >> TICK_SHIFT;

                    // MANUAL mode: sync fc by FULL angle (no residual
                    // lag) — instant rotation. AUTO mode: keep a
                    // rubberness-scaled residual for the rubber tail.
                    SLONG sync_angle;
                    if (camera_is_manual()) {
                        sync_angle = angle;
                    } else {
                        const SLONG lag = (SLONG)orbit_lag_for_rubberness((float)STICK_ORBIT_LAG);
                        if      (angle >  lag) sync_angle = angle - lag;
                        else if (angle < -lag) sync_angle = angle + lag;
                        else                   sync_angle = 0;
                    }

                    SLONG s_full = SIN(angle & 2047);
                    SLONG c_full = COS(angle & 2047);
                    int64_t rx, rz;
#define ORBIT_PT(px, pz, s, c) \
    rx = (int64_t)(px) - fc->focus_x; \
    rz = (int64_t)(pz) - fc->focus_z; \
    (px) = fc->focus_x + (SLONG)((rx * (c) - rz * (s)) >> 16); \
    (pz) = fc->focus_z + (SLONG)((rx * (s) + rz * (c)) >> 16)

                    ORBIT_PT(fc->want_x, fc->want_z, s_full, c_full);

                    if (sync_angle != 0) {
                        SLONG s_sync = SIN(sync_angle & 2047);
                        SLONG c_sync = COS(sync_angle & 2047);
                        ORBIT_PT(fc->x, fc->z, s_sync, c_sync);
                        SLONG yaw_mask = (2048 << 8) - 1;
                        fc->yaw      = (fc->yaw      - (sync_angle << 8)) & yaw_mask;
                        fc->want_yaw = (fc->want_yaw - (sync_angle << 8)) & yaw_mask;
                    }
#undef ORBIT_PT

                    fc->nobehind = 0x2000;
                    fc->last_manual_cam_turn = (SLONG)GAME_TURN; // hand rotate -> lock fight trigger
                }

                // Height: directly move want_y (bypasses the slow Y smoothing).
                // Stick up (negative Y) = raise camera, stick down = lower camera.
                if (abs(stick_y) > 8000) {
                    manual_y_input_this_tick = true;
                    SLONG height_delta = (stick_y * 0x3100) / 32767 * TICK_RATIO >> TICK_SHIFT;
                    const SLONG want_y_pre = fc->want_y;
                    fc->want_y += height_delta;

                    SLONG min_y = fc->focus_y + FC_CAM_Y_MIN_ABOVE_FOCUS;
                    SLONG max_y = fc->focus_y + FC_CAM_Y_MAX_ABOVE_FOCUS;
                    if (fc->want_y < min_y)
                        fc->want_y = min_y;
                    if (fc->want_y > max_y)
                        fc->want_y = max_y;

                    if (camera_is_manual()) {
                        // MANUAL: mirror the clamped delta onto fc.y so
                        // vertical input is instant. AUTO: leave fc.y
                        // alone — position smoothing pulls it.
                        fc->y += (fc->want_y - want_y_pre);
                    }

                    fc->nobehind = 0x2000;
                }
            }
        } else {
            fc->rotate = 0;
        }

        // Cancel toonear (first-person mode) if the camera angle diverges enough.
        if (fc->toonear) {
            dx = abs(fc->want_x - fc->focus_x);
            dz = abs(fc->want_z - fc->focus_z);
            dist = QDIST2(dx, dz);

            if (dist > fc->toonear_dist + 0x4000) {
                SLONG cdx = fc->focus_x - fc->want_x >> 8;
                SLONG cdz = fc->focus_z - fc->want_z >> 8;
                SLONG cangle = -Arctan(cdx, cdz) & 2047;
                SLONG dangle;

                dangle = fc->toonear_focus_yaw - cangle;

                if (dangle < -1024) {
                    dangle += 2048;
                }
                if (dangle > +1024) {
                    dangle -= 2048;
                }

                if (abs(dangle) > 200) {
                    fc->toonear = UC_FALSE;
                    fc->x = fc->want_x = fc->toonear_x;
                    fc->y = fc->want_y = fc->toonear_y;
                    fc->z = fc->want_z = fc->toonear_z;
                    fc->yaw = fc->want_yaw = fc->toonear_yaw;
                    fc->pitch = fc->want_pitch = fc->toonear_pitch;
                    fc->roll = fc->want_roll = fc->toonear_roll;
                    fc->smooth_transition = UC_TRUE;
                }
            }
        }

        // OpenChaos: in fight mode the original never gets behind the
        // player. We re-enable it as a one-shot smooth swing whenever
        // the HUD's selected target changes (fight start, manual switch
        // with circle, or the old target dying and auto-reselecting) --
        // but ONLY while actually in fight stance, never on losing the
        // target (-> none), and not within the manual-rotation lockout.
        // A focus swap just resyncs (no swing). Re-triggering mid-swing
        // simply re-arms the window so it keeps smoothing to the new
        // behind. Manual camera input still interrupts via `nobehind`.
        {
            UWORD cur_t = (fc->focus->Class == CLASS_PERSON)
                ? fc->focus->Genus.Person->Target : 0;
            bool in_fight = (fc->focus->Class == CLASS_PERSON
                && fc->focus->Genus.Person->Mode == PERSON_MODE_FIGHT);

            if (fc->fight_track_focus != fc->focus) {
                fc->fight_track_focus = fc->focus; // new focus -> resync only
            } else if (in_fight && cur_t != 0 && cur_t != fc->fight_prev_target) {
                if ((SLONG)GAME_TURN - fc->last_manual_cam_turn >= FC_FIGHT_BEHIND_LOCK_TURNS)
                    fc->fight_behind_until = (SLONG)GAME_TURN + FC_FIGHT_BEHIND_TURNS;
            }
            fc->fight_prev_target = cur_t;

            // Inactivity auto-trigger: every FC_FIGHT_AUTO_PERIOD while
            // fighting (and outside the manual-rotation lockout) re-arm
            // the swing, so the camera keeps coming back behind Darci
            // even with no target change (enemy circled to the side).
            // When ineligible (not fighting / inside the lockout) the
            // next-due time is deferred so the period restarts cleanly
            // once eligible again.
            if (in_fight
                && (SLONG)GAME_TURN - fc->last_manual_cam_turn >= FC_FIGHT_BEHIND_LOCK_TURNS) {
                if ((SLONG)GAME_TURN >= fc->fight_auto_next) {
                    fc->fight_behind_until = (SLONG)GAME_TURN + FC_FIGHT_BEHIND_TURNS;
                    fc->fight_auto_next = (SLONG)GAME_TURN + FC_FIGHT_AUTO_PERIOD;
                }
            } else {
                fc->fight_auto_next = (SLONG)GAME_TURN + FC_FIGHT_AUTO_PERIOD;
            }
        }

        // Excess-speed translation tracking: when the focus moves
        // faster than VC_FOCUS_SPEED_CAP per tick (= when driving),
        // anchor want_x/z to the EXCESS portion of the focus motion
        // (everything above the cap). Below the cap nothing is
        // applied — the camera-relative-to-focus geometry then sees
        // an effective focus motion of at most the cap value.
        //
        // Two outcomes:
        //   • Running / walking (focus speed below cap): tracking
        //     contributes 0, default get-behind/orbit behaviour kept
        //     verbatim → "snap to behind on movement" feel preserved.
        //   • Driving fast (focus speed above cap): tracking absorbs
        //     the excess, so the camera follows the car closely AND
        //     stick rotation only fights against `cap`-per-tick drag
        //     instead of full vehicle speed → manual orbit reaches
        //     the front of the car.
        //
        // Guard against huge deltas (focus teleport on level load /
        // mission start) so we don't launch want across the map.
        // (s_prev_focus_x/z lifted to FC_process scope for the
        // cut-scene short-circuit at the top of the loop.)
        {
            SLONG focus_dx = fc->focus_x - s_prev_focus_x[cam];
            SLONG focus_dz = fc->focus_z - s_prev_focus_z[cam];
            const SLONG VC_FOCUS_SPEED_CAP   = 0x3000;   // ~9 cm/tick
            const SLONG TELEPORT_THRESHOLD   = 0x40000;
            // Guard against huge deltas (focus teleport on level load /
            // mission start) so we don't launch want across the map.
            if (abs(focus_dx) < TELEPORT_THRESHOLD
                && abs(focus_dz) < TELEPORT_THRESHOLD) {
                if (camera_is_manual()) {
                    // MANUAL: rigidly translate the camera by the FULL
                    // focus delta — not just the excess above a cap. This
                    // is what keeps the camera's world-space angle FIXED
                    // relative to the character: if camera and focus move
                    // by the same vector each tick, the camera→focus
                    // offset is constant, so look_at_focus computes the
                    // same want_yaw and the camera never spontaneously
                    // rotates on character motion. Manual input is the
                    // ONLY thing that changes the angle.
                    //
                    // EXCEPT when standing on a moving vehicle platform:
                    // the platform_thing block above already moved
                    // want_x/z (rigid rotate + translate around vehicle
                    // pivot) to inherit vehicle motion. Adding focus_dx
                    // again here would DOUBLE-APPLY vehicle motion onto
                    // want — over-shooting the focus, which then makes
                    // look_at_focus compute a yaw offset and the camera
                    // auto-rotates (visible as "camera auto-rotation
                    // while standing on a moving vehicle"). AUTO mode
                    // doesn't hit this bug because its excess-speed
                    // tracking below contributes 0 for typical vehicle
                    // speeds, leaving platform inheritance as the single
                    // motion source. Skip translation tracking here on
                    // MANUAL when on a platform to match.
                    //
                    // All downstream smoothing is preserved: want→x
                    // position smoothing, distance clamp, and the
                    // character side's own animation/physics smoothing
                    // of focus_x/z all still run. So bumps, stairs, and
                    // mid-step jitter come through with the same feel as
                    // AUTO — just no auto-rotation.
                    if (!fc->platform_thing) {
                        fc->want_x += focus_dx;
                        fc->want_z += focus_dz;
                    }
                } else {
                    // AUTO: excess-speed tracking. Below cap the
                    // contribution is 0 (original walk/run feel kept,
                    // get-behind decides where the camera ends up);
                    // above cap the excess is absorbed so manual orbit
                    // can fight only `cap`-per-tick drag instead of full
                    // vehicle speed.
                    SLONG track_dx = 0;
                    SLONG track_dz = 0;
                    if      (focus_dx >  VC_FOCUS_SPEED_CAP) track_dx = focus_dx - VC_FOCUS_SPEED_CAP;
                    else if (focus_dx < -VC_FOCUS_SPEED_CAP) track_dx = focus_dx + VC_FOCUS_SPEED_CAP;
                    if      (focus_dz >  VC_FOCUS_SPEED_CAP) track_dz = focus_dz - VC_FOCUS_SPEED_CAP;
                    else if (focus_dz < -VC_FOCUS_SPEED_CAP) track_dz = focus_dz + VC_FOCUS_SPEED_CAP;
                    fc->want_x += track_dx;
                    fc->want_z += track_dz;
                }
            }
            s_prev_focus_x[cam] = fc->focus_x;
            s_prev_focus_z[cam] = fc->focus_z;
        }

        // L2-held suppression: while the player holds L2 on the gamepad,
        // freeze the auto-get-behind logic completely so the camera doesn't
        // follow the character into a new orientation mid-tap. This keeps
        // the player's reference frame stable while they line up a roll /
        // backflip or commit to a slow-walk in a character-relative
        // direction. The get-behind math has no accumulator — every tick
        // it recomputes from focus_x/focus_yaw — so skipping it while L2
        // is held has no lingering state; the next tick after L2 release
        // resumes auto-positioning immediately.
        //
        // fc->nobehind is left untouched here on purpose: it's the
        // *manual cam input* suppression mechanism (mouse / right-stick
        // orbit / L1 zoom rotate refresh it on demand), and we don't want
        // L2 release to wipe an active mouse-orbit lock-out. The two
        // suppressions are independent.
        const bool l2_freeze_get_behind = input_l2_is_held();

        // Scene override — vehicle entry: the enter animation programmatically
        // rotates the camera behind the (entry-facing) character REGARDLESS of
        // camera mode. The player's manual input is already suppressed during
        // entry (see the `entering_vehicle` gates on the mouse / stick input
        // blocks above), so on KBM/MANUAL the camera would otherwise just sit
        // still. Force get-behind here, and bypass the nobehind / fight skips
        // so a mouse orbit done right before entry can't block the lock. Auto-
        // released when SubState leaves ENTERING_VEHICLE (anim end).
        const bool entering_vehicle_scene = CAR_ENTER_EXIT_CAM_LOCK
            && fc->focus
            && fc->focus->Class == CLASS_PERSON
            && (fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE
                || fc->focus->SubState == SUB_STATE_EXITING_VEHICLE);

        // MANUAL mode: no automatic camera rotation at all. Manual input
        // (mouse / stick orbit) is the ONLY thing that rotates the camera
        // — no get-behind pull when the character moves, no inactivity-
        // timer snap-back. Same philosophy as modern FPS / TPS games.
        // We still let `nobehind` decrement normally so that if the
        // player switches back to AUTO mode the state is sane (the
        // residual manual-input lockout counts down rather than being
        // frozen at whatever the last manual orbit set it to).
        const bool manual_freeze_get_behind = camera_is_manual() && !entering_vehicle_scene;
        if (!fc->toonear && !l2_freeze_get_behind) {
            if (fc->nobehind && !entering_vehicle_scene) {
                fc->nobehind -= 0x80 * TICK_RATIO >> TICK_SHIFT;
                if (fc->nobehind < 0) {
                    fc->nobehind = 0;
                }
            } else if (manual_freeze_get_behind) {
                // MANUAL mode: skip auto get-behind entirely.
                // Manual orbit is the sole camera rotation source.
            } else if (!entering_vehicle_scene
                && fc->focus->Class == CLASS_PERSON
                && fc->focus->Genus.Person->Mode == PERSON_MODE_FIGHT
                && (SLONG)GAME_TURN >= fc->fight_behind_until) {
                // Don't get behind fighting people -- except briefly
                // after a target change (while GAME_TURN is still below
                // fight_behind_until, the else-branch runs the normal
                // smooth get-behind for one swing).
            } else {
                // Get-behind: gently push camera to behind Darci.
                behind_x = fc->focus_x + (SIN(fc->focus_yaw) * ((fc->cam_dist * offset_dist) >> 8) >> 8);
                behind_z = fc->focus_z + (COS(fc->focus_yaw) * ((fc->cam_dist * offset_dist) >> 8) >> 8);

                dx = behind_x - fc->want_x;
                dz = behind_z - fc->want_z;

                if (fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE) {
                    fc->want_x += dx >> 3;
                    fc->want_z += dz >> 3;
                } else if (fc->focus->Class == CLASS_PERSON && fc->focus->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                    fc->want_x += dx >> 5;
                    fc->want_z += dz >> 5;
                }

                fc->want_x += dx >> 3;
                fc->want_z += dz >> 3;

                if (person_has_gun_out(fc->focus) && !(fc->focus->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING))) {
                    fc->want_x += dx >> 4;
                    fc->want_z += dz >> 4;
                }
            }
        }

        if (!fc->toonear) {
            // Track focus character vertically.
            //
            // Mouse and gamepad need different semantics here:
            //   • Gamepad with stick Y idle: stick is at neutral. The
            //     camera should drift back to a default height above
            //     the character — original convergence toward
            //     goto_y = focus_y + FC_focus_above + offset_height.
            //   • Gamepad with stick Y deflected: stick is actively
            //     setting the height. Convergence would fight the
            //     input — every tick the stick subtracts ~3000 from
            //     want_y and the convergence's 0xd00 cap adds it
            //     right back, so the camera doesn't move (and jerks
            //     between the two values). Use the same delta-track
            //     path as mouse — want_y stays where the stick puts
            //     it, only character vertical motion is tracked.
            //   • Mouse: there is no "released" position. Once the
            //     player moves the mouse to set a height, that height
            //     should PERSIST — no drift toward a default. We still
            //     have to track the character's own vertical motion
            //     (jumps, stairs, platforms) so the camera doesn't
            //     get left behind, so we apply only the focus_y
            //     DELTA from the previous tick, not absolute
            //     convergence.
            // (s_prev_focus_y lifted to FC_process scope for the
            // cut-scene short-circuit at the top of the loop.)
            SLONG focus_y_delta = fc->focus_y - s_prev_focus_y[cam];

            // Mode-driven Y behaviour:
            //   MANUAL: always delta-track (no auto convergence — player
            //           sets the height with manual input, it persists).
            //   AUTO + manual Y input this tick: delta-track (input would
            //           otherwise fight the convergence pull each tick).
            //   AUTO + no manual Y input: converge toward default height
            //           above focus (idle-return feature of AUTO mode).
            //
            // `manual_y_input_this_tick` is set by the mouse and stick
            // input blocks above when a non-zero vertical input arrived.
            // For gamepad we also detect a held-deflection that didn't
            // pass through the input block this tick (input block runs
            // on a different path but the held-stick semantics are the
            // same — convergence would still fight it).
            if (!manual_y_input_this_tick && input_gamepad_connected()) {
                SLONG sy = input_stick_y_axis(GAXIS_RIGHT) - 32768;
                if (abs(sy) > 8000) manual_y_input_this_tick = true;
            }

            if (camera_is_manual() || manual_y_input_this_tick) {
                fc->want_y += focus_y_delta;
                // Clamp to the unified Y range.
                SLONG min_y = fc->focus_y + FC_CAM_Y_MIN_ABOVE_FOCUS;
                SLONG max_y = fc->focus_y + FC_CAM_Y_MAX_ABOVE_FOCUS;
                if (fc->want_y < min_y)
                    fc->want_y = min_y;
                if (fc->want_y > max_y)
                    fc->want_y = max_y;
            } else {
                // AUTO + no manual Y input: track character vertical
                // motion 1:1 AND slowly drift toward default height
                // above focus (idle-return feature).
                //
                // 1:1 delta tracking is necessary so the camera Y stays
                // locked to character Y when running over bumps, curbs,
                // jumps, climbs. Without it, want_y would only converge
                // toward default via the slow >>3 step below — at the
                // moment focus_y bumps up, want_y lags by ~1 second
                // before catching up, and look-at-focus computes a
                // shifting want_pitch (camera looking up at character)
                // → at low rubberness the angle snap to that shifting
                // pitch reads as a camera jerk on every step.
                //
                // The drift then closes the residual offset (e.g. after
                // player released Y stick at a non-default height) over
                // ~1 second per the >>3 cap rate. Drift's per-tick cap
                // is intentionally NOT scaled by rubberness — translation
                // is always smoothed.
                fc->want_y += focus_y_delta;

                SLONG goto_y = fc->focus_y + FC_focus_above(fc) + offset_height;

                if (GAME_FLAGS & GF_NO_FLOOR) {
                    if (goto_y < 0x28000) {
                        goto_y = 0x28000;
                    }
                }

                dy = goto_y - fc->want_y;

                if (dy > 0x10000) {
                    // Well below target: move faster.
                    dy >>= 3;
                    if (dy > 0x2000) {
                        dy = 0x2000;
                    }
                } else {
                    dy >>= 3;
                    if (dy > 0x0d00) {
                        dy = 0x0d00;
                    }
                    if (dy < -0x0c00) {
                        dy = -0x0c00;
                    }
                }

                // Removed: WMOVE-platform ×32 multiplier on dy. The
                // original engine had a hack that multiplied the
                // convergence step by 32 when the character was
                // standing on a face flagged FACE_FLAG_WMOVE
                // (escalators, moving lifts, cars). Intended to keep
                // the camera caught up to a rising platform, but the
                // flag is set in level data regardless of whether
                // the platform is actually moving — so a parked car
                // (also WMOVE-flagged in its walkable face) triggers
                // the ×32 multiplier and produces an overshoot when
                // the camera converges back to default after a stick
                // height-down input. Camera-convergence rate should
                // depend only on want/goto/time, not on what's
                // underfoot; if a real moving lift later looks too
                // laggy, tune the base convergence rate (dy >> 3 /
                // 0xd00 cap) instead of re-introducing surface-
                // dependent speed bumps.

                fc->want_y += dy;
            }

            s_prev_focus_y[cam] = fc->focus_y;
        }

        // Distance clamp: keep camera within [FC_DIST_MIN, FC_DIST_MAX] from focus XZ.
        dx = fc->focus_x - fc->want_x;
        dz = fc->focus_z - fc->want_z;

        dist = QDIST2(abs(dx), abs(dz));

        {
            // QDIST2 lacks accuracy; use integer sqrt instead.
            dist = (dx >> 8) * (dx >> 8) + (dz >> 8) * (dz >> 8);
            dist = Root(dist);
            dist <<= 8;
        }

        // On keyboard+mouse the XZ-distance clamp is one-sided: ONLY the
        // "too far" branch runs. The "too close" branch (push want away
        // from focus when XZ < target) would undo the mouse orbital
        // pitch on every tick — orbital deliberately shrinks XZ as the
        // camera arcs up around the focus. Skipping it preserves orbital.
        //
        // The "too far" branch is kept because it provides sprint
        // recovery: during sprint, want's XZ stays at target (translation
        // tracking preserves the offset), but `fc` lags via position
        // smoothing and the visible camera-to-focus XZ distance grows
        // (the zoom-out effect — desired immersion). When the sprint
        // ends, the position smoothing collapse threshold (`if QDIST3 <
        // 0x800 { want = fc }`) pins want at fc's lagged position. The
        // very next tick this clamp catches it (want's XZ > target),
        // pulls want back to target XZ, and fc smooths in.
        // MANUAL mode skips the "too close" clamp: manual vertical orbit
        // (mouse pitch / stick orbit) deliberately shrinks XZ as the camera
        // arcs up around the focus; min-clamping would undo that on every
        // tick. The "too far" branch below is kept for both modes — it
        // provides sprint recovery (lagged fc catches up to want after
        // sprint zoom-out ends) and that feels right on MANUAL too.
        //
        // EXCEPTION — vehicle entry: keep the min-clamp ON even in MANUAL so
        // the programmatic get-behind orbits AROUND the character (constant
        // radius) like it does on the gamepad. Without it, want is pulled in a
        // straight chord to the behind point and the camera cuts directly
        // across instead of arcing round.
        const bool min_clamp_enabled = camera_is_auto() || entering_vehicle_scene;
        if (min_clamp_enabled && dist < FC_DIST_MIN) {
            if (!fc->toonear) {
                ddist = FC_DIST_MIN - dist;

                dx >>= 6;
                dy >>= 6;
                dz >>= 6;

                dist >>= 6;
                dist += 1;

                fc->want_x -= ddist * dx / dist;
                fc->want_z -= ddist * dz / dist;
            }
        } else if (dist > FC_DIST_MAX) {
            ddist = dist - FC_DIST_MAX;

            dx >>= 8;
            dz >>= 8;

            dist >>= 8;
            dist += 1;

            fc->want_x += ((ddist >> 4) * dx / dist) << 4;
            fc->want_z += ((ddist >> 4) * dz / dist) << 4;

            fc->toonear = UC_FALSE;
        }

        // Smooth position: want → actual.
        dx = fc->want_x - fc->x;
        dy = fc->want_y - fc->y;
        dz = fc->want_z - fc->z;

        if (QDIST3(abs(dx), abs(dy), abs(dz)) > 0x800) {
            // Position smoothing — camera following the character.
            // ALWAYS on in BOTH modes (rubberness does NOT scale this):
            // even in MANUAL the camera should smoothly follow jumps,
            // stairs, platforms etc. without snapping. Rubberness is
            // specifically about rotation rubber (angle / wall / orbit
            // lag), not position rubber.
            fc->x += dx >> 2;
            fc->y += dy >> 3;
            fc->z += dz >> 2;
        } else {
            fc->want_x = fc->x;
            fc->want_y = fc->y;
            fc->want_z = fc->z;
            fc->smooth_transition = UC_FALSE;
        }

        FC_look_at_focus(fc);

        // Smooth angles: want → actual.
        //
        // MANUAL mode: zero rotation smoothing — any smoothing here is
        // perceived as a rubber-band tail on manual rotation (the mouse
        // mdx/mdy blocks above also snap fc directly to want for the same
        // reason). Just collapse fc = want every tick.
        //
        // AUTO mode: smoothing scaled by rubberness. Shipping default
        // (rubberness 0.5) keeps the legacy `>>2` step exactly — i.e.
        // 25%/tick toward want. Lower rubberness → snappier. Higher →
        // laggier rubber-band.
        //
        // EXCEPTION — vehicle entry: use the smoothed path even in MANUAL so
        // the programmatic entry rotation eases the same way it does on the
        // gamepad (AUTO). Without this the snap makes the lock-rotation look
        // abrupt on KBM.
        if (camera_is_manual() && !entering_vehicle_scene) {
            fc->yaw   = fc->want_yaw;
            fc->pitch = fc->want_pitch;
            fc->roll  = fc->want_roll;
        } else {
            SLONG dyaw = fc->want_yaw - fc->yaw;
            SLONG dpitch = fc->want_pitch - fc->pitch;
            SLONG droll = fc->want_roll - fc->roll;

            dyaw &= (2048 << 8) - 1;
            dpitch &= (2048 << 8) - 1;
            droll &= (2048 << 8) - 1;

            if (dyaw > (1024) << 8) {
                dyaw -= (2048 << 8);
            }
            if (dpitch > (1024) << 8) {
                dpitch -= (2048 << 8);
            }
            if (droll > (1024) << 8) {
                droll -= (2048 << 8);
            }

            if (QDIST3(abs(dyaw), abs(dpitch), abs(droll)) > 0x180) {
                // Legacy step was `>> 2` (drop 25% of remaining delta per
                // tick → keep 75% of the old angle). Express that as a
                // "keep" fraction so rubberness can scale it. At
                // rubberness 0.5 the helper returns exactly 0.75, the
                // multiplication below collapses to `* 0.25` = `>>2`,
                // bit-identical to the original.
                //
                // CAP at default (0.75): position smoothing is fixed at
                // `>>2` (keep 0.75) and NOT rubberness-scaled — if angle
                // smoothing went slower than position smoothing (keep
                // approaching 1.0 at high rubberness), fc.x/z would
                // reach the target orbital position several ticks before
                // fc.yaw caught up to want_yaw. Camera physically there
                // but looking in old direction → character drifts off-
                // centre during rotation. Capping at 0.75 keeps angle
                // and position smoothing rates equal, so the camera
                // tracks the look target throughout the catch-up.
                //
                // Effectively rubberness [0..0.5] scales angle smoothing
                // from snap to default; rubberness [0.5..1] leaves angle
                // at default. The extra "laziness" of rubberness > 0.5
                // is then expressed via the wall-collision blend (which
                // doesn't interact with position smoothing) and the
                // orbit-lag residual for fast mouse flicks.
                float keep = keep_for_rubberness(0.75f);
                if (keep > 0.75f) keep = 0.75f;
                const float drop = 1.0f - keep;
                fc->yaw   += (SLONG)((float)dyaw   * drop);
                fc->pitch += (SLONG)((float)dpitch * drop);
                fc->roll  += (SLONG)((float)droll  * drop);
            } else {
                fc->want_yaw = fc->yaw;
                fc->want_pitch = fc->pitch;
                fc->want_roll = fc->roll;
            }
        }

        // Fixed lens (FOV). Fight zoom is commented out in original.
        fc->lens = 0x28000 * CAM_MORE_IN;

        // Camera shake from explosions.
        if (fc->shake) {
            SLONG shake_x = (rand() % fc->shake) - (fc->shake >> 1);
            SLONG shake_y = (rand() % fc->shake) - (fc->shake >> 1);
            SLONG shake_z = (rand() % fc->shake) - (fc->shake >> 1);

            fc->x += shake_x << 7;
            fc->y += shake_y << 7;
            fc->z += shake_z << 7;

            fc->shake -= 1;
            fc->shake -= fc->shake >> 3;
        }
    }
}

// Fast LOS reject: traces from camera to a point through 5 steps.
// Returns UC_TRUE if the path is clear (none of the steps are inside geometry).
// uc_orig: FC_can_see_point (fallen/Source/fc.cpp)
SLONG FC_can_see_point(
    SLONG cam,
    SLONG x,
    SLONG y,
    SLONG z)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG i;

    SLONG dx = (fc->x >> 8) - x >> 3;
    SLONG dy = (fc->y >> 8) - y >> 3;
    SLONG dz = (fc->z >> 8) - z >> 3;

    for (i = 0; i < 5; i++) {
        x += dx;
        y += dy;
        z += dz;

        if (MAV_inside(x, y, z)) {
            return UC_FALSE;
        }
    }

    return UC_TRUE;
}

// Returns UC_TRUE if the camera can see the given person.
// Handles cut-scene camera position. Always returns UC_TRUE for climbers/danglers.
// Checks head, torso, and 4 shoulder points; UC_FALSE only if all 6 are obscured.
// uc_orig: FC_can_see_person (fallen/Source/fc.cpp)
SLONG FC_can_see_person(SLONG cam, Thing* p_person)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG x, y, z;
    SLONG old_fc_x = fc->x;
    SLONG old_fc_y = fc->y;
    SLONG old_fc_z = fc->z;
    SLONG junk;

    if (p_person->State == STATE_CLIMB_LADDER || p_person->State == STATE_DANGLING || p_person->State == STATE_CLIMBING) {
        return UC_TRUE;
    }

    // Get cut-scene camera position if active.
    EWAY_grab_camera(
        &fc->x, &fc->y, &fc->z,
        &junk, &junk, &junk, &junk);

    // Always visible if close enough.
    SLONG dx = abs(p_person->WorldPos.X - fc->x);
    SLONG dy = abs(p_person->WorldPos.Y - fc->y);
    SLONG dz = abs(p_person->WorldPos.Z - fc->z);
    SLONG dist = abs(dx) + abs(dy) + abs(dz);

    if (dist < 0x58000) {
        fc->x = old_fc_x;
        fc->y = old_fc_y;
        fc->z = old_fc_z;
        return UC_TRUE;
    }

    x = p_person->WorldPos.X >> 8;
    y = p_person->WorldPos.Y >> 8;
    z = p_person->WorldPos.Z >> 8;

    if (!FC_can_see_point(cam, x, y + FC_CSP_HEIGHT, z) && !FC_can_see_point(cam, x, y, z)) {
        if (!FC_can_see_point(cam, x + FC_CSP_RADIUS, y + FC_CSP_HEIGHT / 2, z) && !FC_can_see_point(cam, x - FC_CSP_RADIUS, y + FC_CSP_HEIGHT / 2, z) && !FC_can_see_point(cam, x, y + FC_CSP_HEIGHT / 2, z + FC_CSP_RADIUS) && !FC_can_see_point(cam, x, y + FC_CSP_HEIGHT / 2, z - FC_CSP_RADIUS)) {
            fc->x = old_fc_x;
            fc->y = old_fc_y;
            fc->z = old_fc_z;
            return UC_FALSE;
        }
    }

    fc->x = old_fc_x;
    fc->y = old_fc_y;
    fc->z = old_fc_z;

    return UC_TRUE;
}

// Sets up an over-the-shoulder view at the given pitch, entering toonear/first-person mode.
// toonear_dist=0x90000 is the sentinel value for this mode.
// uc_orig: FC_position_for_lookaround (fallen/Source/fc.cpp)
void FC_position_for_lookaround(SLONG cam, SLONG pitch)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];
    SLONG vector[3];

    FMATRIX_vector(vector, fc->focus_yaw, pitch);

    fc->want_x = fc->focus_x + (vector[0] * 3 >> 2);
    fc->want_y = fc->focus_y + 0xb000 + (vector[1] * 3 >> 2);
    fc->want_z = fc->focus_z + (vector[2] * 3 >> 2);

    fc->toonear = UC_TRUE;
    fc->toonear_dist = 0x90000;
}

// Shakes each active camera proportionally to an explosion's proximity and force.
// Shake amount decays each frame in FC_process.
// uc_orig: FC_explosion (fallen/Source/fc.cpp)
void FC_explosion(SLONG x, SLONG y, SLONG z, SLONG force)
{
    SLONG i;
    SLONG dx, dy, dz;
    SLONG dist;
    SLONG shake;

    FC_Cam* fc;

    for (i = 0; i < FC_MAX_CAMS; i++) {
        fc = &FC_cam[i];

        dx = abs((fc->x >> 8) - x);
        dy = abs((fc->y >> 8) - y);
        dz = abs((fc->z >> 8) - z);

        dist = QDIST3(dx, dy, dz);

        if (dist < force * 16) {
            shake = 256 * ((force * 16) - dist) / (force * 16);
            SATURATE(shake, 0, 255);
            fc->shake = shake;

            // uc_orig: PSX_SetShock (fallen/Source/fc.cpp:2171)
            gamepad_set_shock(0, shake);
        }
    }
}
