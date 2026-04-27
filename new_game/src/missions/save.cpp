// In-game save/load system for quick save/load during gameplay (PC only).
// Save format: sequential per-thing records tagged with a type byte, then EWAY timers.
// On save: walks all THING_array slots and emits a compressed or full record per thing class.
// On load: restores all living things from the stream and rebuilds the linked lists.

#include "game/game_types.h"
#include "ai/pcom.h"
#include "things/core/statedef.h"
#include "missions/eway_globals.h"
#include "things/characters/anim_ids.h"
#include "things/items/special.h"
#include "effects/environment/tracks.h"

#include "missions/save.h"
#include "missions/save_globals.h"
#include "things/characters/person.h"  // set_anim

// Record type tags written into the save stream to identify what follows.
// uc_orig: SAVE_PERSON_TYPE_NORMAL (fallen/Source/save.cpp)
#define SAVE_PERSON_TYPE_NORMAL 0
// uc_orig: SAVE_PERSON_TYPE_WANDERING_DRIVER (fallen/Source/save.cpp)
#define SAVE_PERSON_TYPE_WANDERING_DRIVER 1
// uc_orig: SAVE_PERSON_TYPE_DEAD (fallen/Source/save.cpp)
#define SAVE_PERSON_TYPE_DEAD 2
// uc_orig: SAVE_PERSON_TYPE_ARRESTED (fallen/Source/save.cpp)
#define SAVE_PERSON_TYPE_ARRESTED 3
// uc_orig: SAVE_PERSON_TYPE_WANDERING_CIV (fallen/Source/save.cpp)
#define SAVE_PERSON_TYPE_WANDERING_CIV 4
// uc_orig: SAVE_PERSON_TYPE_FULL (fallen/Source/save.cpp)
#define SAVE_PERSON_TYPE_FULL 5
// uc_orig: SAVE_SPECIAL_TYPE_FULL (fallen/Source/save.cpp)
#define SAVE_SPECIAL_TYPE_FULL 6
// uc_orig: SAVE_VEHICLE_TYPE_FULL (fallen/Source/save.cpp)
#define SAVE_VEHICLE_TYPE_FULL 7
// uc_orig: SAVE_VEHICLE_TYPE_WANDERING (fallen/Source/save.cpp)
#define SAVE_VEHICLE_TYPE_WANDERING 8
// uc_orig: SAVE_SKIP (fallen/Source/save.cpp)
#define SAVE_SKIP 99
// uc_orig: SAVE_SKIP_CLASS_NONE (fallen/Source/save.cpp)
#define SAVE_SKIP_CLASS_NONE 100
// uc_orig: SAVE_GAME_EWAY (fallen/Source/save.cpp)
#define SAVE_GAME_EWAY 101

// Binary-file format structures: 1-byte packed for save file compatibility.
#pragma pack(push, 1)

// Compact person record for dead, arrested, and wandering states.
// uc_orig: SAVE_Person (fallen/Source/save.cpp)
typedef struct
{
    UBYTE type;
    UBYTE yaw;
    SBYTE health;
    UBYTE looklike; // Top four bits = PersonID, bottom four = PersonType.
    UWORD x;
    SWORD y;
    UWORD z;
    UWORD other_a; // Driving: the car this person is in. Dead: the current anim.
    UWORD other_b; // Driving: the passenger. Arrested: substate.
    UBYTE ware;
    UBYTE drop;
    UWORD onface;
} SAVE_Person;

// Extended header for full-state person records (precedes the raw Person/DrawTween/Thing blobs).
// uc_orig: SAVE_Person_extra (fallen/Source/save.cpp)
typedef struct
{
    UBYTE Type;
    UBYTE Person;
    UWORD Thing;
    UWORD DrawTween;
} SAVE_Person_extra;

// Extended header for full-state special records.
// uc_orig: SAVE_Special_extra (fallen/Source/save.cpp)
typedef struct
{
    UBYTE Type;
    UBYTE Pad;
    UWORD Thing;
    UWORD Special;
    UWORD DrawMesh;
} SAVE_Special_extra;

// Compact vehicle record (wandering driver only, currently unused — always falls through to full).
// uc_orig: SAVE_just_vehicle (fallen/Source/save.cpp)
typedef struct
{
    UBYTE Type;
    UBYTE Yaw;
    UWORD Thing;
    UWORD x;
    SWORD y;
    UWORD z;
    UWORD driver;
    UWORD passenger;
} SAVE_just_vehicle;

// Extended header for full-state vehicle records.
// uc_orig: SAVE_Vehicle_extra (fallen/Source/save.cpp)
typedef struct
{
    UBYTE Type;
    UBYTE Pad;
    UWORD Thing;
    UWORD Vehicle;
    UWORD DrawMesh;
} SAVE_Vehicle_extra;

#pragma pack(pop)

static_assert(sizeof(SAVE_Person) == 18, "SAVE_Person: binary file layout");
static_assert(sizeof(SAVE_Person_extra) == 6, "SAVE_Person_extra: binary file layout");
static_assert(sizeof(SAVE_Special_extra) == 8, "SAVE_Special_extra: binary file layout");
static_assert(sizeof(SAVE_just_vehicle) == 14, "SAVE_just_vehicle: binary file layout");
static_assert(sizeof(SAVE_Vehicle_extra) == 8, "SAVE_Vehicle_extra: binary file layout");

// Forward declarations of helpers not yet defined at point of first use.
// uc_orig: LOAD_person_dead (fallen/Source/save.cpp)
static void LOAD_person_dead(Thing* p_person);
// uc_orig: LOAD_person_arrested (fallen/Source/save.cpp)
static void LOAD_person_arrested(Thing* p_person);
// uc_orig: LOAD_person_full (fallen/Source/save.cpp)
static void LOAD_person_full(Thing* p_person);
// uc_orig: LOAD_special_full (fallen/Source/save.cpp)
static void LOAD_special_full(Thing* p_special);
// uc_orig: LOAD_vehicle_full (fallen/Source/save.cpp)
static void LOAD_vehicle_full(Thing* p_vehicle);
// uc_orig: LOAD_eways (fallen/Source/save.cpp)
static SLONG LOAD_eways(void);
// uc_orig: fix_thing_lists (fallen/Source/save.cpp)
static void fix_thing_lists(void);
// uc_orig: remove_specials (fallen/Source/save.cpp)
static void remove_specials(void);
// uc_orig: set_person_default_data (fallen/Source/save.cpp)
static void set_person_default_data(Thing* p_person, SAVE_Person* sp);
// uc_orig: LOAD_open (fallen/Source/save.cpp)
static FILE* LOAD_open(void);
// uc_orig: SAVE_things (fallen/Source/save.cpp)
static SLONG SAVE_things(void);
// uc_orig: SAVE_eways (fallen/Source/save.cpp)
static SLONG SAVE_eways(void);
// uc_orig: SAVE_special (fallen/Source/save.cpp)
static SLONG SAVE_special(Thing* p_special);
// uc_orig: SAVE_vehicle (fallen/Source/save.cpp)
static SLONG SAVE_vehicle(Thing* p_vehicle);
// uc_orig: SAVE_person (fallen/Source/save.cpp)
static SLONG SAVE_person(Thing* p_person);
// uc_orig: LOAD_types (fallen/Source/save.cpp)
static SLONG LOAD_types(void);

extern UWORD* thing_class_head;
extern SLONG find_empty_special(void);
extern void free_special(Thing* s_thing);
extern void reload_level(void);

// Writes bytes to SAVE_handle; returns UC_FALSE on I/O error.
// uc_orig: SAVE_out_data (fallen/Source/save.cpp)
static SLONG SAVE_out_data(void* data, ULONG num_bytes)
{
    if (fwrite(data, 1, num_bytes, SAVE_handle) != num_bytes) {
        return UC_FALSE;
    } else {
        return UC_TRUE;
    }
}

// Reads bytes from SAVE_handle; returns UC_FALSE on I/O error.
// uc_orig: LOAD_in_data (fallen/Source/save.cpp)
static SLONG LOAD_in_data(void* data, ULONG num_bytes)
{
    if (fread(data, 1, num_bytes, SAVE_handle) != num_bytes) {
        return UC_FALSE;
    } else {
        return UC_TRUE;
    }
}

// uc_orig: LOAD_open (fallen/Source/save.cpp)
static FILE* LOAD_open()
{
    return MF_Fopen("ingame.sav", "rb");
}


// Serializes a special-class thing (full state: header + Special + DrawMesh + Thing blobs).
// uc_orig: SAVE_special (fallen/Source/save.cpp)
static SLONG SAVE_special(Thing* p_special)
{
    SLONG ret = 1;
    SAVE_Special_extra extra;

    extra.Type = SAVE_SPECIAL_TYPE_FULL;
    extra.Thing = THING_NUMBER(p_special);
    extra.Special = SPECIAL_NUMBER(p_special->Genus.Special);

    ret &= SAVE_out_data(&extra, sizeof(extra));
    ret &= SAVE_out_data(p_special->Genus.Special, sizeof(Special));
    ret &= SAVE_out_data(p_special->Draw.Mesh, sizeof(DrawMesh));
    ret &= SAVE_out_data(p_special, sizeof(Thing));

    return (ret);
}

// Serializes a vehicle-class thing. Wandering drivers (PCOM_AI_DRIVER+WANDER, normal state)
// emit a skip tag and are regenerated by the EWAY system on load.
// uc_orig: SAVE_vehicle (fallen/Source/save.cpp)
static SLONG SAVE_vehicle(Thing* p_vehicle)
{
    Thing* p_driver;
    SLONG ret = 1;
    SAVE_Vehicle_extra extra;

    if (p_vehicle->Genus.Vehicle->Driver) {
        p_driver = TO_THING(p_vehicle->Genus.Vehicle->Driver);
        if (p_driver->Genus.Person->pcom_ai == PCOM_AI_DRIVER && p_driver->Genus.Person->pcom_move == PCOM_MOVE_WANDER) {
            if (p_driver->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                if (p_driver->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
                    SAVE_out_data(&skip, sizeof(skip));
                    return (1);

                    /*
                    // (Original half-vehicle save code, never reached)
                    SAVE_just_vehicle extrav;
                    extrav.Type=SAVE_VEHICLE_TYPE_HALF;
                    extrav.Yaw =(p_vehicle->Genus.Vehicle->Angle&2047)>>3;
                    extrav.Thing = THING_NUMBER(p_vehicle);
                    extrav.x=p_vehicle->WorldPos.X>>8;
                    extrav.y=p_vehicle->WorldPos.Y>>8;
                    extrav.z=p_vehicle->WorldPos.Z>>8;
                    extrav.driver=p_vehicle->Genus.Vehicle->Driver;
                    extrav.passenger=p_vehicle->Genus.Vehicle->Passenger;
                    return(SAVE_out_data(&extrav, sizeof(extrav)));
                    */
                }
            }
        }
    }

    extra.Type = SAVE_VEHICLE_TYPE_FULL;
    extra.Thing = THING_NUMBER(p_vehicle);
    extra.Vehicle = VEHICLE_NUMBER(p_vehicle->Genus.Vehicle);

    ret &= SAVE_out_data(&extra, sizeof(extra));
    ret &= SAVE_out_data(p_vehicle->Genus.Vehicle, sizeof(Vehicle));
    ret &= SAVE_out_data(p_vehicle, sizeof(Thing));

    return (ret);
}

// Serializes a person-class thing. Wandering civs and wandering drivers emit a skip tag
// and are regenerated from waypoints on load. Dead/arrested persons use compact records.
// uc_orig: SAVE_person (fallen/Source/save.cpp)
static SLONG SAVE_person(Thing* p_person)
{
    SAVE_Person sp;
    SLONG ret;

    ASSERT(p_person->Class == CLASS_PERSON);

    memset(&sp, 0, sizeof(sp));

    sp.type = SAVE_PERSON_TYPE_NORMAL;

    if (p_person->Genus.Person->pcom_ai == PCOM_AI_CIV && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER) {
        // Wandering civs are regenerated from level waypoints on load.
        SAVE_out_data(&skip, sizeof(skip));
        return (1);
        sp.type = SAVE_PERSON_TYPE_WANDERING_CIV; // Unreachable — kept 1:1 with original.
        return SAVE_out_data(&sp.type, sizeof(UBYTE));
    }

    if (p_person->Genus.Person->pcom_ai == PCOM_AI_DRIVER && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER) {
        if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
                SAVE_out_data(&skip, sizeof(skip));
                return (1);

                sp.type = SAVE_PERSON_TYPE_WANDERING_DRIVER; // Unreachable — kept 1:1.
                sp.other_a = p_person->Genus.Person->InCar;
                sp.other_b = p_person->Genus.Person->Passenger;
            }
        }
    }

    if (p_person->State == STATE_DEAD) {
        if (p_person->Genus.Person->Flags & FLAG_PERSON_ARRESTED) {
            sp.type = SAVE_PERSON_TYPE_ARRESTED;
            sp.other_b = p_person->SubState;
        } else {
            sp.type = SAVE_PERSON_TYPE_DEAD;
        }
        sp.other_a = p_person->Draw.Tweened->CurrentAnim;
    }

    if (sp.type) {
        sp.x = p_person->WorldPos.X >> 8;
        sp.y = p_person->WorldPos.Y >> 8;
        sp.z = p_person->WorldPos.Z >> 8;
        sp.yaw = p_person->Draw.Tweened->Angle >> 3;
        sp.health = p_person->Genus.Person->Health;
        sp.ware = p_person->Genus.Person->Ware;
        sp.drop = p_person->Genus.Person->drop;
        sp.onface = p_person->OnFace;

        if (!SAVE_out_data(&sp, sizeof(SAVE_Person))) {
            return UC_FALSE;
        }
    } else {
        SAVE_Person_extra extra;

        ret = 1;

        extra.Type = SAVE_PERSON_TYPE_FULL;
        extra.Thing = THING_NUMBER(p_person);
        extra.Person = PERSON_NUMBER(p_person->Genus.Person);
        extra.DrawTween = DRAW_TWEEN_NUMBER(p_person->Draw.Tweened);

        ret &= SAVE_out_data(&extra, sizeof(SAVE_Person_extra));
        ret &= SAVE_out_data(p_person->Draw.Tweened, sizeof(DrawTween));
        ret &= SAVE_out_data(p_person->Genus.Person, sizeof(Person));
        ret &= SAVE_out_data(p_person, sizeof(Thing));
        if (ret == 0)
            return (UC_FALSE);
    }
    return (UC_TRUE);
}

// Iterates all thing slots and serializes each according to its class.
// uc_orig: SAVE_things (fallen/Source/save.cpp)
static SLONG SAVE_things(void)
{
    SLONG index;
    Thing* p_thing;

    for (index = 0; index < MAX_THINGS; index++) {
        p_thing = TO_THING(index);

        switch (p_thing->Class) {
        case CLASS_PERSON:
            if (!SAVE_person(p_thing)) {
                return (UC_FALSE);
            }
            break;
        case CLASS_SPECIAL:
            if (!SAVE_special(p_thing)) {
                return (UC_FALSE);
            }
            break;
        case CLASS_VEHICLE:
            if (!SAVE_vehicle(p_thing)) {
                return (UC_FALSE);
            }
            break;
        case CLASS_NONE:
            SAVE_out_data(&skip_class_none, sizeof(skip_class_none));
            break;

        default:
            SAVE_out_data(&skip, sizeof(skip));
            break;
        }
    }
    return (0);
}

// Serializes EWAY way-flags and countdown timers plus the timer array.
// uc_orig: SAVE_eways (fallen/Source/save.cpp)
static SLONG SAVE_eways(void)
{
    UBYTE marker = SAVE_GAME_EWAY;
    SLONG c0, res = 1;
    EWAY_Way* ew;

    if (!SAVE_out_data(&marker, sizeof(marker))) {
        return UC_FALSE;
    }

    for (c0 = 0; c0 < EWAY_way_upto; c0++) {
        ew = &EWAY_way[c0];

        if (ew->flag & EWAY_FLAG_COUNTDOWN) {
            res &= SAVE_out_data(&ew->flag, sizeof(ew->flag));
            res &= SAVE_out_data(&ew->timer, sizeof(ew->timer));
        } else {
            res &= SAVE_out_data(&ew->flag, sizeof(ew->flag));
        }

        if (!res)
            return (UC_FALSE);
    }
    res &= SAVE_out_data(EWAY_timer, sizeof(UWORD) * EWAY_MAX_TIMERS);
    return UC_TRUE;
}


// Restores EWAY flags, countdown timers, and the timer array from the save stream.
// uc_orig: LOAD_eways (fallen/Source/save.cpp)
static SLONG LOAD_eways(void)
{
    SLONG c0, res = 1;
    EWAY_Way* ew;

    for (c0 = 0; c0 < EWAY_way_upto; c0++) {
        ew = &EWAY_way[c0];

        res &= LOAD_in_data(&ew->flag, sizeof(ew->flag));
        if (ew->flag & EWAY_FLAG_COUNTDOWN) {
            res &= LOAD_in_data(&ew->timer, sizeof(ew->timer));
        }
        if (!res)
            return (UC_FALSE);
    }
    res &= LOAD_in_data(EWAY_timer, sizeof(UWORD) * EWAY_MAX_TIMERS);
    return (res);
}

// Applies compact position/orientation/status fields to an existing person thing.
// uc_orig: set_person_default_data (fallen/Source/save.cpp)
static void set_person_default_data(Thing* p_person, SAVE_Person* sp)
{
    p_person->WorldPos.X = sp->x << 8;
    p_person->WorldPos.Y = sp->y << 8;
    p_person->WorldPos.Z = sp->z << 8;
    p_person->Draw.Tweened->Angle = sp->yaw << 3;
    p_person->Genus.Person->Health = sp->health;
    p_person->Genus.Person->Ware = sp->ware;
    p_person->Genus.Person->drop = sp->drop;
    p_person->OnFace = sp->onface;
}

// uc_orig: LOAD_person_dead (fallen/Source/save.cpp)
static void LOAD_person_dead(Thing* p_person)
{
    SAVE_Person sp;

    ASSERT(p_person->Class == CLASS_PERSON);

    if (p_person->Flags & FLAGS_ON_MAPWHO)
        remove_thing_from_map(p_person);

    LOAD_in_data(&sp.yaw, sizeof(SAVE_Person) - 1);

    set_person_default_data(p_person, &sp);

    set_anim(p_person, sp.other_a);
    set_generic_person_state_function(p_person, STATE_DEAD);
    p_person->Genus.Person->Action = ACTION_DEAD;
    p_person->Genus.Person->Timer1 = 0;
    p_person->SubState = 0;
    TRACKS_Bloodpool(p_person);
    TRACKS_Bloodpool(p_person);

    p_person->Flags &= ~FLAGS_ON_MAPWHO;
    add_thing_to_map(p_person);
}

// uc_orig: LOAD_person_arrested (fallen/Source/save.cpp)
static void LOAD_person_arrested(Thing* p_person)
{
    SAVE_Person sp;

    ASSERT(p_person->Class == CLASS_PERSON);

    if (p_person->Flags & FLAGS_ON_MAPWHO)
        remove_thing_from_map(p_person);

    LOAD_in_data((&sp.yaw), sizeof(SAVE_Person) - 1);

    set_person_default_data(p_person, &sp);

    set_anim(p_person, sp.other_a);
    set_generic_person_state_function(p_person, STATE_DEAD);
    p_person->Genus.Person->Action = ACTION_DEAD;
    p_person->Genus.Person->Timer1 = 0;
    p_person->SubState = sp.other_b;
    TRACKS_Bloodpool(p_person);
    TRACKS_Bloodpool(p_person);

    p_person->Flags &= ~FLAGS_ON_MAPWHO;
    add_thing_to_map(p_person);
}

// Restores full person state: reads DrawTween, Person, and Thing blobs directly.
// uc_orig: LOAD_person_full (fallen/Source/save.cpp)
static void LOAD_person_full(Thing* p_person)
{
    SAVE_Person_extra extra;

    ASSERT(p_person->Class == CLASS_PERSON);

    if (p_person->Flags & FLAGS_ON_MAPWHO)
        remove_thing_from_map(p_person);

    LOAD_in_data((&extra.Person), sizeof(extra) - 1);

    ASSERT(extra.Thing == THING_NUMBER(p_person));
    ASSERT(extra.Person == PERSON_NUMBER(p_person->Genus.Person));
    ASSERT(extra.DrawTween == DRAW_TWEEN_NUMBER(p_person->Draw.Tweened));

    LOAD_in_data(p_person->Draw.Tweened, sizeof(DrawTween));
    LOAD_in_data(p_person->Genus.Person, sizeof(Person));
    LOAD_in_data(p_person, sizeof(Thing));

    if (p_person->Flags & FLAGS_ON_MAPWHO) {
        p_person->Flags &= ~FLAGS_ON_MAPWHO;
        add_thing_to_map(p_person);
    }
}

// Restores full special state: reads Special, DrawMesh, and Thing blobs.
// If the thing slot was repurposed (not CLASS_SPECIAL), allocates a new special.
// uc_orig: LOAD_special_full (fallen/Source/save.cpp)
static void LOAD_special_full(Thing* p_special)
{
    DrawMesh* draw_mesh;
    Special* special;

    if (p_special->Flags & FLAGS_ON_MAPWHO)
        remove_thing_from_map(p_special);

    SAVE_Special_extra extra;
    LOAD_in_data((&extra.Pad), sizeof(extra) - 1);

    if (p_special->Class != CLASS_SPECIAL) {
        SLONG index;
        DrawMesh* dm;

        index = find_empty_special();
        p_special->Genus.Special = TO_SPECIAL(index);

        dm = alloc_draw_mesh();
        ASSERT(dm);
        p_special->Draw.Mesh = dm;
    }

    LOAD_in_data(p_special->Genus.Special, sizeof(Special));
    LOAD_in_data(p_special->Draw.Mesh, sizeof(DrawMesh));
    special = p_special->Genus.Special;
    draw_mesh = p_special->Draw.Mesh;

    LOAD_in_data(p_special, sizeof(Thing));
    p_special->Genus.Special = special;
    p_special->Draw.Mesh = draw_mesh;

    if (p_special->Flags & FLAGS_ON_MAPWHO) {
        p_special->Flags &= ~FLAGS_ON_MAPWHO;
        add_thing_to_map(p_special);
    }
}

// Restores full vehicle state: reads Vehicle and Thing blobs.
// uc_orig: LOAD_vehicle_full (fallen/Source/save.cpp)
static void LOAD_vehicle_full(Thing* p_vehicle)
{
    SAVE_Vehicle_extra extra;

    ASSERT(p_vehicle->Class == CLASS_VEHICLE);

    if (p_vehicle->Flags & FLAGS_ON_MAPWHO)
        remove_thing_from_map(p_vehicle);

    LOAD_in_data((&extra.Pad), sizeof(extra) - 1);

    ASSERT(extra.Thing == THING_NUMBER(p_vehicle));
    ASSERT(extra.Vehicle == VEHICLE_NUMBER(p_vehicle->Genus.Vehicle));

    LOAD_in_data(p_vehicle->Genus.Vehicle, sizeof(Vehicle));
    LOAD_in_data(p_vehicle, sizeof(Thing));

    if (p_vehicle->Flags & FLAGS_ON_MAPWHO) {
        p_vehicle->Flags &= ~FLAGS_ON_MAPWHO;
        add_thing_to_map(p_vehicle);
    }
}

// Reads all thing records sequentially from SAVE_handle, dispatching by type tag.
// uc_orig: LOAD_types (fallen/Source/save.cpp)
static SLONG LOAD_types(void)
{
    UBYTE type;
    Thing* p_thing;

    p_thing = TO_THING(0);
    while (LOAD_in_data(&type, 1)) {
        switch (type) {
        case SAVE_PERSON_TYPE_NORMAL:
            ASSERT(0);
            break;
        case SAVE_PERSON_TYPE_WANDERING_DRIVER:
            ASSERT(0);
            break;
        case SAVE_PERSON_TYPE_DEAD:
            LOAD_person_dead(p_thing);
            break;
        case SAVE_PERSON_TYPE_ARRESTED:
            LOAD_person_arrested(p_thing);
            break;
        case SAVE_PERSON_TYPE_WANDERING_CIV:
            ASSERT(0);
            break;
        case SAVE_PERSON_TYPE_FULL:
            LOAD_person_full(p_thing);
            break;
        case SAVE_SPECIAL_TYPE_FULL:
            LOAD_special_full(p_thing);
            break;
        case SAVE_VEHICLE_TYPE_FULL:
            LOAD_vehicle_full(p_thing);
            break;
        case SAVE_GAME_EWAY:
            LOAD_eways();
        case SAVE_SKIP: // Fall-through intentional.
            switch (p_thing->Class) {
            case CLASS_SPECIAL:
                remove_thing_from_map(p_thing);
                p_thing->Class = CLASS_NONE;
                break;
            }
            break;
        case SAVE_SKIP_CLASS_NONE:
            remove_thing_from_map(p_thing);
            switch (p_thing->Class) {
            case CLASS_NONE:
                break;
            case CLASS_SPECIAL:
                void free_special(Thing * special_thing);
                free_special(p_thing);
                break;
            case CLASS_TRACK:
                break;
            default:
                ASSERT(0);
                break;
            }
            p_thing->Class = CLASS_NONE;
            break;
        default:
            ASSERT(0);
            break;
        }
        p_thing++;
    }

    return 0;
}

// Rebuilds PRIMARY_USED/UNUSED/COUNT linked lists and thing_class_head[] from scratch.
// Must be called after LOAD_types() restores raw thing data.
// uc_orig: fix_thing_lists (fallen/Source/save.cpp)
static void fix_thing_lists(void)
{
    SLONG c0;
    Thing* p_thing;

    PRIMARY_USED = 0;
    PRIMARY_UNUSED = 0;
    PRIMARY_COUNT = 0;

    for (c0 = 0; c0 < MAX_THINGS; c0++) {
        p_thing = TO_THING(c0);

        if (p_thing->Class != CLASS_NONE) {
            p_thing->LinkParent = 0;
            p_thing->LinkChild = PRIMARY_USED;
            PRIMARY_USED = c0;
            PRIMARY_COUNT++;

            thing_class_head[p_thing->Class] = c0;
        } else {
            p_thing->LinkParent = 0;
            p_thing->LinkChild = PRIMARY_UNUSED;
            PRIMARY_UNUSED = c0;
        }
    }
}

// Frees all CLASS_SPECIAL things (called before reloading a level to clear stale state).
// uc_orig: remove_specials (fallen/Source/save.cpp)
static void remove_specials(void)
{
    SLONG index, next;
    Thing* p_special;

    index = thing_class_head[CLASS_SPECIAL];

    while (index) {
        p_special = TO_THING(index);
        next = p_special->NextLink;
        free_special(p_special);
        index = next;
    }
}

