#include "ai/combat.h"

#include "fallen/Headers/pcom.h"       // Temporary: PCOM_*, PCOM_AI_*
#include "ui/hud/overlay.h"            // track_enemy (already migrated)
#include "engine/audio/mfx.h"         // MFX_play_thing, MFX_play_xyz, MFX_stop (already migrated)
#include "engine/audio/sound.h"       // PainSound (already migrated)
#include "engine/effects/psystem.h"   // PARTICLE_Add, PFLAG_* (already migrated)
#include "engine/graphics/pipeline/poly.h" // POLY_PAGE_SMOKECLOUD2 (already migrated)
#include "actors/characters/anim_ids.h"
#include "fallen/Headers/statedef.h"  // Temporary: ANIM_*, SUB_STATE_*, STATE_*, ACTION_*
#include "fallen/Headers/pap.h"       // Temporary: PAP_calc_height_at_thing, PAP_SIZE_HI, PAP_SHIFT_HI
#include "fallen/Headers/Sound.h"     // Temporary: SOUND_Range, SOUND_Gender, S_*
#include "assets/anim_globals.h"      // estate, semtex (already migrated)
#include "fallen/Headers/dirt.h"      // Temporary: DIRT_new_water, DIRT_TYPE_BLOOD
#include "missions/eway.h"            // EWAY_get_person (already migrated)

// Functions not yet in any header: declared inline here as in the original.
// uc_orig: set_face_thing (fallen/Source/Person.cpp)
extern SLONG set_face_thing(Thing* p_person, Thing* p_target);
// uc_orig: e_draw_3d_line (fallen/DDEngine/Source/aeng.cpp)
extern void e_draw_3d_line(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);
// uc_orig: e_draw_3d_line_col (fallen/DDEngine/Source/aeng.cpp)
extern void e_draw_3d_line_col(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG r, SLONG g, SLONG b);
// uc_orig: check_combat_hit_with_person (fallen/Source/Combat.cpp)
SLONG check_combat_hit_with_person(Thing* p_victim, SLONG x, SLONG y, SLONG z, struct GameFightCol* fight, Thing* p_agressor, SLONG* ret_angle);
// uc_orig: check_combat_grapple_with_person (fallen/Source/Combat.cpp)
SLONG check_combat_grapple_with_person(Thing* p_victim, MAPCO16 x, MAPCO16 y, MAPCO16 z, struct GameFightCol* fight, Thing* p_agressor, SLONG* ret_angle, SLONG grapple);
// uc_orig: reset_gang_attack (fallen/Source/pcom.cpp)
extern void reset_gang_attack(Thing* p_target);
// uc_orig: scare_gang_attack (fallen/Source/pcom.cpp)
extern void scare_gang_attack(Thing* p_target);
// uc_orig: person_has_gun_out (fallen/Source/Person.cpp)
extern SLONG person_has_gun_out(Thing* p_person);
// uc_orig: drop_current_gun (fallen/Source/Person.cpp)
extern void drop_current_gun(Thing* p_person, SLONG change_anim);
// uc_orig: is_person_dead (fallen/Source/Person.cpp)
extern SLONG is_person_dead(Thing* p_person);
// uc_orig: is_person_ko (fallen/Source/Person.cpp)
extern SLONG is_person_ko(Thing* p_person);
// uc_orig: set_person_kick_near (fallen/Source/Person.cpp)
extern SLONG set_person_kick_near(Thing* p_person, SLONG dist);
// uc_orig: dist_to_target (fallen/Source/Person.cpp)
extern SLONG dist_to_target(Thing* p_person_a, Thing* p_person_b);
// uc_orig: is_person_ko_and_lay_down (fallen/Source/Person.cpp)
extern SLONG is_person_ko_and_lay_down(Thing* p_person);
// uc_orig: person_on_floor (fallen/Source/Person.cpp)
extern SLONG person_on_floor(Thing* p_person);
// uc_orig: is_there_room_behind_person (fallen/Source/Person.cpp)
extern SLONG is_there_room_behind_person(Thing* p_person, SLONG hit_from_behind);
// uc_orig: set_person_stomp (fallen/Source/Person.cpp)
extern SLONG set_person_stomp(Thing* p_person);
// uc_orig: move_thing_on_map (fallen/Source/Person.cpp)
extern void move_thing_on_map(Thing* p_thing, GameCoord* new_position);
// uc_orig: set_person_dead (fallen/Source/Person.cpp)
extern void set_person_dead(Thing* p_thing, Thing* p_aggressor, SLONG death_type, SLONG behind, SLONG height);
// set_person_recoil declared in Person.h (UBYTE flags param)
// uc_orig: person_is_lying_on_what (fallen/Source/Person.cpp)
extern SLONG person_is_lying_on_what(Thing* p_person);
// uc_orig: there_is_a_los_things (fallen/Source/collide.cpp)
extern SLONG there_is_a_los_things(Thing* p_a, Thing* p_b, SLONG flags);
// uc_orig: PersonIsMIB (fallen/Source/pcom.cpp)
// return type is BOOL (not SLONG) — matches pcom.h declaration
extern BOOL PersonIsMIB(Thing* p_person);
// uc_orig: check_eway_talk (fallen/Source/eway.cpp) -- also in eway.h already
extern void check_eway_talk(SLONG stop);
// uc_orig: sweep_feet (fallen/Source/Person.cpp)
extern void sweep_feet(Thing* p_person, Thing* p_aggressor, SLONG death_type);
// uc_orig: person_enter_fight_mode (fallen/Source/Person.cpp)
extern void person_enter_fight_mode(Thing* p_person);
// uc_orig: really_on_floor (fallen/Source/Person.cpp)
extern SLONG really_on_floor(Thing* p_person);
// uc_orig: is_there_room_in_front_of_me (fallen/Source/Person.cpp)
extern SLONG is_there_room_in_front_of_me(Thing* p_person, SLONG how_much_room);
// uc_orig: emergency_uncarry (fallen/Source/Person.cpp)
extern void emergency_uncarry(Thing* p_person);
// uc_orig: PCOM_new_gang_attack (fallen/Source/pcom.cpp)
extern void PCOM_new_gang_attack(Thing* p_person, Thing* p_target);
// uc_orig: PCOM_attack_happened_but_missed (fallen/Source/pcom.cpp)
extern void PCOM_attack_happened_but_missed(Thing* p_victim, Thing* p_agressor);
// uc_orig: PCOM_attack_happened (fallen/Source/pcom.cpp)
extern void PCOM_attack_happened(Thing* p_thing, Thing* p_aggressor);
// uc_orig: calc_angle (fallen/Source/Person.cpp)
extern SLONG calc_angle(SLONG dx, SLONG dz);
// uc_orig: angle_diff (fallen/Source/Person.cpp)
extern SLONG angle_diff(SLONG a, SLONG b);
// uc_orig: set_person_kick (fallen/Source/Person.cpp)
extern SLONG set_person_kick(Thing* p_person);
// uc_orig: set_person_punch (fallen/Source/Person.cpp)
extern SLONG set_person_punch(Thing* p_person);

// Half-width of the cone (0-2047 system) in which a punch/kick can connect.
// Value 400 = ~70 degree total arc. Wide by design for playability.
// uc_orig: FIGHT_ANGLE_RANGE (fallen/Source/Combat.cpp)
#define FIGHT_ANGLE_RANGE (400)

// Maximum people to consider when finding a combat target (sphere query cap).
// uc_orig: STANCE_MAX_FIND (fallen/Source/Combat.cpp)
#define STANCE_MAX_FIND 8

// Search radius in 8-bits-per-mapsquare units (~1.5 map squares).
// uc_orig: STANCE_RADIUS (fallen/Source/Combat.cpp)
#define STANCE_RADIUS 0x200

// uc_orig: get_anim_and_node_for_action (fallen/Source/Combat.cpp)
SLONG get_anim_and_node_for_action(UBYTE current_node, UBYTE action, UWORD* new_anim)
{
    SLONG new_node;
    new_node = fight_tree[current_node][action];
    *new_anim = fight_tree[new_node][0];
    return (new_node);
}

// uc_orig: get_combat_type_for_node (fallen/Source/Combat.cpp)
SLONG get_combat_type_for_node(UBYTE current_node)
{
    return (fight_tree[current_node][9]);
}

// uc_orig: init_gangattack (fallen/Source/Combat.cpp)
void init_gangattack(void)
{
    memset((UBYTE*)gang_attacks, 0, sizeof(GangAttack) * MAX_HISTORY);
}

// uc_orig: get_gangattack (fallen/Source/Combat.cpp)
SLONG get_gangattack(Thing* p_person)
{
    UWORD c0;
    SLONG oldest = 0, best = 0;
    for (c0 = 1; c0 < MAX_HISTORY; c0++) {
        SLONG ticks;

        ticks = ((UWORD)GAME_TURN) - gang_attacks[c0].LastUsed;

        if (ticks > oldest) {
            oldest = ticks;
            best = c0;
        }
        if (gang_attacks[c0].Owner == 0) {
            p_person->Genus.Person->GangAttack = c0;
            gang_attacks[c0].Owner = THING_NUMBER(p_person);
            gang_attacks[c0].Index = 0;
            gang_attacks[c0].Count = 0;
            gang_attacks[c0].LastUsed = (UWORD)GAME_TURN;
            memset(&gang_attacks[c0].Perp[0], 0, 16);

            return (c0);
        }
    }

    if (best) {
        Thing* p_owner;

        p_owner = TO_THING(gang_attacks[best].Owner);
        p_owner->Genus.Person->GangAttack = 0;
        p_person->Genus.Person->GangAttack = best;
        gang_attacks[best].Owner = THING_NUMBER(p_person);
        memset(&gang_attacks[best].Perp[0], 0, 16);
        gang_attacks[best].Index = 0;
        gang_attacks[best].Count = 0;
        gang_attacks[best].LastUsed = (UWORD)GAME_TURN;
    }
    ASSERT(best);
    return (best);
}

// Tests each person within radius 0x280 of (x,y,z) for a melee hit.
// Returns sum of (Damage+5) for all people hit. Sets *p_victim to the last hit.
// uc_orig: find_possible_combat_target (fallen/Source/Combat.cpp)
SLONG find_possible_combat_target(struct GameFightCol* p_fight, SLONG x, SLONG y, SLONG z, Thing* p_thing, Thing** p_victim)
{
    SLONG i;
    SLONG hit;
    SLONG angle;
    Thing* t_thing;
    SLONG found_upto;

    found_upto = THING_find_sphere(x, y, z, 0x280, found, 16, (1 << CLASS_PERSON));

    hit = 0;

    for (i = 0; i < found_upto; i++) {
        t_thing = TO_THING(found[i]);

        if (t_thing != p_thing) {
            if (check_combat_hit_with_person(
                    t_thing,
                    x, y, z,
                    p_fight,
                    p_thing,
                    &angle)
                > 0) {
                hit += p_fight->Damage + 5;
                *p_victim = t_thing;
            }
        }
    }

    return hit;
}

// Tests each person within radius 0x280 of (x,y,z) for a grapple hit.
// Additional filters: victim must not already be grappled, dead, or KO'd.
// Returns 1000 per valid grapple target (large value ensures grapple beats normal hit in scoring).
// uc_orig: find_possible_grapple_target (fallen/Source/Combat.cpp)
SLONG find_possible_grapple_target(struct GameFightCol* p_fight, SLONG x, SLONG y, SLONG z, Thing* p_thing, Thing** p_victim, SLONG grapple_idx)
{
    SLONG i;
    SLONG hit;
    SLONG angle;
    Thing* t_thing;
    SLONG found_upto;

    found_upto = THING_find_sphere(x, y, z, 0x280, found, 16, (1 << CLASS_PERSON));

    hit = 0;

    for (i = 0; i < found_upto; i++) {
        t_thing = TO_THING(found[i]);
        if (p_thing != t_thing)
            if (people_allowed_to_hit_each_other(t_thing, p_thing)) {
                if (t_thing != p_thing && t_thing->SubState != SUB_STATE_GRAPPLE_HOLD && t_thing->SubState != SUB_STATE_GRAPPLE_HELD && t_thing->SubState != SUB_STATE_GRAPPLE_ATTACK && t_thing->SubState != SUB_STATE_ESCAPE && t_thing->State != STATE_DEAD && !is_person_ko(t_thing) && t_thing->Draw.Tweened->CurrentAnim != ANIM_PISTOL_WHIP_TAKE) {
                    if (check_combat_grapple_with_person(
                            t_thing,
                            x, y, z,
                            p_fight,
                            p_thing,
                            &angle, grapple_idx)
                        > 0) {
                        hit += 1000;
                        *p_victim = t_thing;
                    }
                }
            }
    }

    return hit;
}

// Walks all keyframes of 'anim', sums hit scores from find_possible_combat_target per frame.
// Uses the pelvis sub-object position as the attack origin.
// uc_orig: find_hit_value (fallen/Source/Combat.cpp)
SLONG find_hit_value(Thing* p_person, SLONG anim, Thing** p_victim)
{
    GameKeyFrame* current;
    struct GameFightCol* fight_info;
    SLONG x, y, z;
    SLONG dx, dy, dz;
    SLONG hits = 0;

    current = global_anim_array[p_person->Genus.Person->AnimType][anim];

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, 0, &x, &y, &z);
    x += p_person->WorldPos.X >> 8;
    y += p_person->WorldPos.Y >> 8;
    z += p_person->WorldPos.Z >> 8;

    while (current) {
        if (current->Fight) {
            hits += find_possible_combat_target(current->Fight, x, y, z, p_person, p_victim);
        }
        current = current->NextFrame;
    }
    return (hits);
}

// Walks all keyframes of 'anim', sums grapple scores from find_possible_grapple_target.
// Uses the person's WorldPos directly (not pelvis sub-object).
// uc_orig: find_grapple_value (fallen/Source/Combat.cpp)
SLONG find_grapple_value(Thing* p_person, SLONG anim, Thing** p_victim, SLONG grapple_idx)
{
    GameKeyFrame* current;
    struct GameFightCol* fight_info;
    SLONG x, y, z;
    SLONG dx, dy, dz;
    SLONG hits = 0;

    current = global_anim_array[p_person->Genus.Person->AnimType][anim];

    x = p_person->WorldPos.X >> 8;
    y = p_person->WorldPos.Y >> 8;
    z = p_person->WorldPos.Z >> 8;

    hits += find_possible_grapple_target(0, x, y, z, p_person, p_victim, grapple_idx);
    return (hits);
}

// Physically positions the victim at 'dist' units in front of the attacker,
// and sets the victim's facing angle to match the grapple type.
// uc_orig: set_grapple_pos (fallen/Source/Combat.cpp)
void set_grapple_pos(Thing* p_person, Thing* p_victim, SLONG dist, SLONG anim, SLONG grapple_idx)
{
    SLONG dx, dy, dz, len;
    SLONG angle;
    GameCoord new_position;

    dx = -(SIN(p_person->Draw.Tweened->Angle) * dist) >> 16;
    dz = -(COS(p_person->Draw.Tweened->Angle) * dist) >> 16;

    new_position.X = p_person->WorldPos.X + (dx << 8);
    new_position.Y = p_person->WorldPos.Y;
    new_position.Z = p_person->WorldPos.Z + (dz << 8);

    move_thing_on_map(p_victim, &new_position);

    p_victim->Draw.Tweened->Angle = (p_person->Draw.Tweened->Angle + grapples[grapple_idx].Angle + 1024) & 2047;
}

// Initiates a grapple between attacker and victim: sets animations, SubState fields,
// positions the victim, scares nearby gang attackers, and plays a taunt sound.
// Special cases: SNAP_KNECK kills instantly; GRAB_ARM holds victim.
// uc_orig: set_grapple (fallen/Source/Combat.cpp)
SLONG set_grapple(Thing* p_person, Thing* p_victim, SLONG anim, SLONG grapple_idx)
{
    UBYTE taunt_prob = 0;
    set_face_thing(p_person, p_victim);
    set_anim(p_person, anim);
    p_person->Genus.Person->Target = THING_NUMBER(p_victim);
    ASSERT(p_person->Genus.Person->Target != THING_NUMBER(p_person));
    ASSERT(TO_THING(p_person->Genus.Person->Target)->Class == CLASS_PERSON);

    p_victim->Genus.Person->Target = THING_NUMBER(p_person);
    ASSERT(p_victim->Genus.Person->Target != THING_NUMBER(p_victim));
    ASSERT(TO_THING(p_victim->Genus.Person->Target)->Class == CLASS_PERSON);

    switch (anim) {
    case ANIM_SNAP_KNECK:
        set_anim(p_victim, ANIM_DIE_KNECK);
        set_grapple_pos(p_person, p_victim, 70, anim, grapple_idx);
        p_victim->SubState = SUB_STATE_GRAPPLEE;
        p_person->SubState = SUB_STATE_GRAPPLE;
        break;
    case ANIM_PISTOL_WHIP:
        set_anim(p_victim, ANIM_PISTOL_WHIP_TAKE);
        set_grapple_pos(p_person, p_victim, 70, anim, grapple_idx);
        p_victim->SubState = SUB_STATE_GRAPPLEE;
        p_person->SubState = SUB_STATE_GRAPPLE;
        break;
    case ANIM_GRAB_ARM:
        set_grapple_pos(p_person, p_victim, 90, anim, grapple_idx);
        set_anim(p_victim, ANIM_GRAB_ARMV);
        p_victim->Genus.Person->Action = ACTION_GRAPPLEE;
        p_victim->Genus.Person->Escape = 0;
        p_victim->SubState = SUB_STATE_GRAPPLE_HELD;
        p_person->SubState = SUB_STATE_GRAPPLE;
        break;
    case ANIM_STRANGLE:
        set_grapple_pos(p_person, p_victim, 100, anim, grapple_idx);
        set_anim(p_victim, ANIM_STRANGLE_VICTIM);
        p_victim->Genus.Person->Action = ACTION_GRAPPLEE;
        p_victim->Genus.Person->Escape = 0;
        p_victim->SubState = SUB_STATE_STRANGLEV;
        p_person->SubState = SUB_STATE_STRANGLE;
        taunt_prob = 50;
        break;
    case ANIM_HEADBUTT:
        set_grapple_pos(p_person, p_victim, 100, anim, grapple_idx);
        set_anim(p_victim, ANIM_HEADBUTT_VICTIM);
        p_victim->Genus.Person->Action = ACTION_GRAPPLEE;
        p_victim->Genus.Person->Escape = 0;
        p_victim->SubState = SUB_STATE_HEADBUTTV;
        p_person->SubState = SUB_STATE_HEADBUTT;
        taunt_prob = 50;
        break;
    }
    set_generic_person_state_function(p_person, STATE_FIGHTING);
    set_generic_person_state_function(p_victim, STATE_FIGHTING);
    p_victim->Genus.Person->Flags |= FLAG_PERSON_HELPLESS;
    p_person->Genus.Person->Action = ACTION_GRAPPLE;
    scare_gang_attack(p_person);

    if ((Random() & 0xff) < taunt_prob) {
        SLONG sound = 0;
        switch (p_person->Genus.Person->PersonType) {
        case PERSON_ROPER:
            if (Random() & 2)
                sound = SOUND_Range(S_ROPER_FIGHT_TAUNT_START, S_ROPER_FIGHT_TAUNT_END);
            else
                sound = SOUND_Range(S_ROPER_DEADSAFE_TAUNT_START, S_ROPER_DEADSAFE_TAUNT_END);
            break;
        }
        if (sound)
            MFX_play_thing(THING_NUMBER(p_person), sound, MFX_QUEUED | MFX_SHORT_QUEUE, p_person);
    }

    return (0);
}

// Scans grapples[] table for the best move that would connect with a nearby target.
// iam=1 for Darci, iam=2 for Roper; controls which grapples each character can perform.
// Skips MIB characters (immune to grapples) and avoids grappling near walls.
// uc_orig: find_best_grapple (fallen/Source/Combat.cpp)
SLONG find_best_grapple(Thing* p_person)
{
    SLONG c0 = 0;
    SLONG best = -1, best_hit = 0, hit;
    Thing *p_target = 0, *best_target = 0;
    SLONG only_kneck = 0;
    SLONG iam = 1;
    if (p_person->Genus.Person->AnimType == ANIM_TYPE_ROPER) {
        iam = 2;
    }

    if (!p_person->Genus.Person->PlayerID || p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        // Skip neck snap for non-players
        c0 = 1;
    }
    if ((p_person->Genus.Person->PlayerID && p_person->SubState == SUB_STATE_STEP_FORWARD && p_person->Draw.Tweened->FrameIndex < 3) || ((!p_person->Genus.Person->PlayerID) && ((GAME_TURN + THING_NUMBER(p_person)) & 0x3) == 0)) {
        only_kneck = 0;
    } else {
        only_kneck = 1;
        if (c0 == 1) {
            // Neck snap has been skipped; nothing else to try
            return (0);
        }
    }

    while (grapples[c0].Anim) {
        if (grapples[c0].Peep & iam) {
            hit = find_grapple_value(p_person, grapples[c0].Anim, &p_target, c0);
            if (hit > best_hit) {
                best = c0;
                best_hit = hit;
                best_target = p_target;
            }
        }
        c0++;
        if (only_kneck && c0 == 1) {
            break;
        }
    }

    if (best >= 0) {
        if (best_target && (best_target->Genus.Person->AnimType != ANIM_TYPE_ROPER)) {
            if (best_target->Genus.Person->PersonType == PERSON_MIB1 || best_target->Genus.Person->PersonType == PERSON_MIB2 || best_target->Genus.Person->PersonType == PERSON_MIB3) {
                return UC_FALSE;
            }

            if (grapples[best].Anim == ANIM_PISTOL_WHIP) {
                if (best_target->Genus.Person->pcom_ai == PCOM_AI_FIGHT_TEST) {
                    return UC_FALSE;
                }
            }

            if (grapples[best].Anim == ANIM_STRANGLE || grapples[best].Anim == ANIM_HEADBUTT) {
                // These grapples throw the victim forward; avoid near walls.
                if (!is_there_room_in_front_of_me(p_person, 150)) {
                    return UC_FALSE;
                }
            }

            set_grapple(p_person, best_target, grapples[best].Anim, best);
            return (1);
        }
        return (0);
    } else
        return (0);
}

// Walks keyframes of 'anim' and returns the Height field of the first GameFightCol.
// Height: 0=low, 1=mid, 2=high. Used by should_i_block() to classify the attack.
// uc_orig: find_anim_fight_height (fallen/Source/Combat.cpp)
SLONG find_anim_fight_height(SLONG anim, SLONG person)
{
    struct GameKeyFrame* f;

    f = global_anim_array[person][anim];
    while (f) {
        if (f->Fight) {
            return (f->Fight->Height);
        }
        if (f->Flags & ANIM_FLAG_LAST_FRAME)
            return (0);
        f = f->NextFrame;
    }
    return (0);
}

// Decides if p_person will attempt a block against the incoming attack.
// Players auto-block only when stepping back (ANIM_FIGHT_STEP_S).
// AI: base probability 60 + 12*Skill; halved if attacker not visible.
// uc_orig: should_i_block (fallen/Source/Combat.cpp)
SLONG should_i_block(Thing* p_person, Thing* p_agressor, SLONG anim)
{
    SLONG pos;
    SLONG history;
    SLONG count;
    SLONG perp;
    SLONG block_prob = (20 * 256) / 100;
    SLONG fheight;
    SLONG same = 0, other = 0;
    SLONG can_see = 0;

    if (p_person->Genus.Person->PlayerID) {
        if (p_person->SubState == SUB_STATE_STEP_FORWARD) {
            if (p_person->Draw.Tweened->CurrentAnim == ANIM_FIGHT_STEP_S) {
                return (1);
            }
        }
        return (0);
    }

    fheight = find_anim_fight_height(anim, p_agressor->Genus.Person->AnimType);
    perp = THING_NUMBER(p_agressor);

    block_prob = 60 + GET_SKILL(p_person) * 12;

    if (!can_a_see_b(p_person, p_agressor)) {
        block_prob >>= 1;
    }

    if (block_prob > 150 + GET_SKILL(p_person) * 5)
        block_prob = 150 + GET_SKILL(p_person) * 5;

    {
        CBYTE str[100];
        sprintf(str, "block prob %d    same %d other %d \n", (block_prob * 100) >> 8, same, other);
    }

    if ((rand() & 255) < block_prob)
        return (1);
    else
        return (0);
}

// Draws the current fight-zone arc in world space (debug visualization).
// uc_orig: show_fight_range (fallen/Source/Combat.cpp)
void show_fight_range(Thing* p_thing)
{
    SLONG temp_angle, temp_angle2;
    SLONG x, z;
    struct GameFightCol* fight;
    SLONG dist;

    x = p_thing->WorldPos.X >> 8;
    z = p_thing->WorldPos.Z >> 8;

    fight = p_thing->Draw.Tweened->CurrentFrame->Fight;
    temp_angle = +(fight->Angle << 3) - p_thing->Draw.Tweened->Angle;
    temp_angle -= (FIGHT_ANGLE_RANGE >> 1);
    if (temp_angle < 0)
        temp_angle = 2048 + temp_angle;
    temp_angle = temp_angle & 2047;

    e_draw_3d_line(x, 0, z, x + (COS(temp_angle) >> 8), 0, z + (SIN(temp_angle) >> 8));

    temp_angle2 = temp_angle + FIGHT_ANGLE_RANGE;
    temp_angle2 = temp_angle2 & 2047;
    e_draw_3d_line(x, 0, z, x + (COS(temp_angle2) >> 8), 0, z + (SIN(temp_angle2) >> 8));
}

// Core melee hit detection: tests if the attack at (x,y,z) hits p_victim.
// Steps: 1) skip if KO'd (unless stomping) or dead. 2) compute 3D offset to victim pelvis.
// 3) reject if Y diff >100. 4) stomp uses right-foot position. 5) distance + angle cone check.
// FIGHT_ANGLE_RANGE=400 (~70 degrees). Near misses trigger PCOM_attack_happened_but_missed.
// uc_orig: check_combat_hit_with_person (fallen/Source/Combat.cpp)
SLONG check_combat_hit_with_person(Thing* p_victim, MAPCO16 x, MAPCO16 y, MAPCO16 z, struct GameFightCol* fight, Thing* p_agressor, SLONG* ret_angle)
{
    SLONG dist, dx, dy, dz, adx, adz;
    SLONG mx, my, mz;
    SLONG victim_add_y = 0;

    if (p_agressor->Draw.Tweened->CurrentAnim == ANIM_FIGHT_STOMP) {
        if (!is_person_ko_and_lay_down(p_victim))
            return (0);
        victim_add_y = 100;
    }
    if (p_victim->Genus.Person->Flags & FLAG_PERSON_KO) {
        if (p_agressor->Draw.Tweened->CurrentAnim != ANIM_FIGHT_STOMP) {
            return (0);
        }
    } else if (is_person_dead(p_victim))
        return (0);

    calc_sub_objects_position(p_victim, p_victim->Draw.Tweened->AnimTween, 0, &mx, &my, &mz);

    mx += p_victim->WorldPos.X >> 8;
    my += p_victim->WorldPos.Y >> 8;
    mz += p_victim->WorldPos.Z >> 8;

    dx = mx - x;
    dy = my - y;
    dz = mz - z;

    adx = abs(dx);
    adz = abs(dz);

    if (abs(dy + victim_add_y) > 100) {
        return (0); // Height mismatch
    }

    if (p_agressor->Draw.Tweened->CurrentAnim == ANIM_FIGHT_STOMP) {
        // Special stomp hit: test right-foot position against victim body parts.
        SLONG fx, fy, fz;
        SLONG pass = 0;

        calc_sub_objects_position(
            p_agressor,
            p_agressor->Draw.Tweened->AnimTween,
            SUB_OBJECT_RIGHT_FOOT,
            &fx, &fy, &fz);

        fx += p_agressor->WorldPos.X >> 8;
        fy += p_agressor->WorldPos.Y >> 8;
        fz += p_agressor->WorldPos.Z >> 8;

        for (pass = 0; pass < 3; pass++) {
            if (pass > 0) {
                SLONG ob = SUB_OBJECT_HEAD;
                if (pass == 2)
                    ob = SUB_OBJECT_RIGHT_TIBIA;

                calc_sub_objects_position(p_victim, p_victim->Draw.Tweened->AnimTween, ob, &mx, &my, &mz);

                mx += p_victim->WorldPos.X >> 8;
                my += p_victim->WorldPos.Y >> 8;
                mz += p_victim->WorldPos.Z >> 8;
            }

            dx = abs(mx - fx);
            dz = abs(mz - fz);

            dist = QDIST2(dx, dz);

            if (dist < 0x53) {
                return UC_TRUE;
            }
        }
        return UC_FALSE;
    }

    dist = QDIST2(adx, adz);

// Tolerance added to Dist2 before distance is considered too far.
// uc_orig: COMBAT_HIT_DIST_LEEWAY (fallen/Source/Combat.cpp)
#define COMBAT_HIT_DIST_LEEWAY 0x30

    if (WITHIN(dist, 0, fight->Dist2 + COMBAT_HIT_DIST_LEEWAY + 128)) {
        SLONG angle, temp_angle;

        if (fight->Dist1 < 40) {
            // Intersecting the enemy
            if (dist < 35)
                return (1);
        }

        angle = -Arctan(-dx, dz) - 512;
        if (angle < 0)
            angle = 2048 + angle;
        angle = angle & 2047;

        *ret_angle = angle;

        angle -= ((fight->Angle << 3) - p_agressor->Draw.Tweened->Angle);

        if (angle < 0)
            angle = 2048 + angle;
        angle = angle & 2047;

        if (angle < (FIGHT_ANGLE_RANGE >> 1) || angle > 2048 - (FIGHT_ANGLE_RANGE >> 1)) {
            if (WITHIN(dist, 0, fight->Dist2 + COMBAT_HIT_DIST_LEEWAY)) {
                return (1);
            } else {
                PCOM_attack_happened_but_missed(p_victim, p_agressor);
                return (0);
            }
        } else {
            if (angle < (FIGHT_ANGLE_RANGE >> 0) || angle > 2048 - (FIGHT_ANGLE_RANGE >> 0)) {
                PCOM_attack_happened_but_missed(p_victim, p_agressor);
            }
            return (0);
        }
    } else {
        return (0);
    }
}

// Grapple-range version of check_combat_hit_with_person.
// Uses grapples[grapple_idx].Dist/Range instead of fight zone distances.
// Angle check compares victim's facing vs attacker's facing (not attack origin).
// Y tolerance is 50 (tighter than melee's 100 -- grapples require same floor level).
// uc_orig: check_combat_grapple_with_person (fallen/Source/Combat.cpp)
SLONG check_combat_grapple_with_person(Thing* p_victim, MAPCO16 x, MAPCO16 y, MAPCO16 z, struct GameFightCol* fight, Thing* p_agressor, SLONG* ret_angle, SLONG grapple_idx)
{
    SLONG dist, dx, dy, dz, adx, adz;
    SLONG mx, my, mz;
    struct Grapples* pg;

    pg = &grapples[grapple_idx];

    mx = p_victim->WorldPos.X >> 8;
    my = p_victim->WorldPos.Y >> 8;
    mz = p_victim->WorldPos.Z >> 8;

    dx = mx - x;
    dy = my - y;
    dz = mz - z;

    adx = abs(dx);
    adz = abs(dz);

    if (abs(dy) > 50)
        return (0); // Height mismatch

    dist = QDIST2(adx, adz);

    if (abs(dist - pg->Dist) < pg->Range) {
        SLONG angle, temp_angle;

        // Angle between victim's and attacker's facing directions
        angle = ((p_victim->Draw.Tweened->Angle + 2048) & 2047) - ((p_agressor->Draw.Tweened->Angle + 2048) & 2047);
        angle = (angle + 2048) & 2047;

        angle += (pg->Angle - 1024) & 2047;

        if (angle > 160 && angle < 2048 - 160)
            return (0);

        angle = Arctan(-dx, dz);
        if (angle < 0)
            angle = 2048 + angle;
        angle = angle & 2047;

        *ret_angle = angle;

        MSG_add(" angle to him %d myangle %d\n", angle, p_agressor->Draw.Tweened->Angle);

        angle -= (p_agressor->Draw.Tweened->Angle);

        if (angle < 0)
            angle = 2048 + angle;
        angle = angle & 2047;

        if (angle < (FIGHT_ANGLE_RANGE >> 1) || angle > 2048 - (FIGHT_ANGLE_RANGE >> 1)) {
            SLONG dx, dz;
            return (1);
        } else {
            return (0);
        }
    } else {
        return (0);
    }
}

// Returns UC_TRUE if p_him is behind p_me (based on facing dot product).
// uc_orig: check_hit_from_behind (fallen/Source/Combat.cpp)
SLONG check_hit_from_behind(Thing* p_me, Thing* p_him)
{
    SLONG mx, my, mz;
    SLONG hx, hy, hz;
    SLONG dx, dz;
    SLONG dprod;

    ASSERT(p_me->Class == CLASS_PERSON);
    ASSERT(p_him->Class == CLASS_PERSON);

    calc_sub_objects_position(p_me, p_me->Draw.Tweened->AnimTween, 0, &mx, &my, &mz);
    mx += p_me->WorldPos.X >> 8;
    mz += p_me->WorldPos.Z >> 8;

    calc_sub_objects_position(p_him, p_him->Draw.Tweened->AnimTween, 0, &hx, &hy, &hz);
    hx += p_him->WorldPos.X >> 8;
    hz += p_him->WorldPos.Z >> 8;

    dx = -SIN(p_me->Draw.Tweened->Angle);
    dz = -COS(p_me->Draw.Tweened->Angle);

    SLONG vx = hx - mx;
    SLONG vz = hz - mz;

    dprod = dx * vx + dz * vz;

    if (dprod < 0) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// Returns true if 'anim' is a finishing combo move (3rd in a chain).
// uc_orig: is_combo_anim (fallen/Source/Combat.cpp)
SLONG is_combo_anim(SLONG anim)
{
    return anim == ANIM_PUNCH_COMBO3 || anim == ANIM_PUNCH_COMBO3b || anim == ANIM_KICK_COMBO3 || anim == ANIM_KICK_COMBO3b;
}

// Master damage application: applies damage, recoil, audio, and death to the hit victim.
// p_aggressor may be NULL for environment damage. fight may be NULL for gun shots.
// uc_orig: apply_hit_to_person (fallen/Source/Combat.cpp)
SLONG apply_hit_to_person(Thing* p_thing, SLONG angle, SLONG type, SLONG damage, Thing* p_aggressor, struct GameFightCol* fight)
{
    SLONG hit_wave;
    DrawTween* draw_info;
    SLONG behind;
    SLONG block = 0;
    UBYTE player_hit = 0;
    UBYTE skill = 1, shot = 0, stomped = 0, ko = 0, ko_ed = 0, batted = 0, knifed = 0;
    GameCoord vec;
    SLONG nad = 0, taunt_prob = 15;

    switch (type) {
    case HIT_TYPE_GUN_SHOT_H:
    case HIT_TYPE_GUN_SHOT_M:
    case HIT_TYPE_GUN_SHOT_L:
    case HIT_TYPE_GUN_SHOT_PISTOL:
    case HIT_TYPE_GUN_SHOT_SHOTGUN:
    case HIT_TYPE_GUN_SHOT_AK47:
        shot = 1;
        if (!p_thing->Genus.Person->PlayerID)
            p_thing->Flags |= FLAGS_PERSON_BEEN_SHOT;
    }

    if (p_thing->Genus.Person->PlayerID) {
        player_hit = 1;
    }

    if (p_aggressor) {
        SLONG node;
        SLONG hit_anim, hit;
        ASSERT(p_aggressor->Class == CLASS_PERSON);

        if (player_hit || p_aggressor->Genus.Person->PlayerID) {
            // One of them is a player: verify line-of-sight for melee hits
            if (!(p_thing->Genus.Person->Flags & FLAG_PERSON_KO)) {
                if (!shot)
                    if (!there_is_a_los_things(p_thing, p_aggressor, LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                        return (0);
                    }
            }
        }

        node = p_aggressor->Genus.Person->CombatNode;
        if (node < 0)
            node = 0;

        hit_anim = p_aggressor->Draw.Tweened->CurrentAnim;

        // Per-animation damage bonuses on top of fight_tree[node][DAMAGE]:
        // KICK_NAD: +50 to males, +10 to females, taunt_prob->75%.
        // BAT/FLYKICK/KICK_BEHIND: +20; KICK_R/L: +30; combo finishers: ko=UC_TRUE.
        // FIGHT_STOMP: +50. KICK_NEAR: fixed 40. KNIFE_ATTACK*: 30/50/70.
        switch (hit_anim) {
        case ANIM_KICK_NAD:
            if (SOUND_Gender(p_thing) == 1) {
                damage += 50;
                nad = 1;
                taunt_prob = 75;
            } else {
                damage += 10;
            }
            break;
        case ANIM_BAT_HIT1:
        case ANIM_BAT_HIT2:
            batted = 1;
            // fall through
        case ANIM_FLYKICK_START:
        case ANIM_FLYKICK_LAND:
        case ANIM_FLYKICK_FALL:
            taunt_prob += 10;
            // fall through
        case ANIM_KICK_BEHIND:
            damage += 20;
            // fall through
        case ANIM_KICK_RIGHT:
        case ANIM_KICK_LEFT:
            damage += 30;
            // fall through
        case ANIM_PUNCH_COMBO3:
        case ANIM_PUNCH_COMBO3b:
        case ANIM_KICK_COMBO3:
        case ANIM_KICK_COMBO3b:
            if (p_thing->Genus.Person->PersonType == PERSON_MIB1 || p_thing->Genus.Person->PersonType == PERSON_MIB2 || p_thing->Genus.Person->PersonType == PERSON_MIB3) {
                // MIB can't be knocked out
            } else if (p_thing->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
                // Invulnerable people can't be knocked out
            } else {
                ko = UC_TRUE;
            }

            if (p_thing->Genus.Person->pcom_ai == PCOM_AI_FIGHT_TEST) {
                // Training dummies die from special moves even if invulnerable
                if (((p_thing->Genus.Person->pcom_ai_other & PCOM_COMBAT_COMBO_PPP) && (hit_anim == ANIM_PUNCH_COMBO3)) || ((p_thing->Genus.Person->pcom_ai_other & PCOM_COMBAT_COMBO_KKK) && (hit_anim == ANIM_KICK_COMBO3)) || ((p_thing->Genus.Person->pcom_ai_other & PCOM_COMBAT_COMBO_ANY) && is_combo_anim(hit_anim)) || ((p_thing->Genus.Person->pcom_ai_other & PCOM_COMBAT_SIDE_KICK) && (hit_anim == ANIM_KICK_RIGHT || hit_anim == ANIM_KICK_LEFT)) || ((p_thing->Genus.Person->pcom_ai_other & PCOM_COMBAT_BACK_KICK) && (hit_anim == ANIM_KICK_BEHIND))) {
                    ko = UC_TRUE;
                    p_thing->Genus.Person->Health = 0;
                }
            }
            break;
        case ANIM_FIGHT_STOMP:
            damage += 50;
            stomped = 1;
            break;
        case ANIM_KICK_NEAR:
            damage = 40;
            break;
        case ANIM_KNIFE_ATTACK1:
            damage = 30;
            knifed = 1;
            break;
        case ANIM_KNIFE_ATTACK2:
            damage = 50;
            knifed = 1;
            break;
        case ANIM_KNIFE_ATTACK3:
            damage = 70;
            knifed = 1;
            break;
        }

        if ((Random() & 0xff) < taunt_prob) {
            SLONG sound = 0;
            switch (p_aggressor->Genus.Person->PersonType) {
            case PERSON_DARCI:
                if (Random() & 3)
                    sound = SOUND_Range(S_DARCI_FIGHT_TAUNT_START, S_DARCI_FIGHT_TAUNT_END);
                else
                    sound = SOUND_Range(S_DARCI_DEADSAFE_TAUNT_START, S_DARCI_DEADSAFE_TAUNT_END);
                break;
            case PERSON_ROPER:
                if (Random() & 2)
                    sound = SOUND_Range(S_ROPER_FIGHT_TAUNT_START, S_ROPER_FIGHT_TAUNT_END);
                else
                    sound = SOUND_Range(S_ROPER_DEADSAFE_TAUNT_START, S_ROPER_DEADSAFE_TAUNT_END);
                break;
            case PERSON_COP:
                sound = SOUND_Range(S_COP_TAUNT_START, S_COP_TAUNT_END);
                break;
            case PERSON_CIV:
                break;
            case PERSON_THUG_RASTA:
                sound = SOUND_Range(S_RASTA_TAUNT_START, S_RASTA_TAUNT_END);
                break;
            case PERSON_THUG_GREY:
                sound = SOUND_Range(S_GGREY_TAUNT_START, S_GGREY_TAUNT_END);
                break;
            case PERSON_THUG_RED:
                if (THING_NUMBER(p_aggressor) & 1)
                    sound = SOUND_Range(S_GRED_TAUNT_START, S_GRED_TAUNT_END);
                else
                    sound = SOUND_Range(S_GRED2_TAUNT_START, S_GRED2_TAUNT_END);
                break;
            }
            if (sound)
                MFX_play_thing(THING_NUMBER(p_aggressor), sound, MFX_QUEUED | MFX_SHORT_QUEUE, p_aggressor);
        }

        if (hit = fight_tree[node][FIGHT_TREE_DAMAGE]) {
            damage += hit;
        }

    } else if (damage < 30) {
        damage = 30;
    }

    if (is_person_ko_and_lay_down(p_thing)) {
        ko_ed = 1;
    }

    if (p_aggressor->Genus.Person->PersonType == PERSON_ROPER) {
        if (shot)
            damage <<= 1;
        else
            damage += 20;
    }
    if (p_aggressor->Genus.Person->PlayerID) {
        if (shot)
            damage += NET_PLAYER(p_aggressor->Genus.Person->PlayerID - 1)->Genus.Player->Skill;
        else
            damage += NET_PLAYER(p_aggressor->Genus.Person->PlayerID - 1)->Genus.Player->Strength;
    }

    if (player_hit) {
        if (shot == 0)
            if (p_aggressor->Genus.Person->PlayerID) {
                PCOM_new_gang_attack(p_aggressor, p_thing);
            }
        damage >>= 1;
    } else {
        skill = GET_SKILL(p_thing);
        damage -= skill;
        if (damage <= 0) {
            damage = 2;
        }
    }

    damage += GET_SKILL(p_aggressor);

    {
        ASSERT(p_thing->Class == CLASS_PERSON);

        if (p_thing->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
            emergency_uncarry(p_thing);
        }
    }

    if (p_thing->SubState == SUB_STATE_BLOCK && (type == 0 || type > HIT_TYPE_GUN_SHOT_L)) {
        block = 1;
    } else {
        p_thing->Genus.Person->Agression -= damage;
        if (p_thing->Genus.Person->Agression > 0)
            p_thing->Genus.Person->Agression = 0;

        p_aggressor->Genus.Person->Agression += damage;
        if (p_aggressor->Genus.Person->Agression < 0)
            p_aggressor->Genus.Person->Agression = 0;
    }

    if (ko_ed) {
        if (!stomped && !shot) {
            return (0);
        }
    } else {
        stomped = 0;
    }

    if (is_person_dead(p_thing))
        return (0);

    if (p_thing->SubState == SUB_STATE_GRAPPLE_HELD || p_thing->SubState == SUB_STATE_GRAPPLE_ATTACK || p_thing->SubState == SUB_STATE_ESCAPE)
        return (0);

    behind = check_hit_from_behind(p_thing, p_aggressor);
    if (behind > 0)
        behind = 1;
    else
        behind = 0;

    if (shot == 0) {
        if (p_thing->Genus.Person->PlayerID) {
            if (p_thing->Genus.Person->Target == 0) {
                person_enter_fight_mode(p_thing);
                p_thing->Genus.Person->Target = THING_NUMBER(p_aggressor);
                ASSERT(p_thing->Genus.Person->Target != THING_NUMBER(p_thing));
                ASSERT(TO_THING(p_thing->Genus.Person->Target)->Class == CLASS_PERSON);
            }
        }
    }

    draw_info = p_thing->Draw.Tweened;

    MFX_stop(THING_NUMBER(p_thing), S_SEARCH_END);

    if (block == 0) {
        SLONG pain = 0;
        if (p_thing == talk_thing) {
            PainSound(p_thing);
            pain = 1;
            check_eway_talk(1);
        }

        switch (type) {
        case HIT_TYPE_GUN_SHOT_H:
        case HIT_TYPE_GUN_SHOT_M:
        case HIT_TYPE_GUN_SHOT_L:
        case HIT_TYPE_GUN_SHOT_PISTOL:
        case HIT_TYPE_GUN_SHOT_SHOTGUN:
        case HIT_TYPE_GUN_SHOT_AK47:
            break;
        default:
            // Spawn a blood droplet at the attacker's left hand position
            calc_sub_objects_position(
                p_aggressor,
                p_aggressor->Draw.Tweened->AnimTween,
                SUB_OBJECT_LEFT_HAND,
                &vec.X, &vec.Y, &vec.Z);
            vec.X += (Random() & 0x1f);
            vec.Y += (Random() & 0x1f);
            vec.Z += (Random() & 0x1f);
            vec.X <<= 8;
            vec.Y <<= 8;
            vec.Z <<= 8;
            vec.X += p_thing->WorldPos.X;
            vec.Y += p_thing->WorldPos.Y;
            vec.Z += p_thing->WorldPos.Z;

            if (VIOLENCE)
                DIRT_new_water(vec.X >> 8, vec.Y >> 8, vec.Z >> 8, (Random() & 0xf) - 7, 0, (Random() & 0xf) - 7, DIRT_TYPE_BLOOD);
            if (knifed) {
                hit_wave = SOUND_Range(S_STAB_START, S_STAB_END);
                MFX_play_xyz(0, hit_wave, 0, p_thing->WorldPos.X, p_thing->WorldPos.Y, p_thing->WorldPos.Z);

                if (VIOLENCE)
                    for (hit_wave = 0; hit_wave < 5; hit_wave++)
                        DIRT_new_water(vec.X >> 8, vec.Y >> 8, vec.Z >> 8, (Random() & 0xf) - 7, 0, (Random() & 0xf) - 7, DIRT_TYPE_BLOOD);
            } else if (batted) {
                if (SOUND_Gender(p_thing) == 1)
                    hit_wave = SOUND_Range(S_BAT_MALE_START, S_BAT_MALE_END);
                else
                    hit_wave = SOUND_Range(S_BAT_FEMALE_START, S_BAT_FEMALE_END);

                MFX_play_xyz(0, hit_wave, 0, p_thing->WorldPos.X, p_thing->WorldPos.Y, p_thing->WorldPos.Z);

            } else if (nad)
                MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_KICK_IN_THE_NUTS_START, S_KICK_IN_THE_NUTS_END), 0, p_thing);
            else {
                hit_wave = S_PUNCH_START + (GAME_TURN & 3);
                MFX_play_xyz(0, hit_wave, 0, p_thing->WorldPos.X, p_thing->WorldPos.Y, p_thing->WorldPos.Z);
                if (!pain)
                    PainSound(p_thing);
            }

            // Blood splat particle on victim's left hand
            if (VIOLENCE) {
                calc_sub_objects_position(
                    p_thing,
                    p_thing->Draw.Tweened->AnimTween,
                    SUB_OBJECT_LEFT_HAND,
                    &vec.X, &vec.Y, &vec.Z);
                vec.X += (Random() & 0x1f);
                vec.Y += (Random() & 0x1f);
                vec.Z += (Random() & 0x1f);
                vec.X <<= 8;
                vec.Y <<= 8;
                vec.Z <<= 8;
                vec.X += p_thing->WorldPos.X;
                vec.Y += p_thing->WorldPos.Y;
                vec.Z += p_thing->WorldPos.Z;
                PARTICLE_Add(vec.X, vec.Y, vec.Z, 0, 0, 0,
                    POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), 0x7FFF0000,
                    PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE, 10, 75, 1, 20, 5);
            }
        }

        if ((p_thing->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL) && (!p_aggressor || !p_aggressor->Genus.Person->PlayerID)) {
            // Only the player can hurt this person -- skip damage
        } else if (p_thing->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
            // Invulnerable -- skip damage
        } else {
            p_thing->Genus.Person->Health -= damage;
        }

        if (p_thing->Genus.Person->Health <= 0) {
            if (p_aggressor) {
                p_aggressor->Genus.Person->Flags2 |= FLAG2_PERSON_IS_MURDERER;
            }

            p_thing->Genus.Person->Health = 0;

            if (stomped && ko_ed == 0) {
                ASSERT(0);
            }
            if (!is_there_room_behind_person(p_thing, behind)) {
                behind = !behind;
            }

            if (ko_ed) {
                p_thing->Genus.Person->Flags &= ~(FLAG_PERSON_KO | FLAG_PERSON_HELPLESS);
                set_person_dead(p_thing, p_aggressor, PERSON_DEATH_TYPE_COMBAT_PRONE, behind, 0);
            } else if (fight) {
                set_person_dead(p_thing, p_aggressor, PERSON_DEATH_TYPE_COMBAT, behind, fight->Height & 0x3);
            } else {
                SLONG death_type = PERSON_DEATH_TYPE_COMBAT;

                if (shot) {
                    if (person_on_floor(p_thing) && p_thing->OnFace == 0 && really_on_floor(p_thing)) {
                        switch (type) {
                        case HIT_TYPE_GUN_SHOT_PISTOL:
                            death_type = PERSON_DEATH_TYPE_SHOT_PISTOL;
                            break;
                        case HIT_TYPE_GUN_SHOT_SHOTGUN:
                            death_type = PERSON_DEATH_TYPE_SHOT_SHOTGUN;
                            break;
                        case HIT_TYPE_GUN_SHOT_AK47:
                            death_type = PERSON_DEATH_TYPE_SHOT_AK47;
                            break;
                        }
                    } else {
                        death_type = PERSON_DEATH_TYPE_COMBAT;
                    }
                }
                set_person_dead(p_thing, p_aggressor, death_type, behind, 0);
            }

            return (damage);
        }

        if (!shot) {
            if (person_has_gun_out(p_thing)) {
                drop_current_gun(p_thing, 0);
            }
        }

        if (p_aggressor->Genus.Person->PlayerID) {
            track_enemy(p_thing);
            GAME_SCORE(p_aggressor->Genus.Person->PlayerID - 1) += 10;
        }

        if (!p_thing->Genus.Person->PlayerID)
            if (!ko_ed)
                if (!behind)
                    set_face_thing(p_thing, p_aggressor);

        if (!ko_ed && p_thing->Genus.Person->PlayerID == 0) {
            PCOM_attack_happened(p_thing, p_aggressor);
        }
    }

    if (!ko_ed) {
        SLONG anim, flag;
        SLONG height;
        UBYTE history;

        if (fight) {
            height = fight->Height;
            height &= 3;
        } else {
            height = 2;
        }

        MSG_add(" take hit height %d behind %d \n", height, behind);
        ASSERT(height + behind * 3 <= 5);
        anim = take_hit[height + behind * 3][0];
        flag = take_hit[height + behind * 3][1];

        if (block && height != 0) {
        } else {
            if (ko) {
                p_thing->Genus.Person->Agression = -60;

                if (!is_there_room_behind_person(p_thing, behind)) {
                    behind = !behind;
                }
                set_person_dead(p_thing, 0, PERSON_DEATH_TYPE_STAY_ALIVE, behind, 0);
            } else {
                if (height == 0) {
                    sweep_feet(p_thing, p_aggressor, 0);
                } else {
                    if (shot && p_thing->Genus.Person->PlayerID && p_thing->State == STATE_MOVEING) {
                        return (damage);
                    } else {
                        if (nad) {
                            set_person_recoil(p_thing, ANIM_KICK_NAD_TAKE, flag);
                        } else {
                            set_person_recoil(p_thing, anim, flag);
                        }
                    }
                }
            }
        }
    } else {
        SLONG anim;

        switch (person_is_lying_on_what(p_thing)) {
        case PERSON_ON_HIS_FRONT:
            anim = ANIM_FIGHT_STOMPED_BACK;
            break;
        case PERSON_ON_HIS_BACK:
            anim = ANIM_FIGHT_STOMPED_FRONT;
            break;
        default:
            // Headbutted people cause a crash here
            ASSERT(0);
            break;
        }

        set_person_ko_recoil(p_thing, anim, 0);
    }

    PCOM_oscillate_tympanum(
        PCOM_SOUND_FIGHT,
        p_aggressor,
        p_aggressor->WorldPos.X >> 8,
        p_aggressor->WorldPos.Y >> 8,
        p_aggressor->WorldPos.Z >> 8);

    if (block)
        return (0);
    else
        return (damage);
}

// uc_orig: people_allowed_to_hit_each_other (fallen/Source/Combat.cpp)
SLONG people_allowed_to_hit_each_other(Thing* p_victim, Thing* p_agressor)
{
    ULONG will_hit = 0xffffffff;

    if (p_agressor->Genus.Person->PlayerID == 0)
        if (p_agressor->Genus.Person->Target && p_agressor->Genus.Person->Target == THING_NUMBER(p_victim)) {

            if (p_agressor->Genus.Person->PersonType == PERSON_COP)
                ASSERT(p_victim->Genus.Person->PersonType != PERSON_COP);
            if (p_agressor->Genus.Person->PersonType == PERSON_ROPER) {
                if (p_victim->Genus.Person->PersonType == PERSON_TRAMP) {
                    extern UBYTE estate;
                    if (estate)
                        return (0);
                }
            }
            return (1);
        }

    if (p_agressor->Genus.Person->pcom_ai == PCOM_AI_BODYGUARD) {
        UWORD i_client = EWAY_get_person(p_agressor->Genus.Person->pcom_ai_other);

        if (i_client == THING_NUMBER(p_victim)) {
            return UC_FALSE;
        }
    }

    if (p_victim->Genus.Person->pcom_bent & p_agressor->Genus.Person->pcom_bent & PCOM_BENT_GANG) {
        if (p_victim->Genus.Person->pcom_colour == p_agressor->Genus.Person->pcom_colour) {
            return UC_FALSE;
        }
    }

    switch (p_agressor->Genus.Person->PersonType) {
    case PERSON_DARCI:
        if (p_victim->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY)
            return (1);

        if (VIOLENCE == 0) {
            if (p_victim->Genus.Person->PersonType == PERSON_CIV && (p_victim->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER)) {
                return (0);
            }
            if ((p_victim->Genus.Person->PersonType == PERSON_CIV && p_victim->Genus.Person->pcom_ai == PCOM_AI_CIV) || p_victim->Genus.Person->PersonType == PERSON_HOSTAGE) {
                return (0);
            }
        }

        will_hit ^= (1 << PERSON_COP) | (1 << PERSON_ROPER);

        if (semtex)
            will_hit ^= (1 << PERSON_TRAMP);

        break;

    case PERSON_TRAMP:
        if (semtex) {
            will_hit ^= (1 << PERSON_DARCI);
            will_hit ^= ((1 << PERSON_THUG_GREY) | (1 << PERSON_THUG_RASTA) | (1 << PERSON_THUG_RED));
        }
        break;

    case PERSON_ROPER:
        if (p_agressor->Genus.Person->PlayerID == 0)
            if (p_victim->Genus.Person->PersonType == PERSON_TRAMP) {
                extern UBYTE estate;
                if (estate) {
                    return (0);
                }
            }

        if (VIOLENCE == 0) {
            if (p_victim->Genus.Person->PersonType == PERSON_CIV && (p_victim->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER)) {
                return (0);
            }
            if ((p_victim->Genus.Person->PersonType == PERSON_CIV && p_victim->Genus.Person->pcom_ai == PCOM_AI_CIV) || p_victim->Genus.Person->PersonType == PERSON_HOSTAGE) {
                return (0);
            }
        }

        will_hit ^= ((1 << PERSON_COP) | (1 << PERSON_DARCI));
        break;

    case PERSON_COP:
        if (p_victim->Genus.Person->Flags & FLAG_PERSON_FELON) {
            ASSERT(p_victim->Genus.Person->PersonType != PERSON_COP);
            return (1);
        }
        if (p_victim->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY) {
            ASSERT(p_victim->Genus.Person->PersonType != PERSON_COP);
            return (1);
        }
        will_hit ^= ((1 << PERSON_COP) | (1 << PERSON_DARCI) | (1 << PERSON_ROPER));
        break;

    case PERSON_THUG_GREY:
    case PERSON_THUG_RASTA:
    case PERSON_THUG_RED:
        if (p_agressor->Genus.Person->PlayerID) {
            return (1);
        }
        will_hit ^= ((1 << PERSON_THUG_GREY) | (1 << PERSON_THUG_RASTA) | (1 << PERSON_THUG_RED));
        will_hit ^= ((1 << PERSON_MIB1) | (1 << PERSON_MIB2) | (1 << PERSON_MIB3));
        if (semtex)
            will_hit ^= (1 << PERSON_TRAMP);
        break;

    case PERSON_MIB1:
    case PERSON_MIB2:
    case PERSON_MIB3:
        will_hit ^= ((1 << PERSON_MIB1) | (1 << PERSON_MIB2) | (1 << PERSON_MIB3));
        break;
    }

    if (will_hit & (1 << p_victim->Genus.Person->PersonType)) {
        if ((p_victim->Genus.Person->pcom_bent & PCOM_BENT_GANG) && (p_agressor->Genus.Person->pcom_bent & PCOM_BENT_GANG) && (p_victim->Genus.Person->pcom_colour == p_agressor->Genus.Person->pcom_colour)) {
            return (0);
        }

        if (p_agressor->Genus.Person->PersonType == PERSON_COP)
            ASSERT(p_victim->Genus.Person->PersonType != PERSON_COP);
        return (1);
    } else {
        return (0);
    }
}

// Applies damage to all nearby enemies the attacker is allowed to hit.
// Filters out targets with invincibility frames (FLIPING, SLIDER_HOLD, KICK_NS, LEG_SWEEP).
// uc_orig: find_combat_target (fallen/Source/Combat.cpp)
SLONG find_combat_target(struct GameFightCol* p_fight, SLONG x, SLONG y, SLONG z, Thing* p_thing)
{
    SLONG i;
    SLONG hit;
    SLONG angle;
    Thing* t_thing;
    SLONG found_upto;

    found_upto = THING_find_sphere(x, y, z, 0x280, found, 16, (1 << CLASS_PERSON));

    hit = 0;

    for (i = 0; i < found_upto; i++) {
        t_thing = TO_THING(found[i]);

        if (t_thing != p_thing) {
            if (people_allowed_to_hit_each_other(t_thing, p_thing)) {
                if (t_thing->SubState != SUB_STATE_FLIPING)
                    if (t_thing->Draw.Tweened->CurrentAnim != ANIM_SLIDER_HOLD)
                        if (t_thing->Draw.Tweened->CurrentAnim != ANIM_KICK_NS)
                            if (t_thing->Draw.Tweened->CurrentAnim != ANIM_LEG_SWEEP) {
                                if (check_combat_hit_with_person(
                                        t_thing,
                                        x, y, z,
                                        p_fight,
                                        p_thing,
                                        &angle)
                                    > 0) {
                                    hit += apply_hit_to_person(t_thing, 0, 0, 0, p_thing, p_fight);
                                }
                            }
            }
        }
    }

    return hit;
}

// Top-level melee hit tick; called when a fight animation frame has a non-NULL Fight pointer.
// Uses pelvis (sub-object 0) as the attack origin. Iterates fight_info->Next chain (max 5 zones).
// uc_orig: apply_violence (fallen/Source/Combat.cpp)
SLONG apply_violence(Thing* p_thing)
{
    struct GameFightCol* fight_info;
    SLONG hits = 0, count = 0;
    SLONG x, y, z;

    calc_sub_objects_position(p_thing, p_thing->Draw.Tweened->AnimTween, 0, &x, &y, &z);
    x += p_thing->WorldPos.X >> 8;
    y += p_thing->WorldPos.Y >> 8;
    z += p_thing->WorldPos.Z >> 8;

    fight_info = p_thing->Draw.Tweened->CurrentFrame->Fight;

    while (fight_info && count++ < 5) {
        hits += find_combat_target(fight_info, x, y, z, p_thing);
        fight_info = fight_info->Next;
    }
    ASSERT(count < 5);
    return (hits);
}

// Scores all nearby people (up to STANCE_MAX_FIND) and returns the best target + position.
// Scoring: score = -dist - abs(dangle)*4. Returns 1 if found; fills stance_target etc.
// The returned stance_position is where the attacker SHOULD stand (attack_distance from target).
// uc_orig: find_attack_stance (fallen/Source/Combat.cpp)
SLONG find_attack_stance(
    Thing* p_person,
    SLONG attack_direction,
    SLONG attack_distance,
    SLONG attack_range,
    Thing** stance_target,
    GameCoord* stance_position,
    SLONG* stance_angle)
{
    SLONG i;
    SLONG dx, dy, dz;
    SLONG angle, dangle, wangle;
    SLONG px, py, pz;
    SLONG dist, score;
    SLONG min_dist, max_dist;
    Thing* p_target;
    SLONG best_score, best_angle;
    Thing* best_target;
    SLONG best_dist, best_dx, best_dz;
    SLONG ox, oy, oz;
    SLONG found_upto;

    found_upto = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        STANCE_RADIUS,
        found,
        STANCE_MAX_FIND,
        THING_FIND_PEOPLE);

    best_target = NULL;
    best_dist = 0;
    best_dx = 0;
    best_dz = 0;
    best_angle = p_person->Draw.Tweened->Angle;
    best_score = -UC_INFINITY;

// uc_orig: STANCE_DIST_LEEWAY (fallen/Source/Combat.cpp)
#define STANCE_DIST_LEEWAY 0x60

    min_dist = attack_distance - attack_range;
    max_dist = attack_distance + attack_range;

    wangle = p_person->Draw.Tweened->Angle;

    switch (attack_direction) {
    case FIND_DIR_FRONT:
        wangle += 0;
        break;
    case FIND_DIR_BACK:
        wangle += 1024;
        break;
    case FIND_DIR_LEFT:
        wangle += 512;
        break;
    case FIND_DIR_RIGHT:
        wangle -= 512;
        break;
    case FIND_DIR_TURN_LEFT:
        wangle += 256;
        break;
    case FIND_DIR_TURN_RIGHT:
        wangle -= 256;
        break;
    default:
        ASSERT(0);
        break;
    }

    wangle &= 2047;

    for (i = 0; i < found_upto; i++) {
        p_target = TO_THING(found[i]);

        if (p_target == p_person) {
            continue;
        }

        if (is_person_dead(p_target)) {
            continue;
        }

        calc_sub_objects_position(p_target, p_target->Draw.Tweened->AnimTween, SUB_OBJECT_PELVIS, &ox, &oy, &oz);
        dx = ox + (p_target->WorldPos.X - p_person->WorldPos.X >> 8);
        dy = oy + (p_target->WorldPos.Y - p_person->WorldPos.Y >> 8);
        dz = oz + (p_target->WorldPos.Z - p_person->WorldPos.Z >> 8);

        dist = QDIST2(abs(dx), abs(dz));

        if (!WITHIN(dist, min_dist, max_dist)) {
            continue;
        }

        angle = calc_angle(dx, dz) + 1024;
        angle &= 2047;
        dangle = angle_diff(angle, wangle);

// uc_orig: STANCE_DANGLE_IMPORTANCE (fallen/Source/Combat.cpp)
#define STANCE_DANGLE_IMPORTANCE 4

        switch (attack_direction) {
        case FIND_DIR_FRONT:
        case FIND_DIR_BACK:
        case FIND_DIR_LEFT:
        case FIND_DIR_RIGHT:
            if (abs(dangle) > 300) {
                score = -UC_INFINITY;
            } else {
                score = 0;
                score -= dist;
                score -= abs(dangle) << STANCE_DANGLE_IMPORTANCE;
            }
            break;

        case FIND_DIR_TURN_LEFT:
            if (0 && !WITHIN(dangle, -512, 256)) {
                score = -UC_INFINITY;
            } else {
                score = 0;
                score -= dist;
                score -= abs(dangle) << STANCE_DANGLE_IMPORTANCE;
            }
            break;

        case FIND_DIR_TURN_RIGHT:
            if (0 && !WITHIN(dangle, -256, +512)) {
                score = -UC_INFINITY;
            } else {
                score = 0;
                score -= dist;
                score -= abs(dangle) << STANCE_DANGLE_IMPORTANCE;
            }
            break;

        default:
            ASSERT(0);
            break;
        }

        if (score > best_score) {
            best_score = score;
            best_target = p_target;
            best_angle = angle;
            best_dx = ox;
            best_dz = oz;
            best_dist = dist;
        }
    }

    if (best_target) {
        switch (attack_direction) {
        case FIND_DIR_FRONT:
            dangle = 0;
            break;
        case FIND_DIR_BACK:
            dangle = 1024;
            break;
        case FIND_DIR_LEFT:
            dangle = -512;
            break;
        case FIND_DIR_RIGHT:
            dangle = +512;
            break;
        case FIND_DIR_TURN_LEFT:
            dangle = 0;
            break;
        case FIND_DIR_TURN_RIGHT:
            dangle = 0;
            break;
        default:
            ASSERT(0);
            break;
        }

        angle = best_angle + dangle;
        angle &= 2047;

        dx = SIN(best_angle) * attack_distance >> 16;
        dz = COS(best_angle) * attack_distance >> 16;

        px = (best_target->WorldPos.X >> 8) + dx + best_dx;
        pz = (best_target->WorldPos.Z >> 8) + dz + best_dz;

        py = PAP_calc_height_at_thing(p_person, px, pz);

        ASSERT(WITHIN(px, 0, (PAP_SIZE_HI << PAP_SHIFT_HI) - 1));
        ASSERT(WITHIN(pz, 0, (PAP_SIZE_HI << PAP_SHIFT_HI) - 1));

        *stance_target = best_target;
        *stance_angle = angle;
        stance_position->X = px << 8;
        stance_position->Y = py << 8;
        stance_position->Z = pz << 8;
        return (1);

    } else {
        *stance_target = NULL;
        *stance_angle = p_person->Draw.Tweened->Angle;
        *stance_position = p_person->WorldPos;
        return (0);
    }
}

// Calls find_attack_stance then rotates p_person to face the best target.
// Returns THING_NUMBER of target if found, negative value if none.
// uc_orig: turn_to_target (fallen/Source/Combat.cpp)
SLONG turn_to_target(Thing* p_person, SLONG find_dir)
{
    Thing* stance_target;
    GameCoord stance_position;
    SLONG stance_angle;
    SLONG ret;

    ret = find_attack_stance(
        p_person,
        find_dir & FIND_DIR_MASK,
        0x80, // Hard-coded melee fight distance
        0x60, // Hard-coded melee fight range
        &stance_target,
        &stance_position,
        &stance_angle);

    p_person->Draw.Tweened->Angle = stance_angle & 2047;
    if (stance_target) {
        if (p_person->Genus.Person->Target == 0) {
            p_person->Genus.Person->Target = THING_NUMBER(stance_target);
            ASSERT(TO_THING(p_person->Genus.Person->Target)->Class == CLASS_PERSON);
            ASSERT(p_person->Genus.Person->Target != THING_NUMBER(p_person));
        }
    }

    if (stance_target)
        return (THING_NUMBER(stance_target));
    return (-ret);
}

// Like turn_to_target but also adjusts the final angle for directional (left/right/back) attacks.
// Used by interfac.cpp when the player inputs a directional attack.
// uc_orig: turn_to_direction_and_find_target (fallen/Source/Combat.cpp)
SLONG turn_to_direction_and_find_target(Thing* p_person, SLONG find_dir)
{
    Thing* stance_target;
    GameCoord stance_position;
    SLONG stance_angle;
    SLONG ret;

    ret = find_attack_stance(
        p_person,
        find_dir & FIND_DIR_MASK,
        0xf0, // Hard-coded extended fight distance
        0xd0, // Hard-coded extended fight range
        &stance_target,
        &stance_position,
        &stance_angle);

    switch (find_dir & FIND_DIR_MASK) {
    case FIND_DIR_LEFT:
        stance_angle += 512;
        break;
    case FIND_DIR_RIGHT:
        stance_angle -= 512;
        break;
    case FIND_DIR_BACK:
        stance_angle += 1024;
        break;
    }

    p_person->Draw.Tweened->Angle = stance_angle & 2047;
    if (stance_target) {
        p_person->Genus.Person->Target = THING_NUMBER(stance_target);
        ASSERT(TO_THING(p_person->Genus.Person->Target)->Class == CLASS_PERSON);
        ASSERT(p_person->Genus.Person->Target != THING_NUMBER(p_person));
        return (THING_NUMBER(stance_target));
    } else {
        p_person->Genus.Person->Target = 0;
        return (0);
    }
}

// Turns toward the best target then plays a punch animation.
// Also offers the target a block roll via should_i_block().
// uc_orig: turn_to_target_and_punch (fallen/Source/Combat.cpp)
SLONG turn_to_target_and_punch(Thing* p_person)
{
    SLONG ret;
    SLONG anim;

    ret = turn_to_target(p_person, FIND_DIR_FRONT);

    anim = set_person_punch(p_person);
    if (ret > 0 && anim) {
        Thing* p_target;
        p_target = TO_THING(ret);
        if (should_i_block(p_target, p_person, anim))
            p_target->Genus.Person->Flags |= FLAG_PERSON_REQUEST_BLOCK;
    }

    return (ret);
}

// Turns toward the best target then plays a kick animation.
// Uses stomp if target is KO'd; uses kick_near if very close (dist < 120).
// uc_orig: turn_to_target_and_kick (fallen/Source/Combat.cpp)
SLONG turn_to_target_and_kick(Thing* p_person)
{
    SLONG ret;
    SLONG anim;
    Thing* p_target;

    ret = turn_to_target(p_person, FIND_DIR_FRONT);

    p_target = TO_THING(ret);
    if (ret && is_person_ko_and_lay_down(p_target)) {
        set_person_stomp(p_person);
    } else {
        SLONG dist = 0;
        if (ret && (dist = dist_to_target(p_person, TO_THING(ret))) < 120)
            anim = set_person_kick_near(p_person, dist);
        else
            anim = set_person_kick(p_person);
        if (ret > 0 && anim) {
            Thing* p_target;
            p_target = TO_THING(ret);
            if (should_i_block(p_target, p_person, anim))
                p_target->Genus.Person->Flags |= FLAG_PERSON_REQUEST_BLOCK;
        }
    }

    return (ret);
}

// Finds the nearest person actively targeting p_person with KILLING or NAVTOKILL ai_state.
// any_state must be UC_FALSE (legacy parameter, always ignored). radius controls search area.
// uc_orig: is_person_under_attack_low_level (fallen/Source/Combat.cpp)
Thing* is_person_under_attack_low_level(Thing* p_person, SLONG any_state, SLONG radius)
{
    SLONG i;
    SLONG num;
    Thing* p_found;
    SLONG dx, dz, dist;
    SLONG best_dist = UC_INFINITY;
    Thing* best_person = NULL;

    ASSERT(any_state == UC_FALSE);

    num = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        radius,
        found,
        8,
        1 << CLASS_PERSON);

    for (i = 0; i < num; i++) {
        p_found = TO_THING(found[i]);

        if (p_found == p_person) {
            // Ignore yourself
        } else if (is_person_dead(p_found)) {
            // Ignore dead or dying (but not KO'd) people
        } else if (p_found->Genus.Person->Flags & FLAG_PERSON_ARRESTED) {
            // Ignore arrested people
        } else if (abs(p_found->WorldPos.Y - p_person->WorldPos.Y) < 0x4000) {
            if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_KILLING || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL) {
                Thing* p_attacking = NULL;

                if (p_found->Genus.Person->pcom_ai_arg) {
                    p_attacking = TO_THING(p_found->Genus.Person->pcom_ai_arg);
                }

                if (p_attacking == p_person || (p_attacking && p_attacking->Genus.Person->pcom_ai == PCOM_AI_BODYGUARD && p_attacking->Genus.Person->pcom_ai_other == THING_NUMBER(p_person))) {
                    dx = p_found->WorldPos.X - p_person->WorldPos.X >> 8;
                    dz = p_found->WorldPos.Z - p_person->WorldPos.Z >> 8;
                    dist = abs(dx) + abs(dz);

                    if (dist < best_dist) {
                        best_dist = dist;
                        best_person = p_found;
                    }
                }
            }
        }
    }

    return best_person;
}

// uc_orig: is_person_under_attack (fallen/Source/Combat.cpp)
Thing* is_person_under_attack(Thing* p_person)
{
    return (is_person_under_attack_low_level(p_person, 0, 0x800));
}
