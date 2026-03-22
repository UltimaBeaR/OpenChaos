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
#include "actors/vehicles/vehicle.h"         // Temporary: VEH_find_door
#include "actors/vehicles/vehicle_globals.h" // Temporary: sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim
#include "effects/pyro_globals.h"   // Temporary: col_with[] array (shared collision buffer)
#include "actors/items/guns.h"      // Temporary: find_target_new
#include "actors/items/special.h"   // Temporary: alloc_special, special_drop, person_has_special
#include "ai/combat.h"              // Temporary: apply_hit_to_person
#include "effects/pyro.h"           // Temporary: PYRO_hitspang
#include "actors/animals/bat.h"     // Temporary: BAT_apply_hit
#include "actors/items/barrel.h"    // Temporary: BARREL_shoot
#include "actors/core/interact.h"   // Temporary: find_grab_face (chunk 6)
#include "fallen/Headers/prim.h"    // Temporary: which_side, two4_line_intersection, signed_dist_to_line_with_normal, does_fence_lie_along_line (chunk 6)

// External helpers declared in their own files (not yet migrated or in old headers).
extern BOOL allow_debug_keys;
// person_holding_2handed: now defined in this file (chunk 9 below)
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
// chunk 5 functions are now defined in this file (below) — no extern needed for:
//   set_person_locked_idle_ready, carry_running, set_person_stand_carry,
//   set_person_sidle, emergency_uncarry, set_anim_running, set_anim_idle, set_person_sneaking
extern SLONG remove_from_gang_attack(Thing* p_person, Thing* p_target);
extern SLONG continue_pressing_action(Thing* p_person);
extern void set_action_used(Thing* p_person);
extern UBYTE stealth_debug;
extern SLONG plant_feet(Thing* p_person);
extern SLONG calc_angle(SLONG dx, SLONG dz);
extern SLONG slide_ladder;
extern SLONG yomp_speed;
extern SLONG sprint_speed;
// find_face_for_this_pos declared via fallen/Headers/walkable.h above
extern void add_thing_to_map(Thing* p_thing);
extern SLONG people_allowed_to_hit_each_other(Thing* p_victim, Thing* p_agressor);
// locked_anim_change / locked_anim_change_of_type: now defined in person.cpp chunk 7 (below)
extern SLONG get_cable_along(SLONG facet, SLONG ax, SLONG az);
extern SWORD people_types[50];
// do_person_on_cable is defined below in chunk 8 (no extern needed)
// get_yomp_anim: now defined in this file (chunk 9 below)
// person_holding_bat: now defined in this file (chunk 9 below, wrapper around person_holding_special)
// chunk 4 additional externs (Person.cpp later chunks or other files not yet migrated)
extern Thing* is_person_under_attack_low_level(Thing* p_person, SLONG any_state, SLONG radius);
extern SLONG might_i_be_a_villain(Thing* p_person);
// chunk 5 additional externs (Person.cpp later chunks or other files not yet migrated)
extern void get_car_enter_xz(Thing* p_vehicle, SLONG door, SLONG* cx, SLONG* cz);
extern SLONG find_best_grapple(Thing* p_person);
extern UWORD PCOM_person_wants_to_kill(Thing* p_person);
extern SLONG continue_moveing(Thing* p_person); // interfac.cpp
extern SLONG person_is_on_sewer(Thing* p_person); // Person.cpp (not yet migrated)
// chunk 9 additional externs
extern SLONG mount_ladder(Thing* p_thing, SLONG facet); // collide.cpp
extern SLONG player_running_aim_gun(Thing* p_person); // Person.cpp (not yet migrated)
extern SLONG continue_firing(Thing* p_person); // interfac.cpp
extern void set_person_do_a_simple_anim(Thing* p_person, SLONG anim); // Person.cpp (not yet migrated)

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
// emergency_uncarry is defined in chunk 5 (below)
// fight_any_gang_attacker / set_person_pos_for_fence_vault / set_person_pos_for_half_step:
// now defined in person.cpp chunk 7 (below); declared in person.h
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

// ============================================================
// Chunk 4: set_person_aim..drop_all_items (original lines 4742-6140)
// ============================================================

// Sets person into gun-aim stance.
// locked != 0 forces a locked-anim transition at (locked-1) blend.
// uc_orig: set_person_aim (fallen/Source/Person.cpp)
void set_person_aim(Thing* p_person, SLONG locked)
{
    SLONG anim;
    Thing* p_special;

    if (p_person->Genus.Person->SpecialUse) {
        // Two-handed shotgun weapon — use shotgun aim.
        anim = ANIM_SHOTGUN_AIM;
    } else {
        anim = ANIM_PISTOL_AIM_AHEAD;
    }

    set_generic_person_state_function(p_person, STATE_GUN);
    if (locked)
        set_locked_anim(p_person, anim, locked - 1);
    else
        set_anim(p_person, anim);

    p_person->Genus.Person->Action = ACTION_AIM_GUN;
    p_person->SubState = SUB_STATE_AIM_GUN;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
}

// Returns an effective distance scaled for accuracy based on the weapon type.
// Negative distances are returned as-is (point-blank bonus).
// uc_orig: weapon_accuracy_at_dist (fallen/Source/Person.cpp)
static inline SLONG weapon_accuracy_at_dist(Thing* p_person, SLONG dist)
{
    if (dist < 0)
        return (dist);

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_AK47:
            if (!p_person->Genus.Person->PlayerID) {
                // AI needs high skill to use AK47 accurately.
                return ((dist * (280 - (GET_SKILL(p_person) << 2))) >> 8);
            } else {
                return ((dist * 200) >> 8);
            }
            break;

        case SPECIAL_SHOTGUN:
            return (dist >> 2);
            break;

        default:
            ASSERT(0);
            return 0;
        }

    } else {
        // Pistol — accuracy unmodified.
        return (dist);
    }
}

// Returns TRUE if the given vehicle is driven or occupied by any MIB agent.
// uc_orig: VehicleBelongsToMIB (fallen/Source/Person.cpp)
UBYTE VehicleBelongsToMIB(Thing* p_target)
{
    Vehicle* veh = p_target->Genus.Vehicle;
    Thing* thing;
    SWORD passenger;

    if ((p_target->Class != CLASS_VEHICLE) || !veh)
        return 0;

    if (veh->Driver) {
        thing = TO_THING(veh->Driver);
        if (PersonIsMIB(thing))
            return 1;
    }
    passenger = veh->Passenger;
    while (passenger) {
        thing = TO_THING(passenger);
        if (PersonIsMIB(thing))
            return 1;
        passenger = thing->Genus.Person->Passenger;
    }
    return 0;
}

// Computes shot damage from p_person to p_target.
// Returns 0 on miss. Sets *gun_type to HIT_TYPE_GUN_SHOT_*.
// Hit chance depends on target type, movement, attacker skill, and distance.
// uc_orig: get_shoot_damage (fallen/Source/Person.cpp)
SLONG get_shoot_damage(Thing* p_person, Thing* p_target, SLONG* gun_type)
{
    SLONG damage;

    SLONG dx = abs(p_target->WorldPos.X - p_person->WorldPos.X >> 8);
    SLONG dz = abs(p_target->WorldPos.Z - p_person->WorldPos.Z >> 8);
    SLONG dist = QDIST2(dx, dz);

    {
        SLONG chance; // 0 => never hit, >= 256 => guaranteed hit

        if (p_target->Class == CLASS_VEHICLE) {
            // Vehicles are too big to miss.
            chance = 256;
        } else if (p_target->Class == CLASS_BAT) {
            // Bats are tricky — but we want them easy to kill for fun.
            chance = 200;
        } else if (p_target->Class == CLASS_BARREL) {
            // Barrels don't move.
            chance = 200;
        } else if (p_target->Class == CLASS_SPECIAL) {
            // Mines are small, but the player takes a long time aiming.
            chance = 250;
        } else if (p_target->Class == CLASS_PERSON) {
            // Turning target is harder to hit; roll maxes at ~100.
            chance = 230 - (abs(p_target->Draw.Tweened->Roll) >> 1);
            // Moving target is harder to hit.
            chance -= p_target->Velocity;

            if (!p_person->Genus.Person->PlayerID) {
                if (p_target->Genus.Person->PlayerID) {
                    chance -= 64; // enemies are less accurate against player
                } else {
                    chance += 100; // AI vs AI: very accurate
                }
                chance += GET_SKILL(p_person) << 3;
            } else {
                chance += 64; // player is better than default
            }
            if (p_target->SubState == SUB_STATE_FLIPING) {
                chance -= 96;
            }
        } else {
            // Invalid target (e.g. freed barrel or mine): clear it.
            p_person->Genus.Person->Target = NULL;
            return 0;
        }

        // Distance penalty/bonus.
        if (p_target->Genus.Person->PlayerID) {
            dist -= 0x2a0;
        } else {
            dist -= 0x400;
        }
        if (dist < 0) {
            dist >>= 2;
        } else {
            dist >>= 3;
        }

        {
            SLONG dchance;
            dchance = chance;
            chance -= weapon_accuracy_at_dist(p_person, dist);
        }

        if (p_target->State == STATE_MOVEING && p_target->SubState == SUB_STATE_FLIPING) {
            chance >>= 1;
        }

        SATURATE(chance, 20, 256);

        if (chance < (Random() & 0xff)) {
            return 0; // missed
        }
    }

    if (p_target->Genus.Player->PlayerID) {
        dist -= (300 >> 3);
    }

    // Hit — determine damage by weapon type.
    if (PersonIsMIB(p_person)) {
        // MIB always carry AK47s.
        *gun_type = HIT_TYPE_GUN_SHOT_AK47;
        damage = 40;
    } else if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_AK47:
            *gun_type = HIT_TYPE_GUN_SHOT_AK47;
            if (p_person->Genus.Person->PlayerID) {
                damage = 100;
            } else {
                damage = 40;
            }
            break;

        case SPECIAL_SHOTGUN:
            *gun_type = HIT_TYPE_GUN_SHOT_SHOTGUN;
            // Shotgun is lethal up close, weak at range.
            if (p_target->Genus.Player->PlayerID || p_person->Genus.Person->PlayerID) {
                dist <<= 2;
            }
            damage = 300 - (dist);
            SATURATE(damage, 0, 250);
            break;

        default:
            ASSERT(0);
            return 0;
        }

    } else {
        ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT);
        *gun_type = HIT_TYPE_GUN_SHOT_PISTOL;
        damage = 70; // three shots kill
    }

    if (p_target->Genus.Person->PlayerID) {
        damage >>= 1;
    }

    return damage;
}

// Sentinels returned by shoot_get_ammo_sound_anim_time.
// uc_orig: NOT_A_GUN_YOU_SHOOT (fallen/Source/Person.cpp)
#define NOT_A_GUN_YOU_SHOOT (-1)
// uc_orig: HAD_TO_CHANGE_CLIP (fallen/Source/Person.cpp)
#define HAD_TO_CHANGE_CLIP  (-2)

// Consumes ammo and fills in sound/anim/time for a shot.
// Returns ammo count, NOT_A_GUN_YOU_SHOOT (non-shootable special), or HAD_TO_CHANGE_CLIP.
// uc_orig: shoot_get_ammo_sound_anim_time (fallen/Source/Person.cpp)
SLONG shoot_get_ammo_sound_anim_time(Thing* p_person, SLONG* sound, SLONG* anim, SLONG* time)
{
    SLONG ammo = FALSE;
    SLONG ammo_in_clip;

    if (PersonIsMIB(p_person)) {
        // MIB have AK47s with unlimited ammo.
        *anim = ANIM_AK_FIRE;
        ammo = 50;
        *time = 64;
        *sound = S_MIB_GUN_WDOWN;
        return ammo;
    }

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        *sound = S_SHOTGUN_SHOT;

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_AK47:
            if (p_person->Genus.Person->PlayerID == 0) {
                // Enemies never run out of AK ammo.
                p_special->Genus.Special->ammo++;
            }
            *anim = ANIM_AK_FIRE;
            *time = 64;
            *sound = S_AK47_BURST;
            break;

        case SPECIAL_SHOTGUN:
            *anim = ANIM_SHOTGUN_FIRE;
            *time = 400;
            DIRT_new_sparks(p_person->Genus.Person->GunMuzzle.X >> 8, p_person->Genus.Person->GunMuzzle.Y >> 8, p_person->Genus.Person->GunMuzzle.Z >> 8, 2 | 32);
            break;

        case SPECIAL_GRENADE:
            // Non-player grenade throw path.
            SPECIAL_prime_grenade(p_special);
            set_person_can_release(p_person, 128);
            return NOT_A_GUN_YOU_SHOOT;

        default:
            // Not a shootable weapon.
            return NOT_A_GUN_YOU_SHOOT;
        }

        if (p_special->Genus.Special->ammo) {
            p_special->Genus.Special->ammo -= 1;
            ammo = TRUE;
        } else {
            // Try to reload from carried ammo packs.
            switch (p_special->Genus.Special->SpecialType) {
            case SPECIAL_AK47:
                if (p_person->Genus.Person->ammo_packs_ak47) {
                    ammo_in_clip = p_person->Genus.Person->ammo_packs_ak47;
                    ammo_in_clip -= SPECIAL_AMMO_IN_A_AK47;
                    if (ammo_in_clip < 0) {
                        p_special->Genus.Special->ammo = p_person->Genus.Person->ammo_packs_ak47;
                        p_person->Genus.Person->ammo_packs_ak47 = 0;
                    } else {
                        p_special->Genus.Special->ammo = SPECIAL_AMMO_IN_A_AK47;
                        p_person->Genus.Person->ammo_packs_ak47 = ammo_in_clip;
                    }
                    ammo = HAD_TO_CHANGE_CLIP;
                }
                break;
            case SPECIAL_SHOTGUN:
                if (p_person->Genus.Person->ammo_packs_shotgun) {
                    ammo_in_clip = p_person->Genus.Person->ammo_packs_shotgun;
                    ammo_in_clip -= SPECIAL_AMMO_IN_A_SHOTGUN;
                    if (ammo_in_clip < 0) {
                        p_special->Genus.Special->ammo = p_person->Genus.Person->ammo_packs_shotgun;
                        p_person->Genus.Person->ammo_packs_shotgun = 0;
                    } else {
                        p_special->Genus.Special->ammo = SPECIAL_AMMO_IN_A_SHOTGUN;
                        p_person->Genus.Person->ammo_packs_shotgun = ammo_in_clip;
                    }
                    ammo = HAD_TO_CHANGE_CLIP;
                }
                break;
            }
        }
    } else {
        // Pistol.
        *sound = SOUND_Range(S_PISTOL_SHOT, S_PISTOL_SHOT_END);
        *time = 140;

        if (p_person->Genus.Person->Ammo) {
            p_person->Genus.Person->Ammo--;
            ammo = TRUE;
            *anim = ANIM_PISTOL_SHOOT;
        } else {
            if (p_person->Genus.Person->ammo_packs_pistol) {
                ammo_in_clip = p_person->Genus.Person->ammo_packs_pistol;
                ammo_in_clip -= SPECIAL_AMMO_IN_A_PISTOL;
                if (ammo_in_clip < 0) {
                    p_person->Genus.Person->Ammo = p_person->Genus.Person->ammo_packs_pistol;
                    p_person->Genus.Person->ammo_packs_pistol = 0;
                } else {
                    p_person->Genus.Person->Ammo = SPECIAL_AMMO_IN_A_PISTOL;
                    p_person->Genus.Person->ammo_packs_pistol = ammo_in_clip;
                }
                ammo = HAD_TO_CHANGE_CLIP;
            }
        }
    }

    return ammo;
}

// Fires the gun: muzzle flash dynamic light, brass eject, hit or ricochet effects.
// uc_orig: actually_fire_gun (fallen/Source/Person.cpp)
void actually_fire_gun(Thing* p_person)
{
    SLONG rico_id;

    GameCoord shotPosition = p_person->WorldPos;

    PCOM_oscillate_tympanum(
        PCOM_SOUND_GUNSHOT,
        p_person,
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8);

    if (p_person->Genus.Person->PlayerID) {
        GAME_FLAGS |= GF_PLAYER_FIRED_GUN;
        if (p_person->Genus.Person->Target)
            timer_bored = 0;
    }

    {
        // Single-frame muzzle flash dynamic light.
        UBYTE dlight;

        dlight = NIGHT_dlight_create(
            (p_person->WorldPos.X >> 8) - (SIN(p_person->Draw.Tweened->Angle) >> 9),
            (p_person->WorldPos.Y >> 8) + 0x60,
            (p_person->WorldPos.Z >> 8) - (COS(p_person->Draw.Tweened->Angle) >> 9),
            100,
            30,
            25,
            5);

        if (dlight) {
            NIGHT_dlight[dlight].flag |= NIGHT_DLIGHT_FLAG_REMOVE;
        }
    }

    extern void DIRT_create_brass(SLONG x, SLONG y, SLONG z, SLONG angle);

    {
        GameCoord vec;

        calc_sub_objects_position(
            p_person,
            p_person->Draw.Tweened->AnimTween,
            SUB_OBJECT_LEFT_HAND,
            &vec.X,
            &vec.Y,
            &vec.Z);
        vec.X += p_person->WorldPos.X >> 8;
        vec.Y += p_person->WorldPos.Y >> 8;
        vec.Z += p_person->WorldPos.Z >> 8;
        DIRT_create_brass(vec.X, vec.Y, vec.Z, (p_person->Draw.Tweened->Angle + 512) & 2047);
        if (p_person->Genus.Person->SpecialUse || PersonIsMIB(p_person)) {
            Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);
            if ((!p_person->Genus.Person->SpecialUse) || (p_special->Genus.Special->SpecialType == SPECIAL_AK47)) {
                // AK47 ejects extra brass casings.
                DIRT_create_brass(vec.X, vec.Y, vec.Z, (p_person->Draw.Tweened->Angle + 512) & 2047);
                DIRT_create_brass(vec.X, vec.Y, vec.Z, (p_person->Draw.Tweened->Angle + 512) & 2047);
            }
        }
    }

    if (p_person->Genus.Person->Target) {
        SLONG damage;
        Thing* p_target = TO_THING(p_person->Genus.Person->Target);
        GameCoord vec;
        SLONG gun_type;

        if (p_target->Class == CLASS_PERSON && (p_target->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER))) {
            // Shoot the car this person is in rather than the person.
            p_person->Genus.Person->Target = p_target->Genus.Person->InCar;
            p_target = TO_THING(p_target->Genus.Person->InCar);
        }

        damage = get_shoot_damage(p_person, p_target, &gun_type);

        if (damage) {
            PYRO_hitspang(p_person, p_target);
            timer_bored = 0;

            if (p_target->Class == CLASS_PERSON) {
                if (!p_target->Genus.Person->PlayerID) {

                    if (p_target->Genus.Person->Health > 0 && !is_person_ko(p_target)) {
                        SLONG skill = GET_SKILL(p_target);

                        if (PersonIsMIB(p_person)) {
                            // MIB are expert bullet dodgers.
                            skill += 5;
                        }

                        // High-skill NPCs can dodge bullets.
                        if (skill >= 7) {
                            skill -= 5;

                            if (p_person->Genus.Person->SpecialUse) {
                                Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);
                                if (p_special->Genus.Special->SpecialType == SPECIAL_SHOTGUN) {
                                    skill -= 5; // shotgun spread is harder to dodge
                                }
                            }

                            if ((Random() & 0x1f) < skill) {
                                PCOM_attack_happened(p_target, p_person);
                                set_person_flip(p_target, Random() & 0x1);
                                return;
                            }
                        }
                    }
                }

                // Blood splat on hit.
                if (VIOLENCE) {
                    calc_sub_objects_position(
                        p_target,
                        p_target->Draw.Tweened->AnimTween,
                        SUB_OBJECT_LEFT_HAND,
                        &vec.X,
                        &vec.Y,
                        &vec.Z);
                    vec.X += (Random() & 0x1f);
                    vec.Y += (Random() & 0x1f);
                    vec.Z += (Random() & 0x1f);
                    vec.X <<= 8;
                    vec.Y <<= 8;
                    vec.Z <<= 8;
                    vec.X += p_target->WorldPos.X;
                    vec.Y += p_target->WorldPos.Y;
                    vec.Z += p_target->WorldPos.Z;
                    PARTICLE_Add(vec.X, vec.Y, vec.Z, 0, 0, 0,
                        POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), 0x7FFF0000,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE, 10, 75, 1, 20, 5);
                }
                apply_hit_to_person(
                    p_target,
                    0,
                    gun_type,
                    damage,
                    p_person,
                    NULL);

                if (p_person->Genus.Person->PlayerID) {
                    // Clear target once killed.
                    if (p_target->Genus.Person->Health <= 0) {
                        p_person->Genus.Person->Target = 0;
                    }
                }
            } else if (p_target->Class == CLASS_BAT) {
                BAT_apply_hit(
                    p_target,
                    p_person,
                    damage);
            } else if (p_target->Class == CLASS_VEHICLE) {
                extern void VEH_reduce_health(Thing * p_car, Thing * p_person, SLONG damage);

                VEH_reduce_health(
                    p_target,
                    p_person,
                    damage);

                p_target->Genus.Vehicle->Flags |= FLAG_VEH_SHOT_AT;
            } else if (p_target->Class == CLASS_SPECIAL) {
                // Must be a mine.
                void special_activate_mine(Thing * p_mine);
                special_activate_mine(p_target);
            } else {
                ASSERT(p_target->Class == CLASS_BARREL);
                BARREL_shoot(
                    p_target,
                    p_person);
            }
        } else {
            // Miss — play ricochet sound and spark effect.
            shotPosition.X -= (SIN(p_person->Draw.Tweened->Angle) * 1024) >> 8;
            shotPosition.Z -= (COS(p_person->Draw.Tweened->Angle) * 1024) >> 8;

            rico_id = ((Random() * (S_RICOCHET_END - S_RICOCHET_START)) >> 16) + S_RICOCHET_START;

            MFX_play_xyz(0, rico_id, 0, shotPosition.X, shotPosition.Y, shotPosition.Z);

            // Find the impact point on a missed shot.
            SLONG hitx;
            SLONG hity;
            SLONG hitz;

            SLONG b_index = THING_find_nearest(
                p_target->WorldPos.X >> 8,
                p_target->WorldPos.Y >> 8,
                p_target->WorldPos.Z >> 8,
                0xa0,
                1 << CLASS_BARREL);

            if (b_index) {
                Thing* p_barrel = TO_THING(b_index);

                hitx = p_barrel->WorldPos.X >> 8;
                hity = p_barrel->WorldPos.Y >> 8;
                hitz = p_barrel->WorldPos.Z >> 8;

                BARREL_shoot(
                    p_barrel,
                    p_person);
            } else {
                hitx = p_target->WorldPos.X + (Random() & 0x1fff) - 0xfff;
                hitz = p_target->WorldPos.Z + (Random() & 0x1fff) - 0xfff;
                hity = PAP_calc_map_height_at(hitx >> 8, hitz >> 8) + 0x1000;
            }

            PYRO_hitspang(
                p_person,
                hitx,
                hity,
                hitz);
        }
    } else {
        // No target — try to shoot a coke can or spark off the environment.
        if (DIRT_shoot(p_person)) {
            shotPosition.X -= (SIN(p_person->Draw.Tweened->Angle) * 1024) >> 8;
            shotPosition.Z -= (COS(p_person->Draw.Tweened->Angle) * 1024) >> 8;
            MFX_play_xyz(0, S_PISTOL_SHOT, 0, shotPosition.X, shotPosition.Y, shotPosition.Z);
        } else {
            shotPosition.X -= (SIN(p_person->Draw.Tweened->Angle) * 1024) >> 8;
            shotPosition.Z -= (COS(p_person->Draw.Tweened->Angle) * 1024) >> 8;

            rico_id = ((Random() * (S_RICOCHET_END - S_RICOCHET_START)) >> 16) + S_RICOCHET_START;
            MFX_play_xyz(0, rico_id, 0, shotPosition.X, shotPosition.Y, shotPosition.Z);

            // Trace line-of-sight to find where the stray shot hit the world.
            {
                SLONG endx = p_person->WorldPos.X - (SIN(p_person->Draw.Tweened->Angle) << 2) >> 8;
                SLONG endy = p_person->WorldPos.Y + 0x6000 >> 8;
                SLONG endz = p_person->WorldPos.Z - (COS(p_person->Draw.Tweened->Angle) << 2) >> 8;

                if (there_is_a_los(
                        p_person->WorldPos.X >> 8,
                        p_person->WorldPos.Y >> 8,
                        p_person->WorldPos.Z >> 8,
                        endx,
                        endy,
                        endz,
                        0)) {
                    PYRO_hitspang(
                        p_person,
                        endx << 8,
                        endy << 8,
                        endz << 8);
                } else {
                    PYRO_hitspang(
                        p_person,
                        los_failure_x << 8,
                        los_failure_y << 8,
                        los_failure_z << 8);
                }
            }
        }
    }
}

// Fires weapon while running, enforcing per-weapon cooldown timer.
// uc_orig: set_person_running_shoot (fallen/Source/Person.cpp)
void set_person_running_shoot(Thing* p_person)
{
    SLONG ammo, sound, anim, time;

    if (p_person->Genus.Person->Timer1) {
        // Still cooling down from last shot.
        return;
    }

    ammo = shoot_get_ammo_sound_anim_time(p_person, &sound, &anim, &time);

    if (ammo == NOT_A_GUN_YOU_SHOOT) {
        return;
    }

    if (!ammo || ammo == HAD_TO_CHANGE_CLIP) {
        MFX_play_thing(THING_NUMBER(p_person), S_PISTOL_DRY, MFX_REPLACE, p_person);
        return;
    }

    MFX_play_thing(THING_NUMBER(p_person), sound, MFX_REPLACE, p_person);
    actually_fire_gun(p_person);

    p_person->Genus.Person->Timer1 = time;
}

// Returns the best weapon type (with ammo) from the person's inventory.
// Priority: AK47 > shotgun > gun > bat > knife.
// Returns SPECIAL_NONE if no viable weapon found.
// uc_orig: get_persons_best_weapon_with_ammo (fallen/Source/Person.cpp)
SLONG get_persons_best_weapon_with_ammo(Thing* p_person)
{
    Thing* p_special;

    static UBYTE weapon_order[5] = {
        SPECIAL_AK47,
        SPECIAL_SHOTGUN,
        SPECIAL_GUN,
        SPECIAL_BASEBALLBAT,
        SPECIAL_KNIFE
    };

    SLONG i;

    for (i = 0; i < 5; i++) {
        if (i == 2) {
            if (p_person->Flags & FLAGS_HAS_GUN) {
                if (p_person->Genus.Person->Ammo) {
                    return SPECIAL_GUN;
                }
            }
        } else {
            if (p_special = person_has_special(p_person, weapon_order[i])) {
                if (i < 2) {
                    if (p_special->Genus.Special->ammo) {
                        return weapon_order[i];
                    }
                } else {
                    return weapon_order[i];
                }
            }
        }
    }

    return SPECIAL_NONE;
}

// Returns TRUE if a cutscene is in progress and the NPC should not shoot p_target.
// uc_orig: dont_hurt_target_during_cutscene (fallen/Source/Person.cpp)
SLONG dont_hurt_target_during_cutscene(Thing* p_person, Thing* p_target)
{
    if (!p_person->Genus.Person->PlayerID) {
        if (p_target->Class == CLASS_PERSON) {
            SLONG dont_shoot_in_a_cutscene = FALSE;

            if (p_target->Genus.Person->PlayerID) {
                dont_shoot_in_a_cutscene = TRUE;
            } else if (p_target->Genus.Person->pcom_move == PCOM_MOVE_FOLLOW) {
                // Don't shoot people following the player during a cutscene.
                UWORD i_follow = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);

                if (i_follow) {
                    Thing* p_follow = TO_THING(i_follow);

                    if (p_follow->Class == CLASS_PERSON && p_follow->Genus.Person->PlayerID) {
                        dont_shoot_in_a_cutscene = TRUE;
                    }
                }
            }

            if (dont_shoot_in_a_cutscene && EWAY_stop_player_moving()) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

// Full shoot action: handles ammo check, target acquisition, weapon-swap on empty,
// guard reactions, and cutscene suppression.
// uc_orig: set_person_shoot (fallen/Source/Person.cpp)
void set_person_shoot(Thing* p_person, UWORD shoot_target)
{
    SLONG dx, dz;
    SLONG anim = ANIM_PISTOL_SHOOT;
    SLONG ammo = FALSE;
    SLONG sound;
    SLONG time;

    if (p_person->State == STATE_CARRY) {
        // Can't shoot while carrying someone.
        return;
    }

    if (might_i_be_a_villain(p_person)) {
        // Shooting in public attracts cop attention.
        PCOM_call_cop_to_arrest_me(p_person, 1);
    }

    if (p_person->SubState == SUB_STATE_RUNNING) {
        set_person_running_shoot(p_person);
        return;
    }

    p_person->Genus.Person->Timer1 = 0;

    if (p_person->Genus.Person->PlayerID) {
        // Player-specific pre-shot checks.
        if (p_person->Genus.Person->Target) {
            Thing* p_target;
            p_target = TO_THING(p_person->Genus.Person->Target);

            if (p_target->Class == CLASS_PERSON) {
                if (p_person->Genus.Person->PersonType == PERSON_DARCI) {
                    if (p_target->Draw.Tweened->CurrentAnim == ANIM_HANDS_UP || p_target->Draw.Tweened->CurrentAnim == ANIM_HANDS_UP_LOOP) {
                        PANEL_new_text(p_person, 8000, XLAT_str(X_GET_DOWN));
                        set_person_dead(p_target, p_person, PERSON_DEATH_TYPE_GET_DOWN, 0, 0);
                        p_person->Genus.Person->Target = NULL;
                        return;
                    }
                    if (p_target->Genus.Person->PersonType == PERSON_COP && !(p_target->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY)) {
                        PANEL_new_text(p_person, 8000, XLAT_str(X_CANT_SHOOT_COP));
                        return;
                    }
                }
            }
        }
    } else {
        // NPC-specific pre-shot checks.
        if (p_person->Genus.Person->Target) {
            Thing* p_target;
            p_target = TO_THING(p_person->Genus.Person->Target);

            if (p_target->Class == CLASS_VEHICLE) {
                if (p_target->State == STATE_DEAD) {
                    p_person->Genus.Person->Target = NULL;
                }
            } else {
                if (dont_hurt_target_during_cutscene(p_person, p_target)) {
                    return;
                }
            }
        }
    }

    ammo = shoot_get_ammo_sound_anim_time(p_person, &sound, &anim, &time);

    if (ammo == NOT_A_GUN_YOU_SHOOT) {
        return;
    }

    if (!ammo || ammo == HAD_TO_CHANGE_CLIP) {
        MFX_play_thing(THING_NUMBER(p_person), S_PISTOL_DRY, MFX_REPLACE, p_person);

        if (p_person->Genus.Person->PlayerID && ammo != HAD_TO_CHANGE_CLIP) {
            // Auto-switch to best available weapon.
            SLONG special = get_persons_best_weapon_with_ammo(p_person);

            if (special) {
                if (special == SPECIAL_GUN) {
                    set_person_draw_gun(p_person);
                } else {
                    set_person_draw_item(p_person, special);
                }
            } else {
                if (p_person->Genus.Person->SpecialUse) {
                    set_person_item_away(p_person);
                } else {
                    set_person_gun_away(p_person);
                }
            }
        }

        return;
    }

    set_anim(p_person, anim);
    p_person->Genus.Person->Action = ACTION_SHOOT;
    set_generic_person_state_function(p_person, STATE_GUN);
    p_person->SubState = SUB_STATE_SHOOT_GUN;
    p_person->Genus.Person->Flags |= (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
    p_person->Draw.Tweened->Flags |= DT_FLAG_GUNFLASH;

    {
        if (!shoot_target || p_person->Genus.Person->Target == 0) {
            // Find someone to shoot.
            p_person->Genus.Person->Target = find_target_new(p_person);

            if (p_person->Genus.Person->Target) {
                Thing* p_target;
                p_target = TO_THING(p_person->Genus.Person->Target);

                set_face_thing(p_person, p_target);

                if (p_target->Class == CLASS_PERSON) {
                    if (p_person->Genus.Person->PersonType == PERSON_DARCI) {
                        if (p_target->Draw.Tweened->CurrentAnim == ANIM_HANDS_UP || p_target->Draw.Tweened->CurrentAnim == ANIM_HANDS_UP_LOOP) {
                            PANEL_new_text(p_person, 8000, XLAT_str(X_GET_DOWN));
                            set_person_dead(p_target, p_person, PERSON_DEATH_TYPE_GET_DOWN, 0, 0);
                            return;
                        }

                        if (p_target->Draw.Tweened->CurrentAnim == ANIM_HANDS_UP_LIE) {
                            // Can't shoot civs while they're getting down.
                            return;
                        }

                        if (p_target->Genus.Person->PersonType == PERSON_COP && !(p_target->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY)) {
                            PANEL_new_text(p_person, 8000, XLAT_str(X_CANT_SHOOT_COP));
                            return;
                        }
                    }

                    if (p_target->Genus.Person->PlayerID) {
                        if (EWAY_stop_player_moving()) {
                            // Cutscene active — don't shoot.
                            return;
                        }
                    }
                }
            }
        }

        MFX_play_thing(THING_NUMBER(p_person), sound, MFX_REPLACE, p_person);

        ASSERT(p_person->Genus.Person->Target != THING_NUMBER(p_person));

        actually_fire_gun(p_person);
    }
}

// Starts grappling hook windup animation.
// uc_orig: set_person_grapple_windup (fallen/Source/Person.cpp)
void set_person_grapple_windup(Thing* p_person)
{
    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_GRAPPLING);
    set_anim(p_person, ANIM_GRAPPLING_HOOK_WINDUP);
    set_generic_person_state_function(p_person, STATE_GRAPPLING);
    p_person->SubState = SUB_STATE_GRAPPLING_WINDUP;
}

// Starts grappling hook release animation.
// uc_orig: set_person_grappling_hook_release (fallen/Source/Person.cpp)
void set_person_grappling_hook_release(Thing* p_person)
{
    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_GRAPPLING);
    set_anim(p_person, ANIM_GRAPPLING_HOOK_RELEASE);
    set_generic_person_state_function(p_person, STATE_GRAPPLING);
    p_person->SubState = SUB_STATE_GRAPPLING_RELEASE;
}

// Returns SPECIAL_TYPE if person has a gun-type weapon out, else FALSE.
// uc_orig: person_has_gun_out (fallen/Source/Person.cpp)
SLONG person_has_gun_out(Thing* p_person)
{
    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        return (SPECIAL_GUN);
    }

    if (!p_person->Genus.Person->SpecialUse) {
        return FALSE;
    }

    {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        if (p_special->Genus.Special->SpecialType == SPECIAL_SHOTGUN || p_special->Genus.Special->SpecialType == SPECIAL_AK47) {
            return (p_special->Genus.Special->SpecialType);
        }
    }

    return FALSE;
}

// Drops the currently equipped gun or special weapon to the ground.
// If change_anim is set, transitions to idle after dropping a special.
// uc_orig: drop_current_gun (fallen/Source/Person.cpp)
void drop_current_gun(Thing* p_person, SLONG change_anim)
{
    SLONG gx;
    SLONG gy;
    SLONG gz;

    Thing* p_special;

    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        Thing* p_gun;

        find_nice_place_near_person(
            p_person,
            &gx,
            &gy,
            &gz);

        p_gun = alloc_special(
            SPECIAL_GUN,
            SPECIAL_SUBSTATE_NONE,
            gx,
            gy,
            gz,
            NULL);

        if (p_gun) {
            if (p_person->Genus.Person->PlayerID) {
                p_gun->Genus.Special->ammo = p_person->Genus.Person->Ammo;
            } else {
                p_gun->Genus.Special->ammo = (Random() & 0x3) + 3;
            }
        }

        p_person->Draw.Tweened->PersonID &= ~0xe0;
        p_person->Genus.Person->Flags &= ~FLAG_PERSON_GUN_OUT;
        p_person->Flags &= ~FLAGS_HAS_GUN;
    } else if (p_person->Genus.Person->SpecialUse) {
        p_special = TO_THING(p_person->Genus.Person->SpecialUse);
        special_drop(p_special, p_person);

        p_person->Genus.Person->SpecialUse = NULL;
        p_person->Draw.Tweened->PersonID &= ~0xe0;

        if (change_anim)
            set_person_idle(p_person);
    }
}

// Drops all items the person is carrying: gun, all specials, and bounty drop.
// If is_being_searched and something was found, plays the item-revealed sound.
// uc_orig: drop_all_items (fallen/Source/Person.cpp)
void drop_all_items(Thing* p_person, UBYTE is_being_searched)
{
    SLONG gx;
    SLONG gy;
    SLONG gz;
    UBYTE found_something = 0;

    if (p_person->Flags & FLAGS_HAS_GUN) {
        find_nice_place_near_person(
            p_person,
            &gx,
            &gy,
            &gz);

        {
            Thing* p_gun = alloc_special(
                SPECIAL_GUN,
                SPECIAL_SUBSTATE_NONE,
                gx,
                gy,
                gz,
                NULL);

            if (p_gun) {
                if (p_person->Genus.Person->PlayerID) {
                    p_gun->Genus.Special->ammo = p_person->Genus.Person->Ammo;
                } else {
                    p_gun->Genus.Special->ammo = (Random() & 0x3) + 3;
                }
                found_something = 1;
            }
        }

        p_person->Flags &= ~FLAGS_HAS_GUN;
    }

    while (p_person->Genus.Person->SpecialList) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialList);

        if (p_person->Genus.Person->drop == p_special->Genus.Special->SpecialType) {
            // Already dropping his bounty via SpecialList — clear the separate drop field.
            p_person->Genus.Person->drop = NULL;
        }

        special_drop(p_special, p_person);
        found_something = 1;
    }

    if (p_person->Genus.Person->drop) {
        find_nice_place_near_person(
            p_person,
            &gx,
            &gy,
            &gz);

        alloc_special(
            p_person->Genus.Person->drop,
            SPECIAL_SUBSTATE_NONE,
            gx,
            gy,
            gz,
            NULL);

        p_person->Genus.Person->drop = NULL;
        found_something = 1;
    }
    if (is_being_searched && found_something)
        MFX_play_ambient(THING_NUMBER(p_person), S_ITEM_REVEALED, 0);
}

// ---- Chunk 5: set_person_idle_uncroutch..set_person_carry ----
// (lines ~6144-7647 of original Person.cpp)

// Forward declarations for chunk 5 internal cross-references
// (set_person_idle calls set_person_stand_carry which is defined later in this chunk)
// uc_orig: set_person_stand_carry (fallen/Source/Person.cpp)
void set_person_stand_carry(Thing* p_person);

// Transitions person from crouching-idle back to standing idle with uncrouch animation.
// uc_orig: set_person_idle_uncroutch (fallen/Source/Person.cpp)
void set_person_idle_uncroutch(Thing* p_person)
{
    SLONG anim;
    set_generic_person_state_function(p_person, STATE_IDLE);
    p_person->Genus.Person->Action = ACTION_IDLE;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;
    p_person->SubState = SUB_STATE_IDLE_UNCROUCH;

    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        anim = ANIM_UNCROUTCH_PISTOL;
    } else if (person_holding_2handed(p_person)) {
        anim = ANIM_UNCROUTCH_AK;
    } else {
        anim = ANIM_CROUTCH_GETUP;
    }

    set_anim(p_person, anim);

    return;
}

// Starts the turn-to-face-wall animation (entering hug-wall mode from front).
// uc_orig: set_person_turn_to_hug_wall (fallen/Source/Person.cpp)
void set_person_turn_to_hug_wall(Thing* p_person)
{
    SLONG anim;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        anim = ANIM_PRESS_WALL_TURN_PISTOL;
    } else if (person_holding_2handed(p_person)) {
        anim = ANIM_PRESS_WALL_TURN_AK;
    } else {
        anim = ANIM_PRESS_WALL_TURN;
    }
    set_generic_person_state_function(p_person, STATE_HUG_WALL);
    p_person->SubState = SUB_STATE_HUG_WALL_TURN;
    set_anim(p_person, anim);
    p_person->Genus.Person->Action = ACTION_HUG_WALL;
}

// Starts sidling along wall in the given direction (1=left, 0=right) while hugging wall.
// uc_orig: set_person_hug_wall_dir (fallen/Source/Person.cpp)
void set_person_hug_wall_dir(Thing* p_person, SLONG dir)
{
    SLONG gun;
    SLONG anim;

    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)
        gun = 2;
    else
        gun = person_holding_2handed(p_person);
    if (dir == 1) {
        if (gun == 2)
            anim = ANIM_PRESS_WALL_SIDLE_R_PISTOL;
        else if (gun == 1)
            anim = ANIM_PRESS_WALL_SIDLE_R_AK;
        else
            anim = ANIM_PRESS_WALL_SIDLE_R;

        set_anim(p_person, anim);
        p_person->SubState = SUB_STATE_HUG_WALL_STEP_LEFT;
    } else {
        if (gun == 2)
            anim = ANIM_PRESS_WALL_SIDLE_L_PISTOL;
        else if (gun == 1)
            anim = ANIM_PRESS_WALL_SIDLE_L_AK;
        else
            anim = ANIM_PRESS_WALL_SIDLE_L;

        set_anim(p_person, anim);
        p_person->SubState = SUB_STATE_HUG_WALL_STEP_RIGHT;
    }
}

// Starts the peek-around-wall animation in the given direction while hugging wall.
// dir=1 peeks left, dir=0 peeks right.
// uc_orig: set_person_hug_wall_look (fallen/Source/Person.cpp)
void set_person_hug_wall_look(Thing* p_person, SLONG dir)
{
    SLONG gun;
    SLONG anim;

    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
    p_person->Genus.Person->InsideRoom = 0;

    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)
        gun = 2;
    else
        gun = person_holding_2handed(p_person);
    if (dir == 1) {
        if (gun == 2)
            anim = ANIM_PRESS_WALL_LOOK_L_PISTOL;
        else if (gun == 1)
            anim = ANIM_PRESS_WALL_LOOK_L_AK;
        else
            anim = ANIM_PRESS_WALL_LOOK_L;

        set_anim(p_person, anim);
        p_person->SubState = SUB_STATE_HUG_WALL_LOOK_L;
    } else {
        if (gun == 2)
            anim = ANIM_PRESS_WALL_LOOK_R_PISTOL;
        else if (gun == 1)
            anim = ANIM_PRESS_WALL_LOOK_R_AK;
        else
            anim = ANIM_PRESS_WALL_LOOK_R;

        set_anim(p_person, anim);
        p_person->SubState = SUB_STATE_HUG_WALL_LOOK_R;
    }
}

// Transitions person to standing idle, choosing appropriate animation and state based on
// current mode, weapon, combat state, and whether they are carrying someone.
// uc_orig: set_person_idle (fallen/Source/Person.cpp)
void set_person_idle(Thing* p_person)
{
    SLONG anim;
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_KO;

    if (check_on_slippy_slope(p_person))
        return;

    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
        set_person_stand_carry(p_person);
        return;
    }

    if (p_person->Genus.Person->PlayerID) {
        if (p_person->Genus.Person->PlayerID && !person_has_gun_out(p_person)) {
            Thing* p_attacker = is_person_under_attack_low_level(p_person, FALSE, 0x200);

            if (p_attacker) {
                SLONG anim;

                ASSERT(p_attacker->Class == CLASS_PERSON);

                p_person->Genus.Person->Target = THING_NUMBER(p_attacker);
                person_enter_fight_mode(p_person);
                set_person_fight_idle(p_person);
                return;
            } else {
                p_person->Genus.Person->Target = 0;
                p_person->Genus.Person->Mode = PERSON_MODE_RUN;
                p_person->Genus.Person->Agression = 0;
            }
            GAME_FLAGS &= ~GF_SIDE_ON_COMBAT;
        }

        if (p_person->Genus.Person->Mode != PERSON_MODE_FIGHT)
        {
            if (continue_moveing(p_person)) {
                set_person_running(p_person);
                return;
            }
        }
    }

    p_person->SubState = 0;
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_HELPLESS;

    if (p_person->Genus.Person->Flags & FLAG_PERSON_GRAPPLING) {
        set_person_grapple_windup(p_person);

        return;
    }
    if (person_has_gun_out(p_person)) {
        set_person_aim(p_person);

        return;
    }

    p_person->Genus.Person->Timer1 = (Random() & 0x1ff) + 400;
    set_generic_person_state_function(p_person, STATE_IDLE);
    p_person->Genus.Person->Action = ACTION_IDLE;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

    p_person->Velocity = 0;

    set_anim_idle(p_person);

    set_person_sidle(p_person);
}

// Sets person into a locked idle ready stance (respects gun/weapon state).
// Called e.g. when a locked animation finishes and person needs to return to idle.
// uc_orig: set_person_locked_idle_ready (fallen/Source/Person.cpp)
void set_person_locked_idle_ready(Thing* p_person)
{
    SLONG anim = ANIM_STAND_READY;

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special;

        p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_GUN:
            anim = ANIM_PISTOL_AIM_AHEAD;
            break;

        case SPECIAL_BASEBALLBAT:
            anim = ANIM_BAT_IDLE;
            break;
        case SPECIAL_SHOTGUN:
        case SPECIAL_AK47:
            anim = ANIM_SHOTGUN_STAND;
            break;
        }
    }

    p_person->SubState = 0;
    if (p_person->Genus.Person->Flags & FLAG_PERSON_GRAPPLING) {
        set_person_grapple_windup(p_person);

        return;
    }

    p_person->Genus.Person->Timer1 = (Random() & 0x1ff) + 400;
    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        if (p_person->Genus.Person->PlayerID) {
            camera_shoot();
        }

        locked_anim_change(p_person, 0, anim, 0);
        set_person_aim(p_person);
        return;
    }
    if (p_person->Genus.Person->PlayerID) {
        camera_normal();
    }

    p_person->SubState = 0;
    set_generic_person_state_function(p_person, STATE_IDLE);
    p_person->SubState = 0;

    set_locked_anim(p_person, anim, 0);

    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_IDLE;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

    if (person_has_gun_out(p_person)) {
        set_person_aim(p_person);

        return;
    }

    set_person_sidle(p_person);
}

// Checks if the person's back is against a wall and sets sidle (wall-hug) sub-state.
// Returns 1 if sidling is now active, 0 otherwise. Body is disabled in the original
// (the implementation is commented out — only early return remains).
// uc_orig: set_person_sidle (fallen/Source/Person.cpp)
SLONG set_person_sidle(struct Thing* p_person)
{
    SLONG dx, dz;
    SLONG index, facet;
    SLONG dist;
    SLONG px, pz, mx, mz;
    SLONG angle;
    SLONG ret_x, ret_z;
    SLONG mdx, mdz;
    return (0);
    /*
    (original body disabled in the prerelease source — returns 0 always)
    */
}

// Starts a flip animation in the given direction (0=left, 1=right).
// uc_orig: set_person_flip (fallen/Source/Person.cpp)
void set_person_flip(Thing* p_person, SLONG dir)
{
    MSG_add(" start flipping");
    switch (dir) {
    case 0:
        set_anim(p_person, ANIM_FLIP_LEFT);
        p_person->Genus.Person->Action = ACTION_FLIP_LEFT;
        break;

    case 1:
        set_anim(p_person, ANIM_FLIP_RIGHT);
        p_person->Genus.Person->Action = ACTION_FLIP_RIGHT;
        break;
    }
    set_generic_person_state_function(p_person, STATE_MOVEING);
    p_person->SubState = SUB_STATE_FLIPING;
    p_person->Genus.Person->Flags |= (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
}

// Sets person into running state, choosing run/walk/sprint/sneak based on Mode.
// Handles carry-mode running and yomp-start animations.
// uc_orig: set_person_running (fallen/Source/Person.cpp)
void set_person_running(Thing* p_person)
{
    p_person->Draw.Tweened->Locked = 0;
    p_person->Genus.Person->Timer1 = 0;

    if (p_person->Genus.Person->PlayerID) {
        camera_normal();
    }

    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
        Thing* p_target;
        p_target = TO_THING(p_person->Genus.Person->Target);

        set_anim(p_person, ANIM_START_WALK_CARRY);
        set_anim(p_target, ANIM_START_WALK_CARRY_V);
        p_target->SubState = SUB_STATE_CARRY_MOVE_V;

        goto run;
    }

    switch (p_person->Genus.Person->Mode) {
    case PERSON_MODE_WALK:
        set_person_walking(p_person);
        break;
    case PERSON_MODE_SPRINT:
        if (p_person->Genus.Person->PersonType == PERSON_ROPER)
            goto yomp;

        set_anim_running(p_person);
        goto run;
    case PERSON_MODE_RUN:
    case PERSON_MODE_FIGHT:
    yomp:;
        if (person_holding_bat(p_person)) {
            set_anim(p_person, ANIM_YOMP_START_BAT);

        } else if (person_holding_2handed(p_person)) {
            set_anim(p_person, ANIM_YOMP_START_AK);

        } else {
            if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
                set_anim(p_person, ANIM_YOMP_START_PISTOL);

            } else
                set_anim(p_person, ANIM_YOMP_START);
        }
        if (p_person->Genus.Person->PlayerID)
            p_person->Genus.Person->Target = 0;
    run:;
        set_generic_person_state_function(p_person, STATE_MOVEING);
        p_person->SubState = SUB_STATE_RUNNING;
        if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING)
            p_person->Genus.Person->Action = ACTION_NONE;
        else
            p_person->Genus.Person->Action = ACTION_RUN;
        p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
        p_person->Genus.Person->Mode = PERSON_MODE_RUN;
        break;
    case PERSON_MODE_SNEAK:
        set_person_sneaking(p_person);
        break;
    default:
        ASSERT(0);
    }
}

// Sets person into running state starting from a specific animation frame index.
// Only supports Mode==0 (run), Mode==1 (walk), Mode==2 (sneak).
// uc_orig: set_person_running_frame (fallen/Source/Person.cpp)
void set_person_running_frame(Thing* p_person, SLONG frame)
{
    switch (p_person->Genus.Person->Mode) {
    case 0:
        MSG_add(" start running");
        queue_anim(p_person, ANIM_RUN);

        set_generic_person_state_function(p_person, STATE_MOVEING);
        p_person->SubState = SUB_STATE_RUNNING;
        p_person->Genus.Person->Action = ACTION_RUN;
        p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
        while (frame) {
            p_person->Draw.Tweened->QueuedFrame = p_person->Draw.Tweened->QueuedFrame->NextFrame;
            frame--;
        }
        break;
    case 1:
        set_person_walking(p_person);
        break;
    case 2:
        set_person_sneaking(p_person);
        break;
    }
}

// Cycles through the person's carried specials and switches to the next one.
// Plays a draw-gun sound to alert nearby NPCs.
// uc_orig: set_person_draw_special (fallen/Source/Person.cpp)
void set_person_draw_special(Thing* p_person)
{
    Thing* p_special = NULL;
    {
        if (p_person->Genus.Person->SpecialUse == NULL) {
            if (p_person->Genus.Person->SpecialList) {
                p_special = TO_THING(p_person->Genus.Person->SpecialList);
            }
        } else {
            p_special = TO_THING(p_person->Genus.Person->SpecialUse);

            if (p_special->Genus.Special->NextSpecial) {
                p_special = TO_THING(p_special->Genus.Special->NextSpecial);
            } else {
                p_special = NULL;
            }
        }

        if (p_special) {
            set_person_draw_item(p_person, p_special->Genus.Special->SpecialType);
        }
    }

    PCOM_oscillate_tympanum(
        PCOM_SOUND_DRAW_GUN,
        p_person,
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8);
}

// Draws the pistol: plays draw animation, sets mode to run (exits fight mode), alerts NPCs.
// Does nothing if the person has no gun or is currently carrying someone.
// uc_orig: set_person_draw_gun (fallen/Source/Person.cpp)
void set_person_draw_gun(Thing* p_person)
{
    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
        return;
    }

    if (!(p_person->Flags & FLAGS_HAS_GUN)) {
        return;
    }

    MSG_add(" start draw gun");
    p_person->Genus.Person->Mode = PERSON_MODE_RUN;
    set_anim(p_person, ANIM_PISTOL_DRAW);
    set_thing_velocity(p_person, 0);
    set_generic_person_state_function(p_person, STATE_GUN);

    p_person->SubState = SUB_STATE_DRAW_GUN;
    p_person->Genus.Person->Action = ACTION_DRAW_SPECIAL;
    p_person->Genus.Person->SpecialDraw = NULL;
    p_person->Genus.Person->SpecialUse = NULL;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_C | FLAG_PERSON_NON_INT_M;
    p_person->Genus.Person->Target = NULL;

    if (p_person->Genus.Person->PlayerID) {
        camera_shoot();
    }

    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        p_person->Genus.Person->Mode = PERSON_MODE_RUN;
    }

    PCOM_oscillate_tympanum(
        PCOM_SOUND_DRAW_GUN,
        p_person,
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8);
}

// Holsters the gun: clears GUN_OUT flag and transitions to idle.
// uc_orig: set_person_gun_away (fallen/Source/Person.cpp)
void set_person_gun_away(Thing* p_person)
{
    p_person->Genus.Person->SpecialUse = NULL;
    p_person->Draw.Tweened->PersonID &= ~0xe0;
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_GUN_OUT;
    set_person_idle(p_person);

    if (p_person->Genus.Person->PlayerID) {
        camera_normal();
    }
}

// Starts a sidestep-left animation.
// uc_orig: set_person_step_left (fallen/Source/Person.cpp)
void set_person_step_left(Thing* p_person)
{
    MSG_add(" start step_left");
    if (p_person->SubState == SUB_STATE_STEP_LEFT)
        return;
    set_anim(p_person, ANIM_STEP_LEFT);
    set_thing_velocity(p_person, 0);
    set_generic_person_state_function(p_person, STATE_MOVEING);
    p_person->SubState = SUB_STATE_STEP_LEFT;
    p_person->Genus.Person->Action = ACTION_SIDE_STEP;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
}

// Starts a sidestep-right animation.
// uc_orig: set_person_step_right (fallen/Source/Person.cpp)
void set_person_step_right(Thing* p_person)
{
    MSG_add(" start step_left");
    if (p_person->SubState == SUB_STATE_STEP_RIGHT)
        return;
    set_anim(p_person, ANIM_STEP_RIGHT);
    set_thing_velocity(p_person, 0);
    set_generic_person_state_function(p_person, STATE_MOVEING);
    p_person->SubState = SUB_STATE_STEP_RIGHT;
    p_person->Genus.Person->Action = ACTION_SIDE_STEP;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
}

// Stub for setting a vehicle animation. Always asserts — left unimplemented in the original.
// uc_orig: set_vehicle_anim (fallen/Source/Person.cpp)
void set_vehicle_anim(Thing* p_vehicle, SLONG anim)
{
    // 1 is still, 2 is open/close
    ASSERT(0);
    return;
    p_vehicle->Genus.Vehicle->Draw.CurrentFrame = game_chunk[5].AnimList[anim];
    p_vehicle->Genus.Vehicle->Draw.AnimTween = 0;

    if (anim == 2) {
        p_vehicle->Genus.Vehicle->Draw.NextFrame = p_vehicle->Genus.Vehicle->Draw.CurrentFrame->NextFrame;
    } else {
        p_vehicle->Genus.Vehicle->Draw.NextFrame = p_vehicle->Genus.Vehicle->Draw.CurrentFrame;
    }
}

// Positions person at the correct entry point for the given vehicle door (0=driver, 1=passenger).
// Sets person angle to match vehicle, with a 512-unit offset based on door side.
// uc_orig: position_person_for_vehicle (fallen/Source/Person.cpp)
void position_person_for_vehicle(Thing* p_person, Thing* p_vehicle, SLONG door)
{
    SLONG ix, iz;
    GameCoord new_position;

    ASSERT(door == 0 || door == 1);

    get_car_enter_xz(p_vehicle, door, &ix, &iz);

    new_position.X = ix << 8;
    new_position.Z = iz << 8;

    new_position.Y = p_person->WorldPos.Y;
    move_thing_on_map(p_person, &new_position);

    p_person->Draw.Tweened->Angle = p_vehicle->Genus.Vehicle->Angle;

    if (door) {
        p_person->Draw.Tweened->Angle += 512;
    } else {
        p_person->Draw.Tweened->Angle -= 512;
    }

    p_person->Draw.Tweened->Angle &= 2047;
}

// Starts person entering a vehicle: checks key, checks occupancy, plays enter animation.
// door=0 for driver side, door=1 for passenger side.
// uc_orig: set_person_enter_vehicle (fallen/Source/Person.cpp)
void set_person_enter_vehicle(Thing* p_person, Thing* p_vehicle, SLONG door)
{
    ASSERT(door == 0 || door == 1);

    if (p_vehicle->Genus.Vehicle->Driver) {
        p_person->Genus.Person->InCar = 0;
        return;
    }

    if (p_vehicle->State == STATE_DEAD) {
        return;
    }

    if (p_vehicle->Genus.Vehicle->key != SPECIAL_NONE) {
        if (!person_has_special(p_person, p_vehicle->Genus.Vehicle->key)) {

            if (p_person->Genus.Person->PlayerID) {
                PANEL_new_info_message(XLAT_str(X_CAR_LOCKED));
            }

            p_person->Genus.Person->InCar = 0;
            return;
        }
    }

    set_thing_velocity(p_person, 0);
    set_generic_person_state_function(p_person, STATE_MOVEING);
    p_person->SubState = SUB_STATE_ENTERING_VEHICLE;
    p_person->Genus.Person->Action = ACTION_ENTER_VEHICLE;
    p_person->Genus.Person->Flags |= (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

    {
        // Temporarily enables special offset mode in get_car_door_offsets() (vehicle.cpp)
        // so that position_person_for_vehicle() places the person at the right entry point.
        sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim = TRUE;

        position_person_for_vehicle(p_person, p_vehicle, door);

        sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim = FALSE;
    }

    set_locked_anim(p_person, (door) ? ANIM_ENTER_TAXI : ANIM_ENTER_CAR, SUB_OBJECT_LEFT_FOOT);
    p_person->Genus.Person->InCar = THING_NUMBER(p_vehicle);
    p_vehicle->Genus.Vehicle->Flags |= FLAG_VEH_ANIMATING;

    MFX_play_thing(THING_NUMBER(p_vehicle), S_CAR_DOOR, 0, p_vehicle);
}

// Appends person to the head of the vehicle's passenger linked list.
// uc_orig: add_person_to_passenger_list (fallen/Source/Person.cpp)
void add_person_to_passenger_list(Thing* p_person, Thing* p_vehicle)
{
    ASSERT(p_person->Class == CLASS_PERSON);
    ASSERT(p_vehicle->Class == CLASS_VEHICLE);

    p_person->Genus.Person->Passenger = p_vehicle->Genus.Vehicle->Passenger;
    p_vehicle->Genus.Vehicle->Passenger = THING_NUMBER(p_person);
}

// Removes person from the vehicle's passenger linked list (asserts if not found).
// uc_orig: remove_person_from_passenger_list (fallen/Source/Person.cpp)
void remove_person_from_passenger_list(Thing* p_person, Thing* p_vehicle)
{
    Thing* p_passenger;

    ASSERT(p_person->Class == CLASS_PERSON);
    ASSERT(p_vehicle->Class == CLASS_VEHICLE);

    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_PASSENGER);

    UWORD next;
    UWORD* prev;

    prev = &p_vehicle->Genus.Vehicle->Passenger;
    next = p_vehicle->Genus.Vehicle->Passenger;

    while (1) {
        if (next == NULL) {
            ASSERT(0);
            return;
        }

        p_passenger = TO_THING(next);

        ASSERT(p_passenger->Class == CLASS_PERSON);

        if (p_passenger == p_person) {
            *prev = p_person->Genus.Person->Passenger;

            return;
        }

        prev = &p_passenger->Genus.Person->Passenger;
        next = p_passenger->Genus.Person->Passenger;
    }
}

// Instantly places person inside vehicle as a passenger (no entry animation).
// uc_orig: set_person_passenger_in_vehicle (fallen/Source/Person.cpp)
void set_person_passenger_in_vehicle(Thing* p_person, Thing* p_vehicle, SLONG door)
{
    set_thing_velocity(p_person, 0);
    set_generic_person_state_function(p_person, STATE_MOVEING);
    remove_thing_from_map(p_person);
    add_person_to_passenger_list(p_person, p_vehicle);

    p_person->SubState = SUB_STATE_INSIDE_VEHICLE;
    p_person->Genus.Person->Action = ACTION_INSIDE_VEHICLE;
    p_person->Genus.Person->Flags |= (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C | FLAG_PERSON_PASSENGER);
    p_person->Genus.Person->InCar = THING_NUMBER(p_vehicle);
}

// Exits person from vehicle: finds a door (tries both sides), repositions on map,
// removes from driver/passenger lists, stops engine sounds.
// uc_orig: set_person_exit_vehicle (fallen/Source/Person.cpp)
void set_person_exit_vehicle(Thing* p_person)
{
    Thing* p_vehicle;

    p_vehicle = TO_THING(p_person->Genus.Person->InCar);

    SLONG door_x;
    SLONG door_y;
    SLONG door_z;
    SLONG side;
    SLONG otherside = FALSE;
    SLONG dx, dz;

    GameCoord newpos;

    side = (p_person->Genus.Person->Flags & FLAG_PERSON_PASSENGER);

try_again:;

    VEH_find_door(
        p_vehicle,
        side,
        &door_x,
        &door_z);

    dx = 0;
    dz = 0;

    SLONG mx = door_x + dx >> 8;
    SLONG mz = door_z + dz >> 8;

    door_y = PAP_calc_map_height_at(door_x + dx, door_z + dz);

    if (abs(door_y - (p_person->WorldPos.Y >> 8)) > 150 || (PAP_2HI(mx, mz).Flags & PAP_FLAG_NOGO) || !there_is_a_los(p_vehicle->WorldPos.X >> 8, p_vehicle->WorldPos.Y + 0x6000 >> 8, p_vehicle->WorldPos.Z >> 8, door_x + dx, door_y + 0x60, door_z + dz, LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
        if (!otherside) {
            side = !side;
            otherside = TRUE;

            goto try_again;
        } else {
            // Both doors blocked — exit inside the vehicle footprint.
            door_x = p_vehicle->WorldPos.X >> 8;
            door_z = p_vehicle->WorldPos.Z >> 8;
        }
    }

    newpos.X = door_x << 8;
    newpos.Y = PAP_calc_map_height_at(door_x, door_z);
    newpos.Z = door_z << 8;

    move_thing_on_map(p_person, &newpos);

    if (p_person->Genus.Person->Flags & FLAG_PERSON_PASSENGER) {
        remove_person_from_passenger_list(p_person, p_vehicle);

        p_person->Genus.Person->Flags &= ~(FLAG_PERSON_PASSENGER | FLAG_PERSON_DRIVING);
        p_person->Genus.Person->InCar = NULL;
        p_person->Genus.Person->Passenger = NULL;
    } else {
        p_person->Genus.Person->Flags &= ~(FLAG_PERSON_PASSENGER | FLAG_PERSON_DRIVING);
        p_person->Genus.Person->InCar = NULL;
        p_vehicle->Genus.Vehicle->Flags &= ~FLAG_FURN_DRIVING;
        p_vehicle->Genus.Vehicle->Driver = NULL;

        MFX_stop(THING_NUMBER(p_vehicle), S_CARX_START);
        MFX_stop(THING_NUMBER(p_vehicle), S_CARX_CRUISE);
        MFX_stop(THING_NUMBER(p_vehicle), S_CARX_IDLE);
        if (p_vehicle->Genus.Vehicle->Flags & FLAG_VEH_FX_STATE) {
            p_vehicle->Genus.Vehicle->Flags &= ~FLAG_VEH_FX_STATE;
        }
        MFX_stop(THING_NUMBER(p_vehicle), MFX_WAVE_ALL);
        MFX_play_thing(THING_NUMBER(p_vehicle), S_CARX_END, 0, p_vehicle);
    }

    add_thing_to_map(p_person);
    set_person_idle(p_person);
    plant_feet(p_person);
}

// Selects and plays the correct walk animation for person type and equipped weapon.
// uc_orig: set_anim_walking (fallen/Source/Person.cpp)
void set_anim_walking(Thing* p_person)
{
    SLONG anim;

    if (person_holding_2handed(p_person)) {
        if (p_person->Genus.Person->PersonType != PERSON_DARCI && p_person->Genus.Person->PersonType != PERSON_ROPER) {
            set_anim(p_person, ANIM_THUG_WALK_SHOTGUN);
            return;
        }
    }
    if (p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_THUG_RED || p_person->Genus.Person->PersonType == PERSON_THUG_GREY) {
        set_anim(p_person, ANIM_THUG_WALK);
    } else

        if (p_person->Genus.Person->PersonType == PERSON_COP) {
        SLONG old;
        old = p_person->Genus.Person->AnimType;
        set_anim_of_type(p_person, COP_ROPER_ANIM_WALK, ANIM_TYPE_ROPER);
        p_person->Genus.Person->AnimType = old;
    } else if (p_person->Genus.Person->PersonType == PERSON_CIV) {
        switch (p_person->Draw.Tweened->MeshID) {
        case 7:
        case 8:
            set_anim_of_type(p_person, CIV_M_ANIM_WALK, ANIM_TYPE_CIV);
            break;
        case 9:
            set_anim_of_type(p_person, CIV_F_ANIM_WALK, ANIM_TYPE_CIV);
            break;
        default:
            ASSERT(0);
            break;
        }
    } else {

        set_anim(p_person, ANIM_WALK);
    }
}

// Selects and plays the correct run animation for person type.
// uc_orig: set_anim_running (fallen/Source/Person.cpp)
void set_anim_running(Thing* p_person)
{
    SLONG anim;

    if (p_person->Genus.Person->PersonType == PERSON_ROPER) {
        set_anim(p_person, ANIM_YOMP);
    } else if (p_person->Genus.Person->PersonType == PERSON_COP) {
        SLONG old;
        old = p_person->Genus.Person->AnimType;
        set_anim_of_type(p_person, COP_ROPER_ANIM_RUN, ANIM_TYPE_ROPER);
        p_person->Genus.Person->AnimType = old;
    } else if (p_person->Genus.Person->PersonType == PERSON_CIV) {
        SLONG old;
        switch (p_person->Draw.Tweened->MeshID) {
        case 7:
        case 8:
            set_anim_of_type(p_person, CIV_M_ANIM_RUN, ANIM_TYPE_CIV);
            break;
        case 9:
            set_anim_of_type(p_person, CIV_F_ANIM_RUN, ANIM_TYPE_CIV);
            break;
        default:
            ASSERT(0);
            break;
        }
    } else {

        if (person_has_gun_out(p_person)) {
            set_anim(p_person, ANIM_AK_JOG);
        } else {
            set_anim(p_person, ANIM_RUN);
        }
    }
}

// Selects and plays the correct idle animation based on person type, mode, and weapon.
// uc_orig: set_anim_idle (fallen/Source/Person.cpp)
void set_anim_idle(Thing* p_person)
{
    SLONG anim;

    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        anim = find_idle_fight_stance(p_person);

    } else {
        if (person_holding_2handed(p_person) && p_person->Genus.Person->PersonType != PERSON_ROPER) {
            anim = ANIM_SHOTGUN_IDLE;
            set_anim(p_person, anim);
        } else {

            switch (p_person->Genus.Person->PersonType) {
            case PERSON_SLAG_TART:
            case PERSON_SLAG_FATUGLY:
                set_anim(p_person, ANIM_STAND_HIP);
                break;
            case PERSON_CIV:
                switch (p_person->Draw.Tweened->MeshID) {
                case 7:
                case 8:
                    set_anim_of_type(p_person, CIV_M_ANIM_STAND, ANIM_TYPE_CIV);
                    break;
                case 9:
                    set_anim(p_person, ANIM_STAND_HIP);

                    break;
                default:
                    ASSERT(0);
                    break;
                }
                break;
            case PERSON_COP:
                set_anim_of_type(p_person, COP_ROPER_ANIM_READY, ANIM_TYPE_ROPER);
                break;
            default:
                anim = ANIM_STAND_READY;
                set_anim(p_person, anim);
                break;
            }
        }
    }
}

// Starts walking at velocity 5 with the appropriate walk animation.
// uc_orig: set_person_walking (fallen/Source/Person.cpp)
void set_person_walking(Thing* p_person)
{
    set_anim_walking(p_person);

    set_thing_velocity(p_person, 5);
    set_generic_person_state_function(p_person, STATE_MOVEING);
    p_person->SubState = SUB_STATE_WALKING;
    p_person->Genus.Person->Action = ACTION_WALK;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
}

// Starts walking backwards at velocity -5.
// uc_orig: set_person_walk_backwards (fallen/Source/Person.cpp)
void set_person_walk_backwards(Thing* p_person)
{
    if (person_holding_2handed(p_person)) {
        set_anim(p_person, ANIM_BACK_WALK_AK);
    } else {
        set_anim(p_person, ANIM_BACK_WALK);
    }

    set_thing_velocity(p_person, -5);
    set_generic_person_state_function(p_person, STATE_MOVEING);
    p_person->SubState = SUB_STATE_WALKING_BACKWARDS;
    p_person->Genus.Person->Action = ACTION_WALK;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
}

// Starts sneaking (slow walk with sneak animation).
// uc_orig: set_person_sneaking (fallen/Source/Person.cpp)
void set_person_sneaking(Thing* p_person)
{
    set_anim(p_person, ANIM_SNEAK);

    set_thing_velocity(p_person, 5);
    set_generic_person_state_function(p_person, STATE_MOVEING);
    p_person->SubState = SUB_STATE_WALKING;
    p_person->Genus.Person->Action = ACTION_WALK;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
}

// Starts a backward hop (short backward jump, used in fight responses).
// uc_orig: set_person_hop_back (fallen/Source/Person.cpp)
void set_person_hop_back(Thing* p_person)
{
    set_thing_velocity(p_person, 0);
    set_generic_person_state_function(p_person, STATE_MOVEING);
    p_person->SubState = SUB_STATE_HOP_BACK;
    p_person->Genus.Person->Action = ACTION_HOP_BACK;
    p_person->Genus.Person->Timer1 = 3;
    p_person->Genus.Person->Flags |= (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
    set_anim(p_person, ANIM_BACK_HOP);
}

// Returns the best fight-idle animation for the person given their current weapon.
// uc_orig: find_idle_fight_stance (fallen/Source/Person.cpp)
SLONG find_idle_fight_stance(Thing* p_person)
{
    SLONG anim = ANIM_FIGHT;

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special;

        p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_KNIFE:
            anim = ANIM_KNIFE_FIGHT_READY;
            break;

        case SPECIAL_BASEBALLBAT:
            anim = ANIM_BAT_IDLE;

            break;
        case SPECIAL_SHOTGUN:
        case SPECIAL_AK47:
            break;
        }
    }
    return (anim);
}

// Transitions person to fight idle: if player has no attacker, exits fight mode.
// Plays the idle fight animation locking on foot or floor as appropriate.
// uc_orig: set_person_fight_idle (fallen/Source/Person.cpp)
void set_person_fight_idle(Thing* p_person)
{
    SLONG anim;

    p_person->Genus.Person->Flags &= ~FLAG_PERSON_KO;

    if (p_person->Genus.Person->PlayerID) {
        Thing* p_attacker = is_person_under_attack_low_level(p_person, FALSE, 0x200);

        if (p_attacker == NULL) {
            p_person->Genus.Person->Agression = 0;
            p_person->Genus.Person->Mode = PERSON_MODE_RUN;
            p_person->Genus.Person->Target = 0;
            set_person_idle(p_person);
            if (p_person->State == STATE_IDLE)
                set_anim(p_person, ANIM_UNFIGHT);
            GAME_FLAGS &= ~GF_SIDE_ON_COMBAT;
            return;
        }
    }

    person_enter_fight_mode(p_person);

    set_generic_person_state_function(p_person, STATE_IDLE);

    anim = find_idle_fight_stance(p_person);
    if (person_on_floor(p_person)) {
        set_anim(p_person, anim);
    } else {
        locked_anim_change(p_person, SUB_OBJECT_RIGHT_FOOT, anim, 0);
    }

    p_person->Draw.Tweened->AnimTween = 0;
    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_IDLE;
    p_person->SubState = 0;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

    plant_feet(p_person);
}

// Starts a fight step in one of four cardinal directions (0=N, 1=E, 2=S, 3=W).
// Uses bat-specific anims if person holds a bat.
// uc_orig: set_person_fight_step (fallen/Source/Person.cpp)
void set_person_fight_step(Thing* p_person, SLONG dir)
{
    SLONG anim;
    p_person->Genus.Person->Timer1 = 0;
    if (person_holding_bat(p_person)) {
        switch (dir) {
        case 0:
            anim = ANIM_FIGHT_STEP_N_BAT;
            break;
        case 1:
            anim = ANIM_FIGHT_STEP_E_BAT;
            break;
        case 2:
            anim = ANIM_FIGHT_STEP_S_BAT;
            break;
        case 3:
            anim = ANIM_FIGHT_STEP_W_BAT;
            break;
        }
    } else {
        switch (dir) {
        case 0:
            anim = ANIM_FIGHT_STEP_N;
            break;
        case 1:
            anim = ANIM_FIGHT_STEP_E;
            break;
        case 2:
            anim = ANIM_FIGHT_STEP_S;
            break;
        case 3:
            anim = ANIM_FIGHT_STEP_W;
            break;
        }
    }
    if (anim == p_person->Draw.Tweened->CurrentAnim)
        return;
    set_locked_anim(p_person, anim, SUB_OBJECT_LEFT_FOOT);
    set_generic_person_state_function(p_person, STATE_FIGHTING);
    p_person->SubState = SUB_STATE_STEP_FORWARD;
    p_person->Velocity = (p_person->Genus.Person->AnimType == ANIM_TYPE_ROPER) ? 10 : 20;
    p_person->Genus.Person->Timer1 = 0;
}

// Starts a fight step forward if not already in that animation.
// uc_orig: set_person_fight_step_forward (fallen/Source/Person.cpp)
void set_person_fight_step_forward(Thing* p_person)
{
    if (p_person->Draw.Tweened->CurrentAnim != ANIM_FIGHT_STEP_N) {
        set_anim(p_person, ANIM_FIGHT_STEP_N);
        set_generic_person_state_function(p_person, STATE_FIGHTING);
        p_person->SubState = SUB_STATE_STEP_FORWARD;
        p_person->Velocity = 10;
    }
}

// Plays the block animation for the current weapon (bat, shotgun, or fist).
// uc_orig: set_person_block (fallen/Source/Person.cpp)
void set_person_block(Thing* p_person)
{
    SLONG anim = ANIM_BLOCK_HIGH;

    set_generic_person_state_function(p_person, STATE_FIGHTING);

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special;

        p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_BASEBALLBAT:
            anim = ANIM_BAT_BLOCK;
            break;
        case SPECIAL_SHOTGUN:
            anim = ANIM_SHOTGUN_BLOCK;
            break;
        }
    }

    if (anim) {
        p_person->Draw.Tweened->Locked = 0;
        locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, anim, 0);
        p_person->Genus.Person->CombatNode = 0;
        p_person->Velocity = 0;
        p_person->Genus.Person->Action = ACTION_FIGHT_PUNCH;
        p_person->SubState = SUB_STATE_BLOCK;
        p_person->Draw.Tweened->Locked = 0;
        p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
    }
}

// Transitions to crouching idle state with the appropriate duck animation.
// uc_orig: set_person_idle_croutch (fallen/Source/Person.cpp)
void set_person_idle_croutch(Thing* p_person)
{
    SLONG anim;
    set_generic_person_state_function(p_person, STATE_IDLE);
    anim = ANIM_IDLE_CROUTCH;

    if (anim) {
        p_person->Draw.Tweened->Locked = 0;
        set_anim(p_person, anim);

        p_person->Genus.Person->CombatNode = 0;
        p_person->Velocity = 0;
        p_person->Genus.Person->Action = ACTION_CROUTCH;
        p_person->SubState = SUB_STATE_IDLE_CROUTCHING;

        p_person->Draw.Tweened->Locked = 0;
    }
}

// Forcibly releases person being carried when carrier is hit or shot.
// Sets carried person into a plunge-forward death animation.
// uc_orig: emergency_uncarry (fallen/Source/Person.cpp)
void emergency_uncarry(Thing* p_person)
{
    ASSERT(p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING);

    Thing* p_target = TO_THING(p_person->Genus.Person->Target);

    p_target->DY = -(4 << 8);
    p_target->OnFace = 0;
    p_target->SubState = SUB_STATE_DYING_PRONE;

    set_generic_person_state_function(p_target, STATE_DYING);

    locked_anim_change(p_target, 0, ANIM_PLUNGE_FORWARD, 0);

    p_person->Genus.Person->Flags2 &= ~FLAG2_PERSON_CARRYING;
}

// Updates the carried person's world position to stay 90 units in front of carrier.
// Called each frame during carry-running state.
// uc_orig: carry_running (fallen/Source/Person.cpp)
void carry_running(Thing* p_person)
{
    Thing* p_target;

    SLONG dx;
    SLONG dz;
    GameCoord new_position = p_person->WorldPos;
    p_target = TO_THING(p_person->Genus.Person->Target);

    dx = -(SIN(p_person->Draw.Tweened->Angle) * 90) >> 8;
    dz = -(COS(p_person->Draw.Tweened->Angle) * 90) >> 8;
    new_position.X += dx;
    new_position.Z += dz;

    move_thing_on_map(p_target, &new_position);
    p_target->Draw.Tweened->Angle = p_person->Draw.Tweened->Angle + 1024;
    p_target->Draw.Tweened->Angle &= 2047;
}

// Begins carrying another person: sets carry state, pickup animations, and positions victim.
// s_index is the thing-number of the victim to carry.
// uc_orig: set_person_carry (fallen/Source/Person.cpp)
void set_person_carry(Thing* p_person, SLONG s_index)
{
    Thing* p_target;
    SLONG dx;
    SLONG dz;
    GameCoord new_position = p_person->WorldPos;

    p_person->Genus.Person->Mode = PERSON_MODE_RUN;
    p_person->Genus.Person->Flags2 |= FLAG2_PERSON_CARRYING;

    dx = -(SIN(p_person->Draw.Tweened->Angle) * 90) >> 8;
    dz = -(COS(p_person->Draw.Tweened->Angle) * 90) >> 8;
    new_position.X += dx;
    new_position.Z += dz;

    p_person->Genus.Person->Target = s_index;
    p_target = TO_THING(s_index);
    set_generic_person_state_function(p_person, STATE_CARRY);
    set_anim(p_person, ANIM_PICKUP_CARRY);
    p_person->SubState = SUB_STATE_PICKUP_CARRY;

    set_anim(p_target, ANIM_PICKUP_CARRY_V);
    p_target->SubState = SUB_STATE_PICKUP_CARRY_V;

    move_thing_on_map(p_target, &new_position);
    p_target->Draw.Tweened->Angle = p_person->Draw.Tweened->Angle + 1024;
    p_target->Draw.Tweened->Angle &= 2047;
    if (p_target->Genus.Person->Target) {
        remove_from_gang_attack(p_target, TO_THING(p_target->Genus.Person->Target));
    }
}

// --- chunk 6: set_person_uncarry..set_person_pos_for_fence_vault ---

// Starts the put-down carry animation for both carrier and victim.
// uc_orig: set_person_uncarry (fallen/Source/Person.cpp)
void set_person_uncarry(Thing* p_person)
{
    Thing* p_target;
    GameCoord new_position = p_person->WorldPos;

    p_target = TO_THING(p_person->Genus.Person->Target);
    set_generic_person_state_function(p_person, STATE_CARRY);
    set_anim(p_person, ANIM_PUTDOWN_CARRY);
    p_person->SubState = SUB_STATE_DROP_CARRY;

    set_anim(p_target, ANIM_PUTDOWN_CARRY_V);
    p_target->SubState = SUB_STATE_DROP_CARRY_V;
}

// Switches to standing carry state (both carrier and victim play stand-carry anims).
// uc_orig: set_person_stand_carry (fallen/Source/Person.cpp)
void set_person_stand_carry(Thing* p_person)
{
    Thing* p_target;
    p_target = TO_THING(p_person->Genus.Person->Target);

    set_generic_person_state_function(p_person, STATE_CARRY);

    set_anim(p_person, ANIM_STAND_CARRY);
    p_person->SubState = SUB_STATE_STAND_CARRY;

    set_anim(p_target, ANIM_STAND_CARRY_V);
    p_target->SubState = SUB_STATE_STAND_CARRY_V;
}

// State handler for the carry state: handles pickup, put-down and continuous stand-carry.
// uc_orig: fn_person_carry (fallen/Source/Person.cpp)
void fn_person_carry(Thing* p_person)
{
    SLONG end;
    Thing* p_target;

    p_target = TO_THING(p_person->Genus.Person->Target);
    switch (p_person->SubState) {
    case SUB_STATE_PICKUP_CARRY:
        end = person_normal_animate(p_person);
        if (end) {
            set_person_stand_carry(p_person);
        }
        break;
    case SUB_STATE_DROP_CARRY:
        end = person_normal_animate(p_person);
        if (end) {
            p_person->Genus.Person->Flags2 &= ~FLAG2_PERSON_CARRYING;
            set_person_idle(p_person);
        }
        break;
    case SUB_STATE_STAND_CARRY: {
        SLONG dx;
        SLONG dz;
        GameCoord new_position = p_person->WorldPos;

        dx = -(SIN(p_person->Draw.Tweened->Angle) * 90) >> 8;
        dz = -(COS(p_person->Draw.Tweened->Angle) * 90) >> 8;
        new_position.X += dx;
        new_position.Z += dz;
        move_thing_on_map(p_target, &new_position);
        p_target->Draw.Tweened->Angle = (p_person->Draw.Tweened->Angle + 1024) & 2047;
        p_target->Draw.Tweened->Roll = (2048 - p_person->Draw.Tweened->Roll) & 2047;
    } break;
    }
}

// Initiates the arrest sequence: sets arresting officer into crouch-arrest idle,
// positions and cuffs the target, plays arrest sound, and alerts nearby NPCs.
// s_index is the thing-number of the person being arrested.
// uc_orig: set_person_arrest (fallen/Source/Person.cpp)
void set_person_arrest(Thing* p_person, SLONG s_index)
{
    SLONG anim;
    ASSERT(s_index);

    set_generic_person_state_function(p_person, STATE_IDLE);
    anim = ANIM_ARREST_CROUTCH;

    p_person->Draw.Tweened->Locked = 0;
    locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, anim, 0);

    p_person->Genus.Person->CombatNode = 0;
    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_CROUTCH;
    p_person->SubState = SUB_STATE_IDLE_CROUTCH_ARREST;

    p_person->Draw.Tweened->Locked = 0;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
    p_person->Genus.Person->Target = s_index;

    if (s_index) {
        SLONG ax, ay, az;
        Thing* p_target;
        SLONG dx, dz;
        SLONG on_what_side;
        GameCoord new_position;

        p_target = TO_THING(s_index);
        p_target->Genus.Person->Flags |= FLAG_PERSON_ARRESTED;

        switch (p_target->Genus.Person->PersonType) {
        case PERSON_THUG_RASTA:
        case PERSON_THUG_GREY:
        case PERSON_THUG_RED:
        case PERSON_MIB1:
        case PERSON_MIB2:
        case PERSON_MIB3:
            stat_arrested_thug++;
            break;
        default:
            stat_arrested_innocent++;
            break;
        }

        on_what_side = person_is_lying_on_what(p_target);

        if (on_what_side == PERSON_ON_HIS_BACK) {
            set_locked_anim(p_target, ANIM_ARREST_ROLL, 0);
            p_target->SubState = SUB_STATE_DEAD_ARREST_TURN_OVER;
        } else {
            set_locked_anim(p_target, ANIM_ARREST_BE_CUFFED, 0);
            p_target->SubState = SUB_STATE_DEAD_CUFFED;
            p_target->Draw.Tweened->Angle &= 2047;
        }

        calc_sub_objects_position(p_target, p_target->Draw.Tweened->AnimTween, SUB_OBJECT_PELVIS, &ax, &ay, &az);

        ax += p_target->WorldPos.X >> 8;
        ay += p_target->WorldPos.Y >> 8;
        az += p_target->WorldPos.Z >> 8;

        dx = (SIN(p_target->Draw.Tweened->Angle) * 60) >> 16;
        dz = (COS(p_target->Draw.Tweened->Angle) * 60) >> 16;

        ax -= dx;
        az -= dz;

        dx = ((p_person->WorldPos.X >> 8) - ax) << 8;
        dz = ((p_person->WorldPos.Z >> 8) - az) << 8;

        new_position.X = p_target->WorldPos.X + dx;
        new_position.Y = p_target->WorldPos.Y;
        new_position.Z = p_target->WorldPos.Z + dz;

        move_thing_on_map(p_target, &new_position);

        p_person->Draw.Tweened->Angle = (p_target->Draw.Tweened->Angle + 1024) & 2047;

        if (!remove_from_gang_attack(p_target, p_person)) {
            Thing* p_victim;
            if (p_target->Genus.Person->Target) {
                p_victim = TO_THING(p_target->Genus.Person->Target);
                if (p_victim->Class == CLASS_PERSON)
                    remove_from_gang_attack(p_target, p_victim);
            }
        }

        // Set arrested person to dead state so they don't cause further trouble.
        set_generic_person_state_function(p_target, STATE_DEAD);
        p_target->Genus.Person->Action = ACTION_DEAD;
        p_target->Genus.Person->Timer1 = 0;

        switch (p_person->Genus.Person->PersonType) {
        case PERSON_DARCI:
            MFX_play_thing(THING_NUMBER(p_person), SOUND_Range(S_DARCI_ARREST_START, S_DARCI_ARREST_END), MFX_MOVING | MFX_OVERLAP, p_person);
            break;
        case PERSON_COP:
            MFX_play_thing(THING_NUMBER(p_person), SOUND_Range(S_COP_ARREST_START, S_COP_ARREST_END), MFX_MOVING | MFX_OVERLAP, p_person);
            break;
        }

        PCOM_oscillate_tympanum(
            PCOM_SOUND_HEY,
            p_person,
            p_person->WorldPos.X >> 8,
            p_person->WorldPos.Y >> 8,
            p_person->WorldPos.Z >> 8);
    }
}

// Transitions person into crouching idle; if Darci and an arrestee is nearby, arrests them instead.
// uc_orig: set_person_croutch (fallen/Source/Person.cpp)
void set_person_croutch(Thing* p_person)
{
    SLONG anim;
    SLONG index;

    if (p_person->Genus.Person->PersonType == PERSON_DARCI && (index = find_arrestee(p_person))) {
        set_person_arrest(p_person, index);
        return;
    }

    set_generic_person_state_function(p_person, STATE_IDLE);
    if (person_has_gun_out(p_person)) {
        if (person_holding_2handed(p_person)) {
            anim = ANIM_SHOTGUN_DUCK;
        } else {
            anim = ANIM_PISTOL_DUCK;
        }
    } else {
        anim = ANIM_CROUTCH_DOWN;
    }

    if (anim) {
        // locked_anim_change causes Roper to slide backwards, so plain set_anim is used here.
        p_person->Draw.Tweened->Locked = 0;
        set_anim(p_person, anim);
        p_person->Genus.Person->CombatNode = 0;
        p_person->Velocity = 0;
        p_person->Genus.Person->Action = ACTION_CROUTCH;
        p_person->SubState = SUB_STATE_IDLE_CROUTCH;
        p_person->Draw.Tweened->Locked = 0;
    }
}

// Transitions person into crawling state. Sniper uses PELVIS lock; others use LEFT_FOOT lock.
// If the person has a gun out and is a sniper, also slides them a bit forward first.
// uc_orig: set_person_crawling (fallen/Source/Person.cpp)
void set_person_crawling(Thing* p_person)
{
    set_generic_person_state_function(p_person, STATE_MOVEING);

    if (person_has_gun_out(p_person)) {
        SLONG x1 = p_person->WorldPos.X;
        SLONG y1 = p_person->WorldPos.Y;
        SLONG z1 = p_person->WorldPos.Z;

        SLONG x2 = p_person->WorldPos.X;
        SLONG y2 = p_person->WorldPos.Y;
        SLONG z2 = p_person->WorldPos.Z;

        slide_along(
            x1, y1, z2,
            &x2, &y2, &z2,
            0,
            50,
            0);

        GameCoord newpos;
        newpos.X = x2;
        newpos.Y = y2;
        newpos.Z = z2;
        move_thing_on_map(p_person, &newpos);

        p_person->Draw.Tweened->Locked = 0;
        locked_anim_change(p_person, SUB_OBJECT_PELVIS, ANIM_SNIPER_CRAWL, 0);
    } else {
        p_person->Draw.Tweened->Locked = 0;
        locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, ANIM_CRAWL, 0);
    }

    p_person->Genus.Person->CombatNode = 0;
    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_CRAWLING;
    p_person->SubState = SUB_STATE_CRAWLING;
    p_person->Draw.Tweened->Locked = 0;
}

// Initiates a leg-sweep kick attack (fight state, kick sub-state).
// uc_orig: set_person_leg_sweep (fallen/Source/Person.cpp)
SLONG set_person_leg_sweep(Thing* p_person)
{
    set_generic_person_state_function(p_person, STATE_FIGHTING);
    set_anim(p_person, ANIM_LEG_SWEEP);
    p_person->Genus.Person->CombatNode = 0;
    p_person->SubState = SUB_STATE_KICK;
    return (0);
}

// Initiates a punch attack. Checks for better grapple first.
// Handles knife, bat, and shotgun/AK variants. Tracks Darci's punch EWAY event.
// uc_orig: set_person_punch (fallen/Source/Person.cpp)
SLONG set_person_punch(Thing* p_person)
{
    SLONG anim;
    SLONG node = 1;

    p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_PUNCH;

    if (!p_person->Genus.Person->PlayerID) {
        // For NPC: don't punch during cutscenes (unfair to player).
        UWORD i_target = PCOM_person_wants_to_kill(p_person);
        if (i_target) {
            Thing* p_target = TO_THING(i_target);
            if (dont_hurt_target_during_cutscene(p_person, p_target)) {
                return 0;
            }
        }
    }

    if (find_best_grapple(p_person)) {
        return (0);
    }

    set_generic_person_state_function(p_person, STATE_FIGHTING);

    anim = ANIM_PUNCH_COMBO1;

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special;
        p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_KNIFE:
            anim = ANIM_KNIFE_ATTACK1;
            node = 14;
            break;

        case SPECIAL_BASEBALLBAT:
            anim = ANIM_BAT_HIT1;
            break;
            node = 19; // unreachable — original copy-paste bug, kept 1:1
            break;

        case SPECIAL_SHOTGUN:
        case SPECIAL_AK47:
            switch ((Random() >> 4) & 1) {
            case 0:
                anim = ANIM_SHOTGUN_WHIP1;
                break;
            case 1:
                anim = ANIM_SHOTGUN_WHIP2;
                break;
            }
            break;
        }
    }

    if (anim) {
        if (anim == ANIM_PUNCH_COMBO1 && p_person == NET_PERSON(0)) {
            EWAY_darci_move |= EWAY_DARCI_MOVE_PUNCH;
        }

        p_person->Draw.Tweened->Locked = 0;
        set_anim(p_person, anim);
        p_person->Genus.Person->CombatNode = node;
        p_person->Velocity = 0;
        p_person->Genus.Person->Action = ACTION_FIGHT_PUNCH;
        p_person->SubState = SUB_STATE_PUNCH;
        p_person->Draw.Tweened->Locked = 0;
        p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
    }

    return (anim);
}

// Initiates a directional kick. dir: 0=N, 1=E, 2=S, 3=W.
// uc_orig: set_person_kick_dir (fallen/Source/Person.cpp)
SLONG set_person_kick_dir(Thing* p_person, SLONG dir)
{
    SLONG anim;
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_KICK;

    if (p_person->Genus.Person->PlayerID) {
        camera_fight();
    }

    set_generic_person_state_function(p_person, STATE_FIGHTING);

    anim = ANIM_KICK_COMBO1;

    switch (dir) {
    case 0: // n
        break;
    case 1: // e
        anim = ANIM_KICK_RIGHT;
        break;
    case 2: // s
        anim = ANIM_KICK_BEHIND;
        break;
    case 3: // w
        anim = ANIM_KICK_LEFT;
        break;
    }

    if (p_person == NET_PERSON(0)) {
        EWAY_darci_move |= EWAY_DARCI_MOVE_KICK;
    }

    if (anim) {
        p_person->Draw.Tweened->Locked = 0;
        locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, anim, 0);
        p_person->Genus.Person->CombatNode = 1;
        p_person->Velocity = 0;
        p_person->Genus.Person->Action = ACTION_FIGHT_PUNCH;
        p_person->SubState = SUB_STATE_KICK;
        p_person->Draw.Tweened->Locked = 0;
        p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
    }

    return (anim);
}

// Initiates a fight animation by anim type, entering fight state.
// uc_orig: set_person_fight_anim (fallen/Source/Person.cpp)
void set_person_fight_anim(Thing* p_person, SLONG anim)
{
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_KICK;
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_PUNCH;

    set_generic_person_state_function(p_person, STATE_FIGHTING);

    if (anim) {
        p_person->Draw.Tweened->Locked = 0;
        locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, anim, 0);
        p_person->Genus.Person->CombatNode = 1;
        p_person->Velocity = 0;
        p_person->Genus.Person->Action = ACTION_FIGHT_PUNCH;
        p_person->SubState = SUB_STATE_KICK;
        p_person->Draw.Tweened->Locked = 0;
        p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
    }
}

// Initiates a standard front kick (ANIM_KICK_COMBO1). Tracks Darci's kick EWAY event.
// Checks for cutscene protection for NPC attackers.
// uc_orig: set_person_kick (fallen/Source/Person.cpp)
SLONG set_person_kick(Thing* p_person)
{
    SLONG anim;

    p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_KICK;

    if (!p_person->Genus.Person->PlayerID) {
        UWORD i_target = PCOM_person_wants_to_kill(p_person);
        if (i_target) {
            Thing* p_target = TO_THING(i_target);
            if (dont_hurt_target_during_cutscene(p_person, p_target)) {
                return 0;
            }
        }
    }

    if (p_person == NET_PERSON(0)) {
        EWAY_darci_move |= EWAY_DARCI_MOVE_KICK;
    }

    set_generic_person_state_function(p_person, STATE_FIGHTING);

    anim = ANIM_KICK_COMBO1;
    p_person->Draw.Tweened->Locked = 0;
    p_person->Genus.Person->CombatNode = 6;
    set_anim(p_person, anim);
    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_FIGHT_PUNCH;
    p_person->SubState = SUB_STATE_KICK;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
    return (anim);
}

// Initiates a near kick: ANIM_KICK_NAD if close + Darci + stepping forward, else ANIM_KICK_NEAR.
// uc_orig: set_person_kick_near (fallen/Source/Person.cpp)
SLONG set_person_kick_near(Thing* p_person, SLONG dist)
{
    SLONG anim;
    SLONG not_nad = 0;

    if (p_person == NET_PERSON(0)) {
        EWAY_darci_move |= EWAY_DARCI_MOVE_KICK;
    }

    p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_KICK;

    set_generic_person_state_function(p_person, STATE_FIGHTING);

    if (p_person->Genus.Person->Target) {
        Thing* p_victim;
        p_victim = TO_THING(p_person->Genus.Person->Target);

        if (p_victim->Draw.Tweened->CurrentAnim == ANIM_KICK_NAD_TAKE
         || p_victim->Draw.Tweened->CurrentAnim == ANIM_KICK_NAD_STUNNED
         || p_victim->Draw.Tweened->CurrentAnim == ANIM_KICK_NAD_RECOVER)
            not_nad = 1;
    }

    if (p_person->Genus.Person->PersonType == PERSON_DARCI && dist < 100
     && p_person->SubState == SUB_STATE_STEP_FORWARD && !not_nad)
        anim = ANIM_KICK_NAD;
    else
        anim = ANIM_KICK_NEAR;

    p_person->Draw.Tweened->Locked = 0;
    p_person->Genus.Person->CombatNode = 0;

    if (p_person->SubState == SUB_STATE_STEP_FORWARD)
        set_anim(p_person, anim);
    else
        locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, anim, 0);

    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_FIGHT_PUNCH;
    p_person->SubState = SUB_STATE_KICK;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
    return (anim);
}

// Initiates a stomp attack (stomp on a downed opponent).
// uc_orig: set_person_stomp (fallen/Source/Person.cpp)
SLONG set_person_stomp(Thing* p_person)
{
    SLONG anim;

    p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_KICK;
    if (p_person->Genus.Person->PlayerID) {
        camera_fight();
    }

    set_generic_person_state_function(p_person, STATE_FIGHTING);

    anim = ANIM_FIGHT_STOMP;
    p_person->Draw.Tweened->Locked = 0;
    p_person->Genus.Person->CombatNode = 0;
    locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, anim, 0);
    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_FIGHT_PUNCH;
    p_person->SubState = SUB_STATE_KICK;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
    return (anim);
}

// Positions a person at the base of a ladder facet, facing the correct direction.
// scale=256 for most persons; scale=320 for Roper.
// uc_orig: set_person_position_for_ladder (fallen/Source/Person.cpp)
void set_person_position_for_ladder(Thing* p_person, UWORD facet)
{
    SLONG angle, px, pz;
    GameCoord new_position;
    SLONG scale = 256;

    if (p_person->Genus.Person->PersonType == PERSON_ROPER)
        scale = 320;

    correct_pos_for_ladder(&dfacets[facet], &px, &pz, &angle, scale);

    new_position.X = px << 8;
    new_position.Y = p_person->WorldPos.Y;
    new_position.Z = pz << 8;

    p_person->Draw.Tweened->Angle = angle;

    move_thing_on_map(p_person, &new_position);
}

// Plays a terrain-appropriate sound when jumping (e.g. sewer water splash).
// Uses a static channel so the sound can be re-used without stacking.
// uc_orig: play_jump_sound (fallen/Source/Person.cpp)
inline void play_jump_sound(Thing* p_person)
{
    static SLONG jump_chan = 0;
    SLONG jump_snd = 0;
    if (p_person->Flags & FLAGS_IN_SEWERS) {
        switch (person_is_on_sewer(p_person)) {
        case PERSON_ON_SEWATER:
            jump_snd = S_CLIMB_SEWER;
            break;
        case PERSON_ON_WATER:
            jump_snd = S_FOOTS_PUDDLE_START;
            break;
        }
    }
    if (jump_snd)
        MFX_play_thing(THING_NUMBER(p_person), jump_snd, 0, p_person);
}

// Starts a person climbing a ladder: sets STATE_CLIMB_LADDER, positions them, plays jump sound.
// Roper, cops, and thugs use the roper-type ladder animation.
// uc_orig: set_person_climb_ladder (fallen/Source/Person.cpp)
void set_person_climb_ladder(Thing* p_person, UWORD storey)
{
    set_generic_person_state_function(p_person, STATE_CLIMB_LADDER);
    if (p_person->Genus.Person->PersonType == PERSON_ROPER) {
        set_anim_of_type(p_person, COP_ROPER_ANIM_LADDER_START, ANIM_TYPE_ROPER);
    } else if (p_person->Genus.Person->PersonType == PERSON_COP
            || p_person->Genus.Person->PersonType == PERSON_THUG_GREY
            || p_person->Genus.Person->PersonType == PERSON_THUG_RASTA
            || p_person->Genus.Person->PersonType == PERSON_THUG_RED) {
        set_anim_of_type(p_person, COP_ROPER_ANIM_LADDER_START, ANIM_TYPE_ROPER);
    } else {
        set_anim(p_person, ANIM_MOUNT_LADDER);
    }

    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_CLIMBING;
    p_person->SubState = SUB_STATE_MOUNT_LADDER;
    p_person->Genus.Person->OnFacet = storey;
    set_person_position_for_ladder(p_person, storey);
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;

    play_jump_sound(p_person);
}

// Switches to the looping on-ladder animation and enters the on-ladder sub-state.
// Roper, cops, and thugs use the roper-type ladder loop animation.
// uc_orig: set_person_on_ladder (fallen/Source/Person.cpp)
void set_person_on_ladder(Thing* p_person)
{
    if (p_person->Genus.Person->PersonType == PERSON_ROPER) {
        locked_anim_change_of_type(p_person, 0, COP_ROPER_ANIM_LADDER_LOOP, ANIM_TYPE_ROPER);
    } else if (p_person->Genus.Person->PersonType == PERSON_COP
            || p_person->Genus.Person->PersonType == PERSON_THUG_GREY
            || p_person->Genus.Person->PersonType == PERSON_THUG_RASTA
            || p_person->Genus.Person->PersonType == PERSON_THUG_RED) {
        locked_anim_change_of_type(p_person, 0, COP_ROPER_ANIM_LADDER_LOOP, ANIM_TYPE_ROPER);
    } else {
        locked_anim_change(p_person, 0, ANIM_ON_LADDER, 0);
    }

    // Ensure the person is not in fight mode while on the ladder.
    p_person->Genus.Person->Mode = PERSON_MODE_RUN;

    p_person->Draw.Tweened->AnimTween = 0;
    p_person->Velocity = 0;
    p_person->SubState = SUB_STATE_ON_LADDER;
    p_person->Genus.Person->Action = ACTION_CLIMBING;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M);
    p_person->OnFace = NULL;
}

// Starts the climb-up-fence animation used when landing on a climbable fence.
// uc_orig: set_person_on_fence (fallen/Source/Person.cpp)
void set_person_on_fence(Thing* p_person)
{
    set_anim(p_person, ANIM_CLIMB_UP_FENCE);
    p_person->Draw.Tweened->AnimTween = 0;
    p_person->Velocity = 0;
    p_person->SubState = SUB_STATE_CLIMB_AROUND_WALL;
    p_person->Genus.Person->Action = ACTION_CLIMBING;
    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
    p_person->OnFace = NULL;
}

// Initiates a standing (in-place) jump. Checks for a climbable fence in front first —
// if found, climbs it instead of jumping. Skips if inside a building or slipping.
// uc_orig: set_person_standing_jump (fallen/Source/Person.cpp)
void set_person_standing_jump(Thing* p_person)
{
    SLONG anim;
    if (p_person->Genus.Person->InsideIndex)
        return;
    if (p_person->SubState == SUB_STATE_SLIPPING)
        return;

    // Check for climbable fence in front of person.
    {
        SLONG x1;
        SLONG y1;
        SLONG z1;

        SLONG x2;
        SLONG y2;
        SLONG z2;

        SLONG dx = -SIN(p_person->Draw.Tweened->Angle) >> 2;
        SLONG dz = -COS(p_person->Draw.Tweened->Angle) >> 2;

        x1 = p_person->WorldPos.X >> 8;
        y1 = p_person->WorldPos.Y + 0xa000 >> 8;
        z1 = p_person->WorldPos.Z >> 8;

        x2 = p_person->WorldPos.X + dx >> 8;
        y2 = p_person->WorldPos.Y + 0xa000 >> 8;
        z2 = p_person->WorldPos.Z + dz >> 8;

        if (!there_is_a_los(
                x1, y1, z1,
                x2, y2, z2,
                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
            if (los_failure_dfacet) {
                DFacet* df;
                df = &dfacets[los_failure_dfacet];

                if (df->FacetType == STOREY_TYPE_FENCE
                 || df->FacetType == STOREY_TYPE_FENCE_FLAT
                 || df->FacetType == STOREY_TYPE_FENCE_BRICK) {
                    if (!(df->FacetFlags & FACET_FLAG_UNCLIMBABLE)) {
                        set_person_land_on_fence(p_person, los_failure_dfacet, 1);
                        return;
                    }
                }
            }
        }
    }

    if (person_holding_2handed(p_person))
        anim = ANIM_STANDING_JUMP_AK;
    else
        anim = ANIM_STANDING_JUMP;

    set_generic_person_state_function(p_person, STATE_JUMPING);
    set_anim(p_person, anim);
    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_STANDING_JUMP;
    p_person->SubState = SUB_STATE_STANDING_JUMP;
    play_jump_sound(p_person);
    p_person->OnFace = 0;
}

// Initiates a forward standing jump. Skips if on a steep slope or inside a building.
// Delegates to set_person_running_jump for the actual state setup.
// uc_orig: set_person_standing_jump_forwards (fallen/Source/Person.cpp)
void set_person_standing_jump_forwards(Thing* p_person)
{
    SLONG slope;
    SLONG angle;
    if (p_person->OnFace < 0) {
        slope = RFACE_on_slope(-p_person->OnFace, p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle);
    } else {
        slope = PAP_on_slope(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle) >> 1;
    }
    if (slope > 50)
        return;

    if (p_person->Genus.Person->InsideIndex)
        return;
    if (p_person->SubState == SUB_STATE_SLIPPING)
        return;
    set_person_running_jump(p_person);
}

// Initiates a backwards standing jump (back-flip). Skips if on steep slope/inside/slipping.
// uc_orig: set_person_standing_jump_backwards (fallen/Source/Person.cpp)
void set_person_standing_jump_backwards(Thing* p_person)
{
    SLONG slope;
    SLONG angle;
    if (p_person->OnFace < 0) {
        slope = RFACE_on_slope(-p_person->OnFace, p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle);
    } else {
        slope = PAP_on_slope(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle) >> 1;
    }
    if (slope > 50)
        return;

    if (p_person->Genus.Person->InsideIndex)
        return;
    if (p_person->SubState == SUB_STATE_SLIPPING)
        return;
    set_person_running_jump(p_person);

    set_generic_person_state_function(p_person, STATE_JUMPING);
    set_anim(p_person, ANIM_BACK_FLIP);
    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_STANDING_JUMP;
    p_person->SubState = SUB_STATE_STANDING_JUMP_BACKWARDS;
    play_jump_sound(p_person);
    p_person->OnFace = 0;
}

// Forward declaration for set_person_running_jump_lr (used inside set_person_running_jump).
// uc_orig: set_person_running_jump_lr (fallen/Source/Person.cpp)
void set_person_running_jump_lr(Thing* p_person, SLONG dir);

// Initiates a running jump: STATE_JUMPING + run-jump-left anim, sets initial DY.
// Skips if on steep slope, slipping, or inside. NPC jump speed from PCOM.
// uc_orig: set_person_running_jump (fallen/Source/Person.cpp)
void set_person_running_jump(Thing* p_person)
{
    SLONG slope;
    SLONG angle;
    if (p_person->OnFace < 0) {
        slope = RFACE_on_slope(-p_person->OnFace, p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle);
    } else {
        slope = PAP_on_slope(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle) >> 1;
    }
    if (slope > 50)
        return;

    if (p_person->SubState == SUB_STATE_SLIPPING || p_person->Genus.Person->InsideIndex)
        return;

    set_generic_person_state_function(p_person, STATE_JUMPING);
    if (person_has_gun_out(p_person)) {
        set_anim(p_person, ANIM_RUN_JUMP_LEFT_AK);
    } else {
        set_anim(p_person, ANIM_RUN_JUMP_LEFT);
    }

    if (p_person->Genus.Person->PlayerID) {
        if (p_person->Velocity < 16) {
            p_person->Velocity = 16;
        }
    } else {
        p_person->Velocity = PCOM_if_i_wanted_to_jump_how_fast_should_i_do_it(p_person);
    }

    p_person->DY = 0;
    p_person->DY = 10 << 8;
    p_person->Genus.Person->Action = ACTION_RUNNING_JUMP;
    p_person->SubState = SUB_STATE_RUNNING_JUMP;
    p_person->OnFace = 0;

    play_jump_sound(p_person);
}

// Stub for left/right running jump variant — body was removed before shipping in original.
// uc_orig: set_person_running_jump_lr (fallen/Source/Person.cpp)
void set_person_running_jump_lr(Thing* p_person, SLONG dir)
{
}

// Checks whether a traverse move is possible in the given direction (right=1, left=0)
// by predicting hand position at the end of the traverse arc and finding a grab face.
// Also repositions person to align with the current grab face to reduce drift.
// Returns 1 if traverse is possible, 0 if blocked.
// uc_orig: traverse_pos (fallen/Source/Person.cpp)
SLONG traverse_pos(Thing* p_person, SLONG right)
{
    SLONG lhx, lhy, lhz;
    SLONG angle;
    SLONG grab_x, grab_y, grab_z, type;
    SLONG x, y, z;
    SLONG face, grab_angle;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_HAND, &lhx, &lhy, &lhz);

    x = lhx + (p_person->WorldPos.X >> 8);
    y = lhy + (p_person->WorldPos.Y >> 8);
    z = lhz + (p_person->WorldPos.Z >> 8);

    if (right) {
        angle = (p_person->Draw.Tweened->Angle - 512) & 2047;
    } else {
        angle = (p_person->Draw.Tweened->Angle + 512) & 2047;
    }

    // Predict hand position at end of sideways movement.
    x += -(SIN(angle) * 74) >> 16;
    z += -(COS(angle) * 74) >> 16;

    // If the destination square is too near a nogo zone or underground, abort.
    {
        SLONG dx;
        SLONG dz;
        SLONG cx;
        SLONG cz;

        for (dx = -32; dx <= 32; dx += 32)
            for (dz = -32; dz <= 32; dz += 32) {
                cx = x + dx;
                cz = z + dz;

                if (!WITHIN(cx >> 8, 0, PAP_SIZE_HI - 1) || !WITHIN(cz >> 8, 0, PAP_SIZE_HI - 1)) {
                    return FALSE;
                }

                if (PAP_calc_map_height_at(cx, cz) > y + 0x30) {
                    return FALSE;
                }

                cx >>= 8;
                cz >>= 8;

                if (PAP_2HI(cx, cz).Flags & PAP_FLAG_NOGO) {
                    return FALSE;
                }
            }
    }

    // Check if there is a face the hands can grab at the predicted end position.
    face = find_grab_face(
        x, y, z,
        20, 40,
        p_person->Draw.Tweened->Angle,
        &grab_x,
        &grab_y,
        &grab_z,
        &grab_angle,
        0,
        0, &type,
        p_person);

    if (face == 0)
        return (0);

    if (type != 0)
        return (0); // ladder or cable or other odd face type — can't traverse there

    {
        SLONG new_x, new_y, new_z;
        GameCoord temp_pos;

        // Revert predicted offset to get back to current hand position.
        x -= -(SIN(angle) * 74) >> 16;
        z -= -(COS(angle) * 74) >> 16;

        // Repos against current grab face to keep person aligned on straight ledge.
        face = find_grab_face(
            x, y, z,
            20, 40,
            p_person->Draw.Tweened->Angle,
            &grab_x,
            &grab_y,
            &grab_z,
            &grab_angle,
            0,
            0, &type,
            p_person);

        if (face == 0) {
            return (1); // no current face to repos against — proceed without repos
            ASSERT(0);
        }

        calc_sub_objects_position(p_person, 0, SUB_OBJECT_LEFT_HAND, &new_x, &new_y, &new_z);

        new_x += p_person->WorldPos.X >> 8;
        new_y += p_person->WorldPos.Y >> 8;
        new_z += p_person->WorldPos.Z >> 8;

        temp_pos.X = ((grab_x - new_x) << 8) + p_person->WorldPos.X;
        temp_pos.Y = ((grab_y - new_y) << 8) + p_person->WorldPos.Y;
        temp_pos.Z = ((grab_z - new_z) << 8) + p_person->WorldPos.Z;

        move_thing_on_map(p_person, &temp_pos);
    }
    return (1);
}

// Starts a traverse (sideways ledge move) in the given direction if possible.
// right=1 for traverse right, right=0 for traverse left.
// uc_orig: set_person_traverse (fallen/Source/Person.cpp)
void set_person_traverse(Thing* p_person, SLONG right)
{
    if (traverse_pos(p_person, right) == 0)
        return;

    set_generic_person_state_function(p_person, STATE_DANGLING);
    if (right) {
        p_person->SubState = SUB_STATE_TRAVERSE_RIGHT;
        p_person->Genus.Person->Action = ACTION_TRAVERSE_RIGHT;
        set_locked_anim(p_person, ANIM_TRAVERSE_RIGHT, 0);
    } else {
        p_person->SubState = SUB_STATE_TRAVERSE_LEFT;
        p_person->Genus.Person->Action = ACTION_TRAVERSE_LEFT;
        set_locked_anim(p_person, ANIM_TRAVERSE_LEFT, 0);
    }
    p_person->Draw.Tweened->Locked = 0;
}

// Initiates a pull-up from a dangling position. Checks that the destination
// square is not out of bounds or in a nogo zone before proceeding.
// uc_orig: set_person_pulling_up (fallen/Source/Person.cpp)
void set_person_pulling_up(Thing* p_person)
{
    {
        SLONG dx = -SIN(p_person->Draw.Tweened->Angle) >> 2;
        SLONG dz = -COS(p_person->Draw.Tweened->Angle) >> 2;

        SLONG mx = p_person->WorldPos.X + dx >> 16;
        SLONG mz = p_person->WorldPos.Z + dz >> 16;

        if (!WITHIN(mx, 0, PAP_SIZE_HI - 1) || !WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
            return;
        }

        if (PAP_2HI(mx, mz).Flags & PAP_FLAG_NOGO) {
            return;
        }
    }

    set_generic_person_state_function(p_person, STATE_DANGLING);
    p_person->SubState = SUB_STATE_PULL_UP;
    set_locked_anim(p_person, ANIM_PULL_UP_NEW, 0);
    p_person->Genus.Person->Action = ACTION_PULL_UP;
    p_person->Draw.Tweened->Locked = 0;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;
}

// Transitions person into a falling/dropping state (STATE_DANGLING + SUB_STATE_DROP_DOWN).
// flag bits: PERSON_DROP_DOWN_KEEP_VEL = don't reset horizontal speed,
//            PERSON_DROP_DOWN_KEEP_DY  = don't reset vertical speed,
//            PERSON_DROP_DOWN_QUEUED   = use queued (smooth) falling anim,
//            PERSON_DROP_DOWN_OFF_FACE = use off-face backwards substate.
// Also clears zipwire state and slide sound.
// uc_orig: set_person_drop_down (fallen/Source/Person.cpp)
void set_person_drop_down(Thing* p_person, SLONG flag)
{
    SLONG dv = -2;
    SLONG shotgun = 0;

    SlideSoundCheck(p_person, 1);

    if (person_holding_2handed(p_person))
        shotgun = 1;
    MSG_add(" set person drop");
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_ON_CABLE;
    MFX_stop(THING_NUMBER(p_person), S_ZIPWIRE);
    if (p_person->State == STATE_DANGLING
     && (p_person->SubState == SUB_STATE_DANGLING_CABLE
      || p_person->SubState == SUB_STATE_DANGLING_CABLE_FORWARD
      || p_person->SubState == SUB_STATE_DANGLING_CABLE_BACKWARD)) {
        if (p_person->OnFace) {
            if (p_person->Genus.Person->Flags & FLAG_PERSON_ON_CABLE) {
                dv = 0;
            }
        }
    }
    if (p_person->SubState == SUB_STATE_SLIPPING || p_person->SubState == SUB_STATE_SLIPPING_END) {
        p_person->Genus.Person->Flags |= FLAG_PERSON_MOVE_ANGLETO;
    } else {
        p_person->Genus.Person->Flags &= ~FLAG_PERSON_MOVE_ANGLETO;
    }

    set_generic_person_state_function(p_person, STATE_DANGLING);

    if (flag & PERSON_DROP_DOWN_OFF_FACE) {
        p_person->SubState = SUB_STATE_DROP_DOWN_OFF_FACE;
    } else {
        p_person->SubState = SUB_STATE_DROP_DOWN;
    }

    if (!(flag & PERSON_DROP_DOWN_QUEUED)) {
        if (shotgun)
            locked_anim_change(p_person, 0, ANIM_FALLING_AK, 0);
        else
            locked_anim_change(p_person, 0, ANIM_FALLING, 0);
    } else {
        if (shotgun)
            queue_anim(p_person, ANIM_FALLING_QUEUED_AK);
        else
            queue_anim(p_person, ANIM_FALLING_QUEUED);
    }
    p_person->Genus.Person->Action = ACTION_DROP_DOWN;
    p_person->Draw.Tweened->AnimTween = 0;
    p_person->Draw.Tweened->Locked = 0;

    if (!(flag & PERSON_DROP_DOWN_KEEP_VEL)) {
        p_person->Velocity = -8;
    } else {
        if (abs(p_person->Velocity) < 8) {
            if (p_person->Velocity < 0)
                p_person->Velocity = -8;
            else
                p_person->Velocity = 8;
        }
    }

    p_person->DeltaVelocity = 0;

    if (!(flag & PERSON_DROP_DOWN_KEEP_DY)) {
        p_person->DY = -(4 << 8);
    }

    p_person->OnFace = 0;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
}

// Like set_person_drop_down but uses a locked anim transition and sets DY from vely parameter.
// uc_orig: set_person_locked_drop_down (fallen/Source/Person.cpp)
void set_person_locked_drop_down(Thing* p_person, SLONG vely)
{
    MSG_add(" set person drop locked");
    set_generic_person_state_function(p_person, STATE_DANGLING);
    p_person->SubState = SUB_STATE_DROP_DOWN;
    locked_anim_change(p_person, 0, ANIM_FALLING, 0);
    p_person->Genus.Person->Action = ACTION_DROP_DOWN;
    p_person->Draw.Tweened->Locked = 0;
    p_person->Velocity = 0;
    p_person->DeltaVelocity = 0;
    p_person->DY = (vely << 8);
    p_person->OnFace = 0;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M;
}

// Nearest-point extern needed for wall-bump check below.
// uc_orig: nearest_point_on_line_and_dist (fallen/Headers/collide.h)
extern SLONG nearest_point_on_line_and_dist(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_z);

// Returns true if the wall described by col facet is suitable for a bump-and-turn:
// checks that the square ahead is well below the current height (i.e. not a ceiling).
// uc_orig: is_wall_good_for_bump_and_turn (fallen/Source/Person.cpp)
SLONG is_wall_good_for_bump_and_turn(Thing* p_person, SLONG col)
{
    SLONG dist, angle, wy, wx, wz;
    SLONG mx, my, mz, dx, dz;

    angle = p_person->Draw.Tweened->Angle;

    wx = p_person->WorldPos.X >> 8;
    wy = p_person->WorldPos.Y >> 8;
    wz = p_person->WorldPos.Z >> 8;

    dx = -(SIN(angle)) >> 9;
    dz = -(COS(angle)) >> 9;

    mx = (wx + dx) >> 8;
    mz = (wz + dz) >> 8;

    my = MAVHEIGHT(mx, mz) << 6;

    if (wy > my - 196)
        return (0);

    return (1);
}

// Returns true if person is facing a wall described by col facet within vault_da angle tolerance.
// Sets *wall_angle to the wall's normal angle. For fence facets also checks which side.
// vault_da default: 128 (approximately +/-22.5 degrees).
// uc_orig: am_i_facing_wall (fallen/Source/Person.cpp)
SLONG am_i_facing_wall(Thing* p_person, SLONG col, SLONG* wall_angle, SLONG vault_da)
{
    SLONG mdx, mdz, dx, dz, len, dist;
    SLONG near_x, near_z;
    SLONG wx, wy, wz;
    SLONG angle;
    GameCoord new_position;
    SLONG on, norm_x, norm_z;
    struct DFacet* p_facet;
    SLONG req_dist = 50;
    SLONG side;

    p_facet = &dfacets[col];

    dx = p_facet->x[1] - p_facet->x[0] << 8;
    dz = p_facet->z[1] - p_facet->z[0] << 8;

    mdx = abs(dx);
    mdz = abs(dz);

    {
        SLONG da;
        angle = Arctan(-dx, dz) - 512;
        if (angle < 0)
            angle = 2048 + angle;
        angle = angle & 2047;
        *wall_angle = angle;

        da = abs(angle - p_person->Draw.Tweened->Angle);

        if (p_facet->FacetType == STOREY_TYPE_FENCE || p_facet->FacetType == STOREY_TYPE_FENCE_FLAT) {
            extern SLONG which_side(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b);

            if (which_side(p_facet->x[0] << 8, p_facet->z[0] << 8, p_facet->x[1] << 8, p_facet->z[1] << 8, p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8) < 0) {
                da += 1024;
            }
        }
        if ((da < vault_da && da > -vault_da) || (da > 2048 - vault_da && da < 2048 + vault_da))
            return (1);
    }
    return (0);
}

// Returns true if person is approximately along the middle (not off the end) of the fence facet.
// uc_orig: along_middle_of_facet (fallen/Source/Person.cpp)
SLONG along_middle_of_facet(Thing* p_person, SLONG col)
{
    SLONG wx, wy, wz;
    SLONG on, norm_x, norm_z, dist;
    struct DFacet* p_facet;

    p_facet = &dfacets[col];

    wx = p_person->WorldPos.X >> 8;
    wy = p_person->WorldPos.Y >> 8;
    wz = p_person->WorldPos.Z >> 8;

    signed_dist_to_line_with_normal(
        p_facet->x[0] << 8, p_facet->z[0] << 8,
        p_facet->x[1] << 8, p_facet->z[1] << 8,
        wx, wz,
        &dist,
        &norm_x,
        &norm_z,
        &on);

    if (on) {
        return (1);
    } else {
        return (0);
    }
}

// Positions person facing a fence vault: checks fence Y range, aligns angle, moves person to
// correct stand-off distance (req_dist=50), and ensures the jump destination is not a nogo zone.
// Returns 1 on success, 0 if vault not possible from this position.
// uc_orig: set_person_pos_for_fence_vault (fallen/Source/Person.cpp)
SLONG set_person_pos_for_fence_vault(Thing* p_person, SLONG col)
{
    SLONG mdx, mdz, dx, dz, len, dist;
    SLONG near_x, near_z;
    SLONG wx, wy, wz;
    SLONG angle;
    GameCoord new_position;
    SLONG on, norm_x, norm_z;
    struct DFacet* p_facet;
    SLONG req_dist = 50;
    SLONG side;
    SLONG bot;
    SLONG top;

    // Check that person is in the correct Y range for the fence.
    bot = get_fence_bottom(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, col);
    top = get_fence_top(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, col);

    if (!WITHIN(p_person->WorldPos.Y >> 8, bot - 30, top)) {
        return FALSE;
    }

    p_facet = &dfacets[col];

    dx = p_facet->x[1] - p_facet->x[0] << 8;
    dz = p_facet->z[1] - p_facet->z[0] << 8;

    mdx = abs(dx);
    mdz = abs(dz);

#undef VAULT_DA
#define VAULT_DA (p_person->Genus.Person->PlayerID ? 128 : 256)

    {
        SLONG da;
        angle = Arctan(-mdx, mdz) + 1024 + 512;
        if (angle < 0)
            angle = 2048 + angle;
        angle = angle & 2047;

        da = abs(angle - p_person->Draw.Tweened->Angle);

        if ((da > VAULT_DA && da < 1024 - VAULT_DA) || (da > 1024 + VAULT_DA && da < 2048 - VAULT_DA))
            return (0);

        if (da > 512 && da < 2048 - 512) {
            angle += 1024;
            angle &= 2047;
        }
    }

    wx = p_person->WorldPos.X >> 8;
    wy = p_person->WorldPos.Y >> 8;
    wz = p_person->WorldPos.Z >> 8;

    signed_dist_to_line_with_normal(
        p_facet->x[0] << 8, p_facet->z[0] << 8,
        p_facet->x[1] << 8, p_facet->z[1] << 8,
        wx, wz,
        &dist,
        &norm_x,
        &norm_z,
        &on);

    if (!on)
        return (0);

    // Check person is facing the fence and not looking away from it.
    SLONG ldx = -SIN(angle) >> 8;
    SLONG ldz = -COS(angle) >> 8;

    if (!two4_line_intersection(
            p_facet->x[0] << 8, p_facet->z[0] << 8,
            p_facet->x[1] << 8, p_facet->z[1] << 8,
            wx, wz,
            wx + ldx, wz + ldz)) {
        return 0;
    }

    // Abort if the landing square is a nogo zone.
    if (PAP_2HI(wx + ldx >> 8, wz + ldz >> 8).Flags & PAP_FLAG_NOGO) {
        p_person->Velocity = 0;
        return 0;
    }

    // Abort if vaulting into a building (landing height too high).
    {
        SLONG flx = wx + ldx + (-ldz >> 3) >> 8;
        SLONG fly = wz + ldz + (+ldx >> 3) >> 8;

        SLONG frx = wx + ldx - (-ldz >> 3) >> 8;
        SLONG fry = wz + ldz - (+ldx >> 3) >> 8;

        SLONG fl_height = MAVHEIGHT(flx, fly) << 6;
        SLONG fr_height = MAVHEIGHT(frx, fry) << 6;

        if (fl_height - (p_person->WorldPos.Y >> 8) > 0x50
         || fr_height - (p_person->WorldPos.Y >> 8) > 0x50) {
            return 0;
        }
    }

    p_person->Draw.Tweened->Angle = angle;

    // Move person to the correct stand-off distance from the fence.
    {
        SLONG len;
        SLONG adx, adz;
        SLONG odx, odz;
        if (dist < 0) {
            norm_x = -norm_x;
            norm_z = -norm_z;
        }
        dist = abs(dist);

        adx = abs(norm_x);
        adz = abs(norm_z);

        len = QDIST2(adx, adz);
        if (len == 0)
            len = 1;

        mdx = ((norm_x * (req_dist - dist)) << 8) / len;
        mdz = ((norm_z * (req_dist - dist)) << 8) / len;

        new_position.X = p_person->WorldPos.X + (mdx);
        new_position.Z = p_person->WorldPos.Z + (mdz);
        new_position.Y = p_person->WorldPos.Y;
        move_thing_on_map(p_person, &new_position);
    }

    return (1);
}

// --- chunk 7: set_person_pos_for_fence..find_best_cable_angle ---

// Positions person relative to a fence facet at the specified standoff distance.
// Checks facing, nogo zone in front, and hand height vs fence bounds.
// Returns 0 on success (person repositioned), 1 if not possible.
// uc_orig: set_person_pos_for_fence (fallen/Source/Person.cpp)
SLONG set_person_pos_for_fence(Thing* p_person, SLONG col, SLONG set_pos, SLONG req_dist)
{
    SLONG mdx, mdz, dx, dz, len, dist;
    SLONG near_x, near_z;
    SLONG wx, wy, wz;
    SLONG angle;
    GameCoord new_position;
    SLONG on, norm_x, norm_z;
    struct DFacet* p_facet;

    p_facet = &dfacets[col];

    dx = p_facet->x[1] - p_facet->x[0] << 8;
    dz = p_facet->z[1] - p_facet->z[0] << 8;

    mdx = abs(dx);
    mdz = abs(dz);

    {
        angle = Arctan(-mdx, mdz) + 1024 + 512;
        if (angle < 0)
            angle = 2048 + angle;
        angle = angle & 2047;

        SLONG da = angle - p_person->Draw.Tweened->Angle;

        if ((abs(da) > 112 && abs(da) < 1024 - 112) || (abs(da) > 1024 + 112 && abs(da) < 2048 - 112))
            return (1);

        if (abs(da) > 512 && abs(da) < 2048 - 512) {
            angle += 1024;
            angle &= 2047;
        }

        MSG_add(" SET LAND ON FENCE angle %d person angle %d  \n", angle, p_person->Draw.Tweened->Angle);
    }

    // Check that there is no nogo zone in front of the person.
    {
        SLONG dax = -SIN(angle);
        SLONG daz = -COS(angle);

        SLONG mx = p_person->WorldPos.X + dax >> 16;
        SLONG mz = p_person->WorldPos.Z + daz >> 16;

        if (!WITHIN(mx, 0, PAP_SIZE_HI - 1) || !WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
            return 1;
        }

        if (PAP_2HI(mx, mz).Flags & PAP_FLAG_NOGO) {
            return 1;
        }
    }

    // Check if person can actually grab the fence (their hands must be in range).
    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, 0, &wx, &wy, &wz);

    wx += p_person->WorldPos.X >> 8;
    wy += p_person->WorldPos.Y >> 8;
    wz += p_person->WorldPos.Z >> 8;

    {
        SLONG fence_top = get_fence_top(wx, wz, col);
        SLONG fence_bottom = get_fence_bottom(wx, wz, col);

        if (WITHIN(wy, fence_bottom + 64, fence_top - 64)) {
            // Okay to grab.
        } else {
            // Wrong y-position to grab fence.
            return 1;
        }
    }

    // Check the person is positioned along the middle of the fence facet.
    signed_dist_to_line_with_normal(
        p_facet->x[0] << 8, p_facet->z[0] << 8,
        p_facet->x[1] << 8, p_facet->z[1] << 8,
        wx, wz,
        &dist,
        &norm_x,
        &norm_z,
        &on);

    if (!on)
        return (1);

    // Commit angle and position.
    p_person->Draw.Tweened->Angle = angle;

    {
        SLONG len;
        SLONG adx, adz;
        if (dist < 0) {
            norm_x = -norm_x;
            norm_z = -norm_z;
        }
        dist = abs(dist);

        adx = abs(norm_x);
        adz = abs(norm_z);

        len = QDIST2(adx, adz);
        if (len == 0)
            len = 1;

        mdx = ((norm_x * (req_dist - dist)) << 8) / len;
        mdz = ((norm_z * (req_dist - dist)) << 8) / len;

        new_position.X = p_person->WorldPos.X + (mdx);
        new_position.Z = p_person->WorldPos.Z + (mdz);

        new_position.Y = p_person->WorldPos.Y;
        move_thing_on_map(p_person, &new_position);
    }

    return (0);
}

// Positions person against a short wall step (half-step facet) using the signed
// distance to that facet's line. Returns 1 if repositioning succeeded, 0 otherwise.
// uc_orig: set_person_pos_for_half_step (fallen/Source/Person.cpp)
SLONG set_person_pos_for_half_step(Thing* p_person, SLONG col)
{
    SLONG dx, dz;
    SLONG wall_angle;
    struct DFacet* p_facet;
    p_facet = &dfacets[col];
    SLONG on, norm_x, norm_z;

    SLONG wx, wy, wz;

    if (am_i_facing_wall(p_person, col, &wall_angle)) {
        SLONG dist, angle, wy;
        SLONG mx, my, mz;

        wx = p_person->WorldPos.X >> 8;
        wy = p_person->WorldPos.Y >> 8;
        wz = p_person->WorldPos.Z >> 8;

        dx = -(SIN(wall_angle)) >> 9;
        dz = -(COS(wall_angle)) >> 9;

        mx = (wx + dx) >> 8;
        mz = (wz + dz) >> 8;

        if (p_person->Genus.Person->Ware) {
            // Would this take Darci out of her warehouse?
            if (!WARE_in_floorplan(p_person->Genus.Person->Ware, mx, mz)) {
                return 0;
            }

            my = WARE_calc_height_at(p_person->Genus.Person->Ware, (mx << 8) + 0x80, (mz << 8) + 0x80);
        } else {
            my = MAVHEIGHT(mx, mz) << 6;
        }

        if (wy < my - 196 || wy > my - 64)
            return (0);

        signed_dist_to_line_with_normal(
            p_facet->x[0] << 8, p_facet->z[0] << 8,
            p_facet->x[1] << 8, p_facet->z[1] << 8,
            wx, wz,
            &dist,
            &norm_x,
            &norm_z,
            &on);

        if (!on)
            return (0);

        {
            SLONG ax1;
            SLONG az1;
            SLONG ax2;
            SLONG az2;

            if (p_facet->x[0] == p_facet->x[1]) {
                ax1 = p_facet->x[0] << 8;
                ax2 = p_facet->x[0] << 8;

                az1 = wz & ~0xff;
                az2 = az1 + 256;
            } else {
                ax1 = wx & ~0xff;
                ax2 = ax1 + 256;

                az1 = p_facet->z[0] << 8;
                az2 = p_facet->z[0] << 8;
            }

            // If there is a fence lying along this facet, can't step up through it.
            if (does_fence_lie_along_line(
                    ax1, az1,
                    ax2, az2)) {
                return FALSE;
            }
        }

        if (abs(dist) < 60) {
            dist = abs(dist);
            p_person->Draw.Tweened->Angle = wall_angle;
            angle = (p_person->Draw.Tweened->Angle) & 2047;

            dx = (SIN(angle) * (40 - dist)) >> 8;
            dz = (COS(angle) * (40 - dist)) >> 8;
            person_normal_move_dxdz(p_person, dx, dz);
            return (1);
        }
    }
    return (0);
}

// Returns true if the given facet is a short (height==2, blockheight==16) vaultable fence.
// Original was declared inline; made non-inline so it can be forward-declared in old/Person.cpp.
// uc_orig: is_facet_vaultable (fallen/Source/Person.cpp)
SLONG is_facet_vaultable(SLONG facet)
{
    struct DFacet* p_facet;

    p_facet = &dfacets[facet];
    if (p_facet->FacetType == STOREY_TYPE_FENCE || p_facet->FacetType == STOREY_TYPE_FENCE_FLAT)
        if (p_facet->Height == 2)
            if (p_facet->BlockHeight == 16) {
                return (1);
            }

    return (0);
}

// Returns true if the given facet is a short normal-type step (height*blockheight*4 == 128).
// Original was declared inline; made non-inline so it can be forward-declared in old/Person.cpp.
// uc_orig: is_facet_half_step (fallen/Source/Person.cpp)
SLONG is_facet_half_step(SLONG facet)
{
    struct DFacet* p_facet;

    p_facet = &dfacets[facet];

    if (p_facet->FacetType == STOREY_TYPE_NORMAL)
        if ((p_facet->Height * p_facet->BlockHeight << 2) == 128) {
            return (1);
        }

    return (0);
}

// Attempts to land the person onto a fence at facet col.
// If the fence is vaultable and while_walking==0, tries a vault first.
// Returns 1 on success (person attached to fence), 0 if not possible.
// uc_orig: set_person_land_on_fence (fallen/Source/Person.cpp)
SLONG set_person_land_on_fence(Thing* p_person, SLONG col, SLONG set_pos, SLONG while_walking)
{
    SLONG ret;
    SLONG dist = 45;

    SLONG wall_angle;

    if (!am_i_facing_wall(p_person, col, &wall_angle, 256)) {
        return (0);
    }

    if (p_person->Genus.Person->PersonType == PERSON_ROPER)
        dist = 60;

    MSG_add(" set person land on fence");

    if (while_walking == 0)
        if (is_facet_vaultable(col)) {
            if (set_person_vault(p_person, col)) {
                return 1;
            }
        }

    // Leave fight mode when grabbing a fence.
    p_person->Genus.Person->Mode = PERSON_MODE_RUN;

    if (ret = set_person_pos_for_fence(p_person, col, set_pos, dist)) {
        switch (ret) {
        case 1:
            // Hands above fence — can't grab. Original has unreachable code after return here.
            return (0);
            if (while_walking == 0)
                set_person_drop_down(p_person, PERSON_DROP_DOWN_KEEP_VEL | PERSON_DROP_DOWN_KEEP_DY);
            p_person->Velocity = 1;
            break;
        }
        return (1);
    }
    set_generic_person_state_function(p_person, STATE_CLIMBING);
    p_person->SubState = SUB_STATE_CLIMB_LANDING;
    queue_anim(p_person, ANIM_LAND_ON_FENCE);
    p_person->Genus.Person->Action = ACTION_CLIMBING;
    p_person->Draw.Tweened->Locked = 0;
    p_person->Velocity = 0;
    p_person->DeltaVelocity = -2;
    p_person->DY = 0;
    p_person->Genus.Person->OnFacet = col;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;
    return (1);
}

// Positions person for a wall kick (pressing against wall col at dist=100/110 for Roper).
// Returns 1 on success, 0 if not possible.
// uc_orig: set_person_kick_off_wall (fallen/Source/Person.cpp)
SLONG set_person_kick_off_wall(Thing* p_person, SLONG col, SLONG set_pos)
{
    SLONG dist = 100;
    MSG_add(" set person land on fence");
    if (p_person->Genus.Person->PersonType == PERSON_ROPER)
        dist = 110;

    if (set_person_pos_for_fence(p_person, col, set_pos, dist) == -1)
        return (0);
    set_generic_person_state_function(p_person, STATE_FIGHTING);
    p_person->SubState = SUB_STATE_WALL_KICK;
    queue_anim(p_person, ANIM_WALL_KICK);

    p_person->Genus.Person->Action = ACTION_FIGHT_KICK;
    p_person->Draw.Tweened->Locked = 0;
    p_person->Velocity = 0;
    p_person->DeltaVelocity = 0;
    p_person->DY = 0;
    return (1);
}

// If this person has a gang attacker, switches them to fight mode targeting that attacker.
// uc_orig: fight_any_gang_attacker (fallen/Source/Person.cpp)
SLONG fight_any_gang_attacker(Thing* p_person)
{
    if (p_person->SubState != SUB_STATE_IDLE_CROUTCH_ARREST)
        if (p_person->Genus.Person->GangAttack) {
            UWORD attacker;
            if (attacker = get_any_gang_member(p_person)) {
                ASSERT(TO_THING(attacker)->Class == CLASS_PERSON);
                ASSERT(TO_THING(attacker)->State != STATE_DEAD);
                p_person->Genus.Person->Target = attacker;
                if (p_person->Genus.Person->Mode != PERSON_MODE_FIGHT) {
                    person_enter_fight_mode(p_person);
                    set_anim(p_person, ANIM_FIGHT_READY);
                }
            }
        }

    return (0);
}

// Finds a knocked-out target within 0x280 units suitable for arrest.
// Scores candidates by distance/type; returns thing index or 0 if none.
// uc_orig: find_arrestee (fallen/Source/Person.cpp)
UWORD find_arrestee(Thing* p_person)
{
    SLONG i;
    SLONG dist;
    SLONG score;
    SLONG best_score;
    SLONG best_answer;

    best_score = 0;
    best_answer = NULL;

    extern THING_INDEX col_with[];

    SLONG col_with_upto = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        0x280,
        col_with,
        MAX_COL_WITH,
        1 << CLASS_PERSON);

    for (i = 0; i < col_with_upto; i++) {
        Thing* p_found = TO_THING(col_with[i]);

        if (p_found->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
            // You can't arrest invulnerable people, except FIGHT_TEST dummies.
            if (p_found->Genus.Person->pcom_ai != PCOM_AI_FIGHT_TEST) {
                continue;
            }
        }

        dist = dist_to_target_pelvis(p_person, p_found);
        score = 0;

        if (dist < 128) {
            if (is_person_ko_and_lay_down(p_found)) {
                score = 500 - dist;

                if (p_found->Genus.Person->PersonType == PERSON_MIB1 || p_found->Genus.Person->PersonType == PERSON_MIB2 || p_found->Genus.Person->PersonType == PERSON_MIB3) {
                    score = 0;
                }

                if (p_found->Genus.Person->PersonType == PERSON_COP) {
                    if (!(p_found->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY)) {
                        // Can't arrest innocent cops.
                        score = 0;
                    }
                }

                if (p_found->Genus.Person->PersonType == PERSON_ROPER || p_found->Genus.Person->PersonType == PERSON_HOSTAGE) {
                    score -= 250;
                }

                if (p_found->Genus.Person->pcom_ai == PCOM_AI_BODYGUARD && EWAY_get_person(p_found->Genus.Person->pcom_ai_other) == THING_NUMBER(p_person)) {
                    // Don't arrest your own bodyguards!
                    score = 0;
                }

            } else if (p_found->Genus.Person->pcom_ai == PCOM_AI_FIGHT_TEST) {
                if (p_found->State == STATE_DEAD && p_found->SubState == SUB_STATE_DEAD_INJURED) {
                    score = 500 - dist;
                }
            }
        }

        if (score > 0) {
            if (best_score < score) {
                best_score = score;
                best_answer = col_with[i];
            }
        }
    }

    return best_answer;
}

// Finds the nearest dead (STATE_DEAD, SUB_STATE_DEAD_INJURED) person within 256 units.
// uc_orig: find_corpse (fallen/Source/Person.cpp)
UWORD find_corpse(Thing* p_person)
{
    UWORD s_index;
    Thing* p_target;
    extern SLONG THING_find_nearest_person(Thing * p_person, SLONG radius, ULONG classes);
    s_index = THING_find_nearest_person(p_person, 256, 1 << CLASS_PERSON);

    if (s_index) {
        p_target = TO_THING(s_index);

        if (dist_to_target_pelvis(p_person, p_target) < 128 && p_target->State == STATE_DEAD && p_target->SubState == SUB_STATE_DEAD_INJURED)
        {
            return (s_index);
        }
    }

    return (0);
}

// Executes the arrest of s_index; displays the arrested HUD message.
// Returns 1 on success.
// uc_orig: perform_arrest (fallen/Source/Person.cpp)
UWORD perform_arrest(Thing* p_person, UWORD s_index)
{
    Thing* p_target;

    p_target = TO_THING(s_index);

    if (s_index) {
        PANEL_new_info_message(XLAT_str(X_ARRESTED));

        return (1);
    }
    return (0);
}

// Forward decl — defined in interfac.cpp, only referenced in a commented-out block below.
void lock_to_compass(Thing* p_thing);

// STATE_SEARCH state machine: handles prim search and corpse looting animations.
// uc_orig: fn_person_search (fallen/Source/Person.cpp)
void fn_person_search(Thing* p_person)
{
    UWORD last_timer;

    switch (p_person->SubState) {
    case SUB_STATE_SEARCH_PRIM:
    case SUB_STATE_SEARCH_CORPSE:

        person_normal_animate(p_person);

        if (!continue_pressing_action(p_person)) {
            MFX_stop(THING_NUMBER(p_person), S_SEARCH_END);

            if (p_person->SubState == SUB_STATE_SEARCH_CORPSE) {
                set_anim(p_person, ANIM_CROUTCH_GETUP);
                p_person->SubState = SUB_STATE_SEARCH_GETUP;
            } else {
                set_person_idle(p_person);
            }

            return;
        }

        last_timer = p_person->Genus.Person->Timer1;

        p_person->Genus.Person->Timer1 += 400 * TICK_RATIO >> TICK_SHIFT;

        if (p_person->Genus.Person->Timer1 >= (125 << 8)) // Stays at 100 for a while!
        {
            set_action_used(p_person);
            set_person_idle(p_person);

            return;
        }

        if (last_timer < (100 << 8) && p_person->Genus.Person->Timer1 >= (100 << 8)) {
            release_searched_item(p_person);

            if (p_person->SubState == SUB_STATE_SEARCH_CORPSE) {
                set_anim(p_person, ANIM_CROUTCH_GETUP);
                p_person->SubState = SUB_STATE_SEARCH_GETUP;
                p_person->Genus.Person->Target = 0;
            }
        }

        break;

    case SUB_STATE_SEARCH_GETUP:

    {
        SLONG end = person_normal_animate(p_person);

        if (end) {
            set_action_used(p_person);
            set_person_idle(p_person);
        }
    }

    break;
    }
}

// Picks a random idle animation for the person based on their type,
// and resets Timer1 to a random delay before the next idle.
// uc_orig: set_person_random_idle (fallen/Source/Person.cpp)
void set_person_random_idle(Thing* p_person)
{
    SLONG anim = Random();

    if (person_holding_2handed(p_person)) {
        queue_anim(p_person, anim);
    }

    p_person->Genus.Person->Timer1 = (Random() & 0xff) + 400;

    switch (p_person->Genus.Person->PersonType) {
    case PERSON_ROPER:
        queue_anim(p_person, anim & 1 ? ANIM_BREATHE : ANIM_IDLE_SCRATCH1);
        break;

    case PERSON_COP: {
        switch (anim & 3) {
        case 0:
            anim = COP_ROPER_ANIM_IDLE1;
            break;
        case 1:
            anim = COP_ROPER_ANIM_IDLE2;
            break;
        case 2:
            anim = COP_ROPER_ANIM_IDLE3;
            break;
        case 3:
            anim = COP_ROPER_ANIM_IDLE4;
            break;
        }
        p_person->Draw.Tweened->QueuedFrame = game_chunk[ANIM_TYPE_ROPER].AnimList[anim];
        p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
        p_person->Draw.Tweened->CurrentAnim = anim;
        p_person->Draw.Tweened->FrameIndex = 0;
    }

    break;

    default:
        switch (anim % 3) {
        case 0:
            anim = ANIM_IDLE_SCRATCH1;
            break;
        case 1:
            anim = ANIM_IDLE_SCRATCH2;
            break;
        case 2:
            anim = ANIM_BREATHE;
            break;
        }

        queue_anim(p_person, anim);
        break;
    }
}

// STATE_IDLE state machine — handles idle, crouch-arrest, slope-slip, and fight-mode idle logic.
// Also manages breath-cloud particles in snow world and the Roper's magic bottle item ID hack.
// uc_orig: fn_person_idle (fallen/Source/Person.cpp)
void fn_person_idle(Thing* p_person)
{
    SLONG end;

    if ((world_type == WORLD_TYPE_SNOW) && !PersonIsMIB(p_person)) {
        // Breath cloud puff from the head in snowy levels.
        if (MagicFrameCheck(p_person, 5)) {
            SLONG x, y, z;
            calc_sub_objects_position(
                p_person, p_person->Draw.Tweened->AnimTween,
                SUB_OBJECT_HEAD,
                &x, &y, &z);
            x <<= 8;
            y <<= 8;
            z <<= 8;
            x += p_person->WorldPos.X;
            y += p_person->WorldPos.Y;
            z += p_person->WorldPos.Z;
            PARTICLE_Add(x, y, z, 256, 10, 256, POLY_PAGE_SMOKECLOUD2,
                2 + ((Random() & 3) << 2), 0x7FFFFFFF, PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE,
                30, 30, 1, 10, 1);
        }
    }

    SlideSoundCheck(p_person);

    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        if (p_person->SubState != SUB_STATE_IDLE_CROUTCH_ARREST) {
            if (p_person->Genus.Person->Target)
                if (TO_THING(p_person->Genus.Person->Target)->State != STATE_DEAD)
                    turn_to_face_thing(p_person, TO_THING(p_person->Genus.Person->Target), 0);

            {
                Thing* p_attacker = is_person_under_attack_low_level(p_person, FALSE, 0x200);

                if (p_attacker == NULL) {
                    // Nobody to fight — exit fight mode.
                    {
                        p_person->Genus.Person->Agression = 0;
                        p_person->Genus.Person->Mode = PERSON_MODE_RUN;
                        p_person->Genus.Person->Target = 0;
                        set_person_idle(p_person);
                        if (p_person->State == STATE_IDLE)
                            set_anim(p_person, ANIM_UNFIGHT);
                    }
                } else {
                    if (p_person->Genus.Person->Target == NULL) {
                        p_person->Genus.Person->Target = THING_NUMBER(p_attacker);
                    }
                }
            }
        }

    } else {

        if (p_person->OnFace > 0) {
            ASSERT(WITHIN(p_person->OnFace, 1, next_prim_face4 - 1));

            PrimFace4* f4 = &prim_faces4[p_person->OnFace];

            ASSERT(f4->FaceFlags & FACE_FLAG_WALKABLE);

            // Surfing is disabled for now.
        }

        if (p_person->Genus.Person->PlayerID && p_person->Genus.Person->Mode != PERSON_MODE_FIGHT && !person_has_gun_out(p_person)) {
            if (p_person->SubState != SUB_STATE_IDLE_CROUTCH_ARREST) {
                Thing* p_attacker = is_person_under_attack_low_level(p_person, FALSE, 0x100);

                if (p_person->Genus.Person->pcom_colour) {
                    p_person->Genus.Person->pcom_colour--;

                } else if (p_attacker) {
                    SLONG anim;

                    ASSERT(p_attacker->Class == CLASS_PERSON);

                    p_person->Genus.Person->Target = THING_NUMBER(p_attacker);
                    person_enter_fight_mode(p_person);
                    anim = find_idle_fight_stance(p_person);
                    set_anim(p_person, anim);
                }
            }
        }
    }

    switch (p_person->SubState) {
    case 0:
        // Check if the person should slip down the slope they're idle on.
        {
            SLONG slope, angle;
            if (p_person->Genus.Person->InsideIndex) {
                slope = 0;
            } else {

                if (p_person->OnFace < 0) {
                    slope = RFACE_on_slope(-p_person->OnFace, p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle);
                } else {

                    slope = PAP_on_slope(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, &angle) >> 1;
                }

                if (slope > 50) {
                    set_anim(p_person, ANIM_FALLING);
                    set_generic_person_state_function(p_person, STATE_MOVEING);
                    p_person->SubState = SUB_STATE_SLIPPING;
                    p_person->Draw.Tweened->AngleTo = angle;
                    slope = MIN(slope - 50, 10);
                    slope = MAX(slope, 50);
                    change_velocity_to(p_person, slope);

                    return;
                }
            }
        }

        if (p_person->Draw.Tweened->DRoll && p_person->Draw.Tweened->FrameIndex == 2) {
            end = 0;
        } else {
            end = person_normal_animate(p_person);
        }
        p_person->Draw.Tweened->DRoll = 0;

        if (end && (p_person->Draw.Tweened->CurrentAnim != ANIM_IDLE_SCRATCH1)) {
            p_person->Draw.Tweened->CurrentAnim = ANIM_STAND_READY;
        }

        if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT) {
            if (end == 1) {
                SLONG anim;
                anim = find_idle_fight_stance(p_person);

                queue_anim(p_person, anim);
            }
        } else {
            if (p_person->Genus.Person->SpecialUse) {

            } else {

                if (p_person->Genus.Person->Timer1-- == 0) {
                    set_person_random_idle(p_person);
                }

                // Roper's magic bottle PersonID hack: show bottle in frames 3-16,
                // hide in frame 17+.  Original dev comment: "It says here that
                // 'Drinky Winky' of the teletubbies is obviously an abusive drunk
                // because of the bottle of booze he carries..."
                if ((p_person->Genus.Person->PersonType == PERSON_ROPER) && (p_person->Draw.Tweened->CurrentAnim == ANIM_IDLE_SCRATCH1)) {
                    if (p_person->Draw.Tweened->FrameIndex >= 3) {
                        if (p_person->Draw.Tweened->FrameIndex >= 17)
                            p_person->Draw.Tweened->PersonID &= ~0xe0;
                        else {
                            p_person->Draw.Tweened->PersonID &= ~0xe0;
                            p_person->Draw.Tweened->PersonID |= 7 << 5; // roper's magic bottle
                        }
                    }
                }
            }
        }

        break;

    case SUB_STATE_IDLE_CROUTCH:
        end = person_normal_animate(p_person);

        if (end == 1) {
            p_person->SubState = SUB_STATE_IDLE_CROUTCHING;
            p_person->Genus.Person->Flags &= ~FLAG_PERSON_NON_INT_M;
        }
        break;
    case SUB_STATE_IDLE_CROUTCH_ARREST:
        end = person_normal_animate(p_person);

        if (end == 1) {
            p_person->Genus.Person->Flags &= ~FLAG_PERSON_NON_INT_M;
            perform_arrest(p_person, p_person->Genus.Person->Target);

            set_person_idle(p_person);
        }
        break;
    case SUB_STATE_IDLE_CROUTCHING:
        break;

    case SUB_STATE_IDLE_UNCROUCH:

        end = person_normal_animate(p_person);

        if (p_person->Genus.Person->PersonType == PERSON_ROPER) {
            if (p_person->Draw.Tweened->FrameIndex > 1)
                end = 1;
        }

        if (end) {
            set_person_idle(p_person);
        }

        break;
    }
}

// Forward decls for AI functions in pcom.cpp (not yet migrated).
void PCOM_set_person_ai_flee_person(Thing* p_person, Thing* p_scary);
void PCOM_make_driver_run_away(Thing* p_driver, Thing* p_scary);

// Puts p_person in the driver's seat of p_car.
// Handles the case where the car already has a driver (player vs NPC priority).
// Plays engine start and idle sounds (play calls currently commented out in original).
// uc_orig: set_person_in_vehicle (fallen/Source/Person.cpp)
void set_person_in_vehicle(Thing* p_person, Thing* p_car)
{
    SLONG sample;

    if (p_car->Genus.Vehicle->Driver) {
        Thing* other_driver;

        // Both tried to get in at the same time.
        other_driver = TO_THING(p_car->Genus.Vehicle->Driver);

        if (other_driver->Genus.Person->PlayerID) {
            // Player already in the car — AI must run away.
            p_person->Genus.Person->InCar = 0;
            PCOM_set_person_ai_flee_person(p_person, other_driver);
        } else if (p_person->Genus.Person->PlayerID) {
            // Player taking a car with an AI driver — stop car and eject driver.
            p_car->Velocity = 0;
            PCOM_make_driver_run_away(other_driver, p_person);
        }
    }

    ASSERT(p_car->Class == CLASS_VEHICLE);
    p_person->Genus.Person->Flags |= FLAG_PERSON_DRIVING;
    p_person->Genus.Person->InCar = THING_NUMBER(p_car);
    p_person->SubState = SUB_STATE_INSIDE_VEHICLE;
    remove_thing_from_map(p_person);
    p_car->Genus.Vehicle->Flags |= FLAG_FURN_DRIVING;
    p_car->Genus.Vehicle->Flags &= ~FLAG_VEH_ANIMATING;
    p_car->Genus.Vehicle->Driver = THING_NUMBER(p_person);
    set_state_function(p_car, STATE_FDRIVING);

    switch (p_car->Genus.Vehicle->Type) {
    case VEH_TYPE_AMBULANCE:
    case VEH_TYPE_VAN:
    case VEH_TYPE_JEEP:
    case VEH_TYPE_MEATWAGON:
    case VEH_TYPE_WILDCATVAN:
        sample = S_VAN_START;
        break;

    case VEH_TYPE_CAR:
    case VEH_TYPE_TAXI:
    case VEH_TYPE_POLICE:
    case VEH_TYPE_SEDAN:
        sample = S_CAR_START;
        break;

    default:
        ASSERT(0);
    }

    switch (p_car->Genus.Vehicle->Type) {
    case VEH_TYPE_AMBULANCE:
    case VEH_TYPE_VAN:
    case VEH_TYPE_JEEP:
    case VEH_TYPE_MEATWAGON:
    case VEH_TYPE_WILDCATVAN:
        sample = S_VAN_IDLE;
        break;

    case VEH_TYPE_CAR:
    case VEH_TYPE_TAXI:
    case VEH_TYPE_POLICE:
    case VEH_TYPE_SEDAN:
        sample = S_CAR_IDLE;
        break;

    default:
        ASSERT(0);
    }
}

// Removes p_person from their current vehicle.
// uc_orig: set_person_out_of_vehicle (fallen/Source/Person.cpp)
void set_person_out_of_vehicle(Thing* p_person)
{
    Thing* p_car;

    ASSERT(p_person->Genus.Person->InCar);

    p_car = TO_THING(p_person->Genus.Person->InCar);

    ASSERT(p_car->Class == CLASS_VEHICLE);

    p_person->Genus.Person->Flags &= ~(FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER);
    p_person->Genus.Person->InCar = 0;
    set_person_locked_idle_ready(p_person);
    plant_feet(p_person);
    p_car->Genus.Vehicle->Flags &= ~FLAG_FURN_DRIVING;
}

// Changes to a fresh animation while keeping a specific limb locked in world-space.
// Computes the delta between the limb's old and new positions and moves the person accordingly.
// dangle: optional rotation offset applied before computing the new limb position.
// uc_orig: locked_anim_change (fallen/Source/Person.cpp)
void locked_anim_change(Thing* p_person, UWORD locked_object, UWORD anim, SLONG dangle)
{
    SLONG lock_x1, lock_y1, lock_z1;
    SLONG lock_x2, lock_y2, lock_z2;
    DrawTween* draw_info;
    GameCoord temp_pos;

    ASSERT(anim);

    ASSERT(global_anim_array[p_person->Genus.Person->AnimType][anim]);

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, locked_object, &lock_x1, &lock_y1, &lock_z1);

    draw_info = p_person->Draw.Tweened;
    draw_info->CurrentFrame = global_anim_array[p_person->Genus.Person->AnimType][anim];
    draw_info->QueuedFrame = 0;
    draw_info->NextFrame = draw_info->CurrentFrame->NextFrame;
    draw_info->AnimTween = 0;
    draw_info->CurrentAnim = anim;
    draw_info->FrameIndex = 0;

    draw_info->Angle += dangle;
    draw_info->Angle &= 2047;

    calc_sub_objects_position(p_person, 0, locked_object, &lock_x2, &lock_y2, &lock_z2);

    temp_pos.X = ((+lock_x1 - lock_x2) << 8) + p_person->WorldPos.X;
    temp_pos.Y = ((+lock_y1 - lock_y2) << 8) + p_person->WorldPos.Y;
    temp_pos.Z = ((+lock_z1 - lock_z2) << 8) + p_person->WorldPos.Z;

    move_thing_on_map(p_person, &temp_pos);
}

// Like locked_anim_change but plays an animation from a specific animation type
// (e.g. ANIM_TYPE_ROPER) rather than the person's own AnimType.
// uc_orig: locked_anim_change_of_type (fallen/Source/Person.cpp)
void locked_anim_change_of_type(Thing* p_person, UWORD locked_object, UWORD anim, SLONG type)
{
    SLONG lock_x1, lock_y1, lock_z1;
    SLONG lock_x2, lock_y2, lock_z2;
    DrawTween* draw_info;
    GameCoord temp_pos;

    ASSERT(anim);

    ASSERT(global_anim_array[type][anim]);

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, locked_object, &lock_x1, &lock_y1, &lock_z1);

    draw_info = p_person->Draw.Tweened;
    draw_info->CurrentFrame = game_chunk[type].AnimList[anim];
    draw_info->QueuedFrame = 0;
    draw_info->NextFrame = draw_info->CurrentFrame->NextFrame;
    draw_info->AnimTween = 0;
    draw_info->CurrentAnim = anim;

    calc_sub_objects_position(p_person, 0, locked_object, &lock_x2, &lock_y2, &lock_z2);

    temp_pos.X = ((+lock_x1 - lock_x2) << 8) + p_person->WorldPos.X;
    temp_pos.Y = ((+lock_y1 - lock_y2) << 8) + p_person->WorldPos.Y;
    temp_pos.Z = ((+lock_z1 - lock_z2) << 8) + p_person->WorldPos.Z;

    move_thing_on_map(p_person, &temp_pos);
}

// Like locked_anim_change_of_type but only adjusts Y (height), not X/Z.
// Used for ladder animation transitions where horizontal position stays fixed.
// uc_orig: locked_anim_change_height_type (fallen/Source/Person.cpp)
void locked_anim_change_height_type(Thing* p_person, UWORD locked_object, UWORD anim, SLONG type)
{
    SLONG lock_x1, lock_y1, lock_z1;
    SLONG lock_x2, lock_y2, lock_z2;
    DrawTween* draw_info;
    GameCoord temp_pos;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, locked_object, &lock_x1, &lock_y1, &lock_z1);

    draw_info = p_person->Draw.Tweened;
    draw_info->CurrentFrame = game_chunk[type].AnimList[anim];
    draw_info->QueuedFrame = 0;
    draw_info->NextFrame = draw_info->CurrentFrame->NextFrame;
    draw_info->AnimTween = 0;
    draw_info->CurrentAnim = anim;

    calc_sub_objects_position(p_person, 0, locked_object, &lock_x2, &lock_y2, &lock_z2);

    temp_pos.X = p_person->WorldPos.X;
    temp_pos.Y = ((+lock_y1 - lock_y2) << 8) + p_person->WorldPos.Y;
    temp_pos.Z = p_person->WorldPos.Z;

    move_thing_on_map(p_person, &temp_pos);
}

// Adjusts person's Y so that limb obj is at world-space height y.
// uc_orig: set_limb_to_y (fallen/Source/Person.cpp)
SLONG set_limb_to_y(Thing* p_person, SLONG obj, SLONG y)
{
    SLONG x1, y1, z1;
    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, obj, &x1, &y1, &z1);
    y1 += (p_person->WorldPos.Y >> 8);

    y -= y1;

    p_person->WorldPos.Y += y << 8;
    return (0);
}

// Switches to a queued frame while keeping the given locked limb in the same world position.
// uc_orig: locked_next_anim_change (fallen/Source/Person.cpp)
void locked_next_anim_change(Thing* p_person, UWORD locked_object, GameKeyFrame* queued_frame)
{
    SLONG lock_x1, lock_y1, lock_z1;
    SLONG lock_x2, lock_y2, lock_z2;
    DrawTween* draw_info;
    GameCoord temp_pos;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, locked_object, &lock_x1, &lock_y1, &lock_z1);

    draw_info = p_person->Draw.Tweened;
    draw_info->NextFrame = queued_frame;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, locked_object, &lock_x2, &lock_y2, &lock_z2);

    temp_pos.X = ((+lock_x1 - lock_x2) << 8) + p_person->WorldPos.X;
    temp_pos.Y = ((+lock_y1 - lock_y2) << 8) + p_person->WorldPos.Y;
    temp_pos.Z = ((+lock_z1 - lock_z2) << 8) + p_person->WorldPos.Z;

    move_thing_on_map(p_person, &temp_pos);
}

// Like locked_anim_change_of_type but jumps to the last frame of the animation.
// Used for smooth reverse-climb transitions on ladders.
// uc_orig: locked_anim_change_end_type (fallen/Source/Person.cpp)
void locked_anim_change_end_type(Thing* p_person, UWORD locked_object, UWORD anim, SLONG type)
{
    SLONG lock_x1, lock_y1, lock_z1;
    SLONG lock_x2, lock_y2, lock_z2;
    DrawTween* draw_info;
    GameCoord temp_pos;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, locked_object, &lock_x1, &lock_y1, &lock_z1);

    draw_info = p_person->Draw.Tweened;
    draw_info->CurrentFrame = game_chunk[type].AnimList[anim];
    draw_info->QueuedFrame = 0;
    draw_info->NextFrame = draw_info->CurrentFrame->NextFrame;

    while (draw_info->NextFrame->NextFrame) {
        draw_info->CurrentFrame = draw_info->NextFrame;
        draw_info->NextFrame = draw_info->NextFrame->NextFrame;
    }
    draw_info->AnimTween = 255;
    draw_info->CurrentAnim = anim;

    calc_sub_objects_position(p_person, draw_info->AnimTween, locked_object, &lock_x2, &lock_y2, &lock_z2);

    temp_pos.X = ((+lock_x1 - lock_x2) << 8) + p_person->WorldPos.X;
    temp_pos.Y = ((+lock_y1 - lock_y2) << 8) + p_person->WorldPos.Y;
    temp_pos.Z = ((+lock_z1 - lock_z2) << 8) + p_person->WorldPos.Z;

    move_thing_on_map(p_person, &temp_pos);
}

// Like locked_anim_change but jumps to the last frame using the person's own AnimType.
// uc_orig: locked_anim_change_end (fallen/Source/Person.cpp)
void locked_anim_change_end(Thing* p_person, UWORD locked_object, UWORD anim)
{
    SLONG lock_x1, lock_y1, lock_z1;
    SLONG lock_x2, lock_y2, lock_z2;
    DrawTween* draw_info;
    GameCoord temp_pos;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, locked_object, &lock_x1, &lock_y1, &lock_z1);

    draw_info = p_person->Draw.Tweened;
    draw_info->CurrentFrame = global_anim_array[p_person->Genus.Person->AnimType][anim];
    draw_info->QueuedFrame = 0;
    draw_info->NextFrame = draw_info->CurrentFrame->NextFrame;

    while (draw_info->NextFrame->NextFrame) {
        draw_info->CurrentFrame = draw_info->NextFrame;
        draw_info->NextFrame = draw_info->NextFrame->NextFrame;
    }
    draw_info->AnimTween = 255;
    draw_info->CurrentAnim = anim;

    calc_sub_objects_position(p_person, draw_info->AnimTween, locked_object, &lock_x2, &lock_y2, &lock_z2);

    temp_pos.X = ((+lock_x1 - lock_x2) << 8) + p_person->WorldPos.X;
    temp_pos.Y = ((+lock_y1 - lock_y2) << 8) + p_person->WorldPos.Y;
    temp_pos.Z = ((+lock_z1 - lock_z2) << 8) + p_person->WorldPos.Z;

    move_thing_on_map(p_person, &temp_pos);
}

// Returns the slope (dy/horiz_length * 256) of a cable facet.
// Used to decide whether a cable is steep enough for a death-slide.
// uc_orig: steep_cable (fallen/Source/Person.cpp)
SLONG steep_cable(SLONG facet)
{
    struct DFacet* p_facet;
    SLONG dx, dy, dz, len, m;
    p_facet = &dfacets[facet];

    dx = abs(p_facet->x[1] - p_facet->x[0] << 8);
    dz = abs(p_facet->z[1] - p_facet->z[0] << 8);

    len = QDIST2(dx, dz);

    dy = p_facet->Y[1] - p_facet->Y[0];

    if (len == 0)
        len = 1;

    m = (dy << 8) / len;

    return (m);
}

// Sets the person's angle to face downhill along a cable facet.
// uc_orig: face_down_cable (fallen/Source/Person.cpp)
void face_down_cable(Thing* p_person, SLONG facet)
{
    struct DFacet* p_facet;
    SLONG dx, dy, dz, len, m;
    p_facet = &dfacets[facet];

    dx = (p_facet->x[1] - p_facet->x[0] << 8);
    dy = (p_facet->Y[1] - p_facet->Y[0]);
    dz = (p_facet->z[1] - p_facet->z[0] << 8);

    if (dy > 0) {
        p_person->Draw.Tweened->Angle = calc_angle(dx, dz);
    } else {
        p_person->Draw.Tweened->Angle = calc_angle(-dx, -dz);
    }
}

// Returns the cable angle (0-2047) that is closest to the person's current facing.
// Flips by 1024 if the person is facing the opposite end of the cable.
// uc_orig: find_best_cable_angle (fallen/Source/Person.cpp)
SLONG find_best_cable_angle(Thing* p_person, SLONG facet)
{
    struct DFacet* p_facet;
    SLONG dx, dy, dz, len, m;
    SLONG dangle, cable_angle;

    p_facet = &dfacets[facet];

    dx = (p_facet->x[1] - p_facet->x[0]) << 8;
    dz = (p_facet->z[1] - p_facet->z[0]) << 8;

    cable_angle = calc_angle(dx, dz);

    dangle = p_person->Draw.Tweened->Angle - cable_angle;

    if (abs(dangle) > 1024)
        return ((cable_angle + 1024) & 2047);
    else
        return (cable_angle);
}

// =============================================================================
// chunk 8: grab_ledge..do_person_on_cable (original lines ~10702-12291)
// =============================================================================

// uc_orig: grab_ledge (fallen/Source/Person.cpp)
SLONG grab_ledge(Thing* p_person)
{
    SLONG x, y, z;
    SLONG grab_x, grab_y, grab_z, type;
    SLONG grab_angle;
    SLONG face;

    SLONG ignore_building;

    if (p_person->Genus.Person->InsideIndex)
        return (0);

    // While grabbing a ledge, also try grabbing a balloon.
    BALLOON_find_grab(THING_NUMBER(p_person));

    if (p_person->Flags & FLAGS_IN_BUILDING) {
        ignore_building = INDOORS_DBUILDING;
    } else {
        ignore_building = 0;
    }

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_HAND, &x, &y, &z);

    {
        SLONG rx, ry, rz;

        calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_RIGHT_HAND, &rx, &ry, &rz);

        x += rx;
        y += ry;
        z += rz;

        x >>= 1;
        y >>= 1;
        z >>= 1;
    }
    x += p_person->WorldPos.X >> 8;
    y += p_person->WorldPos.Y >> 8;
    z += p_person->WorldPos.Z >> 8;

    // In sewers, grab the sewer floor instead of a normal face.
    if (p_person->Flags & FLAGS_IN_SEWERS) {
        face = find_grab_face_in_sewers(
            x, y, z,
            50, 40,
            p_person->Draw.Tweened->Angle,
            &grab_x,
            &grab_y,
            &grab_z,
            &grab_angle);
    } else {
        SLONG radius = 80;
        if (p_person->State == STATE_CLIMBING)
            radius = 100;
        face = find_grab_face(
            x, y, z,
            radius, 40,
            p_person->Draw.Tweened->Angle,
            &grab_x,
            &grab_y,
            &grab_z,
            &grab_angle,
            ignore_building,
            FALSE, &type,
            p_person);
    }

    if (face) {
        // Leave fight mode when you grab something.
        p_person->Genus.Person->Mode = PERSON_MODE_RUN;

        SLONG new_x, new_y, new_z, old_substate;
        GameCoord temp_pos;
        DrawTween* draw_info;

        if (type == 2) {
            if (p_person->SubState != SUB_STATE_RUNNING_JUMP_FLY && p_person->SubState != SUB_STATE_DROP_DOWN)
                return (0);
            if (p_person->DY > 0)
                return (0);

            // grab_ladder: position person at the ladder XZ and switch to STATE_CLIMB_LADDER.
            temp_pos.X = grab_x << 8;
            temp_pos.Y = p_person->WorldPos.Y;
            temp_pos.Z = grab_z << 8;

            move_thing_on_map(p_person, &temp_pos);
            p_person->Genus.Person->OnFacet = face;
            p_person->Draw.Tweened->Angle = grab_angle;
            set_generic_person_state_function(p_person, STATE_CLIMB_LADDER);

            set_person_on_ladder(p_person);
            return (1);
        }

        draw_info = p_person->Draw.Tweened;

        draw_info->AnimTween = 0;
        p_person->Genus.Person->Action = ACTION_GRABBING_LEDGE;
        set_generic_person_state_function(p_person, STATE_DANGLING);
        old_substate = p_person->SubState;
        p_person->SubState = SUB_STATE_GRAB_TO_DANGLE;

        if (type == 1) {
            SLONG grad;
            // Grabbed a cable — enter death-slide if steep enough.
            p_person->Genus.Person->Flags |= FLAG_PERSON_ON_CABLE;
            set_anim(p_person, ANIM_DEATH_SLIDE);

            MinorEffortSound(p_person);

            grad = steep_cable(face);
            if (abs(grad) > 10) {
                p_person->SubState = SUB_STATE_DEATH_SLIDE;
                p_person->Velocity = 0;
                grad >>= 6;
                if (grad > 16)
                    grad = 16;
                if (grad < 2)
                    grad = 2;

                p_person->DeltaVelocity = abs(grad);
                face_down_cable(p_person, face);
                p_person->Genus.Person->Action = ACTION_DEATH_SLIDE;
                p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
            }
        } else {
            // Grabbed a ledge — play the appropriate grab sound based on previous sub-state.
            switch (old_substate) {
            case SUB_STATE_STANDING_JUMP:
                MinorEffortSound(p_person);
                break;
            case SUB_STATE_RUNNING_JUMP:
                MinorEffortSound(p_person);
                break;
            case SUB_STATE_DROP_DOWN:
                StopScreamFallSound(p_person);
                if (p_person->Genus.Person->PersonType == PERSON_ROPER)
                    MFX_play_thing(THING_NUMBER(p_person), S_ROPER_HIT_END - 1, 0, p_person);
                else
                    MinorEffortSound(p_person);
                break;
            case SUB_STATE_RUNNING_JUMP_FLY:
                switch (p_person->Genus.Person->PersonType) {
                case PERSON_DARCI:
                    MFX_play_thing(THING_NUMBER(p_person), S_DARCI_HIT_END, 0, p_person);
                    break;
                case PERSON_ROPER:
                    MFX_play_thing(THING_NUMBER(p_person), S_ROPER_HIT_END - 1, 0, p_person);
                    break;
                }
                break;
            }

            set_anim(p_person, ANIM_FLY_GRABBING_LEDGE);
            p_person->Draw.Tweened->Angle = grab_angle;
        }

        // Reposition person so that their hand midpoint aligns with the grabbed point.
        {
            SLONG lx;
            SLONG ly;
            SLONG lz;

            calc_sub_objects_position(p_person, 0, SUB_OBJECT_LEFT_HAND, &lx, &ly, &lz);

            SLONG rx;
            SLONG ry;
            SLONG rz;

            calc_sub_objects_position(p_person, 0, SUB_OBJECT_RIGHT_HAND, &rx, &ry, &rz);

            new_x = lx + rx >> 1;
            new_y = ly + ry >> 1;
            new_z = lz + rz >> 1;
        }

        new_x += p_person->WorldPos.X >> 8;
        new_y += p_person->WorldPos.Y >> 8;
        new_z += p_person->WorldPos.Z >> 8;

        MSG_add(" left hand y %d grab_y %d tween %d\n", new_y, grab_y, draw_info->AnimTween);

        temp_pos.X = ((grab_x - new_x) << 8) + p_person->WorldPos.X;
        temp_pos.Y = ((grab_y - new_y) << 8) + p_person->WorldPos.Y;
        temp_pos.Z = ((grab_z - new_z) << 8) + p_person->WorldPos.Z;

        if (face > 0 && (prim_faces4[face].FaceFlags & (FACE_FLAG_WMOVE | FACE_FLAG_PRIM))) {
            // For moving/primitive faces, nudge person slightly away from the face
            // to avoid falling through when pulled up.
            temp_pos.X += -SIN(p_person->Draw.Tweened->Angle) >> 5;
            temp_pos.Z += -COS(p_person->Draw.Tweened->Angle) >> 5;
        }

        move_thing_on_map(p_person, &temp_pos);

        draw_info->Locked = SUB_OBJECT_LEFT_HAND;
        if (face == GRAB_FLOOR || face == GRAB_SEWERS) {
            MSG_add("grab floor or sewers\n");
            p_person->OnFace = 0;
        } else {
            if (type == 1) {
                p_person->Genus.Person->OnFacet = face;
                p_person->OnFace = 0;
            } else {
                p_person->OnFace = face;
            }
        }

        // Automatically pull up if grabbing a moving/primitive walkable face.
        if (face > 0 && (prim_faces4[face].FaceFlags & (FACE_FLAG_WMOVE | FACE_FLAG_PRIM))) {
            // Don't auto-pull-up on crates (prim == 129).
            if (prim_faces4[face].FaceFlags & FACE_FLAG_PRIM) {
                SLONG ob_index = -prim_faces4[face].ThingIndex;

                ASSERT(WITHIN(ob_index, 1, OB_ob_upto - 1));

                if (OB_ob[ob_index].prim == 129) {
                    goto dont_pull_up;
                }
            }

            set_person_pulling_up(p_person);

        dont_pull_up:;
        }

        return (1);
    }

    return (0);
}

// uc_orig: set_tween_for_dy (fallen/Source/Person.cpp)
void set_tween_for_dy(Thing* p_person, SLONG dy)
{
    MSG_add(" tween dy %d \n", dy);

    dy -= 28;

    if (dy > 128)
        dy = 0;
    else if (dy < 0)
        dy = 255;
    else
        dy = (128 - dy) << 1;

    p_person->Draw.Tweened->AnimTween = dy;
}

// uc_orig: set_tween_for_height (fallen/Source/Person.cpp)
void set_tween_for_height(Thing* p_person)
{
    SLONG x, y, z;
    SLONG floor_y, dy;

    if (p_person->DY > 0)
        return;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, 3, &x, &y, &z);
    y += (p_person->WorldPos.Y) >> 8;

    if (p_person->Flags & FLAGS_IN_SEWERS) {
        floor_y = NS_calc_height_at(x, z);
    } else {
        floor_y = PAP_calc_height_at_thing(p_person, x, z);
    }

    dy = y - floor_y;
    set_tween_for_dy(p_person, 256 + (p_person->DY >> 5));
}

// uc_orig: over_nogo (fallen/Source/Person.cpp)
SLONG over_nogo(Thing* p_person)
{
    SLONG mx, mz;

    if (p_person->Genus.Person->Ware)
        return (0);
    mx = p_person->WorldPos.X >> 16;
    mz = p_person->WorldPos.Z >> 16;
    if (mx >= 0 && mx < 128 && mz >= 0 && mz <= 128) {
        if (PAP_2HI((mx) & 127, (mz) & 127).Flags & PAP_FLAG_NOGO) {
            return 1;
        }
    }
    return (0);
}

// STATE_JUMPING state machine. Key sub-states:
//   SUB_STATE_STANDING_JUMP_FORWARDS/BACKWARDS: short forward/backward jump arc.
//   SUB_STATE_STANDING_JUMP: standing jump — grab check, sound at frame 4.
//   SUB_STATE_RUNNING_JUMP: running jump phase 1; clamps velocity [37..45] for player.
//   SUB_STATE_RUNNING_JUMP_FLY: main air phase; grab check; landing triggers land anims.
//   SUB_STATE_RUNNING_JUMP_LAND_FAST: sliding landing into run.
//   SUB_STATE_FLYING_KICK: aerial kick sequence.
// plant_feet() is intentionally commented out everywhere in this function (original).
// uc_orig: fn_person_jumping (fallen/Source/Person.cpp)
void fn_person_jumping(Thing* p_person)
{
    SLONG end = 0,
          grab = 0,
          wave_id1,
          wave_id2;
    GameCoord new_position;
    SLONG old_frame, frame;

    SlideSoundCheck(p_person);

    switch (p_person->SubState) {
    case SUB_STATE_STANDING_JUMP_FORWARDS:
        change_velocity_to(p_person, 32);

        if (over_nogo(p_person))
            p_person->Velocity = 0;

        p_person->DeltaVelocity = 0;
        p_person->DY = 0;
        end = projectile_move_thing(p_person, 1);
        if (end == 100) {
            return;
        }

        if (!end) {
            end = person_normal_animate(p_person);
            if (end) {
                p_person->DeltaVelocity = 0;
                p_person->DY = 10 << 8;
                p_person->SubState = SUB_STATE_RUNNING_JUMP_FLY;
                p_person->Genus.Person->Action = ACTION_RUN_JUMP;
                if (person_has_gun_out(p_person)) {
                    set_locked_anim(p_person, ANIM_MID_AIR_TWEEN_LEFT_AK, 0);
                } else {
                    set_locked_anim(p_person, ANIM_MID_AIR_TWEEN_LEFT, 0);
                }
                p_person->Draw.Tweened->NextFrame = global_anim_array[p_person->Genus.Person->AnimType][ANIM_MID_AIR_TWEEN_LEFT];
            }
        }

        grab = grab_ledge(p_person);

        break;
    case SUB_STATE_STANDING_JUMP_BACKWARDS:
        change_velocity_to(p_person, -32);
        if (over_nogo(p_person))
            p_person->Velocity = 0;

        p_person->DeltaVelocity = 0;
        p_person->DY = 0;
        if (p_person->Draw.Tweened->FrameIndex > 2) {
            if (p_person->Draw.Tweened->FrameIndex > 4)
                end = projectile_move_thing(p_person, 3);
            else
                end = projectile_move_thing(p_person, 1);
            if (end == 100) {
                return;
            }
            if (end == 1) {
                set_person_idle(p_person);
                return;
            } else if (end == 2) {
                set_person_drop_down(p_person, 0);
                p_person->Velocity = 0;
                return;
            }
        } else {
            end = 0;
        }

        if (!end)
            end = person_normal_animate(p_person);

        if (end == 1) {
            set_person_drop_down(p_person, 0);
            p_person->DY = -20 << 8;
        }

        break;

    case SUB_STATE_STANDING_JUMP:
        MSG_add(" standing jump ");
        end = person_normal_animate(p_person);
        grab = grab_ledge(p_person);
        SLONG pinnacle_frame;

        switch (p_person->Genus.Person->AnimType) {
        case ANIM_TYPE_ROPER:
            pinnacle_frame = 5;
            break;
        default:
            pinnacle_frame = 3;
        }
        if (end && !grab) {
            set_person_drop_down(p_person, PERSON_DROP_DOWN_KEEP_VEL | PERSON_DROP_DOWN_QUEUED);
        } else {
            if (end == 1) {
                MSG_add("end standing jump anim");
            }
            if (grab) {
                MSG_add("grab standing jump");
            }

            if ((end == 1) && !grab) {
                set_person_idle(p_person);
                person_splash(p_person, -1);
            }

            if (p_person->Draw.Tweened->FrameIndex == 4) {
                wave_id1 = footstep_wave(p_person);
                wave_id2 = footstep_wave(p_person);
                MFX_play_thing(THING_NUMBER(p_person), wave_id1, MFX_REPLACE, p_person);
                MFX_play_thing(THING_NUMBER(p_person), wave_id2, MFX_REPLACE, p_person);
            }
        }

        break;
    case SUB_STATE_RUNNING_JUMP:
        if (p_person->Genus.Person->PlayerID) {
            // Clamp velocity to [37..45] for running jumps (player only).
            SLONG reqd_vel = (50 * 15) / 20;
            if (p_person->Velocity < (50 * 15) / 20)
                p_person->Velocity = (50 * 15) / 20;
            else if (p_person->Velocity > (60 * 15) / 20)
                p_person->Velocity = (60 * 15) / 20;
        }
        if (over_nogo(p_person))
            p_person->Velocity = 0;

        p_person->DeltaVelocity = 0;
        end = projectile_move_thing(p_person, 1);
        if (end == 100) {
            return;
        }

        old_frame = p_person->Draw.Tweened->FrameIndex;

        if (old_frame < 3) {
            if (check_on_slippy_slope(p_person))
                return;
        }
        if (!end)
            end = person_normal_animate(p_person);
        frame = p_person->Draw.Tweened->FrameIndex;

        p_person->DY = 0;

        grab = grab_ledge(p_person);
        if (grab) {
            MSG_add(" running jump grabbed 1\n");
        }
        if (!grab) {
            if (end == 1) {
                p_person->Draw.Tweened->Locked = 0;

                p_person->Flags |= FLAGS_PROJECTILE_MOVEMENT;
                p_person->DeltaVelocity = 0;
                p_person->DY = 10 << 8;
                p_person->SubState = SUB_STATE_RUNNING_JUMP_FLY;
                p_person->Genus.Person->Action = ACTION_RUN_JUMP;
                if (person_has_gun_out(p_person)) {
                    p_person->Draw.Tweened->NextFrame = global_anim_array[p_person->Genus.Person->AnimType][ANIM_MID_AIR_TWEEN_LEFT_AK];
                } else {
                    p_person->Draw.Tweened->NextFrame = global_anim_array[p_person->Genus.Person->AnimType][ANIM_MID_AIR_TWEEN_LEFT];
                }

                p_person->Draw.Tweened->AnimTween = 0;
            }
        }
        break;
    case SUB_STATE_RUNNING_JUMP_FLY_STOP:
        change_velocity_to(p_person, 0);
        goto jump_fly;

    case SUB_STATE_RUNNING_JUMP_FLY:
        if (continue_moveing(p_person) && !over_nogo(p_person)) {
            // keep going
        } else {
            set_person_drop_down(p_person, PERSON_DROP_DOWN_KEEP_VEL | PERSON_DROP_DOWN_KEEP_DY | PERSON_DROP_DOWN_QUEUED);
            p_person->Velocity >>= 1;
            return;
        }

    jump_fly:;

        if (p_person->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_START || p_person->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_FORWARDS) {
            end = person_normal_animate(p_person);

            if (end == 1) {
                if (p_person->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_START) {
                    locked_anim_change(p_person, SUB_OBJECT_PELVIS, ANIM_PLUNGE_FORWARDS);
                }
            }
        } else {
            set_tween_for_height(p_person);
        }

        end = projectile_move_thing(p_person, 3);

        if (end == 100) {
            return;
        }

        grab = grab_ledge(p_person);
        if (grab) {
            MSG_add("grab face in running jump fly \n");
            if (end == 2) {
                MSG_add(" hit fence, but grabbed something as well");
            }
        } else if (end == 1) {
            p_person->Genus.Person->Action = ACTION_LANDING;
            person_splash(p_person, -1);

            DrawTween* draw_info;

            draw_info = p_person->Draw.Tweened;
            if (continue_moveing(p_person)) {
                // Player wants to land and keep running.
                if (person_has_gun_out(p_person)) {
                    set_locked_anim(p_person, ANIM_LAND_RIGHT_AK, SUB_OBJECT_RIGHT_FOOT);
                } else {
                    set_locked_anim(p_person, ANIM_LAND_RIGHT, SUB_OBJECT_RIGHT_FOOT);
                }

                if (p_person->OnFace == 0) {
                    // already set by projectile_move_thing
                } else {
                }

                p_person->SubState = SUB_STATE_RUNNING_JUMP_LAND_FAST;
                p_person->Genus.Person->Action = ACTION_RUN;

            } else {
                // Player stopped pressing forward — land and stand idle.
                set_locked_anim(p_person, ANIM_LAND_STAND, 0);

                if (p_person->OnFace == 0) {
                    // already set
                } else {

                    SLONG new_y, face;
                    face = find_face_for_this_pos(p_person->WorldPos.X >> 8, p_person->WorldPos.Y >> 8, p_person->WorldPos.Z >> 8, &new_y, 0, 0);
                    {
                        ASSERT(face);
                        p_person->OnFace = face;
                    }
                }

                set_person_idle(p_person);
            }

            draw_info->AnimTween = 0;
        } else if (end == 2) {
            MSG_add(" hit fence, so hold on");
        } else {
            // Check for flying kick request during air phase.
            if (p_person->Genus.Person->Flags & FLAG_PERSON_REQUEST_KICK) {
                p_person->Genus.Person->Flags &= ~FLAG_PERSON_REQUEST_KICK;
                set_anim(p_person, ANIM_FLYKICK_START);
                p_person->SubState = SUB_STATE_FLYING_KICK;
            }
        }
        break;
    case SUB_STATE_FLYING_KICK:
        end = person_normal_animate(p_person);
        if (end) {
            queue_anim(p_person, ANIM_FLYKICK_FALL);
            p_person->SubState = SUB_STATE_FLYING_KICK_FALL;
        }
        // fall through into next case
    case SUB_STATE_FLYING_KICK_FALL:
        end = person_normal_animate(p_person);
        if (over_nogo(p_person))
            p_person->Velocity = 0;
        end = projectile_move_thing(p_person, 3);
        if (end == 100) {
            return;
        } else if (end == 1) {
            p_person->Genus.Person->Action = ACTION_LANDING;
            person_splash(p_person, -1);

            DrawTween* draw_info;

            draw_info = p_person->Draw.Tweened;
            {
                set_locked_anim(p_person, ANIM_FLYKICK_LAND, SUB_OBJECT_RIGHT_FOOT);

                if (p_person->OnFace == 0) {
                    p_person->WorldPos.Y = PAP_calc_height_at_thing(p_person, p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8) << 8;
                } else {
                }

                p_person->SubState = SUB_STATE_RUNNING_JUMP_LAND_FAST;
            }

            draw_info->AnimTween = 0;
        }
        break;

        break; // unreachable duplicate break (original bug, kept 1:1)
    case SUB_STATE_RUNNING_JUMP_LAND:
        MSG_add("RJ        LANDING ");
        change_velocity_to(p_person, yomp_speed);
        if (over_nogo(p_person))
            p_person->Velocity = 0;
        end = projectile_move_thing(p_person, 3);
        if (end == 100) {
            return;
        }
        if (end) {
            set_person_idle(p_person);
        }
        break;
    case SUB_STATE_RUNNING_JUMP_LAND_FAST:
        MSG_add("RJ        LANDING FAST");
        change_velocity_to(p_person, yomp_speed);
        if (over_nogo(p_person))
            p_person->Velocity = 0;
        person_normal_move(p_person);

        if (p_person->SubState != SUB_STATE_RUNNING_JUMP_LAND_FAST) {
            p_person->DY = -4001;
            return;
        }
        end = person_normal_animate(p_person);

        if (end || !continue_moveing(p_person)) {
            SLONG anim;

            anim = get_yomp_anim(p_person);

            set_anim(p_person, anim);
            set_generic_person_state_function(p_person, STATE_MOVEING);
            p_person->SubState = SUB_STATE_RUNNING;
            p_person->Genus.Person->Action = ACTION_RUN;
            p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

            wave_id1 = footstep_wave(p_person);
            wave_id2 = footstep_wave(p_person);
            MFX_play_thing(THING_NUMBER(p_person), wave_id1, MFX_REPLACE, p_person);
            MFX_play_thing(THING_NUMBER(p_person), wave_id2, MFX_REPLACE, p_person);
        }
        break;
    default:
        MSG_add("JUMPING unknow substate %d \n", p_person->SubState);
        break;
    }
}

// uc_orig: position_person_at_ladder_top (fallen/Source/Person.cpp)
void position_person_at_ladder_top(Thing* p_person, SLONG limb)
{
    SLONG x1, y1, z1;
    SLONG x[2], z[2], y, wall;
    struct DFacet* p_facet;
    SLONG top;

    p_facet = &dfacets[p_person->Genus.Person->OnFacet];

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, limb, &x1, &y1, &z1);

    y1 += p_person->WorldPos.Y >> 8;

    top = p_facet->Y[0] + p_facet->Height * 64;
    y1 -= top - 20;

    // y1 is the difference between the limb position and the ladder top.
    p_person->WorldPos.Y -= y1 << 8;
}

// uc_orig: position_person_at_ladder_bot (fallen/Source/Person.cpp)
void position_person_at_ladder_bot(Thing* p_person, SLONG limb)
{
    SLONG x1, y1, z1;
    SLONG x[2], z[2], y, wall;
    struct DFacet* p_facet;
    SLONG bot;

    p_facet = &dfacets[p_person->Genus.Person->OnFacet];

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, limb, &x1, &y1, &z1);

    y1 += p_person->WorldPos.Y >> 8;

    bot = p_facet->Y[0];
    y1 -= bot;

    // y1 is the difference between the limb position and the ladder bottom.
    p_person->WorldPos.Y = bot << 8;
}

// Returns a bitmask of PERSON_LIMB_* flags indicating where the sub_part limb
// is relative to the ladder facet (on/off top/bottom).
// i_am_going_down adds tolerance at the bottom for stepping off.
// uc_orig: check_limb_pos_on_ladder (fallen/Source/Person.cpp)
ULONG check_limb_pos_on_ladder(Thing* p_person, SLONG sub_part, SLONG i_am_going_down)
{
    SLONG x1, y1, z1;
    UWORD facet;
    SLONG x[2], z[2], y, wall;
    SLONG top;
    SLONG bot;
    ULONG ans;

    struct DFacet* p_facet;

    facet = p_person->Genus.Person->OnFacet;

    p_facet = &dfacets[facet];

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, sub_part, &x1, &y1, &z1);

    x1 += p_person->WorldPos.X >> 8;
    y1 += p_person->WorldPos.Y >> 8;
    z1 += p_person->WorldPos.Z >> 8;

    x[0] = p_facet->x[0] << 8;
    z[0] = p_facet->z[0] << 8;

    x[1] = p_facet->x[1] << 8;
    z[1] = p_facet->z[1] << 8;

    bot = p_facet->Y[0];

    top = bot + p_facet->Height * 64;

    ans = 0;

    if (WITHIN(y1, bot + ((i_am_going_down) ? 20 : 0), top - 20)) {
        ans |= PERSON_LIMB_ON_LADDER;

        if (y1 > top - 256) {
            ans |= PERSON_LIMB_TOP_BLOCK;
        }
        if (y1 < bot + 256) {
            ans |= PERSON_LIMB_BOT_BLOCK;
        }
    } else {
        if (y1 < bot + 20) {
            ans |= PERSON_LIMB_OFF_BOT;
        }
        if (y1 > top + 20) {
            ans |= PERSON_LIMB_OFF_TOP;
        }
    }

    return ans;
}

// Returns TRUE if the sub_part limb is within the height range of the fence facet.
// Limbs too close to the bottom do not count as "on the fence".
// uc_orig: check_limb_pos_on_fence (fallen/Source/Person.cpp)
SLONG check_limb_pos_on_fence(Thing* p_person, SLONG sub_part)
{
    SLONG x1, y1, z1;
    SLONG col;
    SLONG top, bottom;
    SLONG x, z;

    x = p_person->WorldPos.X >> 8;
    z = p_person->WorldPos.Z >> 8;
    col = p_person->Genus.Person->OnFacet;

    top = get_fence_top(x, z, col);
    bottom = get_fence_bottom(x, z, col);

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, sub_part, &x1, &y1, &z1);

    y1 += p_person->WorldPos.Y >> 8;

    if (WITHIN(y1, bottom + 20, top)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

// Returns TRUE if the sub_part limb is within the sideways extent of the fence facet
// (not off either end). Uses calc_along_vect to determine position along the fence.
// uc_orig: check_limb_pos_on_fence_sideways (fallen/Source/Person.cpp)
SLONG check_limb_pos_on_fence_sideways(Thing* p_person, SLONG sub_part)
{
    SLONG x1, y1, z1;
    SLONG col;
    SLONG along;

    col = p_person->Genus.Person->OnFacet;

    // calc_along_vect is a file-local function in collide.cpp; forward declared inline here.
    SLONG calc_along_vect(SLONG ax, SLONG az, struct DFacet * p_vect);

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, sub_part, &x1, &y1, &z1);

    x1 += p_person->WorldPos.X >> 8;
    z1 += p_person->WorldPos.Z >> 8;

    along = calc_along_vect(x1, z1, &dfacets[col]);

    if (along <= 0 || along >= 255) {
        return (0);
    } else {
        return (1);
    }
}

// STATE_CLIMB_LADDER state machine. Handles mounting, climbing up/down, and
// stepping off the top or bottom of ladders. Plays rung sounds on each stride.
// uc_orig: fn_person_laddering (fallen/Source/Person.cpp)
void fn_person_laddering(Thing* p_person)
{
    BOOL play_it = FALSE;
    UBYTE last_frame;
    SLONG end = 0, hit,
          foot_step_wave;
    ULONG on_ladder, on_ladder_left, on_ladder_right;

    switch (p_person->SubState) {
    case SUB_STATE_MOUNT_LADDER:
        last_frame = p_person->Draw.Tweened->FrameIndex;
        end = person_normal_animate(p_person);
        if (p_person->Draw.Tweened->FrameIndex != last_frame && p_person->Draw.Tweened->FrameIndex == 3)
            play_it = TRUE;

        if (end == 1) {
            set_person_on_ladder(p_person);
        }
        break;
    case SUB_STATE_ON_LADDER:
    case SUB_STATE_STOPPING:
        break;
    case SUB_STATE_CLIMB_UP_LADDER:

        on_ladder_left = check_limb_pos_on_ladder(p_person, SUB_OBJECT_LEFT_HAND, FALSE);
        on_ladder_right = check_limb_pos_on_ladder(p_person, SUB_OBJECT_RIGHT_HAND, FALSE);

        if ((on_ladder_left & on_ladder_right) & PERSON_LIMB_ON_LADDER) {
            last_frame = p_person->Draw.Tweened->FrameIndex;
            end = person_normal_animate(p_person);
            if (p_person->Draw.Tweened->FrameIndex != last_frame && ((p_person->Draw.Tweened->FrameIndex % 3 == 0) || (p_person->Draw.Tweened->FrameIndex % 6 == 0)))
                play_it = TRUE;

            if (end == 1) {
                if (p_person->Genus.Person->PersonType == PERSON_ROPER)
                    locked_anim_change_height_type(p_person, SUB_OBJECT_LEFT_HAND, COP_ROPER_ANIM_LADDER_LOOP, p_person->Genus.Person->AnimType);
                else
                    if (p_person->Genus.Person->PersonType == PERSON_COP || p_person->Genus.Person->PersonType == PERSON_THUG_GREY || p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_THUG_RED)
                        locked_anim_change_height_type(p_person, SUB_OBJECT_LEFT_HAND, COP_ROPER_ANIM_LADDER_LOOP, ANIM_TYPE_ROPER);
                    else
                        locked_anim_change_height_type(p_person, SUB_OBJECT_LEFT_HAND, ANIM_ON_LADDER, ANIM_TYPE_DARCI);

                if (p_person->Genus.Person->PlayerID) {
                    p_person->SubState = SUB_STATE_STOPPING;
                }
            }

            // Commented-out sewer ladder exit code (disabled in original):
            /*
            if (p_person->Flags & FLAGS_IN_SEWERS) {
                SLONG   ladder = p_person->Genus.Person->OnFacet;
                DFacet *df;
                ASSERT(WITHIN(ladder, 1, next_dfacet - 1));
                ASSERT(dfacets[ladder].FacetType == STOREY_TYPE_LADDER);
                df = &dfacets[ladder];
                if (df->FacetFlags & FACET_FLAG_LADDER_LINK)
                {
                    if ((on_ladder_left|on_ladder_right) & PERSON_LIMB_TOP_BLOCK)
                    {
                        p_person->Flags &= ~FLAGS_IN_SEWERS;
                    }
                }
            }
            */
        } else {
            // Hands reached the top — climb off.
            p_person->SubState = SUB_STATE_CLIMB_OFF_LADDER_TOP;
            if (p_person->Genus.Person->PersonType == PERSON_ROPER)
                set_anim_of_type(p_person, COP_ROPER_ANIM_LADDER_END_L, ANIM_TYPE_ROPER);
            else if (p_person->Genus.Person->PersonType == PERSON_COP || p_person->Genus.Person->PersonType == PERSON_THUG_GREY || p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_THUG_RED)
                set_anim_of_type(p_person, COP_ROPER_ANIM_LADDER_END_L, ANIM_TYPE_ROPER);
            else
                set_anim(p_person, ANIM_OFF_LADDER_TOP);

            if (on_ladder_left)
                position_person_at_ladder_top(p_person, SUB_OBJECT_RIGHT_HAND);
            else
                position_person_at_ladder_top(p_person, SUB_OBJECT_LEFT_HAND);
            p_person->Genus.Person->Action = ACTION_CLIMBING;
        }

        break;
    case SUB_STATE_CLIMB_DOWN_ONTO_LADDER:
        end = person_backwards_animate(p_person);
        if (end == 1) {
            set_person_on_ladder(p_person);
        }
        break;
    case SUB_STATE_CLIMB_DOWN_LADDER:

        on_ladder_left = check_limb_pos_on_ladder(p_person, SUB_OBJECT_LEFT_FOOT, TRUE);
        on_ladder_right = check_limb_pos_on_ladder(p_person, SUB_OBJECT_RIGHT_FOOT, TRUE);

        if ((on_ladder_left & on_ladder_right) & PERSON_LIMB_ON_LADDER) {
            last_frame = p_person->Draw.Tweened->FrameIndex;
            end = person_backwards_animate(p_person);
            if (p_person->Draw.Tweened->FrameIndex != last_frame && ((p_person->Draw.Tweened->FrameIndex % 3 == 0) || (p_person->Draw.Tweened->FrameIndex % 6 == 0)))
                play_it = TRUE;

            if (end == 1) {
                MSG_add(" LOCKED CHANGE END \n");
                p_person->Draw.Tweened->AnimTween = 0;
                if (p_person->Genus.Person->PersonType == PERSON_ROPER)
                    locked_anim_change_end_type(p_person, SUB_OBJECT_LEFT_HAND, COP_ROPER_ANIM_LADDER_LOOP, p_person->Genus.Person->AnimType);
                else
                    if (p_person->Genus.Person->PersonType == PERSON_COP || p_person->Genus.Person->PersonType == PERSON_THUG_GREY || p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_THUG_RED)
                        locked_anim_change_end_type(p_person, SUB_OBJECT_LEFT_HAND, COP_ROPER_ANIM_LADDER_LOOP, ANIM_TYPE_ROPER);
                    else
                        locked_anim_change_end_type(p_person, SUB_OBJECT_LEFT_HAND, ANIM_ON_LADDER, p_person->Genus.Person->AnimType);

                p_person->SubState = SUB_STATE_STOPPING;
            }

            // Commented-out sewer ladder entry code (disabled in original):
            /*
            if (p_person->Flags & FLAGS_IN_SEWERS) {
                SLONG   ladder = p_person->Genus.Person->OnFacet;
                DFacet *df;
                ASSERT(WITHIN(ladder, 1, next_dfacet - 1));
                ASSERT(dfacets[ladder].FacetType == STOREY_TYPE_LADDER);
                df = &dfacets[ladder];
                if (df->FacetFlags & FACET_FLAG_LADDER_LINK)
                {
                    if (!(on_ladder_left & PERSON_LIMB_TOP_BLOCK))
                    {
                        p_person->Flags |= FLAGS_IN_SEWERS;
                    }
                }
            }
            */
        } else {
            // Feet reached the bottom — climb off.
            p_person->SubState = SUB_STATE_CLIMB_OFF_LADDER_BOT;
            set_anim(p_person, ANIM_OFF_LADDER_BOT);
            position_person_at_ladder_bot(p_person, SUB_OBJECT_LEFT_FOOT);

            p_person->Genus.Person->Action = ACTION_CLIMBING;
        }

        break;
    case SUB_STATE_CLIMB_OFF_LADDER_BOT:
        end = person_normal_animate(p_person);
        if (end == 1) {
            set_person_idle(p_person);
            p_person->Draw.Tweened->CurrentFrame = p_person->Draw.Tweened->QueuedFrame;
            p_person->Draw.Tweened->NextFrame = p_person->Draw.Tweened->QueuedFrame;
            p_person->Draw.Tweened->QueuedFrame = 0;
            locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, ANIM_STAND_READY);
            plant_feet(p_person);
        }

        break;
    case SUB_STATE_CLIMB_OFF_LADDER_TOP:
        last_frame = p_person->Draw.Tweened->FrameIndex;
        end = person_normal_animate(p_person);
        if (p_person->Draw.Tweened->FrameIndex != last_frame && (p_person->Draw.Tweened->FrameIndex == 1 || p_person->Draw.Tweened->FrameIndex == 3))
            play_it = TRUE;

        if (end == 1) {
            person_bodge_forward(p_person, 40);
            locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, ANIM_STAND_READY);
            plant_feet(p_person);
            if (p_person->State != STATE_DANGLING)
                set_person_idle(p_person);
        }

        break;
    default:
        MSG_add("LADDERING unknow substate %d \n", p_person->SubState);
        break;
    }
    if (play_it) {
        foot_step_wave = ((Random() * (S_FOOTS_RUNG_END - S_FOOTS_RUNG_START)) >> 16) + S_FOOTS_RUNG_START;
        MFX_play_thing(THING_NUMBER(p_person), foot_step_wave, MFX_OVERLAP, p_person);
    }
}

// STATE_CLIMBING state machine. Handles fence climbing sub-states:
// landing on fence, climbing up/down wall, climbing over fence top, and electric fence damage.
// uc_orig: fn_person_climbing (fallen/Source/Person.cpp)
void fn_person_climbing(Thing* p_person)
{
    SLONG end, hit;
    SLONG left_foot, right_foot;

    switch (p_person->SubState) {
    case SUB_STATE_CLIMB_LANDING:
        end = person_normal_animate(p_person);

        if (end == 1) {
            if (p_person->Genus.Person->AnimType != ANIM_TYPE_ROPER)
                queue_anim(p_person, ANIM_LANDED_ON_FENCE);
            p_person->SubState = SUB_STATE_CLIMB_LANDING2;

            {
                // If person touches an electric fence, spark effects and knockback.
                if (p_person->Genus.Person->OnFacet > 0) {
                    SLONG col = p_person->Genus.Person->OnFacet;

                    if (col > 0) {
                        if (dfacets[col].FacetFlags & FACET_FLAG_ELECTRIFIED) {
                            SLONG i;

                            SLONG px;
                            SLONG py;
                            SLONG pz;
                            SPARK_Pinfo p1;
                            SPARK_Pinfo p2;

                            UBYTE limb[4] = {
                                SUB_OBJECT_LEFT_FOOT,
                                SUB_OBJECT_LEFT_HAND,
                                SUB_OBJECT_RIGHT_FOOT,
                                SUB_OBJECT_RIGHT_HAND
                            };

                            for (i = 0; i < 4; i++) {
                                calc_sub_objects_position(
                                    p_person,
                                    p_person->Draw.Tweened->AnimTween,
                                    limb[i],
                                    &px,
                                    &py,
                                    &pz);

                                px += p_person->WorldPos.X >> 8;
                                py += p_person->WorldPos.Y >> 8;
                                pz += p_person->WorldPos.Z >> 8;

                                p1.type = SPARK_TYPE_LIMB;
                                p1.flag = 0;
                                p1.person = THING_NUMBER(p_person);
                                p1.limb = limb[i];

                                p2.type = SPARK_TYPE_POINT;
                                p2.flag = SPARK_FLAG_STILL;
                                p2.x = px;
                                p2.y = py;
                                p2.z = pz;

                                SPARK_create(
                                    &p1,
                                    &p2,
                                    20);
                            }

                            {
                                SLONG origin_x = p_person->WorldPos.X - SIN(p_person->Draw.Tweened->Angle) >> 8;
                                SLONG origin_z = p_person->WorldPos.Z - COS(p_person->Draw.Tweened->Angle) >> 8;

                                knock_person_down(
                                    p_person,
                                    30,
                                    origin_x,
                                    origin_z,
                                    NULL);
                            }

                            return;
                        }
                    }
                }
            }
        }

        break;

    case SUB_STATE_CLIMB_LANDING2:
        end = person_normal_animate(p_person);
        if (end == 1) {
            set_person_on_fence(p_person);
        }
        break;

    case SUB_STATE_CLIMB_AROUND_WALL:
        break;

    case SUB_STATE_CLIMB_UP_WALL:

        // If hand touches the top of a barbed-wire or angle-top fence, fall off.
        if (dfacets[p_person->Genus.Person->OnFacet].FacetType == STOREY_TYPE_FENCE_BRICK || dfacets[p_person->Genus.Person->OnFacet].FacetType == STOREY_TYPE_FENCE) {
            if (!check_limb_pos_on_fence(p_person, SUB_OBJECT_RIGHT_HAND) || !check_limb_pos_on_fence(p_person, SUB_OBJECT_LEFT_HAND)) {
                set_person_drop_down(p_person, 0);

                goto dont_check_climbing_over;
            }
        }

        if (check_limb_pos_on_fence(p_person, SUB_OBJECT_HEAD)) {
            end = person_normal_animate(p_person);
            if (end == 1) {
                locked_anim_change(p_person, SUB_OBJECT_LEFT_HAND, ANIM_CLIMB_UP_FENCE);

                if (p_person->Genus.Person->PlayerID) {
                    p_person->SubState = SUB_STATE_STOPPING;
                }
            }
        } else {
            if (dfacets[p_person->Genus.Person->OnFacet].FacetType == STOREY_TYPE_NORMAL) {
                grab_ledge(p_person);
            } else if (dfacets[p_person->Genus.Person->OnFacet].FacetType != STOREY_TYPE_FENCE_BRICK) {
                // Climb off top of fence.
                p_person->SubState = SUB_STATE_CLIMB_OVER_WALL;
                locked_anim_change(p_person, 0, ANIM_CLIMB_OVER_FENCE);
                set_limb_to_y(p_person, SUB_OBJECT_LEFT_HAND, get_fence_top(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, p_person->Genus.Person->OnFacet));
            } // else: blocked from climbing by barbed wire
        }

    dont_check_climbing_over:;

        break;

    case SUB_STATE_CLIMB_DOWN_WALL:

        left_foot = check_limb_pos_on_fence(p_person, SUB_OBJECT_LEFT_FOOT);
        right_foot = check_limb_pos_on_fence(p_person, SUB_OBJECT_RIGHT_FOOT);

        if (left_foot && right_foot) {
            end = person_backwards_animate(p_person);

            if (end == 1) {
                locked_anim_change_end(
                    p_person,
                    SUB_OBJECT_LEFT_HAND,
                    ANIM_CLIMB_UP_FENCE);

                if (p_person->Genus.Person->PlayerID) {
                    p_person->SubState = SUB_STATE_STOPPING;
                }
            }
        } else {
            // Check if the bottom of the fence is close to the ground.
            {
                SLONG ground_y = PAP_calc_height_at_thing(
                    p_person,
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Z >> 8);

                if ((p_person->WorldPos.Y >> 8) <= ground_y + 0x40) {
                    // Climb off onto the ground.
                    p_person->SubState = SUB_STATE_CLIMB_OFF_BOT_WALL;
                    set_anim(p_person, ANIM_OFF_LADDER_BOT);
                    set_limb_to_y(p_person, SUB_OBJECT_RIGHT_FOOT, get_fence_bottom(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8, p_person->Genus.Person->OnFacet));
                } else {
                    // Fence is in mid-air — fall off backwards.
                    set_person_drop_down(p_person, 0);
                }
            }
        }

        break;
    case SUB_STATE_CLIMB_OFF_BOT_WALL:
        end = person_normal_animate(p_person);
        if (end == 1) {
            set_person_idle(p_person);
            p_person->Draw.Tweened->CurrentFrame = p_person->Draw.Tweened->QueuedFrame;
            p_person->Draw.Tweened->NextFrame = p_person->Draw.Tweened->QueuedFrame;
            p_person->Draw.Tweened->QueuedFrame = 0;
            plant_feet(p_person);
            if (p_person->State != STATE_DANGLING)
                locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, ANIM_STAND_READY);
        }
        break;
    case SUB_STATE_CLIMB_OVER_WALL:
        end = person_normal_animate(p_person);
        if (end == 1) {
            set_person_locked_drop_down(p_person, -32);
            p_person->WorldPos.Y += p_person->DY;
        }
        break;
    case SUB_STATE_STOPPING:
        break;
    default:
        MSG_add("CLIMBING unknow substate %d \n", p_person->SubState);
        break;
    }
}

// uc_orig: set_cable_angle (fallen/Source/Person.cpp)
void set_cable_angle(Thing* p_person)
{
    SLONG dx, dz;
    struct DFacet* p_facet;
    SLONG angle;

    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_ON_CABLE);
    ASSERT(dfacets[p_person->Genus.Person->OnFacet].FacetType == STOREY_TYPE_CABLE);

    p_facet = &dfacets[p_person->Genus.Person->OnFacet];

    dx = p_facet->x[1] - p_facet->x[0] << 8;
    dz = p_facet->z[1] - p_facet->z[0] << 8;

    angle = calc_angle(dx, dz);

    p_person->Draw.Tweened->Angle = angle;
}

// Tracks person Y to the cable height at their current XZ position.
// Both hands are averaged to find the grip point along the cable.
// uc_orig: do_person_on_cable (fallen/Source/Person.cpp)
void do_person_on_cable(Thing* p_person)
{
    SLONG along;
    SLONG mx, my, mz;

    SLONG hx;
    SLONG hy;
    SLONG hz;

    SLONG lhx, lhy, lhz;
    SLONG rhx, rhy, rhz;

    if (p_person->Genus.Person->Flags & FLAG_PERSON_ON_CABLE) {
        calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_HAND, &lhx, &lhy, &lhz);
        calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_HAND, &rhx, &rhy, &rhz);

        hx = lhx + rhx >> 1;
        hy = lhy + rhy >> 1;
        hz = lhz + rhz >> 1;

        along = get_cable_along(p_person->Genus.Person->OnFacet, (p_person->WorldPos.X >> 8) + hx, (p_person->WorldPos.Z >> 8) + hz);

        my = find_cable_y_along(&dfacets[p_person->Genus.Person->OnFacet], along);
        MSG_add(" death slide along %d my %d\n", along, my);

        p_person->WorldPos.Y = (my - hy) << 8;
    } else {
        // This should never happen — person should always be on cable here.
        ASSERT(0);
    }
}

// ============================================================
// chunk 9: fn_person_dangling..fn_person_moveing (lines 12330-13977)
// ============================================================

// trickle_velocity_to: used in SUB_STATE_STOPPING_OT (active code path).
extern void trickle_velocity_to(Thing* p_thing, SWORD velocity);

// STATE_DANGLING state machine.
// Sub-states cover: ledge grab transition, static hang, zip-wire traversal (forward/backward),
// free-fall drop (with grab_ledge each frame), landing, and lateral traverse.
// uc_orig: fn_person_dangling (fallen/Source/Person.cpp)
void fn_person_dangling(Thing* p_person)
{
    SLONG ignore_building;
    SLONG end;
    SLONG hit;
    SLONG grab;
    SLONG wave_id1,
        wave_id2;

    switch (p_person->SubState) {
    case SUB_STATE_STOPPING:
        if (p_person->Genus.Person->OnFacet > 0 && (p_person->Genus.Person->Flags & FLAG_PERSON_ON_CABLE))
            p_person->Genus.Person->Action = ACTION_DANGLING_CABLE;

        break;

    case SUB_STATE_GRAB_TO_DANGLE:
        MSG_add("grab to dangle \n");
        end = person_normal_animate(p_person);
        if (end == 1) {
            MSG_add("anim end \n");
            p_person->Draw.Tweened->Locked = 0;

            if (p_person->Genus.Person->Flags & FLAG_PERSON_ON_CABLE)
            {

                set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
                /*

                                                                p_person->SubState=SUB_STATE_DANGLING_CABLE;
                                                                MSG_add(" its a cable grab face %d",p_person->Genus.Person->OnFacet);
                                                                p_person->Draw.Tweened->Angle=find_best_cable_angle(p_person,p_person->Genus.Person->OnFacet);

                                                                p_person->Genus.Person->Action=ACTION_DANGLING_CABLE;
                                                                locked_anim_change(p_person,SUB_OBJECT_LEFT_HAND,ANIM_HAND_OVER_HAND);
                */

            } else

            {
                p_person->SubState = SUB_STATE_DANGLING;
                p_person->Genus.Person->Action = ACTION_DANGLING;
                p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);
            }
        }
        break;
    case SUB_STATE_DEATH_SLIDE:
        end = person_normal_animate(p_person);
        if (end) {
            if (p_person->Draw.Tweened->CurrentAnim == ANIM_DEATH_SLIDE) {
                set_anim(p_person, ANIM_WIRE_SLIDE_PULLUP);
            } else if (p_person->Draw.Tweened->CurrentAnim == ANIM_WIRE_SLIDE_PULLUP) {
                set_anim(p_person, ANIM_WIRE_SLIDE_HANG);
            }
        }
        p_person->Velocity += p_person->DeltaVelocity;
        if (p_person->Velocity > 256)
            p_person->Velocity = 256;
        person_death_slide(p_person);
        break;

    case SUB_STATE_DANGLING:
        MSG_add(" dangling NORMAL ");

        /*

        if(Keys[KB_0])
                set_person_idle(p_person);

        */

        break;
    case SUB_STATE_DANGLING_CABLE:

        break;
    case SUB_STATE_DANGLING_CABLE_FORWARD:
        p_person->Draw.Tweened->Locked = 0;
        end = person_normal_animate(p_person);
        if (end == 1) {

            set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
            return;
        }
        do_person_on_cable(p_person);

        break;
    case SUB_STATE_DANGLING_CABLE_BACKWARD:
        end = person_backwards_animate(p_person);
        if (end == 1) {
            set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
            return;
        }
        do_person_on_cable(p_person);

        break;

    case SUB_STATE_DROP_DOWN_OFF_FACE:
    case SUB_STATE_DROP_DOWN:

        end = person_normal_animate(p_person);

        if (end == 1) {
            if (p_person->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_START) {
                locked_anim_change(p_person, SUB_OBJECT_PELVIS, ANIM_PLUNGE_FORWARDS);
            }
        }

        // Knock people over when you jump on them.
        drop_on_heads(p_person);

        {
            SLONG temp_angle;

            // If you slip off a face your velocity is not necessarily in the direction you're facing.
            // While slipping, AngleTo is the movement angle; drop-down continues with that.
            if (p_person->Genus.Person->Flags & FLAG_PERSON_MOVE_ANGLETO) {
                temp_angle = p_person->Draw.Tweened->Angle;
                p_person->Draw.Tweened->Angle = p_person->Draw.Tweened->AngleTo;
            }
            if (over_nogo(p_person))
                p_person->Velocity = 0;

            if (p_person->DY < -4000) {
                if (p_person->Velocity > 0) {
                    hit = projectile_move_thing(p_person, 3);
                } else {
                    hit = projectile_move_thing(p_person, 2 | 8 | 1);
                }
            } else {
                if (p_person->Velocity > 0) {
                    hit = projectile_move_thing(p_person, 1);
                } else {
                    hit = projectile_move_thing(p_person, 1 | 8);
                }
            }

            if (p_person->Genus.Person->Flags & FLAG_PERSON_MOVE_ANGLETO) {
                p_person->Draw.Tweened->Angle = temp_angle;

                if (hit) {
                    p_person->Genus.Person->Flags &= ~FLAG_PERSON_MOVE_ANGLETO;
                }
            }
        }

        if (hit == 100) {
            // projectile_move_thing killed the person.
            return;
        }

        if (p_person->SubState == SUB_STATE_DROP_DOWN_OFF_FACE) {
            // Don't grab faces when falling backwards off a face.
            grab = FALSE;
        } else {
            grab = grab_ledge(p_person);
        }

        if (!grab && hit == 1) {
            // projectile_move_thing sets person foot to floor height, so we need to
            // maintain foot pos in world across anims.
            if (p_person->DY < -15000) {
                locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, ANIM_BIG_LAND);
            } else {
                locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, ANIM_LAND_VERT);
            }

            p_person->SubState = SUB_STATE_DROP_DOWN_LAND;
            p_person->Genus.Person->Action = ACTION_LANDING;

            person_splash(p_person, -1);

            {
                wave_id1 = footstep_wave(p_person);
                wave_id2 = footstep_wave(p_person);

                MFX_play_thing(THING_NUMBER(p_person), wave_id1, MFX_REPLACE, p_person);
                MFX_play_thing(THING_NUMBER(p_person), wave_id2, MFX_REPLACE, p_person);
            }
        }

        break;

        /*

                        case	SUB_STATE_DROP_DOWN_OFF_FACE:
                                        {
                                                SLONG	temp_angle;

        //If you slip of a face then your velocity is not necessarily in the direction your facing
        //(while slipping angleto is your movement angle), and it continues with dropdown

                                                if(p_person->Genus.Person->Flags&FLAG_PERSON_MOVE_ANGLETO)
                                                {
                                                        temp_angle=p_person->Draw.Tweened->Angle;
                                                        p_person->Draw.Tweened->Angle=p_person->Draw.Tweened->AngleTo;
                                                }

                                                if(p_person->Velocity>0)
                                                        hit=projectile_move_thing(p_person,3);
                                                else
                                                        hit=projectile_move_thing(p_person,2); //2

                                                if(p_person->Genus.Person->Flags&FLAG_PERSON_MOVE_ANGLETO)
                                                {
                                                        p_person->Draw.Tweened->Angle=temp_angle;
                                                        if(hit)
                                                                p_person->Genus.Person->Flags&=~FLAG_PERSON_MOVE_ANGLETO;

                                                }
                                        }
                                        drop_on_heads(p_person);
                                        if(hit==100)
                                        {
                                                //
                                                // dead
                                                //

                                                return;
                                        }
                                        if(hit==1)
                                        {
        //				MSG_add(" hit something while falling ");
                                                if (p_person->DY < -15000)
                                                {
                                                        locked_anim_change(p_person,SUB_OBJECT_LEFT_FOOT,ANIM_BIG_LAND);
                                                } else {
                                                        locked_anim_change(p_person,SUB_OBJECT_LEFT_FOOT,ANIM_LAND_VERT);
                                                }
                                                p_person->SubState		=	SUB_STATE_DROP_DOWN_LAND;
                                                p_person->Genus.Person->Action		=	ACTION_LANDING;

                                                person_splash(p_person, -1);

                                                {
                                                        wave_id1	=	footstep_wave(p_person);
                                                        wave_id2	=	footstep_wave(p_person);
                                                        MFX_play_thing(THING_NUMBER(p_person),wave_id1,MFX_REPLACE,p_person);
                                                        MFX_play_thing(THING_NUMBER(p_person),wave_id2,MFX_REPLACE,p_person);
                                                }
                                        }

                                break;

                        case	SUB_STATE_DROP_DOWN:
                                        person_normal_animate(p_person);
                                        {
                                                SLONG	temp_angle;
                                                if(p_person->Genus.Person->Flags&FLAG_PERSON_MOVE_ANGLETO)
                                                {
                                                        temp_angle=p_person->Draw.Tweened->Angle;
                                                        p_person->Draw.Tweened->Angle=p_person->Draw.Tweened->AngleTo;
                                                }
                                                if(p_person->Velocity>0)
                                                        hit=projectile_move_thing(p_person,3);
                                                else
                                                        hit=projectile_move_thing(p_person,2);
                                                if(p_person->Genus.Person->Flags&FLAG_PERSON_MOVE_ANGLETO)
                                                {
                                                        p_person->Draw.Tweened->Angle=temp_angle;
                                                        if(hit)
                                                                p_person->Genus.Person->Flags&=~FLAG_PERSON_MOVE_ANGLETO;
                                                }
                                        }
                                        drop_on_heads(p_person);
                                        if(hit==100)
                                        {
                                                // dead
                                                return;
                                        }
                                        grab=grab_ledge(p_person);
                                        if(grab)
                                                MSG_add(" running jump grabbed \n");
                                        if(!grab)
                                        if(hit==1)
                                        {
                                                if (p_person->DY < -15000)
                                                {
                                                        locked_anim_change(p_person,SUB_OBJECT_LEFT_FOOT,ANIM_BIG_LAND);
                                                } else {
                                                        locked_anim_change(p_person,SUB_OBJECT_LEFT_FOOT,ANIM_LAND_VERT);
                                                }
                                                p_person->SubState		=	SUB_STATE_DROP_DOWN_LAND;
                                                p_person->Genus.Person->Action		=	ACTION_LANDING;
                                                person_splash(p_person, -1);
                                                {
                                                        wave_id1	=	footstep_wave(p_person);
                                                        wave_id2	=	footstep_wave(p_person);
                                                        MFX_play_thing(THING_NUMBER(p_person),wave_id1,MFX_REPLACE,p_person);
                                                        MFX_play_thing(THING_NUMBER(p_person),wave_id2,MFX_REPLACE,p_person);
                                                }
                                        }
                                break;
        */

    case SUB_STATE_DROP_DOWN_LAND:
        change_velocity_to(p_person, 0);
        if (over_nogo(p_person))
            p_person->Velocity = 0;
        person_normal_move(p_person);
        end = person_normal_animate(p_person);
        if (end == 1) {
            set_person_idle(p_person);
            // plant_feet only here — this is the unique landing call site noted in KB.
            if (p_person->SubState != SUB_STATE_RUNNING)
                plant_feet(p_person);
        }
        break;
    case SUB_STATE_TRAVERSE_LEFT:
    case SUB_STATE_TRAVERSE_RIGHT:
        end = person_normal_animate(p_person);
        if (end) {
            SLONG dist, dx, dz;
            locked_anim_change(p_person, SUB_OBJECT_PELVIS, ANIM_DANGLE);

            p_person->SubState = SUB_STATE_DANGLING;
            p_person->Genus.Person->Action = ACTION_DANGLING;
            /*
                                            if((dist=abs(check_near_facet(p_person,64,64,(p_person->WorldPos.X)>>8,(p_person->WorldPos.Z)>>8)))==0)
                                            {
                                                    return;
                                            }

                                            if(abs(32-dist)<32)
                                            {
                                                    SLONG	angle;
                                                    angle=(p_person->Draw.Tweened->Angle+1024)&2047;

                                                    dx = -(SIN(angle) * (32-dist)) >> 8;
                                                    dz = -(COS(angle) * (32-dist)) >> 8;

                                            }

                                            person_normal_move_dxdz(p_person,dx,dz);
            */
        }

        break;
    case SUB_STATE_PULL_UP:

        end = person_normal_animate(p_person);

        if (MagicFrameCheck(p_person, 2))
            EffortSound(p_person);

        if (end == 1) {
            // Finished pulling ourselves up.
            SLONG face, new_y;
            p_person->Draw.Tweened->AnimTween = 0;
            locked_anim_change(p_person, SUB_OBJECT_LEFT_FOOT, ANIM_STAND_READY);

            if (p_person->Flags & FLAGS_IN_BUILDING) {
                ignore_building = INDOORS_DBUILDING;
            } else {
                ignore_building = NULL;
            }

            // Find what face we are standing on now.
            if (p_person->Flags & FLAGS_IN_SEWERS) {
                p_person->OnFace = 0;
                p_person->WorldPos.Y = NS_calc_height_at(p_person->WorldPos.X >> 8, p_person->WorldPos.Z >> 8) << 8;
            } else {
                face = find_face_for_this_pos(p_person->WorldPos.X >> 8, p_person->WorldPos.Y >> 8, p_person->WorldPos.Z >> 8, &new_y, ignore_building, 0);

                if (face == GRAB_FLOOR) {
                    ASSERT(0);

                    p_person->OnFace = 0;
                    p_person->WorldPos.Y = new_y << 8;
                } else if (face) {
                    p_person->OnFace = face;
                    p_person->WorldPos.Y = new_y << 8;
                } else {
                    ASSERT(0);

                    p_person->WorldPos.Y = 0;
                }
            }

            // This is a subset of set_person_idle (the set_anim stuff is removed).
            set_person_idle(p_person);

            /*
            set_generic_person_state_function(p_person,STATE_IDLE);
            p_person->SubState=0;
            p_person->Genus.Person->Action=ACTION_IDLE;
            p_person->Genus.Person->Flags&=~(FLAG_PERSON_NON_INT_M|FLAG_PERSON_NON_INT_C);
            */
        }
        break;

    default:
        set_person_drop_down(p_person, 0); // EMERGENCY!
        break;
    }
}

// Transitions person into a stopping animation after running.
// leg=0 left foot, leg=1 right foot (animation index offset).
// uc_orig: set_person_running_stop (fallen/Source/Person.cpp)
void set_person_running_stop(Thing* p_person, SLONG leg)
{
    // tween_to_anim(p_person,ANIM_STOP_RUN_L+leg);
    p_person->SubState = SUB_STATE_STOPPING;
}

// Returns TRUE if a player running into this facet should automatically begin climbing it.
// Only checks climbable fence types; checks that the player is roughly facing the fence.
// Wire-cutters in hand suppresses auto-climb (player may want to cut the fence instead).
// uc_orig: should_person_automatically_land_on_fence (fallen/Source/Person.cpp)
SLONG should_person_automatically_land_on_fence(Thing* p_person, SLONG facet)
{
    SLONG dx;
    SLONG dz;
    SLONG da;
    SLONG mdx;
    SLONG mdz;
    SLONG angle;

    DFacet* df;

    df = &dfacets[facet];

    // Only players auto-climb.
    if (p_person->Genus.Person->PlayerID == 0) {
        return FALSE;
    }

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        if (p_special->Genus.Special->SpecialType == SPECIAL_WIRE_CUTTER) {
            // While holding wire cutters don't auto climb fences.
            return (FALSE);
        }
    }

    // Is this a climbable fence facet?
    if (df->FacetType == STOREY_TYPE_FENCE || df->FacetType == STOREY_TYPE_NORMAL || df->FacetType == STOREY_TYPE_FENCE_FLAT || df->FacetType == STOREY_TYPE_FENCE_BRICK) {
        if (!(df->FacetFlags & FACET_FLAG_UNCLIMBABLE)) {
            // Is the person's angle correct?
            dx = abs(df->x[1] - df->x[0] << 8);
            dz = abs(df->z[1] - df->z[0] << 8);

            angle = Arctan(-dx, dz) + 1536;
            angle &= 2047;

            da = abs(angle - p_person->Draw.Tweened->Angle);

// uc_orig: FENCE_DA (fallen/Source/Person.cpp)
#define FENCE_DA 128

            if ((da > FENCE_DA && da < 1024 - FENCE_DA) || (da > FENCE_DA + 1024 && da < 2048 - FENCE_DA)) {
                return FALSE;
            } else {
                return TRUE;
            }
        }
    }

    /*

    if (da > 512 && da < 2048 - 512)
    {
            angle += 1024;
            angle &= 2047;
    }

    */

    return FALSE;
}

#undef FENCE_DA

// Advances vault animation; at frame 6 checks whether person has overshot the fence
// and drops them into free-fall if so. On completion resumes running or idles.
// uc_orig: process_a_vaulting_person (fallen/Source/Person.cpp)
void process_a_vaulting_person(Thing* p_person)
{
    SLONG end;

    end = person_normal_animate(p_person);

    if (p_person->Draw.Tweened->FrameIndex == 6) {
        SLONG wx, wy, wz, fy;

        calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_FOOT, &wx, &wy, &wz);

        wx += p_person->WorldPos.X >> 8;
        wy += p_person->WorldPos.Y >> 8;
        wz += p_person->WorldPos.Z >> 8;

        fy = PAP_calc_height_at(wx, wz);

        if (abs(fy - (wy - 12)) > 8) {
            p_person->Velocity = 6;
            p_person->DY = -(10 << 8);
            set_person_drop_down(p_person, PERSON_DROP_DOWN_KEEP_VEL | PERSON_DROP_DOWN_KEEP_DY);
        }
    }

    if (end == 1) {
        p_person->Genus.Person->Flags &= ~(FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C);

        locked_anim_change(p_person, 0, ANIM_STAND_READY);

        // Only non-player characters can continue running.
        if (p_person->Genus.Person->PlayerID && continue_moveing(p_person)) {
            set_person_running(p_person);
        } else {
            set_person_idle(p_person);
        }
    }
}

// Puts person into sitting-on-bench state (MOVEING/SIMPLE_ANIM with SIT_DOWN animation).
// Bench health regen is disabled for other persons while one person is sitting.
// uc_orig: set_person_sit_down (fallen/Source/Person.cpp)
void set_person_sit_down(Thing* p_person)
{
    set_generic_person_state_function(p_person, STATE_MOVEING);

    set_person_do_a_simple_anim(p_person, ANIM_SIT_DOWN);

    p_person->SubState = SUB_STATE_SIMPLE_ANIM;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
    p_person->Genus.Person->Action = ACTION_SIT_BENCH;

    if (GAME_FLAGS & GF_DISABLE_BENCH_HEALTH) {
        p_person->Genus.Person->Timer1 = 100;
    } else {
        p_person->Genus.Person->Timer1 = 0;
        GAME_FLAGS |= GF_DISABLE_BENCH_HEALTH;
    }
}

// Transitions person out of sitting state (sit-to-stand animation).
// uc_orig: set_person_unsit (fallen/Source/Person.cpp)
void set_person_unsit(Thing* p_person)
{
    set_generic_person_state_function(p_person, STATE_MOVEING);

    set_person_do_a_simple_anim(p_person, ANIM_SIT_TO_STAND);

    p_person->SubState = SUB_STATE_SIMPLE_ANIM;
    p_person->Genus.Person->Flags &= ~FLAG_PERSON_NO_RETURN_TO_NORMAL;
    p_person->Genus.Person->Action = ACTION_UNSIT;
}

// Returns non-zero if person is holding a two-handed weapon (shotgun or AK47).
// Note: baseball bat check is commented out in the original — kept 1:1.
// uc_orig: person_holding_2handed (fallen/Source/Person.cpp)
SLONG person_holding_2handed(Thing* p_person)
{
    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        // The shotgun and AK47 have their own special two-handed yomp animation.
        if (p_special->Genus.Special->SpecialType == SPECIAL_SHOTGUN ||
            //			p_special->Genus.Special->SpecialType == SPECIAL_BASEBALLBAT   ||
            p_special->Genus.Special->SpecialType == SPECIAL_AK47) {
            return (1);
        }
    }
    return (0);
}

// Returns non-zero if person's active special item matches the given special type.
// uc_orig: person_holding_special (fallen/Source/Person.cpp)
SLONG person_holding_special(Thing* p_person, UBYTE special)
{
    if (!p_person->Genus.Person->SpecialUse)
        return 0;

    Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

    return (p_special->Genus.Special->SpecialType == special) ? 1 : 0;
}

// Returns non-zero if person is holding a baseball bat.
// uc_orig: person_holding_bat (fallen/Source/Person.cpp)
SLONG person_holding_bat(Thing* p_person)
{
    return person_holding_special(p_person, SPECIAL_BASEBALLBAT);
}

// Returns the running animation index appropriate for the person's current weapon state.
// Checks: gun out → pistol jog; baseball bat → bat yomp; shotgun/AK47 → AK jog;
// cop/roper → roper run; otherwise default yomp.
// uc_orig: get_yomp_anim (fallen/Source/Person.cpp)
SLONG get_yomp_anim(Thing* p_person)
{
    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)
        return (ANIM_PISTOL_JOG);

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        // The shotgun and AK47 have their own special shotgun yomp.
        if (p_special->Genus.Special->SpecialType == SPECIAL_BASEBALLBAT) {
            return (ANIM_YOMP_BAT);
        }

        if (p_special->Genus.Special->SpecialType == SPECIAL_SHOTGUN || p_special->Genus.Special->SpecialType == SPECIAL_AK47) {
            return (ANIM_AK_JOG);
        }
    }
    if (p_person->Genus.Person->PersonType == PERSON_COP) {
        return (COP_ROPER_ANIM_RUN);
    } else
        return (ANIM_YOMP);
}

// STATE_MOVEING state machine: handles all movement sub-states.
// Key sub-states: RUNNING, WALKING, SNEAKING, CRAWLING, RUNNING_THEN_JUMP, RUNNING_SKID_STOP,
// RUNNING_VAULT, ENTERING_VEHICLE, INSIDE_VEHICLE, EXITING_VEHICLE, SLIPPING, FLIPING,
// MOUNTING_BIKE, RIDING_BIKE, DISMOUNTING_BIKE, SIMPLE_ANIM, SIMPLE_ANIM_OVER.
// Footstep sounds fire on FrameIndex==1 (right) and ==5 (left) during running/walking/sneaking.
// uc_orig: fn_person_moveing (fallen/Source/Person.cpp)
void fn_person_moveing(Thing* p_person)
{
    UBYTE last_frame;
    SLONG end,
        foot_step_wave;
    SLONG yomp, sprint, change, stamina_used;

    MSG_add(" state %d substate %d vel %d \n", p_person->State, p_person->SubState, p_person->Velocity);

    if (p_person->Genus.Person->PlayerID) {
        camera_normal();
    }

    if (p_person->SubState == SUB_STATE_RUNNING || p_person->SubState == SUB_STATE_WALKING) {
        switch (p_person->Draw.Tweened->CurrentAnim) {
        case ANIM_STAND_HIP:
        case ANIM_STAND_READY:
        case ANIM_SHOTGUN_IDLE:
        case COP_ROPER_ANIM_READY:
        case ANIM_FIGHT: {
            SLONG anim;
            anim = get_yomp_anim(p_person);
            if (anim == COP_ROPER_ANIM_RUN) {
                set_anim_of_type(p_person, anim, ANIM_TYPE_ROPER);
            } else
                set_anim(p_person, anim);
        } break;
        }
    }

    if (p_person->SubState == SUB_STATE_RUNNING || p_person->SubState == SUB_STATE_WALKING || p_person->SubState == SUB_STATE_SNEAKING || p_person->SubState == SUB_STATE_RUNNING_SKID_STOP || p_person->SubState == SUB_STATE_RUNNING_THEN_JUMP) {
        if (p_person->Draw.Tweened->FrameIndex == 1) {
            person_splash(p_person, SUB_OBJECT_RIGHT_FOOT);
        } else if (p_person->Draw.Tweened->FrameIndex == 5) {
            person_splash(p_person, SUB_OBJECT_LEFT_FOOT);
        }
    }
    SlideSoundCheck(p_person);
    switch (p_person->SubState) {
    case SUB_STATE_ENTERING_VEHICLE:
        end = person_normal_animate(p_person);
        if (end) {
            set_person_in_vehicle(p_person, TO_THING(p_person->Genus.Person->InCar));
        }
        break;

    case SUB_STATE_INSIDE_VEHICLE:

        // We are not on the mapwho at this point — just track the position of
        // the vehicle we are in and teleport to that place.
        p_person->WorldPos = TO_THING(p_person->Genus.Person->InCar)->WorldPos;
        p_person->Draw.Tweened->Angle = TO_THING(p_person->Genus.Person->InCar)->Genus.Vehicle->Angle;

        // Vehicles can drive off the map!
        SATURATE(p_person->WorldPos.X, 0, (PAP_SIZE_HI << 16) - 1);
        SATURATE(p_person->WorldPos.Z, 0, (PAP_SIZE_HI << 16) - 1);

        break;

    case SUB_STATE_EXITING_VEHICLE:
        end = person_normal_animate(p_person);
        if (end) {
            set_person_out_of_vehicle(p_person);
        }
        break;

    case SUB_STATE_SLIPPING_END:
        change_velocity_to(p_person, 0);
        if (p_person->Velocity == 0) {
            set_person_idle(p_person);
            return;
        }

    case SUB_STATE_SLIPPING: {
        SLONG angle;
        angle = p_person->Draw.Tweened->Angle;
        p_person->Draw.Tweened->Angle = p_person->Draw.Tweened->AngleTo;

        person_normal_move(p_person);

        p_person->Draw.Tweened->Angle = angle;
    }

        {
            SLONG fx, fy, fz, px, pz, rgb, sz, i, j;
            calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween,
                SUB_OBJECT_LEFT_FOOT, &fx, &fy, &fz);
            fx = (fx << 8) + p_person->WorldPos.X;
            fy = (fy << 8) + p_person->WorldPos.Y;
            fz = (fz << 8) + p_person->WorldPos.Z;
            rgb = 0x7F - (p_person->Velocity << 1);
            if (rgb < 0) {
                rgb = 0;
            }
            rgb = (rgb << 24) | 0xc9b7a3;
            j = (p_person->Velocity > 40) ? 7 : 3;
            for (i = 0; i < j; i++) {
                px = fx + (((Random() & 0x7ff) - 0x3ff) << 2);
                pz = fz + (((Random() & 0x7ff) - 0x3ff) << 2);
                PARTICLE_Add(px, fy, pz, (Random() & 7) - 3, 20, (Random() & 7) - 3, POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), rgb, PFLAG_FADE | PFLAG_RESIZE, 40, 25, 1, 10, 4);
            }
        }

        break;

        break;

    case SUB_STATE_RUNNING_HALF_BLOCK:
        end = person_normal_animate(p_person);
        if (end) {
            locked_anim_change(p_person, 0, ANIM_STAND_READY);
            plant_feet(p_person);
            if (p_person->State != STATE_DANGLING)
                set_person_idle(p_person);
        }
        break;
    case SUB_STATE_RUNNING_VAULT:
        process_a_vaulting_person(p_person);
        break;

    case SUB_STATE_RUNNING_HIT_WALL:
        end = person_normal_animate(p_person);
        if (end) {
            set_person_idle(p_person);
        }
        break;
    case SUB_STATE_RUNNING:

        last_frame = p_person->Draw.Tweened->FrameIndex;
        last_slide_colvect = 0;

        if (!(p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING))
            if (p_person->Genus.Person->PlayerID)
            {
                if (p_person->Genus.Person->Timer1) {
                    SLONG ticks = 16 * TICK_RATIO >> TICK_SHIFT;

                    // Countdown since the person last fired his gun.
                    if (p_person->Genus.Person->Timer1 <= ticks) {
                        p_person->Genus.Person->Timer1 = 0;

                        if (p_person->Genus.Person->SpecialUse) {
                            Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

                            if (p_special->Genus.Special->SpecialType == SPECIAL_AK47) {
                                if (continue_firing(p_person)) {
                                    set_person_running_shoot(p_person);
                                }
                            }
                        }
                    } else {
                        p_person->Genus.Person->Timer1 -= ticks;
                    }
                }

                if (person_has_gun_out(p_person)) {
                    // We are jogging along with a shotgun or AK47 or pistol — aim at people.
                    player_running_aim_gun(p_person);
                }
            }
        end = person_normal_animate(p_person);

        p_person->DY = 0;

        if (end == 1) {
            switch (p_person->Draw.Tweened->CurrentAnim) {
            case ANIM_HIT_WALL:
            case ANIM_YOMP_START:
            case ANIM_YOMP_START_PISTOL:
            case ANIM_YOMP_START_AK:
            case ANIM_YOMP_START_BAT:

            {
                UWORD anim;

                anim = get_yomp_anim(p_person);
                if (anim == COP_ROPER_ANIM_RUN) {
                    set_anim_of_type(p_person, anim, ANIM_TYPE_ROPER);
                } else
                    set_anim(p_person, anim);
            }

            break;
            case ANIM_START_WALK_CARRY: {
                Thing* p_target;
                p_target = TO_THING(p_person->Genus.Person->Target);
                set_anim(p_target, ANIM_WALK_CARRY_V);
                set_anim(p_person, ANIM_WALK_CARRY);
            }
            }
        }

        /*
                                        switch(p_person->Genus.Person->AnimType)
                                        {
                                                case ANIM_TYPE_ROPER:
                                                        sprint=60;
                                                        yomp=34;
                                                        change=40; // immediately :}
                                                        stamina_used=4; // since roper runs all the time...
                                                        break;
                                                default:	// darci, others
                                                        sprint=sprint_speed;
                                                        yomp=yomp_speed;
                                                        change=50;
                                                        stamina_used=2;
                                                        break;
                                        }
        */

        sprint = sprint_speed;
        yomp = yomp_speed;
        change = 50;
        stamina_used = ((PTIME(p_person) & 0x3) == 0);

        if (p_person->Genus.Person->AnimType == ANIM_TYPE_ROPER) {
            stamina_used = 4;
        }

        {
            SLONG run_speed = yomp;
            if (p_person->Genus.Person->Mode == PERSON_MODE_SPRINT) {
                run_speed = sprint;

                if (p_person->Genus.Person->Stamina < 10) {
                    p_person->Genus.Person->Stamina = 0;
                    p_person->Genus.Person->Mode = PERSON_MODE_RUN;
                    switch (p_person->Genus.Person->PersonType) {
                    case PERSON_DARCI:
                        MFX_play_thing(THING_NUMBER(p_person), S_DARCI_OUTOFBREATH, MFX_MOVING, p_person);
                        break;
                    case PERSON_ROPER:
                        MFX_play_thing(THING_NUMBER(p_person), S_ROPER_OUTOFBREATH, MFX_MOVING, p_person);
                        break;
                    }
                } else {
                    p_person->Genus.Person->Stamina -= stamina_used;
                }

                if (p_person->Draw.Tweened->CurrentAnim == ANIM_YOMP) {
                    if (end || (change == 1))
                        if (p_person->Velocity > change)
                            set_anim(p_person, ANIM_RUN);
                }

                // Accelerate twice in sprint mode.
                change_velocity_to(p_person, run_speed);

            } else {
                if (p_person->Draw.Tweened->CurrentAnim == ANIM_RUN) {
                    if (end)
                        if (p_person->Velocity < change - 5)
                            set_anim(p_person, ANIM_YOMP);
                }
            }

            change_velocity_to(p_person, run_speed);
            extern UBYTE cheat;
            // if(cheat==1)
            //     change_velocity_to(p_person,run_speed*2);
        }

        if (p_person->Draw.Tweened->CurrentAnim == ANIM_YOMP_START || p_person->Draw.Tweened->CurrentAnim == ANIM_YOMP_START_AK || p_person->Draw.Tweened->CurrentAnim == ANIM_YOMP_START_PISTOL || p_person->Draw.Tweened->CurrentAnim == ANIM_YOMP_START_BAT) {
            if (p_person->Draw.Tweened->FrameIndex < 2)
                p_person->Velocity = 0;
        }

        last_slide_colvect = 0;
        person_normal_move(p_person);

        if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
            carry_running(p_person);
        } else {
            if (last_slide_colvect && along_middle_of_facet(p_person, last_slide_colvect)) {
                if (is_facet_vaultable(last_slide_colvect)) {
                    if (set_person_vault(p_person, last_slide_colvect)) {
                        return;
                    }
                }
                if (is_facet_half_step(last_slide_colvect)) {
                    if (set_person_climb_half(p_person, last_slide_colvect)) {
                        return;
                    }
                }

                if (should_person_automatically_land_on_fence(p_person, last_slide_colvect)) {
                    set_person_land_on_fence(p_person, last_slide_colvect, 1, 1);
                    return;
                }

                // #define WE_WANT_TO_TURN_AND_PUT_OUR_BACK_TO_THE_WALL	77179
                if (0)
                    if (p_person->Velocity > 35) {
                        SLONG wall_angle;
                        if (is_wall_good_for_bump_and_turn(p_person, last_slide_colvect))
                            if (am_i_facing_wall(p_person, last_slide_colvect, &wall_angle)) {
                                p_person->Velocity = 0;
                                ASSERT((wall_angle & 0x1ff) == 0);
                                p_person->Draw.Tweened->Angle = (wall_angle) & 2047;

                                set_anim(p_person, ANIM_HIT_WALL);
                                p_person->SubState = SUB_STATE_RUNNING_HIT_WALL;
                                if (p_person->Genus.Person->PersonType == PERSON_DARCI)
                                    MFX_play_thing(THING_NUMBER(p_person), SOUND_Range(S_DARCI_EFFORT_START, S_DARCI_EFFORT_END), 0, p_person);
                                return;
                            }
                    }
            }

            if (p_person->Genus.Person->PlayerID)
                if (slide_ladder) {
                    if (mount_ladder(p_person, slide_ladder))
                        return;
                }
        }
        if (p_person->Draw.Tweened->FrameIndex == 1 || p_person->Draw.Tweened->FrameIndex == (4 + (p_person->Genus.Person->Mode == PERSON_MODE_RUN))) {
            if (p_person->Flags & FLAGS_PLAYED_FOOTSTEP) {
                // Don't play twice!
            } else {
                foot_step_wave = footstep_wave(p_person);

                /*

                oscilate_tinpanum(
                        p_person->WorldPos.X >> 8,
                        p_person->WorldPos.Y >> 8,
                        p_person->WorldPos.Z >> 8,
                        p_person,
                        64);

                */

                PCOM_oscillate_tympanum(
                    PCOM_SOUND_FOOTSTEP,
                    p_person,
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y >> 8,
                    p_person->WorldPos.Z >> 8);

                /*
                if(GAME_FLAGS&GF_SEWERS && p_person->Draw.Tweened->FrameIndex!=last_frame)
                {
                        foot_step_wave	=	((Random()*(S_FOOTS_SEWER_END-S_FOOTS_SEWER_START))>>16)+S_FOOTS_SEWER_START;;
                        play_quick_wave_old(&foot_step,foot_step_wave,0,0);
                }
                else
                {
                        play_quick_wave_old(&foot_step,foot_step_wave,0,0);
                }
                */
                MFX_play_thing(THING_NUMBER(p_person), foot_step_wave, MFX_REPLACE, p_person);

                p_person->Flags |= FLAGS_PLAYED_FOOTSTEP;
            }
        } else {
            p_person->Flags &= ~FLAGS_PLAYED_FOOTSTEP;
        }

        break;
    case SUB_STATE_CRAWLING:
        if (slope_ahead(p_person, 100))
            return;

        /*
        switch(p_person->Genus.Person->AnimType)
        {
                default:	// darci, others
        */
        change_velocity_to(p_person, 15);
        /*
                        break;
        }
        */
        person_normal_move(p_person);
        person_normal_animate(p_person);
        break;
    case SUB_STATE_WALKING:

        if (p_person->Genus.Person->PersonType == PERSON_CIV) {
            SLONG old;
            if (!(p_person->Draw.Tweened->MeshID & 1))
                change_velocity_to(p_person, 14);
            else
                change_velocity_to(p_person, 10); // women walk slower
        } else
            change_velocity_to(p_person, 16);
        person_normal_move(p_person);

        // Walking people can vault fences too.
        if (last_slide_colvect && is_facet_vaultable(last_slide_colvect)) {
            set_person_vault(p_person, last_slide_colvect);
            return;
        }

        if (p_person->Genus.Person->PlayerID)
            if (slide_ladder) {
                if (mount_ladder(p_person, slide_ladder))
                    return;
            }

        person_normal_animate(p_person);
        if (p_person->Draw.Tweened->FrameIndex == 1 || p_person->Draw.Tweened->FrameIndex == 5) {
            if (p_person->Flags & FLAGS_PLAYED_FOOTSTEP) {
                // Don't play twice!
            } else {
                foot_step_wave = footstep_wave(p_person);

                /* //nah were only walking
                                                                PCOM_oscillate_tympanum(
                                                                        PCOM_SOUND_FOOTSTEP,
                                                                        p_person,
                                                                        p_person->WorldPos.X >> 8,
                                                                        p_person->WorldPos.Y >> 8,
                                                                        p_person->WorldPos.Z >> 8);
                */

                MFX_play_thing(THING_NUMBER(p_person), foot_step_wave, MFX_REPLACE, p_person);

                p_person->Flags |= FLAGS_PLAYED_FOOTSTEP;
            }
        } else {
            p_person->Flags &= ~FLAGS_PLAYED_FOOTSTEP;
        }

        break;
    case SUB_STATE_WALKING_BACKWARDS:
        change_velocity_to(p_person, -16);
        person_normal_move(p_person);
        person_normal_animate(p_person);
        break;
        /*
                        case	SUB_STATE_SNEAKING:
                                        change_velocity_to(p_person,16);
                                        person_normal_move(p_person);
                                        person_normal_animate(p_person);
                                break;
        */
    case SUB_STATE_RUN_STOP:
        person_normal_move(p_person);
        end = person_normal_animate(p_person);
        change_velocity_to(p_person, 0);

        if (end) {
            p_person->SubState = SUB_STATE_STOPPING;
        }
        break;
    case SUB_STATE_RUNNING_SKID_STOP:
        person_normal_move(p_person);
        if (check_on_slippy_slope(p_person))
            return;
        if (slope_ahead(p_person, 50)) {
            return;
        }

        end = person_normal_animate(p_person);
        switch (p_person->Draw.Tweened->CurrentAnim) {
        case ANIM_SLIDER_START:
            if (p_person->Velocity > 42)
                p_person->Velocity = 42;
            if (end) {
                set_anim(p_person, ANIM_SLIDER_HOLD);
            }
            break;
        case ANIM_SLIDER_HOLD:

        {
            SLONG fx, fy, fz, px, pz, rgb, sz, i, j;
            calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween,
                SUB_OBJECT_LEFT_FOOT, &fx, &fy, &fz);
            fx = (fx << 8) + p_person->WorldPos.X;
            fy = (fy << 8) + p_person->WorldPos.Y;
            fz = (fz << 8) + p_person->WorldPos.Z;
            rgb = 0xFF - (p_person->Velocity << 2);
            if (rgb < 0)
                rgb = 0;
            rgb = (rgb << 24) | 0xc9b7a3;
            j = (p_person->Velocity > 40) ? 7 : 3;
            for (i = 0; i < j; i++) {
                px = fx + (((Random() & 0x7ff) - 0x3ff) << 2);
                pz = fz + (((Random() & 0x7ff) - 0x3ff) << 2);
                PARTICLE_Add(px, fy, pz, (Random() & 7) - 3, 20, (Random() & 7) - 3, POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), rgb, PFLAG_FADE | PFLAG_RESIZE | PFLAG_SPRITEANI | PFLAG_SPRITELOOP, 40, 25, 1, 10, 4);
            }
        }
            change_velocity_to_slow(p_person, 0);
            if (p_person->Velocity < 25) {
                set_anim(p_person, ANIM_SLIDER_END);
            }
            break;
        case ANIM_SLIDER_END:
            MFX_stop(THING_NUMBER(p_person), S_SLIDE_START);
            if (p_person->Velocity > 5) {
                SLONG px, py, pz, rgb, sz;
                calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween,
                    SUB_OBJECT_LEFT_FOOT, &px, &py, &pz);
                px = (px << 8) + p_person->WorldPos.X;
                py = (py << 8) + p_person->WorldPos.Y;
                pz = (pz << 8) + p_person->WorldPos.Z;
                rgb = 0xFF - (p_person->Velocity << 2);
                if (rgb < 0)
                    rgb = 0;
                rgb = (rgb << 24) | 0xc9b7a3;
                PARTICLE_Add(px, py, pz, (Random() & 7) - 3, 20, (Random() & 7) - 3, POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), rgb, PFLAG_FADE | PFLAG_RESIZE | PFLAG_SPRITEANI | PFLAG_SPRITELOOP, 40, 25, 1, 10, 4);
            }
            change_velocity_to(p_person, 0);
            if (end) {
                set_person_locked_idle_ready(p_person);
                p_person->Velocity = 1;

                person_normal_move(p_person);
            }
            break;
        }

        /*
                                        if (WITHIN(p_person->Draw.Tweened->FrameIndex, 0, 0))
                                        {
                                                set_person_running_stop(p_person,0);
                                                p_person->SubState=SUB_STATE_STOPPING_OT;
                                        }
                                        else
                                        if (WITHIN(p_person->Draw.Tweened->FrameIndex, 3, 3))
                                        {
                                                // jump frame found
                                                set_person_running_stop(p_person,1);
                                        }
        */

        break;

    case SUB_STATE_RUNNING_THEN_JUMP:

        if (check_on_slippy_slope(p_person))
            return;
        change_velocity_to(p_person, 32);
        person_normal_move(p_person);
        person_normal_animate(p_person);

        // Put some leeway in the jump frames so she jumps more responsively.
        if (WITHIN(p_person->Draw.Tweened->FrameIndex, 0, 1)) {
            set_person_running_jump_lr(p_person, 0);
        } else if (WITHIN(p_person->Draw.Tweened->FrameIndex, 3, 4)) {
            // jump frame found
            set_person_running_jump_lr(p_person, 1);
        }

        /*
        if (WITHIN(p_person->Draw.Tweened->FrameIndex, 3, 3))
        {
                set_person_running_jump_lr(p_person,2);
        }
        else
        if (WITHIN(p_person->Draw.Tweened->FrameIndex, 7, 7))
        {
                // jump frame found
                set_person_running_jump_lr(p_person,3);
        }
        */

        break;

    case SUB_STATE_STOPPING:
    case SUB_STATE_STOPPING_DEAD:
        if (check_on_slippy_slope(p_person))
            return;
        end = person_normal_animate(p_person);
        change_velocity_to(p_person, 0);

        if (p_person->Velocity < 5) {
            set_person_idle(p_person);
            plant_feet(p_person);
        }

        break;
        // Duplicate break — bug in original, kept 1:1.
        break;

    case SUB_STATE_STOPPING_OT:
        end = person_normal_animate(p_person);

        if ((p_person->Draw.Tweened->FrameIndex < 2))
            person_normal_move(p_person);
        else if ((p_person->Draw.Tweened->FrameIndex == 2)) {
            ASSERT(0);
            if (p_person->SubState == SUB_STATE_STOPPING_OT)
                p_person->Draw.Tweened->Locked = SUB_OBJECT_RIGHT_FOOT;
            else
                p_person->Draw.Tweened->Locked = SUB_OBJECT_LEFT_FOOT;
            p_person->Velocity = 0;
        }
        /*
                                MSG_add("stopping vel %d",p_person->Velocity);
                                        if( (p_person->Draw.Tweened->FrameIndex==0))
                                                end=person_normal_animate(p_person);
                                        else
                                        if( (p_person->Draw.Tweened->FrameIndex==1))
                                                end=person_normal_animate_tween(p_person,64);
                                        else
                                                end=person_normal_animate(p_person);

                                        if( (p_person->Draw.Tweened->FrameIndex<2))
                                                trickle_velocity_to(p_person,0);
                                        else
                                                change_velocity_to(p_person,0);

                                                person_normal_move(p_person);

                                        if(end==1)
                                                p_person->Velocity=0;
                                        else
                                                trickle_velocity_to(p_person,0);

        */
        if (end)
        {
            set_person_locked_idle_ready(p_person);
            plant_feet(p_person);
            p_person->Draw.Tweened->Locked = 0;
        }
        break;
    case SUB_STATE_HOP_BACK:

        // change_velocity_to(p_person,-10);
        // person_normal_move(p_person);
        // end = person_backwards_animate(p_person);

        end = person_normal_animate(p_person);
        person_normal_animate(p_person);
        if (end == 1 && p_person->SubState == SUB_STATE_HOP_BACK)
            set_person_locked_idle_ready(p_person);

        break;
    case SUB_STATE_STEP_LEFT:
        end = person_normal_animate(p_person);

        if (end == 1) {
            set_person_locked_idle_ready(p_person);
        }

        break;
    case SUB_STATE_STEP_RIGHT:
        end = person_normal_animate(p_person);
        if (end == 1) {
            set_person_locked_idle_ready(p_person);
        }
        break;
    case SUB_STATE_FLIPING:
        if (p_person->Draw.Tweened->FrameIndex > 5)
            change_velocity_to_slow(p_person, 20);
        else if (p_person->Draw.Tweened->FrameIndex > 1)
            set_thing_velocity(p_person, 50);

        if (p_person->Draw.Tweened->FrameIndex < 11) {
            SLONG dx;
            SLONG dz;
            SLONG angle;
            angle = p_person->Draw.Tweened->Angle;
            if (p_person->Genus.Person->Action == ACTION_FLIP_LEFT) {
                angle += 512;
            } else
                angle -= 512;

            angle &= 2047;
            dx = -(SIN(angle) * p_person->Velocity) >> 8;
            dz = -(COS(angle) * p_person->Velocity) >> 8;
            person_normal_move_dxdz(p_person, dx, dz);

            if (check_on_slippy_slope(p_person))
                return;
        }

        end = person_normal_animate(p_person);
        if (end == 1) {
            /*
                                                    if(continue_action(p_person))
                                                    {
                                                            switch(p_person->Genus.Person->Action)
                                                            {
                                                                    case	ACTION_FLIP_LEFT:
                                                                            locked_anim_change(p_person,SUB_OBJECT_LEFT_FOOT,ANIM_FLIP_LEFT_CONT);
                                                                            p_person->Draw.Tweened->FrameIndex=0;
                                                                            if(!foot_hold_available(p_person,100,0,0))
                                                                            {
                                                                                    set_person_locked_idle_ready(p_person);
                                                                            }
                                                                            break;
                                                                    case	ACTION_FLIP_RIGHT:
                                                                            locked_anim_change(p_person,SUB_OBJECT_LEFT_FOOT,ANIM_FLIP_RIGHT_CONT);
                                                                            p_person->Draw.Tweened->FrameIndex=0;
                                                                            if(!foot_hold_available(p_person,-100,0,0))
                                                                            {
                                                                                    set_person_locked_idle_ready(p_person);
                                                                            }
                                                                            break;
                                                            }
                                                    }
                                                    else
            */
            set_locked_anim(p_person, ANIM_STAND_READY, SUB_OBJECT_LEFT_FOOT);
            plant_feet(p_person);
            if (p_person->State != STATE_DANGLING)
                set_person_locked_idle_ready(p_person);

            if (p_person->Genus.Person->PlayerID) {
                // 50 game turns of not going into fight mode; pcom_colour is reused here.
                p_person->Genus.Person->pcom_colour = 20;
                p_person->Genus.Person->Mode = PERSON_MODE_RUN;
            }
            p_person->Velocity = 0;
        }
        break;

        // Bike riding sub-states.

    case SUB_STATE_MOUNTING_BIKE:

        end = person_normal_animate(p_person);

        /*
        //dog poo
                        if(!p_person->Genus.Person->Channel)
                                p_person->Genus.Person->Channel=play_object_wave(p_person->Genus.Person->Channel,p_person,S_BIKE_IDLE,WAVE_PLAY_NO_INTERUPT|WAVE_LOOP);
        #ifdef USE_A3D
        //		if(p_person->Genus.Person->Channel)
        //			A3DPosition(person->Genus.Person->Channel,p_person->WorldPos.X,p_person->WorldPos.Y,p_person->WorldPos.Z);
        #endif
        */
        if (end == 1) {
            p_person->SubState = SUB_STATE_RIDING_BIKE;
        }

        break;

    case SUB_STATE_RIDING_BIKE:

        {
            Thing* p_bike = TO_THING(p_person->Genus.Person->InCar);

            // Move to the same position above the bike.
            GameCoord newpos = p_bike->WorldPos;

            move_thing_on_map(p_person, &newpos);

            // The code to animate the person is all done in the engine nowadays
            // just before the person is drawn.
        }

        break;

    case SUB_STATE_DISMOUNTING_BIKE:

        end = person_normal_animate(p_person);

        if (end == 1) {
            p_person->Genus.Person->Flags &= ~FLAG_PERSON_BIKING;
            p_person->Genus.Person->InCar = 0;
            p_person->Draw.Tweened->Tilt = 0;
            p_person->Draw.Tweened->Roll = 0;

            set_person_idle(p_person);
        }

        break;

    case SUB_STATE_SIMPLE_ANIM:

        // A simple animation (maybe a taunt) set off by PCOM_set_person_move_animation().
        end = person_normal_animate(p_person);

        if (end == 1) {
            if (p_person->Genus.Person->Flags & FLAG_PERSON_NO_RETURN_TO_NORMAL) {
                p_person->Genus.Person->Flags &= ~FLAG_PERSON_NO_RETURN_TO_NORMAL;

                p_person->SubState = SUB_STATE_SIMPLE_ANIM_OVER;
            } else {
                if (p_person->Draw.Tweened->CurrentAnim == ANIM_VALVE_LOOP) {
                    p_person->Genus.Person->Timer1--;
                    if (p_person->Genus.Person->Timer1 <= 0)
                        set_anim(p_person, ANIM_VALVE_END);
                } else {
                    if (p_person->Draw.Tweened->CurrentAnim == ANIM_VALVE_END)
                        set_person_locked_idle_ready(p_person);
                    else
                        set_person_idle(p_person);
                }
            }
        }

        break;

    case SUB_STATE_SIMPLE_ANIM_OVER:

        if (p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_DOWN || p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_IDLE) {
            // Every once in a while do an idle anim.
            if (p_person->Genus.Person->PlayerID) {
                if ((PTIME(p_person) & 0x3) == 0) {
                    p_person->Genus.Person->Timer1 += 1;

                    if (p_person->Genus.Person->Timer1 < 40) {
                        p_person->Genus.Person->Health += 1;

                        if (p_person->Genus.Person->PersonType == PERSON_ROPER) {
                            SATURATE(p_person->Genus.Person->Health, 0, 400);
                        } else {
                            SATURATE(p_person->Genus.Person->Health, 0, 200);
                        }
                    }
                }
            }

            if (((GAME_TURN + THING_NUMBER(p_person) * 8) & 0x3f) < 4) {
                set_person_do_a_simple_anim(p_person, ANIM_SIT_IDLE);

                p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
            }
        } else if (p_person->Draw.Tweened->CurrentAnim == ANIM_HANDS_UP || p_person->Draw.Tweened->CurrentAnim == ANIM_HANDS_UP_LOOP) {
            // Every once in a while do an idle anim.
            if (((GAME_TURN + THING_NUMBER(p_person) * 8) & 0x3f) < 4) {
                set_person_do_a_simple_anim(p_person, ANIM_HANDS_UP_LOOP);

                p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
            }
        } else {
            person_normal_animate(p_person);
        }

        break;

    default:
        MSG_add("MOVEING unknow substate %d \n", p_person->SubState);
        set_person_idle(p_person);
        break;
    }
}

