//
// Objects (prims) on the map.
//

// claude-ai: ============================================================
// claude-ai: FILE OVERVIEW: ob.cpp
// claude-ai: ============================================================
// claude-ai: Implements the OB (Object) system — static decorative/interactive
// claude-ai: objects placed on the outdoor map (street level). Examples:
// claude-ai: lamp posts, benches, bollards, fire hydrants, bins, traffic cones,
// claude-ai: switches, valves, tripwires, helicopter pads, weapon pickups, etc.
// claude-ai:
// claude-ai: DISTINCTION FROM FURN (furniture):
// claude-ai:   OB = outdoor, static, tile-grid indexed, not a Thing entity.
// claude-ai:   FURN = indoor/moveable objects that ARE Thing entities.
// claude-ai:   OBs are purely visual + collision data; no AI or physics.
// claude-ai:   Exception: fire hydrant has simple particle simulation (OB_process).
// claude-ai:
// claude-ai: DATA STRUCTURES:
// claude-ai:   OB_Ob [OB_MAX_OBS]          — flat array of all objects on map.
// claude-ai:     Fields: x(8-bit subunit), z(8-bit subunit), y(height), prim(index),
// claude-ai:             yaw(>>3 compressed), flags(OB_FLAG_*), InsideIndex(room id)
// claude-ai:     Coordinate encoding: x = (worldX & 0x3ff) >> 2  (0-255 within square)
// claude-ai:                          z = (worldZ & 0x3ff) >> 2
// claude-ai:                          yaw = rawYaw >> 3  (0-255 for full circle)
// claude-ai:   OB_workaround (OB_mapwho) [OB_SIZE][OB_SIZE] — spatial index.
// claude-ai:     Each cell: index = start of this square's objects in OB_ob[],
// claude-ai:                num   = count of objects on this square.
// claude-ai:   OB_ob[] is a packed flat array; objects for one square are
// claude-ai:   contiguous. OB_compress() defragments the array when needed.
// claude-ai:
// claude-ai: OB_SIZE = size of the OB_mapwho grid (in mapsquares, matching PAP_SIZE_LO).
// claude-ai: OB_MAX_PER_SQUARE = max objects allowed per mapsquare.
// claude-ai: OB_MAX_OBS = total object array capacity.
// claude-ai:
// claude-ai: COORDINATE SYSTEM:
// claude-ai:   WorldPos coords passed to OB_create() are in >>10 units (mapsquare
// claude-ai:   coords x 1024). Internally stored as subunit offsets within cell.
// claude-ai:   OB_find() returns OB_Info with: x=(mx<<10)+(oo->x<<2), etc.
// claude-ai:   So OB_Info coordinates are in the same fine units as WorldPos>>8.
// claude-ai:
// claude-ai: KEY FUNCTIONS:
// claude-ai:   OB_init()         — clears OB_mapwho and hydrant arrays; sets ob_upto=1
// claude-ai:                       (index 0 is reserved/null sentinel)
// claude-ai:   OB_create()       — adds a new object to a mapsquare's slot.
// claude-ai:                       If square not at end of OB_ob[], copies it there
// claude-ai:                       first (may trigger OB_compress() if full).
// claude-ai:                       PC/editor only (#ifndef PSX, #ifndef TARGET_DC).
// claude-ai:   OB_compress()     — defragments OB_ob[] by rebuilding it in
// claude-ai:                       mapsquare order; updates all OB_mapwho indices.
// claude-ai:   OB_find(mx,mz)    — returns null-terminated OB_Info[] of all objects
// claude-ai:                       on that mapsquare. Expands stored coords back to
// claude-ai:                       fine units. Applies damaged state (lean/crumple).
// claude-ai:   OB_find_inside()  — like OB_find() but filters by InsideIndex (room).
// claude-ai:   OB_remove()       — removes object by swapping with last in square.
// claude-ai:                       Also removes any walkable prim faces with matching
// claude-ai:                       ThingIndex == -oi->index.
// claude-ai:   OB_process()      — per-frame update for active fire hydrants only.
// claude-ai:   OB_avoid()        — collision avoidance helper: given a movement
// claude-ai:                       vector (x1,z1)->(x2,z2) and an OB prim, returns
// claude-ai:                       -1 (go left), +1 (go right), 0 (no collision).
// claude-ai:                       Uses MAV_inside() to check which side has room.
// claude-ai:   OB_find_type()    — scans a radius for nearest object matching
// claude-ai:                       given PRIM_FLAG_* or special flags (tripwire,
// claude-ai:                       switch, valve). Used by mission scripts.
// claude-ai:
// claude-ai: DAMAGE SYSTEM (OB_FLAG_DAMAGED + OB_Info::crumple/roll/pitch):
// claude-ai:   When OB_FLAG_DAMAGED set, OB_find() applies visual deformation:
// claude-ai:   PRIM_DAMAGE_LEAN prims: roll/pitch set to ±40 + dlean[] table offset
// claude-ai:                           (makes lamp posts lean uniquely per index).
// claude-ai:   Other prims: crumple = (flags>>6)+1  (used by renderer to scale down).
// claude-ai:
// claude-ai: FIRE HYDRANT SPECIAL CASE (OB_Hydrant, OB_hydrant[4]):
// claude-ai:   When a hydrant is damaged, it's registered in OB_hydrant[].
// claude-ai:   OB_process() counts down life, spawns DIRT_new_water() particles
// claude-ai:   in the direction determined by flags>>6 (4 possible spray angles).
// claude-ai:   Multiple streams if life > 5s or > 10s thresholds.
// claude-ai:   Sound: S_FIRE_HYDRANT via MFX_stop() when expired.
// claude-ai:
// claude-ai: WALKABLE FACES — OB_add_walkable_faces():
// claude-ai:   For prims with PRIM_FLAG_CONTAINS_WALKABLE_FACES, transforms each
// claude-ai:   FACE_FLAG_WALKABLE quad into world space (yaw-only rotation via
// claude-ai:   FMATRIX_calc), snaps points to map grid (OB_SNAP_WIDTH=6), then
// claude-ai:   creates new prim_faces4[] entries with FACE_FLAG_PRIM set and
// claude-ai:   ThingIndex = -oi->index (negative = identifies source OB).
// claude-ai:   Calls attach_walkable_to_map() for each. Only prims 12-17 (fire
// claude-ai:   escapes), 29 (metal platform), 109, 221 get FACE_FLAG_METAL.
// claude-ai:
// claude-ai: GENERAL PRIM LOADING — load_general_prims():
// claude-ai:   Hardcoded list of prim IDs always needed: vehicles (van, car, taxi,
// claude-ai:   police, ambulance, jeep, meatwagon, sedan, wildcat), weapons (gun,
// claude-ai:   knife, shotgun, bat, AK47 + flash prims), chopper, general pickups.
// claude-ai:   PRIM_OBJ_BAT (weapon bat) is loaded here — distinct from bat.cpp
// claude-ai:   creatures which are Thing entities, not OB prims.
// claude-ai: ============================================================

#include <MFStdLib.h>
#include "game.h"
#include "pap.h"
#include "ob.h"
#include "io.h"
#include "fmatrix.h"
#include "build2.h"
#include "dirt.h"
#include "pow.h"
#include "memory.h"
#include "sound.h"
#include "mav.h"

#include "..\ddengine\headers\poly.h"
#include "..\ddengine\headers\texture.h"

OB_Ob* OB_ob; //[OB_MAX_OBS];
OB_workaround* OB_mapwho; //[OB_SIZE][OB_SIZE];

SLONG OB_ob_upto;

#define PRIM_OBJ_CHOPPER 74
#define PRIM_OBJ_CHOPPER_BLADES 75

//
// The array returned by OB_find.
//

OB_Info OB_found[OB_MAX_PER_SQUARE + 1];
OB_Info OB_return;

//
// The damaged fire hydrants.
//

typedef struct
{
    UWORD life; // 0 => Unused.
    UWORD index;
    UWORD x;
    UWORD z;

} OB_Hydrant;

#define OB_MAX_HYDRANTS 4

OB_Hydrant OB_hydrant[OB_MAX_HYDRANTS];
UBYTE OB_hydrant_last;

// claude-ai: OB_init() — clears the spatial index and hydrant tracking array.
// claude-ai: OB_ob_upto starts at 1 (index 0 = null sentinel, never used).
// claude-ai: PC build also logs sizeof() for debugging during development.
void OB_init()
{

    OB_ob_upto = 1;

    memset((UBYTE*)OB_mapwho, 0, sizeof(OB_Mapwho) * OB_SIZE * OB_SIZE);
    memset((UBYTE*)OB_hydrant, 0, sizeof(OB_Hydrant) * OB_MAX_HYDRANTS);
}

void OB_compress()
{
    SLONG x;
    SLONG z;

    OB_Mapwho* om;

    OB_Ob* comp;
    SLONG comp_upto;

    comp = (OB_Ob*)MemAlloc(sizeof(OB_Ob) * OB_MAX_OBS);
    comp_upto = 1;

    if (comp) {
        for (x = 0; x < OB_SIZE; x++)
            for (z = 0; z < OB_SIZE; z++) {
                om = &OB_mapwho[x][z];

                ASSERT(comp_upto + om->num <= OB_MAX_OBS);

                memcpy(&comp[comp_upto], &OB_ob[om->index], om->num * sizeof(OB_Ob));

                om->index = comp_upto;
                comp_upto += om->num;
            }

        //
        // Copy over the compressed array.
        //

        memcpy(OB_ob, comp, sizeof(OB_Ob) * OB_MAX_OBS);
        OB_ob_upto = comp_upto;

        MemFree(comp);
    }
}
void OB_create(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG yaw,
    SLONG pitch,
    SLONG roll,
    SLONG prim,
    UBYTE flag,
    UWORD inside,
    UBYTE room)
{
    SLONG mx;
    SLONG mz;

    mx = x >> 10;
    mz = z >> 10;

    if (!WITHIN(mx, 0, OB_SIZE - 1) || !WITHIN(mz, 0, OB_SIZE - 1)) {
        //
        // Off the map!
        //

        return;
    }

    OB_Ob* oo;
    OB_Mapwho* om = &OB_mapwho[mx][mz];

    if (om->num >= OB_MAX_PER_SQUARE) {
        //
        // Too many prims on this square already.
        //

        return;
    }

    //
    // Are this square's objects already at the end of the array?
    //

    if (om->index + om->num == OB_ob_upto) {
        //
        // Yes nothing to do.
        //
    } else {
        //
        // Enough room to copy this squares object to the end of the array?
        //

        if (OB_ob_upto + om->num + 1 > OB_MAX_OBS) {
            //
            // No. We have to compress the array.
            //

            OB_compress();

            if (OB_ob_upto + om->num + 1 > OB_MAX_OBS) {
                //
                // No more memory left.
                //

                return;
            }
        }

        //
        // Copy this square's objects to the end of the array.
        //

        memcpy(&OB_ob[OB_ob_upto], &OB_ob[om->index], sizeof(OB_Ob) * om->num);

        om->index = OB_ob_upto;
        OB_ob_upto = om->index + om->num;
        ASSERT(OB_ob_upto < 3000);
    }

    ASSERT(OB_ob_upto == om->index + om->num);

    //
    // Create the new object at the end of the array.
    //

    oo = &OB_ob[OB_ob_upto];

    oo->x = (x & 0x3ff) >> 2;
    oo->z = (z & 0x3ff) >> 2;
    oo->y = y;
    oo->prim = prim;
    oo->yaw = yaw >> 3;
    oo->flags = flag;
    oo->InsideIndex = inside;

    om->num += 1;
    OB_ob_upto += 1;
}

// claude-ai: OB_process() — per-frame update for all active fire hydrants.
// claude-ai: Called each game tick. Counts down life (in ticks scaled by TICK_RATIO).
// claude-ai: When life reaches 0: clears entry, stops MFX S_FIRE_HYDRANT sound.
// claude-ai: While active: sprays DIRT_new_water() particles; direction from flags>>6
// claude-ai: (4 spray orientations offset by ±256 from the hydrant's stored yaw).
// claude-ai: Extra streams at life > 20*16*5 (5 sec) and > 20*16*10 (10 sec) thresholds.
void OB_process()
{
    SLONG i;
    SLONG yaw;
    SLONG dx;
    SLONG dz;
    SLONG tick = 16 * TICK_RATIO >> TICK_SHIFT;

    OB_Hydrant* oh;
    OB_Ob* oo;

    for (i = 0; i < OB_MAX_HYDRANTS; i++) {
        oh = &OB_hydrant[i];
        ;

        if (oh->life) {
            if (oh->life < tick) {
                oh->life = 0;
                MFX_stop(MAX_THINGS + i, S_FIRE_HYDRANT);
            } else {
                oh->life -= tick;

                //
                // The OB the water is spurting out of.
                //

                ASSERT(WITHIN(oh->index, 1, OB_ob_upto - 1));

                oo = &OB_ob[oh->index];

                ASSERT(oo->prim == PRIM_OBJ_HYDRANT && (oo->flags & OB_FLAG_DAMAGED));

                //
                // What is the angle of this hydrant's water jet?
                //

                yaw = oo->yaw << 3;

                switch (oo->flags >> 6) {
                case 0:
                    yaw += 1024 + 256;
                    break;
                case 1:
                    yaw += 1024 - 256;
                    break;
                case 2:
                    yaw += +256;
                    break;
                case 3:
                    yaw += -256;
                    break;
                }

                yaw &= 2047;

                //
                // The (dx,dz) for this yaw.
                //

                dx = SIN(yaw) >> 12;
                dz = COS(yaw) >> 12;

                //
                // Create more water the greater the remaining life of the hydrant.
                //

                DIRT_new_water(oh->x, oo->y, oh->z, dx, 16, dz);

                if (oh->life > 16 * 20 * 5) {
                    DIRT_new_water(oh->x, oo->y, oh->z, dx + 1, 15, dz - 1);

                    if (oh->life > 16 * 20 * 10) {
                        DIRT_new_water(oh->x, oo->y, oh->z, dx - 1, 17, dz + 1);
                    }
                }
            }
        }
    }
}

// claude-ai: OB_find() — returns null-terminated OB_Info[] for mapsquare (x, z).
// claude-ai: Expands compressed OB_Ob fields back to full coordinates.
// claude-ai: For damaged objects: lean prims get roll/pitch set for visual tilt;
// claude-ai: other damaged prims get crumple value for scale-down by renderer.
// claude-ai: The dlean[] table {25,-16,-28,+15} ensures each lamp post leans uniquely.
OB_Info* OB_find(SLONG x, SLONG z)
{
    OB_Info* of;

    SLONG num;
    SLONG index;

    OB_Mapwho* om;
    OB_Ob* oo;

    ASSERT(WITHIN(x, 0, OB_SIZE - 1));
    ASSERT(WITHIN(z, 0, OB_SIZE - 1));

    om = &OB_mapwho[x][z];
    index = om->index;
    num = om->num;
    of = OB_found;

    //
    // Build an array of the indices of the objects
    // above this mapwho square that don't have NULL prim
    // indices.
    //

    while (num--) {
        ASSERT(WITHIN(index, 1, OB_ob_upto - 1));

        oo = &OB_ob[index];

        if (oo->prim) {
            of->prim = oo->prim;
            of->x = (x << 10) + (oo->x << 2);
            of->z = (z << 10) + (oo->z << 2);
            of->y = oo->y;
            of->yaw = oo->yaw << 3;
            of->pitch = 0;
            of->roll = 0;
            of->index = index;
            of->crumple = 0;
            of->InsideIndex = oo->InsideIndex;
            of->flags = oo->flags;

            if (oo->flags & OB_FLAG_DAMAGED) {
                if (prim_objects[oo->prim].damage & PRIM_DAMAGE_LEAN) {
                    static SBYTE dlean[4] = { 25, -16, -28, +15 };

                    //
                    // Make the object lean.
                    //

                    of->roll = (oo->flags & OB_FLAG_RESERVED1) ? 40 : 2048 - 40;
                    of->pitch = (oo->flags & OB_FLAG_RESERVED2) ? 40 : 2048 - 40;

                    of->roll += dlean[(index + 0) & 3];
                    of->pitch += dlean[(index + 1) & 3];
                } else {
                    //
                    // Crumple up the object.
                    //

                    of->crumple = oo->flags >> 6;
                    of->crumple += 1;
                }
            }

            of += 1;
        }

        index += 1;
    }

    //
    // The array is terminated by NULL in the prim field.
    //

    of->prim = NULL;

    return OB_found;
}
OB_Info* OB_find_inside(SLONG x, SLONG z, SLONG indoors)
{
    OB_Info* of;

    SLONG num;
    SLONG index;

    OB_Mapwho* om;
    OB_Ob* oo;

    ASSERT(WITHIN(x, 0, OB_SIZE - 1));
    ASSERT(WITHIN(z, 0, OB_SIZE - 1));

    om = &OB_mapwho[x][z];
    index = om->index;
    num = om->num;
    of = OB_found;

    //
    // Build an array of the indices of the objects
    // above this mapwho square that don't have NULL prim
    // indices.
    //

    while (num--) {
        ASSERT(WITHIN(index, 1, OB_ob_upto - 1));

        oo = &OB_ob[index];
        if (oo->InsideIndex == indoors)
            if (oo->prim) {
                of->prim = oo->prim;
                of->x = (x << 10) + (oo->x << 2);
                of->z = (z << 10) + (oo->z << 2);
                of->y = oo->y;
                of->yaw = oo->yaw << 3;
                of->pitch = 0;
                of->roll = 0;
                of->index = index;
                of->InsideIndex = oo->InsideIndex;

                of += 1;
            }

        index += 1;
    }

    //
    // The array is terminated by NULL in the prim field.
    //

    of->prim = NULL;

    return OB_found;
}
// claude-ai: OB_avoid() — steerable avoidance helper for moving entities.
// claude-ai: Extends movement vector (x1,z1)->(x2,z2) by 8x to look ahead further.
// claude-ai: Computes two tangent points px1/pz1, px2/pz2 on either side of the prim.
// claude-ai: Uses MAV_inside() to check which side has walkable space.
// claude-ai: Returns -1 = steer left, +1 = steer right, 0 = no avoidance needed.
// claude-ai: PRIM_COLLIDE_NONE prims are skipped (return 0 immediately).
// claude-ai: Coords passed in are >>8 WorldPos units (fine map units).
SLONG OB_avoid(
    SLONG ob_x,
    SLONG ob_y,
    SLONG ob_z,
    SLONG ob_yaw,
    SLONG ob_prim,
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG x;
    SLONG y;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG da;
    SLONG db;

    SLONG dist;
    SLONG dist1;
    SLONG dist2;

    SLONG px1;
    SLONG pz1;
    SLONG px2;
    SLONG pz2;

    SLONG ob_radius;

    PrimInfo* pi = get_prim_info(ob_prim);

    x1 >>= 8;
    z1 >>= 8;

    x2 >>= 8;
    z2 >>= 8;

    //
    // Lengthen the movement vector...
    //

    dx = x2 - x1 << 3;
    dz = z2 - z1 << 3;

    x2 = x1 + dx;
    z2 = z1 + dz;

#define OB_AVOID_LOOK_EXTRA 0x40

    switch (prim_get_collision_model(ob_prim)) {
    case PRIM_COLLIDE_BOX:
        ob_radius = pi->radius + OB_AVOID_LOOK_EXTRA;
        break;

    case PRIM_COLLIDE_CYLINDER:
        ob_radius = 0x40 + OB_AVOID_LOOK_EXTRA;
        break;

    case PRIM_COLLIDE_NONE:
        return 0;
        break;

    default:
        ASSERT(0);
        break;
    }

    dx = ob_x - x1;
    dz = ob_z - z1;

    //
    // Normalise dx,dz to the radius of the object.
    //

    dist = QDIST2(abs(dx), abs(dz)) + 1;

    dx = dx * ob_radius / dist;
    dz = dz * ob_radius / dist;

    px1 = ob_x + (-dz);
    pz1 = ob_z + (+dx);

    px2 = ob_x - (-dz);
    pz2 = ob_z - (+dx);

    // #ifdef PSX

    SLONG roomy1 = !MAV_inside(px1, ob_y + 0x40, pz1);
    SLONG roomy2 = !MAV_inside(px2, ob_y + 0x40, pz2);

    if (roomy1 ^ roomy2) {
        //
        // Turn towards the side of the prim we can go around.
        //

        if (roomy1) {
            return -1;
        } else {
            return +1;
        }
    } else {
        //
        // Turn towards the nearest side of the prim to where you are going.
        //

        dx = abs(px1 - x2);
        dz = abs(pz1 - z2);

        dist1 = QDIST2(dx, dz);

        dx = abs(px2 - x2);
        dz = abs(pz2 - z2);

        dist2 = QDIST2(dx, dz);

        if (dist1 < dist2) {
            return -1;
        } else {
            return +1;
        }
    }
}

void load_general_prims(void)
{
    //
    //  Stat Up's
    //
    load_prim_object(71);
    load_prim_object(94);
    load_prim_object(81);
    load_prim_object(39);

    //
    // Needed for helicopters...
    //

    load_prim_object(PRIM_OBJ_CHOPPER);
    load_prim_object(PRIM_OBJ_CHOPPER_BLADES);

    //
    // The van and the car
    //

    load_prim_object(PRIM_OBJ_VAN_WHEEL);
    load_prim_object(PRIM_OBJ_VAN_BODY);
    load_prim_object(PRIM_OBJ_CAR_WHEEL);
    load_prim_object(PRIM_OBJ_CAR_BODY);
    load_prim_object(PRIM_OBJ_TAXI_BODY);
    load_prim_object(PRIM_OBJ_POLICE_BODY);
    load_prim_object(PRIM_OBJ_AMBULANCE_BODY);
    load_prim_object(PRIM_OBJ_JEEP_BODY);
    load_prim_object(PRIM_OBJ_MEATWAGON_BODY);
    load_prim_object(PRIM_OBJ_SEDAN_BODY);
    load_prim_object(PRIM_OBJ_WILDCATVAN_BODY);

    //
    // Various extras.
    //

    // no	load_prim_object(PRIM_OBJ_HOOK);	// The grappling hook
    load_prim_object(PRIM_OBJ_CAN);
    // no	load_prim_object(PRIM_OBJ_BALLOON);
    load_prim_object(PRIM_OBJ_BARREL);
    load_prim_object(PRIM_OBJ_TRAFFIC_CONE);
    load_prim_object(145);

    //
    // The "Fun Day" stuff 14th Jan 1999
    //

    load_prim_object(PRIM_OBJ_MINE);
    load_prim_object(PRIM_OBJ_THERMODROID);

    //
    // Stairs
    //

    //	load_prim_object(27);
    //	load_prim_object(28);
    //	load_prim_object(29);

    //
    // WEAPONS
    //
    load_prim_object(PRIM_OBJ_WEAPON_GUN);
    load_prim_object(PRIM_OBJ_WEAPON_KNIFE);
    load_prim_object(PRIM_OBJ_WEAPON_SHOTGUN);
    load_prim_object(PRIM_OBJ_WEAPON_BAT);
    load_prim_object(PRIM_OBJ_WEAPON_AK47);
    load_prim_object(PRIM_OBJ_WEAPON_GUN_FLASH);
    load_prim_object(PRIM_OBJ_WEAPON_SHOTGUN_FLASH);
    load_prim_object(PRIM_OBJ_WEAPON_AK47_FLASH);
}

void set_face_type(SLONG prim, SLONG type)
{
    PrimObject* po;
    SLONG j;

    //
    // Shrink the selected prims.
    //

    po = &prim_objects[prim];

    for (j = po->StartFace4; j < po->EndFace4; j++) {
        if (type == 0) {
            prim_faces4[j].FaceFlags &= ~FACE_FLAG_METAL;

        } else {
            prim_faces4[j].FaceFlags |= type;
        }
    }
}

void OB_load_needed_prims()
{
    SLONG i;
    SLONG j;

    {
        for (i = 1; i < OB_ob_upto; i++) {
            load_prim_object(OB_ob[i].prim);

            if (OB_ob[i].prim == PRIM_OBJ_SWITCH_OFF) {
                load_prim_object(PRIM_OBJ_SWITCH_ON);
            }

            if (OB_ob[i].prim == 29) {
                //				prim_faces4[next_prim_face4-3].FaceFlags|=FACE_FLAG_WALKABLE;
                set_face_type(OB_ob[i].prim, FACE_FLAG_WALKABLE);
                prim_objects[29].flag |= PRIM_FLAG_CONTAINS_WALKABLE_FACES;
            }
            if (OB_ob[i].prim >= 12 && OB_ob[i].prim <= 17) {
                set_face_type(OB_ob[i].prim, FACE_FLAG_FIRE_ESCAPE);
            } else {
                if ((OB_ob[i].prim == 29) || (OB_ob[i].prim == 109) || (OB_ob[i].prim == 221)) {
                    set_face_type(OB_ob[i].prim, FACE_FLAG_METAL); // actually, it's a metal platform, but Mike said to use outline.
                } else {
                    set_face_type(OB_ob[i].prim, 0);
                }
            }
        }
        load_general_prims();
    }

    //
    // The specials.
    //

    SLONG page;

    for (i = 1; i < SPECIAL_NUM_TYPES; i++) {
        load_prim_object(SPECIAL_info[i].prim);

        //
        // Centre the specials!
        //

        {
            SLONG min_x = +INFINITY;
            SLONG min_y = +INFINITY;
            SLONG min_z = +INFINITY;

            SLONG max_x = -INFINITY;
            SLONG max_y = -INFINITY;
            SLONG max_z = -INFINITY;

            SLONG mid_x;
            SLONG mid_y;
            SLONG mid_z;

            PrimObject* po;

            //
            // Shrink the selected prims.
            //

            po = &prim_objects[SPECIAL_info[i].prim];

            for (j = po->StartPoint; j < po->EndPoint; j++) {
                if (prim_points[j].X < min_x) {
                    min_x = prim_points[j].X;
                }
                if (prim_points[j].Y < min_y) {
                    min_y = prim_points[j].Y;
                }
                if (prim_points[j].Z < min_z) {
                    min_z = prim_points[j].Z;
                }

                if (prim_points[j].X > max_x) {
                    max_x = prim_points[j].X;
                }
                if (prim_points[j].Y > max_y) {
                    max_y = prim_points[j].Y;
                }
                if (prim_points[j].Z > max_z) {
                    max_z = prim_points[j].Z;
                }
            }

            mid_x = min_x + max_x >> 1;
            mid_y = min_y + max_y >> 1;
            mid_z = min_z + max_z >> 1;

            for (j = po->StartPoint; j < po->EndPoint; j++) {
                prim_points[j].X -= mid_x;
                prim_points[j].Y -= mid_y;
                prim_points[j].Z -= mid_z;
            }
        }
    }

    load_prim_object(PRIM_OBJ_ITEM_KEY);
    load_prim_object(117);

    void re_center_prim(SLONG prim, SLONG dx, SLONG dy, SLONG dz);

    re_center_prim(27, 128, 0, -256);
    re_center_prim(28, 128, -72, -256);
    re_center_prim(29, 128, -64, -256);

    load_prim_object(PRIM_OBJ_BIN);
}

void envmap_specials(void)
{
    SLONG i;
    SLONG j;

    //
    // The specials.
    //

    SLONG page;

    for (i = 1; i < SPECIAL_NUM_TYPES; i++) {
        {
            PrimObject* po;

            po = &prim_objects[SPECIAL_info[i].prim];

            //
            // Make them all environment mapped.
            //

            for (j = po->StartFace3; j < po->EndFace3; j++) {
                PrimFace3* p_f3 = &prim_faces3[j];

                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;
                page += FACE_PAGE_OFFSET;

                if (POLY_page_flag[page] & (POLY_PAGE_FLAG_ALPHA | POLY_PAGE_FLAG_TRANSPARENT)) {
                    //
                    // Don't make these environment mapped.
                    //

                    prim_faces3[j].FaceFlags &= ~FACE_FLAG_ENVMAP;
                } else {
                    prim_faces3[j].FaceFlags |= FACE_FLAG_ENVMAP;
                }
            }

            for (j = po->StartFace4; j < po->EndFace4; j++) {
                PrimFace4* p_f4 = &prim_faces4[j];

                page = p_f4->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f4->TexturePage;
                page += FACE_PAGE_OFFSET;

                if (POLY_page_flag[page] & (POLY_PAGE_FLAG_ALPHA | POLY_PAGE_FLAG_TRANSPARENT)) {
                    //
                    // Don't make these environment mapped.
                    //

                    prim_faces4[j].FaceFlags &= ~FACE_FLAG_ENVMAP;
                } else {
                    prim_faces4[j].FaceFlags |= FACE_FLAG_ENVMAP;
                }
            }

            po->flag |= PRIM_FLAG_ENVMAPPED;
        }
    }

    //
    // Make sure no faces are envmapped AND transparent...
    //

    for (i = 1; i < 256; i++) {
        PrimObject* po;

        po = &prim_objects[i];

        if (po->flag & PRIM_FLAG_ENVMAPPED) {
            //
            // Make them all environment mapped.
            //

            for (j = po->StartFace3; j < po->EndFace3; j++) {
                PrimFace3* p_f3 = &prim_faces3[j];

                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;
                page += FACE_PAGE_OFFSET;

                if (POLY_page_flag[page] & (POLY_PAGE_FLAG_ALPHA | POLY_PAGE_FLAG_TRANSPARENT)) {
                    //
                    // Make sure these faces aren't envmapped...
                    //

                    prim_faces3[j].FaceFlags &= ~FACE_FLAG_ENVMAP;
                }
            }

            for (j = po->StartFace4; j < po->EndFace4; j++) {
                PrimFace4* p_f4 = &prim_faces4[j];

                page = p_f4->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f4->TexturePage;
                page += FACE_PAGE_OFFSET;

                if (POLY_page_flag[page] & (POLY_PAGE_FLAG_ALPHA | POLY_PAGE_FLAG_TRANSPARENT)) {
                    //
                    // Make sure these faces aren't envmapped...
                    //

                    prim_faces4[j].FaceFlags &= ~FACE_FLAG_ENVMAP;
                }
            }
        }
    }
}

SLONG ob_allowed_to_be_walkable(SLONG prim)
{
    /*

    //
    // FIXED IN ANOTHER WAY NOW. WHEN OB_remove() IS
    // CALLED- IT REMOVES ANY OF THAT FACES PRIM_FACES.
    //

    extern SLONG playing_level(const CBYTE *name);

    if (playing_level("Finale1.ucm"))
    {
            if (prim == 44)
            {
                    //
                    // To stop the moving platforms on the last level having
                    // two walkable faces!
                    //

                    return 0;
            }
    }

    */

    if (prim == 175 || prim == 176 || prim == 171 || prim == 173 || prim == 174 || prim == 183)
        return (0);

    return (1);
}
void OB_add_walkable_faces()
{
    SLONG i;
    SLONG j;
    SLONG k;

    SLONG px;
    SLONG py;
    SLONG pz;

    SLONG mx;
    SLONG mz;

    PrimObject* po;
    PrimPoint* pp;
    PrimFace4* f4;
    OB_Info* oi;

    SLONG matrix[9];

    prim_objects[29].flag |= PRIM_FLAG_CONTAINS_WALKABLE_FACES;

    for (mx = 0; mx < PAP_SIZE_LO; mx++)
        for (mz = 0; mz < PAP_SIZE_LO; mz++) {
            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                po = &prim_objects[oi->prim];
                //			if(oi->prim!=175 && oi->prim!=176 && oi->prim!=171 && oi->prim!=173 &&oi->prim!=174 && oi->prim!=183)
                if (ob_allowed_to_be_walkable(oi->prim))
                    if (po->flag & PRIM_FLAG_CONTAINS_WALKABLE_FACES) {
                        //
                        // This objects rotation matrix.
                        //

                        FMATRIX_calc(
                            matrix,
                            oi->yaw,
                            0,
                            0);

                        //
                        // Add this prim's walkable faces to the map.
                        //

                        for (i = po->StartFace4; i < po->EndFace4; i++) {
                            f4 = &prim_faces4[i];

                            if (f4->FaceFlags & FACE_FLAG_WALKABLE) {
                                //
                                // Create a walkable face for this prim.
                                //

                                for (j = 0; j < 4; j++) {
                                    ASSERT(WITHIN(f4->Points[j], 1, next_prim_point - 1));
                                    ASSERT(WITHIN(next_prim_point, 1, MAX_PRIM_POINTS - 1));

                                    px = prim_points[f4->Points[j]].X;
                                    py = prim_points[f4->Points[j]].Y;
                                    pz = prim_points[f4->Points[j]].Z;

                                    FMATRIX_MUL_BY_TRANSPOSE(
                                        matrix,
                                        px,
                                        py,
                                        pz);

                                    px += oi->x;
                                    py += oi->y;
                                    pz += oi->z;

                            //
                            // Snap the points to mapsquare boundaries.
                            //

#define OB_SNAP_WIDTH 6

                                    if (((px + OB_SNAP_WIDTH) & 0xff) < OB_SNAP_WIDTH * 2) {
                                        px += OB_SNAP_WIDTH;
                                        px &= ~0xff;
                                    }
                                    if (((py + OB_SNAP_WIDTH) & 0xff) < OB_SNAP_WIDTH * 2) {
                                        py += OB_SNAP_WIDTH;
                                        py &= ~0xff;
                                    }
                                    if (((pz + OB_SNAP_WIDTH) & 0xff) < OB_SNAP_WIDTH * 2) {
                                        pz += OB_SNAP_WIDTH;
                                        pz &= ~0xff;
                                    }

                                    if (!WITHIN(px, 0, (PAP_SIZE_HI << 8) - 1) || !WITHIN(pz, 0, (PAP_SIZE_HI << 8) - 1)) {
                                        goto abandon_face;
                                    }

                                    prim_points[next_prim_point].X = px;
                                    prim_points[next_prim_point].Y = py;
                                    prim_points[next_prim_point].Z = pz;

                                    next_prim_point += 1;
                                }

                                //
                                // Done the points... now for the face.
                                //

                                ASSERT(WITHIN(next_prim_face4, 1, MAX_PRIM_FACES4 - 1));

                                prim_faces4[next_prim_face4] = *f4;
                                prim_faces4[next_prim_face4].Points[0] = next_prim_point - 4;
                                prim_faces4[next_prim_face4].Points[1] = next_prim_point - 3;
                                prim_faces4[next_prim_face4].Points[2] = next_prim_point - 2;
                                prim_faces4[next_prim_face4].Points[3] = next_prim_point - 1;
                                prim_faces4[next_prim_face4].FaceFlags &= ~FACE_FLAG_SLIDE_EDGE_ALL;
                                prim_faces4[next_prim_face4].FaceFlags |= FACE_FLAG_PRIM;
                                prim_faces4[next_prim_face4].ThingIndex = -oi->index;

                                next_prim_face4 += 1;

                                //
                                // Add this walkable face to the map.
                                //

                                attach_walkable_to_map(next_prim_face4 - 1);

                            abandon_face:;
                            }
                        }
                    }
            }
        }

    //
    // Make sure none of the prim objects have walkable faces in them so calc_slide_edges()
    // won't cock up.
    //

    for (i = 0; i < MAX_PRIM_OBJECTS; i++) {
        po = &prim_objects[i];

        for (j = po->StartFace4; j < po->EndFace4; j++) {
            f4 = &prim_faces4[j];

            f4->FaceFlags &= ~FACE_FLAG_WALKABLE;
        }
    }
}

// claude-ai: OB_remove() — removes an object from the map by swapping with the last
// claude-ai: object in its mapsquare's slot (O(1) unordered removal). Also scans
// claude-ai: prim_faces4[] for any walkable faces tagged with this OB's index
// claude-ai: (FACE_FLAG_PRIM + ThingIndex == -oi->index) and removes them from
// claude-ai: the walkable face map. Used e.g. when a lamp post is destroyed.
void OB_remove(OB_Info* oi)
{
    SLONG lo_map_x = oi->x >> PAP_SHIFT_LO;
    SLONG lo_map_z = oi->z >> PAP_SHIFT_LO;

    ASSERT(WITHIN(lo_map_x, 0, PAP_SIZE_LO - 1));
    ASSERT(WITHIN(lo_map_z, 0, PAP_SIZE_LO - 1));

    OB_Mapwho* om = &OB_mapwho[lo_map_x][lo_map_z];

    //
    // Make sure the OB_ob with this index is indeed on this
    // mapsquare.
    //

    ASSERT(WITHIN(oi->index, om->index, om->index + om->num));

    //
    // Take this objects out of the array.
    //

    OB_ob[oi->index] = OB_ob[om->index + --om->num]; // Cunning use of -- !

    //
    // If this face has any prims already on the map- then remove them!
    //

    SLONG i;

    PrimFace4* f4;

    for (i = 1; i < next_prim_face4; i++) {
        f4 = &prim_faces4[i];

        if (f4->FaceFlags & FACE_FLAG_PRIM) {
            if (f4->ThingIndex == -oi->index) {
                //
                // Remove this face!
                //

                remove_walkable_from_map(i);

                //				return;
            }
        }
    }
}

SLONG special_object_flag(OB_Info* ob, SLONG flags)
{
    if (flags & FIND_OB_TRIPWIRE) {
        if (ob->prim == PRIM_OBJ_TRIPWIRE) {
            return TRUE;
        }
    }

    if (flags & FIND_OB_SWITCH_OR_VALVE) {
        if (ob->prim == PRIM_OBJ_VALVE) {
            return TRUE;
        }

        if (ob->prim == PRIM_OBJ_SWITCH_OFF) {
            return TRUE;
        }
    }

    return FALSE;
}

//
// Finds the nearest object whose prim object contains one
// of the given flags.  Returns FALSE if no object was found
// in the range.
//

//
// if prim_flags is >255 then its a special object flag found in a special way :)
//
// claude-ai: OB_find_type() — spatial search for the nearest OB matching criteria.
// claude-ai: Scans mapsquares in a bounding box around (mid_x, mid_y, mid_z).
// claude-ai: prim_flags: if <= 255 = PRIM_FLAG_* bitmask from prim_objects[].flag;
// claude-ai:             if > 255  = special object type checked via special_object_flag()
// claude-ai:                         (FIND_OB_TRIPWIRE, FIND_OB_SWITCH_OR_VALVE).
// claude-ai: Returns FALSE if no matching object found within max_range.
// claude-ai: Out-params: ob_x/y/z, ob_yaw, ob_prim, ob_index of nearest found object.
SLONG OB_find_type(
    SLONG mid_x,
    SLONG mid_y,
    SLONG mid_z,
    SLONG max_range,
    ULONG prim_flags,
    SLONG* ob_x,
    SLONG* ob_y,
    SLONG* ob_z,
    SLONG* ob_yaw,
    SLONG* ob_prim,
    SLONG* ob_index)
{
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
    SLONG best_dist = INFINITY;
    SLONG best_x;
    SLONG best_y;
    SLONG best_z;
    SLONG best_yaw;
    SLONG best_prim;
    SLONG best_index;

    OB_Info* oi;

    mx1 = mid_x - max_range >> PAP_SHIFT_LO;
    mz1 = mid_z - max_range >> PAP_SHIFT_LO;
    mx2 = mid_x + max_range >> PAP_SHIFT_LO;
    mz2 = mid_z + max_range >> PAP_SHIFT_LO;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1);
    SATURATE(mz1, 0, PAP_SIZE_LO - 1);
    SATURATE(mx2, 0, PAP_SIZE_LO - 1);
    SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                if ((prim_objects[oi->prim].flag & prim_flags) || (prim_flags > 255 && special_object_flag(oi, prim_flags))) {
                    dx = oi->x - mid_x;
                    dy = oi->y - mid_y;
                    dz = oi->z - mid_z;

                    dist = abs(dx) + abs(dy) + abs(dz);

                    if (dist < max_range) {
                        if (dist < best_dist) {
                            best_x = oi->x;
                            best_y = oi->y;
                            best_z = oi->z;
                            best_yaw = oi->yaw;
                            best_prim = oi->prim;
                            best_dist = dist;
                            best_index = oi->index;
                        }
                    }
                }
            }
        }

    if (best_dist == INFINITY) {
        return FALSE;
    } else {
        *ob_x = best_x;
        *ob_y = best_y;
        *ob_z = best_z;
        *ob_yaw = best_yaw;
        *ob_prim = best_prim;
        *ob_index = best_index;

        return TRUE;
    }
}

OB_Info* OB_find_index(SLONG mid_x, SLONG mid_y, SLONG mid_z, SLONG max_range, SLONG must_be_searchable)
{
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
    SLONG best_dist = INFINITY;
    //	SLONG	best_ob=0;

    OB_Info *oi, *best_ob = 0;

    mx1 = mid_x - max_range >> PAP_SHIFT_LO;
    mz1 = mid_z - max_range >> PAP_SHIFT_LO;
    mx2 = mid_x + max_range >> PAP_SHIFT_LO;
    mz2 = mid_z + max_range >> PAP_SHIFT_LO;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1);
    SATURATE(mz1, 0, PAP_SIZE_LO - 1);
    SATURATE(mx2, 0, PAP_SIZE_LO - 1);
    SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                if (!must_be_searchable || (oi->flags & OB_FLAG_SEARCHABLE)) {
                    dx = oi->x - mid_x;
                    dy = oi->y - mid_y;
                    dz = oi->z - mid_z;

                    dist = abs(dx) + abs(dy) + abs(dz);

                    if (dist < max_range) {
                        if (dist < best_dist) {
                            best_ob = oi; //->index;
                            best_dist = dist;
                        }
                    }
                }
            }
            if (best_ob) {
                OB_return = *best_ob;
                best_ob = &OB_return;
            }
        }

    return (best_ob);
}

SLONG OB_find_min_y(SLONG prim)
{
    SLONG sp, ep;
    SLONG c0;
    SLONG min_y = 999999;

    sp = prim_objects[prim].StartPoint;
    ep = prim_objects[prim].EndPoint;
    for (c0 = sp; c0 < ep; c0++) {
        if (prim_points[c0].Y < min_y)
            min_y = prim_points[c0].Y;
    }
    return (min_y);
}

//
// any ob's locked to floor need to have their height changed
// taking into account water and stuff like that

void OB_height_fiddle_de_dee(void)
{
    /*

    //
    // This isn't the flavour of the month with the artists anymore.
    //

    SLONG	c0;
    for(c0=1;c0<OB_ob_upto;c0++)
    {
            if(OB_ob[c0].flags&OB_FLAG_JUST_FLOOR)
            {
                    SLONG	min_y,y;
                    if(PAP_2HI(OB_ob[c0].x>>8,OB_ob[c0].z>>8).Flags&PAP_FLAG_WATER)
                    {
                            y=PAP_2LO(OB_ob[c0].x>>10,OB_ob[c0].z>>10).water;
                            min_y=0;
                    }
                    else
                    {
                            y=PAP_calc_height_at(OB_ob[c0].x,OB_ob[c0].z);
                            min_y=OB_find_min_y(c0);
                    }
                    OB_ob[c0].y=y-min_y;
            }
            else
            if(OB_ob[c0].flags&OB_FLAG_ON_FLOOR) //|| (prim_objects[c0].flag & PRIM_FLAG_ON_FLOOR) )
            {
                    SLONG	min_y,y;
extern	SLONG find_alt_for_this_pos(SLONG  x,SLONG  z);
                    y=find_alt_for_this_pos(OB_ob[c0].x,OB_ob[c0].z);
                    min_y=OB_find_min_y(c0);
                    OB_ob[c0].y=y-min_y;

            }
    }

    */
}

void OB_damage(
    SLONG index, // The index of this object,
    SLONG from_dx,
    SLONG from_dz,
    SLONG x, // The position of this object!
    SLONG z,
    Thing* p_aggressor)
{
    OB_Ob* oo;
    OB_Hydrant* oh;
    PrimObject* po;

    //
    // Ignore other agreossors... the Balrog for instance!
    //

    if (p_aggressor && p_aggressor->Class != CLASS_PERSON) {
        p_aggressor = NULL;
    }

    ASSERT(WITHIN(index, 1, OB_ob_upto - 1));

    oo = &OB_ob[index];

    ASSERT(WITHIN(oo->prim, 1, next_prim_object - 1));

    po = &prim_objects[oo->prim];

    if (po->damage & PRIM_DAMAGE_DAMAGABLE) {
        if (po->damage & PRIM_DAMAGE_LEAN) {
            //
            // Which way should the object lean?
            //

            SLONG lx = SIN(oo->yaw << 3) >> 10;
            SLONG lz = COS(oo->yaw << 3) >> 10;

            SLONG dprod = lx * from_dx + lz * from_dz;
            SLONG cprod = lx * from_dz - lz * from_dx;

            oo->flags &= 0x3f;

            if (cprod > 0) {
                oo->flags |= OB_FLAG_RESERVED1;
            }
            if (dprod > 0) {
                oo->flags |= OB_FLAG_RESERVED2;
            }

            oo->flags |= Random() & 0xc0;
        } else if (po->damage & PRIM_DAMAGE_CRUMPLE) {
            //
            // Crumple the object.
            //

            UBYTE crumple;

            crumple = oo->flags >> 6;
            crumple += 1;

            if (crumple == 5) {
                crumple = 3;
            }

            oo->flags &= 0x3f;
            oo->flags |= crumple << 6;
        }

        if (oo->prim == PRIM_OBJ_HYDRANT) {
            //
            // Create a new hydrant water spout.
            //

            OB_hydrant_last += 1;
            OB_hydrant_last &= OB_MAX_HYDRANTS - 1;

            oh = &OB_hydrant[OB_hydrant_last];

            oh->life = 16 * 20 * 15;
            oh->index = index;
            oh->x = x;
            oh->z = z;

            MFX_play_xyz(MAX_THINGS + OB_hydrant_last, S_FIRE_HYDRANT, MFX_LOOPED, x << 8, PAP_calc_map_height_at(x, z) << 8, z << 8);
        }

        if (((po->damage & PRIM_DAMAGE_EXPLODES) || (po->flag & PRIM_FLAG_LAMPOST)) && !(oo->flags & OB_FLAG_DAMAGED)) {
            GameCoord pos;

            pos.X = x << 8;
            pos.Z = z << 8;
            pos.Y = oo->y << 8;

            //
            // Explode this bomb.
            //

            /*

            PYRO_construct(
                    pos,
               -1,
                    256);

            */
            PYRO_create(pos, PYRO_FIREBOMB);
            MFX_play_xyz(0, SOUND_Range(S_EXPLODE_MEDIUM, S_EXPLODE_BIG), 0, pos.X, pos.Y, pos.Z);

            //
            // Mark the ob as damaged before we do another create_shockwave()
            //

            oo->flags |= OB_FLAG_DAMAGED;
            create_shockwave(
                x, oo->y, z,
                0x300,
                150,
                p_aggressor);
        }

        //
        // Mark the ob as damaged.
        //

        oo->flags |= OB_FLAG_DAMAGED;
    }
}

void OB_convert_dustbins_to_barrels(void)
{
    SLONG mx;
    SLONG mz;

    OB_Info* oi;

    for (mx = 0; mx < PAP_SIZE_LO; mx++)
        for (mz = 0; mz < PAP_SIZE_LO; mz++) {

        start_again_because_weve_removed_an_ob:;

            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                if (oi->prim == 33) {
                    //
                    // This is the barrel prim
                    //

                    BARREL_alloc(
                        BARREL_TYPE_NORMAL,
                        oi->prim,
                        oi->x,
                        oi->z,
                        NULL);

                    OB_remove(oi);

                    goto start_again_because_weve_removed_an_ob;
                }
            }
        }
}

SLONG OB_inside_prim(SLONG x, SLONG y, SLONG z)
{
    SLONG mx;
    SLONG mz;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;

    SLONG mx1 = x - 0x180 >> 10;
    SLONG mz1 = z - 0x180 >> 10;
    SLONG mx2 = x + 0x180 >> 10;
    SLONG mz2 = z + 0x180 >> 10;

    OB_Info* oi;
    PrimInfo* pi;

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                dx = x - oi->x;
                dy = y - oi->y;
                dz = z - oi->z;

                if (dy < 0x300) {
                    dist = QDIST2(abs(dx), abs(dz));

                    if (dist < 0x100) {
                        return TRUE; // Very rough!
                    }
                }
            }
        }

    return FALSE;
}

void OB_make_all_the_switches_be_at_the_proper_height()
{
    SLONG i;

    SLONG mx;
    SLONG mz;

    OB_Info* oi;

    for (mx = 0; mx < PAP_SIZE_LO; mx++)
        for (mz = 0; mz < PAP_SIZE_LO; mz++) {
            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                if (oi->flags & OB_FLAG_WAREHOUSE) {
                    //
                    // This prim is in a warehouse...
                    //

                    continue;
                }

                if (oi->prim == PRIM_OBJ_SWITCH_OFF) {
                    SLONG gx = oi->x + (SIN(oi->yaw) >> 10);
                    SLONG gz = oi->z + (COS(oi->yaw) >> 10);

                    SLONG gy = PAP_calc_map_height_at(gx, gz);

                    OB_ob[oi->index].y = gy + 0x80;
                }

                //
                // All these prims are locked to the map height.
                //

                static UBYTE prims_to_snap[] = {
                    1,
                    2,
                    3,
                    34,
                    35,
                    52,
                    53,
                    54,
                    55,
                    58,
                    59,
                    70,
                    78,
                    93,
                    107,
                    134,
                    149,
                    161,
                    162,
                    163,
                    164,
                    169,
                    206,
                    213,
                    214,
                    218,
                    207,
                    222,
                    228,
                    230,
                    0
                };

                for (i = 0; prims_to_snap[i]; i++) {
                    if (oi->prim == prims_to_snap[i]) {
                        OB_ob[oi->index].y = PAP_calc_map_height_at(oi->x, oi->z);
                    }
                }

                {
                    //
                    // Clunk the 'y' of all the walkways...
                    //

                    for (i = 0; i < OB_ob_upto; i++) {
                        if (OB_ob[i].prim == 29 || OB_ob[i].prim == 129 || WITHIN(OB_ob[i].prim, 12, 17)) {
                            OB_ob[i].y += 0x20;
                            OB_ob[i].y &= ~0x3f;
                        }
                    }
                }
            }
        }
}
