#!/usr/bin/env python3
"""Append PCOM chunk 8 to new/ai/pcom.cpp"""

code = r"""
// uc_orig: PCOM_process_state_change (fallen/Source/pcom.cpp)
void PCOM_process_state_change(Thing* p_person)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;
    SLONG home_x;
    SLONG home_z;
    SLONG bomb;
    Thing* p_target;
    SLONG i_target;

    // By default no NPC should have their gun drawn while idle.
    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
        if ((p_person->Flags & FLAGS_HAS_GUN) && (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)) {
            if (p_person->Genus.Person->pcom_move_state != PCOM_MOVE_STATE_ANIMATION || p_person->Genus.Person->pcom_move_substate != PCOM_MOVE_SUBSTATE_GUNAWAY) {
                PCOM_set_person_move_gun_away(p_person);
            }
            return;
        }
    }

    // Non-driver AIs that find themselves driving exit the vehicle.
    if ((p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) && p_person->Genus.Person->pcom_ai != PCOM_AI_DRIVER && p_person->Genus.Person->pcom_ai != PCOM_AI_COP_DRIVER && p_person->Genus.Person->pcom_ai_state != PCOM_AI_STATE_LEAVECAR) {
        PCOM_set_person_ai_leavecar(p_person, PCOM_EXCAR_NORMAL, 0, 0);
    }

    switch (p_person->Genus.Person->pcom_ai) {
    case PCOM_AI_NONE:
        break;

    case PCOM_AI_CIV:
        PCOM_process_default(p_person);
        /*
                if (((GAME_TURN + THING_NUMBER(p_person)) & 0xff) == 0)
                {
                        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL   ||
                                p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK ||
                                p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS)
                        {
                                if (p_person->Genus.Person->pcom_ai_memory && !EWAY_stop_player_moving())
                                {
                                        Thing *p_nasty = TO_THING(p_person->Genus.Person->pcom_ai_memory);
                                        if (PCOM_get_dist_between(p_person, p_nasty) < 0x120)
                                        {
                                                PCOM_set_person_ai_talk_to(p_person, p_nasty, PCOM_AI_SUBSTATE_TALK_TELL, FALSE);
                                                if (p_nasty->Genus.Person->PlayerID && p_nasty->Genus.Person->PersonType == PERSON_DARCI)
                                                {
                                                        PANEL_new_text(p_person, 4000, EWAY_get_fake_wander_message(EWAY_FAKE_MESSAGE_ANNOYED));
                                                }
                                        }
                                }
                        }
                }
        */
        break;

    case PCOM_AI_GUARD:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NAVTOKILL:

        {
            Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

            if (PCOM_get_dist_between(p_person, p_target) > 20 * 0x100) {
                // Target too far away -- give up.
                PCOM_set_person_ai_normal(p_person);
                return;
            }
        }

            if (p_person->Genus.Person->pcom_zone) {
                // Zone code will keep the guard in range; just pursue.
                PCOM_process_navtokill(p_person);
            } else if (p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER) {
                // Wandering guards don't check home distance.
                PCOM_process_navtokill(p_person);
            } else {
                // If the guard is far from his guard post, return home first.
                home_x = p_person->Genus.Person->HomeX; //<< 8;
                home_z = p_person->Genus.Person->HomeZ; //<< 8;

                dist = PCOM_person_dist_from(p_person, home_x, home_z);

                if (dist > 16 * 0x100) // If more than sixteen mapsquares away
                {
                    PCOM_set_person_ai_homesick(p_person);
                } else {
                    PCOM_process_navtokill(p_person);
                }
            }

            break;

        default:
            PCOM_process_default(p_person);
            break;
        }

        // Every 4 ticks check if the guard can see a threat.
        //			if ((GAME_TURN & 0x3) == 0)
        if ((PTIME(p_person) & 0x3) == 0) {
            SLONG look = FALSE;

            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK) {
                look = TRUE;
            }

            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING) {
                if (p_person->Genus.Person->pcom_ai_substate == PCOM_AI_SUBSTATE_SUPRISED) {
                    look = (p_person->Genus.Person->pcom_ai_counter >= PCOM_get_duration(4));
                } else {
                    look = TRUE;
                }
            }

            if (look) {
                // Can you see the player?
                p_target = PCOM_can_i_see_person_to_attack(p_person);

                if (p_target) {
                    PCOM_alert_nearby_mib_to_attack(p_person);
                    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
                        // cant navigate to enemy so taunt
                        PCOM_set_person_ai_taunt(p_person, p_target);
                    }
                    //						else //play wav anyway MikeD
                    {
                        /*
                                                        if(((GAME_TURN+THING_NUMBER(p_person))&255)==0)
                                                                MFX_play_thing(THING_NUMBER(p_person),S_HEY_YOU,MFX_REPLACE,p_person);
                        */
                    }
                    PCOM_oscillate_tympanum(
                        PCOM_SOUND_HEY,
                        p_person,
                        p_person->WorldPos.X >> 8,
                        p_person->WorldPos.Y >> 8,
                        p_person->WorldPos.Z >> 8);

                    // track_enemy(p_person);
                }
            }
        }

        break;

    case PCOM_AI_ASSASIN:
    case PCOM_AI_SHOOT_DEAD:

        if (p_person->Genus.Person->pcom_ai_other == NULL) {
        } else {
            if (p_person->Genus.Person->pcom_ai_other == NULL) {
            } else {

                i_target = EWAY_get_person(p_person->Genus.Person->pcom_ai_other);

                if (i_target) {
                    p_target = TO_THING(i_target);

                    if (is_person_dead(p_target)) {
                        // Assassin changes to gang behaviour once the target is dead.
                        p_person->Genus.Person->pcom_ai = PCOM_AI_GANG;
                        p_person->Genus.Person->pcom_move = PCOM_MOVE_WANDER;
                        PCOM_set_person_ai_normal(p_person);
                        return;
                    }

                    switch (p_person->Genus.Person->pcom_ai_state) {
                    case PCOM_AI_STATE_NORMAL:

                        switch (p_person->Genus.Person->pcom_move) {
                        case PCOM_MOVE_STILL:
                            PCOM_set_person_ai_snipe(p_person, p_target);
                            break;
                        case PCOM_MOVE_PATROL:
                        case PCOM_MOVE_PATROL_RAND:
                        case PCOM_MOVE_WANDER:
                            p_person->Genus.Person->pcom_move = PCOM_MOVE_STILL;
                            break;
                        case PCOM_MOVE_FOLLOW:
                            PCOM_set_person_ai_navtokill(p_person, p_target);
                            break;
                        }

                        break;

                    default:
                        PCOM_process_default(p_person);
                        break;
                    }
                } else {
                    // This assasin has no target yet...
                    PCOM_process_default(p_person);
                }
            }
        }

        break;

    case PCOM_AI_BOSS:
        PCOM_process_default(p_person);
        break;

    case PCOM_AI_COP:
        PCOM_process_default(p_person);
        break;

    case PCOM_AI_GANG:

        PCOM_process_default(p_person);

        {
            SLONG look = FALSE;

            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING) {
                if (p_person->Genus.Person->pcom_ai_substate == PCOM_AI_SUBSTATE_SUPRISED) {
                    look = (p_person->Genus.Person->pcom_ai_counter >= PCOM_get_duration(4));
                } else {
                    look = TRUE;
                }
            }

            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK) {
                look = TRUE;
            }

            if (look) {
                ASSERT(p_person->SubState != SUB_STATE_GRAPPLEE);
                ASSERT(p_person->SubState != SUB_STATE_GRAPPLE_HELD);

                if ((PTIME(p_person) & 0x7) == 0)
                //					if ((GAME_TURN & 0x7) == 0)
                {
                    // Can I see somebody worth taunting?
                    p_target = PCOM_can_i_see_person_to_taunt(p_person);

                    if (p_target) {
                        PCOM_set_person_ai_taunt(p_person, p_target);

                        PCOM_oscillate_tympanum(
                            PCOM_SOUND_WANKER,
                            p_person,
                            p_person->WorldPos.X >> 8,
                            p_person->WorldPos.Y >> 8,
                            p_person->WorldPos.Z >> 8);

                        return;
                    }
                }
            }
        }

        break;

    case PCOM_AI_DOORMAN:
        PCOM_process_default(p_person);
        break;

    case PCOM_AI_BODYGUARD:

    {
        UWORD i_client = EWAY_get_person(p_person->Genus.Person->pcom_ai_other);
        Thing* p_client = NULL;

        if (i_client) {
            p_client = TO_THING(i_client);
        }

        p_person->Genus.Person->Flags2 &= ~FLAG2_PERSON_INVULNERABLE;

        // Turn invulnerable when very far from player client.
        if (p_client && p_client->Genus.Person->PlayerID) {
            SLONG dx = abs(p_client->WorldPos.X - p_person->WorldPos.X);
            SLONG dy = abs(p_client->WorldPos.Y - p_person->WorldPos.Y);
            SLONG dz = abs(p_client->WorldPos.Z - p_person->WorldPos.Z);

            SLONG dist = QDIST3(dx, dy, dz);

            if (dist > 20 * 0x10000) {
                p_person->Genus.Person->Flags2 |= FLAG2_PERSON_INVULNERABLE;
            }
        }

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:
        case PCOM_AI_STATE_FOLLOWING:

        {
            // Is our client under attack?
            if (p_client) {
                Thing* p_target = PCOM_find_bodyguard_victim(p_person, p_client);

                if (p_target) {
                    //								MFX_play_thing(THING_NUMBER(p_person),S_OIGUVNOR,MFX_REPLACE,p_person);
                    PCOM_set_person_ai_navtokill(p_person, p_target);
                }
            }
        }

            // Fall-through to default processing.

        default:
            PCOM_process_default(p_person);
            break;
        }
    }

    break;

    case PCOM_AI_COP_DRIVER:

        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
            /*
                            if ((GAME_TURN & 0x7) == 0)
                            {
                                    p_target = PCOM_can_i_see_person_to_arrest(p_person);

                                    if (p_target)
                                    {
                                            p_target->Genus.Person->Flags |= FLAG_PERSON_FELON;

                                            PCOM_set_person_ai_leavecar(
                                                    p_person,
                                                    PCOM_EXCAR_ARREST_PERSON,
                                                    0,
                                                    THING_NUMBER(p_target));
                                    }
                            }
            */
        }

        // FALLTHROUGH to driving code.

    case PCOM_AI_DRIVER:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:

            if (!(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING)) {
                // Not in a car -- find one.
                PCOM_set_person_ai_findcar(p_person, NULL);
            } else {
                switch (p_person->Genus.Person->pcom_move) {
                case PCOM_MOVE_STILL:
                    PCOM_process_driving_still(p_person);
                    break;
                case PCOM_MOVE_PATROL:
                case PCOM_MOVE_PATROL_RAND:
                    PCOM_process_driving_patrol(p_person);
                    break;
                case PCOM_MOVE_FOLLOW:
                    break;
                case PCOM_MOVE_WANDER:
                    PCOM_process_driving_wander(p_person);
                    break;
                default:
                    ASSERT(0);
                    break;
                }
            }

            break;

        default:
            PCOM_process_default(p_person);
            break;
        }

        break;

    case PCOM_AI_BDISPOSER:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:

            PCOM_process_normal(p_person);

            bomb = PCOM_find_bomb(p_person);

            if (bomb) {
                PCOM_set_person_ai_bdeactivate(p_person, TO_THING(bomb));
            }

            break;

        case PCOM_AI_STATE_FOLLOWING:

            PCOM_process_following(p_person);

            bomb = PCOM_find_bomb(p_person);

            if (bomb) {
                PCOM_set_person_ai_bdeactivate(p_person, TO_THING(bomb));
            }

            break;

        default:
            PCOM_process_default(p_person);
            break;
        }

        break;

    case PCOM_AI_BIKER:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:

        {
            switch (p_person->Genus.Person->pcom_move) {
            case PCOM_MOVE_STILL:
                PCOM_process_driving_still(p_person);
                break;
            case PCOM_MOVE_PATROL:
            case PCOM_MOVE_PATROL_RAND:
                PCOM_process_driving_patrol(p_person);
                break;
            case PCOM_MOVE_FOLLOW:
                break;
            case PCOM_MOVE_WANDER:
                PCOM_process_driving_wander(p_person);
                break;
            default:
                ASSERT(0);
                break;
            }
        }

        break;

        default:
            PCOM_process_default(p_person);
            break;
        }

        break;

    case PCOM_AI_FIGHT_TEST:

        if (p_person->Genus.Person->pcom_ai_other & PCOM_COMBAT_COMBO_KKK) {
            combo_display = 2;
        } else if (p_person->Genus.Person->pcom_ai_other & PCOM_COMBAT_COMBO_PPP) {
            combo_display = 1;
        }

        PCOM_process_default(p_person);

        if (!(p_person->Genus.Person->pcom_bent & PCOM_BENT_ROBOT)) {
            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS) {
                if (PCOM_get_dist_between(p_person, NET_PERSON(0)) < 0x200) {
                    PCOM_alert_nearby_mib_to_attack(p_person); // Not just MIBs!
                }
            }
        }

        break;

    case PCOM_AI_BULLY:

        //			if(Keys[KB_B])
        //				FC_cam[0].focus	= p_person;

        PCOM_process_default(p_person);

        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS) {
            if ((PTIME(p_person) & 0x7) == 0)
            //				if ((GAME_TURN & 0x7) == 0)
            {
                p_target = PCOM_can_i_see_person_to_bully(p_person);

                if (p_target) {
                    PCOM_set_person_ai_kill_person(p_person, p_target);
                    return;
                }
            }
        }

        break;

    case PCOM_AI_SUICIDE:

        // Instant random death -- simplest possible AI.
        p_person->Genus.Person->Health = 0;

        set_person_dead(
            p_person,
            NULL,
            PERSON_DEATH_TYPE_SHOT_PISTOL, // was combat
            Random() & 0x1,
            Random() % 3);

        if (GAME_TURN < 32) {
            SLONG c0;
            for (c0 = 0; c0 < 100; c0++) {
                if (p_person->StateFn)
                    p_person->StateFn(p_person);
            }
        }

        //			while(person_normal_animate(p_person)==0);

        break;

    case PCOM_AI_FLEE_PLAYER:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:
        case PCOM_AI_STATE_HOMESICK:

            // Is this person too near Darci?
            if (PCOM_get_dist_between(p_person, NET_PERSON(0)) < 0x600) {
                PCOM_set_person_ai_flee_person(p_person, NET_PERSON(0));
            }

            // FALL-THROUGH TO DEFAULT PROCESSING!

        default:
            PCOM_process_default(p_person);
            break;
        }

        break;

    case PCOM_AI_KILL_COLOUR:

        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
            // Find nearest person of the hated colour to kill.
            UWORD list;

            SLONG dist;
            SLONG best_dist = INFINITY;
            Thing* best_target = NULL;
            Thing* p_found;

            list = thing_class_head[CLASS_PERSON];

            SLONG hate_colour;
            SLONG hate_example;

            hate_example = EWAY_get_person(p_person->Genus.Person->pcom_ai_other);

            if (hate_example) {
                Thing* p_example = TO_THING(hate_example);

                ASSERT(p_example->Class == CLASS_PERSON);

                hate_colour = p_example->Genus.Person->pcom_colour;

                while (list) {
                    p_found = TO_THING(list);

                    list = p_found->NextLink;

                    if (p_found->Genus.Person->pcom_colour == hate_colour && !is_person_dead(p_found) && !(p_found->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER)) {
                        if (p_person->Genus.Person->pcom_zone) {
                            // Ignore people outside this guard's zone.
                            if (!(PCOM_get_zone_for_position(p_found->WorldPos.X >> 8, p_found->WorldPos.Z >> 8) & p_person->Genus.Person->pcom_zone)) {
                                continue;
                            }
                        }

                        dist = PCOM_get_dist_between(p_person, p_found);

                        if (best_dist > dist) {
                            best_dist = dist;
                            best_target = p_found;
                        }
                    }
                }

                if (best_target) {
                    PCOM_set_person_ai_kill_person(p_person, best_target);
                }

                /*
                else
                {
                        if (p_person->Genus.Person->PersonType == PERSON_COP)
                        {
                                p_person->Genus.Person->pcom_ai = PCOM_AI_COP;
                        }
                        else
                        {
                                p_person->Genus.Person->pcom_ai    = PCOM_AI_CIV;
                                p_person->Genus.Person->pcom_bent |= PCOM_BENT_FIGHT_BACK;
                        }
                }
                */
            }
        }

        PCOM_process_default(p_person);

        break;

    case PCOM_AI_MIB:

        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
            // Kills the player on sight.
            if (can_a_see_b(p_person, NET_PERSON(0)) && !stealth_debug) {
                PCOM_alert_nearby_mib_to_attack(p_person);
            }
        }

        PCOM_process_default(p_person);

        break;

    case PCOM_AI_BANE:

        if (p_person->Genus.Person->pcom_ai_state != PCOM_AI_STATE_SUMMON) {
            PCOM_set_person_ai_summon(p_person);
        }

        PCOM_process_summon(p_person);

        break;

    case PCOM_AI_HYPOCHONDRIA:

        set_person_injured(p_person);

        break;

    default:
        ASSERT(0);
        break;
    }
}

// Bit flags returned by PCOM_find_runover_thing.
// uc_orig: PCOM_RUNOVER_STOP (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_STOP      (1 << 0)
// uc_orig: PCOM_RUNOVER_BEEP_HORN (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_BEEP_HORN (1 << 1)
// uc_orig: PCOM_RUNOVER_SHOUT_OUT (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_SHOUT_OUT (1 << 2)
// uc_orig: PCOM_RUNOVER_TURN_LEFT (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_TURN_LEFT (1 << 3)
// uc_orig: PCOM_RUNOVER_TURN_RIGHT (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_TURN_RIGHT (1 << 4)
// uc_orig: PCOM_RUNOVER_SLOW_DOWN (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_SLOW_DOWN (1 << 5)
// uc_orig: PCOM_RUNOVER_RUNAWAY (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_RUNAWAY   (1 << 6)
// uc_orig: PCOM_RUNOVER_REVERSE (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_REVERSE   (1 << 7)

// BLOCKED(C) is a debug-coloring marker compiled to nothing on PC.
#define BLOCKED(C)

// How many objects PCOM_find_runover_thing scans at once.
// uc_orig: PCOM_RUNOVER_FIND (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_FIND 8

// uc_orig: PCOM_find_runover_thing (fallen/Source/pcom.cpp)
SLONG PCOM_find_runover_thing(Thing* p_person, SLONG dangle)
{
    SLONG i;

    SLONG dx;
    SLONG dz;

    SLONG px;
    SLONG pz;

    SLONG cx;
    SLONG cz;

    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;

    SLONG what;
    SLONG dist;
    SLONG cprod;
    SLONG angle;

    UWORD found[PCOM_RUNOVER_FIND];
    SLONG num;
    SLONG velocity;

    Thing* p_vehicle;
    Vehicle* veh;
    Thing* p_found;
    Vehicle* v_found;

    ASSERT(p_person->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING));

    p_vehicle = TO_THING(p_person->Genus.Person->InCar);
    veh = p_vehicle->Genus.Vehicle;

    // Find all things ahead of the vehicle.
    num = VEH_find_runover_things(p_vehicle, found, PCOM_RUNOVER_FIND, dangle);

    // Vehicle movement vector and speed.
    switch (p_vehicle->Class) {
    case CLASS_VEHICLE:
        angle = p_vehicle->Genus.Vehicle->Angle & 2047;
        velocity = Root((veh->VelX >> 4) * (veh->VelX >> 4) + (veh->VelZ >> 4) * (veh->VelZ >> 4)) >> 4;
        break;

    case CLASS_BIKE:
        angle = p_vehicle->Draw.Mesh->Angle & 2047;
        velocity = 0;
        break;

    default:
        ASSERT(0);
        break;
    }

    dx = -SIN(angle) >> 8;
    dz = -COS(angle) >> 8;

    // get some info
    SLONG wx = p_vehicle->WorldPos.X >> 8;
    SLONG wz = p_vehicle->WorldPos.Z >> 8;

    SLONG onroad = ROAD_is_road(wx >> 8, wz >> 8);

    // find nearest road
    SLONG rn1;
    SLONG rn2;

    if ((p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_DRIVE_DOWN) || (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_PARK_CAR_ON_ROAD)) {
        rn1 = p_person->Genus.Person->pcom_move_arg & 0xFF;
        rn2 = (p_person->Genus.Person->pcom_move_arg >> 8) & 0xFF;
    } else {
        ROAD_find(wx, wz, &rn1, &rn2);
    }

    // get side
    SLONG rd = ROAD_signed_dist(rn1, rn2, wx, wz);

    // find nearest node
    SLONG nn;
    SLONG nnd;

    nn = ROAD_nearest_node(rn1, rn2, wx, wz, &nnd);

    // set action to none
    what = 0;

    // if we're near a junction, add objects
    SLONG orig_num = num;

    if (nnd < NEAR_JUNCTION) {
        SLONG jx, jz;

        ROAD_node_pos(nn, &jx, &jz);

        if ((jx - wx) * dx + (jz - wz) * dz > 0) {
            // approaching it
            num += THING_find_sphere(jx, p_person->WorldPos.Y >> 8, jz, JN_RADIUS_IN, found + num, PCOM_RUNOVER_FIND - num, (1 << CLASS_PERSON) | (1 << CLASS_VEHICLE));
            what = PCOM_RUNOVER_SLOW_DOWN;
        }
    }

    // remove self from list
    for (i = 0; i < num; i++) {
        if ((found[i] == THING_NUMBER(p_person)) || (found[i] == THING_NUMBER(p_vehicle))) {
            found[i] = found[--num];
        }
    }

    // check objects in list
    for (i = 0; i < num; i++) {
        p_found = TO_THING(found[i]);

        BLOCKED(0);

        if ((p_found->Class == CLASS_PERSON) && (p_found->State == STATE_DYING || p_found->State == STATE_DEAD)) {
            // Ignore dead or dying people
            continue;
        }

        // if it's on a junction, check dot product
        if (num >= orig_num) {
            SLONG vx = (p_found->WorldPos.X - p_person->WorldPos.X) >> 8;
            SLONG vz = (p_found->WorldPos.Z - p_person->WorldPos.Z) >> 8;

            if (vx * dx + vz * dz < 0)
                continue; // behind you!
        }

        BLOCKED(0x0000FF);

        // if we're on the road ...
        if (onroad) {
            // then ignore off-road things
            if (!ROAD_is_road(p_found->WorldPos.X >> 16, p_found->WorldPos.Z >> 16))
                continue;

            // find which road the thing is on
            SLONG trn1;
            SLONG trn2;

            ROAD_find(p_found->WorldPos.X >> 8, p_found->WorldPos.Z >> 8, &trn1, &trn2);

            // find nearest node
            SLONG tnn;
            SLONG tnnd;

            tnn = ROAD_nearest_node(trn1, trn2, p_found->WorldPos.X >> 8, p_found->WorldPos.Z >> 8, &tnnd);

            // handle junctions
            // #if 0
            if ((nnd < AT_JUNCTION) && (p_found->Class == CLASS_VEHICLE)) {
                // *on* the junction
                if ((tnnd < AT_JUNCTION) && (tnn == nn) && (ROAD_node_degree(nn) > 2)) {
                    // another car is on the junction
                    if (p_vehicle < p_found) {
                        // reverse back to make room
                        BLOCKED(0x888888);
                        return PCOM_RUNOVER_REVERSE;
                    }
                }
                // ignore other cars when we're *on* the junction
                continue;
            }
            // #endif

            if ((nnd < NEAR_JUNCTION) && (p_found->Class == CLASS_VEHICLE) && (tnn == nn) && (ROAD_node_degree(nn) > 2)) {
                // near junction, looking at a car on the same junction, not a bend
                if (tnnd < AT_JUNCTION) {
                    // stop if approaching a busy junction
                    BLOCKED(0x888888);
                    return PCOM_RUNOVER_STOP;
                } else if (tnnd < NEAR_JUNCTION) {
                    // stop if both approaching and other car is nearer
                    if (tnnd < nnd) {
                        BLOCKED(0x888888);
                        return PCOM_RUNOVER_STOP;
                    }
                }
            }

            if (nnd > AT_JUNCTION) // && (abs(dangle) < 32))
            {
                // look at the side of the road we're both on
                if (((rn1 == trn1) && (rn2 == trn2)) || ((rn1 == trn2) && (rn2 == trn1))) {
                    SLONG trd = ROAD_signed_dist(rn1, rn2, p_found->WorldPos.X >> 8, p_found->WorldPos.Z >> 8);

                    if (abs(trd - rd) > 0xC0)
                        continue;
                }
            }
        }

        switch (p_found->Class) {
        case CLASS_PERSON:
            if (p_found->OnFace)
                break; // ignore people on cars

            if ((veh->Type != VEH_TYPE_VAN) && (veh->Type != VEH_TYPE_AMBULANCE) && (veh->Type != VEH_TYPE_WILDCATVAN)) {
                // can't hijack vans or ambulances
                if ((p_found->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) || (p_found->Genus.Person->SpecialUse)) {
                    // person has a gun out
                    SLONG angle = calc_angle(dx, dz);
                    SLONG dangle = p_found->Draw.Tweened->Angle - angle;

                    if (abs(dangle) < 256) {
                        // pointing it in our direction
                        if (veh->Flags & FLAG_VEH_SHOT_AT) {
                            // and shooting it!
                            if (Random() & 0x08) {
                                // scared!
                                PCOM_runover_scary_person = p_found;
                                BLOCKED(0x888888);
                                return PCOM_RUNOVER_STOP | PCOM_RUNOVER_RUNAWAY;
                            } else {
                                // not scared
                                veh->Flags &= ~FLAG_VEH_SHOT_AT;
                            }
                        }
                    }
                }
            }

            if (ROAD_is_zebra(
                    p_found->WorldPos.X >> 16,
                    p_found->WorldPos.Z >> 16)) {
                // Always stop for people on zebra crossings...
                BLOCKED(0x888888);
                return PCOM_RUNOVER_STOP;
            } else {
                BLOCKED(0x888888);
                switch (((GAME_TURN) + THING_NUMBER(p_vehicle)) & 0x7f) {
                case 0:
                    return PCOM_RUNOVER_STOP | PCOM_RUNOVER_BEEP_HORN;
                case 25:
                    return PCOM_RUNOVER_STOP | PCOM_RUNOVER_SHOUT_OUT;
                default:
                    return PCOM_RUNOVER_STOP;
                }
            }

            break;

        case CLASS_VEHICLE: {
            v_found = p_found->Genus.Vehicle;

            SLONG vel = Root((v_found->VelX >> 4) * (v_found->VelX >> 4) + (v_found->VelZ >> 4) * (v_found->VelZ >> 4)) >> 4;

            // Do we stop for a car in front of us or do we try and drive around?
            SLONG avoid = FALSE;

            // avoid parked (driverless) cars and dead cars
            if (!v_found->Driver)
                avoid = TRUE;
            if ((p_found->State == STATE_DYING) || (p_found->State == STATE_DEAD))
                avoid = TRUE;

            if (p_person->Genus.Person->pcom_bent & PCOM_BENT_DILIGENT) {
                // Diligent people are in a hurry!
                avoid = TRUE;
            }

            if (avoid) {
                if (vel < (velocity >> 2)) {
                    // no, just stop
                    BLOCKED(0x888888);
                    return PCOM_RUNOVER_STOP;
                } else // if (vel < velocity)
                {
                    BLOCKED(0xFF0000);
                    return PCOM_RUNOVER_STOP;
                }
            } else {
                //						if (vel < velocity)
                {
                    BLOCKED(0x00FF00);
                    return PCOM_RUNOVER_STOP;
                }
            }
        } break;

        default:
            ASSERT(0);
            break;
        }
    }

    return what;
}

// uc_orig: PCOM_process_movement (fallen/Source/pcom.cpp)
void PCOM_process_movement(Thing* p_person)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;
    SLONG what;
    SLONG dest_x;
    SLONG dest_z;
    SLONG goal_x;
    SLONG goal_z;
    SLONG ladder;
    SLONG wangle;
    SLONG dangle;
    SLONG wspeed;
    SLONG dspeed;
    SLONG dlane;

    SLONG renav = FALSE;

    Thing* p_vehicle;
    Thing* p_target;
    Thing* p_bike;

    SLONG steer;
    SLONG accel;

    if (p_person->State == STATE_DYING || p_person->State == STATE_DEAD)
        return;

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_PLAYER:
        break;

    case PCOM_MOVE_STATE_STILL:

        if (p_person->SubState == SUB_STATE_GRAPPLE_HELD) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;
        }

        break;

    case PCOM_MOVE_STATE_GOTO_THING_SLIDE:

        // Finished sliding?
        if (p_person->State == STATE_IDLE || p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
        } else {
            // Keep facing the slide target.
            ASSERT(p_person->Genus.Person->pcom_move_arg);
            turn_towards_thing(p_person, TO_THING(p_person->Genus.Person->pcom_move_arg));
        }

        break;

    case PCOM_MOVE_STATE_GOTO_XZ:
    case PCOM_MOVE_STATE_GOTO_WAYPOINT:
    case PCOM_MOVE_STATE_GOTO_THING:

        if (p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS) {
            break;
        }

        switch (p_person->Genus.Person->pcom_move_substate) {
        case PCOM_MOVE_SUBSTATE_GOTO:

            PCOM_get_mav_action_pos(p_person, &goal_x, &goal_z);
            if (ControlFlag && allow_debug_keys) {
                AENG_world_line(
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y >> 8,
                    p_person->WorldPos.Z >> 8,
                    16,
                    0x00ffff00,
                    goal_x,
                    p_person->WorldPos.Y >> 8,
                    goal_z,
                    0,
                    0x000000ff,
                    TRUE);
            }
            /*
                                    goal_x &= 0xffffff00;
                                    goal_z &= 0xffffff00;

                                    goal_x |= 0x80;
                                    goal_z |= 0x80;
            */
            dist = PCOM_person_dist_from(p_person, goal_x, goal_z);

            if (dist < PCOM_ARRIVE_DIST) {
                if (p_person->Genus.Person->pcom_move_ma.action == MAV_ACTION_GOTO) {
                    // Reached subgoal -- request renavigation to next.
                    renav = TRUE;
                } else {
                    // Arrived but need to perform an action (jump, pullup, ladder, etc.).
                    p_person->Draw.Tweened->AngleTo = p_person->Draw.Tweened->Angle = PCOM_get_angle_for_dir(p_person->Genus.Person->pcom_move_ma.dir);

                    switch (p_person->Genus.Person->pcom_move_ma.action) {
                    case MAV_ACTION_JUMP:
                    case MAV_ACTION_JUMPPULL:
                    case MAV_ACTION_JUMPPULL2:
                        set_person_running_jump(p_person);
                        break;

                    case MAV_ACTION_PULLUP:
                        set_person_running_jump(p_person);
                        break;

                    case MAV_ACTION_CLIMB_OVER:
                        set_person_running_jump(p_person);
                        break;

                    case MAV_ACTION_FALL_OFF:
                        set_person_walking(p_person);
                        break;

                    case MAV_ACTION_LADDER_UP:

                        ladder = find_nearby_ladder_colvect(p_person);

                        if (ladder) {
                            set_person_climb_ladder(p_person, ladder);
                        } else {
                            ASSERT(0);
                        }

                        break;

                    default:
                        ASSERT(0);
                        break;
                    }

                    // Process the action.
                    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_ACTION;
                }
            } else {
                if (p_person->State == STATE_DANGLING && p_person->SubState == SUB_STATE_DANGLING) {
                    // Fell and ended up dangling -- pull up immediately.
                    set_person_pulling_up(p_person);
                } else if (p_person->Genus.Person->SlideOdd >= 50) {
                    if (p_person->Genus.Person->pcom_ai == PCOM_AI_CIV && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER && p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
                        // Wandering civs don't jump.
                    } else {
                        // Stuck sliding for 50 frames -- force a jump to escape.
                        set_person_running_jump(p_person);

                        p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_ACTION;
                        p_person->Genus.Person->SlideOdd = 0;
                    }
                } else if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(20)) {
                    // Timeout -- force renavigation if in a safe state.
                    if ((p_person->State == STATE_GOTOING) || (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN) || (p_person->State == STATE_IDLE)) {
                        renav = TRUE;
                    } else {
                        // Don't interrupt a complex manoeuvre (ladder/jump).
                    }
                }
            }

            break;

        case PCOM_MOVE_SUBSTATE_ACTION:

            switch (p_person->State) {
            case STATE_MOVEING:

                if (p_person->SubState == SUB_STATE_RUNNING) {
                    // Running after completing a jump -- renav.
                    renav = TRUE;
                }

                break;

            case STATE_IDLE:

                // Stopped -- time for a renavigation!
                renav = TRUE;

                break;

            case STATE_LANDING:
            case STATE_JUMPING:
            case STATE_FIGHTING:
            case STATE_FALLING:
            case STATE_USE_SCENERY:
            case STATE_DOWN:
            case STATE_HIT:
            case STATE_CHANGE_LOCATION:
            case STATE_DYING:
            case STATE_DEAD:
                break;

            case STATE_DANGLING:

                if (p_person->SubState == SUB_STATE_DANGLING) {
                    switch (p_person->Genus.Person->pcom_move_ma.action) {
                    case MAV_ACTION_JUMPPULL:
                    case MAV_ACTION_JUMPPULL2:
                    case MAV_ACTION_PULLUP:
                        set_person_pulling_up(p_person);
                        break;

                    default:
                        set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
                        break;
                    }
                } else if (p_person->SubState == SUB_STATE_DANGLING_CABLE) {
                    // Should never be on a cable!
                    set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
                }

                break;

            case STATE_CLIMB_LADDER:

                if (p_person->SubState == SUB_STATE_ON_LADDER) {
                    p_person->SubState = SUB_STATE_CLIMB_UP_LADDER;
                }

                break;

            case STATE_HIT_RECOIL:
                break;

            case STATE_CLIMBING:

                if (p_person->SubState == SUB_STATE_STOPPING || p_person->SubState == SUB_STATE_CLIMB_AROUND_WALL) {
                    if (p_person->Genus.Person->pcom_move_ma.action == MAV_ACTION_CLIMB_OVER) {
                        // Climb over.
                        p_person->SubState = SUB_STATE_CLIMB_UP_WALL;
                    } else {
                        // Shouldn't be on a fence.
                        set_person_drop_down(p_person, 0);
                    }
                }

                break;

            case STATE_GUN:

                if (p_person->SubState == SUB_STATE_AIM_GUN) {
                    // Stopped aiming -- renav.
                    renav = TRUE;
                }

                break;

            case STATE_SHOOT:
            case STATE_DRIVING:
            case STATE_NAVIGATING:
            case STATE_WAIT:
            case STATE_FIGHT:
            case STATE_STAND_UP:
            case STATE_MAVIGATING:
            case STATE_GRAPPLING:
                break;

            case STATE_GOTOING:
                break;

            default:
                ASSERT(0);
                break;
            }

            break;

        case PCOM_MOVE_SUBSTATE_LOSMAV:

        {
            ASSERT(p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_GOTO_THING);

            Thing* p_target = TO_THING(p_person->Genus.Person->pcom_move_arg);

            if (!PCOM_should_i_try_to_los_mav_to_person(p_person, p_target)) {
                // Switch to proper MAV navigation.
                renav = TRUE;
            } else {
                // Track target position.
                p_person->Genus.Person->GotoX = p_target->WorldPos.X >> 8;
                p_person->Genus.Person->GotoZ = p_target->WorldPos.Z >> 8;
            }

            if (p_person->State == STATE_IDLE) {
                // Something happened to this bloke! Get him moving again.
                renav = TRUE;
            }
        }

        break;

        default:
            ASSERT(0);
            break;
        }

        if (renav) {
            if (p_person->SubState == SUB_STATE_PULL_UP || p_person->SubState == SUB_STATE_CLIMB_OFF_LADDER_TOP) {
                // Don't renav while doing this...
            } else {
                // Time for a renavigation.
                PCOM_renav(p_person);
            }
        }

        p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        break;

    case PCOM_MOVE_STATE_PAUSE:
        p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
        break;

    case PCOM_MOVE_STATE_WAIT_CIRCLE:
        if ((p_person->State == STATE_IDLE && p_person->SubState != SUB_STATE_IDLE_CROUTCH_ARREST) || (p_person->StateFn == NULL)) {
            // The animation is over.
            set_person_recircle(p_person);
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_CIRCLE;
        } else if (p_person->State == STATE_FIGHTING && (p_person->SubState == SUB_STATE_GRAPPLE_HOLD || p_person->SubState == SUB_STATE_GRAPPLE_HELD)) {
            // The animation is over.
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;
        }
        break;

    case PCOM_MOVE_STATE_ANIMATION:

        p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        if ((p_person->State == STATE_IDLE && p_person->SubState != SUB_STATE_IDLE_CROUTCH_ARREST) || (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN) || (p_person->State == STATE_MOVEING && p_person->SubState == SUB_STATE_INSIDE_VEHICLE) || (p_person->State == STATE_MOVEING && p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER && p_person->Genus.Person->pcom_move != PCOM_MOVE_DANCE && p_person->Genus.Person->pcom_move != PCOM_MOVE_HANDS_UP && p_person->Genus.Person->pcom_move != PCOM_MOVE_TIED_UP) || (p_person->StateFn == NULL)) {
            // The animation is over.
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
        } else if (p_person->State == STATE_FIGHTING && (p_person->SubState == SUB_STATE_GRAPPLE_HOLD || p_person->SubState == SUB_STATE_GRAPPLE_HELD)) {
            // The animation is over.
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;
        }

        break;

    case PCOM_MOVE_STATE_GRAPPLE:
        if (p_person->SubState == SUB_STATE_GRAPPLE_HOLD) {
            p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

            if (p_person->Genus.Person->pcom_move_counter++ > p_person->Genus.Person->pcom_ai_counter) {
                if (p_person->Genus.Person->pcom_ai == PCOM_AI_COP || p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER) {
                    // Cops throw the grappled person down then arrest.
                    p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_PUNCH;
                } else {
                    p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_KICK;
                }

                p_person->Genus.Person->pcom_move_counter = 0;
                p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
            }
        }
        if (p_person->SubState == SUB_STATE_GRAPPLE_HELD) {
            //				if((GAME_TURN+THING_NUMBER(p_person))&1)
            if ((PTIME(p_person)) & 1) {
                p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_BLOCK;
            }
        }
        if (p_person->State == STATE_IDLE) {
            set_person_recircle(p_person); //, TO_THING(p_person->Genus.Person->pcom_move_arg));
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_CIRCLE;
            p_person->Genus.Person->pcom_move_arg = p_person->Genus.Person->Target;
            p_person->Genus.Person->pcom_move_counter = 0;
        }

        break;
    case PCOM_MOVE_STATE_CIRCLE:

        p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        if (p_person->State == STATE_IDLE || (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN)) {
            // This person has probably been punched and has just recovered.
            set_person_recircle(p_person); //, TO_THING(p_person->Genus.Person->pcom_move_arg));
        } else if ((p_person->SubState == SUB_STATE_GRAPPLE_HELD)) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;

        } else if ((p_person->SubState == SUB_STATE_GRAPPLE_HELD)) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;
        }

        break;

    case PCOM_MOVE_STATE_PARK_CAR:

        ParkCar(p_person);

        break;

    case PCOM_MOVE_STATE_DRIVETO:
    case PCOM_MOVE_STATE_DRIVE_DOWN:
    case PCOM_MOVE_STATE_PARK_CAR_ON_ROAD:

        DriveCar(p_person);

        break;

    default:
        ASSERT(0);
        break;
    }
}
"""

with open('C:/WORK/OpenChaos/new_game/src/new/ai/pcom.cpp', 'a', encoding='utf-8') as f:
    f.write(code)

print("Done. Lines appended:", len(code.splitlines()))
