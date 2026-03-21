#include "fallen/Headers/Game.h" // Must be first: pulls in MFStdLib.h (MFFileHandle for supermap.h), string.h
#include "actors/animals/bat_globals.h"
#include "actors/animals/bat.h"
#include "fallen/Headers/animate.h"
#include "fallen/Headers/statedef.h"
#include "fallen/Headers/Sound.h"
#include "fallen/Headers/eway.h"
#include "engine/effects/psystem.h"
#include "engine/graphics/pipeline/poly.h"
#include "ai/mav.h"
#include "world/navigation/wand.h"
#include "effects/spark.h"
#include "fallen/Headers/pcom.h"       // Temporary: PCOM_AI_NONE, PCOM_AI_SUICIDE — pcom not yet migrated
#include "missions/memory.h"
#include "engine/lighting/night.h"

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
// Returns TRUE if the animation has reached its last frame.
SLONG BAT_animate(Thing* p_thing)
{
    SLONG ret = FALSE;
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

        SLONG best_score = INFINITY;
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
