#include "engine/platform/uc_common.h"
#include "engine/platform/sdl3_bridge.h"
#include "things/core/thing.h"
#include "game/game_types.h"
#include "things/core/thing_globals.h"
// functions (add_thing_to_map etc). Pre-existing coupling from original Thing.cpp.
#include "map/pap.h"
#include "map/pap_globals.h"
#include "engine/input/keyboard.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/audio/mfx.h"
#include "engine/input/keyboard_globals.h"  // Keys[] (volatile UBYTE[256])
#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "things/core/statedef.h"

// Forward declarations for external functions not yet migrated.
// uc_orig: Time (MFStdLib/Headers/MFStdLib.h)
void Time(struct MFTime* the_time);
// uc_orig: free_person (fallen/Headers/Person.h)
void free_person(Thing* person_thing);
// uc_orig: free_projectile (fallen/Headers/Pjectile.h)
void free_projectile(Thing* proj_thing);
// uc_orig: free_animal (fallen/Headers/animal.h)
void free_animal(Thing* animal_thing);
// uc_orig: general_process_person (fallen/Headers/Person.h)
void general_process_person(Thing* p_person);
// uc_orig: PCOM_process_person (fallen/Headers/pcom.h)
void PCOM_process_person(Thing* p_person);
// uc_orig: allow_debug_keys (fallen/Source/Game.cpp)
extern BOOL allow_debug_keys;
// uc_orig: SmoothTicks (fallen/Source/Game.cpp)
extern SLONG SmoothTicks(SLONG raw_ticks);
// uc_orig: GAMEMENU_is_paused (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_is_paused(void);
// uc_orig: GAMEMENU_slowdown_mul (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_slowdown_mul(void);
// uc_orig: noise_count (fallen/Source/Sound.cpp)
extern SWORD noise_count;
// uc_orig: process_noises (fallen/Source/Sound.cpp)
extern void process_noises(void);
// uc_orig: WMOVE_process (fallen/Source/wmove.cpp)
extern void WMOVE_process(void);
// uc_orig: do_arrests (fallen/Source/Person.cpp)
extern void do_arrests(void);
// uc_orig: record_video (fallen/DDEngine/Source/aeng.cpp)
extern UBYTE record_video;
// uc_orig: EWAY_stop_player_moving (fallen/Headers/eway.h)
SLONG EWAY_stop_player_moving(void);

// File-local struct for the network/replay packet, not exposed externally.
// uc_orig: NET_packet (fallen/Source/Thing.cpp)
struct NET_packet {
    ULONG Input;
    SLONG Check1;
};

// Timing constant used in do_packets and process_things_tick.
// The original undef+redefine pattern is preserved 1:1.
#undef NORMAL_TICK_TOCK
// uc_orig: NORMAL_TICK_TOCK (fallen/Source/Thing.cpp)
#define NORMAL_TICK_TOCK (1000 / 20)

// uc_orig: init_things (fallen/Source/Thing.cpp)
void init_things(void)
{
    SWORD c0, c1;

    memset((UBYTE*)THINGS, 0, sizeof(Thing) * MAX_THINGS);

    for (c0 = 1; c0 < MAX_PRIMARY_THINGS; c0++) {
        TO_THING(c0)->LinkParent = c0 - 1;
        TO_THING(c0)->LinkChild = c0 + 1;
        TO_THING(c0)->NextLink = 0;
    }
    TO_THING(c0)->LinkParent = c0 - 1;
    PRIMARY_USED = 0;
    PRIMARY_UNUSED = 1;
    PRIMARY_COUNT = 0;

    c1 = (++c0);
    for (; c0 < (MAX_THINGS - 1); c0++) {
        TO_THING(c0)->LinkParent = c0 - 1;
        TO_THING(c0)->LinkChild = c0 + 1;
        TO_THING(c0)->NextLink = 0;
    }
    TO_THING(c1)->LinkParent = 0;
    TO_THING(c0)->LinkParent = c0 - 1;
    SECONDARY_USED = 0;
    SECONDARY_UNUSED = c1;
    SECONDARY_COUNT = 0;

    memset((UBYTE*)thing_class_head, 0, CLASS_END * 2);
}

// uc_orig: alloc_primary_thing (fallen/Source/Thing.cpp)
THING_INDEX alloc_primary_thing(UWORD thing_class)
{
    THING_INDEX new_thing;

    new_thing = PRIMARY_UNUSED;
    if (new_thing) {
        PRIMARY_UNUSED = TO_THING(PRIMARY_UNUSED)->LinkChild;
        TO_THING(PRIMARY_UNUSED)->LinkParent = 0;
        TO_THING(PRIMARY_USED)->LinkParent = new_thing;
        TO_THING(new_thing)->LinkChild = PRIMARY_USED;
        TO_THING(new_thing)->LinkParent = 0;
        PRIMARY_USED = new_thing;
        PRIMARY_COUNT++;
        TO_THING(new_thing)->NextLink = thing_class_head[thing_class];
        thing_class_head[thing_class] = new_thing;
    } else {
        ASSERT(0); // failed to allocate primary thing
    }
    return new_thing;
}

// uc_orig: free_primary_thing (fallen/Source/Thing.cpp)
void free_primary_thing(THING_INDEX thing)
{
    UWORD index;
    UWORD prev;
    if (PRIMARY_USED == thing)
        PRIMARY_USED = TO_THING(thing)->LinkChild;
    else
        TO_THING(TO_THING(thing)->LinkParent)->LinkChild = TO_THING(thing)->LinkChild;

    index = thing_class_head[TO_THING(thing)->Class];
    prev = 0;
    while (index) {
        if (index == thing) {
            if (prev == 0) {
                thing_class_head[TO_THING(thing)->Class] = TO_THING(index)->NextLink;
            } else {
                TO_THING(prev)->NextLink = TO_THING(thing)->NextLink;
            }
            TO_THING(index)->NextLink = 0;
            break;
        }

        prev = index;
        index = TO_THING(index)->NextLink;
    }

    TO_THING(TO_THING(thing)->LinkChild)->LinkParent = TO_THING(thing)->LinkParent;
    TO_THING(PRIMARY_UNUSED)->LinkParent = thing;

    TO_THING(thing)->LinkChild = PRIMARY_UNUSED;
    TO_THING(thing)->LinkParent = 0;
    TO_THING(thing)->Class = CLASS_NONE;
    PRIMARY_UNUSED = thing;
    PRIMARY_COUNT--;
}

// uc_orig: alloc_secondary_thing (fallen/Source/Thing.cpp)
THING_INDEX alloc_secondary_thing(UWORD thing_class)
{
    THING_INDEX new_thing;

    new_thing = SECONDARY_UNUSED;
    if (new_thing) {
        SECONDARY_UNUSED = TO_THING(SECONDARY_UNUSED)->LinkChild;
        TO_THING(SECONDARY_UNUSED)->LinkParent = 0;
        TO_THING(SECONDARY_USED)->LinkParent = new_thing;
        TO_THING(new_thing)->LinkChild = SECONDARY_USED;
        TO_THING(new_thing)->LinkParent = 0;
        SECONDARY_USED = new_thing;
        SECONDARY_COUNT++;
    } else {
        ASSERT(0); // failed to allocate secondary thing
    }
    return new_thing;
}

// uc_orig: free_secondary_thing (fallen/Source/Thing.cpp)
void free_secondary_thing(THING_INDEX thing)
{
    UWORD index, prev;

    index = thing_class_head[TO_THING(thing)->Class];
    prev = 0;
    while (index) {
        if (index == thing) {
            if (prev == 0) {
                thing_class_head[TO_THING(thing)->Class] = TO_THING(index)->NextLink;
            } else {
                TO_THING(prev)->NextLink = TO_THING(thing)->NextLink;
            }
            TO_THING(index)->NextLink = 0;
            break;
        }

        prev = index;
        index = TO_THING(index)->NextLink;
    }

    if (SECONDARY_USED == thing)
        SECONDARY_USED = TO_THING(thing)->LinkChild;
    else
        TO_THING(TO_THING(thing)->LinkParent)->LinkChild = TO_THING(thing)->LinkChild;

    TO_THING(TO_THING(thing)->LinkChild)->LinkParent = TO_THING(thing)->LinkParent;
    TO_THING(SECONDARY_UNUSED)->LinkParent = thing;
    TO_THING(thing)->LinkChild = SECONDARY_UNUSED;
    TO_THING(thing)->LinkParent = 0;
    TO_THING(thing)->Class = CLASS_NONE;
    SECONDARY_UNUSED = thing;
    SECONDARY_COUNT--;
}

// uc_orig: add_thing_to_map (fallen/Source/Thing.cpp)
void add_thing_to_map(Thing* t_thing)
{
    SLONG mx;
    SLONG mz;

    PAP_Lo* pl;

    if (!(t_thing->Flags & FLAGS_ON_MAPWHO)) {
        mx = t_thing->WorldPos.X >> (8 + PAP_SHIFT_LO);
        mz = t_thing->WorldPos.Z >> (8 + PAP_SHIFT_LO);

        if (ON_PAP_LO(mx, mz)) {
            pl = &PAP_2LO(mx, mz);

            t_thing->Parent = 0;
            t_thing->Child = pl->MapWho;

            if (t_thing->Child > 0) {
                TO_THING(t_thing->Child)->Parent = THING_NUMBER(t_thing);
            }

            pl->MapWho = THING_NUMBER(t_thing);

            t_thing->Flags |= FLAGS_ON_MAPWHO;
        }
    }
}

// uc_orig: remove_thing_from_map (fallen/Source/Thing.cpp)
void remove_thing_from_map(Thing* t_thing)
{
    SLONG mx;
    SLONG mz;

    PAP_Lo* pl;

    if (t_thing->Flags & FLAGS_ON_MAPWHO) {
        mx = t_thing->WorldPos.X >> (8 + PAP_SHIFT_LO);
        mz = t_thing->WorldPos.Z >> (8 + PAP_SHIFT_LO);

        if (ON_PAP_LO(mx, mz)) {
            pl = &PAP_2LO(mx, mz);

            if (t_thing->Parent) {
                TO_THING(t_thing->Parent)->Child = t_thing->Child;
            } else {
                pl->MapWho = t_thing->Child;
            }

            if (t_thing->Child > 0) {
                TO_THING(t_thing->Child)->Parent = t_thing->Parent;
            }

            t_thing->Flags &= ~FLAGS_ON_MAPWHO;
        }
    }
}

// uc_orig: move_thing_on_map (fallen/Source/Thing.cpp)
void move_thing_on_map(Thing* t_thing, GameCoord* new_position)
{
    SLONG cur_mx = t_thing->WorldPos.X >> (8 + PAP_SHIFT_LO);
    SLONG cur_mz = t_thing->WorldPos.Z >> (8 + PAP_SHIFT_LO);

    SLONG new_mx = new_position->X >> (8 + PAP_SHIFT_LO);
    SLONG new_mz = new_position->Z >> (8 + PAP_SHIFT_LO);

    // Vehicles are allowed to drive off the map.
    if (t_thing->Class != CLASS_VEHICLE) {
        SATURATE(new_position->X, 0, (128 << 16) - 1);
        SATURATE(new_position->Z, 0, (128 << 16) - 1);

        ASSERT(new_position->X >= 0);
        ASSERT(new_position->X >> 16 < 128);
        ASSERT(new_position->Z >= 0);
        ASSERT(new_position->Z >> 16 < 128);
    }

    if (cur_mx != new_mx || cur_mz != new_mz) {
        remove_thing_from_map(t_thing);
        t_thing->WorldPos = *new_position;
        add_thing_to_map(t_thing);
    } else {
        t_thing->WorldPos = *new_position;
    }
}

// uc_orig: move_thing_on_map_dxdydz (fallen/Source/Thing.cpp)
void move_thing_on_map_dxdydz(Thing* t_thing, SLONG dx, SLONG dy, SLONG dz)
{
    GameCoord new_position;
    new_position.X = t_thing->WorldPos.X + dx;
    new_position.Y = t_thing->WorldPos.Y + dy;
    new_position.Z = t_thing->WorldPos.Z + dz;

    move_thing_on_map(t_thing, &new_position);
}

// uc_orig: log_primary_used_list (fallen/Source/Thing.cpp)
void log_primary_used_list(void)
{
    THING_INDEX thing;

    thing = PRIMARY_USED;
    while (thing) {
        thing = TO_THING(thing)->LinkChild;
    }
}

// uc_orig: log_primary_unused_list (fallen/Source/Thing.cpp)
void log_primary_unused_list(void)
{
    THING_INDEX thing;

    thing = PRIMARY_UNUSED;
    while (thing) {
        thing = TO_THING(thing)->LinkChild;
    }
}

// uc_orig: log_secondary_used_list (fallen/Source/Thing.cpp)
void log_secondary_used_list(void)
{
    THING_INDEX thing;

    thing = SECONDARY_USED;
    while (thing) {
        thing = TO_THING(thing)->LinkChild;
    }
}

// uc_orig: log_secondary_unused_list (fallen/Source/Thing.cpp)
void log_secondary_unused_list(void)
{
    THING_INDEX thing;

    thing = SECONDARY_UNUSED;
    while (thing) {
        thing = TO_THING(thing)->LinkChild;
    }
}

// uc_orig: wait_ticks (fallen/Source/Thing.cpp)
void wait_ticks(SLONG wait)
{
    struct MFTime the_time;

    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
    uint64_t tick_reqd;
    Time(&the_time);
    tick_reqd = the_time.Ticks + wait;
    while (the_time.Ticks < tick_reqd) {
        Time(&the_time);
    }
}

// uc_orig: do_packets (fallen/Source/Thing.cpp)
void do_packets(void)
{
    ULONG input, count = 0;
    NET_packet packets[10];
    SLONG c0;

    if (NO_PLAYERS > 1) {

        for (c0 = 0; c0 < NO_PLAYERS; c0++) {
            input = get_hardware_input(input_type[c0]);
            packets[c0].Input = input;
            PACKET_DATA(c0) = packets[c0].Input;
        }

    } else {

        input = get_hardware_input(INPUT_TYPE_ALL);
        packets[PLAYER_ID].Input = input;

        if (GAME_STATE & GS_PLAYBACK) {
            FileRead(playback_file, &TICK_TOCK, sizeof(TICK_TOCK));
            FileRead(playback_file, &TICK_RATIO, sizeof(TICK_RATIO));
            FileRead(playback_file, &TICK_INV_RATIO, sizeof(TICK_INV_RATIO));
            FileRead(playback_file, &TICK_TOCK, sizeof(TICK_TOCK));

            for (count = 0; count < (unsigned)NO_PLAYERS; count++) {
                if (FileRead(playback_file, &packets[count], sizeof(NET_packet)) != sizeof(NET_packet))
                    GAME_STATE &= ~GS_PLAY_GAME;
            }
        } else if (GAME_STATE & GS_RECORD) {
            FileWrite(playback_file, &TICK_TOCK, sizeof(TICK_TOCK));
            FileWrite(playback_file, &TICK_RATIO, sizeof(TICK_RATIO));
            FileWrite(playback_file, &TICK_INV_RATIO, sizeof(TICK_INV_RATIO));
            FileWrite(playback_file, &TICK_TOCK, sizeof(TICK_TOCK));

            for (count = 0; count < NO_PLAYERS; count++) {
                FileWrite(playback_file, &packets[count], sizeof(NET_packet));
            }
        }

        PACKET_DATA(PLAYER_ID) = packets[PLAYER_ID].Input;
    }
}

// uc_orig: set_slow_motion (fallen/Source/Thing.cpp)
void set_slow_motion(UWORD motion)
{
    slow_mo = motion;
}

// uc_orig: copy_important_thing_bits (fallen/Source/Thing.cpp)
void copy_important_thing_bits(const Thing* src, Thing* dst)
{
    memcpy(dst, src, sizeof(Thing));
    dst->Draw.Tweened = NULL;
    dst->Genus.Vehicle = NULL;
    dst->Flags &= ~FLAGS_HAS_ATTACHED_SOUND;
    dst->StateFn = NULL;
}

// uc_orig: store_thing (fallen/Source/Thing.cpp)
void store_thing(Thing* p_thing)
{
    Thing temp;

    copy_important_thing_bits(p_thing, &temp);

    FileWrite(verifier_file, &temp, sizeof(temp));
}

// uc_orig: check_thing (fallen/Source/Thing.cpp)
void check_thing(Thing* p_thing)
{
    Thing temp;

    copy_important_thing_bits(p_thing, &temp);

    Thing file;

    FileRead(verifier_file, &file, sizeof(file));

    Thing file2;

    copy_important_thing_bits(&file, &file2);

    if (memcmp(&file2, &temp, sizeof(file2))) {
        ASSERT(0);
    }
}

// uc_orig: for_things (fallen/Source/Thing.cpp)
void for_things(void (*fn)(Thing* p_thing))
{
    int ix = 0;

    while (class_check[ix]) {
        UWORD list;

        list = thing_class_head[class_check[ix]];

        while (list) {
            Thing* p_thing = TO_THING(list);

            ASSERT(p_thing->Class == class_check[ix]);

            fn(p_thing);

            list = p_thing->NextLink;
        }

        ix++;
    }
}

// uc_orig: store_thing_data (fallen/Source/Thing.cpp)
void store_thing_data()
{
    for_things(store_thing);
}

// uc_orig: check_thing_data (fallen/Source/Thing.cpp)
void check_thing_data()
{
    for_things(check_thing);
}

// uc_orig: process_things_tick (fallen/Source/Thing.cpp)
void process_things_tick(SLONG frame_rate_independant)
{
    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
    static uint64_t prev_tick = 0;
    uint64_t cur_tick;

    SLONG tick_diff;
    static BOOL first_pass = UC_TRUE;

    cur_tick = sdl3_get_ticks();
    tick_diff = cur_tick - prev_tick;
    prev_tick = cur_tick;

    if (first_pass) {
        tick_diff = NORMAL_TICK_TOCK;
        first_pass = UC_FALSE;
    }

    if (CNET_network_game) {
        tick_diff = 1000 / 20;
    }

    if (frame_rate_independant == 0)
        tick_diff = 1000 / 25; // assume 25 fps

    if (allow_debug_keys)
        if (Keys[KB_COLON]) {
            if (slow_mo)
                slow_mo = 0;
            else
                slow_mo = 32000;
            Keys[KB_COLON] = 0;
        }

    if (record_video)
        tick_diff = 40;

    TICK_TOCK = tick_diff;
    TICK_RATIO = (TICK_TOCK << TICK_SHIFT) / (NORMAL_TICK_TOCK);

    tick_tock_unclipped = TICK_TOCK;

    TICK_RATIO = SmoothTicks(TICK_RATIO);

#define MIN_TICK_RATIO ((1 << TICK_SHIFT) / 2)
#define MAX_TICK_RATIO ((1 << TICK_SHIFT) * 3 >> 1)

    SATURATE(TICK_TOCK, NORMAL_TICK_TOCK >> 1, NORMAL_TICK_TOCK * 2);
    SATURATE(TICK_RATIO, MIN_TICK_RATIO, MAX_TICK_RATIO);

    if (slow_mo) {
        slow_mo--;
        TICK_RATIO = 32;
        TICK_TOCK = (TICK_RATIO * NORMAL_TICK_TOCK) >> TICK_SHIFT;
    }

    REAL_TICK_RATIO = TICK_RATIO;
    if (!GAMEMENU_is_paused())
        if (GAMEMENU_slowdown_mul() != 0x100) {
            TICK_RATIO = TICK_RATIO * GAMEMENU_slowdown_mul() >> 8;
            TICK_TOCK = TICK_RATIO * NORMAL_TICK_TOCK >> TICK_SHIFT;

            if (TICK_RATIO == 0) {
                TICK_RATIO = 1;
            }
        }
    TICK_INV_RATIO = 0x10000 / TICK_RATIO;
}

// uc_orig: process_things (fallen/Source/Thing.cpp)
void process_things(SLONG frame_rate_independant)
{
    Thing* p_thing;
    UWORD index = 0;

    process_things_tick(frame_rate_independant);

    do_packets();

    // Process things by class priority.
    index = 0;
    while (class_priority[index]) {
        UWORD list;

        list = thing_class_head[class_priority[index]];
        while (list) {
            p_thing = TO_THING(list);
            list = p_thing->NextLink;
            ASSERT(p_thing->Class == class_priority[index]);

            if (p_thing->StateFn) {
                {
                    p_thing->StateFn(p_thing);
                    if (noise_count) {
                        process_noises();
                    }

                    if (index == 1) {
                        // The first time through processing the barrels.
                    } else {
                        p_thing->Flags &= ~FLAGS_IN_VIEW;
                    }
                }
            }
        }
        index++;
    }

    // Players don't care about flag_in_view.
    index = thing_class_head[CLASS_PLAYER];
    while (index) {
        p_thing = TO_THING(index);
        index = p_thing->NextLink;

        ASSERT(p_thing->Class == CLASS_PLAYER);

        if (p_thing->StateFn) {
            p_thing->StateFn(p_thing);
        }

        p_thing->Flags &= ~FLAGS_IN_VIEW;
    }

    WMOVE_process();

    index = thing_class_head[CLASS_PERSON];
    while (index) {
        p_thing = TO_THING(index);
        index = p_thing->NextLink;
        ASSERT(p_thing->Class == CLASS_PERSON);

        ++PTIME(p_thing);

        {
            {
                if (p_thing->Genus.Person->InCar) {
                }
                general_process_person(p_thing);
                PCOM_process_person(p_thing);

                if (noise_count) {
                    process_noises();
                }
            }
        }
        p_thing->Flags &= ~FLAGS_IN_VIEW;
    }
    do_arrests();
}

// Returns UC_TRUE if the thing class is primary (lives in the primary pool).
// uc_orig: is_class_primary (fallen/Source/Thing.cpp)
static BOOL is_class_primary(SBYTE classification)
{
    switch (classification) {
    case CLASS_NONE:
    case CLASS_CAMERA:
    case CLASS_SWITCH:
    case CLASS_TRACK:
        return UC_FALSE;
    case CLASS_PLAYER:
    case CLASS_PROJECTILE:
    case CLASS_BUILDING:
    case CLASS_PERSON:
    case CLASS_FURNITURE:
    case CLASS_VEHICLE:
    case CLASS_SPECIAL:
    case CLASS_PYRO:
    case CLASS_PLAT:
    case CLASS_BARREL:
    case CLASS_BIKE:
    case CLASS_BAT:
        return UC_TRUE;
    default:
        ASSERT(0);
        return UC_FALSE;
    }
}

// uc_orig: alloc_thing (fallen/Source/Thing.cpp)
Thing* alloc_thing(SBYTE classification)
{
    Thing* t_thing = NULL;
    THING_INDEX new_thing;

    if (is_class_primary(classification))
        new_thing = alloc_primary_thing(classification);
    else
        new_thing = alloc_secondary_thing(classification);

    if (new_thing) {
        t_thing = TO_THING(new_thing);
        t_thing->Class = classification;
        t_thing->State = STATE_INIT;
        t_thing->Flags = 0;
        t_thing->StateFn = 0;
    }
    return t_thing;
}

// uc_orig: free_thing (fallen/Source/Thing.cpp)
void free_thing(Thing* t_thing)
{
    if (t_thing->Flags & FLAGS_HAS_ATTACHED_SOUND)
        MFX_stop_attached(t_thing);
    if (is_class_primary(t_thing->Class))
        free_primary_thing(THING_NUMBER(t_thing));
    else
        free_secondary_thing(THING_NUMBER(t_thing));
}

// uc_orig: THING_kill (fallen/Source/Thing.cpp)
void THING_kill(Thing* p_thing)
{
    remove_thing_from_map(p_thing);

    switch (p_thing->Class) {
    case CLASS_NONE:
        break;
    case CLASS_PLAYER:
        free_person(p_thing);
        break;
    case CLASS_PROJECTILE:
        free_projectile(p_thing);
        break;
    case CLASS_BUILDING:
        ASSERT(0);
        break;
    case CLASS_PERSON:
        free_person(p_thing);
        break;
    case CLASS_FURNITURE:
    case CLASS_VEHICLE:
        break;
    case CLASS_ANIMAL:
        free_animal(p_thing);
        break;
    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: THING_dist_between (fallen/Source/Thing.cpp)
SLONG THING_dist_between(Thing* p_thing_a, Thing* p_thing_b)
{
    SLONG dx = abs(p_thing_b->WorldPos.X - p_thing_a->WorldPos.X >> 8);
    SLONG dy = abs(p_thing_b->WorldPos.Y - p_thing_a->WorldPos.Y >> 8);
    SLONG dz = abs(p_thing_b->WorldPos.Z - p_thing_a->WorldPos.Z >> 8);

    SLONG dist = QDIST3(dx, dy, dz);

    return dist;
}

// uc_orig: THING_find_sphere (fallen/Source/Thing.cpp)
SLONG THING_find_sphere(SLONG pos_x, SLONG pos_y, SLONG pos_z, SLONG radius, THING_INDEX* array, SLONG array_size, ULONG classes)
{
    UBYTE mx;
    UBYTE mz;

    SWORD x1;
    SWORD z1;
    SWORD x2;
    SWORD z2;

    SWORD dx;
    SWORD dy;
    SWORD dz;

    SLONG dist;

    SWORD array_upto;

    THING_INDEX t_index;
    Thing* p_thing;

    x1 = pos_x - radius >> PAP_SHIFT_LO;
    z1 = pos_z - radius >> PAP_SHIFT_LO;

    x2 = pos_x + radius >> PAP_SHIFT_LO;
    z2 = pos_z + radius >> PAP_SHIFT_LO;

    SATURATE(x1, 0, PAP_SIZE_LO - 1);
    SATURATE(x2, 0, PAP_SIZE_LO - 1);
    SATURATE(z1, 0, PAP_SIZE_LO - 1);
    SATURATE(z2, 0, PAP_SIZE_LO - 1);

    array_upto = 0;

    for (mx = x1; mx <= x2; mx++) {
        for (mz = z1; mz <= z2; mz++) {
            t_index = PAP_2LO(mx, mz).MapWho;

            while (t_index) {
                p_thing = TO_THING(t_index);

                if (classes & (1 << p_thing->Class)) {
                    dx = abs((p_thing->WorldPos.X >> 8) - pos_x);
                    if (classes & 1 << 31)
                        dy = 0;
                    else
                        dy = abs((p_thing->WorldPos.Y >> 8) - pos_y);
                    dz = abs((p_thing->WorldPos.Z >> 8) - pos_z);

                    dist = QDIST3(dx, dy, dz);

                    if (dist <= radius) {
                        if ((!hit_player) && p_thing->Class == CLASS_PERSON && p_thing->Genus.Person->PlayerID && EWAY_stop_player_moving()) {
                            // Can't find player in cut scene.
                        } else {
                            if (array_upto < array_size) {
                                array[array_upto++] = t_index;
                            } else {
                                return array_upto;
                            }
                        }
                    }
                }

                t_index = p_thing->Child;
            }
        }
    }

    return array_upto;
}

// uc_orig: THING_find_box (fallen/Source/Thing.cpp)
SLONG THING_find_box(SLONG x1, SLONG z1, SLONG x2, SLONG z2, THING_INDEX* array, SLONG array_size, ULONG classes)
{
    SLONG mx;
    SLONG mz;

    SLONG array_upto;

    THING_INDEX t_index;
    Thing* p_thing;

    x1 >>= PAP_SHIFT_LO;
    z1 >>= PAP_SHIFT_LO;
    x2 >>= PAP_SHIFT_LO;
    z2 >>= PAP_SHIFT_LO;

    SATURATE(x1, 0, PAP_SIZE_LO - 1);
    SATURATE(x2, 0, PAP_SIZE_LO - 1);
    SATURATE(z1, 0, PAP_SIZE_LO - 1);
    SATURATE(z2, 0, PAP_SIZE_LO - 1);

    array_upto = 0;

    for (mx = x1; mx <= x2; mx++) {
        for (mz = z1; mz <= z2; mz++) {
            t_index = PAP_2LO(mx, mz).MapWho;

            while (t_index) {
                p_thing = TO_THING(t_index);

                if (classes & (1 << p_thing->Class)) {
                    if (array_upto < array_size) {
                        array[array_upto++] = t_index;
                    } else {
                        return array_upto;
                    }
                }

                t_index = p_thing->Child;
            }
        }
    }

    return array_upto;
}

// uc_orig: THING_find_nearest_xyz_p (fallen/Source/Thing.cpp)
static SLONG THING_find_nearest_xyz_p(
    SLONG centre_x,
    SLONG centre_y,
    SLONG centre_z,
    SLONG radius,
    ULONG classes,
    Thing* p_person)
{
    SLONG mx;
    SLONG mz;

    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;

    SLONG best_dist = radius;
    SLONG best_thing = NULL;

    THING_INDEX t_index;
    Thing* p_thing;

    x1 = centre_x - radius >> PAP_SHIFT_LO;
    z1 = centre_z - radius >> PAP_SHIFT_LO;

    x2 = centre_x + radius >> PAP_SHIFT_LO;
    z2 = centre_z + radius >> PAP_SHIFT_LO;

    SATURATE(x1, 0, PAP_SIZE_LO - 1);
    SATURATE(x2, 0, PAP_SIZE_LO - 1);
    SATURATE(z1, 0, PAP_SIZE_LO - 1);
    SATURATE(z2, 0, PAP_SIZE_LO - 1);

    for (mx = x1; mx <= x2; mx++) {
        for (mz = z1; mz <= z2; mz++) {
            t_index = PAP_2LO(mx, mz).MapWho;

            while (t_index) {
                p_thing = TO_THING(t_index);

                if ((classes & (1 << p_thing->Class)) && p_thing != p_person) {
                    dx = abs((p_thing->WorldPos.X >> 8) - centre_x);
                    dy = abs((p_thing->WorldPos.Y >> 8) - centre_y);
                    dz = abs((p_thing->WorldPos.Z >> 8) - centre_z);

                    dist = QDIST3(dx, dy, dz);

                    if (dist < best_dist) {
                        best_dist = dist;
                        best_thing = t_index;
                    }
                }

                t_index = p_thing->Child;
            }
        }
    }

    return best_thing;
}

// uc_orig: THING_find_nearest (fallen/Source/Thing.cpp)
SLONG THING_find_nearest(
    SLONG centre_x,
    SLONG centre_y,
    SLONG centre_z,
    SLONG radius,
    ULONG classes)
{
    return (THING_find_nearest_xyz_p(centre_x, centre_y, centre_z, radius, classes, 0));
}

// uc_orig: THING_find_nearest_person (fallen/Source/Thing.cpp)
SLONG THING_find_nearest_person(Thing* p_person, SLONG radius, ULONG classes)
{
    return (THING_find_nearest_xyz_p(p_person->WorldPos.X >> 8, p_person->WorldPos.Y >> 8, p_person->WorldPos.Z >> 8, radius, classes, p_person));
}
