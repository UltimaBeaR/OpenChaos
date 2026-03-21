// Remaining (not yet migrated) functions from eway.cpp.
// Globals and migrated functions are in new/missions/eway.cpp + eway_globals.cpp.
//
#include "game.h"
#include "cam.h"
#include "eway.h"
#include "bang.h"
#include "ob.h"
#include "fmatrix.h"
#include "trip.h"
#include "dirt.h"
#include "thing.h"
#include "pow.h"
#include "..\headers\music.h"
#include "font2d.h"
#include "..\DDEngine\Headers\console.h"
#include "..\DDEngine\Headers\map.h"
#include "chopper.h"
#include "animal.h"
#include "drive.h"
#include "pcom.h"
#include "supermap.h"
#include "sound.h"
#include "statedef.h"
#include "wmove.h"
#include "aeng.h"
#include "panel.h"
#include "memory.h"
#include "dirt.h"
#include "cnet.h"
#include "mfx.h"
#include "fc.h"
#include "ware.h"
#include "statedef.h"
#include "door.h"
#include "interfac.h"
#include "psystem.h"
#include "poly.h"
#include "playcuts.h"
#include "xlat_str.h"
#include "mist.h"
#include "gamemenu.h"
#include "..\headers\env.h"

extern SLONG person_ok_for_conversation(Thing* p_person);
extern ULONG timer_bored; // I don't care I'm making the game better not the code!


/*
SLONG	global_write=0;
SLONG	global_write1=0;
SLONG	global_write2=0;
SLONG	global_write3=0;
SLONG	global_write4=0;
SLONG	global_write5=0;
SLONG	global_write6=0;
SLONG	global_write7=0;
SLONG	global_write8=0;
SLONG	global_write9=0;
SLONG	global_write10=0;
SLONG	global_write11=0;
SLONG	global_write12=0;
*/

// claude-ai: EWAY_evaluate_condition() — the condition evaluator; the heart of the polling loop.
// claude-ai: Called every tick for every non-dead waypoint by EWAY_process().
// claude-ai: Returns TRUE if the condition is satisfied (fires the action), FALSE otherwise.
// claude-ai: ec->negate inverts the result (logical NOT).
// claude-ai:
// claude-ai: Condition types and their evaluation logic (key ones):
// claude-ai:   EWAY_COND_FALSE/TRUE      — constant false/true
// claude-ai:   EWAY_COND_PROXIMITY       — distance from Darci to waypoint position < ec->arg1 (QDIST3)
// claude-ai:   EWAY_COND_TRIPWIRE        — TRIP_activated(ec->arg1) — tripwire primitive crossed
// claude-ai:   EWAY_COND_SWITCH          — Thing->Flags & FLAGS_SWITCHED_ON for the switch OB
// claude-ai:   EWAY_COND_TIME            — EWAY_time >= ec->arg1 (elapsed mission time)
// claude-ai:   EWAY_COND_DEPENDENT       — EWAY_way[ec->arg1].flag & EWAY_FLAG_ACTIVE (another waypoint fired)
// claude-ai:   EWAY_COND_BOOL_AND/OR     — recursive call on two sub-conditions stored in EWAY_cond[]
// claude-ai:   EWAY_COND_COUNTDOWN       — decrements ec->arg2 by EWAY_tick each frame; fires when zero
// claude-ai:   EWAY_COND_COUNTDOWN_SEE   — same as COUNTDOWN but also draws timer HUD and triggers music
// claude-ai:   EWAY_COND_PERSON_DEAD     — checks Thing->State == STATE_DEAD for the referenced waypoint's NPC
// claude-ai:   EWAY_COND_PERSON_NEAR     — distance check from specific NPC (or any NPC) to waypoint pos
// claude-ai:   EWAY_COND_PLAYER_CUBOID   — player within rectangular XZ bounds (±ec->arg1, ±ec->arg2)
// claude-ai:   EWAY_COND_PLAYER_CUBE     — like CUBOID but also checks Y < ew->y ceiling
// claude-ai:   EWAY_COND_PERSON_ARRESTED — FLAG_PERSON_ARRESTED && SubState == SUB_STATE_DEAD_ARRESTED
// claude-ai:   EWAY_COND_KILLED_NOT_ARRESTED — STATE_DEAD but NOT FLAG_PERSON_ARRESTED (shot, not cuffed)
// claude-ai:   EWAY_COND_A_SEE_B         — line-of-sight check between two referenced NPCs
// claude-ai:   EWAY_COND_GROUP_DEAD      — all EWAY_DO_CREATE_ENEMY waypoints with same colour are dead
// claude-ai:   EWAY_COND_HALF_DEAD       — NPC health in range [10, 100] (damaged but alive)
// claude-ai:   EWAY_COND_ITEM_HELD       — player carrying a specific special item type
// claude-ai:   EWAY_COND_PERSON_USED     — player pressed USE near the NPC (flag on PCOM person)
// claude-ai:   EWAY_COND_RADIUS_USED     — player near waypoint and pressed USE; sets EWAY_magic_radius_flag
// claude-ai:   EWAY_COND_PRIM_DAMAGED    — OB_ob[ec->arg1].flags & OB_FLAG_DAMAGED
// claude-ai:   EWAY_COND_CONVERSE_END    — EWAY_FLAG_FINISHED set on referenced conversation waypoint
// claude-ai:   EWAY_COND_COUNTER_GTEQ    — EWAY_counter[ec->arg1] >= ec->arg2
// claude-ai:   EWAY_COND_CRIME_RATE_GTEQ/LTEQ — CRIME_RATE comparison
// claude-ai:   EWAY_COND_IS_MURDERER     — FLAG2_PERSON_IS_MURDERER on the NPC
// claude-ai:   EWAY_COND_PERSON_IN_VEHICLE — NPC is driving (optionally a specific vehicle)
// claude-ai:   EWAY_COND_THING_RADIUS_DIR — NPC within radius AND facing a specific direction (±128/2048)
// claude-ai:   EWAY_COND_MOVE_RADIUS_DIR  — like THING_RADIUS_DIR; intended for moving direction but uses same facing angle (bug: abs(speed<16) always 0/1)
// claude-ai:   EWAY_COND_SPECIFIC_ITEM_HELD — EWAY_FLAG_GOTITEM on the CREATE_ITEM waypoint
// claude-ai:   EWAY_COND_RANDOM          — fires one random sibling COND_RANDOM, kills all others
// claude-ai:   EWAY_COND_PLAYER_FIRED_GUN — GF_PLAYER_FIRED_GUN flag in GAME_FLAGS
// claude-ai:   EWAY_COND_PRIM_ACTIVATED  — EWAY_FLAG_TRIGGERED set on this waypoint externally
// claude-ai:   EWAY_COND_DARCI_GRABBED   — player in STATE_FIGHTING + SUB_STATE_GRAPPLE_HOLD
// claude-ai:   EWAY_COND_PUNCHED_AND_KICKED — player did ≥2 punches AND ≥2 kicks (tracked via EWAY_darci_move)
// claude-ai:   EWAY_COND_AFTER_FIRST_GAMETURN — GAME_TURN > 0 (avoids frame-0 trigger issues)
// claude-ai:
// claude-ai: Post-evaluation guards (applied before returning):
// claude-ai:   MESSAGE/CONVERSATION waypoints cannot fire on GAME_TURN 0.
// claude-ai:   MESSAGE waypoints delay if another voice is currently playing (unless it's music-only).
// claude-ai:   CAMERA_CREATE waypoints block if camera is active or going inactive.
// claude-ai:   CONVERSATION waypoints require Darci not falling, both NPCs alive and visible to each other.
SLONG EWAY_evaluate_condition(EWAY_Way* ew, EWAY_Cond* ec, SLONG EWAY_sub_condition_of_a_boolean = FALSE)
{
    SLONG ans = FALSE;

    switch (ec->type) {
    case EWAY_COND_FALSE:
        ans = FALSE;
        break;

    case EWAY_COND_TRUE:
        ans = TRUE;
        break;

    case EWAY_COND_PROXIMITY:

    {
        Thing* darci = NET_PERSON(0);

        if (darci) {
            {
                SLONG dx = abs(ew->x - (darci->WorldPos.X >> 8));
                SLONG dy = abs(ew->y - (darci->WorldPos.Y >> 8));
                SLONG dz = abs(ew->z - (darci->WorldPos.Z >> 8));

                SLONG dist = QDIST3(dx, dy, dz);

                ans = (dist < ec->arg1);
            }
        } else {
            ans = FALSE;
        }
    }

    break;

    case EWAY_COND_TRIPWIRE:
        ans = TRIP_activated(ec->arg1);
        break;

    case EWAY_COND_PRESSURE:
        ans = FALSE; // claude-ai: STUB — pressure plates never fire in this codebase (always FALSE)
        break;

    case EWAY_COND_CAMERA:
        ans = FALSE; // claude-ai: STUB — always FALSE here; camera-at condition is satisfied externally via EWAY_FLAG_ACTIVE in EWAY_process_camera()
        break;

    case EWAY_COND_SWITCH:
        ans = (TO_THING(ec->arg1)->Flags & FLAGS_SWITCHED_ON);
        break;

    case EWAY_COND_TIME:
        ans = (EWAY_time >= ec->arg1);
        break;

    case EWAY_COND_DEPENDENT:

        if (ec->arg1 == NULL) {
            sprintf(EWAY_message, "Waypoint %d has a NULL dependency", ew->id);

            CONSOLE_text(EWAY_message, 8000);
            ans = FALSE;

            //
            // Don't print the message again.
            //

            ec->type = EWAY_COND_FALSE;
        } else {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            ans = (EWAY_way[ec->arg1].flag & EWAY_FLAG_ACTIVE);
        }

        break;

    case EWAY_COND_BOOL_AND:

    {
        EWAY_Cond* ec1 = &EWAY_cond[ec->arg1];
        EWAY_Cond* ec2 = &EWAY_cond[ec->arg2];

        ans = EWAY_evaluate_condition(ew, ec1, TRUE) && EWAY_evaluate_condition(ew, ec2, TRUE);
    }

    break;

    case EWAY_COND_BOOL_OR:

    {
        EWAY_Cond* ec1 = &EWAY_cond[ec->arg1];
        EWAY_Cond* ec2 = &EWAY_cond[ec->arg2];

        ans = EWAY_evaluate_condition(ew, ec1, TRUE) || EWAY_evaluate_condition(ew, ec2, TRUE);
    }

    break;

    case EWAY_COND_COUNTDOWN:
        // claude-ai: Fires after a delay. ec->arg1 = waypoint index that must be ACTIVE first (0 = always counting).
        // claude-ai: ec->arg2 starts as the delay in EWAY_tick units (set in .ucm). Decremented each frame.
        // claude-ai: Timers pause while EWAY_conv_active (a conversation is playing).
    case EWAY_COND_COUNTDOWN_SEE:
        // claude-ai: Like COUNTDOWN but also draws a visible HUD countdown timer (PANEL_draw_timer)
        // claude-ai: and switches to MUSIC_MODE_TIMER when under 30 seconds (ec->arg2 < 3000).

        {
            SLONG depend = ec->arg1;
            SLONG timer = ec->arg2;

            ASSERT(depend == 0 || WITHIN(depend, 1, EWAY_way_upto - 1));

            if (!depend || (EWAY_way[depend].flag & EWAY_FLAG_ACTIVE)) {
                if (EWAY_conv_active) {
                    //
                    // Timers stop during conversations.
                    //
                } else {
                    //
                    // Tick down the timer.
                    //
                    if (ec->arg2 <= EWAY_tick) {
                        ec->arg2 = 0;
                        ans = TRUE;
                    } else {
                        ec->arg2 -= EWAY_tick;
                        ans = FALSE;
                    }
                }

                if (ec->type == EWAY_COND_COUNTDOWN_SEE) {
                    if (!(GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON))) {
                        if (ec->arg2 < 3000) // 30 seconds... theoretically
                            //								MUSIC_play(S_TUNE_TIMER1,MUSIC_FLAG_OVERRIDE);
                            music_mode_override = MUSIC_MODE_TIMER;
                        else // maybe we were below 30 seconds but time got added
                        //								if (MUSIC_wave()==S_TUNE_TIMER1) MUSIC_stop(1);
                        {
                            if (music_mode_override) {
                                // this is a little awkward but override must be cleared before the call or it fails
                                music_mode_override = 0;
                                MUSIC_mode(0);
                            } else
                                music_mode_override = 0;
                        }
                    }
                    PANEL_draw_timer(ec->arg2, 320, 50);
                }
            } else {
                //
                // Not counting down.
                //

                ans = FALSE;
            }
        }

        break;

    case EWAY_COND_PERSON_DEAD:

        ans = FALSE; // By default.

        {
            SLONG waypoint;
            SLONG index;

            Thing* p_thing;
            EWAY_Way* ew_dead;

            waypoint = ec->arg1;

            if (waypoint == 0) {
                CBYTE mess[128];

                sprintf(mess, "Waypoint %d: cond person dead has NULL dependency", ew->id);

                CONSOLE_text(mess, 8000);
                ew->flag |= EWAY_FLAG_DEAD;
            } else {
                ew_dead = &EWAY_way[waypoint];

                switch (ew_dead->ed.type) {
                case EWAY_DO_CREATE_PLAYER:
                case EWAY_DO_CREATE_VEHICLE:
                case EWAY_DO_CREATE_ANIMAL:
                case EWAY_DO_CREATE_ENEMY:

                    index = ew_dead->ed.arg1;

                    if (index) {
                        p_thing = TO_THING(index);

                        ans = (p_thing->State == STATE_DEAD);

                        /*
                                                                                        if(ans)
                                                                                        {
                                                                                                printf(" cond deadwp %d\n",ew-EWAY_way);
                                                                                        }
                        */

                        extern BOOL PersonIsMIB(Thing * p_person);

                        if (p_thing->Class == CLASS_PERSON && PersonIsMIB(p_thing)) {
                            if (p_thing->Flags & FLAGS_ON_MAPWHO) {
                                ans = FALSE;
                            }
                        }
                    }

                    break;

                case EWAY_DO_CREATE_ITEM:
                    ans = (ew_dead->flag & EWAY_FLAG_GOTITEM);
                    break;

                case EWAY_DO_CREATE_BARREL:

                {
                    ans = FALSE;

                    if (ew_dead->flag & EWAY_FLAG_ACTIVE) {
                        if (ew_dead->ed.arg1 == NULL) {
                            //
                            // The barrel has been created and destroyed.
                            //

                            ans = TRUE;
                        }
                    }
                }

                break;

                default: {
                    CBYTE mess[128];

                    sprintf(mess, "Waypoint %d: cond person dead dependent on odd waypoint", ew->id);

                    CONSOLE_text(mess, 8000);
                    ew->flag |= EWAY_FLAG_DEAD;
                }

                break;
                }
            }
        }

        break;

    case EWAY_COND_PERSON_NEAR:

    {
        Thing* p_person;

        SLONG dx;
        SLONG dy;
        SLONG dz;
        SLONG dist;
        SLONG waypoint;
        SLONG index;
        SLONG radius;

        waypoint = ec->arg1;
        radius = ec->arg2 * 64; // Radius in quarter map-squares.

        if (waypoint == NULL) {
            //
            // Just check for anybody being near.
            //

            ans = THING_find_nearest(
                ew->x,
                ew->y,
                ew->z,
                radius,
                THING_FIND_PEOPLE);
        } else {
            index = EWAY_get_person(waypoint);

            if (index) {
                p_person = TO_THING(index);

                dx = abs((p_person->WorldPos.X >> 8) - ew->x);
                dy = abs((p_person->WorldPos.Y >> 8) - ew->y);
                dz = abs((p_person->WorldPos.Z >> 8) - ew->z);

                dist = QDIST3(dx, dy, dz);

                ans = dist < radius;
            } else {
                ans = FALSE; // Person hasn't been created yet.
            }
        }
    }

    break;

    case EWAY_COND_CAMERA_AT:

        //
        // This condition is perversely set in EWAY_process_camera().
        //

        ans = FALSE;

        break;

    case EWAY_COND_PLAYER_CUBE:

    {
        Thing* darci = NET_PERSON(0);

        ans = FALSE; // By default...

        if (darci) {
            SLONG dx = ec->arg1;
            SLONG dz = ec->arg2;

            SLONG minx = ew->x - dx;
            SLONG minz = ew->z - dz;
            SLONG maxx = ew->x + dx;
            SLONG maxz = ew->z + dz;
            SLONG maxy = ew->y;

            if (darci->WorldPos.Y < maxy) {
                if (WITHIN(darci->WorldPos.X >> 8, minx, maxx) && WITHIN(darci->WorldPos.Z >> 8, minz, maxz)) {
                    ans = TRUE;
                }
            }
        }
    }

    break;

    case EWAY_COND_A_SEE_B:

        ans = FALSE; // by default...

        {
            SLONG i_a = ec->arg1;
            SLONG i_b = ec->arg2;

            SLONG person_a = EWAY_get_person(i_a);
            SLONG person_b = EWAY_get_person(i_b);

            if (person_a && person_b) {
                if (TO_THING(person_a)->State != STATE_DEAD) {
                    ans = can_a_see_b(
                        TO_THING(person_a),
                        TO_THING(person_b));
                }
            }
        }

        break;

    case EWAY_COND_GROUP_DEAD:

        //
        // Boy, this is slow!
        //

        ans = FALSE;

        {
            SLONG i;
            SLONG found_at_least_one_person;
            SLONG eway_max = EWAY_way_upto;

            EWAY_Way* ew2;

            ew2 = &EWAY_way[0];
            for (i = 0; i < eway_max; i++) {
                //					global_write=i;

                //
                // On Simons, request. This only checks for people in the
                // same colour. It ignores their group.
                //

                if (ew2->colour == ew->colour) {
                    //
                    // This waypoint has our group and colour.
                    //

                    if (ew2->ed.type == EWAY_DO_CREATE_ENEMY) {
                        if (ew2->ed.arg1) {
                            extern SLONG is_person_dead(Thing * p_person);

                            if (!is_person_dead(TO_THING(ew2->ed.arg1))) {
                                //
                                // Found someone alive.
                                //

                                ans = FALSE;

                                break;
                            } else {
                                //
                                // Found at least one dead person- so make the default be TRUE.
                                //

                                ans = TRUE;
                            }
                        }
                    }
                }
                ew2++;
            }
        }

        break;

    case EWAY_COND_HALF_DEAD:

        ans = FALSE;

        {
            SLONG i_person = EWAY_get_person(ec->arg1);

            if (i_person) {
                Thing* p_person = TO_THING(i_person);

                //
                // Is this person half dead?
                //

                if (WITHIN(p_person->Genus.Person->Health, 10, 100)) {
                    // return TRUE;
                    ans = TRUE;
                }
            }
        }

        break;

    case EWAY_COND_ITEM_HELD:

        ans = FALSE;

        //
        // Is the player carrying our item?
        //

        if (ec->arg1 == SPECIAL_GUN) {
            if (NET_PERSON(0)->Flags & FLAGS_HAS_GUN) {
                ans = TRUE;
            }
        } else {
            if (person_has_special(
                    NET_PERSON(0),
                    ec->arg1)) {
                ans = TRUE;
            }
        }

        break;

    case EWAY_COND_PERSON_USED:

        if (ec->arg1) {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            if (EWAY_way[ec->arg1].ed.type == EWAY_DO_CREATE_ENEMY) {
                EWAY_Way* ew2 = &EWAY_way[ec->arg1];

                if (ew2->ed.arg1) {
                    //
                    // Set the PERSON_FLAG_USEABLE so that when the player
                    // presses use near this person the waypoint system
                    // will be alerted.
                    //

                    ASSERT(TO_THING(ew2->ed.arg1)->Class == CLASS_PERSON);

                    TO_THING(ew2->ed.arg1)->Genus.Person->Flags |= FLAG_PERSON_USEABLE;
                }
            }
        }

        ans = FALSE;

        if (ew->es.type == EWAY_STAY_ALWAYS) {
            ans = !!(ew->flag & EWAY_FLAG_ACTIVE);
        }

        break;

    case EWAY_COND_RADIUS_USED:

    {
        Thing* darci = NET_PERSON(0);

        if (darci) {
            SLONG dx = abs(ew->x - (darci->WorldPos.X >> 8));
            SLONG dy = abs(ew->y - (darci->WorldPos.Y >> 8));
            SLONG dz = abs(ew->z - (darci->WorldPos.Z >> 8));

            SLONG dist = QDIST3(dx, dy, dz);

            if (dist < ec->arg1)
                EWAY_magic_radius_flag = ew;
        }

        //
        // Always return FALSE, this waypoint will be set active in
        // do_an_action() in interfac.cpp.
        //

        ans = FALSE;
    }

    break;

    case EWAY_COND_PRIM_DAMAGED:

        ans = FALSE;

        if (ec->arg1) {
            ASSERT(WITHIN(ec->arg1, 1, OB_ob_upto - 1));

            if (OB_ob[ec->arg1].flags & OB_FLAG_DAMAGED) {
                ans = TRUE;
            }
        }

        break;

    case EWAY_COND_CONVERSE_END:

        ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

        ans = FALSE;

        if (EWAY_way[ec->arg1].flag & EWAY_FLAG_FINISHED) {
            ans = TRUE;
        }

        break;

    case EWAY_COND_COUNTER_GTEQ:

        ASSERT(WITHIN(ec->arg1, 0, EWAY_MAX_COUNTERS - 1));

        ans = FALSE;

        if (EWAY_counter[ec->arg1] >= ec->arg2) {
            ans = TRUE;
        }

        break;

    case EWAY_COND_PERSON_ARRESTED:
        // claude-ai: True when the NPC has been handcuffed by Darci (player presses arrest button while grappling).
        // claude-ai: Requires FLAG_PERSON_ARRESTED AND SubState == SUB_STATE_DEAD_ARRESTED (both must be set).
        // claude-ai: FLAG_PERSON_ARRESTED is set in pcom.cpp when TT_PERSON_ARRESTED fires (Darci cuffs someone).
        // claude-ai: This is one of the core mission success conditions for arrest-based missions.

        ans = FALSE; // By default.

        if (ec->arg1 == 0) {
            //
            // Hmm... person waypoint not set.
            //

            ew->flag |= EWAY_FLAG_DEAD;
        } else {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));
            ASSERT(
                EWAY_way[ec->arg1].ed.type == EWAY_DO_CREATE_ENEMY || EWAY_way[ec->arg1].ed.type == EWAY_DO_CREATE_PLAYER);

            UWORD index = EWAY_way[ec->arg1].ed.arg1;

            if (index) {
                Thing* p_person = TO_THING(index);

                ASSERT(p_person->Class == CLASS_PERSON);

                ans = !!(p_person->Genus.Person->Flags & FLAG_PERSON_ARRESTED) && (p_person->SubState == SUB_STATE_DEAD_ARRESTED);
            }
        }

        break;

    case EWAY_COND_PLAYER_CUBOID:
        extern UBYTE is_semtex;
        if ((ew - EWAY_way) == 124 && is_semtex) // miked remove wetback part for PC/Dreamcast
            ans = FALSE;
        else

        {
            SLONG x1;
            SLONG z1;

            SLONG x2;
            SLONG z2;

            SLONG dx = ec->arg1;
            SLONG dz = ec->arg2;

            Thing* darci = NET_PERSON(0);

            {
                x1 = ew->x - dx;
                x2 = ew->x + dx;

                z1 = ew->z - dz;
                z2 = ew->z + dz;

                if (WITHIN(darci->WorldPos.X >> 8, x1, x2) && WITHIN(darci->WorldPos.Z >> 8, z1, z2)) {
                    //						ASSERT(0);
                    ans = TRUE;
                } else {
                    ans = FALSE;
                }
            }
        }

        break;

    case EWAY_COND_KILLED_NOT_ARRESTED:

        ans = FALSE; // By default.

        {
            SLONG waypoint;
            SLONG index;

            Thing* p_thing;
            EWAY_Way* ew_dead;

            waypoint = ec->arg1;

            if (waypoint == 0) {
                CBYTE mess[128];

                sprintf(mess, "Waypoint %d: cond person dead has NULL dependency", ew->id);

                CONSOLE_text(mess, 8000);
                ew->flag |= EWAY_FLAG_DEAD;
            } else {
                ew_dead = &EWAY_way[waypoint];

                switch (ew_dead->ed.type) {
                case EWAY_DO_CREATE_PLAYER:
                case EWAY_DO_CREATE_ENEMY:

                    index = ew_dead->ed.arg1;

                    if (index) {
                        p_thing = TO_THING(index);

                        ASSERT(p_thing->Class == CLASS_PERSON);

                        if (p_thing->State == STATE_DEAD) {
                            //
                            // Not if they're arrested.
                            //

                            if (!(p_thing->Genus.Person->Flags & FLAG_PERSON_ARRESTED)) {
                                ans = TRUE;
                            }
                        }
                    }

                    break;

                default:

                {
                    CBYTE mess[128];

                    sprintf(mess, "Waypoint %d: killed not arrested dependent on odd waypoint", ew->id);

                    CONSOLE_text(mess, 8000);

                    ew->flag |= EWAY_FLAG_DEAD;
                }

                break;
                }
            }
        }

        break;

    case EWAY_COND_CRIME_RATE_GTEQ:
        ans = (CRIME_RATE >= ec->arg1);
        break;
    case EWAY_COND_CRIME_RATE_LTEQ:
        ans = (CRIME_RATE <= ec->arg1);
        break;

    case EWAY_COND_IS_MURDERER:

    {
        ans = FALSE;

        UWORD person = EWAY_get_person(ec->arg1);

        if (person) {
            Thing* p_person = TO_THING(person);

            if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_IS_MURDERER) {
                ans = TRUE;
            }
        }
    }

    break;

    case EWAY_COND_PERSON_IN_VEHICLE:

        ans = FALSE;

        {
            UWORD person;
            UWORD vehicle;

            person = EWAY_get_person(ec->arg1);

            if (person) {
                Thing* p_person = TO_THING(person);

                ASSERT(p_person->Class == CLASS_PERSON);

                if (p_person->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING)) {
                    if (ec->arg2) {
                        //
                        // They must be driving a specific vehicle!
                        //

                        ASSERT(WITHIN(ec->arg2, 1, EWAY_way_upto - 1));
                        ASSERT(EWAY_way[ec->arg2].ed.type == EWAY_DO_CREATE_VEHICLE);

                        if (p_person->Genus.Person->InCar == EWAY_way[ec->arg2].ed.arg1) {
                            ans = TRUE;
                        }
                    } else {
                        ans = TRUE;
                    }
                }
            }
        }

        break;

    case EWAY_COND_THING_RADIUS_DIR:
    case EWAY_COND_MOVE_RADIUS_DIR:

        ans = FALSE;

        {
            UWORD thing = EWAY_get_person(ec->arg1);

            if (thing == NULL) {
            } else {
                Thing* p_thing = TO_THING(thing);
                SLONG radius = ec->arg2 * 64; // Radius in quarter map-squares.

                SLONG dx = abs((p_thing->WorldPos.X >> 8) - ew->x);
                SLONG dy = abs((p_thing->WorldPos.Y >> 8) - ew->y);
                SLONG dz = abs((p_thing->WorldPos.Z >> 8) - ew->z);

                SLONG dist = QDIST3(dx, dy, dz);

                if (dist < radius) {
                    SLONG dangle;
                    SLONG angle;
                    SLONG speed;

                    //
                    // Do the angles match up?
                    //

                    switch (p_thing->Class) {
                    case CLASS_PERSON:
                        angle = p_thing->Draw.Tweened->Angle;
                        speed = p_thing->Velocity;
                        break;

                    case CLASS_VEHICLE:
                        angle = p_thing->Genus.Vehicle->Angle;
                        speed = p_thing->Velocity;
                        break;
                    default:
                        ASSERT(0);
                        break;
                    }

                    if (ec->type == EWAY_COND_MOVE_RADIUS_DIR || abs(speed < 16)) {
                        dangle = angle - (ew->yaw << 3);

                        if (dangle > +1024) {
                            dangle -= 2048;
                        }
                        if (dangle < -1024) {
                            dangle += 2048;
                        }

                        if (WITHIN(dangle, -128, +128)) {
                            //
                            // Angle near enough too...
                            //

                            ans = TRUE;
                        }
                    }
                }
            }
        }

        break;

    case EWAY_COND_SPECIFIC_ITEM_HELD:

        ans = FALSE;

        {
            EWAY_Way* ew_other;

            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            ew_other = &EWAY_way[ec->arg1];

            ASSERT(ew_other->ed.type == EWAY_DO_CREATE_ITEM);

            if (ew_other->ed.arg2) {
                //
                // Has this special been picked up by Darci?
                //

                if (ew_other->flag & EWAY_FLAG_GOTITEM) {
                    ans = TRUE;
                }
            }
        }

        break;

    case EWAY_COND_RANDOM:

    {
        ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

        if (EWAY_way[ec->arg1].flag & EWAY_FLAG_ACTIVE) {
            //
            // Look for all other EWAY_COND_RANDOM waypoints that are dependent
            // on the same waypoint as us.
            //

#define EWAY_MAX_SAME 8

            SLONG i;
            UWORD same[EWAY_MAX_SAME];
            SLONG same_upto = 0;
            SLONG which;

            for (i = 1; i < EWAY_way_upto; i++) {
                //					global_write1=i;
                if (EWAY_way[i].ec.type == EWAY_COND_RANDOM && EWAY_way[i].ec.arg1 == ec->arg1) {
                    ASSERT(WITHIN(same_upto, 0, EWAY_MAX_SAME - 1));

                    same[same_upto++] = i;
                }
            }

            which = Random() % (same_upto + 1);

            if (which == same_upto) {
                //
                // None of the Random waypoint go active.
                //
            } else {
                //
                // Set active the Random waypoint that we've chosen.
                //

                void EWAY_set_active(EWAY_Way * ew);

                EWAY_set_active(&EWAY_way[same[which]]);
            }

            //
            // Kill all the random waypoints.
            //

            for (i = 0; i < same_upto; i++) {
                EWAY_way[same[i]].flag |= EWAY_FLAG_DEAD;
            }
        }
    }

        ans = FALSE;

        break;

    case EWAY_COND_PLAYER_FIRED_GUN:
        ans = !!(GAME_FLAGS & GF_PLAYER_FIRED_GUN);
        break;

    case EWAY_COND_PRIM_ACTIVATED:
        ans = !!(ew->flag & EWAY_FLAG_TRIGGERED);
        break;

    case EWAY_COND_DARCI_GRABBED:
        ans = (NET_PERSON(0)->State == STATE_FIGHTING && NET_PERSON(0)->SubState == SUB_STATE_GRAPPLE_HOLD);
        break;

    case EWAY_COND_PUNCHED_AND_KICKED:

        ans = FALSE;

        if (ec->arg1 == NULL) {
        } else {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            if (EWAY_way[ec->arg1].flag & EWAY_FLAG_ACTIVE) {
                if (ew->flag & EWAY_FLAG_CLEARED) {
                    if (EWAY_darci_move & EWAY_DARCI_MOVE_PUNCH) {
                        ec->arg2 += 1;

                        EWAY_darci_move &= ~EWAY_DARCI_MOVE_PUNCH;
                    }

                    if (EWAY_darci_move & EWAY_DARCI_MOVE_KICK) {
                        ec->arg2 += 0x100;

                        EWAY_darci_move &= ~EWAY_DARCI_MOVE_KICK;
                    }

                    if ((ec->arg2 & 0xff) >= 2 && (ec->arg2 >> 8) >= 2) {
                        ans = TRUE;
                    }
                } else {
                    //
                    // The first process where this waypoint has gone active.
                    //

                    EWAY_darci_move = 0;

                    ew->flag |= EWAY_FLAG_CLEARED;
                }
            }
        }

        break;

    case EWAY_COND_AFTER_FIRST_GAMETURN:
        ans = (GAME_TURN > 0);
        break;

    default:
        ASSERT(0);
        break;
    }

    if (ec->negate) {
        ans = !ans;
    }

    //
    // Skip all this falsifying code if we don't have
    // the final answer it.
    //

    if (EWAY_sub_condition_of_a_boolean) {

        return ans;
    }

    if (ew->ed.type == EWAY_DO_CONVERSATION || ew->ed.type == EWAY_DO_AMBIENT_CONV || ew->ed.type == EWAY_DO_MESSAGE) {
        if (GAME_TURN < 1) {
            ans = FALSE;
        }
    }

    if (ew->ed.type == EWAY_DO_MESSAGE && ans && !(ew->flag & EWAY_FLAG_ACTIVE)) {
        //
        // If there is a voice playing, then delay the message triggering.
        //

        if (!WITHIN(ew->ed.arg1, 0, EWAY_MAX_MESSES - 1)) {
            //
            // Bad message...
            //
        } else {
            if (EWAY_used_thing) {
                if (MFX_QUICK_still_playing()) {
                    if (MUSIC_is_playing()) {
                        //
                        // Just stop the music...
                        //

                        MFX_QUICK_stop();
                        MFX_QUICK_wait();
                    } else {
                        ans = FALSE;
                    }

                    /*

                    if (ew->es.type == EWAY_STAY_ALWAYS ||
                            ew->es.type == EWAY_STAY_DIE)
                    {
                            ew->ec.type = EWAY_COND_TRUE;
                    }

                    */
                }
            } else {
                if (ew->ed.arg2 == EWAY_MESSAGE_WHO_STREETNAME) {
                } else if (ew->ed.arg2 == EWAY_MESSAGE_WHO_TUTORIAL) {
                } else {
                    Thing* who_says = NULL;

                    if (ew->ed.arg2) {
                        SLONG who = EWAY_get_person(ew->ed.arg2);

                        if (who) {
                            who_says = TO_THING(who);
                        }
                    }

                    if (who_says && who_says->State == STATE_DEAD) {
                        //
                        // Dead people can't talk.
                        //
                    } else {
                        if (MFX_QUICK_still_playing()) {
                            if (MUSIC_is_playing()) {
                                //
                                // Just stop the music...
                                //

                                MFX_QUICK_stop();
                                MFX_QUICK_wait();
                            } else {
                                ans = FALSE;
                            }

                            /*

                            if (ew->es.type == EWAY_STAY_ALWAYS ||
                                    ew->es.type == EWAY_STAY_DIE)
                            {
                                    ew->ec.type = EWAY_COND_TRUE;
                            }

                            */
                        }
                    }
                }
            }
        }
    }

    if (ew->ed.type == EWAY_DO_CAMERA_CREATE && ans) {
        if (EWAY_stop_player_moving() && !EWAY_cam_goinactive) {
            ans = FALSE;
        }
    }

    if ((ew->ed.type == EWAY_DO_CONVERSATION || ew->ed.type == EWAY_DO_AMBIENT_CONV) && ans) {
        Thing* darci = NET_PERSON(0);

        //
        // Make sure that Darci isn't jumping or falling through the air!
        //

        if (!person_ok_for_conversation(darci)) {
            ans = FALSE;
        }

        //
        // Make sure each person in the conversation is okay...
        //

        UWORD person1 = EWAY_get_person(ew->ed.arg1);
        UWORD person2 = EWAY_get_person(ew->ed.arg2);

        if (person1 == NULL || person2 == NULL) {
            ans = FALSE;
        } else {
            Thing* p_person1 = TO_THING(person1);
            Thing* p_person2 = TO_THING(person2);

            if (!person_ok_for_conversation(p_person1) || !person_ok_for_conversation(p_person2)) {
                ans = FALSE;
            } else {
                //
                // If there isn't a line of sight between the two people...
                //

                SLONG x1 = (p_person1->WorldPos.X >> 8);
                SLONG y1 = (p_person1->WorldPos.Y >> 8) + 0xa0;
                SLONG z1 = (p_person1->WorldPos.Z >> 8);

                SLONG x2 = (p_person2->WorldPos.X >> 8);
                SLONG y2 = (p_person2->WorldPos.Y >> 8) + 0xa0;
                SLONG z2 = (p_person2->WorldPos.Z >> 8);

                if (!there_is_a_los(
                        x1, y1, z1,
                        x2, y2, z2,
                        LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                    ans = FALSE;
                }
            }
        }
    }

    if (ans) {
        // Bloody annoying on DC, and slows everything to a crawl.
        //		TRACE("EVAL TRUE type %d way %d \n",ec->type,ew-EWAY_way);
    }

    return ans;
}

// claude-ai: EWAY_create_camera() — initialises the scripted cut-scene camera from a CAMERA_CREATE waypoint.
// claude-ai: Sets EWAY_cam_active=TRUE and populates all EWAY_cam_* globals.
// claude-ai: The camera starts at the waypoint position and looks for the first CAMERA_TARGET waypoint
// claude-ai: with the same colour/group (via EWAY_find_waypoint). The camera then moves through
// claude-ai: subsequent CAMERA_WAYPOINT entries each tick (processed in EWAY_process_camera()).
// claude-ai: EWAY_SUBTYPE_CAMERA_LOCK_PLAYER → EWAY_cam_freeze=TRUE → player input disabled during cut-scene.
// claude-ai: EWAY_cam_skip=100 → cannot skip for 1 second after camera starts.
//
// Sets off a new cut-scene camera starting from the given waypoint.
//

void EWAY_create_camera(SLONG waypoint)
{
    SLONG i;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    EWAY_Way* ew;
    EWAY_Way* ew_target;

    ew = &EWAY_way[waypoint];

    EWAY_cam_active = TRUE;
    EWAY_cam_goinactive = FALSE;
    EWAY_cam_x = ew->x << 8;
    EWAY_cam_y = ew->y << 8;
    EWAY_cam_z = ew->z << 8;
    EWAY_cam_dx = 0;
    EWAY_cam_dy = 0;
    EWAY_cam_dz = 0;
    EWAY_cam_yaw = 0;
    EWAY_cam_pitch = 0;
    EWAY_cam_speed = ew->ed.arg1;
    EWAY_cam_delay = ew->ed.arg2 * 10;
    EWAY_cam_waypoint = waypoint;
    EWAY_cam_freeze = !!(ew->ed.subtype & EWAY_SUBTYPE_CAMERA_LOCK_PLAYER);
    EWAY_cam_thing = NULL;
    EWAY_cam_lens = 0x28000;
    EWAY_cam_lock = !!(ew->ed.subtype & EWAY_SUBTYPE_CAMERA_LOCK_DIRECTION);
    EWAY_cam_cant_interrupt = !!(ew->ed.subtype & EWAY_SUBTYPE_CAMERA_CANT_INTERRUPT);

    // This is benign
    ASSERT(EWAY_cam_cant_interrupt == 0);
    EWAY_cam_last_yaw = ew->yaw << 11;
    EWAY_cam_last_x = ew->x << 8;
    EWAY_cam_last_y = ew->y << 8;
    EWAY_cam_last_z = ew->z << 8;
    EWAY_cam_skip = 100; // Can't skip for 1 second.
    EWAY_cam_last_dyaw = 0;
    EWAY_conv_ambient = 0;

    //
    // Find the lowest numbered camera-target waypoint.
    //

    EWAY_cam_target = EWAY_find_waypoint(1, EWAY_DO_CAMERA_TARGET, ew->colour, ew->group, FALSE);

    if (EWAY_cam_target == EWAY_NO_MATCH) {
        //
        // There is nothing for the camera to look at!
        //

        CONSOLE_text("No target for camera", 8000);

        EWAY_cam_active = FALSE;
    }

    MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SEARCH_END);
    MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SLIDE_START);
}

// claude-ai: EWAY_process_camera() — advances the scripted cut-scene camera each tick.
// claude-ai: Reads EWAY_cam_* globals set by EWAY_create_camera() and moves the camera through
// claude-ai: CAMERA_WAYPOINT nodes (waypoints with same colour/group as the CAMERA_CREATE waypoint).
// claude-ai: Camera looks at a CAMERA_TARGET waypoint — can be a fixed position or track a Thing.
// claude-ai: When EWAY_cam_goinactive counts down to 0, sets EWAY_cam_active=FALSE (end of cut-scene).
// claude-ai: Sets EWAY_FLAG_ACTIVE on CAMERA_AT conditions when the camera reaches each node.
// claude-ai: EWAY_cam_freeze=TRUE → player movement is locked while camera is running.
// claude-ai: Outputs final camera position/angles to FC_cam[] for the rendering system.
void EWAY_process_camera(void)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;
    SLONG next;
    SLONG speed;
    SLONG wspeed;

    SLONG target_stationary = FALSE;

    SLONG look_x;
    SLONG look_y;
    SLONG look_z;
    SLONG look_yaw = 0;

    EWAY_Way* ew_go;
    EWAY_Way* ew_look;
    EWAY_Way* ew_next;

    EWAY_cam_warehouse = NULL;

    if (!EWAY_cam_active) {
        return;
    }

    if (EWAY_cam_goinactive) {
        EWAY_cam_goinactive--;
        if (EWAY_cam_goinactive == 0) {
            EWAY_cam_jumped = 10;
            EWAY_cam_active = FALSE;
            EWAY_cam_goinactive = FALSE;
        }
        return;
    }
    timer_bored = 0;

    EWAY_cam_skip -= EWAY_tick;

    //
    // Work out what we should look at.
    //

    if (EWAY_cam_thing) {
        Thing* p_thing = TO_THING(EWAY_cam_thing);

        look_x = (p_thing->WorldPos.X >> 8);
        look_y = (p_thing->WorldPos.Y >> 8) + 0xa0;
        look_z = (p_thing->WorldPos.Z >> 8);

        if (p_thing->Class == CLASS_PERSON) {
            EWAY_cam_warehouse = p_thing->Genus.Person->Ware;

            if (p_thing->State == STATE_IDLE) {
                target_stationary = TRUE;
            }
            if (p_thing->State == STATE_MOVEING) {
                if (p_thing->SubState == SUB_STATE_SIMPLE_ANIM || p_thing->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                {
                    target_stationary = TRUE;
                }
            }
        }
    } else if (!EWAY_cam_target) {
        // No waypoint _or_ thing -- so we must be forcing raw coordinates
        look_x = EWAY_cam_targx >> 8;
        look_y = EWAY_cam_targy >> 8;
        look_z = EWAY_cam_targz >> 8;
    } else {
        //
        // The waypoint  we are looking at.
        //

        ASSERT(WITHIN(EWAY_cam_target, 1, EWAY_way_upto - 1));

        ew_look = &EWAY_way[EWAY_cam_target];

        //
        // Just look at the waypoint by default...
        //

        look_x = ew_look->x;
        look_y = ew_look->y;
        look_z = ew_look->z;

        EWAY_cam_warehouse = ew_look->ware;

        switch (ew_look->ed.subtype) {
        case EWAY_SUBTYPE_CAMERA_TARGET_PLACE:
            target_stationary = TRUE;
            break;

        case EWAY_SUBTYPE_CAMERA_TARGET_THING:

            if (ew_look->ed.arg1) {
                ASSERT(WITHIN(ew_look->ed.arg1, 1, EWAY_way_upto - 1));

                EWAY_Way* ew_thing;

                ew_thing = &EWAY_way[ew_look->ed.arg1];

                //
                // Has this waypoint created a thing yet?
                //

                if (ew_thing->ed.arg1) {
                    //
                    // Yes! Look at the thing.
                    //

                    Thing* p_thing = TO_THING(ew_thing->ed.arg1);

                    look_x = (p_thing->WorldPos.X >> 8);
                    look_y = (p_thing->WorldPos.Y >> 8) + 0xa0;
                    look_z = (p_thing->WorldPos.Z >> 8);

                    if (p_thing->Class == CLASS_PERSON) {
                        EWAY_cam_warehouse = p_thing->Genus.Person->Ware;

                        if (p_thing->State == STATE_IDLE) {
                            target_stationary = TRUE;
                        }
                        if (p_thing->State == STATE_MOVEING) {
                            if (p_thing->SubState == SUB_STATE_SIMPLE_ANIM || p_thing->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                            {
                                target_stationary = TRUE;
                            }
                        }
                    }
                }
            }

            break;

        case EWAY_SUBTYPE_CAMERA_TARGET_NEAR:

            //
            // Search around for the nearest living thing.
            //

            {
                UWORD look_index = THING_find_nearest(
                    ew_look->x,
                    ew_look->y,
                    ew_look->z,
                    0x500,
                    THING_FIND_LIVING);

                if (look_index) {
                    Thing* p_look = TO_THING(look_index);

                    look_x = p_look->WorldPos.X >> 8;
                    look_y = p_look->WorldPos.Y >> 8;
                    look_z = p_look->WorldPos.Z >> 8;

                    if (p_look->Class == CLASS_PERSON) {
                        EWAY_cam_warehouse = p_look->Genus.Person->Ware;

                        if (p_look->State == STATE_IDLE) {
                            target_stationary = TRUE;
                        }
                        if (p_look->State == STATE_MOVEING) {
                            if (p_look->SubState == SUB_STATE_SIMPLE_ANIM || p_look->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                            {
                                target_stationary = TRUE;
                            }
                        }
                    }
                } else {
                    //
                    // If there is nothing to look at nearby- then just look
                    // at the waypoint itself.
                    //

                    look_x = ew_look->x;
                    look_y = ew_look->y;
                    look_z = ew_look->z;
                    look_yaw = ew_look->yaw << 11;
                    target_stationary = TRUE;
                }
            }

            break;

        default:
            ASSERT(0);
            break;
        }
    }

    if (EWAY_cam_waypoint == NULL) {
        //
        // No camera movement required.
        //
    } else {
        ASSERT(WITHIN(EWAY_cam_waypoint, 1, EWAY_way_upto - 1));

        ew_go = &EWAY_way[EWAY_cam_waypoint];

        //
        // The direction locked cameras look in.
        //

        look_yaw = ew_go->yaw << 11;

        if (EWAY_stop_player_moving()) {
            if (!EWAY_cam_cant_interrupt && EWAY_cam_skip <= 0) {
                if (!EWAY_conv_ambient)
                    if (NET_PLAYER(0)->Genus.Player->Pressed & (INPUT_MASK_JUMP | INPUT_MASK_KICK | INPUT_MASK_ACTION | INPUT_MASK_PUNCH)) {
                        MFX_QUICK_stop();
                        //
                        // Mark all waypoints that this camera _would_ go to as arrived at.
                        //

                        while (1) {
                            //
                            // Mark this waypoint as active... if it is a CAMERA_WAYPOINT.
                            //

                            if (ew_go->ec.type == EWAY_COND_CAMERA_AT) {
                                ew_go->flag |= EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD;
                            }

                            next = EWAY_find_waypoint(EWAY_cam_waypoint + 1, EWAY_DO_CAMERA_WAYPOINT, ew_go->colour, ew_go->group, FALSE);

                            if (next == EWAY_NO_MATCH || next <= EWAY_cam_waypoint) {
                                //
                                // The search wrapped... there are no more waypoints for
                                // the camera to use. Go inactive NEXT GAME TURN!
                                //

                                // if (EWAY_cam_goinactive==0)
                                EWAY_cam_goinactive = 2; // TRUE;

                                return;
                            }

                            EWAY_cam_waypoint = next;

                            ASSERT(WITHIN(EWAY_cam_waypoint, 1, EWAY_way_upto - 1));

                            ew_go = &EWAY_way[EWAY_cam_waypoint];
                        }
                    }
            } else {
                //
                // Pressing the key twice skips!
                //

                if (!EWAY_conv_ambient)
                    if (NET_PLAYER(0)->Genus.Player->Pressed & (INPUT_MASK_JUMP | INPUT_MASK_KICK | INPUT_MASK_ACTION | INPUT_MASK_PUNCH)) {
                        EWAY_cam_skip = 0;
                    }
            }
        }
        //
        // How far to our next waypoint?
        //

        dx = ew_go->x - (EWAY_cam_x >> 8);
        dy = ew_go->y - (EWAY_cam_y >> 8);
        dz = ew_go->z - (EWAY_cam_z >> 8);

        dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

        if (dist < 0x80) {
            EWAY_cam_last_x = EWAY_cam_x;
            EWAY_cam_last_y = EWAY_cam_y;
            EWAY_cam_last_z = EWAY_cam_z;
            EWAY_cam_last_yaw = EWAY_cam_yaw;

            if (EWAY_cam_delay == 10 * 100) {
                //
                // Maximum delay really means that it stays here while
                // the waypoint is active.
                //

                if (ew_go->flag & EWAY_FLAG_ACTIVE) {
                    //
                    // Just stay here...
                    //
                } else {
                    //
                    // Move onto the next waypoint.
                    //

                    EWAY_cam_delay = 0;
                }
            } else {
                //
                // Count down the delay.
                //

                EWAY_cam_delay -= EWAY_tick;

                if (EWAY_cam_delay < 0) {
                    //
                    // Mark this waypoint as active... if it is a CAMERA_WAYPOINT.
                    //

                    if (ew_go->ec.type == EWAY_COND_CAMERA_AT) {
                        ew_go->flag |= EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD;
                    }

                    //
                    // Move onto the next waypoint.
                    //

                    next = EWAY_find_waypoint(EWAY_cam_waypoint + 1, EWAY_DO_CAMERA_WAYPOINT, ew_go->colour, ew_go->group, FALSE);

                    if (next == EWAY_NO_MATCH || next <= EWAY_cam_waypoint) {
                        //
                        // The search wrapped... there are no more waypoints for
                        // the camera to use. Go inactive NEXT GAME TURN!
                        //

                        // if (EWAY_cam_goinactive==0)
                        EWAY_cam_goinactive = 2; // TRUE;

                        //
                        // Make the camera look at Darci properly.
                        //
                        return;
                    }

                    ASSERT(WITHIN(next, 1, EWAY_way_upto - 1));

                    ew_next = &EWAY_way[next];

                    EWAY_cam_speed = ew_next->ed.arg1 << 8;
                    EWAY_cam_delay = ew_next->ed.arg2 * 10;

                    EWAY_cam_waypoint = next;
                }
            }
        }

        //
        // Accelerate towards our next waypoint.
        //

#define EWAY_CAM_ACCEL_SPEED (EWAY_cam_speed >> 3)

        dx = (dx * EWAY_CAM_ACCEL_SPEED) / dist;
        dy = (dy * EWAY_CAM_ACCEL_SPEED) / dist;
        dz = (dz * EWAY_CAM_ACCEL_SPEED) / dist;

        EWAY_cam_dx += dx * TICK_RATIO >> TICK_SHIFT;
        EWAY_cam_dy += dy * TICK_RATIO >> TICK_SHIFT;
        EWAY_cam_dz += dz * TICK_RATIO >> TICK_SHIFT;

        //
        // Keep our speed at what we want.
        //

        speed = QDIST3(abs(EWAY_cam_dx), abs(EWAY_cam_dy), abs(EWAY_cam_dz)) + 1;

        if (dist >= 0x80) {
            wspeed = EWAY_cam_speed;
        } else {
            wspeed = (dist * EWAY_cam_speed) / 0x80;
        }

        if (speed > wspeed) {
            EWAY_cam_dx = (EWAY_cam_dx * wspeed) / speed;
            EWAY_cam_dy = (EWAY_cam_dy * wspeed) / speed;
            EWAY_cam_dz = (EWAY_cam_dz * wspeed) / speed;
        }

        //
        // Move the camera.
        //

        EWAY_cam_x += EWAY_cam_dx * TICK_RATIO >> TICK_SHIFT;
        EWAY_cam_y += EWAY_cam_dy * TICK_RATIO >> TICK_SHIFT;
        EWAY_cam_z += EWAY_cam_dz * TICK_RATIO >> TICK_SHIFT;
    }

    //
    // Look at our target.
    //

    dx = look_x - (EWAY_cam_x >> 8) >> 1;
    dy = look_y - (EWAY_cam_y >> 8) >> 1;
    dz = look_z - (EWAY_cam_z >> 8) >> 1;

    SLONG dxz = QDIST2(abs(dx), abs(dz));

    //
    // Look at the right part of the thing.
    //

    EWAY_cam_want_yaw = -Arctan(dx, dz);
    EWAY_cam_want_pitch = Arctan(dy, dxz);

    EWAY_cam_want_yaw &= 2047;
    EWAY_cam_want_pitch &= 2047;

    EWAY_cam_want_yaw <<= 8;
    EWAY_cam_want_pitch <<= 8;

    SLONG dyaw = EWAY_cam_want_yaw - EWAY_cam_yaw;
    SLONG dpitch = EWAY_cam_want_pitch - EWAY_cam_pitch;

    dyaw &= (2048 << 8) - 1;
    dpitch &= (2048 << 8) - 1;

    if (dyaw > (1024 << 8)) {
        dyaw -= (2048 << 8);
    }
    if (dpitch > (1024 << 8)) {
        dpitch -= (2048 << 8);
    }

    if (EWAY_cam_lock) {
        if (EWAY_cam_lock != INFINITY) {
            EWAY_cam_pitch = EWAY_cam_want_pitch;

            EWAY_cam_lock = INFINITY;
        }

        dyaw = look_yaw - EWAY_cam_yaw;
        dyaw &= (2048 << 8) - 1;

        if (dyaw > (1024 << 8)) {
            dyaw -= 2048 << 8;
        }

        dyaw = (EWAY_cam_last_dyaw * 3) + dyaw >> 2;

        EWAY_cam_yaw += dyaw >> 5;
        EWAY_cam_last_dyaw = dyaw;

    } else {
        SLONG max_dangle = 200 << 8;

        if (target_stationary) {
            max_dangle = 100 << 8;
        }

        if (abs(dyaw) > max_dangle || abs(dpitch) > max_dangle) {
            /*
                                    if(dyaw>0)
                                    {
                                            EWAY_cam_yaw   += MAX(dyaw   >> 2,dyaw-max_dangle);
                                    }
                                    else
                                    {
                                            EWAY_cam_yaw   -= MAX(-dyaw   >> 2,-dyaw-max_dangle);
                                    }
                                    if(dpitch>0)
                                    {
                                            EWAY_cam_pitch   += MAX(dpitch   >> 2,dpitch-max_dangle);
                                    }
                                    else
                                    {
                                            EWAY_cam_pitch   -= MAX(-dpitch   >> 2,-dpitch-max_dangle);
                                    }
            */

            EWAY_cam_yaw = EWAY_cam_want_yaw;
            EWAY_cam_pitch = EWAY_cam_want_pitch;
            //			EWAY_cam_yaw   += dyaw   >> 1;
            //			EWAY_cam_pitch += dpitch >> 1;
        }

        else

        {
            EWAY_cam_yaw += dyaw >> 1;
            EWAY_cam_pitch += dpitch >> 1;
        }
    }
}

//
// Finishes a conversation.
//

void EWAY_finish_conversation(void)
{
    EWAY_conv_active = FALSE;

    if (!EWAY_conv_ambient) {
        EWAY_cam_relinquish();
    }

    ASSERT(WITHIN(EWAY_conv_waypoint, 1, EWAY_way_upto - 1));

    EWAY_way[EWAY_conv_waypoint].flag |= EWAY_FLAG_FINISHED;

    PCOM_stop_people_talking_to_eachother(
        TO_THING(EWAY_conv_person_a),
        TO_THING(EWAY_conv_person_b));

    extern THING_INDEX PANEL_wide_top_person;
    extern THING_INDEX PANEL_wide_bot_person;

    PANEL_wide_top_person = NULL;
    PANEL_wide_bot_person = NULL;
}

// claude-ai: EWAY_process_conversation() — advances the active scripted two-person conversation.
// claude-ai: Conversations are triggered by EWAY_DO_CONVERSATION or EWAY_DO_AMBIENT_CONV waypoints.
// claude-ai: The message string in EWAY_mess is pipe-delimited ('|'); speakers alternate via SWAP(person_a, person_b).
// claude-ai: Each line is displayed via PANEL_new_text() and voiced via EWAY_talk_conv().
// claude-ai: Non-ambient conversations: switches game to widescreen mode, locks camera, disables player input.
// claude-ai: EWAY_conv_active flag suppresses countdown timers (EWAY_COND_COUNTDOWN) during conversations.
// claude-ai: When all lines are done, sets EWAY_FLAG_FINISHED on the conversation waypoint.
//
// Processes a conversation.
//

void EWAY_process_conversation(void)
{
    CBYTE* ch;
    CBYTE* str;

    check_eway_talk(0);
    if (!EWAY_conv_active) {
        return;
    }
    timer_bored = 0;

    //	ASSERT(GAME_TURN != 12313);

    //
    // Time for the next person to talk?
    //

    EWAY_conv_timer -= EWAY_tick;
    EWAY_conv_skip -= EWAY_tick;

    if (EWAY_conv_skip <= 0) {
        //		if(0)
        if (!EWAY_conv_ambient) {
            if (NET_PLAYER(0)->Genus.Player->Pressed & (INPUT_MASK_JUMP | INPUT_MASK_KICK | INPUT_MASK_ACTION | INPUT_MASK_PUNCH)) {
                //
                // cut off the voice-overs
                //
                MFX_QUICK_stop();

                EWAY_conv_timer = 0;
            }
        }
    } else {
        //		if(0)
        if (!EWAY_conv_ambient)
            if (NET_PLAYER(0)->Genus.Player->Pressed & (INPUT_MASK_JUMP | INPUT_MASK_KICK | INPUT_MASK_ACTION | INPUT_MASK_PUNCH)) {
                //
                // Two presses in a row skips...
                //

                EWAY_conv_skip = 0;
            }
    }

    if (EWAY_conv_timer <= 0 || (EWAY_conv_talk && MFX_QUICK_still_playing() == 0)) {
        str = &EWAY_mess_buffer[EWAY_conv_str];

        //
        // Reached the end of the conversation?
        //

        if (*str == 0) {
            EWAY_finish_conversation();

            return;
        }

        EWAY_talk_conv((EWAY_way[EWAY_conv_waypoint].yaw << 8) + (EWAY_way[EWAY_conv_waypoint].index), EWAY_conv_str_count);

        //
        // What are they saying? Create a NULL terminated string.  Work out
        // how long the string is and how much time wait before the next string.
        //
        //		ASSERT(0);
        for (ch = str, EWAY_conv_timer = 200; *ch != '\000' && *ch != '|'; ch++, EWAY_conv_str += 1, EWAY_conv_timer += 4)
            ;

        if (ch[0] == '|') {
            EWAY_conv_str += 1;
            EWAY_conv_str_count++;

            ch[0] = '\000';
        }

        EWAY_conv_skip = 50;

        PANEL_new_text(
            TO_THING(EWAY_conv_person_a),
            EWAY_conv_timer * 10 - 500,
            str);

        //
        // Make our two people say stuff.
        //

        PCOM_make_people_talk_to_eachother(
            TO_THING(EWAY_conv_person_a),
            TO_THING(EWAY_conv_person_b),
            ch[-1] == '?' || ch[-2] == '?',
            TRUE);

        if (!EWAY_conv_ambient) {
            //
            // Look at who ever is talking.
            //

            EWAY_cam_converse(TO_THING(EWAY_conv_person_a), TO_THING(EWAY_conv_person_b));
        }

        //
        // Swap whos talking.
        //

        SWAP(EWAY_conv_person_a, EWAY_conv_person_b);
        EWAY_cam_jumped = 10;
    }

    //
    // If either of the people in the conversation are attacked, run-over or if anything
    // unusual happens to them, then stop the conversation.
    //

    {
        Thing* p_person_a = TO_THING(EWAY_conv_person_a);
        Thing* p_person_b = TO_THING(EWAY_conv_person_b);

        if ((p_person_a->SubState != SUB_STATE_SIMPLE_ANIM && p_person_a->SubState != SUB_STATE_SIMPLE_ANIM_OVER && p_person_a->State != STATE_IDLE) || (p_person_b->SubState != SUB_STATE_SIMPLE_ANIM && p_person_b->SubState != SUB_STATE_SIMPLE_ANIM_OVER && p_person_b->State != STATE_IDLE)) {
            EWAY_finish_conversation();

            return;
        }
    }
}

//
// Processes a waypoint that is emitting steam.
//

void EWAY_process_emit_steam(EWAY_Way* ew)
{
    SLONG tick;

    SLONG speed;
    SLONG steps;
    SLONG range;
    SLONG choreography;

    ASSERT(ew->flag & EWAY_FLAG_ACTIVE);
    ASSERT(ew->ed.type == EWAY_DO_EMIT_STEAM);

    speed = (ew->ed.arg2 >> 10) & 0x3f;
    steps = (ew->ed.arg2 >> 6) & 0x0f;
    range = (ew->ed.arg2 >> 0) & 0x3f;

    tick = EWAY_time * speed >> 7;
    tick %= steps;

    choreography = ew->ed.arg1;

    if (choreography & (1 << tick)) {
        SLONG dx;
        SLONG dy;
        SLONG dz;

        switch (ew->ed.subtype) {
        case EWAY_SUBTYPE_STEAM_FORWARD:
            dx = -SIN(ew->yaw << 3) * range >> 8;
            dy = 0;
            dz = -COS(ew->yaw << 3) * range >> 8;
            break;

        case EWAY_SUBTYPE_STEAM_UP:
            dx = 0;
            dy = range << 8;
            dz = 0;
            break;

        case EWAY_SUBTYPE_STEAM_DOWN:
            dx = 0;
            dy = -range << 8;
            dz = 0;
            break;

        default:
            ASSERT(0);
            break;
        }

        dx += (Random() & 0xff) - 0x7f;
        dy += (Random() & 0xff) - 0x7f;
        dz += (Random() & 0xff) - 0x7f;

        PARTICLE_Add(
            ew->x + (Random() & 0x7) - 0x3 << 8,
            ew->y + (Random() & 0x7) - 0x3 << 8,
            ew->z + (Random() & 0x7) - 0x3 << 8,
            dx,
            dy,
            dz,
            POLY_PAGE_SMOKECLOUD2,
            2 + ((Random() & 0x3) << 2),
            0x7fe8ffd0,
            PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE | PFLAG_HURTPEOPLE,
            100,
            40,
            1,
            1,
            1);
    }
}

//
// Does what has to be done when a waypoint goes active.
//

// claude-ai: EWAY_set_active() — fires a waypoint's action. Called when EWAY_evaluate_condition() returns TRUE.
// claude-ai: Sets EWAY_FLAG_ACTIVE on the waypoint, then dispatches based on ew->ed.type (EWAY_DO_*).
// claude-ai:
// claude-ai: Key action types handled here (all EWAY_DO_* cases):
// claude-ai:   CREATE_PLAYER    → EWAY_create_player(); result Thing index stored in ew->ed.arg1
// claude-ai:   CREATE_ANIMAL    → EWAY_create_animal(); gargoyle/balrog/bane via BAT_create()
// claude-ai:   CREATE_ENEMY     → EWAY_create_enemy() using EWAY_Edef params; Thing index → ew->ed.arg1
// claude-ai:   CREATE_ITEM      → EWAY_create_item(); if EWAY_ARG_ITEM_FOLLOW_PERSON, item spawns near condition NPC
// claude-ai:   CREATE_VEHICLE   → EWAY_create_vehicle(); Thing index → ew->ed.arg1
// claude-ai:   SOUND_ALARM      → MFX_play_xyz + PCOM_oscillate_tympanum (alert all nearby NPCs)
// claude-ai:   SOUND_EFFECT     → play_glue_wave at waypoint position
// claude-ai:   CONTROL_DOOR     → DOOR_open(x, z) — opens door at waypoint position
// claude-ai:   EXPLODE          → PYRO_construct() explosion effect + shockwave damage
// claude-ai:   SPOT_FX          → PYRO_create(PYRO_IRONICWATERFALL) persistent particle effect
// claude-ai:   MESSAGE          → plays voice wav (EWAY_talk) + PANEL_new_text() to show subtitle
// claude-ai:   NAV_BEACON       → MAP_beacon_create() — adds a minimap marker
// claude-ai:   ELECTRIFY_FENCE  → set_electric_fence_state(arg1, TRUE)
// claude-ai:   CAMERA_CREATE    → EWAY_create_camera() — starts the cut-scene camera
// claude-ai:   MISSION_FAIL     → GAME_STATE = GS_LEVEL_LOST
// claude-ai:   MISSION_COMPLETE → GAME_STATE = GS_LEVEL_WON; makes Darci invulnerable; calls set_stats()
// claude-ai:   CHANGE_ENEMY     → PCOM_change_person_attributes() — alters NPC AI/behaviour mid-mission
// claude-ai:   CHANGE_ENEMY_FLG → sets pcom_bent and pcom_zone directly on the NPC struct
// claude-ai:   CREATE_PLATFORM  → PLAT_create() — creates a moving platform
// claude-ai:   KILL_WAYPOINT    → sets EWAY_FLAG_DEAD on another waypoint; also destroys its created entity:
// claude-ai:                      enemies teleported to (0x80,0x80), vehicles blown up, nav beacons removed
// claude-ai:   OBJECTIVE        → reduces CRIME_RATE by the points value × CRIME_RATE_SCORE_MUL
// claude-ai:   GROUP_LIFE       → clears EWAY_FLAG_DEAD on all waypoints sharing colour/group
// claude-ai:   WATER_SPOUT      — does nothing here; effect runs each tick in EWAY_process() while active
// claude-ai:   CREATE_BOMB      — bomb was pre-created at EWAY_create() time; nothing to do here
void EWAY_set_active(EWAY_Way* ew)
{
    SLONG has;

    ew->flag |= EWAY_FLAG_ACTIVE;

    switch (ew->ed.type) {
    case EWAY_DO_NOTHING:
        break;

    case EWAY_DO_CREATE_PLAYER:

        has = (ew->ed.subtype & 8) ? 0 : PCOM_HAS_GUN;

        ew->ed.arg1 = EWAY_create_player(
            ew->ed.subtype,
            ew->yaw,
            has,
            ew->x,
            ew->y,
            ew->z);
        break;

    case EWAY_DO_CREATE_ANIMAL:
        ew->ed.arg1 = EWAY_create_animal(
            ew->ed.subtype,
            ew->yaw,
            ew->x,
            ew->y,
            ew->z);
        break;

    case EWAY_DO_CREATE_ENEMY:
        // #ifndef PSX
        {
            EWAY_Edef* ee;

            ASSERT(WITHIN(ew->index, 1, EWAY_edef_upto - 1));

            ee = &EWAY_edef[ew->index];

            ew->ed.arg1 = EWAY_create_enemy(
                ew->ed.subtype,
                ew->yaw,
                ew->colour,
                ew->group,
                ee->pcom_ai,
                ee->pcom_bent,
                ee->pcom_move,
                ee->drop,
                ee->zone,
                ee->ai_skill,
                ee->follow,
                ee->ai_other,
                ee->pcom_has,
                ew->x,
                ew->y,
                ew->z,
                ew - EWAY_way);
        }
        // #endif
        break;

    case EWAY_DO_CREATE_ITEM:

        extern void find_nice_place_near_person(
            Thing * p_person,
            SLONG * nice_x, // 8-bits per mapsquare
            SLONG * nice_y,
            SLONG * nice_z);

        if ((ew->ed.arg1 & EWAY_ARG_ITEM_FOLLOW_PERSON) && ((ew->ec.type == EWAY_COND_KILLED_NOT_ARRESTED) || (ew->ec.type == EWAY_COND_PERSON_ARRESTED) || (ew->ec.type == EWAY_COND_PERSON_DEAD) || (ew->ec.type == EWAY_COND_HALF_DEAD) || (ew->ec.type = EWAY_COND_PERSON_USED))) {
            ASSERT(ew->ec.arg1);

            if (ew->ec.arg1) {
                SLONG person = EWAY_get_person(ew->ec.arg1);

                ASSERT(person);

                if (person) {
                    SLONG item_x;
                    SLONG item_y;
                    SLONG item_z;

                    Thing* p_bloke = TO_THING(person);

                    find_nice_place_near_person(
                        p_bloke,
                        &item_x,
                        &item_y,
                        &item_z);

                    ew->x = item_x;
                    ew->y = item_y;
                    ew->z = item_z;
                }
            }
        }

        ew->ed.arg2 = EWAY_create_item(
            ew->ed.subtype,
            ew->x,
            ew->y,
            ew->z,
            ew);

        break;

    case EWAY_DO_CREATE_VEHICLE:
        ew->ed.arg1 = EWAY_create_vehicle(
            ew->ed.subtype,
            ew->ed.arg1,
            ew->ed.arg2,
            ew->x,
            ew->y,
            ew->z,
            ((ew->yaw << 3) + 1024) & 2047);

        break;
    case EWAY_DO_SOUND_ALARM:

        MFX_play_xyz(0, S_KICK_CAN, MFX_OVERLAP, ew->x << 8, ew->y << 8, ew->z << 8);
        /*			play_quick_wave_xyz(
                                        ew->x << 8,
                                        ew->y << 8,
                                        ew->z << 8,
                                        S_SIREN_START,
                                        0,
                                        WAVE_PLAY_INTERUPT);*/

        PCOM_oscillate_tympanum(
            PCOM_SOUND_ALARM,
            NULL,
            ew->x,
            ew->y,
            ew->z);

        break;

    case EWAY_DO_SOUND_EFFECT:
        play_glue_wave(ew->ed.subtype, ew->ed.arg1, ew->x << 8, ew->y << 8, ew->z << 8);
        break;

    case EWAY_DO_CONTROL_DOOR:

        DOOR_open(
            ew->x,
            ew->z);

        break;

    case EWAY_DO_EXPLODE:

        //
        // Bang!
        //
        /*
                                POW_create(
                                        POW_CREATE_LARGE_SEMI,
                                        ew->x << 8,
                                        ew->y << 8,
                                        ew->z << 8);
        */

        {
            GameCoord posn;

            posn.X = ew->x << 8;
            posn.Y = ew->y << 8;
            posn.Z = ew->z << 8;

            PYRO_construct(
                posn,
                ew->ed.subtype,
                ew->ed.arg1);
        }

        if (ew->ed.subtype = !16) {

            //
            // 16 is fire on its own...
            //

            create_shockwave(
                ew->x,
                ew->y,
                ew->z,
                0x400,
                100,
                NULL);
        }

        break;

    case EWAY_DO_SPOT_FX:

        // water and similar stuff
        GameCoord posn;
        Thing* pyro;

        posn.X = ew->x << 8;
        posn.Y = ew->y << 8;
        posn.Z = ew->z << 8;
        pyro = PYRO_create(posn, PYRO_IRONICWATERFALL);
        if (pyro) {
            IRONICWATERFALL_state_function[STATE_INIT].StateFn(pyro);
            //			PYRO_fn_init(pyro);
            pyro->Genus.Pyro->radius = ew->ed.subtype;
            pyro->Genus.Pyro->scale = ew->ed.arg1;
        }
        break;

    case EWAY_DO_MESSAGE:

        //
        // Add a message to the console.
        //

        if (!WITHIN(ew->ed.arg1, 0, EWAY_MAX_MESSES - 1)) {
            CONSOLE_text("Too many messages for the waypoint system! Tell Mark!", 8000);
        } else {
            SLONG time = ew->ed.subtype;

            if (time) {
                time *= 1000;
            } else {
                time = 8000;
            }

            if (EWAY_mess[ew->ed.arg1] == NULL) {
            } else {
                if (EWAY_used_thing) {

                    talk_thing = TO_THING(EWAY_used_thing);
                    EWAY_talk((ew->yaw << 8) + ew->index);

                    // HACK: JDW - Add waypoint number to start of messages

                    PANEL_new_text(
                        TO_THING(EWAY_used_thing),
                        time,
                        EWAY_mess[ew->ed.arg1]);

                    if (person_ok_for_conversation(TO_THING(EWAY_used_thing))) {
                        PCOM_make_people_talk_to_eachother(
                            TO_THING(EWAY_used_thing),
                            NET_PERSON(0),
                            FALSE,
                            FALSE);
                    }
                } else {
                    if (ew->ed.arg2 == EWAY_MESSAGE_WHO_STREETNAME) {
                        PANEL_new_help_message(EWAY_mess[ew->ed.arg1]);
                    } else if (ew->ed.arg2 == EWAY_MESSAGE_WHO_TUTORIAL) {
                        EWAY_tutorial_string = EWAY_mess[ew->ed.arg1];
                        EWAY_tutorial_counter = 0;

                        MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SEARCH_END);
                        MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SLIDE_START);

                        LastKey = 0;
                    } else {
                        Thing* who_says = NULL;

                        if (ew->ed.arg2) {
                            SLONG who = EWAY_get_person(ew->ed.arg2);

                            if (who) {
                                who_says = TO_THING(who);
                            }
                        }

                        if (who_says && who_says->State == STATE_DEAD) {
                            //
                            // Dead people can't talk.
                            //
                        } else {
                            talk_thing = who_says;
                            EWAY_talk((ew->yaw << 8) + ew->index);

                            PANEL_new_text(
                                who_says,
                                time,
                                EWAY_mess[ew->ed.arg1]);

                            if (who_says && who_says != NET_PERSON(0)) {
                                if (person_ok_for_conversation(who_says)) {
                                    PCOM_make_people_talk_to_eachother(
                                        who_says,
                                        NET_PERSON(0),
                                        FALSE,
                                        FALSE,
                                        FALSE);
                                }
                            }
                        }
                    }
                }
                if (ew->flag & EWAY_FLAG_WHY_LOST) {
                    //
                    // This is a message that comes up when you've lost the level.
                    //

                    GAMEMENU_set_level_lost_reason(EWAY_mess[ew->ed.arg1]);
                }
            }
        }

        break;

    case EWAY_DO_NAV_BEACON:

        //
        // Add a nav beacon to the map.
        //

        {
            CBYTE* str;
            UWORD track_thing = NULL;

            if (!WITHIN(ew->ed.arg1, 0, EWAY_MAX_MESSES - 1)) {
                CONSOLE_text("Too many navbeacon messages for the waypoint system! Tell Mark!");
            } else {
                str = EWAY_mess[ew->ed.arg1];
            }

            track_thing = EWAY_get_person(ew->ed.arg2);

            if (ew->ed.arg2 && track_thing == 0) {
                //
                // Person not created yet, maybe next time!
                //
                ew->flag &= ~EWAY_FLAG_ACTIVE;

                return;

            } else {

                ew->ed.subtype = MAP_beacon_create(ew->x, ew->z, ew->ed.arg1, track_thing);
                ASSERT(ew->ed.subtype);

                if (GAME_TURN < 16) {
                    MFX_play_stereo(0, S_MENU_END, MFX_REPLACE);
                }
            }

            //

            //				CBYTE msg[300];
            //				strcpy(msg,"if it was finished yet, beacon \"");
            //				if (EWAY_mess[ew->ed.arg])
            //					strcat(msg,EWAY_mess[ew->ed.arg]);
            //				else
            //					strcat(msg,"default");
            //				strcat(msg,"\" would be active.");
            //				CONSOLE_text(msg,4000);
            //

            // void	set_beacon(SLONG bx,SLONG by,SLONG bz);
            //				set_beacon(ew->x,ew->y,ew->z);
        }

        break;

    case EWAY_DO_ELECTRIFY_FENCE:

        //
        // Turn on the electric fence.
        //

        if (ew->ed.arg1) {
            set_electric_fence_state(ew->ed.arg1, TRUE);
        }

        break;

    case EWAY_DO_CAMERA_CREATE:
        EWAY_create_camera(ew - EWAY_way);
        if (EWAY_cam_freeze)
            MFX_play_stereo(0, S_CUTSCENE_STING, 0);
        break;

    case EWAY_DO_CAMERA_WAYPOINT:
        break;

    case EWAY_DO_CAMERA_TARGET:
        break;

    case EWAY_DO_MISSION_FAIL:
        // claude-ai: WPT_FAIL_MISSION equivalent — sets GAME_STATE = GS_LEVEL_LOST (unless already won).
        // claude-ai: Game loop detects GS_LEVEL_LOST and shows the failure screen.
        if (GAME_STATE != GS_LEVEL_WON) {
            GAME_STATE = GS_LEVEL_LOST;
        }
        break;

    case EWAY_DO_MISSION_COMPLETE:
        // claude-ai: WPT_WIN_MISSION equivalent — sets GS_LEVEL_WON, makes Darci invulnerable, calls set_stats().
        // claude-ai: set_stats() calculates mission score and updates mission_hierarchy[] completed bit.
        if (GAME_STATE != GS_LEVEL_LOST) {
            //
            // Make sure Darci doesn't die!
            //

            NET_PERSON(0)->Genus.Person->Flags2 |= FLAG2_PERSON_INVULNERABLE;

            GAME_STATE = GS_LEVEL_WON;
            extern void set_stats(void);
            set_stats();
        }
        break;

    case EWAY_DO_CHANGE_ENEMY:

        //			ASSERT(WITHIN(ew->ed.arg1, 1, EWAY_way_upto  - 1));
        ASSERT(WITHIN(ew->index, 1, EWAY_edef_upto - 1));

        {
            EWAY_Edef* ee = &EWAY_edef[ew->index];

            SLONG change = EWAY_get_person(ew->ed.arg1);

            if (change == NULL) {
            } else {
                Thing* p_change = TO_THING(change);

                PCOM_change_person_attributes(
                    p_change,
                    ew->colour,
                    ew->group,
                    ee->pcom_ai,
                    ee->ai_other,
                    ee->pcom_move,
                    ee->follow,
                    ee->pcom_bent,
                    ew->yaw << 3);
            }
        }

        break;

    case EWAY_DO_CHANGE_ENEMY_FLG: {
        SLONG change = EWAY_get_person(ew->ed.arg1);

        if (change == NULL) {
        } else {
            Thing* p_change = TO_THING(change);

            p_change->Genus.Person->pcom_bent = ew->ed.arg2;
            p_change->Genus.Person->pcom_zone = ew->ed.arg2 >> 8;
            extern void PCOM_set_person_ai_normal(Thing * p_person);
            PCOM_set_person_ai_normal(p_change);
        }
    } break;

    case EWAY_DO_CREATE_PLATFORM:
        ew->ed.arg1 = PLAT_create(
            ew->colour,
            ew->group,
            PLAT_MOVE_PATROL,
            ew->ed.arg2,
            ew->ed.arg1,
            ew->x,
            ew->y,
            ew->z);
        break;
    case EWAY_DO_CREATE_BOMB:

        //
        // The 'special' bomb monitors the waypoint that created it and
        // explodes when the waypoint goes active.
        //

        break;

    case EWAY_DO_WATER_SPOUT:

        //
        // This waypoint spouts out water while active. It is done inside EWAY_process()
        //

        break;

    case EWAY_DO_KILL_WAYPOINT:

        if (!(WITHIN(ew->ed.arg1, 1, EWAY_way_upto - 1))) {
        } else {
            EWAY_Way* ewk = &EWAY_way[ew->ed.arg1];

            ewk->flag |= EWAY_FLAG_DEAD;

            if (ewk->ec.type == EWAY_COND_COUNTDOWN_SEE) {
                music_mode_override = 0;
            }

            if (ewk->ed.type == EWAY_DO_CREATE_ENEMY) {
                if (ewk->ed.arg1) {
                    Thing* p_person = TO_THING(ewk->ed.arg1);

                    ASSERT(p_person->Class == CLASS_PERSON);

                    //
                    // Make this person take a holiday in another dimension!
                    //

                    p_person->Genus.Person->pcom_ai = PCOM_AI_NONE;
                    p_person->Genus.Person->pcom_bent = PCOM_BENT_ROBOT;

                    remove_thing_from_map(p_person);

                    p_person->WorldPos.X = 0x8000;
                    p_person->WorldPos.Z = 0x8000;

                    p_person->WorldPos.Y = PAP_calc_map_height_at(0x80, 0x80);

                    set_person_idle(p_person);

                    //
                    // As if this person has never been created.
                    //

                    ewk->ed.arg1 = NULL;
                }
            } else if (ewk->ed.type == EWAY_DO_CREATE_VEHICLE) {
                if (ewk->ed.arg1) {
                    Thing* p_vehicle = TO_THING(ewk->ed.arg1);

                    if (p_vehicle->Class == CLASS_VEHICLE) {
                        //
                        // Make the vehicle blow up!
                        //

                        extern void VEH_reduce_health(Thing * p_car, Thing * p_person, SLONG damage);

                        VEH_reduce_health(
                            p_vehicle,
                            NULL,
                            1050);
                        extern UBYTE hit_player;
                        hit_player = 1;
                        create_shockwave(
                            p_vehicle->WorldPos.X >> 8,
                            p_vehicle->WorldPos.Y >> 8,
                            p_vehicle->WorldPos.Z >> 8,
                            0x300,
                            350,
                            NULL, 1);
                        hit_player = 0;

                        //
                        // As if this vehicle has never been created.
                        //

                        ewk->ed.arg1 = NULL;
                    }
                }
            } else if (ewk->ed.type == EWAY_DO_NAV_BEACON) {
                if (ewk->ed.subtype) {
                    MAP_beacon_remove(ewk->ed.subtype);

                    ewk->ed.subtype = NULL;
                }
            } else if (ewk->ed.type == EWAY_DO_ELECTRIFY_FENCE) {
                //
                // Turn off the electric fence.
                //

                if (ew->ed.arg1) {
                    set_electric_fence_state(ew->ed.arg1, FALSE);
                }
            } else if (ewk->ed.type == EWAY_DO_MAKE_PERSON_PEE) {
                SLONG person = EWAY_get_person(ewk->ed.arg1);

                if (person) {
                    Thing* p_person = TO_THING(person);

                    ASSERT(p_person->Class == CLASS_PERSON);

                    p_person->Genus.Person->Flags &= ~FLAG_PERSON_PEEING;
                }
            } else if (ewk->ec.type == EWAY_COND_TRIPWIRE) {
                if (ewk->ec.arg1) {
                    TRIP_deactivate(ewk->ec.arg1);
                }
            } else if (ewk->ed.type == EWAY_DO_CREATE_BARREL) {
                //
                // Make the barrel dissapear!
                //

                if (ewk->ed.arg1) {
                    Thing* p_barrel = TO_THING(ewk->ed.arg1);

                    if (p_barrel->Class == CLASS_BARREL) {
                        void BARREL_dissapear(Thing * p_barrel);

                        BARREL_dissapear(p_barrel);
                    }

                    ewk->ed.arg1 = 0;
                }
            }
        }

        break;

    case EWAY_DO_OBJECTIVE:
        // claude-ai: EWAY_DO_OBJECTIVE — reduces CRIME_RATE by (arg2 * CRIME_RATE_SCORE_MUL >> 8) percent.
        // claude-ai: arg2 = points value/10 from editor. CRIME_RATE_SCORE_MUL set at level load (EWAY_created_last_waypoint).
        // claude-ai: NOTE: In practice NEVER reached — WPT_BONUS_POINTS (the editor type that maps here) is
        // claude-ai: now translated to EWAY_DO_MESSAGE via if(0) dead-code block in elev.cpp. This is legacy code.
        // claude-ai: CRIME_RATE after: -= percent; SATURATE(0,100). Shows message string from EWAY_mess[arg1].

        {
            UWORD mess = ew->ed.arg1;
            SLONG percent = ew->ed.arg2 * CRIME_RATE_SCORE_MUL >> 8;

            CBYTE* str;

            if (!WITHIN(mess, 0, EWAY_MAX_MESSES - 1)) {
                str = "Too many messages for the waypoint system! Tell Mark!";
            } else {
                if (EWAY_mess[mess] == NULL) {
                    str = "No objective message";
                } else {
                    str = EWAY_mess[mess];
                }
            }

            CRIME_RATE -= percent;

            SATURATE(CRIME_RATE, 0, 100);

            //
            // If its a Roper mission then the crime rate isn't reduced.
            //
        }

        break;

    case EWAY_DO_GROUP_LIFE:
        // claude-ai: GROUP_LIFE — clears EWAY_FLAG_DEAD on all waypoints sharing same colour AND group.
        // claude-ai: Immune: waypoints of type GROUP_LIFE or GROUP_DEATH themselves (skipped).
        // claude-ai: Effect: "resurrects" dead waypoints so they can be evaluated again next frame.

        //
        // Make all waypoints in our colour/group active
        //

        {
            SLONG i;

            for (i = 1; i < EWAY_way_upto; i++) {
                //					global_write2=i;
                if (EWAY_way[i].group == ew->group && EWAY_way[i].colour == ew->colour) {
                    if (EWAY_way[i].ed.type == EWAY_DO_GROUP_LIFE || EWAY_way[i].ed.type == EWAY_DO_GROUP_DEATH) {
                        //
                        // These waypoints are immune.
                        //
                    } else {
                        EWAY_way[i].flag &= ~EWAY_FLAG_DEAD;
                    }
                }
            }
        }

        break;

    case EWAY_DO_GROUP_DEATH:
        // claude-ai: GROUP_DEATH — sets EWAY_FLAG_DEAD on all waypoints sharing same colour AND group.
        // claude-ai: Immune: GROUP_LIFE and GROUP_DEATH types themselves (skipped).
        // claude-ai: Effect: permanently disables entire script branch until a GROUP_LIFE revives it.

        //
        // Make all waypoints in our colour/group dead
        //

        {
            SLONG i;

            for (i = 1; i < EWAY_way_upto; i++) {
                //					global_write3=i;
                if (EWAY_way[i].group == ew->group && EWAY_way[i].colour == ew->colour) {
                    if (EWAY_way[i].ed.type == EWAY_DO_GROUP_LIFE || EWAY_way[i].ed.type == EWAY_DO_GROUP_DEATH) {
                        //
                        // These waypoints are immune.
                        //
                    } else {
                        EWAY_way[i].flag |= EWAY_FLAG_DEAD;
                    }
                }
            }
        }

        break;

    case EWAY_DO_CONVERSATION:
    case EWAY_DO_AMBIENT_CONV:

    {
        UWORD person_a = EWAY_get_person(ew->ed.arg1);
        UWORD person_b = EWAY_get_person(ew->ed.arg2);

        if (person_a && person_b) {
            if (person_ok_for_conversation(TO_THING(person_a)) && person_ok_for_conversation(TO_THING(person_b))) {
                if (EWAY_conv_active) {
                    //
                    // End the current converstaion.
                    //

                    EWAY_finish_conversation();
                }

                //
                // Make sure the people aren't too close together.
                //

                {
                    Thing* p_person_a = TO_THING(person_a);
                    Thing* p_person_b = TO_THING(person_b);

                    extern void push_people_apart(Thing * p_person, Thing * p_avoid);

                    push_people_apart(
                        p_person_a,
                        p_person_b);
                }

                //
                // Start a converstaion.
                //

                EWAY_conv_active = TRUE;
                EWAY_conv_waypoint = ew - EWAY_way;
                EWAY_conv_person_a = person_a;
                EWAY_conv_person_b = person_b;
                EWAY_conv_timer = 0;
                EWAY_conv_str_count = 0;
                EWAY_conv_skip = 100;
                EWAY_conv_ambient = (ew->ed.type == EWAY_DO_AMBIENT_CONV);

                if (!WITHIN(ew->ed.subtype, 0, EWAY_MAX_MESSES - 1)) {
                    CONSOLE_text("Too many messages for the waypoint system! Tell Mark!", 8000);
                } else {
                    if (EWAY_mess[ew->ed.subtype] == NULL) {
                        EWAY_conv_str = 0;
                    } else {
                        EWAY_conv_str = EWAY_mess[ew->ed.subtype] - EWAY_mess_buffer;
                    }
                }
            }
        }
    }

    break;

    case EWAY_DO_INCREASE_COUNTER:
        // claude-ai: Increments EWAY_counter[subtype] by 1. subtype = counter index (0..EWAY_MAX_COUNTERS-1).
        // claude-ai: EWAY_COND_COUNTER_GTEQ checks if EWAY_counter[arg1] >= arg2.
        // claude-ai: WPT_INCREMENT in editor → ed.subtype = Data[1] (counter index).

        ASSERT(WITHIN(ew->ed.subtype, 0, EWAY_MAX_COUNTERS - 1));

        EWAY_counter[ew->ed.subtype] += 1;

        break;

    case EWAY_DO_EMIT_STEAM:

        //
        // The code for doing this is in EWAY_process_emit_steam() and is
        // called _while_ the waypoint is active.
        //

        break;

    case EWAY_DO_TRANSFER_PLAYER:

    {
        THING_INDEX i_player = EWAY_get_person(ew->ed.arg1);

        if (i_player == NULL) {
        } else {
            Thing* p_person = TO_THING(i_player);

            //
            // first, preserve roper/darci's stats in the_game for saving later
            //
            /*
                                                    if (NET_PERSON(0)->Genus.Person->PersonType==PERSON_DARCI)
                                                    {
                                                            the_game.DarciConstitution=NET_PLAYER(0)->Genus.Player->Constitution;
                                                            the_game.DarciStrength=NET_PLAYER(0)->Genus.Player->Strength;
                                                            the_game.DarciStamina=NET_PLAYER(0)->Genus.Player->Stamina;
                                                            the_game.DarciSkill=NET_PLAYER(0)->Genus.Player->Skill;
                                                    }
                                                    else
                                                    {
                                                            the_game.RoperConstitution=NET_PLAYER(0)->Genus.Player->Constitution;
                                                            the_game.RoperStrength=NET_PLAYER(0)->Genus.Player->Strength;
                                                            the_game.RoperStamina=NET_PLAYER(0)->Genus.Player->Stamina;
                                                            the_game.RoperSkill=NET_PLAYER(0)->Genus.Player->Skill;
                                                    }
            */

            //
            // Stop the old player being a player.
            //

            NET_PERSON(0)->Genus.Person->PlayerID = 0;
            NET_PERSON(0)->Genus.Person->pcom_ai = PCOM_AI_CIV;
            NET_PERSON(0)->Genus.Person->pcom_bent = PCOM_BENT_ROBOT | PCOM_BENT_FIGHT_BACK;
            NET_PERSON(0)->Genus.Person->Flags2 |= FLAG2_PERSON_INVULNERABLE;
            NET_PERSON(0)->Genus.Person->HomeX = NET_PERSON(0)->WorldPos.X >> 8;
            NET_PERSON(0)->Genus.Person->HomeZ = NET_PERSON(0)->WorldPos.Z >> 8;
            NET_PERSON(0)->Genus.Person->HomeYaw = NET_PERSON(0)->Draw.Tweened->Angle >> 3;

            SET_SKILL(NET_PERSON(0), 15);

            set_person_idle(NET_PERSON(0));

            //
            // Make the new player be a player.
            //

            NET_PERSON(0) = p_person;
            p_person->Genus.Person->PlayerID = 1;
            NET_PLAYER(0)->Genus.Player->PlayerPerson = p_person;
            NET_PERSON(0)->Genus.Person->pcom_ai = PCOM_AI_NONE;
            NET_PERSON(0)->Genus.Person->Flags2 &= ~FLAG2_PERSON_INVULNERABLE;
            NET_PERSON(0)->Genus.Person->pcom_bent &= ~PCOM_BENT_PLAYERKILL; // PCOM_BENT_ROBOT | PCOM_BENT_FIGHT_BACK;

            SET_SKILL(NET_PERSON(0), 0);

            FC_look_at(0, i_player);
            FC_setup_initial_camera(0);

            if (NET_PERSON(0)->State == STATE_MOVEING) {
                if (NET_PERSON(0)->SubState == SUB_STATE_SIMPLE_ANIM || NET_PERSON(0)->SubState == SUB_STATE_SIMPLE_ANIM_OVER) {
                    set_person_idle(NET_PERSON(0));
                }
            }
        }
    } break;

    case EWAY_DO_AUTOSAVE: {

        /*

        // -- PLACE AUTOSAVE CODE HERE --
#ifndef	PSX
extern	SLONG	SAVE_ingame(CBYTE *fname);
        SAVE_ingame("");
        CONSOLE_text("GAME AUTOSAVED");
#endif
        */

    } break;

    case EWAY_DO_CREATE_BARREL:

        ew->ed.arg1 = BARREL_alloc(
            ew->ed.subtype,
            //(ew->ed.subtype == BARREL_TYPE_CONE) ? PRIM_OBJ_TRAFFIC_CONE : PRIM_OBJ_BARREL,
            ew->ed.arg2,
            ew->x,
            ew->z,
            ew - EWAY_way);

        break;

    case EWAY_DO_LOCK_VEHICLE:

    {
        Thing* p_vehicle;

        if (!WITHIN(ew->ed.arg1, 1, EWAY_way_upto - 1)) {
        } else {
            EWAY_Way* ewv = &EWAY_way[ew->ed.arg1];

            if (ewv->ed.arg1) {
                Thing* p_vehicle = TO_THING(ewv->ed.arg1);

                if (p_vehicle->Class != CLASS_VEHICLE) {
                } else {
                    switch (ew->ed.subtype) {
                    case EWAY_SUBTYPE_VEHICLE_LOCK:
                        p_vehicle->Genus.Vehicle->key = SPECIAL_NUM_TYPES; // Non-existent special.
                        break;

                    case EWAY_SUBTYPE_VEHICLE_UNLOCK:
                        p_vehicle->Genus.Vehicle->key = SPECIAL_NONE;
                        break;

                    default:
                        ASSERT(0);
                        break;
                    }
                }
            }
        }
    }

    break;

    case EWAY_DO_CUTSCENE:
        PLAYCUTS_Play(PLAYCUTS_cutscenes + ew->ed.arg1);
        break;

    case EWAY_DO_GROUP_RESET:

    {
        SLONG i;

        EWAY_Way* ewr;

        for (i = 1; i < EWAY_way_upto; i++) {
            //					global_write4=i;
            ewr = &EWAY_way[i];

            if (ewr->colour == ew->colour && ewr->group == ew->group) {
                if (ewr->ed.type == EWAY_DO_GROUP_RESET) {
                    //
                    // Immune!
                    //
                } else {
                    if (ewr->flag & EWAY_FLAG_ACTIVE) {
                        void EWAY_set_inactive(EWAY_Way * ew);

                        EWAY_set_inactive(ewr);
                    }

                    ewr->flag &= ~(EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD | EWAY_FLAG_FINISHED);
                }
            }
        }
    }

    break;

    case EWAY_DO_VISIBLE_COUNT_UP:

        //
        // The code for doing this is in EWAY_process() and is
        // called _while_ the waypoint is active.
        //

        EWAY_count_up = 0;
        EWAY_count_up_add_penalties = FALSE;
        EWAY_count_up_num_penalties = 0;
        EWAY_count_up_penalty_timer = 0;
        EWAY_counter[3] = 0;

        break;

    case EWAY_DO_RESET_COUNTER:

        if (ew->ed.subtype == 0) {
            UBYTE i;

            for (i = 0; i < EWAY_MAX_COUNTERS; i++) {
                EWAY_counter[i] = 0;
            }
        } else {
            ASSERT(WITHIN(ew->ed.subtype, 0, EWAY_MAX_COUNTERS - 1));

            EWAY_counter[ew->ed.subtype] = 0;
        }

        break;

    case EWAY_DO_CREATE_MIST: {
        static SLONG last_detail = 17;
        static SLONG last_height = 84;

#define MIST_SIZE 0x800

        SLONG x1 = (ew->x) - MIST_SIZE;
        SLONG z1 = (ew->z) - MIST_SIZE;
        SLONG x2 = (ew->x) + MIST_SIZE;
        SLONG z2 = (ew->z) + MIST_SIZE;

        MIST_create(
            last_detail,
            last_height,
            x1, z1,
            x2, z2);

        last_height += (last_height & 0x1) ? -11 : +11;
    } break;

    case EWAY_DO_STALL_CAR:

    {
        UWORD veh = EWAY_get_person(ew->ed.arg1);

        if (veh) {
            Thing* p_vehicle = TO_THING(veh);

            ASSERT(p_vehicle->Class == CLASS_VEHICLE);

            p_vehicle->Genus.Vehicle->Flags |= FLAG_VEH_STALLED;
        }
    }

    break;

    case EWAY_DO_EXTEND_COUNTDOWN:

        if (WITHIN(ew->ed.arg1, 1, EWAY_way_upto - 1)) {
            EWAY_Way* ew_other = &EWAY_way[ew->ed.arg1];

            if (ew_other->ec.type == EWAY_COND_COUNTDOWN_SEE || ew_other->ec.type == EWAY_COND_COUNTDOWN) {
                ew_other->ec.arg2 += ew->ed.arg2 * 100;
            }
        }

        break;

    case EWAY_DO_MOVE_THING:

    {
        UWORD i_thing = EWAY_get_person(ew->ed.arg1);

        if (i_thing) {
            Thing* p_thing = TO_THING(i_thing);

            if (p_thing->State == STATE_DEAD) {
                //
                // Ignore...
                //
            } else {
                GameCoord newpos;

                newpos.X = ew->x << 8;
                newpos.Y = ew->y << 8;
                newpos.Z = ew->z << 8;

                if (p_thing->Flags & FLAGS_ON_MAPWHO) {
                    move_thing_on_map(p_thing, &newpos);
                } else {
                    p_thing->WorldPos = newpos;
                }

                if (p_thing->Class == CLASS_PERSON) {
                    plant_feet(p_thing);
                    p_thing->Draw.Tweened->Angle = ew->yaw << 3;
                    p_thing->Genus.Person->HomeX = ew->x;
                    p_thing->Genus.Person->HomeZ = ew->z;
                    p_thing->Genus.Person->HomeYaw = ew->yaw;
                }
            }
        }
    }

    break;

    case EWAY_DO_MAKE_PERSON_PEE:

    {
        SLONG person = EWAY_get_person(ew->ed.arg1);

        if (person) {
            Thing* p_person = TO_THING(person);

            ASSERT(p_person->Class == CLASS_PERSON);

            p_person->Genus.Person->Flags |= FLAG_PERSON_PEEING;
        }
    }

    break;

    case EWAY_DO_CONE_PENALTIES:
        GAME_FLAGS |= GF_CONE_PENALTIES;
        break;

    case EWAY_DO_SIGN:
        PANEL_flash_sign(ew->ed.arg1, ew->ed.arg2);
        break;

    case EWAY_DO_WAREFX: {
        SLONG ware = WARE_which_contains(ew->x >> 8, ew->z >> 8);
        if (ware)
            WARE_ware[ware].ambience = ew->ed.arg1;
    } break;
        /*
                        case EWAY_DO_WAREFX:
                                {
                                        SLONG ware=WARE_which_contains(ew->x>>8,ew->z>>8);
                                        if (ware)
                                                WARE_ware[ware].ambience=ew->ed.arg1;
                                        else
                                }
                                break;
        */

    case EWAY_DO_NO_FLOOR:
        GAME_FLAGS |= GF_NO_FLOOR;
        break;

    case EWAY_DO_SHAKE_CAMERA:
        break;

    case EWAY_DO_END_OF_WORLD:
        PYRO_create(NET_PERSON(0)->WorldPos, PYRO_GAMEOVER);
        break;

    default:
        ASSERT(0);
        break;
    }

    switch (ew->es.type) {
    case EWAY_STAY_TIME:

        //
        // Start the countdown to going inactive.
        //

        {
            UBYTE i;

            for (i = 0; i < EWAY_MAX_TIMERS; i++) {
                if (EWAY_timer[i] == 0) {
                    EWAY_timer[i] = ew->es.arg * 10; // ew->es.arg is in 10th of a second.
                    ew->timer = i;
                    ew->flag |= EWAY_FLAG_COUNTDOWN;

                    break;
                }
            }
            if (i >= EWAY_MAX_TIMERS) {
                ASSERT(0);
            }
        }

        break;

    case EWAY_STAY_ALWAYS:

        //
        // Never need to process this waypoint anymore.
        //

        ew->flag |= EWAY_FLAG_DEAD;

        break;

    case EWAY_STAY_DIE:

        //
        // Never need to process this waypoint anymore.
        //

        ew->flag |= EWAY_FLAG_DEAD;
        ew->flag &= ~EWAY_FLAG_ACTIVE;

        break;

    default:
        break;
    }
}

//
// Does what has to be done when a waypoint goes inactive.
//

void EWAY_set_inactive(EWAY_Way* ew)
{

    ew->flag &= ~EWAY_FLAG_ACTIVE;

    if (ew->ed.type == EWAY_DO_CONTROL_DOOR) {
        //
        // Close a door!
        //

        DOOR_shut(
            ew->x,
            ew->z);
    }

    if (ew->ed.type == EWAY_DO_ELECTRIFY_FENCE) {
        //
        // Turn off the electric fence.
        //

        if (ew->ed.arg1) {
            set_electric_fence_state(ew->ed.arg1, FALSE);
        }
    }

    /*

    if (ew->ed.type == EWAY_DO_VISIBLE_COUNT_UP)
    {
            //
            // We have to add on all the penalty waypoints visibly on-screen.
            //

            EWAY_count_up_add_penalties = TRUE;
            EWAY_count_up_penalty_timer = 0;

            //
            // Work out how long the count up took and put it into counter 3.
            //

            SLONG secs = (EWAY_count_up + (EWAY_count_up_num_penalties * 500)) / 1000;

            if (secs > 255)
            {
                    secs = 255;
            }

            EWAY_counter[3] = secs;
    }

    */

    if (ew->ed.type == EWAY_DO_NAV_BEACON) {
        MAP_beacon_remove(ew->ed.subtype);
    }
}

SLONG EWAY_stop_player_moving(void)
{
    return (EWAY_cam_active && EWAY_cam_freeze) || GAME_cut_scene;
}

//
// Showing the player the number of penalties incurred!
//

void EWAY_process_penalties()
{
    if (!EWAY_count_up_add_penalties) {
        return;
    }

    EWAY_count_up_penalty_timer += EWAY_tick;

    //
    // Show the time achieved.
    //

    PANEL_draw_timer(EWAY_count_up / 10, 320, 50);

    if (EWAY_count_up_penalty_timer > 100) {
        //
        // Start showing all the penalty points.
        //

        CBYTE str[64];

        if (EWAY_count_up_num_penalties == 0) {
            strcpy(str, XLAT_str(X_NO_CONES_HIT));
        } else {
            sprintf(str, "%s = %d", XLAT_str(X_NUM_CONES_HIT), (EWAY_count_up_num_penalties > 0) ? EWAY_count_up_num_penalties : 0);
        }

        FONT2D_DrawStringCentred(
            str,
            320,
            100,
            0x00ff00);

        if (EWAY_count_up_penalty_timer >= 150) {
            if (EWAY_count_up_num_penalties == 0) {
                if (EWAY_count_up_penalty_timer >= 400) {
                    EWAY_count_up_add_penalties = FALSE;
                }
            } else if (EWAY_count_up_num_penalties == -1) // -1 => We've finished add up penalities...
            {
                if (EWAY_count_up_penalty_timer >= 400) {
                    EWAY_count_up_add_penalties = FALSE;
                }
            } else {
                //
                // Add the penalties up. 3 per second.
                //

                while (EWAY_count_up_penalty_timer >= 200) {
                    EWAY_count_up_penalty_timer -= 10;
                    EWAY_count_up_num_penalties -= 1;
                    EWAY_count_up += 500; // 500 milliseconds per penalty.

                    if (EWAY_count_up_num_penalties == 0) {
                        EWAY_count_up_num_penalties = -1; // -1 => Finished adding up the penalties.
                    }
                }
            }
        }
    }
}

extern SWORD people_types[50];

void count_people_types(void)
{
    return;

    EWAY_Way* ew;
    SLONG i;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];
        if (ew->ed.type == EWAY_DO_CREATE_ENEMY) {
            people_types[ew->ed.subtype]++;
        }
    }
}

// claude-ai: EWAY_process() — the main polling loop, called once per game tick from the game update.
// claude-ai: This is the ENGINE of the entire mission system. It evaluates every waypoint every frame.
// claude-ai:
// claude-ai: Tick timing:
// claude-ai:   EWAY_time_accurate += 80 * TICK_RATIO >> TICK_SHIFT  (80 units/tick at nominal 20Hz)
// claude-ai:   EWAY_time = EWAY_time_accurate >> 4  (roughly 100 time units per second)
// claude-ai:   EWAY_tick = 5 units/tick at 20Hz = effectively 100 units/sec countdown rate
// claude-ai:
// claude-ai: PSX optimisation: processes every other waypoint per frame (step=2, offset=GAME_TURN&1).
// claude-ai: PC/DC: always step=1, offset=0 (all waypoints every frame).
// claude-ai:
// claude-ai: Per-waypoint logic (for each i in 1..EWAY_way_upto):
// claude-ai:   EWAY_FLAG_DEAD → skip (waypoint permanently killed)
// claude-ai:   EWAY_FLAG_ACTIVE:
// claude-ai:     — EMIT_STEAM: run particle emission each tick
// claude-ai:     — SHAKE_CAMERA: ramp up FC_cam[0].shake each tick
// claude-ai:     — VISIBLE_COUNT_UP: accumulate EWAY_count_up (mission timer), draw HUD timer
// claude-ai:     — EWAY_FLAG_COUNTDOWN: decrement EWAY_timer[] by EWAY_tick; if ≤0 → EWAY_set_inactive()
// claude-ai:     — else check stay rule (EWAY_STAY_ALWAYS/WHILE/WHILE_TIME/TIME):
// claude-ai:         STAY_ALWAYS → immediately mark DEAD (one-shot waypoint)
// claude-ai:         STAY_WHILE  → re-evaluate condition; if false → EWAY_set_inactive()
// claude-ai:         STAY_WHILE_TIME → as WHILE, but starts a countdown timer before going inactive
// claude-ai:     — WATER_SPOUT: emit water particles at waypoint position while active
// claude-ai:   not ACTIVE:
// claude-ai:     — EWAY_evaluate_condition() → if TRUE → EWAY_set_active()
// claude-ai:
// claude-ai: After the main loop:
// claude-ai:   EWAY_process_conversation() — advances any active scripted conversation
// claude-ai:   EWAY_process_penalties()    — shows driving-mission cone-penalty count on HUD
// claude-ai:   EWAY_process_camera()       — moves the scripted cut-scene camera
void EWAY_process()
{
    SLONG i;
    SLONG on;
    SLONG timer;

    EWAY_Way* ew;
    SLONG offset, step; //,start,end;
    SLONG eway_max = EWAY_way_upto;

    step = 1;
    offset = 0;

    //		start=1;
    //		end=eway_max;

    //		step=1;
    //		offset=0;
    //	step=2;
    //	offset=GAME_TURN&1;

    //
    // This will be set to TRUE if we draw a the count-up timer.
    //

    EWAY_count_up_visible = FALSE;

    //
    // Update the time.
    //

    EWAY_time_accurate += 80 * TICK_RATIO >> TICK_SHIFT;
    EWAY_time = EWAY_time_accurate >> 4;
    EWAY_tick = 5 * TICK_RATIO >> TICK_SHIFT;

    if (GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON)) {
        //
        // Don't process the waypoint after the game is lost or won.
        //
    } else {
        //
        // Process the waypoints.
        //

        for (i = 1 + (offset); i < eway_max; i += step) {

            ew = &EWAY_way[i];

            if (ew->flag & EWAY_FLAG_DEAD) {
                continue;
            }

            if (ew->flag & EWAY_FLAG_ACTIVE) {
                if (ew->ed.type == EWAY_DO_EMIT_STEAM) {
                    //
                    // This is a waypoint type that does stuff _while_ it
                    // is active not just when it goes active or inactive.
                    //

                    EWAY_process_emit_steam(ew);
                } else if (ew->ed.type == EWAY_DO_SHAKE_CAMERA) {
                    if (FC_cam[0].shake < 100) {
                        FC_cam[0].shake += 3;
                    }
                } else if (ew->ed.type == EWAY_DO_VISIBLE_COUNT_UP) {
                    //
                    // Increase the timer.
                    //

                    EWAY_count_up += (50 * TICK_RATIO >> TICK_SHIFT) * step; // 50 * 20 ticks per second. i.e. 1000

                    //
                    // Print the time.
                    //

                    PANEL_draw_timer(EWAY_count_up / 10, 320, 50);

                    EWAY_count_up_visible = TRUE;

                    {
                        SLONG secs = EWAY_count_up / 1000;

                        if (secs > 255) {
                            secs = 255;
                        }

                        EWAY_counter[3] = secs;
                    }
                }

                if (ew->flag & EWAY_FLAG_COUNTDOWN) {
                    //
                    // The countdown to going inactive.
                    //

                    timer = EWAY_timer[ew->timer];
                    timer -= EWAY_tick * step;

                    if (timer <= 0) {
                        timer = 0;

                        EWAY_set_inactive(ew);
                    }

                    EWAY_timer[ew->timer] = timer;
                } else {
                    //
                    // Do we have to check for going inactive?
                    //

                    switch (ew->es.type) {
                    case EWAY_STAY_ALWAYS:

                        ew->flag |= EWAY_FLAG_DEAD;

                        break;

                    case EWAY_STAY_WHILE:

                        on = EWAY_evaluate_condition(ew, &ew->ec);

                        if (!on) {
                            EWAY_set_inactive(ew);
                        }

                        break;

                    case EWAY_STAY_WHILE_TIME:

                        on = EWAY_evaluate_condition(ew, &ew->ec);

                        if (!on) {
                            //
                            // The countdown to going inactive.
                            //

                            ew->timer = ew->es.arg;
                            ew->flag |= EWAY_FLAG_COUNTDOWN;
                        }

                        break;

                        //
                        // There is no case for EWAY_STAY_TIME because ew->timer
                        // should always be set in this case.
                        //

                    default:
                        ASSERT(0);
                        break;
                    }
                }

                if (ew->ed.type == EWAY_DO_WATER_SPOUT) {
                    //
                    // While this waypoint is active, it spouts out water.
                    //

                    DIRT_new_water(ew->x + 2, ew->y, ew->z, -1, 28, 0);
                    DIRT_new_water(ew->x, ew->y, ew->z + 2, 0, 29, -1);
                    DIRT_new_water(ew->x, ew->y, ew->z - 2, 0, 28, +1);
                    DIRT_new_water(ew->x - 2, ew->y, ew->z, +1, 29, 0);
                }
            } else {
                //
                // Check for going active.
                //

                on = EWAY_evaluate_condition(ew, &ew->ec);

                if (on) {
                    EWAY_set_active(ew);
                }
            }
        }
    }

    //
    // Process any conversation that might be going on.
    //

    EWAY_process_conversation();

    //
    // Show the player the count-up timer penalties after the count_up timer has
    // gone inactive.
    //

    EWAY_process_penalties();

    //
    // Process the camera.
    //

    EWAY_process_camera();
}

void EWAY_get_position(
    SLONG waypoint,
    SLONG* world_x,
    SLONG* world_y,
    SLONG* world_z)
{
    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    *world_x = EWAY_way[waypoint].x;
    *world_y = EWAY_way[waypoint].y;
    *world_z = EWAY_way[waypoint].z;
}

UWORD EWAY_get_angle(SLONG waypoint)
{
    UWORD ans;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ans = EWAY_way[waypoint].yaw << 3;

    return ans;
}

// claude-ai: EWAY_get_person() — given a waypoint index, returns the Thing index of the entity it created.
// claude-ai: Works for CREATE_ENEMY, CREATE_PLAYER, CREATE_VEHICLE, CREATE_ANIMAL.
// claude-ai: ew->ed.arg1 holds the Thing index after the waypoint has fired; NULL if not yet created.
// claude-ai: Returns NULL (0) if waypoint is 0 or the waypoint type doesn't create a Thing.
// claude-ai: Used extensively in condition evaluation to dereference NPC references.
UWORD EWAY_get_person(SLONG waypoint)
{
    UWORD ans;

    if (waypoint == NULL) {
        return NULL;
    }

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    EWAY_Way* ew;

    ew = &EWAY_way[waypoint];

    if (ew->ed.type == EWAY_DO_CREATE_ENEMY || ew->ed.type == EWAY_DO_CREATE_PLAYER || ew->ed.type == EWAY_DO_CREATE_VEHICLE || ew->ed.type == EWAY_DO_CREATE_ANIMAL) {
        ans = ew->ed.arg1;
    } else {
        ans = NULL;
    }

    return ans;
}

// claude-ai: EWAY_find_waypoint() — searches EWAY_way[] for a waypoint matching type/colour/group.
// claude-ai: Starts from 'index' and wraps around (circular search). Returns array index or EWAY_NO_MATCH.
// claude-ai: Pass EWAY_DONT_CARE for any field to match any value.
// claude-ai: Used by EWAY_process_camera() to find the first CAMERA_TARGET waypoint in a colour/group.
// claude-ai: Also used by NPCs (via pcom.cpp) to find patrol/wander waypoints matching their colour.
SLONG EWAY_find_waypoint(
    SLONG index,
    SLONG whatdo, // or EWAY_DONT_CARE
    SLONG colour, // or EWAY_DONT_CARE
    SLONG group, // or EWAY_DONT_CARE
    UBYTE only_active)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        //		global_write6=i;
        if (index >= EWAY_way_upto) {
            index = 1;
        }

        ASSERT(WITHIN(index, 0, EWAY_way_upto - 1));

        ew = &EWAY_way[index];

        if ((colour == EWAY_DONT_CARE || colour == ew->colour) && (group == EWAY_DONT_CARE || group == ew->group) && (whatdo == EWAY_DONT_CARE || whatdo == ew->ed.type)) {
            //
            // Found a matching waypoint
            //

            if (!only_active || (ew->flag & EWAY_FLAG_ACTIVE)) {
                return index;
            }
        }

        index += 1;
    }

    return EWAY_NO_MATCH;
}

SLONG EWAY_find_waypoint_rand(
    SLONG not_this_index,
    SLONG colour,
    SLONG group,
    UBYTE only_active)
{
    SLONG i;
    SLONG ans;

    EWAY_Way* ew;

#define EWAY_FIND_RAND_MAX 32

    UWORD choice[EWAY_FIND_RAND_MAX];
    SLONG choice_upto = 0;

    //
    // Find all the suitable waypoints.
    //

    for (i = 1; i < EWAY_way_upto; i++) {
        //		global_write7=i;
        if (i == not_this_index) {
            //
            // Ignore this one.
            //

            continue;
        }

        ew = &EWAY_way[i];

        if ((colour == EWAY_DONT_CARE || colour == ew->colour) && (group == EWAY_DONT_CARE || group == ew->group)) {
            ASSERT(WITHIN(choice_upto, 0, EWAY_FIND_RAND_MAX - 1));

            //
            // Found a matching waypoint
            //

            if (!only_active || (ew->flag & EWAY_FLAG_ACTIVE)) {
                choice[choice_upto++] = i;

                if (choice_upto >= EWAY_FIND_RAND_MAX) {
                    break;
                }
            }
        }
    }

    if (choice_upto == 0) {
        ans = EWAY_NO_MATCH;
    } else {
        ans = choice[Random() % choice_upto];
    }

    return ans;
}

SLONG EWAY_find_nearest_waypoint(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG colour, // or EWAY_DONT_CARE
    SLONG group) // or EWAY_DONT_CARE
{
    SLONG i;

    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;

    SLONG best_dist = INFINITY;
    SLONG best_waypoint = EWAY_NO_MATCH;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];
        //		global_write8=i;

        if ((colour == EWAY_DONT_CARE || colour == ew->colour) && (group == EWAY_DONT_CARE || group == ew->group)) {
            //
            // Found a matching waypoint
            //

            if (ew->flag & EWAY_FLAG_ACTIVE) {
                dx = abs(ew->x - x);
                dy = abs(ew->y - y);
                dz = abs(ew->z - z);

                dist = QDIST3(dx, dy, dz);

                if (dist < best_dist) {
                    best_dist = dist;
                    best_waypoint = i;
                }
            }
        }
    }

    return best_waypoint;
}

SLONG EWAY_grab_camera(
    SLONG* cam_x,
    SLONG* cam_y,
    SLONG* cam_z,
    SLONG* cam_yaw,
    SLONG* cam_pitch,
    SLONG* cam_roll,
    SLONG* cam_lens)
{
    if (EWAY_cam_active) {
        //
        // If there is analogue control and the player is still moving
        // then we can't grab the camera because it'll confuse the
        // player control.
        //

        extern SLONG analogue;

        if (analogue && !EWAY_stop_player_moving()) {
            return FALSE;
        } else {
            *cam_x = EWAY_cam_x;
            *cam_y = EWAY_cam_y;
            *cam_z = EWAY_cam_z;
            *cam_yaw = EWAY_cam_yaw;
            *cam_pitch = EWAY_cam_pitch;
            *cam_roll = 1024 << 8;
            *cam_lens = EWAY_cam_lens;
        }
    }

    return EWAY_cam_active;
}

UBYTE EWAY_camera_warehouse()
{
    if (EWAY_cam_active) {
        if (EWAY_conv_active) {
            if (EWAY_conv_person_a) {
                ASSERT(TO_THING(EWAY_conv_person_a)->Class == CLASS_PERSON);

                return TO_THING(EWAY_conv_person_a)->Genus.Person->Ware;
            }
        } else {
            return EWAY_cam_warehouse;
        }
    }

    return NULL;
}

void EWAY_item_pickedup(SLONG waypoint)
{
    EWAY_Way* ew;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ew = &EWAY_way[waypoint];

    ASSERT(ew->ed.type == EWAY_DO_CREATE_ITEM);

    ew->flag |= EWAY_FLAG_GOTITEM;
}

SLONG EWAY_get_delay(SLONG waypoint, SLONG default_delay)
{
    EWAY_Way* ew;

    if (waypoint == 0) {
        return default_delay;
    }

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ew = &EWAY_way[waypoint];

    switch (ew->ed.type) {
    case EWAY_DO_NOTHING:
        return ew->ed.arg1 * 100;
        break;

    default:
        return default_delay;
        break;
    }
}

SLONG EWAY_is_active(SLONG waypoint)
{
    EWAY_Way* ew;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ew = &EWAY_way[waypoint];

    if (ew->flag & EWAY_FLAG_ACTIVE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

SLONG EWAY_used_person(UWORD t_index)
{
    UWORD i;
    SLONG ans = FALSE;

    EWAY_Way* ew;

    //
    // Go through EVERY WAYPOINT looking for a waypoint with a condition
    // for person used.
    //

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];
        //		global_write9=i;

        if (ew->flag & EWAY_FLAG_DEAD) {
            continue;
        }

        if (ew->ec.type == EWAY_COND_PERSON_USED) {
            //
            // Is this waypoint looking at our person?
            //

            ASSERT(WITHIN(ew->ec.arg1, 1, EWAY_way_upto - 1));

            if (EWAY_way[ew->ec.arg1].ed.type == EWAY_DO_CREATE_ENEMY) {
                EWAY_Way* ew2 = &EWAY_way[ew->ec.arg1];

                if (ew2->ed.arg1 == t_index) {
                    //
                    // Cool! Set this waypoint active.
                    //

                    EWAY_used_thing = t_index;

                    EWAY_set_active(ew);

                    EWAY_used_thing = NULL;

                    //
                    // Don't mark this person as useable any more.
                    //

                    ASSERT(TO_THING(t_index)->Class == CLASS_PERSON);

                    TO_THING(t_index)->Genus.Person->Flags &= ~FLAG_PERSON_USEABLE;

                    if (ew->ed.type == EWAY_DO_MESSAGE) {
                        ans = TRUE;
                    }
                }
            }
        }
    }

    return ans;
}

UBYTE EWAY_get_warehouse(SLONG waypoint)
{
    UBYTE ans;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ans = EWAY_way[waypoint].ware;

    return ans;
}

void EWAY_work_out_which_ones_are_in_warehouses()
{
    SLONG i;
    SLONG id;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        //		global_write10=i;
        ew = &EWAY_way[i];

        //
        // Is this waypoint in a warehouse?
        //

        if (PAP_2HI(ew->x >> 8, ew->z >> 8).Flags & PAP_FLAG_HIDDEN) {
            SLONG ware_top;

            ware_top = PAP_calc_map_height_at(
                ew->x,
                ew->z);

            if (ew->y < ware_top - 0x80) {
                ew->ware = WARE_which_contains(
                    ew->x >> 8,
                    ew->z >> 8);
            }
        }
    }
}

#define EWAY_CAM_VIEW_ANGLES 16 // Power of 2 please

void EWAY_cam_get_position_for_angle(
    Thing* p_thing,
    SLONG angle,
    SLONG* vx,
    SLONG* vy,
    SLONG* vz)
{
    SLONG dx;
    SLONG dz;

    ASSERT(WITHIN(angle, 0, EWAY_CAM_VIEW_ANGLES - 1));

    angle = angle * (2048 / EWAY_CAM_VIEW_ANGLES) + 100;
    angle &= 2047;

    dx = SIN(angle);
    dz = COS(angle);

    *vx = p_thing->WorldPos.X >> 8;
    *vy = p_thing->WorldPos.Y >> 8;
    *vz = p_thing->WorldPos.Z >> 8;

    *vx += dx >> 7;
    *vz += dz >> 7;
    *vy += 0x140;
}

void EWAY_cam_converse(Thing* p_thing, Thing* p_listener)
{
    SLONG i;
    SLONG j;

    SLONG x;
    SLONG y;
    SLONG z;

    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;

    SLONG cam_dist;

    SLONG best_angle;
    SLONG best_score;

    SLONG side1;
    SLONG side2;

    SLONG angle1;
    SLONG angle2;
    SLONG dangle;

#define EWAY_CONVERSE_ANGLES 16 // Power of 2 please.

    UBYTE score[EWAY_CONVERSE_ANGLES];

    EWAY_cam_thing = THING_NUMBER(p_thing);
    EWAY_cam_waypoint = NULL;
    EWAY_cam_lens = 0x28000;
    EWAY_cam_freeze = TRUE;
    EWAY_cam_lock = FALSE;
    EWAY_cam_goinactive = FALSE;

    //
    // If the current camera can see the listener and the thing, then
    // don't make it move needlessly.
    //

    if (EWAY_cam_active) {
        x = EWAY_cam_x;
        y = EWAY_cam_y;
        z = EWAY_cam_z;
    } else {
        x = FC_cam[0].x;
        y = FC_cam[0].y;
        z = FC_cam[0].z;
    }

    dx = abs(p_thing->WorldPos.X - x);
    dy = abs(p_thing->WorldPos.Y - y);
    dz = abs(p_thing->WorldPos.Z - z);

    dist = QDIST3(dx, dy, dz) >> 8;

    if (WITHIN(dist, 0x180, 0x700) && 0) {
        dx = abs(p_listener->WorldPos.X - x);
        dy = abs(p_listener->WorldPos.Y - y);
        dz = abs(p_listener->WorldPos.Z - z);

        dist = QDIST3(dx, dy, dz) >> 8;

        if (WITHIN(dist, 0x200, 0x700)) {
            x >>= 8;
            y >>= 8;
            z >>= 8;

            if (there_is_a_los(
                    x, y, z,
                    p_thing->WorldPos.X >> 8,
                    p_thing->WorldPos.Y + 0x6000 >> 8,
                    p_thing->WorldPos.Z >> 8,
                    LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                if (there_is_a_los(
                        x, y, z,
                        p_listener->WorldPos.X >> 8,
                        p_listener->WorldPos.Y + 0x6000 >> 8,
                        p_listener->WorldPos.Z >> 8,
                        LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                    EWAY_cam_x = x << 8;
                    EWAY_cam_y = y << 8;
                    EWAY_cam_z = z << 8;

                    EWAY_cam_active = TRUE;

                    return;
                }
            }
        }
    }

    //
    // How far the camera should be from the talking thing.
    //

    dx = p_listener->WorldPos.X - p_thing->WorldPos.X >> 8;
    dz = p_listener->WorldPos.Z - p_thing->WorldPos.Z >> 8;

    cam_dist = QDIST2(abs(dx), abs(dz));

    //
    // Use the distance between the two people... but not too extreme.
    //

    SATURATE(cam_dist, 0x180, 0x300);

    for (i = 0; i < EWAY_CONVERSE_ANGLES; i++) {
        score[i] = 0;

        dx = SIN(i * (2048 / EWAY_CONVERSE_ANGLES)) * cam_dist >> 16;
        dz = COS(i * (2048 / EWAY_CONVERSE_ANGLES)) * cam_dist >> 16;

        x = (p_thing->WorldPos.X >> 8) + dx;
        y = (p_thing->WorldPos.Y >> 8) + 0x100;
        z = (p_thing->WorldPos.Z >> 8) + dz;

        if (!there_is_a_los(
                x, y, z,
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y + 0x6000 >> 8,
                p_thing->WorldPos.Z >> 8,
                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
            //
            // The camera can't see the person who is talking.
            //

            continue;
        }

        score[i] += 40;

        if (!there_is_a_los(
                x, y, z,
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y + 0x6000 >> 8,
                p_thing->WorldPos.Z >> 8,
                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
            //
            // The camera can see the person who is listening. That
            // is a good thing.
            //

            score[i] += 25;
        }

        //
        // Is the listener obscuring the person who is talking.
        //

        dx = (p_thing->WorldPos.X >> 8) - x;
        dz = (p_thing->WorldPos.Z >> 8) - z;

        angle1 = calc_angle(dx, dz);

        dx = (p_listener->WorldPos.X >> 8) - x;
        dz = (p_listener->WorldPos.Z >> 8) - z;

        angle2 = calc_angle(dx, dz);

        dangle = angle2 - angle1;

        if (dangle < -1024) {
            dangle += 2048;
        }
        if (dangle > +1024) {
            dangle -= 2048;
        }

        if (abs(dangle) > 100) {
            //
            // The two people aren't obscuring eachother. That is
            // a good thing.
            //

            score[i] += 15;
        }

        //
        // Is the camera looking at the front of the listener?
        //

        dangle = angle1 - p_thing->Draw.Tweened->Angle;

        if (dangle < -1024) {
            dangle += 2048;
        }
        if (dangle > +1024) {
            dangle -= 2048;
        }

        if (abs(dangle) < 400) {
            //
            // We can see the speakers face. Another good thing.
            //

            score[i] += 10;
        }

        //
        // Avoid prims.
        //

        dx = (p_thing->WorldPos.X >> 8) - x >> 3;
        dy = (p_thing->WorldPos.Y >> 8) - y >> 3;
        dz = (p_thing->WorldPos.Z >> 8) - z >> 3;

        for (j = 0; j < 8; j++) {
            if (OB_inside_prim(x, y, z)) {
                score[i] -= 5;
            }

            x += dx;
            y += dy;
            z += dz;
        }
    }

    //
    // Stop the camera being too near a wall.
    //

    for (i = 0; i < EWAY_CONVERSE_ANGLES; i++) {
        side1 = (i - 1) & (EWAY_CONVERSE_ANGLES - 1);
        side2 = (i + 1) & (EWAY_CONVERSE_ANGLES - 1);

        if (side1 == 0 || side2 == 0) {
            score[i] >>= 1;
        }
    }

    best_angle = -1;
    best_score = 0;

    for (i = 0; i < EWAY_CONVERSE_ANGLES; i++) {
        if (score[i] > best_score) {
            best_score = score[i];
            best_angle = i;
        }
    }

    if (best_score == 0) {
        //
        // Emergency!
        //

        EWAY_cam_x = p_thing->WorldPos.X + p_listener->WorldPos.X >> 1;
        EWAY_cam_y = p_thing->WorldPos.Y + p_listener->WorldPos.Y >> 1;
        EWAY_cam_z = p_thing->WorldPos.Z + p_listener->WorldPos.Z >> 1;

        EWAY_cam_y += 0x30000;
    } else {
        SLONG division = 2048 / EWAY_CONVERSE_ANGLES;
        SLONG angle = best_angle * division;

        // modulate it randomly
        angle += (Random() & (division - 1)) - (division / 2);
        angle &= 2047;

        dx = SIN(angle) * cam_dist >> 8;
        dz = COS(angle) * cam_dist >> 8;

        // find a vector 0 to 50% from speaker to listener
        SLONG rnd = Random() & 127;

        x = ((p_listener->WorldPos.X >> 8) - (p_thing->WorldPos.X >> 8)) * rnd;
        y = ((p_listener->WorldPos.Y >> 8) - (p_thing->WorldPos.Y >> 8)) * rnd;
        z = ((p_listener->WorldPos.Z >> 8) - (p_thing->WorldPos.Z >> 8)) * rnd;

        // add in the origin coordinates
        x += p_thing->WorldPos.X;
        y += p_thing->WorldPos.Y;
        z += p_thing->WorldPos.Z;

        EWAY_cam_targx = x;
        EWAY_cam_targy = y + 0xA000;
        EWAY_cam_targz = z;

        // add the vector to the camera
        EWAY_cam_x = x + dx;
        EWAY_cam_y = y + 0x10000;
        EWAY_cam_z = z + dz;

        EWAY_cam_target = 0;
        EWAY_cam_thing = 0;
    }

    EWAY_cam_active = TRUE;
}

void EWAY_cam_look_at(Thing* p_thing)
{
    SLONG i;

    SLONG dx;
    SLONG dz;

    SLONG cx;
    SLONG cz;

    SLONG dprod;

    SLONG vx;
    SLONG vy;
    SLONG vz;
    SLONG score;

    SLONG best_angle;
    SLONG best_score;

    UWORD thing_yaw;
    UWORD angle;

    SLONG view[EWAY_CAM_VIEW_ANGLES];

    //
    // What direction is the thing facing in?
    //

    switch (p_thing->Class) {
    case CLASS_PERSON:
        thing_yaw = p_thing->Draw.Tweened->Angle;
        break;

    case CLASS_VEHICLE:
        thing_yaw = p_thing->Genus.Vehicle->Angle;
        break;

    default:
        ASSERT(0);
        break;
    }

    dx = -SIN(thing_yaw);
    dz = -COS(thing_yaw);

    //
    // Which angles could we view the thing from?
    //

    for (i = 0; i < EWAY_CAM_VIEW_ANGLES; i++) {
        EWAY_cam_get_position_for_angle(
            p_thing,
            i,
            &vx,
            &vy,
            &vz);

        if (there_is_a_los(
                vx, vy, vz,
                (p_thing->WorldPos.X >> 8),
                (p_thing->WorldPos.Y >> 8) + 0x80,
                (p_thing->WorldPos.Z >> 8),
                0)) {
            cx = vx - (p_thing->WorldPos.X >> 8);
            cz = vz - (p_thing->WorldPos.Z >> 8);

            dprod = dx * cx + dz * cz;

            if (dprod < 1000) {
                dprod = 1000;
            }

            view[i] = dprod;
        } else {
            view[i] = FALSE;
        }
    }

    //
    // Pick the best view.
    //

    best_score = 0;

    ULONG seed = p_thing->WorldPos.X ^ p_thing->WorldPos.Z;

    for (i = 0; i < EWAY_CAM_VIEW_ANGLES; i++) {
        seed *= 69069;
        seed += 1;

        if (!view[i]) {
            continue;
        }

        score = seed & 0x3ffff;
        score += 1; // Incase (seed & 0xfff) is zero. It could happen!
        score += view[i];

        if (view[(i - 1) & (EWAY_CAM_VIEW_ANGLES - 1)]) {
            score += seed & 0x3ffff;
        }
        if (view[(i + 1) & (EWAY_CAM_VIEW_ANGLES - 1)]) {
            score += seed & 0x3ffff;
        }

        if (score > best_score) {
            EWAY_cam_get_position_for_angle(
                p_thing,
                i,
                &vx,
                &vy,
                &vz);

            best_score = score;
            best_angle = i;
        }
    }

    if (best_score) {
        EWAY_cam_get_position_for_angle(
            p_thing,
            best_angle,
            &EWAY_cam_x,
            &EWAY_cam_y,
            &EWAY_cam_z);
    } else {
        //
        // Find a default position to view the thing from.
        //

        EWAY_cam_x = p_thing->WorldPos.X >> 8;
        EWAY_cam_y = p_thing->WorldPos.Y >> 8;
        EWAY_cam_z = p_thing->WorldPos.Z >> 8;

        EWAY_cam_x -= dx >> 8;
        EWAY_cam_y += 0x140;
        EWAY_cam_z -= dz >> 8;
    }

    EWAY_cam_active = TRUE;
    EWAY_cam_thing = THING_NUMBER(p_thing);
    EWAY_cam_waypoint = NULL;
    EWAY_cam_freeze = TRUE;

    EWAY_cam_x <<= 8;
    EWAY_cam_y <<= 8;
    EWAY_cam_z <<= 8;
}

void EWAY_cam_relinquish()
{
    EWAY_cam_goinactive = 2;

    /*

    EWAY_cam_jumped=1;
    EWAY_cam_active   = FALSE;
    EWAY_cam_thing    = NULL;
    EWAY_cam_waypoint = NULL;
    EWAY_cam_freeze   = FALSE;

    */
}

SLONG EWAY_find_or_create_waypoint_that_created_person(Thing* p_person)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        //		global_write11=i;
        ew = &EWAY_way[i];

        if (EWAY_get_person(i) == THING_NUMBER(p_person)) {
            return i;
        }
    }

    ASSERT(WITHIN(EWAY_way_upto, 1, EWAY_MAX_WAYS - 1));

    ew = &EWAY_way[EWAY_way_upto];

    ew->flag = EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD;
    ew->ed.type = EWAY_DO_CREATE_ENEMY;
    ew->ed.arg1 = THING_NUMBER(p_person);

    return EWAY_way_upto++;
}

SLONG EWAY_conversation_happening(
    THING_INDEX* person_a,
    THING_INDEX* person_b)
{
    if (EWAY_conv_active) {
        *person_a = EWAY_conv_person_a;
        *person_b = EWAY_conv_person_b;

        return TRUE;
    } else {
        return FALSE;
    }
}

void EWAY_prim_activated(SLONG ob_index)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        //		global_write12=i;
        ew = &EWAY_way[i];

        if (ew->ec.type == EWAY_COND_PRIM_ACTIVATED && ew->ec.arg1 == ob_index) {
            ew->flag |= EWAY_FLAG_TRIGGERED;

            return;
        }
    }
}

void EWAY_deduct_time_penalty(SLONG time_to_deduct_in_hundreths_of_a_second)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 0; i < EWAY_MAX_WAYS; i++) {
        ew = &EWAY_way[i];

        if (ew->ec.type == EWAY_COND_COUNTDOWN_SEE) {
            SLONG depend = ew->ec.arg1;
            SLONG timer = ew->ec.arg2;

            ASSERT(depend == 0 || WITHIN(depend, 1, EWAY_way_upto - 1));

            if (!depend || (EWAY_way[depend].flag & EWAY_FLAG_ACTIVE)) {
                if (ew->ec.arg2 < time_to_deduct_in_hundreths_of_a_second) {
                    ew->ec.arg2 = 0;
                } else {
                    ew->ec.arg2 -= time_to_deduct_in_hundreths_of_a_second;
                }
            }
        }
    }
}
