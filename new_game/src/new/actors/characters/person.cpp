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
