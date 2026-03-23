#include "effects/dirt.h"
#include "effects/dirt_globals.h"
#include "effects/drip.h"
#include "effects/pyro.h"
#include "engine/effects/psystem.h"
#include "engine/animation/morph.h"
#include "engine/audio/mfx.h"
#include "core/fmatrix.h"
#include "world/map/pap.h"
#include "world/map/ob.h"
#include "world/environment/ns.h"
#include "actors/core/interact.h"
#include "ai/pcom.h"
#include "fallen/Headers/Game.h"       // Temporary: GAME_FLAGS, GF_*, GAME_TURN, Thing, calc_angle, NET_PERSON
#include "fallen/Headers/mav.h"        // Temporary: MAV_SPARE, MAV_SPARE_FLAG_WATER
#include "fallen/Headers/prim.h"       // Temporary: PRIM_FLAG_TREE, PRIM_OBJ_CAN, PRIM_OBJ_ITEM_KEY
#include "assets/sound_id.h"     // Temporary: S_KICK_CAN
#include "engine/audio/sound.h"
#include "engine/graphics/pipeline/poly.h"  // Temporary: POLY_PAGE_EXPLODE1_ADDITIVE, POLY_PAGE_EXPLODE2_ADDITIVE
#include "missions/memory_globals.h"   // Temporary: prim_objects
#include "actors/characters/anim_ids.h"

// uc_orig: TICK_SHIFT_LOWRES (fallen/Source/dirt.cpp)
#define TICK_SHIFT_LOWRES (TICK_SHIFT - 2)

// Number of leaves created per tree when it enters camera range.
// uc_orig: DIRT_LEAVES_PER_TREE (fallen/Source/dirt.cpp)
#define DIRT_LEAVES_PER_TREE 200

// Pigeon state IDs used by DIRT_pigeon_process and state init functions.
// uc_orig: DIRT_PIGEON_WAIT (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_WAIT     0
// uc_orig: DIRT_PIGEON_PECK (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_PECK     1
// uc_orig: DIRT_PIGEON_WALK (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_WALK     2
// uc_orig: DIRT_PIGEON_HOP (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_HOP      3
// uc_orig: DIRT_PIGEON_COURT (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_COURT    4
// uc_orig: DIRT_PIGEON_RUN (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_RUN      5
// uc_orig: DIRT_PIGEON_TAKEOFF (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_TAKEOFF  6
// uc_orig: DIRT_PIGEON_FLY (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_FLY      7
// uc_orig: DIRT_PIGEON_LAND (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_LAND     8
// uc_orig: DIRT_PIGEON_PERCHED (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_PERCHED  9
// uc_orig: DIRT_PIGEON_SIDLE (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_SIDLE    10
// uc_orig: DIRT_PIGEON_NUM_STATES (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_NUM_STATES 11

// uc_orig: DIRT_PIGEON_HOPUP (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_HOPUP   1
// uc_orig: DIRT_PIGEON_HOPDOWN (fallen/Source/dirt.cpp)
#define DIRT_PIGEON_HOPDOWN 2

// Forward declarations for pigeon helpers.
// uc_orig: DIRT_pigeon_init_wait (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_wait(DIRT_Dirt* dd);
// uc_orig: DIRT_pigeon_init_peck (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_peck(DIRT_Dirt* dd);
// uc_orig: DIRT_pigeon_init_walk (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_walk(DIRT_Dirt* dd);
// uc_orig: DIRT_pigeon_init_hop (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_hop(DIRT_Dirt* dd, UBYTE upordown);
// uc_orig: DIRT_pigeon_init_flee (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_flee(DIRT_Dirt* dd, SLONG scare_x, SLONG scare_z);
// uc_orig: DIRT_pigeon_process_wait (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process_wait(DIRT_Dirt* dd);
// uc_orig: DIRT_pigeon_process_peck (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process_peck(DIRT_Dirt* dd);
// uc_orig: DIRT_pigeon_process_walkrun (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process_walkrun(DIRT_Dirt* dd);
// uc_orig: DIRT_pigeon_process_hop (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process_hop(DIRT_Dirt* dd);
// uc_orig: DIRT_pigeon_start_doing_something_new (fallen/Source/dirt.cpp)
static void DIRT_pigeon_start_doing_something_new(DIRT_Dirt* dd);
// uc_orig: DIRT_pigeon_process (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process(DIRT_Dirt* dd);
// uc_orig: DIRT_spark_shower (fallen/Source/dirt.cpp)
static void DIRT_spark_shower(DIRT_Dirt* dd);

// Selects the type of new dirt to place at (x, z).
// Returns DIRT_TYPE_UNUSED if probabilities don't assign a type.
// uc_orig: DIRT_get_new_type (fallen/Source/dirt.cpp)
static SLONG DIRT_get_new_type(SLONG x, SLONG z)
{
    SLONG choice;

    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (WITHIN(mx, DIRT_pigeon_map_x1, DIRT_pigeon_map_x2) && WITHIN(mz, DIRT_pigeon_map_z1, DIRT_pigeon_map_z2)) {
        ASSERT(0);
        return DIRT_TYPE_PIGEON;
    } else {
        if (world_type == WORLD_TYPE_FOREST)
            return DIRT_TYPE_LEAF;
        if (world_type == WORLD_TYPE_SNOW)
            return DIRT_TYPE_SNOW;

        choice = Random() & 0xff;

        choice -= DIRT_prob_leaf;
        if (choice < 0) {
            return DIRT_TYPE_LEAF;
        }
        choice -= DIRT_prob_can;
        if (choice < 0) {
            return DIRT_TYPE_CAN;
        }
        choice -= DIRT_prob_pigeon;
        if (choice < 0) {
            return DIRT_TYPE_PIGEON;
        }
        return DIRT_TYPE_UNUSED;
    }
}

// uc_orig: DIRT_init (fallen/Source/dirt.cpp)
void DIRT_init(
    SLONG prob_leaf,
    SLONG prob_can,
    SLONG prob_pigeon,
    SLONG pigeon_map_x1,
    SLONG pigeon_map_z1,
    SLONG pigeon_map_x2,
    SLONG pigeon_map_z2)
{
    SLONG i;
    SLONG prob_sum;

    DIRT_Dirt* dd;

    THING_INDEX index;
    Thing* p_thing;

    DIRT_focus_x = 0;
    DIRT_focus_z = 0;
    DIRT_focus_radius = 0x100;
    DIRT_focus_first = 4;
    DIRT_check = 0;

    // Pigeons disabled.
    prob_pigeon = 0;

    for (i = 0; i < DIRT_MAX_DIRT; i++) {
        memset((UBYTE*)&DIRT_dirt[i], 0, sizeof(DIRT_Dirt));
        DIRT_dirt[i].type = DIRT_TYPE_UNUSED;
    }

    prob_sum = prob_leaf;
    prob_sum += prob_can;
    prob_sum += prob_pigeon;

    if (prob_sum == 0) {
        DIRT_prob_leaf = 0;
        DIRT_prob_can = 0;
        DIRT_prob_pigeon = 0;
    } else {
        DIRT_prob_leaf = (prob_leaf * 256) / prob_sum;
        DIRT_prob_can = (prob_can * 256) / prob_sum;
        DIRT_prob_pigeon = (prob_pigeon * 256) / prob_sum;
    }

    DIRT_pigeon_map_x1 = pigeon_map_x1;
    DIRT_pigeon_map_z1 = pigeon_map_z1;
    DIRT_pigeon_map_x2 = pigeon_map_x2;
    DIRT_pigeon_map_z2 = pigeon_map_z2;

    DIRT_tree_upto = 0;

    {
        SLONG mx;
        SLONG mz;

        OB_Info* oi;

        for (mx = 0; mx < PAP_SIZE_LO; mx++)
            for (mz = 0; mz < PAP_SIZE_LO; mz++) {
                for (oi = OB_find(mx, mz); oi->prim; oi++) {
                    if (prim_objects[oi->prim].flag & PRIM_FLAG_TREE) {
                        if (WITHIN(DIRT_tree_upto, 0, DIRT_MAX_TREES - 1)) {
                            DIRT_tree[DIRT_tree_upto].x = oi->x;
                            DIRT_tree[DIRT_tree_upto].z = oi->z;
                            DIRT_tree[DIRT_tree_upto].inrange = UC_FALSE;

                            DIRT_tree_upto += 1;
                        }
                    }
                }
            }
    }
}

// Returns a piece of dirt that isn't very important (offscreen or far away).
// Used to recycle slots when creating new dirt entries.
// uc_orig: DIRT_find_useless (fallen/Source/dirt.cpp)
DIRT_Dirt* DIRT_find_useless(void)
{
    SLONG i;

    DIRT_Dirt* dd;

    for (i = 0; i < 8; i++) {
        dd = &DIRT_dirt[Random() & (DIRT_MAX_DIRT - 1)];

        if (dd->type == DIRT_TYPE_UNUSED || (dd->flag & DIRT_FLAG_DELETE_OK)) {
            return dd;
        }
    }

    for (i = 0; i < 8; i++) {
        dd = &DIRT_dirt[Random() & (DIRT_MAX_DIRT - 1)];

        if (dd->type == DIRT_TYPE_LEAF) {
            return dd;
        }
    }

    dd = &DIRT_dirt[Random() & (DIRT_MAX_DIRT - 1)];

    return dd;
}

// Sets the camera focus position and radius, culling and spawning dirt as needed.
// Processes 1/8th of the dirt pool per call (all of it on first call).
// uc_orig: DIRT_set_focus (fallen/Source/dirt.cpp)
void DIRT_set_focus(
    SLONG x,
    SLONG z,
    SLONG radius)
{
    SLONG i;
    SLONG j;
    SLONG k;

    SLONG lx;
    SLONG lz;
    SLONG cx;
    SLONG cz;
    SLONG nx;
    SLONG nz;
    SLONG mx;
    SLONG mz;

    SLONG dx;
    SLONG dz;
    SLONG dist;
    SLONG angle;
    SLONG type;
    SLONG done;

    DIRT_Dirt* dd;
    PAP_Hi* ph;
    PAP_Hi* ph2;

    struct
    {
        SBYTE dx;
        SBYTE dz;

    } order[8] = {
        { -1, -1 },
        { 0, -1 },
        { +1, -1 },
        { -1, 0 },
        { +1, 0 },
        { -1, +1 },
        { 0, +1 },
        { +1, +1 }
    };

    DIRT_focus_x = x;
    DIRT_focus_z = z;
    DIRT_focus_radius = radius;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    for (i = 0; i < DIRT_tree_upto; i++) {
        dx = abs(DIRT_tree[i].x - DIRT_focus_x);
        dz = abs(DIRT_tree[i].z - DIRT_focus_z);

        dist = QDIST2(dx, dz);

        if (dist > DIRT_focus_radius) {
            if (DIRT_tree[i].inrange) {
                for (j = 0; j < DIRT_MAX_DIRT; j++) {
                    dd = &DIRT_dirt[j];

                    if (dd->type == DIRT_TYPE_LEAF && dd->owner == i) {
                        dd->type = DIRT_TYPE_UNUSED;
                    }
                }

                DIRT_tree[i].inrange = UC_FALSE;
            }
        } else {
            if (!DIRT_tree[i].inrange) {
                done = 0;

                dd = &DIRT_dirt[Random() % DIRT_MAX_DIRT];

                for (j = 0; j < DIRT_MAX_DIRT; j++) {
                    dd += 1;

                    if (dd >= &DIRT_dirt[DIRT_MAX_DIRT]) {
                        dd = &DIRT_dirt[0];
                    }

                    if (dd->type == DIRT_TYPE_UNUSED || (dd->flag & DIRT_FLAG_DELETE_OK)) {
                        done += 1;

                        dist = Random() & 0x7f;
                        dist += 150;
                        dist = dist * dist >> 8;
                        angle = Random() & 2047;

                        dx = SIN(angle) * dist >> 16;
                        dz = COS(angle) * dist >> 16;

                        lx = DIRT_tree[i].x + dx;
                        lz = DIRT_tree[i].z + dz;

                        mx = lx >> 8;
                        mz = lz >> 8;

                        if (PAP_on_map_hi(mx, mz)) {
                            if (!(PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN)) {
                                dd->type = DIRT_TYPE_LEAF;
                                dd->owner = i;
                                dd->flag = DIRT_FLAG_STILL;
                                dd->x = lx;
                                dd->z = lz;
                                dd->y = PAP_calc_height_at(lx, lz);
                                dd->dx = 0;
                                dd->dy = 0;
                                dd->dz = 0;
                                dd->yaw = 0;
                                dd->pitch = 0;
                                dd->roll = 0;
                                dd->dyaw = 0;
                                dd->dpitch = 0;
                                dd->droll = 0;
                            }
                        }

                        if (done >= DIRT_LEAVES_PER_TREE) {
                            break;
                        }
                    }
                }

                DIRT_tree[i].inrange = UC_TRUE;
            }
        }
    }

    SLONG number_to_check;

    if (DIRT_focus_first) {
        number_to_check = DIRT_MAX_DIRT;
    } else {
        number_to_check = DIRT_MAX_DIRT / 8;
    }

    for (i = 0; i < number_to_check; i++) {
        DIRT_check += 1;

        if (DIRT_check >= DIRT_MAX_DIRT) {
            DIRT_check = 0;
        }

        dd = &DIRT_dirt[DIRT_check];

        if (dd->type == DIRT_TYPE_LEAF && dd->owner != 255) {
            continue;
        }

        if (dd->type == DIRT_TYPE_MINE) {
            continue;
        }

        dx = abs(dd->x - DIRT_focus_x);
        dz = abs(dd->z - DIRT_focus_z);

        dist = QDIST2(dx, dz);

        if (dist > DIRT_focus_radius || dd->type == DIRT_TYPE_UNUSED) {
            angle = Random() & 2047;

            if (DIRT_focus_first) {
                dist = Random() & 0xff;
                dist += Random() & 0xff;
                dist += Random() & 0xff;
                dist += Random() & 0xff;

                dist = (0x3af - dist) * DIRT_focus_radius >> 10;
            } else {
                dist = DIRT_focus_radius;
                dist -= Random() & 0xff;
                dist -= 0x80;
                if (world_type == WORLD_TYPE_SNOW)
                    dist -= 0xff + (Random() & 0x1ff);
                if (dist < 10)
                    dist = 10;
            }

            cx = DIRT_focus_x + MUL64(SIN(angle), dist);
            cz = DIRT_focus_z + MUL64(COS(angle), dist);

            mx = cx >> PAP_SHIFT_HI;
            mz = cz >> PAP_SHIFT_HI;

            if (PAP_on_map_hi(mx, mz) && ((world_type == WORLD_TYPE_SNOW) || !(PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN)) && !(MAV_SPARE(mx, mz) & MAV_SPARE_FLAG_WATER) && !(MAV_SPARE(mx + 1, mz) & MAV_SPARE_FLAG_WATER) && !(MAV_SPARE(mx - 1, mz) & MAV_SPARE_FLAG_WATER) && !(MAV_SPARE(mx, mz + 1) & MAV_SPARE_FLAG_WATER) && !(MAV_SPARE(mx, mz - 1) & MAV_SPARE_FLAG_WATER)) {
                type = DIRT_get_new_type(cx, cz);

                switch (type) {
                case DIRT_TYPE_UNUSED:
                    dd->type = DIRT_TYPE_UNUSED;
                    break;

                case DIRT_TYPE_SNOW:
                case DIRT_TYPE_LEAF:
                    dd->type = type;
                    dd->owner = 255;
                    dd->flag = DIRT_FLAG_STILL;
                    dd->x = cx;
                    dd->z = cz;
                    dd->y = PAP_calc_height_at(cx, cz);
                    if (type == DIRT_TYPE_SNOW) {
                        if ((GAME_TURN > 100) && (Random() & 1)) {
                            dd->y = NET_PERSON(0)->WorldPos.Y >> 8;
                            dd->y += 700 + (Random() & 0x1ff);
                            dd->flag = 0;
                            dd->UU.Leaf.fade = 0xff;
                        } else
                            dd->UU.Leaf.fade = Random() & 0xff;
                    }
                    dd->dx = 0;
                    dd->dz = 0;
                    dd->dy = 0;
                    dd->yaw = 0;
                    dd->pitch = 0;
                    dd->roll = 0;
                    dd->dyaw = 0;
                    dd->dpitch = 0;
                    dd->droll = 0;
                    break;

                case DIRT_TYPE_CAN:
                    dd->type = DIRT_TYPE_CAN;
                    dd->flag = DIRT_FLAG_STILL;
                    dd->x = cx;
                    dd->y = PAP_calc_height_at(cx, cz) + 4;
                    dd->z = cz;
                    dd->dx = 0;
                    dd->dy = 0;
                    dd->dz = 0;
                    dd->yaw = Random() & 2047;
                    dd->pitch = 0;
                    dd->roll = 0;
                    dd->dyaw = 0;
                    dd->dpitch = 0;
                    dd->droll = 0;
                    break;

                case DIRT_TYPE_PIGEON:
                    dd->type = DIRT_TYPE_PIGEON;
                    dd->flag = 0;
                    dd->x = cx;
                    dd->y = PAP_calc_height_at(cx, cz);
                    dd->z = cz;
                    dd->UU.Pidgeon.state = DIRT_PIGEON_WAIT;
                    dd->counter = 16;
                    dd->UU.Pidgeon.morph1 = MORPH_PIGEON_STAND;
                    dd->UU.Pidgeon.morph2 = MORPH_PIGEON_STAND;
                    dd->UU.Pidgeon.tween = 0;
                    dd->dx = 0;
                    dd->dy = 0;
                    dd->dz = 0;
                    dd->yaw = Random() & 2047;
                    dd->pitch = 0;
                    dd->roll = 0;
                    dd->dyaw = 0;
                    dd->dpitch = 0;
                    dd->droll = 0;
                    break;

                default:
                    ASSERT(0);
                    break;
                }
            } else {
                dd->type = DIRT_TYPE_UNUSED;
            }
        }
    }

    if (DIRT_focus_first) {
        DIRT_focus_first -= 1;
    }
}

// uc_orig: DIRT_pigeon_init_wait (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_wait(DIRT_Dirt* dd)
{
    dd->UU.Pidgeon.state = DIRT_PIGEON_WAIT;
    dd->counter = Random() & 0x1f;
    dd->counter += 16;
    dd->UU.Pidgeon.morph1 = MORPH_PIGEON_STAND;
    dd->UU.Pidgeon.morph2 = MORPH_PIGEON_STAND;
    dd->UU.Pidgeon.tween = Random() & 0xf;
    dd->UU.Pidgeon.tween += 5;
}

// uc_orig: DIRT_pigeon_init_peck (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_peck(DIRT_Dirt* dd)
{
    dd->UU.Pidgeon.state = DIRT_PIGEON_PECK;
    dd->counter = Random() & 0x3;
    dd->counter += 9;
    dd->UU.Pidgeon.morph1 = MORPH_PIGEON_STAND;
    dd->UU.Pidgeon.morph2 = MORPH_PIGEON_PECK;
    dd->UU.Pidgeon.tween = 0;
}

// uc_orig: DIRT_pigeon_init_walk (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_walk(DIRT_Dirt* dd)
{
    SLONG mx;
    SLONG mz;

    SLONG dest_x;
    SLONG dest_z;

    SLONG half_x;
    SLONG half_z;

    SLONG dx;
    SLONG dz;

    SLONG len;
    SLONG count = 0;

    while (1) {
        count += 1;

        if (count > 5) {
            DIRT_pigeon_init_wait(dd);
            return;
        }

        if (count == 1 && dd->UU.Pidgeon.state == DIRT_PIGEON_RUN) {
            dest_x = dd->x + (dd->dx << 4);
            dest_z = dd->z + (dd->dz << 4);
        } else {
            dest_x = dd->x;
            dest_z = dd->z;

            dest_x += Random() & 0x3ff;
            dest_z += Random() & 0x3ff;

            dest_x -= 0x1ff;
            dest_z -= 0x1ff;
        }

        mx = dest_x >> 8;
        mz = dest_z >> 8;

        if (PAP_on_map_hi(mx, mz)) {
            if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
                // Inside a building.
            } else {
                half_x = dd->x + dest_x >> 9;
                half_z = dd->z + dest_z >> 9;

                if (PAP_2HI(half_x, half_z).Flags & PAP_FLAG_HIDDEN) {
                    // Inside a building.
                } else {
                    break;
                }
            }
        }
    }

    dx = dest_x - dd->x;
    dz = dest_z - dd->z;

    len = QDIST2(abs(dx), abs(dz)) + 1;

    dx = (dx << 2) / len;
    dz = (dz << 2) / len;

    dd->UU.Pidgeon.state = DIRT_PIGEON_WALK;
    dd->counter = len >> 2;
    dd->UU.Pidgeon.morph1 = MORPH_PIGEON_WALK1;
    dd->UU.Pidgeon.morph2 = MORPH_PIGEON_WALK2;
    dd->UU.Pidgeon.tween = 0;
    dd->dx = dx;
    dd->dz = dz;
    dd->yaw = calc_angle(-dx, -dz);
}

// uc_orig: DIRT_pigeon_init_hop (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_hop(DIRT_Dirt* dd, UBYTE upordown)
{
    ASSERT(dd->UU.Pidgeon.state == DIRT_PIGEON_WALK || dd->UU.Pidgeon.state == DIRT_PIGEON_RUN);

    dd->UU.Pidgeon.state = DIRT_PIGEON_HOP;
    dd->counter += 4;
    dd->UU.Pidgeon.morph1 = MORPH_PIGEON_WALK1;
    dd->UU.Pidgeon.morph2 = MORPH_PIGEON_WALK1;
    dd->UU.Pidgeon.tween = 10;

    switch (upordown) {
    case DIRT_PIGEON_HOPUP:
        dd->dy = 20 << TICK_SHIFT;
        break;
    case DIRT_PIGEON_HOPDOWN:
        dd->dy = 8 << TICK_SHIFT;
        break;
    default:
        ASSERT(0);
    }
}

// uc_orig: DIRT_pigeon_init_flee (fallen/Source/dirt.cpp)
static void DIRT_pigeon_init_flee(DIRT_Dirt* dd, SLONG scare_x, SLONG scare_z)
{
    SLONG dx;
    SLONG dz;

    SLONG mx;
    SLONG mz;

    SLONG dest_x;
    SLONG dest_z;

    SLONG half_x;
    SLONG half_z;

    SLONG len;
    SLONG overlen;

    dx = dd->x - scare_x;
    dz = dd->z - scare_z;

    len = QDIST2(abs(dx), abs(dz)) + 1;
    overlen = 65536 / len;

    dx = dx * overlen >> 8;
    dz = dz * overlen >> 8;

    dest_x = dd->x + dx;
    dest_z = dd->z + dz;

    mx = dest_x >> 8;
    mz = dest_z >> 8;

    if (PAP_on_map_hi(mx, mz)) {
        if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
            // Inside a building.
        } else {
            half_x = dd->x + dest_x >> 9;
            half_z = dd->z + dest_z >> 9;

            if (PAP_2HI(half_x, half_z).Flags & PAP_FLAG_HIDDEN) {
                // Inside a building.
            } else {
                dd->UU.Pidgeon.state = DIRT_PIGEON_RUN;
                dd->counter = len >> 4;
                dd->UU.Pidgeon.morph1 = MORPH_PIGEON_WALK1;
                dd->UU.Pidgeon.morph2 = MORPH_PIGEON_WALK2;
                dd->UU.Pidgeon.tween = 0;
                dd->dx = dx >> 4;
                dd->dz = dz >> 4;
                dd->yaw = calc_angle(-dx, -dz);

                return;
            }
        }
    }

    return;
}

// uc_orig: DIRT_pigeon_process_wait (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process_wait(DIRT_Dirt* dd)
{
    if (dd->UU.Pidgeon.tween == 0) {
        if (dd->UU.Pidgeon.morph1 == MORPH_PIGEON_STAND) {
            dd->UU.Pidgeon.morph1 = MORPH_PIGEON_HEADCOCK;
            dd->UU.Pidgeon.morph2 = MORPH_PIGEON_HEADCOCK;

            dd->UU.Pidgeon.tween = Random() & 0x7;
            dd->UU.Pidgeon.tween += 3;
        } else {
            dd->UU.Pidgeon.morph1 = MORPH_PIGEON_STAND;
            dd->UU.Pidgeon.morph2 = MORPH_PIGEON_STAND;
            dd->UU.Pidgeon.tween = Random() & 0xf;
            dd->UU.Pidgeon.tween += 5;
        }
    } else {
        dd->UU.Pidgeon.tween -= 1;
    }
}

// uc_orig: DIRT_pigeon_process_peck (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process_peck(DIRT_Dirt* dd)
{
    if (dd->UU.Pidgeon.tween < 255) {
        dd->UU.Pidgeon.tween += 1;
    }
}

// uc_orig: DIRT_pigeon_process_walkrun (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process_walkrun(DIRT_Dirt* dd)
{
    dd->UU.Pidgeon.tween += abs(dd->dx) + abs(dd->dz) << 3;

    dd->x += dd->dx;
    dd->z += dd->dz;
    dd->y = PAP_calc_height_at(dd->x, dd->z);

    SLONG mx1 = dd->x >> 8;
    SLONG mz1 = dd->z >> 8;
    SLONG mx2 = dd->x + (dd->dx * 4) >> 8;
    SLONG mz2 = dd->z + (dd->dz * 4) >> 8;

    ASSERT(WITHIN(mx1, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(mz1, 0, MAP_HEIGHT - 1));

    ASSERT(WITHIN(mx2, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(mz2, 0, MAP_HEIGHT - 1));

    if (mx1 != mx2 || mz1 != mz2) {
        if ((PAP_2HI(mx1, mz1).Flags ^ PAP_2HI(mx2, mz2).Flags) & PAP_FLAG_SINK_SQUARE) {
            DIRT_pigeon_init_hop(dd, (PAP_2HI(mx1, mz1).Flags & PAP_FLAG_SINK_SQUARE) ? DIRT_PIGEON_HOPUP : DIRT_PIGEON_HOPDOWN);
        }
    }
}

// uc_orig: DIRT_pigeon_process_hop (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process_hop(DIRT_Dirt* dd)
{
    if (dd->UU.Pidgeon.tween > 0) {
        dd->UU.Pidgeon.tween -= 1;
        dd->counter += 1;
        return;
    }

    dd->x += dd->dx >> 1;
    dd->y += dd->dy >> TICK_SHIFT;
    dd->z += dd->dz >> 1;

    dd->dy -= 3 * TICK_RATIO;

    SLONG height = PAP_calc_height_at(dd->x, dd->z);

    if (dd->y <= height) {
        dd->UU.Pidgeon.state = DIRT_PIGEON_WALK;
        dd->UU.Pidgeon.morph1 = MORPH_PIGEON_WALK1;
        dd->UU.Pidgeon.morph2 = MORPH_PIGEON_WALK2;
        dd->UU.Pidgeon.tween = 0;
    }
}

// uc_orig: DIRT_pigeon_start_doing_something_new (fallen/Source/dirt.cpp)
static void DIRT_pigeon_start_doing_something_new(DIRT_Dirt* dd)
{
    SLONG i;
    SLONG num;
    SLONG total;
    SLONG state;

    ASSERT(dd->type == DIRT_TYPE_PIGEON);

    UBYTE chance[DIRT_PIGEON_NUM_STATES];

    memset(chance, 0, sizeof(chance));

    switch (dd->UU.Pidgeon.state) {
    case DIRT_PIGEON_WAIT:
        chance[DIRT_PIGEON_WAIT] = 2;
        chance[DIRT_PIGEON_PECK] = 2;
        chance[DIRT_PIGEON_WALK] = 2;
        break;

    case DIRT_PIGEON_PECK:
        chance[DIRT_PIGEON_WAIT] = 1;
        chance[DIRT_PIGEON_PECK] = 3;
        break;

    case DIRT_PIGEON_WALK:
        chance[DIRT_PIGEON_WAIT] = 1;
        chance[DIRT_PIGEON_PECK] = 1;
        break;

    case DIRT_PIGEON_HOP:
        return;

    case DIRT_PIGEON_RUN:
        chance[DIRT_PIGEON_WALK] = 1;
        break;

    default:
        ASSERT(0);
        break;
    }

    total = 0;

    for (i = 0; i < DIRT_PIGEON_NUM_STATES; i++) {
        total += chance[i];
    }

    num = Random() % total;

    for (i = 0; i < DIRT_PIGEON_NUM_STATES; i++) {
        if (num < chance[i]) {
            state = i;
            break;
        } else {
            num -= chance[i];
        }
    }

    switch (state) {
    case DIRT_PIGEON_WAIT:
        DIRT_pigeon_init_wait(dd);
        break;
    case DIRT_PIGEON_PECK:
        DIRT_pigeon_init_peck(dd);
        break;
    case DIRT_PIGEON_WALK:
        DIRT_pigeon_init_walk(dd);
        break;
    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: DIRT_pigeon_process (fallen/Source/dirt.cpp)
static void DIRT_pigeon_process(DIRT_Dirt* dd)
{
    ASSERT(dd->type == DIRT_TYPE_PIGEON);

    if (dd->counter == 0) {
        DIRT_pigeon_start_doing_something_new(dd);
    } else {
        dd->counter -= 1;
    }

    switch (dd->UU.Pidgeon.state) {
    case DIRT_PIGEON_WAIT:
        DIRT_pigeon_process_wait(dd);
        break;

    case DIRT_PIGEON_PECK:
        DIRT_pigeon_process_peck(dd);
        break;

    case DIRT_PIGEON_WALK:
    case DIRT_PIGEON_RUN:
        DIRT_pigeon_process_walkrun(dd);
        break;

    case DIRT_PIGEON_HOP:
        DIRT_pigeon_process_hop(dd);
        break;

    default:
        ASSERT(0);
        break;
    }
}

// Creates a water/blood/urine/sparks droplet with given position and velocity.
// Randomizes velocity slightly. dy is scaled by TICK_SHIFT_LOWRES.
// uc_orig: DIRT_new_water (fallen/Source/dirt.cpp)
void DIRT_new_water(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG dx,
    SLONG dy,
    SLONG dz,
    SLONG dirt_type)
{
    DIRT_Dirt* dd;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    dd = DIRT_find_useless();

    dx <<= 4;
    dy <<= 4;
    dz <<= 4;

    dx += (Random() & 0x1f);
    dy += (Random() & 0x1f);
    dz += (Random() & 0x1f);

    dx -= 0xf;
    dy -= 0xf;
    dz -= 0xf;

    dd->type = dirt_type;

    dd->flag = 0;
    dd->x = x;
    dd->y = y;
    dd->z = z;
    dd->dx = dx;
    dd->dy = dy << TICK_SHIFT_LOWRES;
    dd->dz = dz;
    dd->dyaw = 0;

    if (dd->type == DIRT_TYPE_SPARKS)
        dd->UU.ThingWithTime.timer = 20 + (Random() & 7);
    else if (dd->type == DIRT_TYPE_BLOOD)
        dd->flag = DIRT_FLAG_HIT_FLOOR;

    return;
}

// Creates 4 spark dirt entries plus particle system sparks.
// dir encodes axis (0-5) and optional boost (bits 6-7) and no-drip (bit 5).
// uc_orig: DIRT_new_sparks (fallen/Source/dirt.cpp)
void DIRT_new_sparks(SLONG px, SLONG py, SLONG pz, UBYTE dir)
{
    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    SLONG dx, dy, dz, i, boost, nodrip;
    if (dir & 32) {
        nodrip = 1;
        dir &= ~32;
    }
    if (dir & (128 | 64)) {
        boost = dir >> 6;
        dir &= 63;
    } else
        boost = 0;
    for (i = 0; i < 4; i++) {
        switch (dir) {
        case 0: // X+
            dx = 5 + (Random() & 3);
            dy = (Random() & 7) - 3;
            dz = (Random() & 7) - 3;
            break;
        case 1: // X-
            dx = -(5 + (Random() & 3));
            dy = (Random() & 7) - 3;
            dz = (Random() & 7) - 3;
            break;
        case 2: // Y+
            dy = 5 + (Random() & 3);
            dx = (Random() & 7) - 3;
            dz = (Random() & 7) - 3;
            break;
        case 3: // Y-
            dy = -(5 + (Random() & 3));
            dx = (Random() & 7) - 3;
            dz = (Random() & 7) - 3;
            break;
        case 4: // Z+
            dz = 5 + (Random() & 3);
            dx = (Random() & 7) - 3;
            dy = (Random() & 7) - 3;
            break;
        case 5: // Z-
            dz = -(5 + (Random() & 3));
            dx = (Random() & 7) - 2;
            dy = (Random() & 7) - 2;
            break;
        }
        if (boost) {
            dx <<= boost;
            dy <<= boost;
            dz <<= boost;
        }
        if (!nodrip)
            DIRT_new_water(px, py, pz, dx, dy, dz, DIRT_TYPE_SPARKS);
        PARTICLE_Add(px << 8, py << 8, pz << 8, dx << 9, dy << 9, dz << 9, POLY_PAGE_EXPLODE1_ADDITIVE, 2 + ((Random() & 3) << 2), 0x7Fffffff, PFLAG_FADE | PFLAG_GRAVITY | PFLAG_BOUNCE, 20, 10, 1, 2 + (Random() & 7), 0);
    }
    if (Random() & 1)
        PARTICLE_Add(px << 8, py << 8, pz << 8, 0, 0, 0, POLY_PAGE_EXPLODE1_ADDITIVE, 2 + ((Random() & 3) << 2), 0xFFffffff, PFLAG_FADE, 2, 15 + (Random() & 0x3f), 1, 0x7f, 0);
    else
        PARTICLE_Add(px << 8, py << 8, pz << 8, 0, 0, 0, POLY_PAGE_EXPLODE2_ADDITIVE, 2 + ((Random() & 1) << 2), 0xFFffffff, PFLAG_FADE, 2, 15 + (Random() & 0x3f), 1, 0x7f, 0);
}

// Emits a shower of particle sparks from the given spark dirt entry's position.
// uc_orig: DIRT_spark_shower (fallen/Source/dirt.cpp)
static void DIRT_spark_shower(DIRT_Dirt* dd)
{
    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    UBYTE i;
    for (i = 0; i < 5; i++) {
        if (Random() & 1)
            PARTICLE_Add(dd->x << 8, (dd->y + 10) << 8, dd->z << 8, ((Random() & 0x3f) - 0x1f) << 4, ((Random() & 0x3f) + 0x1f) << 4, ((Random() & 0x3f) - 0x2f) << 4, POLY_PAGE_EXPLODE1_ADDITIVE, 2 | ((Random() & 3) << 2), 0x7Fffffff, PFLAG_FADE | PFLAG_GRAVITY | PFLAG_BOUNCE, 20, 10, 1, 2 + (Random() & 7), 0);
        else
            PARTICLE_Add(dd->x << 8, (dd->y + 10) << 8, dd->z << 8, ((Random() & 0x3f) - 0x1f) << 4, dd->dy >> 4, ((Random() & 0x3f) - 0x1f) << 4, POLY_PAGE_EXPLODE1_ADDITIVE, 2 | ((Random() & 3) << 2), 0x7Fffffff, PFLAG_FADE | PFLAG_GRAVITY | PFLAG_BOUNCE, 20, 10, 1, 2 + (Random() & 7), 0);
    }
}

// Per-frame physics update for all dirt entries.
// Handles leaves/snow, cans/heads/brass, pigeons, sparks, water/urine/blood, held/thrown objects.
// uc_orig: DIRT_process (fallen/Source/dirt.cpp)
void DIRT_process(void)
{
    SLONG i;
    SLONG dy;
    SLONG newy;
    SLONG floor;
    SLONG under;
    SLONG waftz;
    SLONG wafty;
    SLONG waftx;
    SLONG mx;
    SLONG mz;
    SLONG speed;
    SLONG oldx;
    SLONG oldy;
    SLONG oldz;
    SLONG collided;

    DIRT_Dirt* dd;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    for (i = 0; i < DIRT_MAX_DIRT; i++) {
        dd = &DIRT_dirt[i];

        if (dd->type == DIRT_TYPE_UNUSED) {
            continue;
        }

        if (dd->flag & DIRT_FLAG_STILL) {
            if (dd->type == DIRT_TYPE_SNOW) {
                if (dd->UU.Leaf.fade > 4)
                    dd->UU.Leaf.fade -= 4;
                else
                    dd->type = DIRT_TYPE_UNUSED;
            }

            continue;
        }

        switch (dd->type) {
        case DIRT_TYPE_LEAF:
        case DIRT_TYPE_SNOW:

            oldx = dd->x;
            oldy = dd->y;
            oldz = dd->z;

            dd->x += (TICK_RATIO * dd->dx) >> TICK_SHIFT;
            dd->y += (TICK_RATIO * (dd->dy >> TICK_SHIFT)) >> TICK_SHIFT;
            dd->z += (TICK_RATIO * dd->dz) >> TICK_SHIFT;

            dd->yaw += (TICK_RATIO * dd->dyaw) >> TICK_SHIFT;
            dd->pitch += (TICK_RATIO * dd->dpitch) >> TICK_SHIFT;
            dd->roll += (TICK_RATIO * dd->droll) >> TICK_SHIFT;

            mx = dd->x >> 8;
            mz = dd->z >> 8;

            if (((oldx >> 8) != (dd->x >> 8)) || (oldz >> 8) != (dd->z >> 8)) {
                if (PAP_on_map_hi(mx, mz) && (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN)) {
                    dd->x = oldx;
                    dd->dx = -dd->dx;
                    dd->z = oldz;
                    dd->dz = -dd->dz;
                } else {
                }
            }

            floor = PAP_calc_height_at(dd->x, dd->z);

            if (dd->y <= floor) {
                dd->y = floor;
                dd->dy = 0;
                dd->yaw = 0;
                dd->roll = 0;
                dd->pitch = 0;
                dd->dyaw = 0;
                dd->dpitch = 0;
                dd->droll = 0;

                if (abs(dd->dx) <= 3) {
                    dd->dx = 0;
                }
                if (abs(dd->dz) <= 3) {
                    dd->dz = 0;
                }

                if (dd->dx == 0 && dd->dz == 0) {
                    dd->flag |= DIRT_FLAG_STILL;
                }
            }

            dd->dx -= dd->dx / 4;
            dd->dy -= dd->dy / 2;
            dd->dz -= dd->dz / 4;

            dd->dpitch -= dd->dpitch / 32;
            dd->droll -= dd->droll / 32;

            waftz = dd->pitch / 32;
            waftx = dd->roll / 32;

            SATURATE(waftz, -0x5, +0x5);
            SATURATE(waftx, -0x5, +0x5);

            dd->dz -= waftz;
            dd->dx += waftx;

            dd->dpitch -= 0xa * SIGN(dd->pitch);
            dd->droll -= 0xa * SIGN(dd->roll);

            dd->dy -= 4 << TICK_SHIFT;
            dd->dy += MIN(abs(dd->pitch), 180) << (TICK_SHIFT - 5);
            dd->dy += MIN(abs(dd->roll), 180) << (TICK_SHIFT - 5);

            break;

        case DIRT_TYPE_CAN:
        case DIRT_TYPE_HEAD:
        case DIRT_TYPE_BRASS:

            dd->yaw += dd->dyaw;
            dd->pitch += dd->dpitch;

            dd->yaw &= 2047;
            dd->pitch &= 2047;

            oldx = dd->x;
            oldz = dd->z;

            dd->x += dd->dx >> 8;
            dd->z += dd->dz >> 8;

            mx = dd->x >> 8;
            mz = dd->z >> 8;

            newy = PAP_calc_map_height_at(dd->x, dd->z) + 6;

            if (dd->type == DIRT_TYPE_BRASS) {
                newy -= 3;
            } else if (dd->type == DIRT_TYPE_HEAD) {
                newy += 5;
            }

            dy = newy - dd->y;

            if (PAP_on_map_hi(mx, mz)) {
                if ((dy > 8) && (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN)) {
                    if ((dd->x - (dd->dx >> 8) >> 8) != mx) {
                        dd->dx = -dd->dx;
                    }
                    if ((dd->z - (dd->dz >> 8) >> 8) != mz) {
                        dd->dz = -dd->dz;
                    }

                    dd->x += dd->dx >> 7;
                    dd->z += dd->dz >> 7;

                    dd->dyaw += 200;
                    dd->dpitch = 0;

                    break;
                }
            }

            if (dd->y > newy) {
                dd->dy -= 4 << TICK_SHIFT;
                dd->y += dd->dy >> TICK_SHIFT;
            }
            if (dd->y < newy) {
                dd->y = newy;
                if (dd->type != DIRT_TYPE_BRASS)
                    if (dd->dy < -8 << TICK_SHIFT) {
                        MFX_play_xyz(i, S_KICK_CAN, MFX_REPLACE, dd->x << 8, dd->y << 8, dd->z << 8);
                    }

                dd->dy = 0;
            }

            dd->dx -= dd->dx / 16;
            dd->dz -= dd->dz / 16;

            dd->dx -= SIGN(dd->dx);
            dd->dz -= SIGN(dd->dz);

            dd->dyaw -= dd->dyaw / 32;
            dd->dpitch -= dd->dpitch / 32;

            dd->dyaw -= SIGN(dd->dyaw);
            dd->dpitch -= SIGN(dd->dpitch);

            if (dd->dx == 0 && dd->dz == 0 && dd->dyaw == 0 && dd->dpitch == 0) {
                dd->flag |= DIRT_FLAG_STILL;
            }

            break;

        case DIRT_TYPE_PIGEON:
            DIRT_pigeon_process(dd);
            break;

        case DIRT_TYPE_SPARKS:

            if (--dd->UU.ThingWithTime.timer < 1) {
                DIRT_spark_shower(dd);
                dd->type = DIRT_TYPE_UNUSED;
                break;
            }

            // FALL THRU!
        case DIRT_TYPE_WATER:
        case DIRT_TYPE_URINE:
        case DIRT_TYPE_BLOOD:

            dy = dd->dy;
            dy -= 15 * TICK_RATIO >> 2;

            if ((dy >> TICK_SHIFT_LOWRES) < -511)
                dy = -511 << TICK_SHIFT_LOWRES;

            dd->dy = dy;

            oldx = dd->x;
            oldy = dd->y;
            oldz = dd->z;

            dd->x += (TICK_RATIO * (dd->dx + ((Random() & 0x1f) - 0xf))) / (1 << (TICK_SHIFT + 4));
            dd->y += (TICK_RATIO * ((dd->dy >> TICK_SHIFT_LOWRES) + ((Random() & 0x1f) - 0xf))) / (1 << (TICK_SHIFT + 4));
            dd->z += (TICK_RATIO * (dd->dz + ((Random() & 0x1f) - 0xf))) / (1 << (TICK_SHIFT + 4));

            mx = dd->x >> 8;
            mz = dd->z >> 8;

            collided = UC_FALSE;

            if (PAP_on_map_hi(mx, mz)) {
                {
                    collided = (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN);
                }
            } else {
                collided = UC_TRUE;
            }

            if (collided) {
                if ((oldx >> 8) != (dd->x >> 8)) {
                    dd->x = oldx;
                    dd->dx = -dd->dx >> 2;
                    dd->dz = COS((i * 97) & 2047) >> 10;
                }

                if ((oldz >> 8) != (dd->z >> 8)) {
                    dd->z = oldz;
                    dd->dz = -dd->dz >> 2;
                    dd->dx = COS((i * 97) & 2047) >> 10;
                }
            }

            if (GAME_FLAGS & GF_SEWERS) {
                floor = NS_calc_splash_height_at(dd->x, dd->z);
            } else {
                floor = PAP_calc_map_height_at(dd->x, dd->z);
            }

            if (dd->y <= floor) {
                static int tick = 0;

                dd->y = floor + 1;

                if (dd->flag & DIRT_FLAG_HIT_FLOOR) {
                    if (dd->type != DIRT_TYPE_BLOOD) {
                        if (dd->type != DIRT_TYPE_SPARKS) {
                            if (tick++ & 1)
                                DRIP_create(dd->x, dd->y, dd->z, 0);
                        } else
                            DIRT_spark_shower(dd);
                    }

                    dd->type = DIRT_TYPE_UNUSED;

                } else {
                    dd->dy = abs(dd->dy) >> 1;
                    dd->dx = SIN((i * 97) & 2047) >> 10;
                    dd->dz = COS((i * 97) & 2047) >> 10;

                    dd->flag |= DIRT_FLAG_HIT_FLOOR;

                    if (dd->type != DIRT_TYPE_BLOOD) {
                        if (dd->type != DIRT_TYPE_SPARKS) {
                            if (tick++ & 1)
                                DRIP_create(dd->x, dd->y, dd->z, 0);
                        } else
                            DIRT_spark_shower(dd);
                    }
                }
            }

            break;

        case DIRT_TYPE_HELDCAN:
        case DIRT_TYPE_HELDHEAD:

            {
                SLONG px;
                SLONG py;
                SLONG pz;

                Thing* p_person = TO_THING(dd->droll);

                calc_sub_objects_position(
                    p_person,
                    p_person->Draw.Tweened->AnimTween,
                    SUB_OBJECT_PREFERRED_HAND,
                    &px,
                    &py,
                    &pz);

                px += p_person->WorldPos.X >> 8;
                py += p_person->WorldPos.Y >> 8;
                pz += p_person->WorldPos.Z >> 8;

                dd->x = px;
                dd->y = py;
                dd->z = pz;

                dd->yaw = p_person->Draw.Tweened->Angle;
                dd->pitch = 0;
                dd->roll = 0;
            }

            break;

        case DIRT_TYPE_THROWCAN:
        case DIRT_TYPE_THROWHEAD:
        case DIRT_TYPE_MINE:

        {
            dd->dy -= TICK_RATIO;

            if (dd->dy < -0x20 << TICK_SHIFT) {
                dd->dy = -0x20 << TICK_SHIFT;
            }

            oldx = dd->x;
            oldy = dd->y;
            oldz = dd->z;

            dd->x += dd->dx * TICK_RATIO >> TICK_SHIFT;
            dd->y += (dd->dy >> TICK_SHIFT) * TICK_RATIO >> TICK_SHIFT;
            dd->z += dd->dz * TICK_RATIO >> TICK_SHIFT;

            dd->yaw += dd->dyaw;
            dd->pitch += dd->dpitch;

            floor = PAP_calc_map_height_at(dd->x, dd->z) + 6;

            if (dd->type != DIRT_TYPE_THROWCAN) {
                floor += 5;
            }

            if (dd->y < floor) {
                under = floor - dd->y;

                if (abs(dd->dy) > 8 << TICK_SHIFT) {
                    MFX_play_xyz(i, S_KICK_CAN, MFX_REPLACE, dd->x << 8, dd->y << 8, dd->z << 8);

                    PCOM_oscillate_tympanum(
                        PCOM_SOUND_UNUSUAL,
                        TO_THING(dd->droll),
                        oldx,
                        oldy,
                        oldz);
                }

                if ((under > 0x40) || (dd->dy > 0)) {
                    if ((oldx >> 8) != (dd->x >> 8)) {
                        dd->x = oldx;
                        dd->dx = -dd->dx >> 1;
                    }

                    if ((oldz >> 8) != (dd->z >> 8)) {
                        dd->z = oldz;
                        dd->dz = -dd->dz >> 1;
                    }
                } else {
                    dd->dy = abs(dd->dy) >> 1;
                    dd->y = 2 * floor - dd->y;

                    if (abs(dd->dy) < 10 << TICK_SHIFT) {
                        if (dd->type == DIRT_TYPE_THROWCAN) {
                            dd->dy = 0;
                            dd->type = DIRT_TYPE_CAN;

                            dd->dx += (Random() & 0xf) - 0x7;
                            dd->dz += (Random() & 0xf) - 0x7;

                            dd->dx <<= 8;
                            dd->dz <<= 8;

                            dd->dyaw = (Random() & 0x7f) + 0x7f;
                            dd->dpitch = (Random() & 0x3f) + 0x3f;
                        } else if (dd->type == DIRT_TYPE_THROWHEAD) {
                            dd->type = DIRT_TYPE_HEAD;

                            dd->dx = 0;
                            dd->dy = 0;
                            dd->dz = 0;

                            dd->dyaw = 0;
                            dd->dpitch = 0;
                        }
                    } else {
                        dd->dx += (Random() & 0xf) - 0x7;
                        dd->dz += (Random() & 0xf) - 0x7;

                        dd->dyaw += (Random() & 0x7f) - 0x3f;
                        dd->dpitch += (Random() & 0x3f);
                    }
                }
            }
        }

        break;

        default:
            ASSERT(0);
            break;
        }
    }
}

// Pushes nearby dirt away from the point (x1,z1) in the direction of (x2-x1, z2-z1).
// Strength proportional to QDIST2 of the vector. Only affects dirt near the camera focus.
// uc_orig: DIRT_gust (fallen/Source/dirt.cpp)
void DIRT_gust(
    Thing* p_thing,
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG i;
    SLONG dx;
    SLONG dz;
    SLONG dist;

    SLONG push;
    SLONG pushx;
    SLONG pushy;
    SLONG pushz;

    SLONG dpitch;
    SLONG droll;

    DIRT_Dirt* dd;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    dx = DIRT_focus_x - x1;
    dz = DIRT_focus_z - z1;

    dist = abs(dx) + abs(dz);

    if (dist > (DIRT_focus_radius)) {
        return;
    }

    SLONG dgx = x2 - x1;
    SLONG dgz = z2 - z1;

    dgx -= dgx / 4;
    dgz -= dgz / 4;

    SLONG strength;

    strength = QDIST2(abs(dgx), abs(dgz));
    strength += 1;
    strength *= 8;

    for (i = 0; i < DIRT_MAX_DIRT; i++) {
        dd = &DIRT_dirt[i];

        if (dd->type == DIRT_TYPE_UNUSED) {
            continue;
        }

        dx = dd->x - x1;
        dz = dd->z - z1;

        dist = QDIST2(abs(dx), abs(dz)) + 1;

        if (strength > dist) {
            switch (dd->type) {
            case DIRT_TYPE_SNOW:

            case DIRT_TYPE_LEAF:

                pushx = dx * 32 / dist;
                pushz = dz * 32 / dist;

                pushx += dgx;
                pushz += dgz;

                push = 256 - dist * 256 / strength;

                if (push) {
                    pushx = pushx * push >> 12;
                    pushz = pushz * push >> 12;

                    pushy = abs(pushx) + abs(pushz);
                    pushy <<= 1;

                    dpitch = (Random() & 0x3f) - 0x1f;
                    droll = (Random() & 0x3f) - 0x1f;

                    pushx = pushx * TICK_INV_RATIO >> TICK_SHIFT;
                    pushy = pushy * TICK_INV_RATIO >> TICK_SHIFT;
                    pushz = pushz * TICK_INV_RATIO >> TICK_SHIFT;

                    dpitch = dpitch * TICK_INV_RATIO >> TICK_SHIFT;
                    droll = droll * TICK_INV_RATIO >> TICK_SHIFT;

                    dd->dx += pushx;
                    dd->dy += pushy << TICK_SHIFT;
                    dd->dz += pushz;

                    dd->dpitch += dpitch;
                    dd->droll += droll;

                    dd->flag &= ~DIRT_FLAG_STILL;
                }

                break;

            case DIRT_TYPE_CAN:
            case DIRT_TYPE_HEAD:
            case DIRT_TYPE_BRASS:

                if (dist < 32) {
                    dd->dx = (dx << 13) / dist;
                    dd->dz = (dz << 13) / dist;
                    dd->dyaw = Random() & 0xff;
                    dd->dyaw += 0x7f;
                    dd->dpitch = -400;
                    dd->flag = 0;

                    if (dd->type != DIRT_TYPE_BRASS) {
                        MFX_play_xyz(i, S_KICK_CAN, MFX_REPLACE, dd->x << 8, dd->y << 8, dd->z << 8);

                        PCOM_oscillate_tympanum(
                            PCOM_SOUND_UNUSUAL,
                            p_thing,
                            x1,
                            0,
                            z1);
                    }
                }

                break;

            case DIRT_TYPE_PIGEON:
                DIRT_pigeon_init_flee(dd, x1, z1);
                break;

            case DIRT_TYPE_WATER:
                break;

            case DIRT_TYPE_HELDCAN:
                break;

            case DIRT_TYPE_THROWCAN:
                break;

            case DIRT_TYPE_HELDHEAD:
                break;

            case DIRT_TYPE_THROWHEAD:
                break;

            case DIRT_TYPE_MINE:
                break;

            case DIRT_TYPE_URINE:
                break;

            case DIRT_TYPE_SPARKS:
                break;

            case DIRT_TYPE_BLOOD:
                break;

            default:
                ASSERT(0);
                break;
            }
        }
    }
}

// DIRT_gale_height and DIRT_gale are in /* */ block in original — dead code, not migrated.

// uc_orig: DIRT_get_nearest_can_or_head_dist (fallen/Source/dirt.cpp)
SLONG DIRT_get_nearest_can_or_head_dist(SLONG x, SLONG y, SLONG z)
{
    SLONG i;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;
    SLONG best_dist = UC_INFINITY;

    DIRT_Dirt* dd;

    for (i = 0; i < DIRT_MAX_DIRT; i++) {
        dd = &DIRT_dirt[i];

        if (dd->type == DIRT_TYPE_CAN) {
            dx = abs(dd->x - x);
            dy = abs(dd->y - y);
            dz = abs(dd->z - z);

            dist = QDIST3(dx, dy, dz);

            if (dist < best_dist) {
                best_dist = dist;
            }
        }
    }

    return best_dist;
}

// uc_orig: DIRT_pick_up_can_or_head (fallen/Source/dirt.cpp)
void DIRT_pick_up_can_or_head(Thing* p_person)
{
    SLONG i;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;

    SLONG best_can = NULL;
    SLONG best_dist = UC_INFINITY;

    DIRT_Dirt* dd;

    for (i = 0; i < DIRT_MAX_DIRT; i++) {
        dd = &DIRT_dirt[i];

        if (dd->type == DIRT_TYPE_CAN || dd->type == DIRT_TYPE_HEAD) {
            dx = abs(dd->x - (p_person->WorldPos.X >> 8));
            dy = abs(dd->y - (p_person->WorldPos.Y >> 8));
            dz = abs(dd->z - (p_person->WorldPos.Z >> 8));

            dist = QDIST3(dx, dy, dz);

            if (dist < best_dist) {
                best_can = i;
                best_dist = dist;
            }
        }
    }

    if (best_can && best_dist < 0x80) {
        dd = &DIRT_dirt[best_can];

        ASSERT(dd->type == DIRT_TYPE_CAN);

        dd->type = DIRT_TYPE_HELDCAN;
        dd->droll = THING_NUMBER(p_person);
        dd->flag &= ~DIRT_FLAG_STILL;

        p_person->Genus.Person->Flags |= FLAG_PERSON_CANNING;
        p_person->Genus.Person->Hold = best_can;
    }

    return;
}

// uc_orig: DIRT_release_can_or_head (fallen/Source/dirt.cpp)
void DIRT_release_can_or_head(Thing* p_person, SLONG power)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG vector[3];

    DIRT_Dirt* dd;

    FMATRIX_vector(
        vector,
        p_person->Draw.Tweened->Angle,
        0);

    dx = -vector[0] * power >> 18;
    dz = -vector[2] * power >> 18;

    dy = power >> 2;

    ASSERT(WITHIN(p_person->Genus.Person->Hold, 0, DIRT_MAX_DIRT - 1));

    dd = &DIRT_dirt[p_person->Genus.Person->Hold];

    ASSERT(dd->type == DIRT_TYPE_HELDCAN);

    dd->type = DIRT_TYPE_THROWCAN;
    dd->dx = dx;
    dd->dy = dy << TICK_SHIFT;
    dd->dz = dz;
    dd->dyaw = (Random() & 0x3f) - 0x1f;
    dd->dpitch = 50;

    p_person->Genus.Person->Flags &= ~FLAG_PERSON_CANNING;
}

// Returns rendering info for dirt entry 'which'. Returns 0 if nothing to draw.
// Clears DIRT_FLAG_DELETE_OK when called (marks as on-screen).
// uc_orig: DIRT_get_info (fallen/Source/dirt.cpp)
SLONG DIRT_get_info(SLONG which, DIRT_Info* ans)
{
    if (GAME_FLAGS & GF_NO_FLOOR) {
        return UC_FALSE;
    }

    DIRT_Dirt* dd;

    ASSERT(WITHIN(which, 0, DIRT_MAX_DIRT - 1));

    dd = &DIRT_dirt[which];

    dd->flag &= ~DIRT_FLAG_DELETE_OK;

    switch (dd->type) {
    case DIRT_TYPE_UNUSED:
        return (0);
        ans->type = DIRT_INFO_TYPE_UNUSED;
        break;

    case DIRT_TYPE_WATER:
        ans->type = DIRT_INFO_TYPE_WATER;
        // FALL THRU
    case DIRT_TYPE_SPARKS:
        if (dd->type == DIRT_TYPE_SPARKS) {
            ans->type = DIRT_INFO_TYPE_SPARKS;
        }
        // FALL THRU
    case DIRT_TYPE_BLOOD:
        if (dd->type == DIRT_TYPE_BLOOD) {
            ans->type = DIRT_INFO_TYPE_BLOOD;
        }
        // FALL THRU
    case DIRT_TYPE_URINE:
        ans->x = dd->x;
        ans->y = dd->y;
        ans->z = dd->z;
        ans->dx = dd->dx >> 4;
        ans->dy = dd->dy >> (TICK_SHIFT_LOWRES + 4);
        ans->dz = dd->dz >> 4;

        if (dd->type == DIRT_TYPE_URINE) {
            ans->type = DIRT_INFO_TYPE_URINE;
        }

        break;

    case DIRT_TYPE_LEAF:
    case DIRT_TYPE_SNOW:
        ans->tween = dd->UU.Leaf.col;
        if (dd->type == DIRT_TYPE_LEAF)
            ans->type = DIRT_INFO_TYPE_LEAF;
        else
            ans->type = DIRT_INFO_TYPE_SNOW;
        ans->yaw = dd->yaw;
        ans->pitch = dd->pitch;
        ans->roll = dd->roll;
        ans->x = dd->x;
        ans->y = dd->y;
        ans->z = dd->z;
        ans->morph1 = dd->UU.Leaf.fade;
        break;

    case DIRT_TYPE_CAN:
    case DIRT_TYPE_THROWCAN:
        ans->type = DIRT_INFO_TYPE_PRIM;
        ans->prim = PRIM_OBJ_CAN;
        ans->held = UC_FALSE;
        ans->yaw = dd->yaw;
        ans->pitch = dd->pitch;
        ans->roll = dd->roll;
        ans->x = dd->x;
        ans->y = dd->y;
        ans->z = dd->z;
        break;

    case DIRT_TYPE_HELDCAN: {
        Thing* p_person = TO_THING(dd->droll);

        if (p_person->Genus.Person->InCar)
            return (0);

        ans->type = DIRT_INFO_TYPE_PRIM;
        ans->prim = PRIM_OBJ_CAN;
        ans->held = UC_TRUE;
        ans->yaw = dd->yaw;
        ans->pitch = dd->pitch;
        ans->roll = dd->roll;
        ans->x = dd->x;
        ans->y = dd->y;
        ans->z = dd->z;
        break;
    }

    case DIRT_TYPE_PIGEON:
        ans->type = DIRT_INFO_TYPE_MORPH;
        ans->prim = PRIM_OBJ_ITEM_KEY;
        ans->morph1 = dd->UU.Pidgeon.morph1;
        ans->morph2 = dd->UU.Pidgeon.morph2;
        ans->tween = dd->UU.Pidgeon.tween;
        ans->yaw = dd->yaw;
        ans->pitch = dd->pitch;
        ans->roll = dd->roll;
        ans->x = dd->x;
        ans->y = dd->y;
        ans->z = dd->z;

        if (ans->tween == 255) {
            // tween is UBYTE but needs to represent 256.
            ans->tween = 256;
        }
        break;

    case DIRT_TYPE_HEAD:
    case DIRT_TYPE_THROWHEAD:
    case DIRT_TYPE_HELDHEAD:
    case DIRT_TYPE_BRASS:
        ans->type = DIRT_INFO_TYPE_PRIM;
        ans->prim = dd->UU.Head.prim;
        ans->held = UC_FALSE;
        ans->yaw = dd->yaw;
        ans->pitch = dd->pitch;
        ans->roll = dd->roll;
        ans->x = dd->x;
        ans->y = dd->y;
        ans->z = dd->z;
        break;

    case DIRT_TYPE_MINE:
        ans->type = DIRT_INFO_TYPE_UNUSED;
        ans->yaw = dd->yaw;
        ans->pitch = dd->pitch;
        ans->roll = dd->roll;
        ans->x = dd->x;
        ans->y = dd->y;
        ans->z = dd->z;
        return (0);

    default:
        ASSERT(0);
        break;
    }

    return (1);
}

// uc_orig: DIRT_mark_as_offscreen (fallen/Source/dirt.cpp)
void DIRT_mark_as_offscreen(SLONG which)
{
    DIRT_Dirt* dd;

    ASSERT(WITHIN(which, 0, DIRT_MAX_DIRT - 1));

    dd = &DIRT_dirt[which];

    if (dd->type == DIRT_TYPE_LEAF && dd->owner != 255) {
        // Tree-owned leaf — let the tree handle deletion.
    } else {
        DIRT_dirt[which].flag |= DIRT_FLAG_DELETE_OK;
    }
}

// Raycast from p_person's facing direction, returns UC_TRUE if a thrown can/head was hit.
// Hit can/head gets deflected with a random velocity + PYRO_hitspang + sound.
// uc_orig: DIRT_shoot (fallen/Source/dirt.cpp)
SLONG DIRT_shoot(Thing* p_person)
{
    SLONG i;
    SLONG dx;
    SLONG dz;
    SLONG dist;
    SLONG angle;
    SLONG dangle;

    SLONG score;
    SLONG best_dirt;
    SLONG best_score = -UC_INFINITY;

    DIRT_Dirt* dd;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return UC_FALSE;
    }

    for (i = 0; i < DIRT_MAX_DIRT; i++) {
        dd = &DIRT_dirt[i];

        if (dd->type == DIRT_TYPE_THROWCAN || dd->type == DIRT_TYPE_THROWHEAD) {
            dx = dd->x - (p_person->WorldPos.X >> 8);
            dz = dd->z - (p_person->WorldPos.Z >> 8);

            angle = calc_angle(dx, dz);
            angle += 1024;
            angle &= 2047;

            dist = abs(dx) + abs(dz);
            dangle = angle - p_person->Draw.Tweened->Angle;

            if (dangle < -1024) {
                dangle += 2048;
            }
            if (dangle > +1024) {
                dangle -= 2048;
            }

            if (WITHIN(dangle, -200, +200) && dist < 256 * 5) {
                score = UC_INFINITY - (dangle << 8) - (dist << 4);

                if (score > best_score) {
                    best_score = score;
                    best_dirt = i;
                }
            }
        }
    }

    if (best_score > -UC_INFINITY) {
        ASSERT(WITHIN(best_dirt, 0, DIRT_MAX_DIRT - 1));

        dd = &DIRT_dirt[best_dirt];

        switch (dd->type) {
        case DIRT_TYPE_THROWCAN:
        case DIRT_TYPE_THROWHEAD:

            dd->dx = (Random() & 0x1f) - 0xf;
            dd->dz = (Random() & 0x1f) - 0xf;

            dd->dy += (Random() & 0xf) << TICK_SHIFT;
            dd->dy += 0xf << TICK_SHIFT;

            dd->dyaw = Random() & 0x3f;
            dd->dyaw += 0x7f;

            if (Random() & 0x2) {
                dd->yaw = -dd->yaw;
            }
            PYRO_hitspang(p_person, dd->x << 8, dd->y << 8, dd->z << 8);
            MFX_play_xyz(best_dirt, S_KICK_CAN, MFX_REPLACE, dd->x << 8, dd->y << 8, dd->z << 8);

            break;

        default:
            break;
        }

        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: DIRT_behead_person (fallen/Source/dirt.cpp)
void DIRT_behead_person(Thing* p_person, Thing* p_attacker)
{
    // Stub in this codebase — no implementation.
}

// uc_orig: DIRT_create_mine (fallen/Source/dirt.cpp)
UWORD DIRT_create_mine(Thing* p_person)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG power = 128;

    SLONG vector[3];

    DIRT_Dirt* dd = DIRT_find_useless();

    FMATRIX_vector(
        vector,
        p_person->Draw.Tweened->Angle,
        0);

    dx = -vector[0] * power >> 18;
    dz = -vector[2] * power >> 18;

    dy = power >> 2;

    dd->type = DIRT_TYPE_MINE;
    dd->flag = 0;
    dd->dx = dx;
    dd->dy = dy << TICK_SHIFT;
    dd->dz = dz;
    dd->dyaw = (Random() & 0x3f) - 0x1f;
    dd->dpitch = 50;

    SLONG px;
    SLONG py;
    SLONG pz;

    calc_sub_objects_position(
        p_person,
        p_person->Draw.Tweened->AnimTween,
        SUB_OBJECT_LEFT_HAND,
        &px,
        &py,
        &pz);

    px += p_person->WorldPos.X >> 8;
    py += p_person->WorldPos.Y >> 8;
    pz += p_person->WorldPos.Z >> 8;

    dd->x = px;
    dd->y = py;
    dd->z = pz;

    dd->droll = THING_NUMBER(p_person);

    return dd - DIRT_dirt;
}

// uc_orig: DIRT_destroy_mine (fallen/Source/dirt.cpp)
void DIRT_destroy_mine(UWORD dirt_mine)
{
    ASSERT(WITHIN(dirt_mine, 0, DIRT_MAX_DIRT - 1));

    DIRT_dirt[dirt_mine].type = DIRT_TYPE_UNUSED;
}

// Creates up to 4 floating paper/leaf dirt pieces at the given position.
// Steps through pool at stride 16 for quick search.
// uc_orig: DIRT_create_papers (fallen/Source/dirt.cpp)
void DIRT_create_papers(
    SLONG x,
    SLONG y,
    SLONG z)
{
    SLONG i;
    SLONG created;

    DIRT_Dirt* dd;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    created = 0;

    for (i = 0; i < DIRT_MAX_DIRT; i += 16) {
        dd = &DIRT_dirt[i];

        if (dd->type == DIRT_TYPE_UNUSED || (dd->flag & DIRT_FLAG_DELETE_OK)) {
            dd->type = DIRT_TYPE_LEAF;
            dd->x = x + (Random() & 0x3f) - 0x1f;
            dd->z = z + (Random() & 0x3f) - 0x1f;
            dd->y = y + (Random() & 0x1f);
            dd->dy = Random() & 0x7;
            dd->droll = (Random() & 0x7f) - 0x3f;
            dd->dpitch = (Random() & 0x7f) - 0x3f;
            dd->owner = 255;
            dd->dx = 0;
            dd->dy = 0;
            dd->dz = 0;
            dd->flag = 0;

            created += 1;

            if (created >= 4) {
                return;
            }
        }
    }
}

// Creates 5 coke can dirt entries scattered around (x, z) with random velocities.
// uc_orig: DIRT_create_cans (fallen/Source/dirt.cpp)
void DIRT_create_cans(
    SLONG x,
    SLONG z,
    SLONG angle)
{
    SLONG i;
    SLONG useangle;

    DIRT_Dirt* dd;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    for (i = 0; i < 5; i++) {
        dd = DIRT_find_useless();

        if (dd == NULL) {
            return;
        }

        useangle = angle;
        useangle += Random() & 0x7f;
        useangle -= 0x3f;
        useangle &= 2047;

        dd->type = DIRT_TYPE_CAN;
        dd->flag = 0;
        dd->x = x + (Random() & 0x3f) - 0x1f;
        dd->y = PAP_calc_height_at(x, z) + 4;
        dd->z = z + (Random() & 0x3f) - 0x1f;
        dd->dx = SIN(useangle) * (Random() & 0xff) >> 13;
        dd->dy = 0;
        dd->dz = COS(useangle) * (Random() & 0xff) >> 13;
        dd->yaw = Random() & 2047;
        dd->pitch = -400;
        dd->roll = 0;
        dd->dyaw = (Random() & 0xff) - 0x7f;
        dd->dpitch = 0;
        dd->droll = 0;
    }
}

// Creates 1 ejected shell casing at (x,y,z) facing 'angle'.
// uc_orig: DIRT_create_brass (fallen/Source/dirt.cpp)
void DIRT_create_brass(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG angle)
{
    SLONG i;
    SLONG useangle;

    DIRT_Dirt* dd;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    {
        dd = DIRT_find_useless();

        if (dd == NULL) {
            return;
        }

        useangle = angle;
        useangle += Random() & 0x7f;
        useangle -= 0x3f;
        useangle &= 2047;

        i = 0x7f - (Random() & 0x3f);

        dd->type = DIRT_TYPE_BRASS;
        dd->UU.Head.prim = 253;
        dd->flag = 0;
        dd->x = x;
        dd->y = y;
        dd->z = z;
        dd->dx = (SIN(useangle) * i) >> 11;
        dd->dy = 24 << TICK_SHIFT;
        dd->dz = (COS(useangle) * i) >> 11;
        dd->yaw = (useangle + 512) & 2047;
        dd->pitch = 0;
        dd->roll = 0;
        dd->dyaw = 0;
        dd->dpitch = 0;
        dd->droll = 0;
    }
}

// DIRT_gale stub — original was in /* */ dead code block.
// uc_orig: DIRT_gale (fallen/Headers/dirt.h)
void DIRT_gale(SLONG dx, SLONG dz)
{
    // Dead in original: DIRT_gale_height was commented out.
}
