#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/animals/bat_globals.h"
#include "things/animals/bat.h"
#include "things/characters/anim_ids.h"
#include "things/core/statedef.h"
#include "engine/audio/sound.h"
#include "missions/eway.h"
#include "engine/effects/psystem.h"
#include "engine/graphics/pipeline/poly.h"
#include "ai/mav.h"
#include "navigation/wand.h"
#include "effects/combat/spark.h"
#include "ai/pcom.h"
#include "engine/graphics/lighting/night.h"
#include "engine/physics/collide.h"
#include "effects/combat/pyro.h"
#include "assets/formats/level_loader.h"       // load_anim_prim_object
#include "things/characters/person.h"  // set_person_dead, set_face_thing, set_person_recoil, set_person_float_up
#include "assets/formats/anim_globals.h"       // anim_chunk
#include "things/core/interact.h"      // calc_sub_objects_position

// Bitmask flags stored in Bat::flag.
// uc_orig: BAT_FLAG_ATTACKED (fallen/Source/bat.cpp)
#define BAT_FLAG_ATTACKED (1 << 0)
// uc_orig: BAT_FLAG_SYNC_FX (fallen/Source/bat.cpp)
#define BAT_FLAG_SYNC_FX  (1 << 1)
// uc_orig: BAT_FLAG_SYNC_FX2 (fallen/Source/bat.cpp)
#define BAT_FLAG_SYNC_FX2 (1 << 2)

// Timer tick rate for bat AI (not real seconds — game ticks).
// uc_orig: BAT_TICKS_PER_SECOND (fallen/Source/bat.cpp)
#define BAT_TICKS_PER_SECOND 160

// State IDs for Bat::state.
// uc_orig: BAT_STATE_IDLE (fallen/Source/bat.cpp)
#define BAT_STATE_IDLE           0
// uc_orig: BAT_STATE_GOTO (fallen/Source/bat.cpp)
#define BAT_STATE_GOTO           1
// uc_orig: BAT_STATE_CIRCLE (fallen/Source/bat.cpp)
#define BAT_STATE_CIRCLE         2
// uc_orig: BAT_STATE_ATTACK (fallen/Source/bat.cpp)
#define BAT_STATE_ATTACK         3
// uc_orig: BAT_STATE_DYING (fallen/Source/bat.cpp)
#define BAT_STATE_DYING          4
// uc_orig: BAT_STATE_DEAD (fallen/Source/bat.cpp)
#define BAT_STATE_DEAD           5
// uc_orig: BAT_STATE_GROUND (fallen/Source/bat.cpp)
#define BAT_STATE_GROUND         6
// uc_orig: BAT_STATE_RECOIL (fallen/Source/bat.cpp)
#define BAT_STATE_RECOIL         7
// uc_orig: BAT_STATE_BALROG_WANDER (fallen/Source/bat.cpp)
#define BAT_STATE_BALROG_WANDER  8
// uc_orig: BAT_STATE_BALROG_ROAR (fallen/Source/bat.cpp)
#define BAT_STATE_BALROG_ROAR    9
// uc_orig: BAT_STATE_BALROG_FOLLOW (fallen/Source/bat.cpp)
#define BAT_STATE_BALROG_FOLLOW  10
// uc_orig: BAT_STATE_BALROG_CHARGE (fallen/Source/bat.cpp)
#define BAT_STATE_BALROG_CHARGE  11
// uc_orig: BAT_STATE_BALROG_SWIPE (fallen/Source/bat.cpp)
#define BAT_STATE_BALROG_SWIPE   12
// uc_orig: BAT_STATE_BALROG_STOMP (fallen/Source/bat.cpp)
#define BAT_STATE_BALROG_STOMP   13
// uc_orig: BAT_STATE_BALROG_FIREBALL (fallen/Source/bat.cpp)
#define BAT_STATE_BALROG_FIREBALL 14
// uc_orig: BAT_STATE_BALROG_IDLE (fallen/Source/bat.cpp)
#define BAT_STATE_BALROG_IDLE    15
// uc_orig: BAT_STATE_BANE_IDLE (fallen/Source/bat.cpp)
#define BAT_STATE_BANE_IDLE      16
// uc_orig: BAT_STATE_BANE_ATTACK (fallen/Source/bat.cpp)
#define BAT_STATE_BANE_ATTACK    17
// uc_orig: BAT_STATE_BANE_START (fallen/Source/bat.cpp)
#define BAT_STATE_BANE_START     18
// uc_orig: BAT_STATE_NUMBER (fallen/Source/bat.cpp)
#define BAT_STATE_NUMBER         19

// Substate IDs for Bat::substate.
// uc_orig: BAT_SUBSTATE_NONE (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_NONE            0
// uc_orig: BAT_SUBSTATE_CIRCLE_HOME (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_CIRCLE_HOME     1
// uc_orig: BAT_SUBSTATE_CIRCLE_TARGET (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_CIRCLE_TARGET   2
// uc_orig: BAT_SUBSTATE_CIRCLE_WANT (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_CIRCLE_WANT     3
// uc_orig: BAT_SUBSTATE_GROUND_WAIT (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_GROUND_WAIT     4
// uc_orig: BAT_SUBSTATE_GROUND_WAKE_UP (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_GROUND_WAKE_UP  5
// uc_orig: BAT_SUBSTATE_GROUND_FLY_UP (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_GROUND_FLY_UP   6
// uc_orig: BAT_SUBSTATE_DEAD_INITIAL (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_DEAD_INITIAL    7
// uc_orig: BAT_SUBSTATE_DEAD_LOOP (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_DEAD_LOOP       8
// uc_orig: BAT_SUBSTATE_DEAD_FINAL (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_DEAD_FINAL      9
// uc_orig: BAT_SUBSTATE_YOMP_START (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_YOMP_START      10
// uc_orig: BAT_SUBSTATE_YOMP_MIDDLE (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_YOMP_MIDDLE     11
// uc_orig: BAT_SUBSTATE_YOMP_END (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_YOMP_END        12
// uc_orig: BAT_SUBSTATE_FIREBALL_TURN (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_FIREBALL_TURN   13
// uc_orig: BAT_SUBSTATE_FIREBALL_FIRE (fallen/Source/bat.cpp)
#define BAT_SUBSTATE_FIREBALL_FIRE   14

// Animation indices per bat type.
// uc_orig: BAT_ANIM_BAT_FLY (fallen/Source/bat.cpp)
#define BAT_ANIM_BAT_FLY                1
// uc_orig: BAT_ANIM_BAT_DIE (fallen/Source/bat.cpp)
#define BAT_ANIM_BAT_DIE                2
// uc_orig: BAT_ANIM_GARGOYLE_WAIT (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_WAIT          1
// uc_orig: BAT_ANIM_GARGOYLE_WAKE_UP (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_WAKE_UP       15
// uc_orig: BAT_ANIM_GARGOYLE_FLY_UP (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_FLY_UP        3
// uc_orig: BAT_ANIM_GARGOYLE_FLY (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_FLY           2
// uc_orig: BAT_ANIM_GARGOYLE_FLY_FORWARDS (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_FLY_FORWARDS  12
// uc_orig: BAT_ANIM_GARGOYLE_ATTACK (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_ATTACK        4
// uc_orig: BAT_ANIM_GARGOYLE_TAKE_HIT (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_TAKE_HIT      14
// uc_orig: BAT_ANIM_GARGOYLE_START_FALL (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_START_FALL    16
// uc_orig: BAT_ANIM_GARGOYLE_FALL_LOOP (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_FALL_LOOP     17
// uc_orig: BAT_ANIM_GARGOYLE_HIT_GROUND (fallen/Source/bat.cpp)
#define BAT_ANIM_GARGOYLE_HIT_GROUND    18
// uc_orig: BAT_ANIM_BALROG_YOMP (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_YOMP            2
// uc_orig: BAT_ANIM_BALROG_IDLE (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_IDLE            3
// uc_orig: BAT_ANIM_BALROG_SWIPE (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_SWIPE           4
// uc_orig: BAT_ANIM_BALROG_YOMP_START (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_YOMP_START      5
// uc_orig: BAT_ANIM_BALROG_YOMP_END (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_YOMP_END        6
// uc_orig: BAT_ANIM_BALROG_TURN (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_TURN            8
// uc_orig: BAT_ANIM_BALROG_ROAR (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_ROAR            9
// uc_orig: BAT_ANIM_BALROG_STOMP (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_STOMP           10
// uc_orig: BAT_ANIM_BALROG_TAKE_HIT (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_TAKE_HIT        11
// uc_orig: BAT_ANIM_BALROG_DIE (fallen/Source/bat.cpp)
#define BAT_ANIM_BALROG_DIE             12
// uc_orig: BAT_ANIM_BANE_IDLE (fallen/Source/bat.cpp)
#define BAT_ANIM_BANE_IDLE              2
// uc_orig: BAT_ANIM_BANE_ATTACK (fallen/Source/bat.cpp)
#define BAT_ANIM_BANE_ATTACK            3

// Generic animation placeholders resolved at runtime based on bat type.
// uc_orig: BAT_ANIM_GENERIC_FLY (fallen/Source/bat.cpp)
#define BAT_ANIM_GENERIC_FLY      (-1)
// uc_orig: BAT_ANIM_GENERIC_TAKE_HIT (fallen/Source/bat.cpp)
#define BAT_ANIM_GENERIC_TAKE_HIT (-2)

// Width constant for Balrog building-avoidance collision (fixed-point 16.16 units).
// uc_orig: BAT_BALROG_WIDTH (fallen/Source/bat.cpp)
#define BAT_BALROG_WIDTH (0x3000)

// Definitions of globals are in bat_globals.cpp.
// Declarations are in bat_globals.h (included via bat.h → bat_globals.h).

// uc_orig: BAT_find_summon_people (fallen/Source/bat.cpp)
// Scans a sphere of radius 0x800 around the Bane boss for dead/inactive civilians
// (pcom_ai == NONE or SUICIDE, PlayerID == 0) and fills BAT_summon[] with up to 4 of them.
void BAT_find_summon_people(Thing* p_thing)
{
    SLONG i;
    SLONG num;
    SLONG bodies;

    num = THING_find_sphere(
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Y >> 8,
        p_thing->WorldPos.Z >> 8,
        0x800,
        THING_array,
        THING_ARRAY_SIZE,
        1 << CLASS_PERSON);

    memset(BAT_summon, 0, sizeof(BAT_summon));

    bodies = 0;

    for (i = 0; i < num; i++) {
        Thing* p_found = TO_THING(THING_array[i]);

        ASSERT(p_found->Class == CLASS_PERSON);

        if (p_found->Genus.Person->pcom_ai == PCOM_AI_NONE || p_found->Genus.Person->pcom_ai == PCOM_AI_SUICIDE) {
            if (p_found->Genus.Person->PlayerID == 0) {
                ASSERT(WITHIN(bodies, 0, BAT_SUMMON_NUM_BODIES - 1));

                BAT_summon[bodies] = THING_array[i];

                bodies += 1;

                if (bodies == BAT_SUMMON_NUM_BODIES) {
                    break;
                }
            }
        }
    }
}

// uc_orig: BAT_process_bane_sparks (fallen/Source/bat.cpp)
// Creates SPARK beams from the Bane boss to each summoned body once every ~5 seconds
// (16 * 20 * 5 accumulated ticks). Connects Bane's limb 0 to each body's pelvis limb.
void BAT_process_bane_sparks(Thing* p_thing)
{
    SLONG i;

    static UWORD last = 0;

    last += 16 * TICK_RATIO >> TICK_SHIFT;

    if (last > 16 * 20 * 5) {
        last = 0;

        for (i = 0; i < BAT_SUMMON_NUM_BODIES; i++) {
            if (BAT_summon[i]) {
                Thing* p_summon = TO_THING(BAT_summon[i]);

                SPARK_Pinfo p1;
                SPARK_Pinfo p2;

                p1.type = SPARK_TYPE_LIMB;
                p1.flag = 0;
                p1.person = THING_NUMBER(p_thing);
                p1.limb = 0;

                p2.type = SPARK_TYPE_LIMB;
                p2.flag = 0;
                p2.person = THING_NUMBER(p_summon);
                p2.limb = SUB_OBJECT_PELVIS;

                SPARK_create(
                    &p1,
                    &p2,
                    255);
            }
        }
    }
}

// uc_orig: BAT_init (fallen/Headers/bat.h)
// Clears all bat slots and resets the bat count.
void BAT_init()
{
    memset(the_game.Bats, 0, sizeof(Bat) * BAT_MAX_BATS);

    BAT_COUNT = 0;
}

// uc_orig: BAT_set_anim (fallen/Source/bat.cpp)
// Sets the active animation on the bat's DrawTween.
// Negative anim indices are generic placeholders resolved via a per-type lookup table:
//   BAT_ANIM_GENERIC_FLY(-1), BAT_ANIM_GENERIC_TAKE_HIT(-2).
// No-op if the animation is already playing.
void BAT_set_anim(Thing* p_thing, SLONG anim)
{
    if (anim < 0) {
        static UBYTE generic_bat_anim[2][2] = {
            { BAT_ANIM_BAT_FLY, BAT_ANIM_BAT_FLY },
            { BAT_ANIM_GARGOYLE_FLY, BAT_ANIM_GARGOYLE_TAKE_HIT }
        };

        ASSERT(WITHIN(p_thing->Genus.Bat->type - 1, 0, 1));
        ASSERT(WITHIN(-anim - 1, 0, 1));

        anim = generic_bat_anim[p_thing->Genus.Bat->type - 1][-anim - 1];
    }

    DrawTween* dt = p_thing->Draw.Tweened;

    if (dt->CurrentAnim == anim) {
        return;
    }

    dt->AnimTween = 0;
    dt->QueuedFrame = NULL;
    dt->TheChunk = &anim_chunk[p_thing->Genus.Bat->type];
    dt->CurrentFrame = anim_chunk[p_thing->Genus.Bat->type].AnimList[anim];
    dt->NextFrame = dt->CurrentFrame->NextFrame;
    dt->CurrentAnim = anim;
    dt->FrameIndex = 0;

    if (dt->NextFrame == NULL) {
        dt->NextFrame = dt->CurrentFrame->NextFrame;
    }

    p_thing->Genus.Bat->flag &= ~(BAT_FLAG_SYNC_FX | BAT_FLAG_SYNC_FX2);
}

// uc_orig: BAT_animate (fallen/Source/bat.cpp)
// Advances the DrawTween animation by TICK_RATIO-scaled steps.
// TweenStep from the current keyframe is doubled then scaled.
// Returns UC_TRUE if the animation has reached its last frame.
SLONG BAT_animate(Thing* p_thing)
{
    SLONG ret = UC_FALSE;
    SLONG tween_step;

    DrawTween* dt = p_thing->Draw.Tweened;

    tween_step = dt->CurrentFrame->TweenStep << 1;
    tween_step = tween_step * TICK_RATIO >> TICK_SHIFT;

    if (tween_step <= 0) {
        tween_step = 1;
    }

    dt->AnimTween += tween_step;

    while (dt->AnimTween >= 256) {
        dt->AnimTween -= 256;

        if (dt->NextFrame) {
            dt->AnimTween = (dt->AnimTween * dt->NextFrame->TweenStep) / dt->CurrentFrame->TweenStep;
        }

        dt->FrameIndex++;

        // advance_keyframe is defined in old/Darci.cpp (not yet migrated).
        SLONG advance_keyframe(DrawTween * draw_info);

        ret |= advance_keyframe(dt);
    }

    return ret;
}

// uc_orig: BAT_turn_to_target (fallen/Source/bat.cpp)
// Steers the bat toward its current target using angular interpolation.
// Returns the raw angular error (dangle); near-zero means aligned with target.
SLONG BAT_turn_to_target(Thing* p_thing)
{
    SLONG dx;
    SLONG dz;

    SLONG wangle;
    SLONG dangle;

    SLONG turn;

    Thing* p_target;
    Bat* p_bat;

    ASSERT(p_thing->Class == CLASS_BAT);

    p_bat = p_thing->Genus.Bat;
    p_target = TO_THING(p_bat->target);

    ASSERT(p_bat->target);

    dx = p_target->WorldPos.X - p_thing->WorldPos.X >> 8;
    dz = p_target->WorldPos.Z - p_thing->WorldPos.Z >> 8;

    wangle = calc_angle(dx, dz);

    if (p_bat->type == BAT_TYPE_BALROG) {
        // Balrog model faces backward, so compensate by +180 degrees (1024 in 2048-unit circle).
        wangle += 1024;
    }

    dangle = wangle - p_thing->Draw.Tweened->Angle;

    if (dangle > +1024) {
        dangle -= 2048;
    }
    if (dangle < -1024) {
        dangle += 2048;
    }

    turn = dangle >> 4;

    SATURATE(turn, -20, +20);

    p_thing->Draw.Tweened->Angle += turn;
    p_thing->Draw.Tweened->Angle &= 2047;

    return dangle;
}

// uc_orig: BAT_turn_to_place (fallen/Source/bat.cpp)
// Steers the bat toward a specific world position (map units).
// Returns the angular error (dangle); near-zero means aligned with the target point.
SLONG BAT_turn_to_place(Thing* p_thing, SLONG world_x, SLONG world_z)
{
    SLONG dx;
    SLONG dz;

    SLONG wangle;
    SLONG dangle;

    SLONG turn;

    Bat* p_bat;

    ASSERT(p_thing->Class == CLASS_BAT);

    p_bat = p_thing->Genus.Bat;

    dx = world_x - (p_thing->WorldPos.X >> 8);
    dz = world_z - (p_thing->WorldPos.Z >> 8);

    wangle = calc_angle(dx, dz);

    if (p_bat->type == BAT_TYPE_BALROG) {
        // Balrog model faces backward, so compensate by +180 degrees.
        wangle += 1024;
    }

    dangle = wangle - p_thing->Draw.Tweened->Angle;

    if (dangle > +1024) {
        dangle -= 2048;
    }
    if (dangle < -1024) {
        dangle += 2048;
    }

    turn = dangle >> 4;

    SATURATE(turn, -20, +20);

    p_thing->Draw.Tweened->Angle += turn;
    p_thing->Draw.Tweened->Angle &= 2047;

    return dangle;
}

// uc_orig: BAT_emit_fireball (fallen/Source/bat.cpp)
// Launches a POLY_PAGE_METEOR particle from the Balrog toward its current target.
// Direction is normalized and randomized slightly in Y. Uses PFLAG_EXPLODE_ON_IMPACT.
void BAT_emit_fireball(Thing* p_thing)
{
    ASSERT(p_thing->Class == CLASS_BAT);

    Bat* p_bat = p_thing->Genus.Bat;

    ASSERT(p_bat->target);

    Thing* p_target = TO_THING(p_bat->target);

    SLONG dx;
    SLONG dy;
    SLONG dz;

    dx = p_target->WorldPos.X - p_thing->WorldPos.X;
    dy = p_target->WorldPos.Y + 0x3000 - p_thing->WorldPos.Y;
    dz = p_target->WorldPos.Z - p_thing->WorldPos.Z;

    SLONG dist = (QDIST3(abs(dx), abs(dy), abs(dz)) >> 8) + 1;

    dx = 0x40 * dx / dist;
    dy = 0x40 * dy / dist;
    dz = 0x40 * dz / dist;

    dy += Random() & 0x3ff;

    PARTICLE_Add(
        p_thing->WorldPos.X,
        p_thing->WorldPos.Y + 0x5000,
        p_thing->WorldPos.Z,
        dx,
        dy,
        dz,
        POLY_PAGE_METEOR,
        2 + ((Random() & 0x3) << 2),
        0xffffffff,
        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_EXPLODE_ON_IMPACT | PFLAG_GRAVITY | PFLAG_LEAVE_TRAIL,
        100,
        160,
        1,
        1,
        1);

    MFX_play_thing(THING_NUMBER(p_thing), S_BALROG_FIREBALL, 0, p_thing);
}

// uc_orig: BAT_change_state (fallen/Source/bat.cpp)
// Selects a new AI state for the bat/gargoyle/balrog/bane.
// For Balrog: scans radius 0xa00 for all people+vehicles, scores by distance
// (Y weighted 2x), applies hysteresis for current target and player preference.
// Also checks reachability via MAV_do before scoring.
void BAT_change_state(Thing* p_thing)
{
    ASSERT(p_thing->Class == CLASS_BAT);

    SLONG want_x;
    SLONG want_y;
    SLONG want_z;

    SLONG new_state;
    SLONG old_target;

    Bat* p_bat = p_thing->Genus.Bat;
    Thing* darci;

    old_target = p_bat->target;

    SLONG dangle = 0;

    if (p_bat->type == BAT_TYPE_BALROG) {
        if (p_bat->target) {
            Thing* p_target = TO_THING(p_bat->target);

            extern SLONG is_person_dead(Thing * p_person);

            if (p_target->Class == CLASS_PERSON && is_person_dead(p_target)) {
                // Track Balrog kills via EWAY counter index 8 (mission scripting hook).
                extern UBYTE* EWAY_counter;

                if (EWAY_counter[8] < 255) {
                    EWAY_counter[8] += 1;
                }
            }
        }

        SLONG i;

        SLONG dx;
        SLONG dy;
        SLONG dz;
        SLONG dist;
        SLONG score;

        SLONG best_score = UC_INFINITY;
        SLONG best_dist = 0;
        SLONG best_index = NULL;

        Thing* p_found;

        SLONG found_num = THING_find_sphere(
            p_thing->WorldPos.X >> 8,
            p_thing->WorldPos.Y >> 8,
            p_thing->WorldPos.Z >> 8,
            0xa00,
            THING_array,
            THING_ARRAY_SIZE,
            (1 << CLASS_PERSON) | (1 << CLASS_VEHICLE));

        for (i = 0; i < found_num; i++) {
            p_found = TO_THING(THING_array[i]);

            if (p_found == p_thing) {
                continue;
            }

            if (p_found->State == STATE_DEAD || p_found->State == STATE_DYING) {
                continue;
            }

            if (p_found->Class == CLASS_PERSON) {
                extern SLONG is_person_ko(Thing * p_person);

                if (is_person_ko(p_found)) {
                    continue;
                }
            }

            // Only target things the Balrog can actually reach via MAV pathfinding.
            MAV_do(
                p_thing->WorldPos.X >> 16,
                p_thing->WorldPos.Z >> 16,
                p_found->WorldPos.X >> 16,
                p_found->WorldPos.Z >> 16,
                MAV_CAPS_GOTO);

            if (MAV_do_found_dest) {
                dx = abs(p_thing->WorldPos.X - p_found->WorldPos.X);
                dy = abs(p_thing->WorldPos.Y - p_found->WorldPos.Y);
                dz = abs(p_thing->WorldPos.Z - p_found->WorldPos.Z);

                dist = dx + dy + dy + dz >> 8; // Y weighted 2x (intentional)

                score = dist;

                if (p_bat->target == THING_array[i]) {
                    // Hysteresis: prefer current target.
                    score >>= 1;
                }

                if (p_found->Genus.Person->PlayerID) {
                    // Players are priority targets.
                    score >>= 1;
                }

                if (best_score > score) {
                    best_score = score;
                    best_index = THING_array[i];
                    best_dist = dist;
                }
            }
        }

        p_bat->target = best_index;

        if (p_bat->target) {
            dangle = abs(BAT_turn_to_target(p_thing));

            if (p_bat->target != old_target) {
                // Roar when acquiring a new target.
                new_state = BAT_STATE_BALROG_ROAR;
            } else {
                if (best_dist < 0x200 && dangle < 256) {
                    new_state = BAT_STATE_BALROG_SWIPE;
                } else if (best_dist < 0x400) {
                    if (Random() & 0x40) {
                        new_state = BAT_STATE_BALROG_STOMP;
                    } else {
                        new_state = BAT_STATE_BALROG_FOLLOW;
                    }
                } else {
                    if (Random() & 0x20) {
                        if ((Random() & 0x3) == 0) {
                            new_state = BAT_STATE_BALROG_CHARGE;
                        } else {
                            new_state = BAT_STATE_BALROG_FIREBALL;
                        }
                    } else {
                        new_state = BAT_STATE_BALROG_FOLLOW;
                    }
                }
            }
        } else {
            if (p_bat->state == BAT_STATE_BALROG_FIREBALL || p_bat->state == BAT_STATE_BALROG_CHARGE || p_bat->state == BAT_STATE_BALROG_SWIPE || p_bat->state == BAT_STATE_BALROG_STOMP) {
                // Roar after a successful attack sequence.
                new_state = BAT_STATE_BALROG_ROAR;
            } else if ((Random() & 0x3) == 0) {
                if (Random() & 0x2) {
                    new_state = BAT_STATE_BALROG_ROAR;
                } else {
                    new_state = BAT_STATE_BALROG_IDLE;
                }
            } else {
                new_state = BAT_STATE_BALROG_WANDER;
            }
        }
    } else if (p_bat->type == BAT_TYPE_BANE) {
        p_bat->target = NULL;

        if (BAT_summon[0] == NULL) {
            new_state = BAT_STATE_BANE_START;
        } else {
            if ((Random() & 0x3) == 0) {
                new_state = BAT_STATE_BANE_ATTACK;
            } else {
                new_state = BAT_STATE_BANE_IDLE;
            }
        }
    } else {
        // Bat / gargoyle: target player if within range 0x600.
        darci = NET_PERSON(0);

        if (THING_dist_between(darci, p_thing) < 0x600) {
            p_bat->target = THING_NUMBER(darci);
        } else {
            p_bat->target = NULL;
        }

        SLONG dx = p_thing->WorldPos.X - ((p_bat->home_x << 16) + 0x8000);
        SLONG dz = p_thing->WorldPos.Z - ((p_bat->home_z << 16) + 0x8000);

        if (abs(dx) > 0x40000 || abs(dz) > 0x40000) {
            // Too far from home — go back.
            new_state = BAT_STATE_GOTO;
        } else {
            new_state = Random() % 3;

            if (p_bat->type == BAT_TYPE_GARGOYLE && new_state == BAT_STATE_CIRCLE && p_bat->target != NULL) {
                new_state = BAT_STATE_ATTACK;
            }
        }
    }

    p_bat->state = new_state;
    p_bat->flag &= ~BAT_FLAG_ATTACKED;

    switch (new_state) {
    case BAT_STATE_IDLE:

        p_bat->timer = BAT_TICKS_PER_SECOND * (1 + (Random() & 0x1));

        BAT_set_anim(p_thing, BAT_ANIM_GENERIC_FLY);

        break;

    case BAT_STATE_GOTO:

        want_x = p_bat->home_x << 8;
        want_z = p_bat->home_z << 8;

        want_x += Random() & 0x3ff;
        want_z += Random() & 0x3ff;

        want_x -= 0x1ff;
        want_z -= 0x1ff;

        want_y = PAP_calc_map_height_at(want_x, want_z) + 0x80 + (Random() & 0x7f);

        p_bat->want_x = want_x;
        p_bat->want_y = want_y;
        p_bat->want_z = want_z;

        p_bat->timer = BAT_TICKS_PER_SECOND * (2 + (Random() & 0x1));

        BAT_set_anim(p_thing, BAT_ANIM_GENERIC_FLY);

        break;

    case BAT_STATE_CIRCLE:

        if (p_bat->target) {
            p_bat->substate = BAT_SUBSTATE_CIRCLE_TARGET;
        } else {
            p_bat->substate = BAT_SUBSTATE_CIRCLE_HOME;
        }

        p_bat->timer = BAT_TICKS_PER_SECOND * (3 + (Random() & 0x3));

        BAT_set_anim(p_thing, BAT_ANIM_GENERIC_FLY);

        break;

    case BAT_STATE_ATTACK:

        ASSERT(p_bat->type == BAT_TYPE_GARGOYLE);

        BAT_set_anim(p_thing, BAT_ANIM_GARGOYLE_ATTACK);

        break;

    case BAT_STATE_BALROG_WANDER:

    {
        SLONG want_x;
        SLONG want_z;

        MAV_Action ma;

        WAND_get_next_place(
            p_thing,
            &want_x,
            &want_z);

        ma = MAV_do(
            p_thing->WorldPos.X >> 16,
            p_thing->WorldPos.Z >> 16,
            want_x >> 8,
            want_z >> 8,
            MAV_CAPS_GOTO);

        p_bat->want_x = (ma.dest_x << 8) + 0x80;
        p_bat->want_z = (ma.dest_z << 8) + 0x80;

        p_bat->timer = BAT_TICKS_PER_SECOND * (2 + (Random() & 0x1));

        BAT_set_anim(p_thing, BAT_ANIM_BALROG_YOMP_START);

        p_bat->substate = BAT_SUBSTATE_YOMP_START;
    }

    break;

    case BAT_STATE_BALROG_ROAR:

        BAT_set_anim(p_thing, BAT_ANIM_BALROG_ROAR);

        MFX_play_thing(THING_NUMBER(p_thing), S_BALROG_ROAR, 0, p_thing);

        p_bat->dx = 0;
        p_bat->dz = 0;

        break;

    case BAT_STATE_BALROG_CHARGE:
    case BAT_STATE_BALROG_FOLLOW:

        ASSERT(p_bat->target);

        BAT_set_anim(p_thing, BAT_ANIM_BALROG_YOMP_START);

        p_bat->timer = BAT_TICKS_PER_SECOND * (2 + (Random() & 0x1));

        p_bat->substate = BAT_SUBSTATE_YOMP_START;

        break;

    case BAT_STATE_BALROG_SWIPE:
        BAT_set_anim(p_thing, BAT_ANIM_BALROG_SWIPE);
        MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_BALROG_GROWL_START, S_BALROG_GROWL_END), 0, p_thing);
        p_bat->dx = 0;
        p_bat->dz = 0;
        break;

    case BAT_STATE_BALROG_STOMP:
        BAT_set_anim(p_thing, BAT_ANIM_BALROG_STOMP);
        MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_BALROG_GROWL_START, S_BALROG_GROWL_END), 0, p_thing);
        p_bat->dx = 0;
        p_bat->dz = 0;
        break;

    case BAT_STATE_BALROG_FIREBALL:

        if (dangle < 256) {
            BAT_set_anim(p_thing, BAT_ANIM_BALROG_SWIPE);

            p_bat->substate = BAT_SUBSTATE_FIREBALL_FIRE;
            MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_BALROG_GROWL_START, S_BALROG_GROWL_END), 0, p_thing);
        } else {
            BAT_set_anim(p_thing, BAT_ANIM_BALROG_TURN);

            p_bat->substate = BAT_SUBSTATE_FIREBALL_TURN;
        }

        p_bat->dx = 0;
        p_bat->dz = 0;

        break;

    case BAT_STATE_BALROG_IDLE:
        BAT_set_anim(p_thing, BAT_ANIM_BALROG_IDLE);
        p_bat->timer = BAT_TICKS_PER_SECOND * (2 + (Random() & 0x3));
        p_bat->dx = 0;
        p_bat->dz = 0;
        break;

    case BAT_STATE_BANE_IDLE:
        BAT_set_anim(p_thing, BAT_ANIM_BANE_IDLE);
        p_bat->timer = BAT_TICKS_PER_SECOND * (3 + (Random() & 0x3));
        p_bat->dx = 0;
        p_bat->dz = 0;
        break;

    case BAT_STATE_BANE_ATTACK:
        BAT_set_anim(p_thing, BAT_ANIM_BANE_ATTACK);
        p_bat->dx = 0;
        p_bat->dz = 0;
        break;

    case BAT_STATE_BANE_START:

        BAT_find_summon_people(p_thing);

        {
            SLONG i;

            for (i = 0; i < BAT_SUMMON_NUM_BODIES; i++) {
                if (BAT_summon[i]) {
                    Thing* p_summon = TO_THING(BAT_summon[i]);

                    set_person_float_up(p_summon);
                }
            }
        }

        BAT_set_anim(p_thing, BAT_ANIM_BANE_IDLE);

        MFX_play_thing(THING_NUMBER(p_thing), S_RECKONING_LOOP, MFX_LOOPED, p_thing);

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: BAT_balrog_slide_along (fallen/Source/bat.cpp)
// Custom building-avoidance collision for the Balrog.
// Uses MAV_opt traversability flags for edge collision and PAP_FLAG_HIDDEN for building cells.
// Width constant BAT_BALROG_WIDTH (0x3000) is in 16.16 fixed-point units.
void BAT_balrog_slide_along(
    SLONG x1, SLONG z1,
    SLONG* x2, SLONG* z2)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;
    ULONG collide;

    SLONG mx = *x2 >> 16;
    SLONG mz = *z2 >> 16;

    ASSERT(WITHIN(mx, 1, PAP_SIZE_HI - 2));
    ASSERT(WITHIN(mz, 1, PAP_SIZE_HI - 2));

// uc_orig: BAT_COLLIDE_XS (fallen/Source/bat.cpp)
#define BAT_COLLIDE_XS (1 << 0)
// uc_orig: BAT_COLLIDE_XL (fallen/Source/bat.cpp)
#define BAT_COLLIDE_XL (1 << 1)
// uc_orig: BAT_COLLIDE_ZS (fallen/Source/bat.cpp)
#define BAT_COLLIDE_ZS (1 << 2)
// uc_orig: BAT_COLLIDE_ZL (fallen/Source/bat.cpp)
#define BAT_COLLIDE_ZL (1 << 3)
// uc_orig: BAT_COLLIDE_SS (fallen/Source/bat.cpp)
#define BAT_COLLIDE_SS (1 << 4)
// uc_orig: BAT_COLLIDE_LS (fallen/Source/bat.cpp)
#define BAT_COLLIDE_LS (1 << 5)
// uc_orig: BAT_COLLIDE_SL (fallen/Source/bat.cpp)
#define BAT_COLLIDE_SL (1 << 6)
// uc_orig: BAT_COLLIDE_LL (fallen/Source/bat.cpp)
#define BAT_COLLIDE_LL (1 << 7)

    MAV_Opt mo = MAV_opt[MAV_NAV(mx, mz)];

    collide = 0;

    if (!(mo.opt[MAV_DIR_XS] & (MAV_CAPS_GOTO))) {
        collide |= BAT_COLLIDE_XS;
    }
    if (!(mo.opt[MAV_DIR_XL] & (MAV_CAPS_GOTO))) {
        collide |= BAT_COLLIDE_XL;
    }
    if (!(mo.opt[MAV_DIR_ZS] & (MAV_CAPS_GOTO))) {
        collide |= BAT_COLLIDE_ZS;
    }
    if (!(mo.opt[MAV_DIR_ZL] & (MAV_CAPS_GOTO))) {
        collide |= BAT_COLLIDE_ZL;
    }

    // If the Balrog is on top of a building (PAP_FLAG_HIDDEN at his current
    // tile), skip mapsquare collision entirely — otherwise pull mapsquare
    // collision flags from the 8 neighbouring tiles.
    if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
        // Balrog is on a building — skip mapsquare collision.
    } else {
        if (PAP_2HI(mx + 1, mz + 0).Flags & PAP_FLAG_HIDDEN) {
            collide |= BAT_COLLIDE_XL;
        }
        if (PAP_2HI(mx - 1, mz + 0).Flags & PAP_FLAG_HIDDEN) {
            collide |= BAT_COLLIDE_XS;
        }
        if (PAP_2HI(mx + 0, mz + 1).Flags & PAP_FLAG_HIDDEN) {
            collide |= BAT_COLLIDE_ZL;
        }
        if (PAP_2HI(mx + 0, mz - 1).Flags & PAP_FLAG_HIDDEN) {
            collide |= BAT_COLLIDE_ZS;
        }

        if (PAP_2HI(mx + 1, mz + 1).Flags & PAP_FLAG_HIDDEN) {
            collide |= BAT_COLLIDE_LL;
        }
        if (PAP_2HI(mx - 1, mz + 1).Flags & PAP_FLAG_HIDDEN) {
            collide |= BAT_COLLIDE_SL;
        }
        if (PAP_2HI(mx + 1, mz - 1).Flags & PAP_FLAG_HIDDEN) {
            collide |= BAT_COLLIDE_LS;
        }
        if (PAP_2HI(mx - 1, mz - 1).Flags & PAP_FLAG_HIDDEN) {
            collide |= BAT_COLLIDE_SS;
        }
    }

    if (collide & BAT_COLLIDE_XS) {
        if ((*x2 & 0xffff) < BAT_BALROG_WIDTH) {
            *x2 &= ~0xffff;
            *x2 |= BAT_BALROG_WIDTH;
        }
    }

    if (collide & BAT_COLLIDE_XL) {
        if ((*x2 & 0xffff) > 0x10000 - BAT_BALROG_WIDTH) {
            *x2 &= ~0xffff;
            *x2 |= 0x10000 - BAT_BALROG_WIDTH;
        }
    }

    if (collide & BAT_COLLIDE_ZS) {
        if ((*z2 & 0xffff) < BAT_BALROG_WIDTH) {
            *z2 &= ~0xffff;
            *z2 |= BAT_BALROG_WIDTH;
        }
    }

    if (collide & BAT_COLLIDE_ZL) {
        if ((*z2 & 0xffff) > 0x10000 - BAT_BALROG_WIDTH) {
            *z2 &= ~0xffff;
            *z2 |= 0x10000 - BAT_BALROG_WIDTH;
        }
    }

    if (collide & BAT_COLLIDE_SS) {
        dx = *x2 & 0xffff;
        dz = *z2 & 0xffff;

        dist = QDIST2(dx, dz) + 1;

        if (dist < BAT_BALROG_WIDTH) {
            dx = dx * BAT_BALROG_WIDTH / dist;
            dz = dz * BAT_BALROG_WIDTH / dist;
        }

        *x2 &= ~0xffff;
        *z2 &= ~0xffff;

        *x2 |= dx;
        *z2 |= dz;
    }

    if (collide & BAT_COLLIDE_LS) {
        dx = *x2 & 0xffff;
        dz = *z2 & 0xffff;

        dx = 0x10000 - dx;

        dist = QDIST2(dx, dz) + 1;

        if (dist < BAT_BALROG_WIDTH) {
            dx = dx * BAT_BALROG_WIDTH / dist;
            dz = dz * BAT_BALROG_WIDTH / dist;
        }

        dx = 0x10000 - dx;

        *x2 &= ~0xffff;
        *z2 &= ~0xffff;

        *x2 |= dx;
        *z2 |= dz;
    }

    if (collide & BAT_COLLIDE_SL) {
        dx = *x2 & 0xffff;
        dz = *z2 & 0xffff;

        dz = 0x10000 - dz;

        dist = QDIST2(dx, dz) + 1;

        if (dist < BAT_BALROG_WIDTH) {
            dx = dx * BAT_BALROG_WIDTH / dist;
            dz = dz * BAT_BALROG_WIDTH / dist;
        }

        dz = 0x10000 - dz;

        *x2 &= ~0xffff;
        *z2 &= ~0xffff;

        *x2 |= dx;
        *z2 |= dz;
    }

    if (collide & BAT_COLLIDE_LS) {
        dx = *x2 & 0xffff;
        dz = *z2 & 0xffff;

        dx = 0x10000 - dx;
        dz = 0x10000 - dz;

        dist = QDIST2(dx, dz) + 1;

        if (dist < BAT_BALROG_WIDTH) {
            dx = dx * BAT_BALROG_WIDTH / dist;
            dz = dz * BAT_BALROG_WIDTH / dist;
        }

        dz = 0x10000 - dz;

        *x2 &= ~0xffff;
        *z2 &= ~0xffff;

        *x2 |= dx;
        *z2 |= dz;
    }
}

// Forward declaration for there_is_a_los_mav — defined in collide.cpp, not declared in any header.
// uc_orig: there_is_a_los_mav (fallen/Source/collide.cpp)
extern SLONG there_is_a_los_mav(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2);

// uc_orig: BAT_normal (fallen/Source/bat.cpp)
// Per-frame update for all bat-class entities (bat/gargoyle/balrog/bane).
// Called as the Thing::StateFn. Runs the state machine, moves the thing, and ticks the timer.
void BAT_normal(Thing* p_thing)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;

    SLONG ddx;
    SLONG ddz;

    SLONG want_x;
    SLONG want_y;
    SLONG want_z;

    SLONG want_dx;
    SLONG want_dy;
    SLONG want_dz;

    SLONG wangle;
    SLONG dangle;

    SLONG ground;

    SLONG end;

    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    GameCoord newpos;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    SLONG ticks = (BAT_TICKS_PER_SECOND / 20) * TICK_RATIO >> TICK_SHIFT;

    ASSERT(p_thing->Class == CLASS_BAT);
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    ;
    Bat* p_bat = p_thing->Genus.Bat;
    ;
    ;
    ;
    ;
    ;
    Thing* p_target;
    ;
    ;

    // make some batty sounds. if we're not a gargoyle. or a balrog. or bane.
    if (p_bat->type == BAT_TYPE_BAT) {
        if (!(Random() & 0xff))
            MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_BAT_SQUEEK_START, S_BAT_SQUEEK_END), MFX_NEVER_OVERLAP, p_thing);
    }

    switch (p_bat->state) {
    case BAT_STATE_IDLE:

        p_bat->dx -= p_bat->dx / 16;
        p_bat->dy -= p_bat->dy / 16;
        p_bat->dz -= p_bat->dz / 16;

        if (p_bat->target) {
            BAT_turn_to_target(p_thing);
        }

        end = BAT_animate(p_thing);

        break;

    case BAT_STATE_GOTO:

        want_dx = p_bat->want_x - (p_thing->WorldPos.X >> 8) << 5;
        want_dy = p_bat->want_y - (p_thing->WorldPos.Y >> 8) << 5;
        want_dz = p_bat->want_z - (p_thing->WorldPos.Z >> 8) << 5;

        SATURATE(want_dx, -0x2000, +0x2000);
        SATURATE(want_dy, -0x0800, +0x0800);
        SATURATE(want_dz, -0x2000, +0x2000);

        p_bat->dx += want_dx - p_bat->dx >> 4;
        p_bat->dy += want_dy - p_bat->dy >> 4;
        p_bat->dz += want_dz - p_bat->dz >> 4;

        p_bat->dx -= p_bat->dx / 16;
        p_bat->dy -= p_bat->dy / 16;
        p_bat->dz -= p_bat->dz / 16;

        if (p_bat->target) {
            BAT_turn_to_target(p_thing);
        }

        end = BAT_animate(p_thing);

        break;

    case BAT_STATE_CIRCLE:

        switch (p_bat->substate) {
        case BAT_SUBSTATE_CIRCLE_HOME:
            want_x = (p_bat->home_x << 8) + 0x80;
            want_z = (p_bat->home_z << 8) + 0x80;
            want_y = PAP_calc_map_height_at(want_x, want_z) + 0x100;
            break;

        case BAT_SUBSTATE_CIRCLE_WANT:
            want_x = p_bat->want_x;
            want_y = p_bat->want_y;
            want_z = p_bat->want_z;
            break;

        case BAT_SUBSTATE_CIRCLE_TARGET:

            ASSERT(p_bat->target);

            p_target = TO_THING(p_bat->target);

            switch (p_target->Class) {
            case CLASS_PERSON:

                calc_sub_objects_position(
                    p_target,
                    0,
                    SUB_OBJECT_HEAD,
                    &want_x,
                    &want_y,
                    &want_z);

                want_x += p_target->WorldPos.X >> 8;
                want_y += p_target->WorldPos.Y >> 8;
                want_z += p_target->WorldPos.Z >> 8;

                break;

            case CLASS_BAT:
                want_x = p_target->WorldPos.X >> 8;
                want_y = p_target->WorldPos.Y >> 8;
                want_z = p_target->WorldPos.Z >> 8;
                break;

            default:
                ASSERT(0);
                break;
            }

            break;
        }

        dx = want_x - (p_thing->WorldPos.X >> 8);
        dz = want_z - (p_thing->WorldPos.Z >> 8);

        wangle = calc_angle(dx, dz);
        dangle = wangle - p_thing->Draw.Tweened->Angle;

        if (dangle > +1024) {
            dangle -= 2048;
        }
        if (dangle < -1024) {
            dangle += 2048;
        }

        if (p_bat->substate == BAT_SUBSTATE_CIRCLE_TARGET) {
            if (abs(dangle) < 300) {
                p_bat->timer += ticks;

                dy = want_y - (p_thing->WorldPos.Y >> 8);

                if (QDIST3(abs(dx), abs(dy), abs(dz)) < 0x40) {
                    if (p_target->Class == CLASS_PERSON && p_target->Genus.Person->PlayerID && EWAY_stop_player_moving()) {
                        // cut scene so don't hurt player
                    } else if (!(p_bat->flag & BAT_FLAG_ATTACKED)) {
                        PainSound(p_target); // scream yer lungs out!

                        p_target->Genus.Person->Health -= 60;

                        if (p_target->Genus.Person->Health <= 0) {
                            set_person_dead(
                                p_target,
                                NULL,
                                PERSON_DEATH_TYPE_OTHER,
                                UC_FALSE,
                                UC_FALSE);
                        }

                        p_bat->flag |= BAT_FLAG_ATTACKED; // Don't do this until the next swoop.
                    }
                } else {
                    p_bat->flag &= ~BAT_FLAG_ATTACKED; // Allowed to attack again.
                }
            } else {
                want_y = p_bat->want_y;
            }
        }

        dangle >>= 2;
        dist = QDIST2(abs(dx), abs(dz)) >> 4;

        if (dist < 10) {
            dist = 10;
        }

        SATURATE(dangle, -dist, +dist);

        p_thing->Draw.Tweened->Angle += dangle;
        p_thing->Draw.Tweened->Angle &= 2047;

        want_dx = SIN(p_thing->Draw.Tweened->Angle) >> 2;
        want_dz = COS(p_thing->Draw.Tweened->Angle) >> 2;

        // want_dx += want_dx >> 1;
        // want_dz += want_dz >> 1;

        ddx = want_dx - p_bat->dx >> 4;
        ddz = want_dz - p_bat->dz >> 4;

        SATURATE(ddx, -0x800, +0x800);
        SATURATE(ddz, -0x800, +0x800);

        p_bat->dx += ddx;
        p_bat->dz += ddz;

        p_bat->dx -= p_bat->dx / 32;
        p_bat->dy -= p_bat->dy / 4;
        p_bat->dz -= p_bat->dz / 32;

        dy = want_y - (p_thing->WorldPos.Y >> 8);

        p_bat->dy += dy << 2;

        end = BAT_animate(p_thing);

        break;

    case BAT_STATE_ATTACK:

        if (p_bat->target && !(p_bat->flag & BAT_FLAG_ATTACKED) && p_thing->Draw.Tweened->FrameIndex > 6) {
            BAT_emit_fireball(p_thing);

            p_bat->flag |= BAT_FLAG_ATTACKED;
        }

        if (p_bat->target) {
            BAT_turn_to_target(p_thing);
        }

        end = BAT_animate(p_thing);
        p_bat->timer = 0;

        break;
    case BAT_STATE_DYING:

        switch (p_bat->substate) {
        case BAT_SUBSTATE_DEAD_INITIAL:

            //					ASSERT(p_bat->type == BAT_TYPE_GARGOYLE); not anymore

            if (BAT_animate(p_thing)) {
                p_bat->substate = BAT_SUBSTATE_DEAD_LOOP;

                BAT_set_anim(p_thing, BAT_ANIM_GARGOYLE_FALL_LOOP);
            }

            break;

        case BAT_SUBSTATE_DEAD_LOOP:

            BAT_animate(p_thing);

            p_bat->dy -= 0x180;

            p_bat->dx -= p_bat->dx / 32;
            p_bat->dz -= p_bat->dz / 32;

            ground = PAP_calc_map_height_at(
                         p_thing->WorldPos.X >> 8,
                         p_thing->WorldPos.Z >> 8)
                << 8;

            if (p_thing->WorldPos.Y <= ground + 0x4000) {
                if (p_bat->type == BAT_TYPE_BAT && abs(p_bat->dy) > 0x1000) {
                    // Bounce!
                    p_bat->dy = abs(p_bat->dy) >> 2;
                } else {
                    // Dead!
                    p_bat->dx = 0;
                    p_bat->dy = 0;
                    p_bat->dz = 0;

                    if (p_bat->type == BAT_TYPE_BAT) {
                        p_bat->state = BAT_STATE_DEAD;
                    } else {
                        p_bat->substate = BAT_SUBSTATE_DEAD_FINAL;

                        BAT_set_anim(p_thing, BAT_ANIM_GARGOYLE_HIT_GROUND);
                    }
                }
            }

            break;
        case BAT_SUBSTATE_DEAD_FINAL:

            //					ASSERT(p_bat->type == BAT_TYPE_GARGOYLE); balrogs too

            if (BAT_animate(p_thing)) {
                p_bat->state = BAT_STATE_DEAD;
                p_thing->State = STATE_DEAD;
            }

            break;

        default:
            ASSERT(0);
            break;
        }

        end = UC_FALSE;

        break;
    case BAT_STATE_DEAD:

        return;
        // #ifndef PSX
    case BAT_STATE_GROUND:

        switch (p_bat->substate) {
        case BAT_SUBSTATE_GROUND_WAIT:

        {
            Thing* darci = NET_PERSON(0);

            SLONG dx = abs(darci->WorldPos.X - p_thing->WorldPos.X);
            SLONG dz = abs(darci->WorldPos.Z - p_thing->WorldPos.Z);

            SLONG dist = QDIST2(dx, dz);

            if (dist < 0xa0000) {
                BAT_set_anim(p_thing, BAT_ANIM_GARGOYLE_WAKE_UP);

                p_bat->substate = BAT_SUBSTATE_GROUND_WAKE_UP;
            }
        }

        break;

        case BAT_SUBSTATE_GROUND_WAKE_UP:

            if (BAT_animate(p_thing)) {
                BAT_set_anim(p_thing, BAT_ANIM_GARGOYLE_FLY_UP);

                p_bat->substate = BAT_SUBSTATE_GROUND_FLY_UP;
            }

            break;

        case BAT_SUBSTATE_GROUND_FLY_UP:

            if (BAT_animate(p_thing)) {
                BAT_change_state(p_thing);
            }

            break;

        default:
            ASSERT(0);
            break;
        }

        end = UC_FALSE;
        p_bat->timer = 0;

        break;

    case BAT_STATE_RECOIL:
        end = BAT_animate(p_thing);
        p_bat->timer = 0;
        break;
    case BAT_STATE_BALROG_WANDER:

    {
        SLONG dangle;

        dangle = BAT_turn_to_place(
            p_thing,
            p_bat->want_x,
            p_bat->want_z);

        if (abs(dangle) > 256 || p_bat->substate == BAT_SUBSTATE_YOMP_END) {
            p_bat->dx -= p_bat->dx >> 2;
            p_bat->dz -= p_bat->dz >> 2;
        } else {
            want_dx = -SIN(p_thing->Draw.Tweened->Angle) >> 4;
            want_dz = -COS(p_thing->Draw.Tweened->Angle) >> 4;

            want_dx += want_dx >> 1;
            want_dz += want_dz >> 1;

            want_dx -= want_dx >> 5;
            want_dz -= want_dz >> 5;

            p_bat->dx += (want_dx - p_bat->dx) >> 3;
            p_bat->dz += (want_dz - p_bat->dz) >> 3;
        }

        end = BAT_animate(p_thing);

        if (end) {
            if (p_bat->substate == BAT_SUBSTATE_YOMP_START) {
                end = UC_FALSE;

                BAT_set_anim(p_thing, BAT_ANIM_BALROG_YOMP);

                p_bat->substate = BAT_SUBSTATE_YOMP_MIDDLE;
            } else if (p_bat->substate == BAT_SUBSTATE_YOMP_MIDDLE) {
                end = UC_FALSE;

                if (p_bat->timer == 0) {
                    BAT_set_anim(p_thing, BAT_ANIM_BALROG_YOMP_END);

                    p_bat->substate = BAT_SUBSTATE_YOMP_END;
                }
            }
        }
    }

    break;

    case BAT_STATE_BALROG_ROAR:
        end = BAT_animate(p_thing);
        p_bat->timer = 0;
        break;

    case BAT_STATE_BALROG_FOLLOW:
    case BAT_STATE_BALROG_CHARGE:

    {
        if (!p_bat->target) {
            end = UC_TRUE;
            p_bat->timer = 0;
        } else {
            ASSERT(p_bat->target); // triggered

            Thing* p_target = TO_THING(p_bat->target);

            SLONG dangle;

            if (!there_is_a_los_mav(
                    p_thing->WorldPos.X >> 8,
                    p_thing->WorldPos.Y + 0x4000 >> 8,
                    p_thing->WorldPos.Z >> 8,
                    p_target->WorldPos.X >> 8,
                    p_target->WorldPos.Y + 0x4000 >> 8,
                    p_target->WorldPos.Z >> 8)) {
                p_bat->want_x = p_target->WorldPos.X >> 8;
                p_bat->want_z = p_target->WorldPos.Z >> 8;
            } else {
                MAV_Action ma = MAV_do(
                    p_thing->WorldPos.X >> 16,
                    p_thing->WorldPos.Z >> 16,
                    p_target->WorldPos.X >> 16,
                    p_target->WorldPos.Z >> 16,
                    MAV_CAPS_GOTO);

                p_bat->want_x = (ma.dest_x << 8) + 0x80;
                p_bat->want_z = (ma.dest_z << 8) + 0x80;
            }

            dangle = BAT_turn_to_place(
                p_thing,
                p_bat->want_x,
                p_bat->want_z);

            if (abs(dangle) > 256 || p_bat->substate == BAT_SUBSTATE_YOMP_END) {
                p_bat->dx -= p_bat->dx >> 3;
                p_bat->dz -= p_bat->dz >> 3;
            } else {
                want_dx = -SIN(p_thing->Draw.Tweened->Angle) >> 4;
                want_dz = -COS(p_thing->Draw.Tweened->Angle) >> 4;

                want_dx += want_dx >> 1;
                want_dz += want_dz >> 1;

                want_dx -= want_dx >> 5;
                want_dz -= want_dz >> 5;

                p_bat->dx += (want_dx - p_bat->dx) >> 3;
                p_bat->dz += (want_dz - p_bat->dz) >> 3;
            }

            end = BAT_animate(p_thing);

            if (end) {
                if (p_bat->substate == BAT_SUBSTATE_YOMP_START) {
                    end = UC_FALSE;

                    BAT_set_anim(p_thing, BAT_ANIM_BALROG_YOMP);

                    p_bat->substate = BAT_SUBSTATE_YOMP_MIDDLE;
                } else if (p_bat->substate == BAT_SUBSTATE_YOMP_MIDDLE) {
                    end = UC_FALSE;

                    if (p_bat->timer == 0) {
                        BAT_set_anim(p_thing, BAT_ANIM_BALROG_YOMP_END);

                        p_bat->substate = BAT_SUBSTATE_YOMP_END;
                    }
                }
            }
        }
    }

    break;

    case BAT_STATE_BALROG_SWIPE:

        if (p_bat->target && !(p_bat->flag & BAT_FLAG_ATTACKED) && p_thing->Draw.Tweened->FrameIndex > 4) {
            create_shockwave(
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y >> 8,
                p_thing->WorldPos.Z >> 8,
                0x200,
                100,
                p_thing);

            p_bat->flag |= BAT_FLAG_ATTACKED;
        }

        end = BAT_animate(p_thing);
        p_bat->timer = 0;

        break;

    case BAT_STATE_BALROG_STOMP:

        if (p_bat->target && !(p_bat->flag & BAT_FLAG_ATTACKED) && p_thing->Draw.Tweened->FrameIndex > 12) {
            create_shockwave(
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y >> 8,
                p_thing->WorldPos.Z >> 8,
                0x300,
                150,
                p_thing);

            p_bat->flag |= BAT_FLAG_ATTACKED;

            PYRO_create(p_thing->WorldPos, PYRO_DUSTWAVE);
        }

        end = BAT_animate(p_thing);
        p_bat->timer = 0;

        break;

    case BAT_STATE_BALROG_FIREBALL:

        switch (p_bat->substate) {
        case BAT_SUBSTATE_FIREBALL_TURN:

            end = BAT_animate(p_thing);

            if (abs(BAT_turn_to_target(p_thing)) < 128) {
                if (end) {
                    p_bat->substate = BAT_SUBSTATE_FIREBALL_FIRE;

                    BAT_set_anim(p_thing, BAT_ANIM_BALROG_SWIPE);
                }
            }

            break;

        case BAT_SUBSTATE_FIREBALL_FIRE:

            if (p_bat->target && !(p_bat->flag & BAT_FLAG_ATTACKED) && p_thing->Draw.Tweened->FrameIndex > 4) {
                BAT_emit_fireball(p_thing);

                p_bat->flag |= BAT_FLAG_ATTACKED;
            }

            end = BAT_animate(p_thing);
            p_bat->timer = 0;

            break;

        default:
            ASSERT(0);
            break;
        }

        break;

    case BAT_STATE_BALROG_IDLE:
        end = BAT_animate(p_thing);
        break;

    case BAT_STATE_BANE_IDLE:
        end = BAT_animate(p_thing);

        BAT_process_bane_sparks(p_thing);

        p_bat->glow += 64 * TICK_RATIO >> TICK_SHIFT;

        if (p_bat->glow > 0xff00) {
            p_bat->glow = 0xff00;
        }

        break;

    case BAT_STATE_BANE_ATTACK:

        if (!(p_bat->flag & BAT_FLAG_ATTACKED) && p_thing->Draw.Tweened->FrameIndex > 12) {
            create_shockwave(
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y >> 8,
                p_thing->WorldPos.Z >> 8,
                0x300,
                150,
                p_thing);

            p_bat->flag |= BAT_FLAG_ATTACKED;

            PYRO_create(p_thing->WorldPos, PYRO_DUSTWAVE);

            {
                Thing* darci = NET_PERSON(0);

                extern SLONG is_person_dead(Thing * p_person);

                if (!is_person_dead(darci)) {
                    if (THING_dist_between(p_thing, NET_PERSON(0)) < 0x600) {
                        if (there_is_a_los(
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y + 0xc000 >> 8,
                                p_thing->WorldPos.Z >> 8,
                                darci->WorldPos.X >> 8,
                                darci->WorldPos.Y + 0x6000 >> 8,
                                darci->WorldPos.Z >> 8,
                                0)) {
                            SPARK_Pinfo p1;
                            SPARK_Pinfo p2;

                            p1.type = SPARK_TYPE_LIMB;
                            p1.flag = 0;
                            p1.person = THING_NUMBER(p_thing);
                            p1.limb = 0;

                            p2.type = SPARK_TYPE_LIMB;
                            p2.flag = 0;
                            p2.person = THING_NUMBER(darci);
                            p2.limb = SUB_OBJECT_HEAD;

                            SPARK_create(
                                &p1,
                                &p2,
                                50);

                            if (darci->State != STATE_DANGLING && darci->State != STATE_JUMPING) {
                                set_face_thing(
                                    darci,
                                    p_thing);
                            }

                            darci->Genus.Person->Health -= 50;

                            if (darci->Genus.Person->Health <= 0) {
                                set_person_dead(
                                    darci,
                                    NULL,
                                    PERSON_DEATH_TYPE_OTHER,
                                    UC_FALSE,
                                    0);
                            } else {
                                set_person_recoil(
                                    darci,
                                    ANIM_HIT_FRONT_MID,
                                    0);
                            }
                        }
                    }
                }
            }
        }

        end = BAT_animate(p_thing);
        p_bat->timer = 0;

        break;

    case BAT_STATE_BANE_START:

        end = BAT_animate(p_thing);
        p_bat->timer = 0;

        break;

    default:
        ASSERT(0);
        break;
    }

    newpos.X = p_thing->WorldPos.X + (p_bat->dx * TICK_RATIO >> TICK_SHIFT);
    newpos.Y = p_thing->WorldPos.Y + (p_bat->dy * TICK_RATIO >> TICK_SHIFT);
    newpos.Z = p_thing->WorldPos.Z + (p_bat->dz * TICK_RATIO >> TICK_SHIFT);

    if (p_bat->type == BAT_TYPE_BALROG) {
        BAT_balrog_slide_along(
            p_thing->WorldPos.X,
            p_thing->WorldPos.Z,
            &newpos.X,
            &newpos.Z);

        newpos.Y = PAP_calc_height_at(
                       p_thing->WorldPos.X >> 8,
                       p_thing->WorldPos.Z >> 8)
                + 0x40
            << 8;
        NIGHT_dlight_move(p_bat->glow, newpos.X >> 8, (newpos.Y >> 8) + 64, newpos.Z >> 8);

        /*
        #define BAT_ANIM_BALROG_YOMP			2
        #define BAT_ANIM_BALROG_YOMP_START		5
        #define BAT_ANIM_BALROG_YOMP_END		6
        */
        switch (p_thing->Draw.Tweened->CurrentAnim) {
        case BAT_ANIM_BALROG_YOMP:
            if (
                (((p_thing->Draw.Tweened->FrameIndex >= 1) && (p_thing->Draw.Tweened->FrameIndex < 6))
                    || ((p_thing->Draw.Tweened->FrameIndex >= 11) && (p_thing->Draw.Tweened->FrameIndex < 16)))
                && !(p_bat->flag & BAT_FLAG_SYNC_FX)) {
                MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_BALROG_STEP_START, S_BALROG_STEP_END), 0, p_thing);
                p_bat->flag &= ~BAT_FLAG_SYNC_FX2;
                p_bat->flag |= BAT_FLAG_SYNC_FX;
            }
            if ((p_thing->Draw.Tweened->FrameIndex >= 6) && (p_thing->Draw.Tweened->FrameIndex <= 11) && !(p_bat->flag & BAT_FLAG_SYNC_FX2)) {
                MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_BALROG_STEP_START, S_BALROG_STEP_END), 0, p_thing);
                p_bat->flag &= ~BAT_FLAG_SYNC_FX;
                p_bat->flag |= BAT_FLAG_SYNC_FX2;
            }
            break;
        case BAT_ANIM_BALROG_YOMP_END:
            if (
                (((p_thing->Draw.Tweened->FrameIndex >= 1) && (p_thing->Draw.Tweened->FrameIndex < 5))
                    || ((p_thing->Draw.Tweened->FrameIndex >= 11) && (p_thing->Draw.Tweened->FrameIndex < 16)))
                && !(p_bat->flag & BAT_FLAG_SYNC_FX)) {
                MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_BALROG_STEP_START, S_BALROG_STEP_END), 0, p_thing);
                p_bat->flag &= ~BAT_FLAG_SYNC_FX2;
                p_bat->flag |= BAT_FLAG_SYNC_FX;
            }
            if ((p_thing->Draw.Tweened->FrameIndex >= 5) && (p_thing->Draw.Tweened->FrameIndex <= 11) && !(p_bat->flag & BAT_FLAG_SYNC_FX2)) {
                MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_BALROG_STEP_START, S_BALROG_STEP_END), 0, p_thing);
                p_bat->flag &= ~BAT_FLAG_SYNC_FX;
                p_bat->flag |= BAT_FLAG_SYNC_FX2;
            }
            break;
        }
    }

    move_thing_on_map(p_thing, &newpos);

    dx = p_thing->WorldPos.X - ((p_bat->home_x << 16) + 0x8000);
    dz = p_thing->WorldPos.Z - ((p_bat->home_z << 16) + 0x8000);

    if (abs(dx) > 0x50000 || abs(dz) > 0x50000) {
        p_bat->timer >>= 1;
    }

    if (p_bat->timer <= ticks) {
        p_bat->timer = 0;

        if (end) {
            BAT_change_state(p_thing);
        }
    } else {
        p_bat->timer -= ticks;
    }
}

// uc_orig: BAT_create (fallen/Headers/bat.h)
// Allocates a Thing + Bat slot, initializes all fields, and sets up the draw tween.
// Returns the thing index on success, or NULL on any allocation failure.
THING_INDEX BAT_create(
    SLONG type,
    SLONG x,
    SLONG z,
    UWORD yaw)
{
    SLONG i;

    Thing* p_thing;
    Bat* p_bat;
    DrawTween* dt;

    p_thing = alloc_thing(CLASS_BAT);

    if (p_thing == NULL) {
        return NULL;
    }

    for (i = 0; i < BAT_MAX_BATS; i++) {
        p_bat = TO_BAT(i);

        if (p_bat->type == BAT_TYPE_UNUSED) {
            goto found_unused_bat;
        }
    }

    return NULL;

found_unused_bat:;

    dt = alloc_draw_tween(DT_ROT_MULTI);

    if (dt == NULL) {
        return NULL;
    }

    if (anim_chunk[type].MultiObject[0] == 0) {
        load_anim_prim_object(type);
    }

    p_thing->WorldPos.X = x << 8;
    p_thing->WorldPos.Z = z << 8;
    p_thing->WorldPos.Y = PAP_calc_map_height_at(x, z) << 8;

    p_thing->DrawType = DT_ANIM_PRIM;
    p_thing->Draw.Tweened = dt;

    p_thing->Genus.Bat = p_bat;
    p_thing->State = STATE_NORMAL;
    p_thing->SubState = NULL;
    p_thing->StateFn = BAT_normal;
    p_thing->Index = type;

    add_thing_to_map(p_thing);

    dt->Angle = yaw;
    dt->Roll = 0;
    dt->Tilt = 0;
    dt->AnimTween = 0;
    dt->TweenStage = 0;
    dt->NextFrame = NULL;
    dt->QueuedFrame = NULL;
    dt->TheChunk = &anim_chunk[type];
    dt->CurrentFrame = anim_chunk[type].AnimList[1];
    dt->NextFrame = dt->CurrentFrame->NextFrame;
    dt->CurrentAnim = 1;
    dt->FrameIndex = 0;
    dt->Flags = 0;

    p_bat->type = type;
    p_bat->dx = 0;
    p_bat->dy = 0;
    p_bat->dz = 0;
    p_bat->health = 100;
    p_bat->home_x = x >> 8;
    p_bat->home_z = z >> 8;
    p_bat->target = NULL;
    p_bat->timer = 0;
    p_bat->flag = 0;

    switch (type) {
    case BAT_TYPE_BAT:
        p_bat->state = BAT_STATE_IDLE;
        p_bat->substate = BAT_SUBSTATE_NONE;
        p_thing->WorldPos.Y += 0x100 << 8;
        break;

    case BAT_TYPE_GARGOYLE:
        p_bat->state = BAT_STATE_GROUND;
        p_bat->substate = BAT_SUBSTATE_GROUND_WAIT;
        break;
    case BAT_TYPE_BALROG:
        p_bat->state = BAT_STATE_BALROG_ROAR;
        p_bat->substate = BAT_SUBSTATE_NONE;
        p_bat->health = 255;
        BAT_set_anim(p_thing, BAT_ANIM_BALROG_ROAR);
        // let's set this mutha alight
        {
            Thing* pyro;
            pyro = PYRO_create(p_thing->WorldPos, PYRO_IMMOLATE);
            if (pyro) {
                pyro->Genus.Pyro->victim = p_thing;
                pyro->Genus.Pyro->Flags = PYRO_FLAGS_FLICKER;
            }
            // darci->Genus.Person->BurnIndex=PYRO_NUMBER(pyro->Genus.Pyro)+1;
        }
        // and cast some light nearby...
        p_bat->glow = NIGHT_dlight_create(p_thing->WorldPos.X >> 8, p_thing->WorldPos.Y >> 8, p_thing->WorldPos.Z >> 8, 200, 32, 28, 10);

        break;

    case BAT_TYPE_BANE:
        p_bat->state = BAT_STATE_BANE_IDLE;
        p_bat->substate = BAT_SUBSTATE_NONE;
        p_bat->glow = 0x7f00;
        p_thing->WorldPos.Y += 0x60 << 8;
        BAT_set_anim(p_thing, BAT_ANIM_BANE_IDLE);
        break;

    default:
        ASSERT(0);
        break;
    }

    return THING_NUMBER(p_thing);
}

// uc_orig: BAT_apply_hit (fallen/Headers/bat.h)
// Apply damage to a bat-class entity. Bane is completely immune.
// Bats take half damage; Balrog takes 1/16 (very tanky). Blood splash on the aggressor's side.
void BAT_apply_hit(
    Thing* p_me,
    Thing* p_aggressor,
    SLONG damage)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;

    ASSERT(p_me->Class == CLASS_BAT);

    if (p_me->Genus.Bat->type == BAT_TYPE_BANE) {
        return;
    }

    ASSERT(damage >= 0);

    if (p_aggressor) {
        SLONG blood_x;
        SLONG blood_y;
        SLONG blood_z;

        SLONG towards;

        blood_x = p_me->WorldPos.X;
        blood_y = p_me->WorldPos.Y;
        blood_z = p_me->WorldPos.Z;

        switch (p_me->Genus.Bat->type) {
        case BAT_TYPE_BAT:
            blood_y += 0x2000;
            towards = 0x40;
            break;
        case BAT_TYPE_GARGOYLE:
            blood_y += 0x5000;
            towards = 0x60;
            break;
        case BAT_TYPE_BALROG:
            blood_y += 0x8000;
            towards = 0x90;
            break;

        default:
            ASSERT(0);
            break;
        }

        dx = p_aggressor->WorldPos.X - p_me->WorldPos.X;
        dz = p_aggressor->WorldPos.Z - p_me->WorldPos.Z;

        dx >>= 8;
        dz >>= 8;

        SLONG length;

        length = QDIST2(abs(dx), abs(dz)) + 1;

        dx = dx * towards / length;
        dz = dz * towards / length;

        blood_x += dx << 8;
        blood_z += dz << 8;

        // #ifndef VERSION_GERMAN
        if (VIOLENCE)
            PARTICLE_Add(
                blood_x,
                blood_y,
                blood_z,
                0, 0, 0,
                POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), 0x7FFF0000,
                PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE, 10, 75, 1, 20, 5);

        // #endif

        if (p_me->Genus.Bat->type != BAT_TYPE_BALROG) {
            dx = p_aggressor->WorldPos.X - p_me->WorldPos.X;
            dy = p_aggressor->WorldPos.Y - p_me->WorldPos.Y;
            dz = p_aggressor->WorldPos.Z - p_me->WorldPos.Z;

            dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

            dx = (dx << 8) / dist;
            dy = (dy << 8) / dist;
            dz = (dz << 8) / dist;

            p_me->Genus.Bat->dx -= dx << 4;
            p_me->Genus.Bat->dy -= dy << 4;
            p_me->Genus.Bat->dz -= dz << 4;
        }
    }

    // Bats are nails!
    damage >>= 1;

    if (p_me->Genus.Bat->type == BAT_TYPE_BALROG) {
        damage >>= 3;
    }

    damage += 1;

    if (p_me->Genus.Bat->health <= damage) {
        {
            p_me->Genus.Bat->health = 0;
            p_me->Genus.Bat->state = BAT_STATE_DYING;

            if (p_me->Genus.Bat->type == BAT_TYPE_GARGOYLE) {
                BAT_set_anim(p_me, BAT_ANIM_GARGOYLE_START_FALL);

                p_me->Genus.Bat->substate = BAT_SUBSTATE_DEAD_INITIAL;
                p_me->State = STATE_DEAD;
            } else if (p_me->Genus.Bat->type == BAT_TYPE_BALROG) {
                p_me->Genus.Bat->substate = BAT_SUBSTATE_DEAD_FINAL;

                p_me->Genus.Bat->dx = 0;
                p_me->Genus.Bat->dy = 0;
                p_me->Genus.Bat->dz = 0;

                BAT_set_anim(p_me, BAT_ANIM_BALROG_DIE);
                MFX_play_thing(THING_NUMBER(p_me), S_BALROG_DEATH, 0, p_me);

            } else {
                p_me->Genus.Bat->substate = BAT_SUBSTATE_DEAD_LOOP;

                BAT_set_anim(p_me, BAT_ANIM_BAT_DIE);
                p_me->State = STATE_DEAD;
            }
        }
    } else {
        p_me->Genus.Bat->health -= damage;

        if (p_me->Genus.Bat->type == BAT_TYPE_BALROG) {
            if (p_aggressor && p_aggressor->Class == CLASS_PERSON) {
                p_me->Genus.Bat->target = THING_NUMBER(p_aggressor);

                if (p_me->Genus.Bat->state == BAT_STATE_BALROG_IDLE || p_me->Genus.Bat->state == BAT_STATE_BALROG_WANDER) {
                    p_me->Genus.Bat->timer = 0;
                }
            }
        } else {
            // React to being shot!
            BAT_set_anim(p_me, BAT_ANIM_GENERIC_TAKE_HIT);

            p_me->Genus.Bat->state = BAT_STATE_RECOIL;
        }
    }
}
