// uc_orig: Person.cpp (fallen/Source/Person.cpp)
// Person subsystem — chunk 1: dispatch tables, globals, alloc/create/free,
// helper queries, animation setters, and death-slide helpers.
// (lines ~1-1757 of original Person.cpp)

#include "fallen/Headers/Game.h"    // Temporary: Game types, Thing system, state helpers
#include "fallen/Headers/Cop.h"     // Temporary: cop_states dispatch table
#include "fallen/Headers/Darci.h"   // Temporary: darci_states dispatch table
#include "fallen/Headers/Roper.h"   // Temporary: roper_states dispatch table
#include "fallen/Headers/Thug.h"    // Temporary: thug utilities
#include "fallen/Headers/id.h"      // Temporary: SUB_OBJECT_PELVIS, SUB_STATE_* constants
#include "fallen/Headers/statedef.h" // Temporary: STATE_*, ACTION_*, ANIM_* constants
#include "fallen/Headers/animate.h" // Temporary: global_anim_array, game_chunk, DrawTween helpers
#include "fallen/Headers/combat.h"  // Temporary: fight tree, calc_sub_objects_position
#include "fallen/Headers/sample.h"  // Temporary: sound sample IDs (S_*)
#include "fallen/Headers/guns.h"    // Temporary: gun/special types
#include "fallen/Headers/cnet.h"    // Temporary: NETPERSON, NETPLAYERS
#include "fallen/Headers/sewer.h"   // Temporary: sewer utilities
#include "fallen/Headers/mav.h"     // Temporary: MAV_Action, MAV_SPARE, MAV_SPARE_FLAG_WATER
#include "fallen/Headers/Sound.h"   // Temporary: SOUND_Range, SOUND_FXGroups
#include "fallen/Headers/eway.h"    // Temporary: EWAY types
#include "fallen/Headers/spark.h"   // Temporary: spark effects
#include "fallen/Headers/drip.h"    // Temporary: drip effects
#include "fallen/Headers/puddle.h"  // Temporary: puddle effects
#include "fallen/Headers/pap.h"     // Temporary: PAP_2HI, PAP_FLAG_HIDDEN, PAP_calc_height_at_thing
#include "fallen/Headers/supermap.h" // Temporary: dfacets, FACET_FLAG_*, STOREY_TYPE_*
#include "fallen/Headers/ns.h"      // Temporary: NS types
#include "fallen/Headers/dirt.h"    // Temporary: dirt effects
#include "fallen/Headers/hook.h"    // Temporary: grappling hook
#include "fallen/Headers/pcom.h"    // Temporary: PCOM AI types
#include "fallen/Headers/tracks.h"  // Temporary: TRACKS_Add, track types
#include "fallen/DDEngine/Headers/Matrix.h" // Temporary: matrix utilities
#include "fallen/Headers/ob.h"      // Temporary: OB_ob, OB_FLAG_HIDDEN_ITEM
#include "fallen/Headers/wmove.h"   // Temporary: WMOVE types
#include "fallen/Headers/balloon.h" // Temporary: BALLOON_release
#include "fallen/Headers/inside2.h" // Temporary: inside building types
#include "fallen/Headers/walkable.h" // Temporary: walkable utilities
#include "fallen/Headers/overlay.h" // Temporary: overlay types
#include "fallen/Headers/psystem.h" // Temporary: pyro system
#include "fallen/DDEngine/Headers/poly.h" // Temporary: POLY types
#include "fallen/Headers/memory.h"  // Temporary: dfacets extern via memory globals
#include "fallen/Headers/fmatrix.h" // Temporary: fmatrix
#include "fallen/Headers/fc.h"      // Temporary: camera types
#include "fallen/DDLibrary/Headers/MFX.h" // Temporary: MFX_play_thing, MFX_stop, MFX_play_ambient
#include "fallen/Headers/night.h"   // Temporary: night/lighting types
#include "fallen/Headers/ware.h"    // Temporary: WARE_which_contains
#include "fallen/Headers/xlat_str.h" // Temporary: string translation
#include "fallen/Headers/pow.h"     // Temporary: power-up types
#include "fallen/Headers/frontend.h" // Temporary: frontend utilities
#include "fallen/DDEngine/Headers/aeng.h" // Temporary: AENG_ utilities
#include "fallen/DDEngine/Headers/panel.h" // Temporary: panel/HUD utilities
#include "fallen/Headers/collide.h"  // Temporary: there_is_a_los, LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG
#include "fallen/Headers/building.h" // Temporary: CABLE_ALONG_MAX, CABLE_ALONG_SHIFT, FACET_FLAG_*, get_cable_along

#include "actors/characters/person.h"
#include "actors/characters/person_globals.h"
#include "effects/pyro_globals.h"   // Temporary: col_with[] array (shared collision buffer)

// External helpers declared in their own files (not yet migrated or in old headers).
extern BOOL allow_debug_keys;
extern SLONG person_holding_2handed(Thing* p_person);
extern SLONG continue_dir(Thing* p_person, SLONG dir);
extern SLONG should_i_sneak(Thing* p_person);
extern void change_velocity_to(Thing* p_person, SWORD velocity);
extern void change_velocity_to_slow(Thing* p_person, SWORD velocity);
extern SLONG PAP_on_slope(SLONG x, SLONG z, SLONG* angle);
extern SLONG RFACE_on_slope(SLONG face, SLONG x, SLONG z, SLONG* angle);
extern void process_gang_attack(Thing* p_person, Thing* p_target);
extern void reset_gang_attack(Thing* p_target);
extern void drop_on_heads(Thing* p_thing);
extern UWORD count_gang(Thing* p_target);
extern void person_enter_fight_mode(Thing* p_person);
extern UWORD get_any_gang_member(Thing* p_target);
extern void add_damage_value_thing(Thing* p_thing, SLONG value);
extern void set_person_locked_idle_ready(Thing* p_person);
extern SLONG remove_from_gang_attack(Thing* p_person, Thing* p_target);
extern SLONG continue_pressing_action(Thing* p_person);
extern void set_action_used(Thing* p_person);
extern void carry_running(Thing* p_person);
extern void set_person_stand_carry(Thing* p_person);
extern UBYTE stealth_debug;
extern SLONG plant_feet(Thing* p_person);
extern SLONG calc_angle(SLONG dx, SLONG dz);
extern SLONG set_person_sidle(struct Thing* p_person);
extern SLONG slide_ladder;
extern SLONG yomp_speed;
extern SLONG sprint_speed;
// find_face_for_this_pos declared via fallen/Headers/walkable.h above
extern void drop_all_items(Thing* p_person, UBYTE is_being_searched);
extern void add_thing_to_map(Thing* p_thing);
extern void emergency_uncarry(Thing* p_person);
extern SLONG people_allowed_to_hit_each_other(Thing* p_victim, Thing* p_agressor);
extern void locked_anim_change(Thing* p_person, UWORD locked_object, UWORD anim, SLONG dangle);
extern SLONG get_cable_along(SLONG facet, SLONG ax, SLONG az);
extern SWORD people_types[50];
extern void do_person_on_cable(Thing* p_person);

// Local collision query buffer (shares the global col_with pool; MAX_COL_WITH = 16).
#define MAX_COL_WITH 16

// uc_orig: set_stats (fallen/Source/Person.cpp)
void set_stats(void)
{
    stat_game_time = GetTickCount() - stat_start_time;
}

// uc_orig: init_stats (fallen/Source/Person.cpp)
void init_stats(void)
{
    stat_killed_thug = 0;
    stat_killed_innocent = 0;
    stat_arrested_thug = 0;
    stat_arrested_innocent = 0;
    stat_count_bonus = 0;
    stat_start_time = GetTickCount();
}

// Marks the map tile at (x, z) as visited by the player (fog-of-war / minimap).
// uc_orig: set_player_visited (fallen/Source/Person.cpp)
void set_player_visited(UBYTE x, UBYTE z)
{
    UWORD bit;
    ASSERT(WITHIN(x, 0, 127));
    ASSERT(WITHIN(z, 0, 127));
    bit = x & 7;
    x = x >> 3;
    player_visited[x][z] |= 1 << bit;
}

// Returns true if a sound sync event should fire at the given animation frame.
// Used to synchronize sound effects to a specific frame in an animation.
// uc_orig: MagicFrameCheck (fallen/Source/Person.cpp)
BOOL MagicFrameCheck(Thing* p_person, UBYTE frameindex)
{
    if (p_person->Draw.Tweened->FrameIndex >= frameindex) {
        if (!(p_person->Genus.Person->Flags2 & FLAG2_SYNC_SOUNDFX)) {
            p_person->Genus.Person->Flags2 |= FLAG2_SYNC_SOUNDFX;
            return TRUE;
        }
    } else {
        p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
    }
    return FALSE;
}

// uc_orig: PersonIsMIB (fallen/Source/Person.cpp)
BOOL PersonIsMIB(Thing* p_person)
{
    return (p_person->Genus.Person->PersonType == PERSON_MIB1
         || p_person->Genus.Person->PersonType == PERSON_MIB2
         || p_person->Genus.Person->PersonType == PERSON_MIB3);
}

// Manages the sliding sound effect: starts S_SLIDE_START when skid-stopping, stops it otherwise.
// Pass force=1 to unconditionally stop the slide sound.
// uc_orig: SlideSoundCheck (fallen/Source/Person.cpp)
void SlideSoundCheck(Thing* p_person, BOOL force)
{
    if (p_person->Genus.Person->Flags & FLAG_PERSON_SLIDING) {
        MFX_stop(THING_NUMBER(p_person), S_SEARCH_END);
        if (force || !((p_person->State == STATE_MOVEING) && (p_person->SubState == SUB_STATE_RUNNING_SKID_STOP))) {
            MFX_stop(THING_NUMBER(p_person), S_SLIDE_START);
            p_person->Genus.Person->Flags &= ~FLAG_PERSON_SLIDING;
        }
    } else {
        if (p_person->SubState == SUB_STATE_RUNNING_SKID_STOP) {
            MFX_play_thing(THING_NUMBER(p_person), S_SLIDE_START, MFX_LOOPED, p_person);
            p_person->Genus.Person->Flags |= FLAG_PERSON_SLIDING;
        }
    }
}

// uc_orig: init_persons (fallen/Source/Person.cpp)
void init_persons(void)
{
    SLONG c0;
    memset((UBYTE*)PEOPLE, 0, sizeof(Person) * MAX_PEOPLE);
    for (c0 = 0; c0 < MAX_PEOPLE; c0++) {
        PEOPLE[c0].AnimType = PERSON_NONE;
    }
    PERSON_COUNT = 0;
}

// Allocates a new person of the given type from the PEOPLE pool.
// random_number is used for civilian appearance variation.
// Returns the newly allocated Thing, or NULL if the pool is full.
// uc_orig: alloc_person (fallen/Source/Person.cpp)
Thing* alloc_person(UBYTE type, UBYTE random_number)
{
    SLONG c0;
    Person* new_person;
    Thing* person_thing = NULL;

    for (c0 = 0; c0 < MAX_PEOPLE; c0++) {
        if (PEOPLE[c0].AnimType == PERSON_NONE) {
            person_thing = alloc_thing(CLASS_PERSON);
            if (person_thing) {
                new_person = TO_PERSON(c0);
                new_person->PersonType = type;

                new_person->AnimType = anim_type[type];
                new_person->Thing = THING_NUMBER(person_thing);
                new_person->PlayerID = 0;
                new_person->Ammo = 15;
                new_person->SpecialList = 0;
                new_person->SpecialUse = 0;
                new_person->Stamina = 128;
                person_thing->Genus.Person = new_person;
                person_thing->Draw.Tweened = alloc_draw_tween(DT_ROT_MULTI);

                person_thing->OnFace = NULL;

                person_thing->Draw.Tweened->TheChunk = &game_chunk[anim_type[type]];

                person_thing->Draw.Tweened->MeshID = mesh_type[type];

                if (type == PERSON_CIV) {
                    // A civ's PersonID selects their visual appearance (skin/clothes variant).
                    person_thing->Draw.Tweened->PersonID = 6 + random_number % 4;
                }

                set_state_function(person_thing, STATE_INIT);
                person_thing->Genus.Person->Health = health[type];
                return (person_thing);

            } else {
                ASSERT(0);
            }

            break;
        }
    }
    ASSERT(0);
    return person_thing;
}

// uc_orig: free_person (fallen/Source/Person.cpp)
void free_person(Thing* person_thing)
{
    person_thing->Genus.Person->AnimType = PERSON_NONE;
    free_draw_tween(person_thing->Draw.Tweened);
    person_thing->Draw.Tweened = 0;
    free_thing(person_thing);
}

// Allocates a person and places them in the world at (x, y, z).
// Snaps to the nearest floor or building face. Checks for warehouse containment.
// uc_orig: create_person (fallen/Source/Person.cpp)
THING_INDEX create_person(
    SLONG type,
    SLONG random_number,
    SLONG x,
    SLONG y,
    SLONG z)
{
    Thing* p_person = alloc_person(type, random_number);
    global_person++;

    if (p_person) {

        people_types[type]++;
        p_person->WorldPos.X = x;
        p_person->WorldPos.Y = y;
        p_person->WorldPos.Z = z;

        p_person->Genus.Person->HomeX = x >> 8;
        p_person->Genus.Person->HomeZ = z >> 8;

        // Snap person to the nearest floor or building face.
        SLONG new_y;
        SLONG face = find_face_for_this_pos(
            x >> 8,
            y >> 8,
            z >> 8,
            &new_y,
            NULL, 0);

        if (face) {
            if (face == GRAB_FLOOR) {
                p_person->WorldPos.Y = new_y << 8;
            } else {
                p_person->OnFace = face;
                p_person->WorldPos.Y = new_y << 8;
            }
        }

        // Warehouse containment check: if person spawns inside a warehouse column.
        if (PAP_2HI(
                p_person->WorldPos.X >> 16,
                p_person->WorldPos.Z >> 16)
                .Flags
            & PAP_FLAG_HIDDEN) {
            SLONG ware_top;

            ware_top = PAP_calc_map_height_at(
                p_person->WorldPos.X >> 8,
                p_person->WorldPos.Z >> 8);

            if (p_person->WorldPos.Y < (ware_top - 0x80 << 8)) {
                p_person->Genus.Person->Ware = WARE_which_contains(
                    p_person->WorldPos.X >> 16,
                    p_person->WorldPos.Z >> 16);

                if (p_person->Genus.Person->Ware) {
                    p_person->Genus.Person->Flags |= FLAG_PERSON_WAREHOUSE;
                    p_person->Genus.Person->Flags2 |= FLAG2_PERSON_HOME_IN_WAREHOUSE;
                }
            }
        }

        // Run STATE_INIT immediately so the person is ready on the same frame.
        PTIME(p_person) = (UBYTE)Random();
        p_person->StateFn(p_person);

        return THING_NUMBER(p_person);
    } else {
        ASSERT(0);
    }

    return NULL;
}

// Returns non-zero if there is clearance of 'how_much_room' units directly in front of
// the person (no wall/fence blocking), and the ground height difference is not too large.
// uc_orig: is_there_room_in_front_of_me (fallen/Source/Person.cpp)
SLONG is_there_room_in_front_of_me(Thing* p_person, SLONG how_much_room)
{
    SLONG x1;
    SLONG y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG dx;
    SLONG dz;

    x1 = p_person->WorldPos.X >> 8;
    y1 = p_person->WorldPos.Y + 0x6000 >> 8;
    z1 = p_person->WorldPos.Z >> 8;

    dx = -SIN(p_person->Draw.Tweened->Angle) * how_much_room >> 16;
    dz = -COS(p_person->Draw.Tweened->Angle) * how_much_room >> 16;

    x2 = x1 + dx;
    z2 = z1 + dz;

    y2 = PAP_calc_map_height_at(x2, z2) + 0x60;

    if (abs(y1 - y2) > 0x40) {
        // Too much height difference — no room.
        return FALSE;
    }

    if (!there_is_a_los(
            x1, y1, z1,
            x2, y2, z2,
            LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
        return FALSE;
    }

    return TRUE;
}

// Finds the nearest dead (unsearched) person within search range.
// Returns the THING_INDEX of the best candidate or 0 if none found.
// uc_orig: find_searchable_person (fallen/Source/Person.cpp)
SLONG find_searchable_person(Thing* p_person)
{
    SLONG col_with_upto;
    SLONG collide_types = (1 << CLASS_PERSON);
    Thing* col_thing;
    SLONG i;
    SLONG best_dist = INFINITY, best_index = 0, dist;

    col_with_upto = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        256,
        col_with,
        MAX_COL_WITH,
        collide_types);

    for (i = 0; i < col_with_upto; i++) {
        col_thing = TO_THING(col_with[i]);

        if (col_thing->State == STATE_DEAD
            && (col_thing->SubState == SUB_STATE_DEAD_ARRESTED || col_thing->SubState == 0)
            && !(col_thing->Genus.Person->Flags & FLAG_PERSON_SEARCHED)) {
            dist = dist_to_target_pelvis(p_person, col_thing);
            if (dist < best_dist) {
                best_dist = dist;
                best_index = col_with[i];
            }
        }
    }

    if (best_dist < 64) {
        return (best_index);
    } else {
        return (0);
    }
}

// Turns the person to face an object at (ox, oy, oz) and starts the search animation.
// Returns 1 if the person is facing close enough to proceed, 0 if they need to turn more.
// uc_orig: set_person_search (fallen/Source/Person.cpp)
SLONG set_person_search(Thing* p_person, SLONG ob_index, SLONG ox, SLONG oy, SLONG oz)
{
    SLONG dx, dz, angle, dangle;

    dx = (p_person->WorldPos.X >> 8) - ox;
    dz = (p_person->WorldPos.Z >> 8) - oz;

    angle = calc_angle(dx, dz);
    dangle = angle - p_person->Draw.Tweened->Angle;

    if (abs(dangle) < 256 || angle > 2048 - 256) {

        p_person->Draw.Tweened->Angle = angle;
        set_person_locked_idle_ready(p_person);
        set_generic_person_state_function(p_person, STATE_SEARCH);
        p_person->SubState = SUB_STATE_SEARCH_PRIM;
        p_person->Genus.Person->Timer1 = 0;
        p_person->Genus.Person->Flags |= (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

        p_person->Genus.Person->Action = ACTION_NONE;
        p_person->Genus.Person->Target = ob_index;
        return (1);
    } else {
        return (0);
    }
}

// Starts the corpse-search animation on the dead person targeted by p_person.
// uc_orig: set_person_search_corpse (fallen/Source/Person.cpp)
SLONG set_person_search_corpse(Thing* p_person, Thing* p_dead)
{
    set_anim(p_person, ANIM_CROUTCH_DOWN);

    set_generic_person_state_function(p_person, STATE_SEARCH);
    p_person->SubState = SUB_STATE_SEARCH_CORPSE;
    p_person->Genus.Person->Timer1 = 0;
    p_person->Genus.Person->Flags |= (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

    p_person->Genus.Person->Action = ACTION_NONE;
    p_person->Genus.Person->Target = THING_NUMBER(p_dead);
    MFX_play_ambient(THING_NUMBER(p_person), S_SEARCH_END, MFX_LOOPED);
    p_person->Genus.Person->Flags |= FLAG_PERSON_SLIDING; // marks the search-sound as active
    return (1);
}

// Completes a search action: reveals hidden items or marks a corpse as searched.
// uc_orig: release_searched_item (fallen/Source/Person.cpp)
void release_searched_item(Thing* p_person)
{
    SLONG index;
    Thing* p_thing;
    SLONG ob_index;

    MFX_stop(THING_NUMBER(p_person), S_SEARCH_END);
    switch (p_person->SubState) {
    case SUB_STATE_SEARCH_CORPSE:
        drop_all_items(TO_THING(p_person->Genus.Person->Target), 1);
        ASSERT(TO_THING(p_person->Genus.Person->Target)->Class == CLASS_PERSON);
        TO_THING(p_person->Genus.Person->Target)->Genus.Person->Flags |= FLAG_PERSON_SEARCHED;
        MFX_play_ambient(THING_NUMBER(p_person), S_HIDDENITEM, 0);
        break;
    case SUB_STATE_SEARCH_PRIM:
        ob_index = p_person->Genus.Person->Target;
        p_person->Genus.Person->Target = 0;

        if (!(OB_ob[ob_index].flags & OB_FLAG_HIDDEN_ITEM)) {
            return;
        }

        MFX_play_ambient(THING_NUMBER(p_person), S_HIDDENITEM, 0);
        index = thing_class_head[CLASS_SPECIAL];
        while (index) {
            p_thing = TO_THING(index);
            if (p_thing->Flags & FLAG_SPECIAL_HIDDEN) {
                if (p_thing->Genus.Special->counter == ob_index) {
                    add_thing_to_map(p_thing);
                    p_thing->Flags &= ~FLAG_SPECIAL_HIDDEN;
                    p_thing->Genus.Special->counter = 0;
                    OB_ob[ob_index].flags &= ~OB_FLAG_HIDDEN_ITEM;
                }
            }
            index = p_thing->NextLink;
        }
        break;
    }
}

// Returns a floor position near the given person suitable for dropping items.
// Picks a random spot on the same map square; falls back to person's exact position
// if the random spot has a large height difference.
// uc_orig: find_nice_place_near_person (fallen/Source/Person.cpp)
void find_nice_place_near_person(
    Thing* p_person,
    SLONG* nice_x,
    SLONG* nice_y,
    SLONG* nice_z)
{
    SLONG gx;
    SLONG gy;
    SLONG gz;

    SLONG px, py, pz;

    calc_sub_objects_position(
        p_person,
        p_person->Draw.Tweened->AnimTween,
        SUB_OBJECT_PELVIS,
        &px,
        &py,
        &pz);

    px = px + (p_person->WorldPos.X >> 8);
    py = py + (p_person->WorldPos.Y >> 8);
    pz = pz + (p_person->WorldPos.Z >> 8);

    gx = (((px) & 0xff00) | (Random() & 0x1f)) + 0x3f;
    gz = (((pz) & 0xff00) | (Random() & 0x1f)) + 0x3f;

    gy = PAP_calc_height_at_thing(p_person, gx, gz);

    // If huge height difference, fall back to person's current position.
    if (abs(gy - (p_person->WorldPos.Y >> 8)) > 0x50) {
        gx = p_person->WorldPos.X >> 8;
        gz = p_person->WorldPos.Z >> 8;

        gy = PAP_calc_height_at_thing(p_person, gx, gz);
    }

    *nice_x = gx;
    *nice_y = gy + 0x30; // slightly above ground
    *nice_z = gz;
}

// Sets the person into STATE_DYING with the given substate.
// Valid substates: SUB_STATE_DYING_INITIAL_ANI, _FINAL_ANI, _KNOCK_DOWN, _KNOCK_DOWN_WAIT, _ACTUALLY_DIE.
// uc_orig: set_person_dying (fallen/Source/Person.cpp)
void set_person_dying(Thing* p_person, UBYTE substate)
{
    ASSERT(
        substate == SUB_STATE_DYING_ACTUALLY_DIE
        || substate == SUB_STATE_DYING_INITIAL_ANI
        || substate == SUB_STATE_DYING_FINAL_ANI
        || substate == SUB_STATE_DYING_KNOCK_DOWN
        || substate == SUB_STATE_DYING_KNOCK_DOWN_WAIT);

    if (p_person->Genus.Person->Balloon) {
        BALLOON_release(p_person->Genus.Person->Balloon);
    }

    set_generic_person_state_function(p_person, STATE_DYING);

    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_DYING;
    p_person->SubState = substate;
    p_person->Genus.Person->Flags |= (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

    // pcom_ai_counter tracks how long the person has been dead.
    p_person->Genus.Person->pcom_ai_counter = 0;

    if ((substate == SUB_STATE_DYING_KNOCK_DOWN) || (substate == SUB_STATE_DYING_KNOCK_DOWN_WAIT)) {
        return; // not actually dying — they'll get back up
    }

    if (PersonIsMIB(p_person)) {
        MFX_play_thing(THING_NUMBER(p_person), S_MIB_LEVITATE, MFX_LOOPED, p_person);
    }
}

// Returns whether the person is lying face-down or face-up based on current animation.
// uc_orig: person_is_lying_on_what (fallen/Source/Person.cpp)
SLONG person_is_lying_on_what(Thing* p_person)
{
    ASSERT(p_person->Class == CLASS_PERSON);

    switch (p_person->Draw.Tweened->CurrentAnim) {
    case ANIM_KO_BEHIND_BIG:
    case ANIM_KO_BEHIND_BIG_GU:
    case ANIM_KD_FRONT_LOW:
    case ANIM_KD_FRONT_MID:
    case ANIM_KD_FRONT_HI:
    case ANIM_FIGHT_STOMPED_BACK:
    case ANIM_KD_FRONT_LAND:
    case ANIM_HANDS_UP_LIE:
    case ANIM_PISTOL_WHIP_TAKE:
        return PERSON_ON_HIS_FRONT;

    case ANIM_KO_BACK:
    case ANIM_KO_BACK_GU:
    case ANIM_KD_BACK_LOW:
    case ANIM_KD_BACK_MID:
    case ANIM_KD_BACK_HI:
    case ANIM_FIGHT_STOMPED_FRONT:
    case ANIM_KD_BACK_LAND:
    case ANIM_GRAB_ARM_THROWV:
    case ANIM_STRANGLE_VICTIM:
    case ANIM_HEADBUTT_VICTIM:
        return (PERSON_ON_HIS_BACK);

    default:
        return (PERSON_ON_HIS_BACK);
    }
}

// Returns the footstep sound range for the surface the person is currently standing on.
// uc_orig: footstep_wave (fallen/Source/Person.cpp)
SLONG footstep_wave(Thing* p_person)
{
    SLONG start;
    SLONG end;
    SLONG num;

    if (p_person->Genus.Person->Ware) {
        // Inside a warehouse: generic soft footstep.
        return SOUND_Range(S_SOFT_STEP_START, S_SOFT_STEP_END);
    }

    switch (num = person_is_on(p_person)) {

    case PERSON_ON_DUNNO:
    case PERSON_ON_PRIM:
        start = S_FOOTS_ROAD_START;
        end = S_FOOTS_ROAD_END;
        break;
    case PERSON_ON_WATER:
        start = S_FOOTS_PUDDLE_START;
        end = S_FOOTS_PUDDLE_END;
        break;
    case PERSON_ON_SEWATER:
        start = S_FOOTS_SEWER_START;
        end = S_FOOTS_SEWER_END;
        break;
    case PERSON_ON_METAL:
        start = S_FOOTS_RUNG_START;
        end = S_FOOTS_RUNG_END;
        break;
    case PERSON_ON_GRAVEL:
        start = S_FOOTS_GRAVEL_START;
        end = S_FOOTS_GRAVEL_END;
        break;
    case PERSON_ON_WOOD:
        start = S_FOOTS_WOOD_START;
        end = S_FOOTS_WOOD_END;
        break;
    case PERSON_ON_GRASS:
        start = S_FOOTS_GRASS_START;
        end = S_FOOTS_GRASS_END;
        break;
    default:
        if (num == 0xff)
            num = 0;
        if (num <= 0) {
            // Negative index: specific sound group from SOUND_FXGroups table.
            start = SOUND_FXGroups[-num][0];
            end = SOUND_FXGroups[-num][1];
        } else {
            ASSERT(0);
        }
        break;
    }
    return SOUND_Range(start, end);
}

// Returns the relative angle (in 2048-based angle units) from person to target.
// uc_orig: get_dangle (fallen/Source/Person.cpp)
SLONG get_dangle(Thing* p_person, Thing* p_target)
{
    SLONG dx;
    SLONG dz;
    SLONG angle;
    SLONG dangle;

    dx = p_target->WorldPos.X - p_person->WorldPos.X >> 8;
    dz = p_target->WorldPos.Z - p_person->WorldPos.Z >> 8;

    angle = Arctan(dx, -dz) + 1024;
    angle &= 2047;
    dangle = angle - p_person->Draw.Tweened->Angle;
    dangle &= 2047;

    return dangle;
}

// Returns the Y coordinate at the bottom of a fence facet.
// Used for determining vault/jump heights over fences.
// uc_orig: get_fence_bottom (fallen/Source/Person.cpp)
SLONG get_fence_bottom(SLONG x, SLONG z, SLONG col)
{
    SLONG fy;

    if (dfacets[col].FacetFlags & FACET_FLAG_ONBUILDING || dfacets[col].FacetType == STOREY_TYPE_NORMAL) {
        fy = dfacets[col].Y[0];
    } else {
        fy = PAP_calc_height_at(x, z);
    }
    return fy;
}

// Returns the Y coordinate at the top of a fence facet.
// For unclimbable 1-unit-high fences, adds an extra offset so jump detection works.
// uc_orig: get_fence_top (fallen/Source/Person.cpp)
SLONG get_fence_top(SLONG x, SLONG z, SLONG col)
{
    SLONG fy;
    SLONG fheight;
    SLONG ftop;

    if (dfacets[col].FacetFlags & FACET_FLAG_ONBUILDING || dfacets[col].FacetType == STOREY_TYPE_NORMAL) {
        fy = dfacets[col].Y[0];
    } else {
        fy = PAP_calc_height_at(x, z);
    }

    fheight = dfacets[col].Height * 64;
    ftop = fy + fheight;

    if (fheight == 0x100) {
        // Unclimbable 1-unit-high fences: pretend slightly higher so jump detection fires.
        if (dfacets[col].FacetFlags & FACET_FLAG_UNCLIMBABLE) {
            ftop += 0x40;
        }
    }

    return ftop;
}

// Creates foot splash particles and footprint tracks for a person stepping in water/mud.
// limb = SUB_OBJECT_* for a foot bone, or -1 for the center of the person.
// uc_orig: person_splash (fallen/Source/Person.cpp)
void person_splash(
    Thing* p_person,
    SLONG limb)
{
    SLONG i, type;

    SLONG foot_x;
    SLONG foot_y;
    SLONG foot_z;

    SLONG track_x, track_y, track_z, dx, dy, dz;

    SLONG splash_x;
    SLONG splash_y;
    SLONG splash_z;

    if (limb == -1) {
        track_x = 0;
        track_y = 0;
        track_z = 0;
    } else {
        calc_sub_objects_position(
            p_person,
            p_person->Draw.Tweened->AnimTween,
            limb,
            &track_x,
            &track_y,
            &track_z);
    }

    foot_x = track_x + (p_person->WorldPos.X >> 8);
    foot_y = track_y + (p_person->WorldPos.Y >> 8);
    foot_z = track_z + (p_person->WorldPos.Z >> 8);

    for (i = 0; i < 1; i++) {
        splash_x = foot_x + (Random() & 0x1f) - 0xf;
        splash_y = foot_y;
        splash_z = foot_z + (Random() & 0x1f) - 0xf;

        DRIP_create_if_in_puddle(
            splash_x,
            splash_y,
            splash_z);
    }

    PUDDLE_splash(
        foot_x,
        foot_y,
        foot_z);

    // Footprint tracks — static toggle alternates left/right so prints don't double-fire.
    float yaw, pitch, roll;
    float matrix[9], vector[3];
    static int heh_heh_heh = -1;

    if (limb == heh_heh_heh)
        return;
    heh_heh_heh = limb;

    yaw = -float(p_person->Draw.Tweened->Angle) * (2.0F * PI / 2048.0F);
    pitch = -float(p_person->Draw.Tweened->Tilt) * (2.0F * PI / 2048.0F);
    roll = -float(p_person->Draw.Tweened->Roll) * (2.0F * PI / 2048.0F);

    MATRIX_calc(matrix, yaw, pitch, roll);
    vector[2] = 20;
    vector[1] = 0;
    vector[0] = 0;
    MATRIX_MUL(matrix, vector[0], vector[1], vector[2]);
    dx = vector[0];
    dy = vector[1];
    dz = vector[2];

    if ((limb == SUB_OBJECT_RIGHT_FOOT) || (limb == -1)) {
        type = TRACK_TYPE_RIGHT_PRINT;

        calc_sub_objects_position(
            p_person,
            p_person->Draw.Tweened->AnimTween,
            SUB_OBJECT_RIGHT_FOOT,
            &track_x,
            &track_y,
            &track_z);
        track_x <<= 8;
        track_y <<= 8;
        track_z <<= 8;
        track_x += p_person->WorldPos.X;
        track_z += p_person->WorldPos.Z;

        track_y = (PAP_calc_height_at_thing(p_person, track_x >> 8, track_z >> 8) << 8) + 0x180;

        {
            SLONG mx, mz;

            mx = (track_x - (dx * 0x1ff)) >> 16;
            mz = (track_z - (dz * 0x1ff)) >> 16;
            if (mx >= 0 && mz < 128 && mz >= 0 && mz < 128)
                if (!(MAV_SPARE(mx, mz) & MAV_SPARE_FLAG_WATER))
                    p_person->Genus.Person->muckyfootprint = TRACKS_Add(track_x - (dx * 0x1ff), track_y - (dy * 0x1ff), track_z - (dz * 0x1ff), dx * 2, dy * 2, dz * 2, type, p_person->Genus.Person->muckyfootprint);
        }
    }
    if ((limb == SUB_OBJECT_LEFT_FOOT) || (limb == -1)) {
        type = TRACK_TYPE_LEFT_PRINT;

        calc_sub_objects_position(
            p_person,
            p_person->Draw.Tweened->AnimTween,
            SUB_OBJECT_LEFT_FOOT,
            &track_x,
            &track_y,
            &track_z);
        track_x <<= 8;
        track_y <<= 8;
        track_z <<= 8;
        track_x += p_person->WorldPos.X;
        track_z += p_person->WorldPos.Z;
        track_y = (PAP_calc_height_at_thing(p_person, track_x >> 8, track_z >> 8) << 8) + 0x180;

        {
            SLONG mx, mz;

            mx = (track_x - (dx * 0x1ff)) >> 16;
            mz = (track_z - (dz * 0x1ff)) >> 16;
            if (mx >= 0 && mz < 128 && mz >= 0 && mz < 128)
                if (!(MAV_SPARE(mx, mz) & MAV_SPARE_FLAG_WATER))
                    p_person->Genus.Person->muckyfootprint = TRACKS_Add(track_x - (dx * 0x1ff), track_y - (dy * 0x1ff), track_z - (dz * 0x1ff), dx * 2, dy * 2, dz * 2, type, p_person->Genus.Person->muckyfootprint);
        }
    }
}

// Updates the Draw.Tweened->PersonID based on what item/weapon is currently equipped.
// PersonID bits encode weapon type for the renderer to pick the correct mesh variant.
// uc_orig: set_persons_personid (fallen/Source/Person.cpp)
void set_persons_personid(Thing* p_person)
{
    Thing* p_special;

    if (p_person->Genus.Person->PersonType == PERSON_CIV)
        return;

    p_person->Draw.Tweened->PersonID &= 0x1f;

    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        p_person->Draw.Tweened->PersonID |= 1 << 5;
        return;
    }

    if (p_person->Genus.Person->SpecialUse) {
        p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_NONE:
        case SPECIAL_KEY:
            break;
        case SPECIAL_GUN:
            p_person->Draw.Tweened->PersonID |= 1 << 5;
            break;
        case SPECIAL_HEALTH:
        case SPECIAL_BOMB:
            break;
        case SPECIAL_SHOTGUN:
            p_person->Draw.Tweened->PersonID |= 3 << 5;
            break;
        case SPECIAL_KNIFE:
            p_person->Draw.Tweened->PersonID |= 2 << 5;
            break;
        case SPECIAL_EXPLOSIVES:
        case SPECIAL_GRENADE:
            break;
        case SPECIAL_AK47:
            p_person->Draw.Tweened->PersonID |= 5 << 5;
            break;
        case SPECIAL_BASEBALLBAT:
            p_person->Draw.Tweened->PersonID |= 4 << 5;
            break;
        default:
            break;
        }
    }
}

// Empty stub — optimized away by compiler. Originally used to display anim frame
// numbers on screen in debug builds via a #define SHOW_ANIM_NUMS guard.
// uc_orig: ShowAnimNumber (fallen/Source/Person.cpp)
static inline void ShowAnimNumber(SLONG anim)
{
}

// Queues the given animation to play after the current one completes.
// If the person has a locked anim change pending, applies it immediately instead.
// uc_orig: queue_anim (fallen/Source/Person.cpp)
void queue_anim(Thing* p_person, SLONG anim)
{
    ASSERT(anim != 1);
    ASSERT(((ULONG)global_anim_array[p_person->Genus.Person->AnimType][anim]) > 1000);
    p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
    if (p_person->Genus.Person->Flags & FLAG_PERSON_LOCK_ANIM_CHANGE) {
        locked_anim_change(p_person, 0, anim, 0);
        p_person->Genus.Person->Flags &= ~FLAG_PERSON_LOCK_ANIM_CHANGE;
    } else {
        p_person->Draw.Tweened->QueuedFrame = global_anim_array[p_person->Genus.Person->AnimType][anim];
    }
    p_person->Draw.Tweened->CurrentAnim = anim;
    p_person->Draw.Tweened->FrameIndex = 0;
    ASSERT(p_person->Draw.Tweened->CurrentFrame->FirstElement);
    if (p_person->Draw.Tweened->NextFrame)
        ASSERT(p_person->Draw.Tweened->NextFrame->FirstElement);

    ShowAnimNumber(anim);
}

// Blends (tweens) to the target animation from the current one.
// If there is a locked anim change pending, applies it immediately.
// uc_orig: tween_to_anim (fallen/Source/Person.cpp)
void tween_to_anim(Thing* p_person, SLONG anim)
{
    p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
    if (p_person->Genus.Person->Flags & FLAG_PERSON_LOCK_ANIM_CHANGE) {
        locked_anim_change(p_person, 0, anim, 0);
        p_person->Genus.Person->Flags &= ~FLAG_PERSON_LOCK_ANIM_CHANGE;
    } else {
        p_person->Draw.Tweened->NextFrame = global_anim_array[p_person->Genus.Person->AnimType][anim];
        p_person->Draw.Tweened->QueuedFrame = 0;
    }

    p_person->Draw.Tweened->CurrentAnim = anim;
    p_person->Draw.Tweened->FrameIndex = 0;
    ASSERT(p_person->Draw.Tweened->CurrentFrame->FirstElement);
    if (p_person->Draw.Tweened->NextFrame)
        ASSERT(p_person->Draw.Tweened->NextFrame->FirstElement);

    ShowAnimNumber(anim);
}

// Immediately switches to the target animation (no blend/tween).
// If there is a locked anim change pending, applies it immediately.
// uc_orig: set_anim (fallen/Source/Person.cpp)
void set_anim(Thing* p_person, SLONG anim)
{
    ASSERT(anim != 1);
    p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;

    if (p_person->Genus.Person->Flags & FLAG_PERSON_LOCK_ANIM_CHANGE) {
        locked_anim_change(p_person, 0, anim, 0);
        p_person->Genus.Person->Flags &= ~FLAG_PERSON_LOCK_ANIM_CHANGE;
    } else {
        p_person->Draw.Tweened->AnimTween = 0;
        p_person->Draw.Tweened->QueuedFrame = 0;
        p_person->Draw.Tweened->CurrentFrame = global_anim_array[p_person->Genus.Person->AnimType][anim];
        p_person->Draw.Tweened->NextFrame = global_anim_array[p_person->Genus.Person->AnimType][anim]->NextFrame;
        p_person->Draw.Tweened->Locked = 0;
    }
    p_person->Draw.Tweened->FrameIndex = 0;
    p_person->Draw.Tweened->CurrentAnim = anim;
    ASSERT(p_person->Draw.Tweened->CurrentFrame->FirstElement);
    if (p_person->Draw.Tweened->NextFrame)
        ASSERT(p_person->Draw.Tweened->NextFrame->FirstElement);

    ShowAnimNumber(anim);
}

// Switches to a specific animation for a given animation type (e.g. ANIM_TYPE_CIV),
// ignoring any pending locked anim change.
// uc_orig: set_anim_of_type (fallen/Source/Person.cpp)
void set_anim_of_type(Thing* p_person, SLONG anim, SLONG type)
{
    ASSERT(anim != 1);
    p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_LOCK_ANIM_CHANGE;
    p_person->Draw.Tweened->AnimTween = 0;
    p_person->Draw.Tweened->QueuedFrame = 0;
    p_person->Draw.Tweened->CurrentFrame = game_chunk[type].AnimList[anim];
    p_person->Draw.Tweened->NextFrame = game_chunk[type].AnimList[anim]->NextFrame;
    p_person->Draw.Tweened->Locked = 0;
    p_person->Draw.Tweened->FrameIndex = 0;
    p_person->Draw.Tweened->CurrentAnim = anim;
    ASSERT(p_person->Draw.Tweened->CurrentFrame->FirstElement);
    if (p_person->Draw.Tweened->NextFrame)
        ASSERT(p_person->Draw.Tweened->NextFrame->FirstElement);
    ShowAnimNumber(anim);
}

// Switches to an animation on a specific sub-object (e.g. just the upper body),
// using the person's native animation type.
// uc_orig: set_locked_anim (fallen/Source/Person.cpp)
void set_locked_anim(Thing* p_person, SLONG anim, SLONG sub_object)
{
    ASSERT(anim != 1);
    ASSERT(anim);
    p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
    locked_anim_change(p_person, sub_object, anim, 0);
    p_person->Draw.Tweened->AnimTween = 0;
    p_person->Draw.Tweened->QueuedFrame = 0;
    p_person->Draw.Tweened->CurrentFrame = global_anim_array[p_person->Genus.Person->AnimType][anim];
    p_person->Draw.Tweened->NextFrame = global_anim_array[p_person->Genus.Person->AnimType][anim]->NextFrame;
    p_person->Draw.Tweened->CurrentAnim = anim;
    p_person->Draw.Tweened->FrameIndex = 0;
    ShowAnimNumber(anim);
}

// Variant of set_locked_anim with an explicit dangle (angular offset).
// uc_orig: set_locked_anim_angle (fallen/Source/Person.cpp)
void set_locked_anim_angle(Thing* p_person, SLONG anim, SLONG sub_object, SLONG dangle)
{
    ASSERT(anim != 1);
    ASSERT(anim);
    p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
    locked_anim_change(p_person, sub_object, anim, dangle);
    p_person->Draw.Tweened->AnimTween = 0;
    p_person->Draw.Tweened->QueuedFrame = 0;
    p_person->Draw.Tweened->CurrentFrame = global_anim_array[p_person->Genus.Person->AnimType][anim];
    p_person->Draw.Tweened->NextFrame = global_anim_array[p_person->Genus.Person->AnimType][anim]->NextFrame;
    p_person->Draw.Tweened->CurrentAnim = anim;
    p_person->Draw.Tweened->FrameIndex = 0;
    ShowAnimNumber(anim);
}

// Variant of set_locked_anim using game_chunk[AnimType] instead of global_anim_array.
// uc_orig: set_locked_anim_of_type (fallen/Source/Person.cpp)
void set_locked_anim_of_type(Thing* p_person, SLONG anim, SLONG sub_object)
{
    ASSERT(anim != 1);
    ASSERT(anim);
    p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
    locked_anim_change(p_person, sub_object, anim, 0);
    p_person->Draw.Tweened->AnimTween = 0;
    p_person->Draw.Tweened->QueuedFrame = 0;
    p_person->Draw.Tweened->CurrentFrame = game_chunk[p_person->Genus.Person->AnimType].AnimList[anim];
    p_person->Draw.Tweened->NextFrame = game_chunk[p_person->Genus.Person->AnimType].AnimList[anim]->NextFrame;
    p_person->Draw.Tweened->CurrentAnim = anim;
    p_person->Draw.Tweened->FrameIndex = 0;
    ShowAnimNumber(anim);
}

// Returns the squared distance from a person to the end of the cable they are on.
// Used to determine when to stop sliding along a cable (zip-line / rope).
// uc_orig: get_cable_sdist_from_end (fallen/Source/Person.cpp)
static SLONG get_cable_sdist_from_end(SLONG facet, SLONG ax, SLONG az)
{
    SLONG dx;
    SLONG dz;

    SLONG x1;
    SLONG z1;

    SLONG d1;

    struct DFacet* p_facet;

    p_facet = &dfacets[facet];

    // The high end of the cable is the end with the greater Y value.
    if (p_facet->Y[0] > p_facet->Y[1]) {
        x1 = p_facet->x[1] << 8;
        z1 = p_facet->z[1] << 8;

        dx = x1 - ax;
        dz = z1 - az;

        d1 = dx * dx + dz * dz;
    } else {
        x1 = p_facet->x[0] << 8;
        z1 = p_facet->z[0] << 8;

        dx = x1 - ax;
        dz = z1 - az;

        d1 = dx * dx + dz * dz;
    }

    return (d1);
}

// Slides the person along the zip-wire/cable (OnFacet) using current Velocity.
// When the cable end is reached, transitions to drop-down.
// uc_orig: person_death_slide (fallen/Source/Person.cpp)
void person_death_slide(Thing* p_person)
{
    SLONG dx, dy = 0, dz;
    SLONG along;
    SLONG dist, sdist;

    GameCoord new_position = p_person->WorldPos;

    dist = p_person->Velocity >> 2;

    dx = -(SIN(p_person->Draw.Tweened->Angle) * dist) >> 8;
    dz = -(COS(p_person->Draw.Tweened->Angle) * dist) >> 8;
    dx = (dx * TICK_RATIO) >> TICK_SHIFT;
    dz = (dz * TICK_RATIO) >> TICK_SHIFT;

    along = get_cable_along(p_person->Genus.Person->OnFacet, (p_person->WorldPos.X >> 8) + (dx >> 8), (p_person->WorldPos.Z >> 8) + (dz >> 8));
    sdist = get_cable_sdist_from_end(p_person->Genus.Person->OnFacet, (p_person->WorldPos.X >> 8) + (dx >> 8), (p_person->WorldPos.Z >> 8) + (dz >> 8));

#define CABLE_START (1 * CABLE_ALONG_MAX >> 8)
#define CABLE_END (245 * CABLE_ALONG_MAX >> 8)

    // Both old (along range) and new (sdist) checks kept for save-game backwards compatibility.
    if (WITHIN(along, CABLE_START, CABLE_END) && sdist > 64 * 64) {
        // Still on the cable.
        MFX_play_ambient(THING_NUMBER(p_person), S_ZIPWIRE, MFX_LOOPED | MFX_NEVER_OVERLAP);
        MFX_set_pitch(THING_NUMBER(p_person), S_ZIPWIRE, 224 + (p_person->Velocity / 4));

        new_position.X += dx;
        new_position.Z += dz;
        move_thing_on_map(p_person, &new_position);

        do_person_on_cable(p_person);
    } else {
        MFX_stop(THING_NUMBER(p_person), S_ZIPWIRE);

        set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);

        return;
    }
}

#undef CABLE_START
#undef CABLE_END

// ---- Chunk 2: sweep_feet, death helpers, visibility, vault/climb, player AI ----
// (lines ~1769-3193 of original Person.cpp)

// External helpers not yet migrated or declared in other new/ headers.
extern void emergency_uncarry(Thing* p_person);                                  // Person.cpp (later chunk)
extern SLONG fight_any_gang_attacker(Thing* p_person);                           // Person.cpp (later chunk)
extern SLONG set_person_pos_for_fence_vault(Thing* p_person, SLONG col);         // Person.cpp (later chunk)
extern SLONG set_person_pos_for_half_step(Thing* p_person, SLONG col);           // Person.cpp (later chunk)
extern SLONG remove_from_gang_attack(Thing* p_person, Thing* p_target);          // pcom.cpp (migrated)
extern UWORD count_gang(Thing* p_target);                                         // pcom.cpp (migrated)
extern UWORD find_target_from_gang(Thing* p_target);                             // pcom.cpp (migrated)
extern void check_players_gang(Thing* p_target);                                 // pcom.cpp (migrated)
extern SLONG turn_to_face_thing(Thing* p_person, Thing* p_target, SLONG slow);   // Person.cpp (later chunk)
extern void set_fence_hole(struct DFacet* p_facet, SLONG pos);                   // Person.cpp (later chunk)
extern SLONG slide_along(SLONG x1, SLONG y1, SLONG z1, SLONG* x2, SLONG* y2, SLONG* z2, SLONG extra_wall_height, SLONG radius, ULONG flags); // collide.cpp
extern void set_person_mav_to_thing(Thing* p_person, Thing* p_target);            // Person.cpp (later chunk)

// uc_orig: sweep_feet (fallen/Source/Person.cpp)
void sweep_feet(Thing* p_person, Thing* p_aggressor, SLONG death_type)
{
    SlideSoundCheck(p_person, 1);

    if (p_person->State == STATE_JUMPING) {
        return;
    }

    if (p_person->State == STATE_DANGLING) {
        if (p_person->SubState == SUB_STATE_DROP_DOWN) {
            return;
        }
    }

    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
        emergency_uncarry(p_person);
    }

    if (!people_allowed_to_hit_each_other(p_person, p_aggressor)) {
        // Friendly fire: target jumps to avoid instead of getting hurt.
        set_person_standing_jump(p_person);
        return;
    }

    if (PersonIsMIB(p_person)) {
        // MIB ninjas jump to avoid the sweep.
        set_person_standing_jump(p_person);
        return;
    }

    if (PersonIsMIB(p_person)) {
        // Redundant check (original code has it twice).
    }

    if (p_person->Genus.Person->pcom_ai == PCOM_AI_FIGHT_TEST && (p_person->Genus.Person->pcom_ai_other & PCOM_COMBAT_SLIDE)) {
        // Fight-test dummies configured to die on sweep override invulnerability.
        p_person->Genus.Person->Health = 0;
    } else if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
        // Invulnerable people jump instead of getting swept.
        set_person_standing_jump(p_person);
        return;
    }

    {
        SLONG r, prob;
        prob = 40 + (GET_SKILL(p_person) << 3);
        if ((r = (Random() % 160)) < prob) {
            // Person saw the attack coming: tell AI, jump away.
            if (can_a_see_b(p_person, p_aggressor, -1, 1)) {
                PCOM_attack_happened(p_person, p_aggressor);
                set_person_standing_jump(p_person);
                return;
            }
        }
    }

    if ((p_person->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL) && !p_aggressor->Genus.Person->PlayerID) {
        // Only player can hurt this person — no damage.
    } else if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
        // Nothing hurts this person.
    } else {
        if (!p_person->Genus.Person->PlayerID)
            p_person->Genus.Person->Health -= 49;
    }

    if (p_person->Genus.Person->Health <= 0) {
        p_person->Genus.Person->Health = 0;
        death_type = PERSON_DEATH_TYPE_OTHER;

        if (p_aggressor) {
            p_aggressor->Genus.Person->Flags2 |= FLAG2_PERSON_IS_MURDERER;
        }
    } else {
        death_type = PERSON_DEATH_TYPE_LEG_SWEEP;
    }

    PCOM_attack_happened(p_person, p_aggressor);

    set_person_dead(p_person, p_aggressor, death_type, 0, 0);
}

// Returns true if there is room behind the person (in the death-fall direction)
// for them to fall without hitting geometry. hit_from_behind=1 reverses the direction.
// uc_orig: is_there_room_behind_person (fallen/Source/Person.cpp)
SLONG is_there_room_behind_person(Thing* p_person, SLONG hit_from_behind)
{
    ULONG los_flag;

    SLONG dx = SIN(p_person->Draw.Tweened->Angle);
    SLONG dz = COS(p_person->Draw.Tweened->Angle);

    if (hit_from_behind) {
        dx = -dx;
        dz = -dz;
    }

    SLONG h1 = PAP_calc_map_height_at(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Z >> 8);

    SLONG h2 = PAP_calc_map_height_at(
        p_person->WorldPos.X + dx >> 8,
        p_person->WorldPos.Z + dz >> 8);

    if (abs(h2 - h1) > 0x40) {
        return FALSE;
    }

    los_flag = LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_INCLUDE_CARS;

    if (p_person->Genus.Person->Ware) {
        // Skip underground check for people inside warehouses.
        los_flag |= LOS_FLAG_IGNORE_UNDERGROUND_CHECK;
    }

    if (!there_is_a_los(
            (p_person->WorldPos.X >> 8),
            (p_person->WorldPos.Y + 0x3000 >> 8),
            (p_person->WorldPos.Z >> 8),
            (p_person->WorldPos.X + dx >> 8),
            (p_person->WorldPos.Y + 0x3000 >> 8),
            (p_person->WorldPos.Z + dz >> 8),
            los_flag))
    {
        return FALSE;
    }

    return TRUE;
}

// Returns the fractional position along collision facet colvect at world position (x,z).
// Result is in [0,1] range where 0=start vertex, 1=end vertex.
// uc_orig: get_along_facet (fallen/Source/Person.cpp)
SLONG get_along_facet(SLONG x, SLONG z, SLONG colvect)
{
    DFacet* p_facet;
    SLONG dx, dz;
    SLONG along;

    p_facet = &dfacets[colvect];

    dx = p_facet->x[1] - p_facet->x[0];
    dz = p_facet->z[1] - p_facet->z[0];

    if (dx) {
        along = x - (p_facet->x[0] << 8);
        along = along / dx;
    } else {
        along = z - (p_facet->z[0] << 8);
        along = along / dz;
    }

    return (along);
}

// Transitions person into a dying/knockdown state based on death_type.
// Handles animation selection, sound, gang-attack bookkeeping, and score.
// uc_orig: set_person_dead (fallen/Source/Person.cpp)
void set_person_dead(
    Thing* p_thing,
    Thing* p_aggressor,
    SLONG death_type,
    SLONG behind,
    SLONG height)
{
    DrawTween* draw_info;
    SLONG anim;
    SLONG substate;
    SLONG quick = FALSE;
    SLONG locked = FALSE;

    ASSERT(p_thing->Class == CLASS_PERSON);
    p_thing->Draw.Tweened->Roll = 0;
    p_thing->Draw.Tweened->DRoll = 0;

    p_thing->Genus.Person->InsideRoom = 0;

    if (p_thing->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
        emergency_uncarry(p_thing);
    }

    if (p_thing->Genus.Person->PlayerID == 0) {
        PCOM_knockdown_happened(p_thing);
    }

    if (p_aggressor && p_aggressor->Genus.Person->PlayerID) {
        if (death_type != PERSON_DEATH_TYPE_LEG_SWEEP && death_type != PERSON_DEATH_TYPE_STAY_ALIVE && death_type != PERSON_DEATH_TYPE_GET_DOWN && death_type != PERSON_DEATH_TYPE_STAY_ALIVE_PRONE) {
            // Player caused the death — update kill stats.
            switch (p_thing->Genus.Person->PersonType) {
            case PERSON_THUG_RASTA:
            case PERSON_THUG_GREY:
            case PERSON_THUG_RED:
            case PERSON_MIB1:
            case PERSON_MIB2:
            case PERSON_MIB3:
                stat_killed_thug++;
                break;
            default:
                stat_killed_innocent++;
                break;
            }

            if ((p_thing->Genus.Person->pcom_ai == PCOM_AI_CIV && p_thing->Genus.Person->pcom_move == PCOM_MOVE_WANDER) || (p_thing->Genus.Person->PersonType == PERSON_COP)) {
                if (!(p_thing->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY)) {
                    // Player killed an innocent — add a red mark.
                    NET_PLAYER(0)->Genus.Player->RedMarks += 1;
                }
            }
        }
    }

    if (p_thing->SubState == SUB_STATE_GRAPPLE_HELD) {
        // Release the person holding us if they aren't mid-attack.
        if (TO_THING(p_thing->Genus.Person->Target)->SubState != SUB_STATE_GRAPPLE_ATTACK) {
            set_person_fight_idle(TO_THING(p_thing->Genus.Person->Target));
        }
    }

    if (death_type != PERSON_DEATH_TYPE_STAY_ALIVE && death_type != PERSON_DEATH_TYPE_STAY_ALIVE_PRONE && death_type != PERSON_DEATH_TYPE_LEG_SWEEP) {
        if (p_thing->Genus.Person->Target) {
            remove_from_gang_attack(p_thing, TO_THING(p_thing->Genus.Person->Target));
        }
        if (p_aggressor) {
            remove_from_gang_attack(p_thing, p_aggressor);
            if (p_aggressor->Genus.Person->Target == THING_NUMBER(p_thing)) {
                extern UWORD find_target_from_gang(Thing * p_target);
                p_aggressor->Genus.Person->Target = find_target_from_gang(p_aggressor);
                if (p_aggressor->Genus.Person->PlayerID)
                    if (p_aggressor->Genus.Person->Target == 0) {
                        // Nobody is attacking player any more — leave fight mode.
                        p_aggressor->Genus.Person->Mode = PERSON_MODE_RUN;
                    }
            }
        }
    }

    if (p_thing->SubState == SUB_STATE_GRAPPLE_HOLD || p_thing->SubState == SUB_STATE_GRAPPLE_ATTACK) {
        // Dying while grappling — release the grapple target.
        set_person_fight_idle(TO_THING(p_thing->Genus.Person->Target));
    }

// uc_orig: MAX_KNOCK_DOWN (fallen/Source/Person.cpp)
#define MAX_KNOCK_DOWN 6

    // uc_orig: knock_down (fallen/Source/Person.cpp)
    static UBYTE knock_down[MAX_KNOCK_DOWN] = {
        ANIM_KD_FRONT_LOW,
        ANIM_KD_FRONT_MID,
        ANIM_KD_FRONT_HI,
        ANIM_KD_BACK_LOW,
        ANIM_KD_BACK_MID,
        ANIM_KD_BACK_HI
    };

    ASSERT(p_thing->Class == CLASS_PERSON);

    if (death_type != PERSON_DEATH_TYPE_PRONE && death_type != PERSON_DEATH_TYPE_COMBAT_PRONE) {
        if (p_thing->State == STATE_DEAD || p_thing->State == STATE_DYING) {
            return;
        }
    }

    if (p_thing->State == STATE_DYING && p_thing->SubState == SUB_STATE_DYING_FINAL_ANI) {
        // Already playing final death anim — consider them dead.
        return;
    }

    switch (death_type) {
    case PERSON_DEATH_TYPE_PRONE:
        // Already on floor, finish them off quietly.
        substate = SUB_STATE_DYING_ACTUALLY_DIE;
        p_thing->Genus.Person->Flags &= ~(FLAG_PERSON_KO | FLAG_PERSON_HELPLESS);
        quick = TRUE;
        break;

    case PERSON_DEATH_TYPE_COMBAT_PRONE:
        // Person already lying down — play a stomp animation.
        substate = SUB_STATE_DYING_FINAL_ANI;
        p_thing->Genus.Person->Flags &= ~(FLAG_PERSON_KO | FLAG_PERSON_HELPLESS);
        locked = TRUE;

        switch (person_is_lying_on_what(p_thing)) {
        case PERSON_ON_HIS_FRONT:
            anim = ANIM_FIGHT_STOMPED_BACK;
            break;

        case PERSON_ON_HIS_BACK:
            anim = ANIM_FIGHT_STOMPED_FRONT;
            break;

        default:
            ASSERT(0);
            break;
        }

        break;

    case PERSON_DEATH_TYPE_SHOT_PISTOL:
    case PERSON_DEATH_TYPE_SHOT_SHOTGUN:
    case PERSON_DEATH_TYPE_SHOT_AK47:
    {
        // uc_orig: shoot_dead_anim (fallen/Source/Person.cpp)
        static UWORD shoot_dead_anim[3] = {
            ANIM_SHOT_DEAD_HEAD,
            ANIM_SHOT_DEAD_GUT,
            ANIM_SHOT_DEAD_CHEST,
        };
        if (!behind) {
            anim = ANIM_SHOT_DEAD_BACK;
        } else {
            anim = shoot_dead_anim[Random() % 3];
        }

        substate = SUB_STATE_DYING_FINAL_ANI;
        p_thing->Genus.Person->Flags &= ~FLAG_PERSON_KO;
    }
    break;

    case PERSON_DEATH_TYPE_COMBAT:
        ASSERT(WITHIN(behind, 0, 1));
        ASSERT(WITHIN(height, 0, 2));
        ASSERT(WITHIN(behind * 3 + height, 0, MAX_KNOCK_DOWN - 1));

        anim = knock_down[behind * 3 + height];
        substate = SUB_STATE_DYING_INITIAL_ANI;
        break;

    case PERSON_DEATH_TYPE_LAND:
        switch (p_thing->Draw.Tweened->CurrentAnim) {
        case ANIM_PLUNGE_START:
        case ANIM_PLUNGE_FORWARDS:
            anim = ANIM_PLUNGE_FRONT_SLAM;
            if (VIOLENCE)
                PYRO_create(p_thing->WorldPos, PYRO_SPLATTERY);
            break;

        default:
            anim = ANIM_BIGLAND_DIE;
            break;
        }

        StopScreamFallSound(p_thing);
        locked = TRUE;
        substate = SUB_STATE_DYING_FINAL_ANI;
        break;

    case PERSON_DEATH_TYPE_OTHER:
    {
        SLONG ground;
        SLONG dy;

        ground = PAP_calc_height_at_thing(p_thing, p_thing->WorldPos.X >> 8, p_thing->WorldPos.Z >> 8);
        dy = (p_thing->WorldPos.Y >> 8) - ground;

        if (dy < 0x20) {
            anim = (behind) ? ANIM_KO_BEHIND_BIG : ANIM_KO_BACK;
            substate = SUB_STATE_DYING_FINAL_ANI;
        } else {
            anim = ANIM_KD_FRONT_HI;
            substate = SUB_STATE_DYING_INITIAL_ANI;
        }
    }
    break;

    case PERSON_DEATH_TYPE_GET_DOWN:
        set_anim(p_thing, ANIM_HANDS_UP_LIE);
        p_thing->Genus.Person->Flags |= FLAG_PERSON_KO | FLAG_PERSON_HELPLESS;
        substate = SUB_STATE_DYING_KNOCK_DOWN;
        p_thing->Genus.Person->Timer1 = 0;
        quick = TRUE;
        break;

    case PERSON_DEATH_TYPE_STAY_ALIVE_PRONE:
        p_thing->Genus.Person->Flags |= FLAG_PERSON_KO | FLAG_PERSON_HELPLESS;
        substate = SUB_STATE_DYING_KNOCK_DOWN;
        p_thing->Genus.Person->Timer1 = 0;
        quick = TRUE;
        break;

    case PERSON_DEATH_TYPE_STAY_ALIVE:
    {
        SLONG ground;
        SLONG dy;

        ground = PAP_calc_height_at_thing(p_thing, p_thing->WorldPos.X >> 8, p_thing->WorldPos.Z >> 8);
        dy = (p_thing->WorldPos.Y >> 8) - ground;

        p_thing->Genus.Person->Flags |= FLAG_PERSON_KO | FLAG_PERSON_HELPLESS;

        if (dy < 0x20) {
            substate = SUB_STATE_DYING_KNOCK_DOWN;
            anim = (behind) ? ANIM_KO_BEHIND_BIG : ANIM_KO_BACK;
        } else {
            anim = ANIM_KD_FRONT_HI;
            substate = SUB_STATE_DYING_INITIAL_ANI;
        }
    }
    break;

    case PERSON_DEATH_TYPE_LEG_SWEEP:
        anim = ANIM_KD_BACK_LOW;
        substate = SUB_STATE_DYING_INITIAL_ANI;
        p_thing->Genus.Person->Flags |= FLAG_PERSON_KO | FLAG_PERSON_HELPLESS;
        break;

    default:
        ASSERT(0);
        break;
    }

    if (!quick) {
        if (locked) {
            set_locked_anim(p_thing, anim, 0);
        } else {
            set_anim(p_thing, anim);
        }

        switch (p_thing->Genus.Person->PersonType) {
        case PERSON_COP:
            MFX_play_xyz(0, S_MALE_DIE_2, 0, p_thing->WorldPos.X, p_thing->WorldPos.Y, p_thing->WorldPos.Z);
            break;
        case PERSON_DARCI:
            switch (death_type) {
            case HIT_TYPE_GUN_SHOT_H:
            case HIT_TYPE_GUN_SHOT_M:
            case HIT_TYPE_GUN_SHOT_L:
            default:
                MFX_play_xyz(0, S_FEMALE_DIE_2, 0, p_thing->WorldPos.X, p_thing->WorldPos.Y, p_thing->WorldPos.Z);
                break;
            }
            break;
        }
    }

    set_person_dying(p_thing, substate);

    if (p_aggressor) {
        p_aggressor->Genus.Person->InWay = NULL;

        if (p_aggressor->Genus.Person->PlayerID) {
            GAME_SCORE(p_aggressor->Genus.Person->PlayerID - 1) += 50;
        }
    }
}

#undef MAX_KNOCK_DOWN

// Returns guilt level: 0=innocent, 1=guilty/gang, 2=thug, 3=MIB.
// uc_orig: is_person_guilty (fallen/Source/Person.cpp)
SLONG is_person_guilty(Thing* p_person)
{
    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY)
        return (1);

    if (p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_THUG_RED ||
        p_person->Genus.Person->PersonType == PERSON_THUG_GREY) {
        return (2);
    }

    if (PersonIsMIB(p_person))
        return 3;

    if (p_person->Genus.Person->pcom_ai == PCOM_AI_GUARD || p_person->Genus.Person->pcom_ai == PCOM_AI_GANG) {
        return 1;
    }

    return (0);
}

// Returns true if person's state is a ground-contact running/walking/idle state.
// uc_orig: person_on_floor (fallen/Source/Person.cpp)
SLONG person_on_floor(Thing* p_person)
{
    if (p_person->SubState == SUB_STATE_RUNNING || p_person->SubState == SUB_STATE_WALKING || p_person->State == STATE_IDLE || p_person->State == STATE_CIRCLING || p_person->State == STATE_GUN || p_person->State == STATE_HIT_RECOIL) {
        return (1);
    } else {
        return (0);
    }
}

// Returns true if the person's left foot bone is at or near ground height.
// uc_orig: really_on_floor (fallen/Source/Person.cpp)
SLONG really_on_floor(Thing* p_person)
{
    SLONG foot_x;
    SLONG foot_y;
    SLONG foot_z;

    calc_sub_objects_position(
        p_person,
        p_person->Draw.Tweened->AnimTween,
        SUB_OBJECT_LEFT_FOOT,
        &foot_x,
        &foot_y,
        &foot_z);

    foot_x += p_person->WorldPos.X >> 8;
    foot_y += p_person->WorldPos.Y >> 8;
    foot_z += p_person->WorldPos.Z >> 8;

    SLONG ground = PAP_calc_map_height_at(foot_x, foot_z);

    if (foot_y > ground + 32)
        return (0);
    else
        return (1);
}

// uc_orig: is_person_dead (fallen/Source/Person.cpp)
SLONG is_person_dead(Thing* p_person)
{
    if (p_person->State == STATE_DEAD || (p_person->State == STATE_DYING && (p_person->Genus.Person->Flags & FLAG_PERSON_KO) == 0))
        return (1);
    else
        return (0);
}

// Returns true if person is knocked out (lying on ground, not yet dead).
// uc_orig: is_person_ko (fallen/Source/Person.cpp)
SLONG is_person_ko(Thing* p_person)
{
    if (p_person->State == STATE_DYING) {
        switch (p_person->SubState) {
        case SUB_STATE_DYING_KNOCK_DOWN_WAIT:
        case SUB_STATE_DYING_KNOCK_DOWN:
            return TRUE;
        }
    }

    return FALSE;
}

// Returns true if person is knocked out and has finished the fall animation (lying flat).
// uc_orig: is_person_ko_and_lay_down (fallen/Source/Person.cpp)
SLONG is_person_ko_and_lay_down(Thing* p_person)
{
    if (p_person->State == STATE_DYING) {
        switch (p_person->SubState) {
        case SUB_STATE_DYING_KNOCK_DOWN_WAIT:
            return TRUE;
        }
    }

    return FALSE;
}

// Deals hitpoints damage from position (origin_x, origin_z) and knocks person to ground.
// uc_orig: knock_person_down (fallen/Source/Person.cpp)
void knock_person_down(
    Thing* p_person,
    SLONG hitpoints,
    SLONG origin_x,
    SLONG origin_z,
    Thing* p_aggressor)
{
    SLONG death_type;
    SLONG behind;

    // Non-person aggressors (e.g. Balrog) are ignored for attribution.
    if (p_aggressor && p_aggressor->Class != CLASS_PERSON) {
        p_aggressor = NULL;
    }

    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
        emergency_uncarry(p_person);
    }

    set_face_pos(
        p_person,
        origin_x,
        origin_z);

    behind = FALSE;

    if ((p_person->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL) && (!p_aggressor || !p_aggressor->Genus.Person->PlayerID)) {
        // Only player can hurt this person.
    } else if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
        // Nothing hurts this person.
    } else {
        p_person->Genus.Person->Health -= hitpoints;
    }

    if (p_person->Genus.Person->Health <= 0) {
        p_person->Genus.Person->Health = 0;
        death_type = PERSON_DEATH_TYPE_OTHER;

        if (p_aggressor) {
            p_aggressor->Genus.Person->Flags2 |= FLAG2_PERSON_IS_MURDERER;
        }
    } else {
        death_type = PERSON_DEATH_TYPE_STAY_ALIVE;
    }

    if (!is_there_room_behind_person(p_person, behind)) {
        behind = !behind;
    }

    set_person_dead(
        p_person,
        p_aggressor,
        death_type,
        behind,
        0);
}

// Pushes person forward along their facing by dist world units (fixed-point).
// Used to add separation after punch impacts.
// uc_orig: person_bodge_forward (fallen/Source/Person.cpp)
void person_bodge_forward(Thing* p_person, SLONG dist)
{
    SLONG dx, dy = 0, dz;
    GameCoord new_position = p_person->WorldPos;

    dx = -(SIN(p_person->Draw.Tweened->Angle) * dist) >> 8;
    dz = -(COS(p_person->Draw.Tweened->Angle) * dist) >> 8;

    new_position.X += dx;
    new_position.Z += dz;

    move_thing_on_map(p_person, &new_position);
}

// Returns true if there is a clear sight line between the two persons' head positions.
// uc_orig: los_between_heads (fallen/Source/Person.cpp)
SLONG los_between_heads(
    Thing* person_1,
    Thing* person_2)
{
    SLONG x1 = person_1->WorldPos.X >> 8;
    SLONG y1 = person_1->WorldPos.Y >> 8;
    SLONG z1 = person_1->WorldPos.Z >> 8;

    SLONG x2 = person_2->WorldPos.X >> 8;
    SLONG y2 = person_2->WorldPos.Y >> 8;
    SLONG z2 = person_2->WorldPos.Z >> 8;

    y1 += 0x70;
    y2 += 0x70;

    return there_is_a_los(
        x1, y1, z1,
        x2, y2, z2,
        0);
}

// Plays a tin-pan sound at (x,y,z) and makes nearby idle NPCs look toward p_thing.
// NPCs with p_thing as their current target are skipped.
// uc_orig: oscilate_tinpanum (fallen/Source/Person.cpp)
void oscilate_tinpanum(SLONG x, SLONG y, SLONG z, Thing* p_thing, SLONG vol)
{
    SLONG col_with_upto;
    SLONG collide_types = (1 << CLASS_PERSON);
    Thing* col_thing;
    SLONG i;

    col_with_upto = THING_find_sphere(
        x,
        y,
        z,
        5 * 256,
        col_with,
        MAX_COL_WITH,
        collide_types);

    for (i = 0; i < col_with_upto; i++) {
        col_thing = TO_THING(col_with[i]);

        if (col_thing->State == STATE_DEAD || col_thing->State == STATE_DYING) {
            continue;
        }

        switch (col_thing->Class) {
        case CLASS_PERSON:
            if (col_thing == p_thing) {
                // Don't influence the source person.
            } else {
                if ((col_thing->Genus.Person->Target != THING_NUMBER(p_thing)) && (col_thing->Genus.Person->PlayerID == 0)) {
                    if (col_thing->State == STATE_IDLE) {
                        // Only idle NPCs react.
                        if (los_between_heads(col_thing, p_thing)) {
                            set_face_thing(col_thing, p_thing);
                        } else {
                            turn_to_face_thing(col_thing, p_thing, 2);
                        }
                    }
                }
            }
            break;
        }
    }
}

// Returns the 2D approximate distance between two persons' feet.
// uc_orig: dist_to_target (fallen/Source/Person.cpp)
SLONG dist_to_target(Thing* p_person_a, Thing* p_person_b)
{
    SLONG dx, dz;

    dx = abs(p_person_a->WorldPos.X - p_person_b->WorldPos.X) >> 8;
    dz = abs(p_person_a->WorldPos.Z - p_person_b->WorldPos.Z) >> 8;
    return (QDIST2(dx, dz));
}

// Returns the 2D approximate distance between two persons' pelvis bones.
// uc_orig: dist_to_target_pelvis (fallen/Source/Person.cpp)
SLONG dist_to_target_pelvis(Thing* p_person_a, Thing* p_person_b)
{
    SLONG dx, dz;
    SLONG ax, ay, az;
    SLONG bx, by, bz;

    calc_sub_objects_position(p_person_a, p_person_a->Draw.Tweened->AnimTween, SUB_OBJECT_PELVIS, &ax, &ay, &az);
    calc_sub_objects_position(p_person_b, p_person_b->Draw.Tweened->AnimTween, SUB_OBJECT_PELVIS, &bx, &by, &bz);

    dx = (p_person_a->WorldPos.X - p_person_b->WorldPos.X) >> 8;
    dz = (p_person_a->WorldPos.Z - p_person_b->WorldPos.Z) >> 8;

    dx += ax - bx;
    dz += az - bz;

    dx = abs(dx);
    dz = abs(dz);

    return (QDIST2(dx, dz));
}

// Returns true if person's current substate is a crouching animation.
// uc_orig: is_person_crouching (fallen/Source/Person.cpp)
SLONG is_person_crouching(Thing* p_person)
{
    ASSERT(p_person->Class == CLASS_PERSON);

    return p_person->SubState == SUB_STATE_CRAWLING || p_person->SubState == SUB_STATE_STOP_CRAWL || p_person->SubState == SUB_STATE_IDLE_CROUTCH || p_person->SubState == SUB_STATE_IDLE_CROUTCHING;
}

// Returns true if person_a can see person_b.
// range=0: use default 8<<8 units. range<0: override view conditions with abs(range).
// range>0: clip computed view distance to this value.
// no_los=1: skip geometric line-of-sight check (FOV and range only).
// Visibility is reduced in darkness and when the target is crouching.
// uc_orig: can_a_see_b (fallen/Source/Person.cpp)
SLONG can_a_see_b(
    Thing* p_person_a,
    Thing* p_person_b, SLONG range, SLONG no_los)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG view;
    SLONG dist;
    SLONG angle;
    SLONG dangle;
    SLONG p_person_b_moving;

    if (p_person_a->Flags & FLAGS_PERSON_BEEN_SHOT) {
        range = -(9 << 8);
    }

    if (p_person_a->Genus.Person->Ware != p_person_b->Genus.Person->Ware) {
        // One person is in a warehouse, the other isn't — can't see each other.
        return FALSE;
    }

    if (range == 0) {
        range = 8 << 8;
    }

    p_person_b_moving = TRUE;

    if (p_person_b->State == STATE_IDLE || (p_person_b->State == STATE_GUN && p_person_b->SubState == SUB_STATE_AIM_GUN)) {
        p_person_b_moving = FALSE;
    }

    dx = p_person_b->WorldPos.X - p_person_a->WorldPos.X;
    dy = p_person_b->WorldPos.Y - p_person_a->WorldPos.Y;
    dz = p_person_b->WorldPos.Z - p_person_a->WorldPos.Z;

    dx >>= 8;
    dy >>= 8;
    dz >>= 8;

    dist = QDIST2(abs(dx), abs(dz));

    if (range < 0) {
        // Negative range: ignore ambient conditions, use abs value as max view.
        view = -range;
    } else {
        if (p_person_a->Genus.Person->PlayerID) {
            view = 256 << 8;
        } else {
            NIGHT_Colour col;

            col = NIGHT_get_light_at(
                p_person_b->WorldPos.X >> 8,
                p_person_b->WorldPos.Y >> 8,
                p_person_b->WorldPos.Z >> 8);

            // Brighter light = see further.
            view = col.red + col.green + col.blue;
            view += view << 3;
            view += view >> 2;
            view += 256;
        }

        if (is_person_crouching(p_person_b)) {
            view >>= 1;
        }

        if (p_person_b_moving) {
            view += 256;
        }

        if (view > range) {
            view = range;
        }
    }

    if (dist > view) {
        return FALSE;
    }

    {
        angle = Arctan(dx, -dz) + 1024;
        angle &= 2047;
        dangle = angle - p_person_a->Draw.Tweened->Angle;
        dangle &= 2047;

        if (dist < 0xc0) {
            // Very close — use a wide FOV of ~123 degrees.
            if (dangle < 700 || dangle > 2048 - 700) {
                return TRUE;
            } else {
                return FALSE;
            }
        }

// uc_orig: PEOPLE_FOV (fallen/Source/Person.cpp)
#define PEOPLE_FOV 420

        if (dangle < PEOPLE_FOV || dangle > 2048 - PEOPLE_FOV) {
            if (!p_person_b_moving) {
// uc_orig: CORNER_OF_EYE_FOV (fallen/Source/Person.cpp)
#define CORNER_OF_EYE_FOV 250
                if (WITHIN(dangle, CORNER_OF_EYE_FOV, 2048 - CORNER_OF_EYE_FOV)) {
                    if (dist > (view >> 1)) {
                        // Stationary target at edge of vision — too far to notice.
                        return FALSE;
                    }
                }
            }

            UBYTE ahead = (is_person_crouching(p_person_a)) ? 0x20 : 0x60;
            UBYTE bhead = (is_person_crouching(p_person_b)) ? 0x20 : 0x60;

            if (no_los)
                return (TRUE);

            if (p_person_b->Genus.Person->Ware) {
                // In warehouse: use LOS but skip underground geometry check.
                if (there_is_a_los(
                        (p_person_a->WorldPos.X >> 8),
                        (p_person_a->WorldPos.Y >> 8) + ahead,
                        (p_person_a->WorldPos.Z >> 8),
                        (p_person_b->WorldPos.X >> 8),
                        (p_person_b->WorldPos.Y >> 8) + bhead,
                        (p_person_b->WorldPos.Z >> 8),
                        LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                    return TRUE;
                }
            } else {
                if (there_is_a_los(
                        (p_person_a->WorldPos.X >> 8),
                        (p_person_a->WorldPos.Y >> 8) + ahead,
                        (p_person_a->WorldPos.Z >> 8),
                        (p_person_b->WorldPos.X >> 8),
                        (p_person_b->WorldPos.Y >> 8) + bhead,
                        (p_person_b->WorldPos.Z >> 8),
                        0)) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

#undef PEOPLE_FOV
#undef CORNER_OF_EYE_FOV

// Returns true if person can see the given world-space point within their FOV and LOS.
// Uses a fixed range of 0x600 units (no light-based adjustment).
// uc_orig: can_i_see_place (fallen/Source/Person.cpp)
SLONG can_i_see_place(Thing* p_person, SLONG x, SLONG y, SLONG z)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG view;
    SLONG dist;
    SLONG angle;
    SLONG dangle;

    dx = x - (p_person->WorldPos.X >> 8);
    dy = y - (p_person->WorldPos.Y >> 8);
    dz = z - (p_person->WorldPos.Z >> 8);

    dist = QDIST2(abs(dx), abs(dz));

    if (dist > 0x600) {
        return FALSE;
    }

    if (abs(dy) > abs(dist >> 1)) {
        return FALSE;
    }

    angle = Arctan(dx, -dz) + 1024;
    angle &= 2047;
    dangle = angle - p_person->Draw.Tweened->Angle;
    dangle &= 2047;

// uc_orig: PEOPLE_FOV (fallen/Source/Person.cpp)
#define PEOPLE_FOV 420

    if (dangle < PEOPLE_FOV || dangle > 2048 - PEOPLE_FOV) {
        UBYTE ahead = (is_person_crouching(p_person)) ? 0x20 : 0x60;

        if (there_is_a_los(
                (p_person->WorldPos.X >> 8),
                (p_person->WorldPos.Y >> 8) + ahead,
                (p_person->WorldPos.Z >> 8),
                x,
                y + 0x20,
                z,
                0)) {
            return TRUE;
        }
    }

    return FALSE;
}

#undef PEOPLE_FOV

// Enters the sliding tackle state toward p_target. No-ops if already sliding.
// uc_orig: set_person_sliding_tackle (fallen/Source/Person.cpp)
void set_person_sliding_tackle(Thing* p_person, Thing* p_target)
{
    if (p_person->SubState != SUB_STATE_RUNNING_SKID_STOP) {
        set_face_thing(p_person, p_target);
        set_generic_person_state_function(p_person, STATE_MOVEING);
        set_anim(p_person, ANIM_SLIDER_START);
        p_person->SubState = SUB_STATE_RUNNING_SKID_STOP;
    }
}

// Attempts to vault over the fence at facet. Returns true if vault was started.
// uc_orig: set_person_vault (fallen/Source/Person.cpp)
SLONG set_person_vault(Thing* p_person, SLONG facet)
{
    if (set_person_pos_for_fence_vault(p_person, facet)) {
        set_anim(p_person, ANIM_VAULT);
        p_person->SubState = SUB_STATE_RUNNING_VAULT;
        p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;

        set_generic_person_state_function(p_person, STATE_MOVEING);

        return TRUE;
    }

    return FALSE;
}

// Attempts to step up a half-height obstacle at facet. Returns true if climb was started.
// uc_orig: set_person_climb_half (fallen/Source/Person.cpp)
SLONG set_person_climb_half(Thing* p_person, SLONG facet)
{
    if (set_person_pos_for_half_step(p_person, facet)) {
        set_anim(p_person, ANIM_GET_UP_HALF_BLOCK);
        p_person->SubState = SUB_STATE_RUNNING_HALF_BLOCK;
        p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;

        set_generic_person_state_function(p_person, STATE_MOVEING);

        return TRUE;
    }

    return FALSE;
}

// Returns true if person can see player 0.
// uc_orig: can_i_see_player (fallen/Source/Person.cpp)
SLONG can_i_see_player(Thing* p_person)
{
    return can_a_see_b(p_person, NET_PERSON(0));
}

// If person can see player 0, sets them as target and starts navigation toward them.
// uc_orig: do_look_for_enemies (fallen/Source/Person.cpp)
void do_look_for_enemies(Thing* p_person)
{
    if (can_i_see_player(p_person)) {
        p_person->Genus.Person->Target = THING_NUMBER(NET_PERSON(0));
        ASSERT(p_person->Genus.Person->Target != THING_NUMBER(p_person));
        ASSERT(TO_THING(p_person->Genus.Person->Target)->Class == CLASS_PERSON);
        p_person->Genus.Person->Flags |= FLAG_PERSON_NAV_TO_KILL;

        set_person_mav_to_thing(p_person, NET_PERSON(0));
    }
}

// --- chunk 3: lines 3202–4739 of original Person.cpp ---

// get_person_radius() is defined in collide.cpp but not declared in any header.
extern SLONG get_person_radius(SLONG type);
// calc_sub_objects_position_fix8 is declared in Person.cpp at line 4425 (extern inline).
extern void calc_sub_objects_position_fix8(Thing* p_mthing, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z);

// Per-frame player-specific updates: fight-mode correction, gang tracking, boredom timer,
// and camera roll damping during running.
// uc_orig: general_process_player (fallen/Source/Person.cpp)
void general_process_player(Thing* p_person)
{
    DrawTween* dt;

    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        if (p_person->State == STATE_MOVEING && p_person->SubState == SUB_STATE_RUNNING) {
            p_person->Genus.Person->Mode = PERSON_MODE_RUN;
        }
    }

    if (p_person->Genus.Person->GangAttack) {
        extern void check_players_gang(Thing * p_target);
        check_players_gang(p_person);
    }

    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {

        if (p_person->Genus.Person->Target) {
            Thing* p_target;

            p_target = TO_THING(p_person->Genus.Person->Target);

            if (is_person_dead(p_target)) {
                fight_any_gang_attacker(p_person);
            }
        } else if (count_gang(p_person)) {
            fight_any_gang_attacker(p_person);
        }

        timer_bored = 0;
    } else {
        if (!EWAY_stop_player_moving())
            timer_bored += TICK_TOCK;
    }

    dt = p_person->Draw.Tweened;
    if (p_person->SubState == SUB_STATE_RUNNING) {
        if (dt->Roll || dt->DRoll) {
            if (dt->DRoll) {
                if (abs(dt->Roll) < 70)
                    dt->Roll -= dt->DRoll;

                if (dt->Roll > 0 && dt->DRoll > 0)
                    dt->Roll = 0;
                else if (dt->Roll < 0 && dt->DRoll < 0)
                    dt->Roll = 0;
                dt->DRoll = 0;

            } else {
                // uses 2's complement maths shift feature
                if (dt->Roll < 0)
                    dt->Roll -= (dt->Roll >> 1);
                if (dt->Roll > 0)
                    dt->Roll += (-dt->Roll) >> 1;
            }
        }
        if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
            TO_THING(p_person->Genus.Person->Target)->Draw.Tweened->Roll = (2048 - dt->Roll) & 2047;
        }

    } else {
        if (p_person->SubState == SUB_STATE_WALKING) {
            // (sneak-mode detection commented out in original)
        }

        if (p_person->SubState != SUB_STATE_RIDING_BIKE) {
            if (dt->Roll < 0)
                dt->Roll -= (dt->Roll >> 2);
            if (dt->Roll > 0)
                dt->Roll += (-dt->Roll) >> 2;
        }
    }
}

// Selects the best attack target for the player from nearby persons who want to kill them.
// dir=1 cycles forward (next higher thing index), dir=-1 cycles backward.
// uc_orig: person_pick_best_target (fallen/Source/Person.cpp)
void person_pick_best_target(Thing* p_person, SLONG dir)
{
    SLONG i;

    SLONG dx;
    SLONG dz;
    SLONG dist;
    SLONG best_dist = INFINITY;

    UWORD lowest_person = 0xffff;
    UWORD highest_person = 0;
    UWORD next_person = 0xffff;
    UWORD prev_person = 0;

    SLONG num_found = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        0x300,
        THING_array,
        THING_ARRAY_SIZE,
        1 << CLASS_PERSON);

    for (i = 0; i < num_found; i++) {
        Thing* p_found = TO_THING(THING_array[i]);

        if (p_found == p_person) {
            continue;
        }

        if (p_found->State == STATE_DEAD) {
            continue;
        }

        if (PCOM_person_wants_to_kill(p_found) == THING_NUMBER(p_person)) {
            if (p_person->Genus.Person->Target == NULL) {
                dx = abs(p_found->WorldPos.X - p_person->WorldPos.X);
                dz = abs(p_found->WorldPos.Z - p_person->WorldPos.Z);

                dist = QDIST2(dx, dz);

                if (dist < best_dist) {
                    best_dist = dist;
                    next_person = THING_array[i];
                }
            } else {
                if (THING_array[i] < lowest_person) {
                    lowest_person = THING_array[i];
                }
                if (THING_array[i] > highest_person) {
                    highest_person = THING_array[i];
                }

                if (THING_array[i] > p_person->Genus.Person->Target) {
                    if (THING_array[i] < next_person) {
                        next_person = THING_array[i];
                    }
                } else if (THING_array[i] < p_person->Genus.Person->Target) {
                    if (THING_array[i] > prev_person) {
                        prev_person = THING_array[i];
                    }
                }
            }
        }
    }

    if (dir == 1) {
        if (next_person != 0xffff) {
            p_person->Genus.Person->Target = next_person;
        } else if (lowest_person != 0xffff) {
            p_person->Genus.Person->Target = lowest_person;
        }
    } else {
        if (prev_person != 0xffff) {
            p_person->Genus.Person->Target = prev_person;
        } else if (highest_person != 0xffff) {
            p_person->Genus.Person->Target = highest_person;
        }
    }

    if (p_person->Genus.Person->Target) {
        turn_to_face_thing(p_person, TO_THING(p_person->Genus.Person->Target), 0);
    }
}

// Per-frame person update called after the state function.
// Handles: moving platforms (WMOVE), death/dying exit, falling off map (GF_NO_FLOOR),
// stamina recovery, burning (fire damage + immolate pyro), residual burn damage (BurnIndex),
// urination particle effect, bleeding, grappling hook positioning, and player-specific update.
// uc_orig: general_process_person (fallen/Source/Person.cpp)
void general_process_person(Thing* p_person)
{
    // every 64 turns clear the been-shot flag (which grants enhanced vision range)
    if ((PTIME(p_person) & 63) == 0) {
        p_person->Flags &= ~FLAGS_PERSON_BEEN_SHOT;
    }

    if (p_person->OnFace > 0) {
        ASSERT(WITHIN(p_person->OnFace, 1, next_prim_face4 - 1));

        PrimFace4* f4 = &prim_faces4[p_person->OnFace];

        ASSERT(f4->FaceFlags & FACE_FLAG_WALKABLE);

        if (f4->FaceFlags & FACE_FLAG_WMOVE) {
            SLONG now_x;
            SLONG now_y;
            SLONG now_z;
            SLONG now_dangle;
            SLONG wmove_index;

            wmove_index = f4->ThingIndex;
            ASSERT(WITHIN(wmove_index, 1, WMOVE_face_upto - 1));

            WMOVE_relative_pos(
                f4->ThingIndex,
                p_person->WorldPos.X,
                p_person->WorldPos.Y,
                p_person->WorldPos.Z,
                &now_x,
                &now_y,
                &now_z,
                &now_dangle);

            p_person->Draw.Tweened->Angle += now_dangle;
            p_person->Draw.Tweened->Angle &= 2047;

            GameCoord newpos;

            newpos.X = now_x;
            newpos.Y = now_y;
            newpos.Z = now_z;

            if (WITHIN(newpos.X, 2 << 16, (PAP_SIZE_HI - 3) << 16) && WITHIN(newpos.Z, 2 << 16, (PAP_SIZE_HI - 3) << 16)) {

                move_thing_on_map(p_person, &newpos);
            } else {
                // Too close to the edge of the map; fall off
                p_person->OnFace = 0;

                set_person_dead(
                    p_person,
                    NULL,
                    PERSON_DEATH_TYPE_STAY_ALIVE,
                    0,
                    0);
            }
        }
    }

    if (p_person->State == STATE_DEAD || p_person->State == STATE_DYING) {
        return;
    }

    if (GAME_FLAGS & GF_NO_FLOOR) {
        if (p_person->WorldPos.Y < -0x180000) {
            p_person->Genus.Person->Health = 0;

            set_person_dead(p_person, NULL, PERSON_DEATH_TYPE_PRONE, 0, 0);

            remove_thing_from_map(p_person);

            return;
        }
    }

    {
        UWORD max_stamina = 128;
        if (p_person->Genus.Person->PlayerID) {
            max_stamina += NET_PLAYER(p_person->Genus.Person->PlayerID - 1)->Genus.Player->Stamina;
            if (!continue_pressing_action(p_person)) {
                if (p_person->Genus.Person->Stamina < max_stamina) {
                    p_person->Genus.Person->Stamina++;

                    if (p_person->Genus.Person->Stamina == 5) {
                        p_person->Genus.Person->Stamina = 20;
                    }
                }
            }
        } else if (p_person->Genus.Person->Stamina < max_stamina) {
            p_person->Genus.Person->Stamina++;
        }
    }

    SLONG get_person_radius(SLONG type);

    if (p_person->Flags & FLAGS_BURNING) {
        SLONG x2, y2, z2, ndx;
        Thing* thing;
        Pyro* pyro;

        ndx = p_person->Genus.Person->BurnIndex;
        if ((!ndx) || ((pyro = TO_PYRO(ndx - 1))->PyroType == PYRO_NONE)) {
            thing = PYRO_create(p_person->WorldPos, PYRO_IMMOLATE);
            if (thing) {
                pyro = thing->Genus.Pyro;
                pyro->victim = p_person;
                pyro->Flags = PYRO_FLAGS_FLICKER;
                p_person->Genus.Person->BurnIndex = PYRO_NUMBER(pyro) + 1;
                thing->StateFn(thing);
            }
        } else {
            // person is in water — accelerate pyro death so fire goes out
            if (PAP_2HI(p_person->WorldPos.X >> 16, p_person->WorldPos.Z >> 16).Flags & PAP_FLAG_WATER) {
                if (pyro->PyroType == PYRO_IMMOLATE) {
                    pyro->Dummy = 2;
                    pyro->radius = 290;
                }
            }
        }

        if (p_person->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL) {
            // Only the player can hurt this person — burning does nothing
        } else if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
            // Invulnerable — burning does nothing
        } else {
            p_person->Genus.Person->Health -= 30;
        }

        if (p_person->Genus.Person->Health <= 0) {
            p_person->Genus.Person->Health = 0;

            set_person_dead(
                p_person,
                NULL,
                PERSON_DEATH_TYPE_OTHER,
                FALSE,
                0);

        } else {

            p_person->Flags &= ~FLAGS_BURNING;
            collide_against_things(
                p_person,
                get_person_radius(p_person->Genus.Person->PersonType),
                p_person->WorldPos.X, p_person->WorldPos.Y, p_person->WorldPos.Z,
                &x2, &y2, &z2);
        }
    }

    // Residual burn damage from immolate pyro (1 HP per tick until person is healthy or dead).
    if (p_person->Genus.Person->BurnIndex && (p_person->Genus.Person->Health > 0))
        p_person->Genus.Person->Health--;

    if (p_person->Genus.Person->Flags & FLAG_PERSON_PEEING) {
        if (p_person->Flags & FLAGS_IN_VIEW) {
            SLONG penis_x;
            SLONG penis_y;
            SLONG penis_z;

            SLONG dx;
            SLONG dz;

            calc_sub_objects_position(
                p_person,
                p_person->Draw.Tweened->AnimTween,
                SUB_OBJECT_PELVIS,
                &penis_x,
                &penis_y,
                &penis_z);

            penis_x += p_person->WorldPos.X >> 8;
            penis_y += p_person->WorldPos.Y >> 8;
            penis_z += p_person->WorldPos.Z >> 8;

            penis_y -= 0x10;

            dx = -SIN(p_person->Draw.Tweened->Angle) >> 13;
            dz = -COS(p_person->Draw.Tweened->Angle) >> 13;

            DIRT_new_water(
                penis_x,
                penis_y,
                penis_z,
                dx, -2, dz,
                DIRT_TYPE_URINE);
        }
    }

    // Bleeding: persons below 25% health randomly leave blood trails.
    if (p_person->Genus.Person->Health < health[p_person->Genus.Person->PersonType] >> 2) {
        if (p_person->Genus.Person->Stamina > 50)
            p_person->Genus.Person->Stamina -= 1;

        if (!p_person->Genus.Person->InCar) {
            if (((Random() & 0x7f) > p_person->Genus.Person->Health) && (Random() & 1))
                TRACKS_Bleed(p_person);
        }
    }

    // Grappling hook: update hook position to follow Darci's right hand.
    if (p_person->Genus.Person->Flags & FLAG_PERSON_GRAPPLING) {
        SLONG percent;
        SLONG pitch;

        if (p_person->State == STATE_GRAPPLING && p_person->SubState == SUB_STATE_GRAPPLING_WINDUP) {
            percent = p_person->Draw.Tweened->FrameIndex << 8;
            percent |= p_person->Draw.Tweened->AnimTween;
            pitch = (-percent * 2048) / 0x500;
            pitch &= 2047;
        } else {
            pitch = 1536;
        }

        SLONG px;
        SLONG py;
        SLONG pz;

        calc_sub_objects_position(
            p_person,
            p_person->Draw.Tweened->AnimTween,
            SUB_OBJECT_RIGHT_HAND,
            &px,
            &py,
            &pz);

        px += p_person->WorldPos.X >> 8;
        py += p_person->WorldPos.Y >> 8;
        pz += p_person->WorldPos.Z >> 8;

        HOOK_spin(
            px,
            py,
            pz,
            p_person->Draw.Tweened->Angle,
            -pitch);
    }

    // Clear request-kick flag unless person is running-jumping, fighting, or grappling.
    if ((p_person->Genus.Person->Action != ACTION_RUN_JUMP) && (p_person->Genus.Person->Action != ACTION_FIGHT_PUNCH) && (p_person->Genus.Person->Action != ACTION_GRAPPLE)) {
        p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_KICK;
    }

    if (p_person->Genus.Person->PlayerID) {
        general_process_player(p_person);
    }
}

// Returns non-zero if person is on a slope steep enough to cause sliding (> 50 units).
// If slipping, redirects the person downhill along the slope normal.
// Also teleports stuck-in-NOGO persons to the nearest safe tile.
// uc_orig: check_on_slippy_slope (fallen/Source/Person.cpp)
SLONG check_on_slippy_slope(Thing* p_person)
{
    SLONG slope, angle;
    SLONG size = 50;

    if (p_person->Genus.Person->InsideIndex) {
        slope = 0;
    } else {
        if (p_person->OnFace < 0) {
            slope = RFACE_on_slope(-p_person->OnFace, p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle);
        } else {

            slope = PAP_on_slope(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle) >> 1;
        }
    }

    if (slope > size) {
        switch (p_person->SubState) {
        default:
        case SUB_STATE_RUNNING_JUMP:
        case SUB_STATE_RUNNING_JUMP_LAND_FAST:
        case SUB_STATE_RUNNING_JUMP_LAND:
            set_generic_person_state_function(p_person, STATE_MOVEING);
        case SUB_STATE_RUNNING:
        case SUB_STATE_WALKING:
        case SUB_STATE_WALKING_BACKWARDS:
        case SUB_STATE_FLIPING:
        case SUB_STATE_RUNNING_SKID_STOP:

            set_anim(p_person, ANIM_FALLING);

        case SUB_STATE_SLIPPING:
            p_person->SubState = SUB_STATE_SLIPPING;
            p_person->Draw.Tweened->AngleTo = angle;

            slope = MIN(slope - size, 10);
            slope = MAX(slope, size);

            change_velocity_to(p_person, slope);

            break;
        }

        if (p_person->OnFace == 0 && p_person->Genus.Person->Ware == 0)
            if (PAP_2HI((p_person->WorldPos.X >> 16) & 127, (p_person->WorldPos.Z >> 16) & 127).Flags & PAP_FLAG_NOGO) {
                // Teleport person to nearest non-NOGO tile that isn't too steep.
                SLONG angle, step;
                for (step = 64; step < 512; step += 64) {
                    for (angle = 0; angle < 2048; angle += 256) {
                        SLONG dx, dz;
                        dx = (COS(angle) * step) >> 8;
                        dz = (SIN(angle) * step) >> 8;
                        if (!(PAP_2HI(((p_person->WorldPos.X + dx) >> 16) & 127, ((p_person->WorldPos.Z + dz) >> 16) & 127).Flags & PAP_FLAG_NOGO)) {
                            GameCoord newpos;

                            slope = PAP_on_slope((p_person->WorldPos.X + dx) >> 8, (p_person->WorldPos.Z + dz) >> 8, &angle) >> 1;
                            if (slope < 40) {

                                newpos.X = p_person->WorldPos.X + dx;
                                newpos.Z = p_person->WorldPos.Z + dz;
                                newpos.Y = PAP_calc_map_height_at(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8);

                                move_thing_on_map(p_person, &newpos);

                                step = 50000;
                                break;
                            }
                        }
                    }
                }
            }

        return (1);
    } else if (p_person->SubState == SUB_STATE_SLIPPING) {
        p_person->SubState = SUB_STATE_SLIPPING_END;
    }
    return (0);
}

// Temporarily moves person dist units forward and checks for a slippy slope ahead.
// Used to pre-check whether moving forward would put person on a slide.
// uc_orig: slope_ahead (fallen/Source/Person.cpp)
SLONG slope_ahead(Thing* p_person, SLONG dist)
{
    SLONG dx;
    SLONG dz;
    SLONG slippy;

    dx = -(SIN(p_person->Draw.Tweened->Angle) * dist) >> 8;
    dz = -(COS(p_person->Draw.Tweened->Angle) * dist) >> 8;

    p_person->WorldPos.X += dx;
    p_person->WorldPos.Z += dz;

    slippy = check_on_slippy_slope(p_person);

    p_person->WorldPos.X -= dx;
    p_person->WorldPos.Z -= dz;

    return (slippy);
}

// Moves person by (dx, dz) with height-tracking and collision (via move_thing).
// Respects animation-locked movement, turning penalty, step-down detection,
// and slope-slip checks. Called by person_normal_move and person_normal_move_check.
// uc_orig: person_normal_move_dxdz (fallen/Source/Person.cpp)
void person_normal_move_dxdz(Thing* p_person, SLONG dx, SLONG dz)
{
    SLONG dy;
    SLONG new_y;
    SLONG on_face;

    slide_ladder = 0;

    if (p_person->Draw.Tweened->Locked) {
        // The movement is part of the animation.
        return;
    }

    p_person->Genus.Person->Flags &= ~FLAG_PERSON_HIT_WALL;

    dy = 0;

    // Move the person slower if they are at the wrong angle.
    SLONG dangle = p_person->Draw.Tweened->AngleTo - p_person->Draw.Tweened->Angle;
    SLONG dspeed = 300 - abs(dangle);

    SATURATE(dspeed, 0, 256);

    SLONG ratio = TICK_RATIO * dspeed >> 8;

    // Don't apply turn-speed penalty (disabled in original).
    ratio = TICK_RATIO;

    dx = dx * ratio >> TICK_SHIFT;
    dz = dz * ratio >> TICK_SHIFT;

    if (allow_debug_keys)
        if (ShiftFlag && Keys[KB_Q]) {
            dx <<= 2;
            dz <<= 2;
        }

    // Work out the new y-position based on terrain or face height.
    if (p_person->OnFace) {
        if (p_person->OnFace > 0) {
            on_face = calc_height_on_face(
                (p_person->WorldPos.X + dx) >> 8,
                (p_person->WorldPos.Z + dz) >> 8,
                p_person->OnFace,
                &new_y);
        } else {
            on_face = calc_height_on_rface(
                (p_person->WorldPos.X + dx) >> 8,
                (p_person->WorldPos.Z + dz) >> 8,
                -p_person->OnFace,
                &new_y);
        }

        if (!on_face) {
            // Walked off the current face — let move_thing sort out what happens.
            new_y = p_person->WorldPos.Y;
        } else {
            MSG_add("normal move height %d \n", new_y);
            new_y = (new_y) << 8;
        }
    } else {
        {
            SLONG mx, mz;

            mx = (p_person->WorldPos.X + dx) >> 16;
            mz = (p_person->WorldPos.Z + dz) >> 16;
            mx &= 127;
            mz &= 127;

            if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN)
                new_y = PAP_calc_height_at_thing(p_person, (p_person->WorldPos.X + 0 * dx) >> 8, (p_person->WorldPos.Z + 0 * dz) >> 8) << 8;
            else
                new_y = PAP_calc_height_at_thing(p_person, (p_person->WorldPos.X + dx) >> 8, (p_person->WorldPos.Z + dz) >> 8) << 8;
        }
    }

    dy = new_y - p_person->WorldPos.Y;

    if (p_person->SubState != SUB_STATE_SLIPPING)
        if (dy < -60 << 8) {
            set_person_drop_down(p_person, PERSON_DROP_DOWN_KEEP_VEL);
            return;
        }

    if (dx || dz) {
        move_thing(dx, dy, dz, p_person);
    }

    if (dy || p_person->Genus.Person->PlayerID) {
        check_on_slippy_slope(p_person);

    } else {
        if (p_person->SubState == SUB_STATE_SLIPPING) {
            SLONG slope, angle;
            if (p_person->Genus.Person->InsideIndex) {
                slope = 0;
            } else {

                if (p_person->OnFace < 0) {
                    slope = RFACE_on_slope(-p_person->OnFace, p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle);
                } else {

                    slope = PAP_on_slope(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle) >> 1;
                }
            }

            if (slope <= 50)
                p_person->SubState = SUB_STATE_SLIPPING_END;
        }
    }
}

// Moves person forward along their current facing angle by their Velocity.
// uc_orig: person_normal_move (fallen/Source/Person.cpp)
void person_normal_move(Thing* p_person)
{
    SLONG dx;
    SLONG dz;

    dx = -(SIN(p_person->Draw.Tweened->Angle) * p_person->Velocity) >> 8;
    dz = -(COS(p_person->Draw.Tweened->Angle) * p_person->Velocity) >> 8;
    person_normal_move_dxdz(p_person, dx, dz);
}

// Like person_normal_move, but skips movement if target position is outside map bounds.
// uc_orig: person_normal_move_check (fallen/Source/Person.cpp)
void person_normal_move_check(Thing* p_person)
{
    SLONG dx;
    SLONG dz;
    SLONG x, z;

    dx = -(SIN(p_person->Draw.Tweened->Angle) * p_person->Velocity) >> 8;
    dz = -(COS(p_person->Draw.Tweened->Angle) * p_person->Velocity) >> 8;
    x = dx + p_person->WorldPos.X;
    z = dz + p_person->WorldPos.Z;

    if (x < 0 || z < 0 || x >= (128 << 16) || z >= (128 << 16))
        return;

    person_normal_move_dxdz(p_person, dx, dz);
}

// Advances one keyframe in the animation, handling queued frames and looping.
// Returns 1 if animation has ended (looped or stopped), 2 if a queued frame was loaded.
// uc_orig: advance_keyframe (fallen/Source/Person.cpp)
SLONG advance_keyframe(DrawTween* draw_info)
{
    SLONG ret = 0;
    draw_info->CurrentFrame = draw_info->NextFrame;
    if (draw_info->QueuedFrame) {
        draw_info->NextFrame = draw_info->QueuedFrame;
        draw_info->QueuedFrame = 0;
        draw_info->FrameIndex = 0;
        ret = 2; // anim has ended
        ASSERT(draw_info->CurrentFrame->FirstElement);
        ASSERT(draw_info->NextFrame->FirstElement);

    } else {
        if (draw_info->NextFrame->NextFrame) {
            draw_info->NextFrame = draw_info->NextFrame->NextFrame;
            if (draw_info->CurrentFrame->Flags & ANIM_FLAG_LAST_FRAME)
                ret = 1; // hopefully escape funny lock out situations by saying a looped anim has ended

        } else
            ret = 1; // anim has ended
    }
    return (ret);
}

// Retreats one keyframe in the animation (for backwards playback).
// Returns 1 if the start of the animation has been reached.
// uc_orig: retreat_keyframe (fallen/Source/Person.cpp)
SLONG retreat_keyframe(DrawTween* draw_info)
{
    SLONG ret = 0;
    draw_info->NextFrame = draw_info->CurrentFrame;
    if (draw_info->QueuedFrame) {
        // queued-frame retreat not implemented in original
    } else {
        if (draw_info->CurrentFrame->PrevFrame) {
            draw_info->CurrentFrame = draw_info->CurrentFrame->PrevFrame;
        } else {
            ret = 1; // anim has ended
        }
    }
    return (ret);
}

// Adjusts person world position to keep a locked limb stationary between two tween values.
// t1=tween at start of frame, t2=tween at end of frame. Uses fix8 (256-scale) positions.
// uc_orig: move_locked_tween (fallen/Source/Person.cpp)
void move_locked_tween(Thing* p_person, DrawTween* dt, SLONG t1, SLONG t2)
{
    SLONG x1, y1, z1;
    SLONG x2, y2, z2;
    SLONG dx, dy, dz;

    calc_sub_objects_position_fix8(p_person, t1, abs(dt->Locked), &x1, &y1, &z1);
    calc_sub_objects_position_fix8(p_person, t2, abs(dt->Locked), &x2, &y2, &z2);

    dx = (x1 - x2);
    dy = (y1 - y2);
    dz = (z1 - z2);

    if (abs(dy) > 600 << 8) {
        ASSERT(0);
        calc_sub_objects_position_fix8(p_person, t1, abs(dt->Locked), &x1, &y1, &z1);
        calc_sub_objects_position_fix8(p_person, t2, abs(dt->Locked), &x2, &y2, &z2);
    }
    if (p_person->State == STATE_DANGLING) {
        move_thing_quick(dx, dy, dz, p_person);
        MSG_add(" mtq dx %d dy %d dz %d \n", dx, dy, dz);
    } else {
        move_thing(dx, dy, dz, p_person);
        MSG_add(" mt dx %d dy %d dz %d \n", dx, dy, dz);
    }
}

// Animates person forward at the given speed (256=normal). Advances tween and keyframes,
// applies locked-limb movement, handles fight-frame violence and barrel knockover.
// Returns 0=still running, 1=anim ended, 2=queued anim loaded.
// uc_orig: person_normal_animate_speed (fallen/Source/Person.cpp)
SLONG person_normal_animate_speed(Thing* p_person, SLONG speed)
{
    SLONG ret = 0;
    DrawTween* draw_info;
    SLONG old_tween;
    SLONG dx, dy, dz;
    SLONG tween1, tween2;

    draw_info = p_person->Draw.Tweened;

    if (draw_info->CurrentFrame == 0 || draw_info->NextFrame == 0) {
        MSG_add(" !!!!!!!!!!!!!!!!!!!!!!error2 animate 0 frames \n");
        return (1);
    }
    old_tween = draw_info->AnimTween;

    {
        SLONG tween_step = draw_info->CurrentFrame->TweenStep << 1;

        tween1 = draw_info->AnimTween;
        tween_step = (tween_step * TICK_RATIO) >> TICK_SHIFT;
        if (tween_step <= 0)
            tween_step = 1;
        draw_info->AnimTween += tween_step;
        tween2 = draw_info->AnimTween;
        if (draw_info->Locked && draw_info->AnimTween < 256) {
            move_locked_tween(p_person, draw_info, tween1, tween2);
        }
    }

    while (tween2 >= 256) {
        SLONG lock_x1, lock_y1, lock_z1, lock_x2, lock_y2, lock_z2;

        tween2 -= 256;

        if (draw_info->NextFrame)
            tween2 = (tween2 * draw_info->NextFrame->TweenStep) / draw_info->CurrentFrame->TweenStep;
        draw_info->AnimTween = tween2;

        if (draw_info->CurrentFrame->Flags & ANIM_FLAG_LAST_FRAME) {
            draw_info->FrameIndex = 0;
        } else {
            draw_info->FrameIndex++;
        }

        if (draw_info->Locked) {
            GameCoord temp_pos;
            SLONG locked;

            locked = abs(draw_info->Locked);

            calc_sub_objects_position_fix8(p_person, tween1, locked, &lock_x1, &lock_y1, &lock_z1);
            ret |= advance_keyframe(draw_info);
            calc_sub_objects_position_fix8(p_person, tween2, draw_info->Locked, &lock_x2, &lock_y2, &lock_z2);

            dx = (+lock_x1 - lock_x2);
            dy = (+lock_x1 - lock_x2); // NOTE: original uses lock_x1 for all axes (apparent copy-paste bug in original)
            dz = (+lock_x1 - lock_x2);

            if (p_person->State == STATE_DANGLING) {
                MSG_add("MOVE THING QUICK dx %d dy %d dz %d \n", dx, dy, dz);
                move_thing_quick(dx, dy, dz, p_person);
            } else {
                move_thing(dx, dy, dz, p_person);
                MSG_add("MOVE THING dx %d dy %d dz %d \n", dx, dy, dz);
            }

        } else {
            ret |= advance_keyframe(draw_info);
        }

        if (draw_info->CurrentFrame->Fight) {
            if (apply_violence(p_person) == 0) {
                extern BOOL PLAYCUTS_playing;
                if (!PLAYCUTS_playing)
                    MFX_play_thing(THING_NUMBER(p_person), S_KNIFE_START + (Random() & 1), 0, p_person);
            }

            // Knock over barrels within reach during fight frames.
            BARREL_hit_with_sphere(
                p_person->WorldPos.X >> 8,
                p_person->WorldPos.Y >> 8,
                p_person->WorldPos.Z >> 8,
                0xa0);
        }
    }

    if (ret == 1) {
    }

    return (ret);
}

// Animates person forward at normal speed (tween step from animation data).
// uc_orig: person_normal_animate (fallen/Source/Person.cpp)
SLONG person_normal_animate(Thing* p_person)
{
    return (person_normal_animate_speed(p_person, 256));
}

// Animates person backward (retreats keyframes). Handles locked-limb repositioning.
// Returns 1 when start of animation is reached.
// uc_orig: person_backwards_animate (fallen/Source/Person.cpp)
SLONG person_backwards_animate(Thing* p_person)
{
    SLONG ret = 0;
    DrawTween* draw_info;
    SLONG old_tween;
    SLONG tween_step;

    draw_info = p_person->Draw.Tweened;

    if (draw_info->CurrentFrame == NULL) {
        MSG_add(" backwards anim crash");
        return (1);
    }
    tween_step = draw_info->CurrentFrame->TweenStep << 1;

    tween_step = (tween_step * TICK_RATIO) >> TICK_SHIFT;

    old_tween = draw_info->AnimTween;

    draw_info->AnimTween -= tween_step;

    while (draw_info->AnimTween < 0) {
        SLONG lock_x1, lock_y1, lock_z1, lock_x2, lock_y2, lock_z2;

        draw_info->AnimTween += 256;

        if (draw_info->CurrentFrame->Flags & ANIM_FLAG_LAST_FRAME) {
            draw_info->FrameIndex = 0;
        } else {
            draw_info->FrameIndex++;
        }

        if (draw_info->Locked) {
            GameCoord temp_pos;
            SLONG locked;

            locked = abs(draw_info->Locked);

            calc_sub_objects_position(p_person, 0, locked, &lock_x1, &lock_y1, &lock_z1);
            calc_sub_objects_position(p_person, 256, locked, &lock_x2, &lock_y2, &lock_z2);
            ret = retreat_keyframe(draw_info);

            // Difference in lock coordinate is the amount to move to maintain the same limb position.
            temp_pos.X = ((+lock_x1 - lock_x2) << 8) + p_person->WorldPos.X;
            temp_pos.Y = ((+lock_y1 - lock_y2) << 8) + p_person->WorldPos.Y;
            temp_pos.Z = ((+lock_z1 - lock_z2) << 8) + p_person->WorldPos.Z;

            move_thing_on_map(p_person, &temp_pos);
        } else
            ret = retreat_keyframe(draw_info);

        if (draw_info->CurrentFrame->Fight) {
            apply_violence(p_person);
        }
    }
    return (ret);
}

// Camera helpers — all bodies were removed (commented out) in the original before shipping.
// The camera system was moved to a separate camera module.

// uc_orig: camera_shoot (fallen/Source/Person.cpp)
void camera_shoot(void)
{
    // Camera mode adjustments removed from Person.cpp before shipping.
}

// uc_orig: camera_fight (fallen/Source/Person.cpp)
void camera_fight(void)
{
    // Camera mode adjustments removed from Person.cpp before shipping.
}

// uc_orig: camera_normal (fallen/Source/Person.cpp)
void camera_normal(void)
{
    // Camera mode adjustments removed from Person.cpp before shipping.
}

