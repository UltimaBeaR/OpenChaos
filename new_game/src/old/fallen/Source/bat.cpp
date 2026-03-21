// Remaining chunk: BAT_normal, BAT_create, BAT_apply_hit.
// First chunk (globals + BAT_find_summon_people..BAT_balrog_slide_along) migrated to new/actors/animals/bat.cpp.

#include "game.h"
#include "bat.h"
#include "animate.h"
#include "statedef.h"
#include "sound.h"
#include "eway.h"
#include "psystem.h"
#include "poly.h"
#include "mav.h"
#include "wand.h"
#include "spark.h"
#include "pcom.h"
#include "memory.h"
#include "night.h"

#define BAT_FLAG_ATTACKED (1 << 0)
#define BAT_FLAG_SYNC_FX  (1 << 1)
#define BAT_FLAG_SYNC_FX2 (1 << 2)

#define BAT_TICKS_PER_SECOND 160

#define BAT_STATE_IDLE           0
#define BAT_STATE_GOTO           1
#define BAT_STATE_CIRCLE         2
#define BAT_STATE_ATTACK         3
#define BAT_STATE_DYING          4
#define BAT_STATE_DEAD           5
#define BAT_STATE_GROUND         6
#define BAT_STATE_RECOIL         7
#define BAT_STATE_BALROG_WANDER  8
#define BAT_STATE_BALROG_ROAR    9
#define BAT_STATE_BALROG_FOLLOW  10
#define BAT_STATE_BALROG_CHARGE  11
#define BAT_STATE_BALROG_SWIPE   12
#define BAT_STATE_BALROG_STOMP   13
#define BAT_STATE_BALROG_FIREBALL 14
#define BAT_STATE_BALROG_IDLE    15
#define BAT_STATE_BANE_IDLE      16
#define BAT_STATE_BANE_ATTACK    17
#define BAT_STATE_BANE_START     18
#define BAT_STATE_NUMBER         19

#define BAT_SUBSTATE_NONE            0
#define BAT_SUBSTATE_CIRCLE_HOME     1
#define BAT_SUBSTATE_CIRCLE_TARGET   2
#define BAT_SUBSTATE_CIRCLE_WANT     3
#define BAT_SUBSTATE_GROUND_WAIT     4
#define BAT_SUBSTATE_GROUND_WAKE_UP  5
#define BAT_SUBSTATE_GROUND_FLY_UP   6
#define BAT_SUBSTATE_DEAD_INITIAL    7
#define BAT_SUBSTATE_DEAD_LOOP       8
#define BAT_SUBSTATE_DEAD_FINAL      9
#define BAT_SUBSTATE_YOMP_START      10
#define BAT_SUBSTATE_YOMP_MIDDLE     11
#define BAT_SUBSTATE_YOMP_END        12
#define BAT_SUBSTATE_FIREBALL_TURN   13
#define BAT_SUBSTATE_FIREBALL_FIRE   14

#define BAT_ANIM_BAT_FLY                1
#define BAT_ANIM_BAT_DIE                2
#define BAT_ANIM_GARGOYLE_WAIT          1
#define BAT_ANIM_GARGOYLE_WAKE_UP       15
#define BAT_ANIM_GARGOYLE_FLY_UP        3
#define BAT_ANIM_GARGOYLE_FLY           2
#define BAT_ANIM_GARGOYLE_FLY_FORWARDS  12
#define BAT_ANIM_GARGOYLE_ATTACK        4
#define BAT_ANIM_GARGOYLE_TAKE_HIT      14
#define BAT_ANIM_GARGOYLE_START_FALL    16
#define BAT_ANIM_GARGOYLE_FALL_LOOP     17
#define BAT_ANIM_GARGOYLE_HIT_GROUND    18
#define BAT_ANIM_BALROG_YOMP            2
#define BAT_ANIM_BALROG_IDLE            3
#define BAT_ANIM_BALROG_SWIPE           4
#define BAT_ANIM_BALROG_YOMP_START      5
#define BAT_ANIM_BALROG_YOMP_END        6
#define BAT_ANIM_BALROG_TURN            8
#define BAT_ANIM_BALROG_ROAR            9
#define BAT_ANIM_BALROG_STOMP           10
#define BAT_ANIM_BALROG_TAKE_HIT        11
#define BAT_ANIM_BALROG_DIE             12
#define BAT_ANIM_BANE_IDLE              2
#define BAT_ANIM_BANE_ATTACK            3
#define BAT_ANIM_GENERIC_FLY      (-1)
#define BAT_ANIM_GENERIC_TAKE_HIT (-2)

#define BAT_BALROG_WIDTH (0x3000)

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

    SLONG goto_x;
    SLONG goto_y;
    SLONG goto_z;

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
                                FALSE,
                                FALSE);
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

        end = FALSE;

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

        end = FALSE;
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
                end = FALSE;

                BAT_set_anim(p_thing, BAT_ANIM_BALROG_YOMP);

                p_bat->substate = BAT_SUBSTATE_YOMP_MIDDLE;
            } else if (p_bat->substate == BAT_SUBSTATE_YOMP_MIDDLE) {
                end = FALSE;

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
            end = TRUE;
            p_bat->timer = 0;
        } else {
            ASSERT(p_bat->target); // triggered

            Thing* p_target = TO_THING(p_bat->target);

            SLONG dangle;

            extern SLONG there_is_a_los_mav(
                SLONG x1, SLONG y1, SLONG z1,
                SLONG x2, SLONG y2, SLONG z2);

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
                    end = FALSE;

                    BAT_set_anim(p_thing, BAT_ANIM_BALROG_YOMP);

                    p_bat->substate = BAT_SUBSTATE_YOMP_MIDDLE;
                } else if (p_bat->substate == BAT_SUBSTATE_YOMP_MIDDLE) {
                    end = FALSE;

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
                                    FALSE,
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
        extern SLONG load_anim_prim_object(SLONG prim);
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
