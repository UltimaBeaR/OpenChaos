#!/usr/bin/env python3
"""Appends iteration 87 chunk to new/ai/pcom.cpp"""

content = r"""
// ---------------------------------------------------------------------------
// Iteration 87: PCOM_process_findcar .. PCOM_find_bodyguard_victim
// ---------------------------------------------------------------------------

// uc_orig: PCOM_process_findcar (fallen/Source/pcom.cpp)
void PCOM_process_findcar(Thing* p_person)
{
    SLONG door;

    Thing* p_vehicle = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    ASSERT(p_vehicle->Class == CLASS_VEHICLE);

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_GOTOCAR:

        if (p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER || p_person->Genus.Person->pcom_ai == PCOM_AI_DRIVER) {
            // Drivers never enter a car that is already occupied by another driver.
            if (p_vehicle->Genus.Vehicle->Driver) {
                PCOM_set_person_ai_findcar(p_person, NULL);
            }
        }

        {
            // Enter only if out of player view and close enough for 20 consecutive turns.
            if ((p_person->Flags & FLAGS_IN_VIEW) || PCOM_get_dist_between(p_person, p_vehicle) > 0x200) {
                p_person->Genus.Person->pcom_ai_counter = 0;
            } else {
                p_person->Genus.Person->pcom_ai_counter += 1;
            }

            door = 0;

            if (p_person->Genus.Person->pcom_ai_counter > 20 || in_right_place_for_car(p_person, p_vehicle, &door)) {
                ASSERT(door == 0 || door == 1);

                PCOM_set_person_move_getincar(p_person, p_vehicle, FALSE, door);

                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_GETINCAR;
                p_person->Genus.Person->pcom_ai_counter = 0;
            }
        }

        break;

    case PCOM_AI_SUBSTATE_GETINCAR:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            // Successfully inside. Wait until no more passengers are boarding.
            if (((GAME_TURN & 0xf) == (THING_NUMBER(p_person) & 0xf)) || PCOM_are_there_people_who_want_to_enter(p_vehicle)) {
                // Wait a while.
            } else {
                p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NORMAL;
                p_person->Genus.Person->pcom_ai_substate = 0;
                p_person->Genus.Person->pcom_ai_counter = 0;

                p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
                p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_NONE;
            }
        } else {
            if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(40)) {
                // Stuck getting in -- retry findcar.
                PCOM_set_person_ai_findcar(p_person, p_person->Genus.Person->pcom_ai_arg);
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_talk (fallen/Source/pcom.cpp)
void PCOM_process_talk(Thing* p_person)
{
    if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
        // Animation finished. If NO_RETURN_TO_NORMAL was set by mission script, stay put.
        if (p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER) {
            // Flagged as NO_RETURN_TO_NORMAL -- keep this state.
        } else {
            PCOM_set_person_ai_normal(p_person);
        }
    }
}

// uc_orig: PCOM_process_hands_up (fallen/Source/pcom.cpp)
void PCOM_process_hands_up(Thing* p_person)
{
    Thing* p_cop;
    p_cop = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

    if (p_person->Genus.Person->pcom_ai_counter > PCOM_get_duration(20)) {
        // After 20s check if cop is still aiming at us.
        if (p_cop->Genus.Person->Target != THING_NUMBER(p_person) || (p_cop->State != STATE_GUN && p_cop->SubState != SUB_STATE_RUNNING)) {
            // Cop moved on -- return to normal.
            PCOM_set_person_ai_normal(p_person);
        }
    }
}

// uc_orig: PCOM_process_hitch (fallen/Source/pcom.cpp)
void PCOM_process_hitch(Thing* p_person)
{
    SLONG door;

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_GOTOCAR:

    {
        Thing* p_vehicle = TO_THING(p_person->Genus.Person->pcom_ai_arg);

        ASSERT(p_vehicle->Class == CLASS_VEHICLE);

        if ((p_person->Flags & FLAGS_IN_VIEW) || PCOM_get_dist_between(p_person, p_vehicle) > 0x200) {
            p_person->Genus.Person->pcom_ai_counter = 0;
        } else {
            p_person->Genus.Person->pcom_ai_counter += 1;
        }

        if (p_person->Genus.Person->pcom_ai_counter > 20 || in_right_place_for_car(p_person, p_vehicle, &door)) {
            PCOM_set_person_move_getincar(p_person, p_vehicle, TRUE, door);

            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_GETINCAR;
            p_person->Genus.Person->pcom_ai_counter = 0;
        }
    }

    break;

    case PCOM_AI_SUBSTATE_GETINCAR:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HITCHING;
        }

        break;

    case PCOM_AI_SUBSTATE_HITCHING:

        if (p_person->Genus.Person->pcom_move == PCOM_MOVE_FOLLOW) {
            SLONG i_target = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);
            Thing* p_target = TO_THING(i_target);

            ASSERT(p_target->Class == CLASS_PERSON);

            if (!(p_target->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER))) {
                // Followed person left the vehicle -- exit too.
                PCOM_set_person_ai_leavecar(p_person, PCOM_EXCAR_NORMAL, 0, 0);

                return;
            }
        } else {
            ASSERT(0);
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_knockedout (fallen/Source/pcom.cpp)
void PCOM_process_knockedout(Thing* p_person)
{
    if (!(p_person->Genus.Person->Flags & FLAG_PERSON_KO)) {
        // Recovered from being knocked out.
        PCOM_set_person_ai_normal(p_person);
    }
}

// uc_orig: PCOM_process_taunt (fallen/Source/pcom.cpp)
void PCOM_process_taunt(Thing* p_person)
{
    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    // Always face whoever is being taunted.
    set_face_thing(p_person, p_target);

    if ((PTIME(p_person) & 0x7) == 0) {
        if (!can_a_see_b(p_person, p_target)) {
            // Lost sight of target -- stop taunting.
            PCOM_set_person_ai_normal(p_person);

            return;
        }
    }

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        if (p_person->Genus.Person->pcom_zone && (PCOM_get_zone_for_position(p_person) & p_person->Genus.Person->pcom_zone)) {
            // Kill the target if they entered our zone.
            if (PCOM_get_zone_for_position(p_target) & p_person->Genus.Person->pcom_zone) {
                PCOM_set_person_ai_kill_person(p_person, p_target);

                return;
            }
        } else {
            // Random 50% chance to escalate to combat.
            if (Random() & 0x1) {
                PCOM_set_person_ai_kill_person(p_person, p_target);

                return;
            }
        }

        // Pause before next taunt animation.
        PCOM_set_person_move_pause(p_person);

        p_person->Genus.Person->pcom_ai_counter = PCOM_get_duration(Random() & 0xf);

        break;

    case PCOM_MOVE_STATE_PAUSE:

        if (p_person->Genus.Person->pcom_move_counter > p_person->Genus.Person->pcom_ai_counter) {
            if (p_person->Genus.Person->pcom_bent & PCOM_BENT_LAZY) {
                // Lazy people just stand, no taunt animation.
                PCOM_set_person_move_still(p_person);
            } else {
                ASSERT(p_person->SubState != SUB_STATE_GRAPPLEE);
                ASSERT(p_person->SubState != SUB_STATE_GRAPPLE_HELD);

                PCOM_set_person_move_animation(p_person, ANIM_WANKER);
            }
        }

        break;

    case PCOM_MOVE_STATE_ANIMATION:

        if (p_person->Genus.Person->pcom_zone) {
            // Zone check during animation -- kill if target entered zone.
            if (PCOM_get_zone_for_position(p_target) & p_person->Genus.Person->pcom_zone) {
                PCOM_set_person_ai_kill_person(p_person, p_target);

                return;
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_arrest (fallen/Source/pcom.cpp)
void PCOM_process_arrest(Thing* p_person)
{
    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    if (p_target->State == STATE_DEAD) {
        PCOM_set_person_ai_normal(p_person);

        return;
    }

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        // Wait briefly, then escalate to combat if suspect does not comply.
        PCOM_set_person_move_pause(p_person);

        break;

    case PCOM_MOVE_STATE_PAUSE:

        if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(10)) {
            PCOM_set_person_ai_kill_person(
                p_person,
                p_target);

            return;
        }

        break;

    case PCOM_MOVE_STATE_GOTO_THING:

        if (PCOM_get_dist_between(p_person, p_target) < 0x200) {
            PCOM_set_person_move_still(p_person);
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_homesick (fallen/Source/pcom.cpp)
void PCOM_process_homesick(Thing* p_person)
{
    if (PCOM_finished_nav(p_person)) {
        // Arrived home -- return to normal behaviour.
        PCOM_set_person_ai_normal(p_person);
    }
}

// uc_orig: PCOM_process_bdeactivate (fallen/Source/pcom.cpp)
void PCOM_process_bdeactivate(Thing* p_person)
{
    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_GOTOBOMB:

        if (PCOM_finished_nav(p_person)) {
            // Reached the bomb -- begin defusing.
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_CUTWIRES;
            p_person->Genus.Person->pcom_ai_counter = 0;

            PCOM_set_person_move_still(p_person);
        }

        break;

    case PCOM_AI_SUBSTATE_CUTWIRES:

        // Spin in place to visually indicate activity.
        p_person->Draw.Tweened->Angle += 32;

        if (p_person->Genus.Person->pcom_ai_counter >= PCOM_get_duration(100)) {
            // Bomb defused.
            Thing* p_bomb = TO_THING(p_person->Genus.Person->pcom_ai_arg);

            p_bomb->SubState = SPECIAL_SUBSTATE_NONE;

            PCOM_set_person_ai_normal(p_person);
        } else {
            p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_leavecar (fallen/Source/pcom.cpp)
void PCOM_process_leavecar(Thing* p_person)
{
    Thing* p_vehicle;

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        // Finished exiting. Decide next AI state based on exit reason.
        switch (p_person->Genus.Person->pcom_ai_excar_state) {
        case PCOM_EXCAR_NORMAL:
            PCOM_set_person_ai_normal(p_person);
            break;

        case PCOM_EXCAR_FLEE_PERSON:
            PCOM_set_person_ai_flee_person(p_person, TO_THING(p_person->Genus.Person->pcom_ai_excar_arg));
            break;

        case PCOM_EXCAR_ARREST_PERSON:
            PCOM_set_person_ai_arrest(p_person, TO_THING(p_person->Genus.Person->pcom_ai_excar_arg));
            break;

        default:
            ASSERT(0);
            break;
        }

        break;

    case PCOM_MOVE_STATE_PARK_CAR:
    case PCOM_MOVE_STATE_PARK_CAR_ON_ROAD:

        ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

        p_vehicle = TO_THING(p_person->Genus.Person->InCar);

        if (p_vehicle->Velocity == 0) {
            // Car stopped -- play exit animation.
            PCOM_set_person_move_leavecar(p_person);
        }

        break;

    case PCOM_MOVE_STATE_ANIMATION:
        break;

    default:
        ASSERT(0);
        break;
    }
}

// Stationary sniper AI. Three substates:
//   LOOK: wait for line of sight to target.
//   AIMING: draw gun, then fire when counter exceeds shoot_time.
//   NOMOREAMMO: holster gun, return to LOOK.
// Max engagement range 0x600 units.
// PCOM_AI_SHOOT_DEAD variant fires immediately without waiting.
// uc_orig: PCOM_process_snipe (fallen/Source/pcom.cpp)
void PCOM_process_snipe(Thing* p_person)
{
    if (p_person->Genus.Person->pcom_ai_arg == NULL) {
        // No target -- reset to normal.
        p_person->Genus.Person->pcom_ai = PCOM_AI_NONE;

        PCOM_set_person_ai_normal(p_person);

        return;
    }

    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_LOOK:

        p_person->Genus.Person->Target = NULL;

        if (!PCOM_person_has_any_sort_of_gun_with_ammo(p_person)) {
            // No ammo -- nothing to do.
        } else {
            if (can_a_see_b(p_person, p_target)) {
                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_AIMING;
                p_person->Genus.Person->pcom_ai_counter = 0;
            } else {
                p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
                p_person->Genus.Person->Target = NULL;
                // Note: "holster after 10s of no target" code is commented out in original.
                // Gun stays drawn indefinitely while scanning.
            }
        }

        break;

    case PCOM_AI_SUBSTATE_AIMING:

        p_person->Genus.Person->Target = THING_NUMBER(p_target);

        if (!person_has_gun_or_grenade_out(p_person)) {
            // Draw gun first.
            if (p_person->Genus.Person->pcom_move_state != PCOM_MOVE_STATE_ANIMATION) {
                PCOM_set_person_move_draw_gun(p_person);
            }
        } else {
            if (PCOM_get_dist_between(p_person, p_target) < 0x600 && can_a_see_b(p_person, p_target)) {
                SLONG shoot_time;

                shoot_time = PCOM_get_duration(get_rate_of_fire(p_person));
                shoot_time -= PCOM_get_duration100(GET_SKILL(p_person) << 2);

                if (p_person->Genus.Person->pcom_ai_counter > shoot_time || p_person->Genus.Person->pcom_ai == PCOM_AI_SHOOT_DEAD) {
                    if (p_person->Genus.Person->Ammo == 0) {
                        // Dry fire -- play click then holster.
                        set_person_shoot(p_person, 1);

                        PCOM_set_person_move_gun_away(p_person);

                        p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NOMOREAMMO;
                    } else {
                        // Fire!
                        p_person->Genus.Person->Target = THING_NUMBER(p_target);
                        PCOM_set_person_move_shoot(p_person);

                        p_person->Genus.Person->pcom_ai_counter = 0;
                    }
                } else {
                    p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
                }

                // Track target while aiming.
                set_face_thing(p_person, p_target);
            } else {
                // Lost sight or out of range -- scan again.
                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LOOK;
            }
        }

        break;

    case PCOM_AI_SUBSTATE_NOMOREAMMO:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            // Holster done -- resume scanning.
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LOOK;
            p_person->Genus.Person->pcom_ai_counter = 0;
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_warm_hands (fallen/Source/pcom.cpp)
void PCOM_process_warm_hands(Thing* p_person)
{
    SLONG i_fire;
    Thing* p_fire;

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_GOTOFIRE:

        switch (p_person->Genus.Person->pcom_move_state) {
        case PCOM_MOVE_STATE_PAUSE:

            if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(10)) {
                // Search for a nearby fire source (pyro/bonfire) within 0x300 units.
                i_fire = THING_find_nearest(
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y >> 8,
                    p_person->WorldPos.Z >> 8,
                    0x300,
                    1 << CLASS_PYRO);

                if (i_fire) {
                    p_person->Genus.Person->pcom_ai_arg = i_fire;

                    PCOM_set_person_move_mav_to_thing(p_person, TO_THING(i_fire), PCOM_MOVE_SPEED_WALK);
                } else {
                    // No fire found -- wait and try again.
                    PCOM_set_person_move_pause(p_person);
                }
            }

            break;

        case PCOM_MOVE_STATE_GOTO_THING:

            p_fire = TO_THING(p_person->Genus.Person->pcom_ai_arg);

            if (PCOM_get_dist_between(p_person, p_fire) < 0xa0) {
                // Close enough -- start warming hands.
                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_WARMUP;
                set_face_thing(p_person, p_fire);

                PCOM_set_person_move_still(p_person);
            }

            break;
        }

        break;

    case PCOM_AI_SUBSTATE_WARMUP:

        p_fire = TO_THING(p_person->Genus.Person->pcom_ai_arg);

        if (p_fire->Class != CLASS_PYRO) {
            // Fire went out -- look for a new one.
            PCOM_set_person_ai_warm_hands(p_person);
        } else {
            // Warming hands -- animation handled by normal animation system.
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// Returns true if the person was rendered (in view) recently.
// Used to choose between teleporting home and walking home.
// uc_orig: person_drawn_recently (fallen/Source/pcom.cpp)
static SLONG person_drawn_recently(Thing* p_person)
{
    return p_person->Flags & FLAGS_IN_VIEW;
}

// Instantly moves a person to their HomeX/HomeZ without pathfinding.
// Used when the NPC is far from home and not visible to the player.
// uc_orig: PCOM_teleport_home (fallen/Source/pcom.cpp)
void PCOM_teleport_home(Thing* p_person)
{
    GameCoord pos;

    pos.X = p_person->Genus.Person->HomeX << 8;
    pos.Y = p_person->WorldPos.Y;
    pos.Z = p_person->Genus.Person->HomeZ << 8;

    move_thing_on_map(p_person, &pos);

    p_person->Draw.Tweened->Angle = p_person->Genus.Person->HomeYaw << 3;
}

// Search range for bench/sofa used by PCOM_BENT_LAZY in PCOM_MOVE_STILL mode.
// uc_orig: PCOM_MAX_BENCH_WALK (fallen/Source/pcom.cpp)
#define PCOM_MAX_BENCH_WALK 0x200

// Top-level idle/patrol dispatcher for AI_STATE_NORMAL.
// Routes based on pcom_move: STILL (sitting, dancing, guard gun-draw), PATROL, WANDER,
// FOLLOW, WARM_HANDS, FOLLOW_ON_SEE.
// uc_orig: PCOM_process_normal (fallen/Source/pcom.cpp)
void PCOM_process_normal(Thing* p_person)
{
    UWORD i_target;
    Thing* p_target;
    SLONG dist;

    switch (p_person->Genus.Person->pcom_move) {
    case PCOM_MOVE_STILL:
    case PCOM_MOVE_DANCE:
    case PCOM_MOVE_HANDS_UP:
    case PCOM_MOVE_TIED_UP:

        if (p_person->State == STATE_MOVEING && (p_person->SubState == SUB_STATE_SIMPLE_ANIM || p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER)) {
            // Person is animating (sitting, dancing).
            if (p_person->Genus.Person->pcom_move == PCOM_MOVE_STILL) {
                // STILL people should only animate when sitting or scratching.
                if (p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_DOWN || p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_IDLE || p_person->Draw.Tweened->CurrentAnim == ANIM_IDLE_SCRATCH2) {
                    // Keep sitting / scratching.
                } else {
                    if ((PTIME(p_person) & 0x3) == 0) {
                        PCOM_set_person_move_animation(p_person, ANIM_IDLE_SCRATCH2);
                    } else {
                        PCOM_set_person_move_still(p_person);
                    }
                }
            }
        } else {
            // Check distance from home position.
            dist = PCOM_get_dist_from_home(p_person);

            if (dist > 256) {
                // Too far -- walk back home.
                PCOM_set_person_ai_homesick(p_person);
            } else {
                // Face the home direction.
                p_person->Draw.Tweened->Angle = p_person->Genus.Person->HomeYaw << 3;

                if (dist > PCOM_ARRIVE_DIST) {
                    if (!person_drawn_recently(p_person)) {
                        PCOM_teleport_home(p_person);
                    }
                } else {
                    // At home. Start a special animation if needed.
                    if (p_person->Genus.Person->pcom_move == PCOM_MOVE_DANCE || p_person->Genus.Person->pcom_move == PCOM_MOVE_HANDS_UP || p_person->Genus.Person->pcom_move == PCOM_MOVE_TIED_UP) {
                        UWORD anim;

                        if (p_person->Genus.Person->pcom_move == PCOM_MOVE_DANCE) {
                            static UWORD dance_anim[4] = {
                                ANIM_DANCE_BOOGIE,
                                ANIM_DANCE_WOOGIE,
                                ANIM_DANCE_HEADBANG,
                                ANIM_DANCE_BOOGIE
                            };

                            anim = dance_anim[THING_NUMBER(p_person) & 0x3];
                        } else {
                            // HANDS_UP and TIED_UP both use the hands-up animation.
                            anim = ANIM_HANDS_UP;
                        }

                        PCOM_set_person_move_animation(p_person, anim);

                        // Loop the animation in SUB_STATE_SIMPLE_ANIM_OVER.
                        p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
                    } else if (p_person->Genus.Person->pcom_bent & PCOM_BENT_LAZY) {
                        if ((p_person->SubState == SUB_STATE_SIMPLE_ANIM) || (p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER)) {
                            // Already sitting -- stay.
                            break;
                        }

                        if ((PTIME(p_person) & 0x3f) == 0) {
                            // Look for a bench or sofa nearby to sit on.
                            SLONG mx;
                            SLONG mz;
                            SLONG mx1;
                            SLONG mz1;
                            SLONG mx2;
                            SLONG mz2;
                            SLONG dx;
                            SLONG dy;
                            SLONG dz;
                            SLONG dist;
                            SLONG best_x;
                            SLONG best_y;
                            SLONG best_z;
                            SLONG best_yaw;
                            SLONG best_prim = NULL;
                            SLONG best_dist = PCOM_MAX_BENCH_WALK;

                            OB_Info* oi;

                            mx1 = (p_person->WorldPos.X >> 8) - PCOM_MAX_BENCH_WALK >> PAP_SHIFT_LO;
                            mz1 = (p_person->WorldPos.Z >> 8) - PCOM_MAX_BENCH_WALK >> PAP_SHIFT_LO;

                            mx2 = (p_person->WorldPos.X >> 8) + PCOM_MAX_BENCH_WALK >> PAP_SHIFT_LO;
                            mz2 = (p_person->WorldPos.Z >> 8) + PCOM_MAX_BENCH_WALK >> PAP_SHIFT_LO;

                            for (mx = mx1; mx <= mx2; mx++)
                                for (mz = mz1; mz <= mz2; mz++) {
                                    for (oi = OB_find(mx, mz); oi->prim; oi++) {
                                        if (oi->prim == PRIM_OBJ_PARK_BENCH || oi->prim == PRIM_OBJ_SOFA || oi->prim == PRIM_OBJ_ARMCHAIR) {
                                            dx = oi->x - (p_person->WorldPos.X >> 8);
                                            dy = oi->y - (p_person->WorldPos.Y >> 8);
                                            dz = oi->z - (p_person->WorldPos.Z >> 8);

                                            dist = abs(dx) + abs(dz) + abs(dy + dy);

                                            if (best_dist > dist) {
                                                best_prim = oi->prim;
                                                best_dist = dist;
                                                best_x = oi->x;
                                                best_y = oi->y;
                                                best_z = oi->z;
                                                best_yaw = oi->yaw;
                                            }
                                        }
                                    }
                                }

                            if (best_prim) {
                                if (PCOM_position_person_to_sit_on_prim(
                                        p_person,
                                        best_prim,
                                        best_x,
                                        best_y,
                                        best_z,
                                        best_yaw,
                                        person_drawn_recently(p_person))) {
                                    PCOM_set_person_move_animation(p_person, ANIM_SIT_DOWN);

                                    p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
                                }
                            }
                        }
                    } else if (p_person->Genus.Person->pcom_ai == PCOM_AI_GUARD) {
                        // Guards draw their gun while idle (unless they carry a pistol).
                        if (PCOM_person_has_any_sort_of_gun(p_person) && !(p_person->Flags & FLAGS_HAS_GUN)) {
                            if (!PCOM_person_has_gun_in_hand(p_person)) {
                                if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
                                    PCOM_set_person_move_draw_gun(p_person);
                                }
                            }
                        }
                    }
                }
            }
        }

        break;

    case PCOM_MOVE_PATROL:
    case PCOM_MOVE_PATROL_RAND:
        PCOM_process_patrol(p_person);
        break;

    case PCOM_MOVE_WANDER:
        PCOM_process_wander(p_person);
        break;

    case PCOM_MOVE_FOLLOW:

        if (p_person->Genus.Person->pcom_ai_substate == PCOM_AI_SUBSTATE_CANTFIND) {
            if ((GAME_TURN & 0xf) == 0) {
                i_target = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);

                if (i_target) {
                    if (can_a_see_b(p_person, TO_THING(i_target))) {
                        PCOM_set_person_ai_follow(p_person, TO_THING(i_target));
                    }
                }

            } else {
                PCOM_process_wander(p_person);
            }
        } else {
            i_target = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);

            if (i_target) {
                PCOM_set_person_ai_follow(p_person, TO_THING(i_target));
            }
        }

        break;

    case PCOM_MOVE_WARM_HANDS:

        if (PCOM_get_dist_from_home(p_person) > 0x200) {
            // Go home first so the correct barrel is selected.
            PCOM_set_person_ai_homesick(p_person);
        } else {
            PCOM_set_person_ai_warm_hands(p_person);
        }

        break;

    case PCOM_MOVE_FOLLOW_ON_SEE:

        // If close enough and has line of sight, switch to active FOLLOW mode.
        i_target = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);

        if (i_target) {
            if (PCOM_get_dist_between(p_person, TO_THING(i_target)) < 0x200) {
                if (can_a_see_b(p_person, TO_THING(i_target))) {
                    p_person->Genus.Person->pcom_move = PCOM_MOVE_FOLLOW;
                }
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// Scans for an active bomb (SPECIAL_BOMB + SPECIAL_SUBSTATE_ACTIVATED) within 0x300 units
// that p_person has line of sight to. Returns thing index of the nearest such bomb, or NULL.
// uc_orig: PCOM_find_bomb (fallen/Source/pcom.cpp)
UWORD PCOM_find_bomb(Thing* p_person)
{
    SLONG i;
    SLONG score;

    SLONG best_thing;
    SLONG best_score;

    Thing* p_found;

    PCOM_found_num = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        0x300,
        PCOM_found,
        PCOM_MAX_FIND,
        1 << CLASS_SPECIAL);

    best_score = +INFINITY;
    best_thing = NULL;

    for (i = 0; i < PCOM_found_num; i++) {
        p_found = TO_THING(PCOM_found[i]);

        if (p_found->Genus.Special->SpecialType == SPECIAL_BOMB && p_found->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
            if (can_a_see_b(p_person, p_found)) {
                score = THING_dist_between(p_person, p_found);

                if (score < best_score) {
                    best_score = score;
                    best_thing = PCOM_found[i];
                }
            }
        }
    }

    return best_thing;
}

// Routes to the correct PCOM_process_*() handler based on pcom_ai_state.
// Called from PCOM_process_state_change for most AI types.
// uc_orig: PCOM_process_default (fallen/Source/pcom.cpp)
void PCOM_process_default(Thing* p_person)
{
    switch (p_person->Genus.Person->pcom_ai_state) {
    case PCOM_AI_STATE_NORMAL:
        PCOM_process_normal(p_person);
        break;

    case PCOM_AI_STATE_INVESTIGATING:
        PCOM_process_investigating(p_person);
        break;

    case PCOM_AI_STATE_SEARCHING:
        break;

    case PCOM_AI_STATE_KILLING:
        PCOM_process_killing(p_person);
        break;

    case PCOM_AI_STATE_SLEEPING:
        break;

    case PCOM_AI_STATE_FLEE_PLACE:
    case PCOM_AI_STATE_FLEE_PERSON:
        PCOM_process_fleeing(p_person);
        break;

    case PCOM_AI_STATE_FOLLOWING:
        PCOM_process_following(p_person);
        break;

    case PCOM_AI_STATE_NAVTOKILL:
        PCOM_process_navtokill(p_person);
        break;

    case PCOM_AI_STATE_HOMESICK:
        PCOM_process_homesick(p_person);
        break;

    case PCOM_AI_STATE_LOOKAROUND:
        break;

    case PCOM_AI_STATE_FINDCAR:
        PCOM_process_findcar(p_person);
        break;

    case PCOM_AI_STATE_BDEACTIVATE:
        PCOM_process_bdeactivate(p_person);
        break;

    case PCOM_AI_STATE_LEAVECAR:
        PCOM_process_leavecar(p_person);
        break;

    case PCOM_AI_STATE_SNIPE:
        PCOM_process_snipe(p_person);
        break;

    case PCOM_AI_STATE_WARM_HANDS:
        PCOM_process_warm_hands(p_person);
        break;

    case PCOM_AI_STATE_KNOCKEDOUT:
        PCOM_process_knockedout(p_person);
        break;

    case PCOM_AI_STATE_TAUNT:
        PCOM_process_taunt(p_person);
        break;

    case PCOM_AI_STATE_ARREST:
        PCOM_process_arrest(p_person);
        break;

    case PCOM_AI_STATE_TALK:
        PCOM_process_talk(p_person);
        break;

    case PCOM_AI_STATE_HITCH:
        PCOM_process_hitch(p_person);
        break;

    case PCOM_AI_STATE_AIMLESS:
        PCOM_process_wander(p_person);
        break;

    case PCOM_AI_STATE_HANDS_UP:
        PCOM_process_hands_up(p_person);
        break;

    case PCOM_AI_STATE_GETITEM:
        PCOM_process_getitem(p_person);
        break;

    default:
        ASSERT(0);
        break;
    }
}

// When a MIB/GUARD/GANG/FIGHT_TEST NPC spots a threat, simultaneously aggros all
// nearby NPCs of the same type within radius 0x500.
// uc_orig: PCOM_alert_nearby_mib_to_attack (fallen/Source/pcom.cpp)
void PCOM_alert_nearby_mib_to_attack(Thing* p_person)
{
    {
        SLONG i;
        SLONG num_found;
        Thing* p_found;

        num_found = THING_find_sphere(
            p_person->WorldPos.X >> 8,
            p_person->WorldPos.Y >> 8,
            p_person->WorldPos.Z >> 8,
            0x500,
            THING_array,
            THING_ARRAY_SIZE,
            1 << CLASS_PERSON);

        for (i = 0; i < num_found; i++) {
            p_found = TO_THING(THING_array[i]);

            if (p_found->Genus.Person->pcom_ai == PCOM_AI_MIB || p_found->Genus.Person->pcom_ai == PCOM_AI_GUARD || p_found->Genus.Person->pcom_ai == PCOM_AI_GANG || p_found->Genus.Person->pcom_ai == PCOM_AI_FIGHT_TEST) {
                if (!(p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS)) {
                    if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING) {
                        PCOM_set_person_ai_navtokill(p_found, NET_PERSON(0));
                    }
                }
            }
        }
    }
}

// Scans nearby persons and returns the nearest hostile that is actively attacking
// p_client or p_bodyguard (pcom_ai_state KILLING or NAVTOKILL, target == client/guard).
// Knocked-out attackers and those already targeted by the client are deprioritised.
// Returns NULL if no such threat is found.
// uc_orig: PCOM_find_bodyguard_victim (fallen/Source/pcom.cpp)
Thing* PCOM_find_bodyguard_victim(Thing* p_bodyguard, Thing* p_client)
{
    SLONG i;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG num_found;
    SLONG best_score = INFINITY;
    Thing* best_victim = NULL;
    Thing* p_found;

    num_found = THING_find_sphere(
        p_bodyguard->WorldPos.X >> 8,
        p_bodyguard->WorldPos.Y >> 8,
        p_bodyguard->WorldPos.Z >> 8,
        0x800,
        THING_array,
        THING_ARRAY_SIZE,
        1 << CLASS_PERSON);

    for (i = 0; i < num_found; i++) {
        p_found = TO_THING(THING_array[i]);

        if (is_person_dead(p_found)) {
            continue;
        }

        if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_KILLING || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL) {
            if (p_found->Genus.Person->pcom_ai_arg == THING_NUMBER(p_client) || p_found->Genus.Person->pcom_ai_arg == THING_NUMBER(p_bodyguard)) {
                // This person is attacking the client or bodyguard.
                dx = p_found->WorldPos.X - p_bodyguard->WorldPos.X;
                dy = p_found->WorldPos.Y - p_bodyguard->WorldPos.Y;
                dz = p_found->WorldPos.Z - p_bodyguard->WorldPos.Z;

                dist = abs(dx) + abs(dz) + abs(dy << 1);

                if (p_client->Genus.Person->Target == THING_array[i]) {
                    // Deprioritise targets the client is already engaging
                    // so bodyguard covers another attacker.
                    dist <<= 2;
                }

                if (is_person_ko(p_found)) {
                    // KO'd attackers pose less of a threat.
                    dist <<= 1;
                }

                if (dist < best_score) {
                    best_score = dist;
                    best_victim = p_found;
                }
            }
        }
    }

    return best_victim;
}
"""

with open('C:/WORK/OpenChaos/new_game/src/new/ai/pcom.cpp', 'a', encoding='utf-8') as f:
    f.write(content)
print("Done, appended", len(content), "chars")
