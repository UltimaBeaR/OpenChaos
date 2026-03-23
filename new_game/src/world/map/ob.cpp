#include <platform.h>
#include "missions/game_types.h"
#include "assets/anim_globals.h"   // next_prim_point, next_prim_face4
#include "world/map/pap.h"
#include "world/map/ob.h"
#include "world/map/ob_globals.h"
#include "world/environment/build2.h"
#include "engine/audio/mfx.h"
#include "engine/audio/sound.h"
#include "core/math.h"
#include "core/fmatrix.h"
#include "effects/dirt.h"
#include "effects/pow.h"
#include "world/environment/prim_types.h" // PrimObject, PrimFace3/4, PRIM_OBJ_*, FACE_FLAG_*
#include "world/environment/prim.h"       // slide_along_prim, get_prim_info
#include "world/level_pools.h"

#include "engine/graphics/pipeline/poly.h"
#include "assets/texture.h"
#include "assets/level_loader.h"
#include "assets/level_loader_globals.h"
#include "assets/anim_loader.h"
#include "assets/anim_loader_globals.h"
#include "actors/items/special.h"
#include "actors/items/special_globals.h"
#include "actors/items/barrel.h"
#include "engine/physics/collide.h"
#include "ai/mav.h"
#include "effects/pyro.h"

// Chopper prims not defined in prim.h but used in load_general_prims.
// Both values are also defined locally in supermap.cpp.
// uc_orig: PRIM_OBJ_CHOPPER (fallen/Source/ob.cpp)
#define PRIM_OBJ_CHOPPER 74
// uc_orig: PRIM_OBJ_CHOPPER_BLADES (fallen/Source/ob.cpp)
#define PRIM_OBJ_CHOPPER_BLADES 75

// Snap width in world units used when snapping walkable face points to mapsquare grid.
// uc_orig: OB_SNAP_WIDTH (fallen/Source/ob.cpp)
#define OB_SNAP_WIDTH 6

// Look-ahead extra distance for OB_avoid() obstacle radius computation.
// uc_orig: OB_AVOID_LOOK_EXTRA (fallen/Source/ob.cpp)
#define OB_AVOID_LOOK_EXTRA 0x40

// OB_found, OB_return, OB_hydrant, OB_hydrant_last — defined in ob_globals.cpp

// uc_orig: OB_init (fallen/Source/ob.cpp)
void OB_init()
{
    OB_ob_upto = 1;
    memset((UBYTE*)OB_mapwho, 0, sizeof(OB_Mapwho) * OB_SIZE * OB_SIZE);
    memset((UBYTE*)OB_hydrant, 0, sizeof(OB_Hydrant) * OB_MAX_HYDRANTS);
}

// Defragments OB_ob[] by rebuilding it in mapsquare order.
// Called when there is no room to append to the current tail.
// uc_orig: OB_compress (fallen/Source/ob.cpp)
static void OB_compress()
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

        memcpy(OB_ob, comp, sizeof(OB_Ob) * OB_MAX_OBS);
        OB_ob_upto = comp_upto;
        MemFree(comp);
    }
}

// uc_orig: OB_create (fallen/Source/ob.cpp)
void OB_create(
    SLONG x, SLONG y, SLONG z,
    SLONG yaw, SLONG pitch, SLONG roll,
    SLONG prim, UBYTE flag, UWORD inside, UBYTE room)
{
    SLONG mx = x >> 10;
    SLONG mz = z >> 10;

    if (!WITHIN(mx, 0, OB_SIZE - 1) || !WITHIN(mz, 0, OB_SIZE - 1))
        return;

    OB_Ob* oo;
    OB_Mapwho* om = &OB_mapwho[mx][mz];

    if (om->num >= OB_MAX_PER_SQUARE)
        return;

    if (om->index + om->num == OB_ob_upto) {
        // Already at end of array — nothing to do.
    } else {
        if (OB_ob_upto + om->num + 1 > OB_MAX_OBS) {
            OB_compress();
            if (OB_ob_upto + om->num + 1 > OB_MAX_OBS)
                return;
        }
        memcpy(&OB_ob[OB_ob_upto], &OB_ob[om->index], sizeof(OB_Ob) * om->num);
        om->index = OB_ob_upto;
        OB_ob_upto = om->index + om->num;
        ASSERT(OB_ob_upto < 3000);
    }

    ASSERT(OB_ob_upto == om->index + om->num);

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

// uc_orig: OB_process (fallen/Source/ob.cpp)
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

        if (oh->life) {
            if (oh->life < tick) {
                oh->life = 0;
                MFX_stop(MAX_THINGS + i, S_FIRE_HYDRANT);
            } else {
                oh->life -= tick;

                ASSERT(WITHIN(oh->index, 1, OB_ob_upto - 1));
                oo = &OB_ob[oh->index];
                ASSERT(oo->prim == PRIM_OBJ_HYDRANT && (oo->flags & OB_FLAG_DAMAGED));

                yaw = oo->yaw << 3;

                switch (oo->flags >> 6) {
                case 0: yaw += 1024 + 256; break;
                case 1: yaw += 1024 - 256; break;
                case 2: yaw += +256;        break;
                case 3: yaw += -256;        break;
                }

                yaw &= 2047;

                dx = SIN(yaw) >> 12;
                dz = COS(yaw) >> 12;

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

// Expand stored OB_Ob fields to full OB_Info. Applies lean/crumple for damaged objects.
// The dlean[] table gives different lean offsets per index so lamp posts fall in unique directions.
// uc_orig: OB_find (fallen/Source/ob.cpp)
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
                    of->roll = (oo->flags & OB_FLAG_RESERVED1) ? 40 : 2048 - 40;
                    of->pitch = (oo->flags & OB_FLAG_RESERVED2) ? 40 : 2048 - 40;
                    of->roll += dlean[(index + 0) & 3];
                    of->pitch += dlean[(index + 1) & 3];
                } else {
                    of->crumple = oo->flags >> 6;
                    of->crumple += 1;
                }
            }

            of += 1;
        }

        index += 1;
    }

    of->prim = NULL;
    return OB_found;
}

// uc_orig: OB_find_inside (fallen/Source/ob.cpp)
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
                of->flags = oo->flags;
                of += 1;
            }

        index += 1;
    }

    of->prim = NULL;
    return OB_found;
}

// Steerable avoidance: returns -1 = steer left, +1 = steer right, 0 = no avoidance needed.
// Extends the movement vector 8x for look-ahead, then checks which side of the object is open.
// uc_orig: OB_avoid (fallen/Source/ob.cpp)
SLONG OB_avoid(
    SLONG ob_x, SLONG ob_y, SLONG ob_z,
    SLONG ob_yaw, SLONG ob_prim,
    SLONG x1, SLONG z1, SLONG x2, SLONG z2)
{
    SLONG x, y, z;
    SLONG dx, dz;
    SLONG da, db;
    SLONG dist, dist1, dist2;
    SLONG px1, pz1, px2, pz2;
    SLONG ob_radius;

    PrimInfo* pi = get_prim_info(ob_prim);

    x1 >>= 8;
    z1 >>= 8;
    x2 >>= 8;
    z2 >>= 8;

    // Extend movement vector 8x for look-ahead.
    dx = x2 - x1 << 3;
    dz = z2 - z1 << 3;
    x2 = x1 + dx;
    z2 = z1 + dz;

    switch (prim_get_collision_model(ob_prim)) {
    case PRIM_COLLIDE_BOX:
        ob_radius = pi->radius + OB_AVOID_LOOK_EXTRA;
        break;
    case PRIM_COLLIDE_CYLINDER:
        ob_radius = 0x40 + OB_AVOID_LOOK_EXTRA;
        break;
    case PRIM_COLLIDE_NONE:
        return 0;
    default:
        ASSERT(0);
        break;
    }

    dx = ob_x - x1;
    dz = ob_z - z1;

    dist = QDIST2(abs(dx), abs(dz)) + 1;
    dx = dx * ob_radius / dist;
    dz = dz * ob_radius / dist;

    px1 = ob_x + (-dz);
    pz1 = ob_z + (+dx);
    px2 = ob_x - (-dz);
    pz2 = ob_z - (+dx);

    SLONG roomy1 = !MAV_inside(px1, ob_y + 0x40, pz1);
    SLONG roomy2 = !MAV_inside(px2, ob_y + 0x40, pz2);

    if (roomy1 ^ roomy2) {
        return roomy1 ? -1 : +1;
    } else {
        dx = abs(px1 - x2);
        dz = abs(pz1 - z2);
        dist1 = QDIST2(dx, dz);

        dx = abs(px2 - x2);
        dz = abs(pz2 - z2);
        dist2 = QDIST2(dx, dz);

        return (dist1 < dist2) ? -1 : +1;
    }
}

// Hardcoded list of prims that must always be loaded (vehicles, weapons, pickups).
// uc_orig: load_general_prims (fallen/Source/ob.cpp)
static void load_general_prims(void)
{
    load_prim_object(71);
    load_prim_object(94);
    load_prim_object(81);
    load_prim_object(39);

    load_prim_object(PRIM_OBJ_CHOPPER);
    load_prim_object(PRIM_OBJ_CHOPPER_BLADES);

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

    load_prim_object(PRIM_OBJ_CAN);
    load_prim_object(PRIM_OBJ_BARREL);
    load_prim_object(PRIM_OBJ_TRAFFIC_CONE);
    load_prim_object(145);

    load_prim_object(PRIM_OBJ_MINE);
    load_prim_object(PRIM_OBJ_THERMODROID);

    load_prim_object(PRIM_OBJ_WEAPON_GUN);
    load_prim_object(PRIM_OBJ_WEAPON_KNIFE);
    load_prim_object(PRIM_OBJ_WEAPON_SHOTGUN);
    load_prim_object(PRIM_OBJ_WEAPON_BAT);
    load_prim_object(PRIM_OBJ_WEAPON_AK47);
    load_prim_object(PRIM_OBJ_WEAPON_GUN_FLASH);
    load_prim_object(PRIM_OBJ_WEAPON_SHOTGUN_FLASH);
    load_prim_object(PRIM_OBJ_WEAPON_AK47_FLASH);
}

// Sets or clears FACE_FLAG_METAL on all quad faces of a prim.
// uc_orig: set_face_type (fallen/Source/ob.cpp)
static void set_face_type(SLONG prim, SLONG type)
{
    PrimObject* po = &prim_objects[prim];

    for (SLONG j = po->StartFace4; j < po->EndFace4; j++) {
        if (type == 0)
            prim_faces4[j].FaceFlags &= ~FACE_FLAG_METAL;
        else
            prim_faces4[j].FaceFlags |= type;
    }
}

// uc_orig: OB_load_needed_prims (fallen/Source/ob.cpp)
void OB_load_needed_prims()
{
    SLONG i;
    SLONG j;

    for (i = 1; i < OB_ob_upto; i++) {
        load_prim_object(OB_ob[i].prim);

        if (OB_ob[i].prim == PRIM_OBJ_SWITCH_OFF) {
            load_prim_object(PRIM_OBJ_SWITCH_ON);
        }

        if (OB_ob[i].prim == 29) {
            set_face_type(OB_ob[i].prim, FACE_FLAG_WALKABLE);
            prim_objects[29].flag |= PRIM_FLAG_CONTAINS_WALKABLE_FACES;
        }
        if (OB_ob[i].prim >= 12 && OB_ob[i].prim <= 17) {
            set_face_type(OB_ob[i].prim, FACE_FLAG_FIRE_ESCAPE);
        } else {
            if ((OB_ob[i].prim == 29) || (OB_ob[i].prim == 109) || (OB_ob[i].prim == 221)) {
                set_face_type(OB_ob[i].prim, FACE_FLAG_METAL);
            } else {
                set_face_type(OB_ob[i].prim, 0);
            }
        }
    }
    load_general_prims();

    SLONG page;

    for (i = 1; i < SPECIAL_NUM_TYPES; i++) {
        load_prim_object(SPECIAL_info[i].prim);

        // Centre the specials.
        {
            SLONG min_x = +UC_INFINITY;
            SLONG min_y = +UC_INFINITY;
            SLONG min_z = +UC_INFINITY;
            SLONG max_x = -UC_INFINITY;
            SLONG max_y = -UC_INFINITY;
            SLONG max_z = -UC_INFINITY;
            SLONG mid_x, mid_y, mid_z;
            PrimObject* po = &prim_objects[SPECIAL_info[i].prim];

            for (j = po->StartPoint; j < po->EndPoint; j++) {
                if (prim_points[j].X < min_x) min_x = prim_points[j].X;
                if (prim_points[j].Y < min_y) min_y = prim_points[j].Y;
                if (prim_points[j].Z < min_z) min_z = prim_points[j].Z;
                if (prim_points[j].X > max_x) max_x = prim_points[j].X;
                if (prim_points[j].Y > max_y) max_y = prim_points[j].Y;
                if (prim_points[j].Z > max_z) max_z = prim_points[j].Z;
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

// Apply environment mapping to all special prim faces. Skips alpha/transparent pages.
// uc_orig: envmap_specials (fallen/Source/ob.cpp)
void envmap_specials(void)
{
    SLONG i;
    SLONG j;
    SLONG page;

    for (i = 1; i < SPECIAL_NUM_TYPES; i++) {
        PrimObject* po = &prim_objects[SPECIAL_info[i].prim];

        for (j = po->StartFace3; j < po->EndFace3; j++) {
            PrimFace3* p_f3 = &prim_faces3[j];
            page = p_f3->UV[0][0] & 0xc0;
            page <<= 2;
            page |= p_f3->TexturePage;
            page += FACE_PAGE_OFFSET;

            if (POLY_page_flag[page] & (POLY_PAGE_FLAG_ALPHA | POLY_PAGE_FLAG_TRANSPARENT))
                prim_faces3[j].FaceFlags &= ~FACE_FLAG_ENVMAP;
            else
                prim_faces3[j].FaceFlags |= FACE_FLAG_ENVMAP;
        }

        for (j = po->StartFace4; j < po->EndFace4; j++) {
            PrimFace4* p_f4 = &prim_faces4[j];
            page = p_f4->UV[0][0] & 0xc0;
            page <<= 2;
            page |= p_f4->TexturePage;
            page += FACE_PAGE_OFFSET;

            if (POLY_page_flag[page] & (POLY_PAGE_FLAG_ALPHA | POLY_PAGE_FLAG_TRANSPARENT))
                prim_faces4[j].FaceFlags &= ~FACE_FLAG_ENVMAP;
            else
                prim_faces4[j].FaceFlags |= FACE_FLAG_ENVMAP;
        }

        po->flag |= PRIM_FLAG_ENVMAPPED;
    }

    // Strip envmap from any face that is also transparent across all prims.
    for (i = 1; i < 256; i++) {
        PrimObject* po = &prim_objects[i];

        if (po->flag & PRIM_FLAG_ENVMAPPED) {
            for (j = po->StartFace3; j < po->EndFace3; j++) {
                PrimFace3* p_f3 = &prim_faces3[j];
                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;
                page += FACE_PAGE_OFFSET;

                if (POLY_page_flag[page] & (POLY_PAGE_FLAG_ALPHA | POLY_PAGE_FLAG_TRANSPARENT))
                    prim_faces3[j].FaceFlags &= ~FACE_FLAG_ENVMAP;
            }

            for (j = po->StartFace4; j < po->EndFace4; j++) {
                PrimFace4* p_f4 = &prim_faces4[j];
                page = p_f4->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f4->TexturePage;
                page += FACE_PAGE_OFFSET;

                if (POLY_page_flag[page] & (POLY_PAGE_FLAG_ALPHA | POLY_PAGE_FLAG_TRANSPARENT))
                    prim_faces4[j].FaceFlags &= ~FACE_FLAG_ENVMAP;
            }
        }
    }
}

// Returns non-zero if prim is allowed to contribute walkable faces.
// A few hardcoded prim IDs (platforms that move) are excluded.
// uc_orig: ob_allowed_to_be_walkable (fallen/Source/ob.cpp)
static SLONG ob_allowed_to_be_walkable(SLONG prim)
{
    if (prim == 175 || prim == 176 || prim == 171 || prim == 173 || prim == 174 || prim == 183)
        return 0;
    return 1;
}

// uc_orig: OB_add_walkable_faces (fallen/Source/ob.cpp)
void OB_add_walkable_faces()
{
    SLONG i;
    SLONG j;
    SLONG px, py, pz;
    SLONG mx, mz;
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
                if (ob_allowed_to_be_walkable(oi->prim))
                    if (po->flag & PRIM_FLAG_CONTAINS_WALKABLE_FACES) {
                        FMATRIX_calc(matrix, oi->yaw, 0, 0);

                        for (i = po->StartFace4; i < po->EndFace4; i++) {
                            f4 = &prim_faces4[i];

                            if (f4->FaceFlags & FACE_FLAG_WALKABLE) {
                                for (j = 0; j < 4; j++) {
                                    ASSERT(WITHIN(f4->Points[j], 1, next_prim_point - 1));
                                    ASSERT(WITHIN(next_prim_point, 1, MAX_PRIM_POINTS - 1));

                                    px = prim_points[f4->Points[j]].X;
                                    py = prim_points[f4->Points[j]].Y;
                                    pz = prim_points[f4->Points[j]].Z;

                                    FMATRIX_MUL_BY_TRANSPOSE(matrix, px, py, pz);

                                    px += oi->x;
                                    py += oi->y;
                                    pz += oi->z;

                                    // Snap to mapsquare boundaries.
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

                                    if (!WITHIN(px, 0, (PAP_SIZE_HI << 8) - 1) ||
                                        !WITHIN(pz, 0, (PAP_SIZE_HI << 8) - 1)) {
                                        goto abandon_face;
                                    }

                                    prim_points[next_prim_point].X = px;
                                    prim_points[next_prim_point].Y = py;
                                    prim_points[next_prim_point].Z = pz;
                                    next_prim_point += 1;
                                }

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

                                attach_walkable_to_map(next_prim_face4 - 1);

                            abandon_face:;
                            }
                        }
                    }
            }
        }

    // Clear WALKABLE flag from all prim object faces so calc_slide_edges() works correctly.
    for (i = 0; i < MAX_PRIM_OBJECTS; i++) {
        po = &prim_objects[i];
        for (j = po->StartFace4; j < po->EndFace4; j++) {
            prim_faces4[j].FaceFlags &= ~FACE_FLAG_WALKABLE;
        }
    }
}

// uc_orig: OB_remove (fallen/Source/ob.cpp)
void OB_remove(OB_Info* oi)
{
    SLONG lo_map_x = oi->x >> PAP_SHIFT_LO;
    SLONG lo_map_z = oi->z >> PAP_SHIFT_LO;

    ASSERT(WITHIN(lo_map_x, 0, PAP_SIZE_LO - 1));
    ASSERT(WITHIN(lo_map_z, 0, PAP_SIZE_LO - 1));

    OB_Mapwho* om = &OB_mapwho[lo_map_x][lo_map_z];

    ASSERT(WITHIN(oi->index, om->index, om->index + om->num));

    OB_ob[oi->index] = OB_ob[om->index + --om->num];

    SLONG i;
    PrimFace4* f4;

    for (i = 1; i < next_prim_face4; i++) {
        f4 = &prim_faces4[i];
        if (f4->FaceFlags & FACE_FLAG_PRIM) {
            if (f4->ThingIndex == -oi->index) {
                remove_walkable_from_map(i);
            }
        }
    }
}

// Checks whether prim_flags > 255 special type matches this OB.
// uc_orig: special_object_flag (fallen/Source/ob.cpp)
static SLONG special_object_flag(OB_Info* ob, SLONG flags)
{
    if (flags & FIND_OB_TRIPWIRE) {
        if (ob->prim == PRIM_OBJ_TRIPWIRE)
            return UC_TRUE;
    }
    if (flags & FIND_OB_SWITCH_OR_VALVE) {
        if (ob->prim == PRIM_OBJ_VALVE)
            return UC_TRUE;
        if (ob->prim == PRIM_OBJ_SWITCH_OFF)
            return UC_TRUE;
    }
    return UC_FALSE;
}

// uc_orig: OB_find_type (fallen/Source/ob.cpp)
SLONG OB_find_type(
    SLONG mid_x, SLONG mid_y, SLONG mid_z, SLONG max_range,
    ULONG prim_flags,
    SLONG* ob_x, SLONG* ob_y, SLONG* ob_z, SLONG* ob_yaw,
    SLONG* ob_prim, SLONG* ob_index)
{
    SLONG mx, mz;
    SLONG mx1, mz1, mx2, mz2;
    SLONG dx, dy, dz;
    SLONG dist;
    SLONG best_dist = UC_INFINITY;
    SLONG best_x, best_y, best_z, best_yaw, best_prim, best_index;
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
                if ((prim_objects[oi->prim].flag & prim_flags) ||
                    (prim_flags > 255 && special_object_flag(oi, prim_flags))) {
                    dx = oi->x - mid_x;
                    dy = oi->y - mid_y;
                    dz = oi->z - mid_z;
                    dist = abs(dx) + abs(dy) + abs(dz);

                    if (dist < max_range && dist < best_dist) {
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

    if (best_dist == UC_INFINITY)
        return UC_FALSE;

    *ob_x = best_x;
    *ob_y = best_y;
    *ob_z = best_z;
    *ob_yaw = best_yaw;
    *ob_prim = best_prim;
    *ob_index = best_index;
    return UC_TRUE;
}

// uc_orig: OB_find_index (fallen/Source/ob.cpp)
OB_Info* OB_find_index(SLONG mid_x, SLONG mid_y, SLONG mid_z, SLONG max_range,
                       SLONG must_be_searchable)
{
    SLONG mx, mz;
    SLONG mx1, mz1, mx2, mz2;
    SLONG dx, dy, dz;
    SLONG dist;
    SLONG best_dist = UC_INFINITY;
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

                    if (dist < max_range && dist < best_dist) {
                        best_ob = oi;
                        best_dist = dist;
                    }
                }
            }
            if (best_ob) {
                OB_return = *best_ob;
                best_ob = &OB_return;
            }
        }

    return best_ob;
}

// uc_orig: OB_find_min_y (fallen/Source/ob.cpp)
SLONG OB_find_min_y(SLONG prim)
{
    SLONG sp = prim_objects[prim].StartPoint;
    SLONG ep = prim_objects[prim].EndPoint;
    SLONG c0;
    SLONG min_y = 999999;

    for (c0 = sp; c0 < ep; c0++) {
        if (prim_points[c0].Y < min_y)
            min_y = prim_points[c0].Y;
    }
    return min_y;
}

// Empty stub — was functional but disabled and replaced ("FIXED IN ANOTHER WAY NOW").
// uc_orig: OB_height_fiddle_de_dee (fallen/Source/ob.cpp)
static void OB_height_fiddle_de_dee(void)
{
    // Body is fully commented out in the original; kept as dead stub.
}

// uc_orig: OB_damage (fallen/Source/ob.cpp)
void OB_damage(
    SLONG index,
    SLONG from_dx, SLONG from_dz,
    SLONG x, SLONG z,
    Thing* p_aggressor)
{
    OB_Ob* oo;
    OB_Hydrant* oh;
    PrimObject* po;

    // Only CLASS_PERSON aggressors trigger damage effects.
    if (p_aggressor && p_aggressor->Class != CLASS_PERSON)
        p_aggressor = NULL;

    ASSERT(WITHIN(index, 1, OB_ob_upto - 1));
    oo = &OB_ob[index];
    ASSERT(WITHIN(oo->prim, 1, next_prim_object - 1));
    po = &prim_objects[oo->prim];

    if (po->damage & PRIM_DAMAGE_DAMAGABLE) {
        if (po->damage & PRIM_DAMAGE_LEAN) {
            SLONG lx = SIN(oo->yaw << 3) >> 10;
            SLONG lz = COS(oo->yaw << 3) >> 10;
            SLONG dprod = lx * from_dx + lz * from_dz;
            SLONG cprod = lx * from_dz - lz * from_dx;

            oo->flags &= 0x3f;
            if (cprod > 0) oo->flags |= OB_FLAG_RESERVED1;
            if (dprod > 0) oo->flags |= OB_FLAG_RESERVED2;
            oo->flags |= Random() & 0xc0;

        } else if (po->damage & PRIM_DAMAGE_CRUMPLE) {
            UBYTE crumple = oo->flags >> 6;
            crumple += 1;
            if (crumple == 5) crumple = 3;
            oo->flags &= 0x3f;
            oo->flags |= crumple << 6;
        }

        if (oo->prim == PRIM_OBJ_HYDRANT) {
            OB_hydrant_last += 1;
            OB_hydrant_last &= OB_MAX_HYDRANTS - 1;
            oh = &OB_hydrant[OB_hydrant_last];
            oh->life = 16 * 20 * 15;
            oh->index = index;
            oh->x = x;
            oh->z = z;
            MFX_play_xyz(MAX_THINGS + OB_hydrant_last, S_FIRE_HYDRANT, MFX_LOOPED,
                         x << 8, PAP_calc_map_height_at(x, z) << 8, z << 8);
        }

        if (((po->damage & PRIM_DAMAGE_EXPLODES) || (po->flag & PRIM_FLAG_LAMPOST)) &&
            !(oo->flags & OB_FLAG_DAMAGED)) {
            GameCoord pos;
            pos.X = x << 8;
            pos.Z = z << 8;
            pos.Y = oo->y << 8;

            PYRO_create(pos, PYRO_FIREBOMB);
            MFX_play_xyz(0, SOUND_Range(S_EXPLODE_MEDIUM, S_EXPLODE_BIG), 0, pos.X, pos.Y, pos.Z);

            oo->flags |= OB_FLAG_DAMAGED;
            create_shockwave(x, oo->y, z, 0x300, 150, p_aggressor);
        }

        oo->flags |= OB_FLAG_DAMAGED;
    }
}

// uc_orig: OB_convert_dustbins_to_barrels (fallen/Source/ob.cpp)
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
                    BARREL_alloc(BARREL_TYPE_NORMAL, oi->prim, oi->x, oi->z, NULL);
                    OB_remove(oi);
                    goto start_again_because_weve_removed_an_ob;
                }
            }
        }
}

// uc_orig: OB_inside_prim (fallen/Source/ob.cpp)
SLONG OB_inside_prim(SLONG x, SLONG y, SLONG z)
{
    SLONG mx, mz;
    SLONG dx, dy, dz;
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
                    if (dist < 0x100)
                        return UC_TRUE;
                }
            }
        }

    return UC_FALSE;
}

// uc_orig: OB_make_all_the_switches_be_at_the_proper_height (fallen/Source/ob.cpp)
void OB_make_all_the_switches_be_at_the_proper_height()
{
    SLONG i;
    SLONG mx, mz;
    OB_Info* oi;

    // Hardcoded list of prim IDs that must snap their Y to map height.
    static UBYTE prims_to_snap[] = {
        1, 2, 3, 34, 35, 52, 53, 54, 55, 58, 59, 70, 78, 93, 107, 134,
        149, 161, 162, 163, 164, 169, 206, 213, 214, 218, 207, 222, 228, 230, 0
    };

    for (mx = 0; mx < PAP_SIZE_LO; mx++)
        for (mz = 0; mz < PAP_SIZE_LO; mz++) {
            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                if (oi->flags & OB_FLAG_WAREHOUSE)
                    continue;

                if (oi->prim == PRIM_OBJ_SWITCH_OFF) {
                    SLONG gx = oi->x + (SIN(oi->yaw) >> 10);
                    SLONG gz = oi->z + (COS(oi->yaw) >> 10);
                    SLONG gy = PAP_calc_map_height_at(gx, gz);
                    OB_ob[oi->index].y = gy + 0x80;
                }

                for (i = 0; prims_to_snap[i]; i++) {
                    if (oi->prim == prims_to_snap[i]) {
                        OB_ob[oi->index].y = PAP_calc_map_height_at(oi->x, oi->z);
                    }
                }

                {
                    // Clunk the Y of all walkways to the nearest 0x40 boundary.
                    for (i = 0; i < OB_ob_upto; i++) {
                        if (OB_ob[i].prim == 29 || OB_ob[i].prim == 129 ||
                            WITHIN(OB_ob[i].prim, 12, 17)) {
                            OB_ob[i].y += 0x20;
                            OB_ob[i].y &= ~0x3f;
                        }
                    }
                }
            }
        }
}
