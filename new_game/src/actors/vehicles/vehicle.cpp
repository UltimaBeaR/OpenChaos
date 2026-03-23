// uc_orig: vehicle.cpp (fallen/Source/Vehicle.cpp)
// Vehicle physics, collision, rendering, damage, and enter/exit logic.
// All chunks migrated: VehInfo table, alloc/dealloc, draw_car, utility functions,
// collision system (VEH_collide_find_things, CollideCar, CollideWithKerb),
// main vehicle tick (VEH_driving), steering/pedals/suspension physics,
// VEH_reduce_health, and wheel position query API.

#include <math.h>

#include <platform.h>            // base types, ASSERT
#include "missions/game_types.h" // Game struct, TICK_RATIO, pool macros
#include "core/matrix.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/geometry/oval.h"

#include "world/map/pap.h"
#include "world/map/pap_globals.h"
#include "core/fmatrix.h"
#include "actors/core/statedef.h"
#include "world/environment/prim_types.h" // PrimObject, PrimFace3/4, PRIM_OBJ_*, FACE_FLAG_*
#include "world/environment/prim.h"       // slide_along_prim, get_prim_info, etc.
#include "actors/characters/anim_ids.h"
#include "engine/audio/sound.h"
#include "actors/core/interact.h"
#include "actors/core/interact_globals.h"
#include "world/map/ob.h"
#include "world/map/ob_globals.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"
#include "engine/graphics/geometry/bloom.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/geometry/mesh.h"
#include "effects/pow.h"
#include "effects/pow_globals.h"
#include "ui/interfac.h"
#include "ui/interfac_globals.h"
#include "effects/dirt.h"
#include "effects/mist.h"
#include "actors/items/barrel.h"
#include "actors/items/barrel_globals.h"
#include "world/map/road.h"
#include "world/map/road_globals.h"
#include "engine/graphics/resources/font2d.h"
#include "missions/memory.h"
#include "world/level_pools.h"
#include "engine/audio/mfx.h"
#include "effects/tracks.h"
#include "engine/effects/psystem.h"
#include "actors/vehicles/vehicle.h"
#include "actors/vehicles/vehicle_globals.h"
#include "actors/characters/person.h"          // set_person_exit_vehicle, knock_person_down, set_person_flip, set_person_ko_recoil, person_is_lying_on_what
#include "engine/physics/collide.h"            // collide_box_with_line, distance_to_line

#include "ai/pcom.h"
// engine/effects/psystem.h already included above; psystem_globals needed too
#include "engine/effects/psystem_globals.h"

extern BOOL allow_debug_keys;
extern SLONG is_person_ko(Thing* p_person);
extern SLONG is_person_ko_and_lay_down(Thing* p_person);

// Forward declarations for functions defined later in this file (chunks 2 and 3).
// uc_orig: siren (fallen/Source/Vehicle.cpp)
static void siren(Vehicle* veh, UBYTE play);
// uc_orig: GetCarPoints (fallen/Source/Vehicle.cpp)
static inline void GetCarPoints(Thing* p_car, SLONG* x, SLONG* y, SLONG* z, SLONG step);
// uc_orig: init_arctans (fallen/Source/Vehicle.cpp)
static void init_arctans(void);
// uc_orig: process_car (fallen/Source/Vehicle.cpp)
static void process_car(Thing* p_car);
// uc_orig: CollideCar (fallen/Source/Vehicle.cpp)
static SLONG CollideCar(Thing* p_car, SLONG step);
// uc_orig: CollideWithKerb (fallen/Source/Vehicle.cpp)
static void CollideWithKerb(Thing* p_car);
// uc_orig: do_car_input (fallen/Source/Vehicle.cpp)
static void do_car_input(Thing* p_thing);

// Physics and suspension constants.
// uc_orig: MIN_COMPRESSION (fallen/Source/Vehicle.cpp)
#define MIN_COMPRESSION (13 << 8)
// uc_orig: MAX_COMPRESSION (fallen/Source/Vehicle.cpp)
#define MAX_COMPRESSION (115 << 8)
// uc_orig: UNITS_PER_METER (fallen/Source/Vehicle.cpp)
#define UNITS_PER_METER 128
// uc_orig: TICK_LOOP (fallen/Source/Vehicle.cpp)
#define TICK_LOOP (4)
// uc_orig: TICKS_PER_SECOND (fallen/Source/Vehicle.cpp)
#define TICKS_PER_SECOND (20 * TICK_LOOP)
// uc_orig: GRAVITY (fallen/Source/Vehicle.cpp)
#define GRAVITY (-(UNITS_PER_METER * 10 * 256) / (TICKS_PER_SECOND * TICKS_PER_SECOND))

// uc_orig: SKID_START (fallen/Source/Vehicle.cpp)
#define SKID_START 3
// uc_orig: SKID_FORCE (fallen/Source/Vehicle.cpp)
#define SKID_FORCE 8500
// uc_orig: NEAR_SKID_FORCE (fallen/Source/Vehicle.cpp)
#define NEAR_SKID_FORCE 5000
// uc_orig: STABLE_COUNT (fallen/Source/Vehicle.cpp)
#define STABLE_COUNT 16
// uc_orig: CAR_VEL_SHIFT (fallen/Source/Vehicle.cpp)
#define CAR_VEL_SHIFT 4

// uc_orig: get_car_door_offsets (fallen/Source/Vehicle.cpp)
void get_car_door_offsets(SLONG type, SLONG door, SLONG* dx, SLONG* dz)
{
    ASSERT(door == 0 || door == 1);

    if (sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim) {
        *dx = (veh_info[type].DX[door + 1] * 450) >> 8;
    } else {
        *dx = (veh_info[type].DX[door + 1] * 300) >> 8;
    }

    *dz = (veh_info[type].DZ[door + 1] * 50) >> 8;
}

// uc_orig: get_vehicle_body_prim (fallen/Source/Vehicle.cpp)
UWORD get_vehicle_body_prim(SLONG type)
{
    ASSERT(WITHIN(type, 0, VEH_TYPE_NUMBER - 1));

    return (veh_info[type].BodyPrim);
}

// uc_orig: get_vehicle_body_offset (fallen/Source/Vehicle.cpp)
SLONG get_vehicle_body_offset(SLONG type)
{
    ASSERT(WITHIN(type, 0, VEH_TYPE_NUMBER - 1));

    return (SLONG(veh_info[type].BodyOffset));
}

// uc_orig: make_car_matrix (fallen/Source/Vehicle.cpp)
void make_car_matrix(Vehicle* v)
{
    FMATRIX_calc(car_matrix, v->Angle, v->Tilt, v->Roll);
}

// Variant that takes explicit params — used for collision prediction (ignores tilt/roll).
// uc_orig: make_car_matrix_p (fallen/Source/Vehicle.cpp)
void make_car_matrix_p(SLONG angle, SLONG tilt, SLONG roll)
{
    FMATRIX_calc(car_matrix, angle, tilt, roll);
}

// uc_orig: apply_car_matrix (fallen/Source/Vehicle.cpp)
void apply_car_matrix(SLONG* x, SLONG* y, SLONG* z)
{
    SLONG tx, ty, tz;

    tx = *x;
    ty = *y;
    tz = *z;
    FMATRIX_MUL_BY_TRANSPOSE(car_matrix, tx, ty, tz);
    *x = tx;
    *y = ty;
    *z = tz;
}

// uc_orig: get_vehicle_driver (fallen/Source/Vehicle.cpp)
Thing* get_vehicle_driver(Thing* p_vehicle)
{
    Thing* p_driver;

    ASSERT(p_vehicle->Class == CLASS_VEHICLE);

    if (p_vehicle->Genus.Vehicle->Driver) {
        p_driver = TO_THING(p_vehicle->Genus.Vehicle->Driver);
    } else {
        p_driver = NULL;
    }

    return p_driver;
}

// Returns true if the vehicle's driver is the player character.
// Non-static so old/Vehicle.cpp chunks 2+3 can access it until migrated.
// uc_orig: is_driven_by_player (fallen/Source/Vehicle.cpp)
bool is_driven_by_player(Thing* p_car)
{
    if (p_car->Genus.Vehicle->Driver) {
        Thing* p_driver = TO_THING(p_car->Genus.Vehicle->Driver);

        ASSERT(p_driver->Class == CLASS_PERSON);

        if (p_driver->Genus.Person->PlayerID) {
            return true;
        }
    }

    return false;
}

// Searches two sphere regions in front of the vehicle for persons/vehicles to potentially run over.
// Search radius shrinks when the vehicle is turning (tighter cone).
// uc_orig: VEH_find_runover_things (fallen/Source/Vehicle.cpp)
SLONG VEH_find_runover_things(Thing* p_vehicle, UWORD thing_index[], SLONG max_number, SLONG dangle)
{
    SLONG i;

    SLONG dx;
    SLONG dz;

    SLONG cx;
    SLONG cy;
    SLONG cz;

    SLONG num;
    SLONG angle;
    SLONG infront;

#define MAX_INFRONT 512

    switch (p_vehicle->Class) {
    case CLASS_VEHICLE:
        angle = p_vehicle->Genus.Vehicle->Angle & 2047;
        infront = MAX_INFRONT;
        if (!dangle)
            infront -= abs(p_vehicle->Genus.Vehicle->WheelAngle * 3);
        break;

    default:
        ASSERT(0);
        break;
    }

    dx = -SIN(angle);
    dz = -COS(angle);

    SATURATE(infront, 256, MAX_INFRONT);

    dx = dx * infront >> 8;
    dz = dz * infront >> 8;

    cx = p_vehicle->WorldPos.X + dx >> 8;
    cy = p_vehicle->WorldPos.Y >> 8;
    cz = p_vehicle->WorldPos.Z + dz >> 8;

    if (ControlFlag && allow_debug_keys) {
        AENG_world_line(
            p_vehicle->WorldPos.X >> 8,
            p_vehicle->WorldPos.Y >> 8,
            p_vehicle->WorldPos.Z >> 8,
            32,
            0xffffff,
            cx,
            cy,
            cz,
            0,
            0xff0000,
            UC_TRUE);
    }

    num = THING_find_sphere(
        cx, cy, cz,
        infront,
        thing_index,
        max_number,
        (1 << CLASS_PERSON) | (1 << CLASS_VEHICLE));

    cx += dx >> 8;
    cz += dz >> 8;

    num += THING_find_sphere(cx, cy, cz, infront, thing_index + num, max_number - num, (1 << CLASS_PERSON) | (1 << CLASS_VEHICLE));

    for (i = 0; i < num; i++) {
        if (thing_index[i] == THING_NUMBER(p_vehicle)) {
            thing_index[i] = thing_index[--num];

            break;
        }
    }

    return num;
}

// Returns world X/Z position of the driver or passenger door for enter/exit animation.
// uc_orig: VEH_find_door (fallen/Source/Vehicle.cpp)
void VEH_find_door(Thing* p_vehicle, SLONG i_am_a_passenger, SLONG* door_x, SLONG* door_z)
{
    SLONG dx;
    SLONG dz;

    SLONG ix;
    SLONG iz;

    ASSERT(p_vehicle->Class == CLASS_VEHICLE);

    dx = -SIN(p_vehicle->Genus.Vehicle->Angle);
    dz = -COS(p_vehicle->Genus.Vehicle->Angle);

    ix = p_vehicle->WorldPos.X >> 8;
    iz = p_vehicle->WorldPos.Z >> 8;

    if (i_am_a_passenger) {
        /*
                        switch(p_vehicle->Genus.Vehicle->Type)
                        {
                                case VEH_TYPE_VAN:
                                case VEH_TYPE_AMBULANCE:
                                case VEH_TYPE_JEEP:
                                case VEH_TYPE_MEATWAGON:
                                case VEH_TYPE_WILDCATVAN:

                                        ix -= dx >> 10;
                                        iz -= dz >> 10;

                                        break;

                                case VEH_TYPE_CAR:
                                case VEH_TYPE_TAXI:
                                case VEH_TYPE_POLICE:
                                case VEH_TYPE_SEDAN:

                                        ix -= dx >> 10;
                                        iz -= dz >> 10;

                                        ix += dz >> 9;
                                        iz -= dx >> 9;

                                        break;

                                default:
                                        ASSERT(0);
                                        break;
                        }
        */

        ix += dx >> 10;
        iz += dz >> 10;

        ix -= dz >> 9;
        iz += dx >> 9;

        ix -= dz >> 11;
        iz += dx >> 11;

    } else {
        ix += dx >> 10;
        iz += dz >> 10;

        ix += dz >> 9;
        iz -= dx >> 9;

        ix += dz >> 11;
        iz -= dx >> 11;
    }

    *door_x = ix;
    *door_z = iz;
}

// Returns UC_TRUE if all 4 edges of the vehicle (7 sample points each) lie on road tiles.
// step parameter selects current (0) or predicted (TICK_RATIO) position.
// uc_orig: VEH_on_road (fallen/Source/Vehicle.cpp)
SLONG VEH_on_road(Thing* p_vehicle, SLONG step)
{
    SLONG x[4];
    SLONG y[4];
    SLONG z[4];

    GetCarPoints(p_vehicle, x, y, z, step);

    for (int i = 0; i < 4; i++) {
        SLONG x1 = x[i];
        SLONG z1 = z[i];

        SLONG x2 = x[(i + 1) & 0x3];
        SLONG z2 = z[(i + 1) & 0x3];

        SLONG dx = (x2 - x1) >> 3;
        SLONG dz = (z2 - z1) >> 3;

        SLONG cx = x1;
        SLONG cz = z1;

        for (int j = 0; j < 7; j++) {
            if (!ROAD_is_road(cx >> 8, cz >> 8)) {
                return UC_FALSE;
            }

            cx += dx;
            cz += dz;
        }
    }

    return UC_TRUE;
}

// Applies hp damage to one of 6 crumple zones (area 0-5).
// Adjacent zones are equalised and the opposite corner is slightly relieved.
// uc_orig: VEH_add_damage (fallen/Source/Vehicle.cpp)
void VEH_add_damage(Vehicle* vp, UBYTE area, UBYTE hp)
{
    if (hp > 2)
        hp = 2;

    if (!hp) {
        if (vp->damage[area] < 2)
            vp->damage[area]++;
    } else {
        vp->damage[area] += hp;
        if (vp->damage[area] > 4)
            vp->damage[area] = 4;
    }

    if (vp->damage[2] < (vp->damage[0] + vp->damage[4]) / 2) {
        vp->damage[2] = (vp->damage[0] + vp->damage[4]) / 2;
    }
    if (vp->damage[3] < (vp->damage[1] + vp->damage[5]) / 2) {
        vp->damage[3] = (vp->damage[1] + vp->damage[5]) / 2;
    }

    if ((vp->damage[0] + vp->damage[1] > 6) && (area == 0))
        vp->damage[1]--;
    if ((vp->damage[0] + vp->damage[1] > 6) && (area == 1))
        vp->damage[0]--;

    if ((vp->damage[4] + vp->damage[5] > 6) && (area == 4))
        vp->damage[5]--;
    if ((vp->damage[4] + vp->damage[5] > 6) && (area == 5))
        vp->damage[4]--;

    if ((hp == 2) && (Random() > 25000) && (vp->damage[5 - area]))
        vp->damage[5 - area]--;
}

// Applies upward impulse to one or two wheel DY values on impact.
// Area mapping: 0=FL, 1=FR, 2=right side(FR+RR), 3=left side(FL+RL), 4=RR, 5=RL.
// uc_orig: VEH_bounce (fallen/Source/Vehicle.cpp)
void VEH_bounce(Vehicle* vp, UBYTE area, SLONG amount)
{
    amount *= 2;

    switch (area) {
    case 0:
        vp->DY[1] -= amount;
        break;

    case 1:
        vp->DY[0] -= amount;
        break;

    case 2:
        vp->DY[1] -= amount;
        vp->DY[3] -= amount;
        break;

    case 3:
        vp->DY[0] -= amount;
        vp->DY[2] -= amount;
        break;

    case 4:
        vp->DY[3] -= amount;
        break;

    case 5:
        vp->DY[2] -= amount;
        break;
    }
}

// Mark all vehicle pool slots as free.
// uc_orig: init_vehicles (fallen/Source/Vehicle.cpp)
void init_vehicles(void)
{
    SLONG i;

    for (i = 0; i < MAX_VEHICLES; i++) {
        TO_VEHICLE(i)->Spring[0].Compression = VEH_NULL;
    }
}

// Assigns mesh vertices to crumple zones by nearest-neighbor to 6 reference points.
// Also initialises the arctan steering lookup table. Must be called after prims are loaded.
// uc_orig: VEH_init_vehinfo (fallen/Source/Vehicle.cpp)
void VEH_init_vehinfo()
{
    int ii;

    init_arctans();

    for (ii = 0; ii < VEH_TYPE_NUMBER; ii++) {
        if (veh_info[ii].VertexAssignments) {
            MemFree(veh_info[ii].VertexAssignments);
        }
    }

    for (ii = 0; ii < VEH_TYPE_NUMBER; ii++) {
        PrimObject* obj = &prim_objects[veh_info[ii].BodyPrim];
        PrimInfo* inf = get_prim_info(veh_info[ii].BodyPrim);
        SLONG px[6], pz[6];

        px[0] = inf->minx;
        pz[0] = inf->minz;
        px[1] = inf->maxx;
        pz[1] = inf->minz;
        px[2] = inf->minx;
        pz[2] = (inf->minz + inf->maxz) / 2;
        px[3] = inf->maxx;
        pz[3] = (inf->minz + inf->maxz) / 2;
        px[4] = inf->minx;
        pz[4] = inf->maxz;
        px[5] = inf->maxx;
        pz[5] = inf->maxz;

        veh_info[ii].NumVertices = obj->EndPoint - obj->StartPoint;

        veh_info[ii].VertexAssignments = (UBYTE*)MemAlloc(obj->EndPoint - obj->StartPoint);

        for (int jj = obj->StartPoint; jj < obj->EndPoint; jj++) {
            SLONG maxdist = 0x7FFFFFFF;
            SLONG best = -1;

            SLONG x = prim_points[jj].X;
            SLONG z = prim_points[jj].Z;

            for (int kk = 0; kk < 6; kk++) {
                SLONG dist = (x - px[kk]) * (x - px[kk]) + (z - pz[kk]) * (z - pz[kk]);

                if (dist < maxdist) {
                    maxdist = dist;
                    best = kk;
                }
            }

            ASSERT(best != -1);

            veh_info[ii].VertexAssignments[jj - obj->StartPoint] = best;
        }
    }
}

// Returns vertex-to-crumple-zone assignment map for a body prim by scanning veh_info[].
// uc_orig: VEH_get_assignments (fallen/Source/Vehicle.cpp)
UBYTE* VEH_get_assignments(ULONG prim)
{
    for (int ii = 0; ii < VEH_TYPE_NUMBER; ii++) {
        if (veh_info[ii].BodyPrim == prim) {
            return veh_info[ii].VertexAssignments;
        }
    }
    return NULL;
}

// Scans the Vehicle pool for a free slot (Spring[0].Compression == VEH_NULL).
// Claims the slot by setting Spring[0].Compression = MIN_COMPRESSION.
// uc_orig: VEH_alloc (fallen/Source/Vehicle.cpp)
Vehicle* VEH_alloc(void)
{
    SLONG i;

    for (i = 0; i < MAX_VEHICLES; i++) {
        if (TO_VEHICLE(i)->Spring[0].Compression == VEH_NULL) {
            Vehicle* ans = TO_VEHICLE(i);

            ans->Spring[0].Compression = MIN_COMPRESSION;

            return ans;
        }
    }

    ASSERT(0);

    return NULL;
}

// uc_orig: VEH_dealloc (fallen/Source/Vehicle.cpp)
void VEH_dealloc(Vehicle* veh)
{
    veh->Spring[0].Compression = VEH_NULL;
    if (veh->dlight)
        NIGHT_dlight_destroy(veh->dlight);
}

// Spawns a vehicle Thing at world position with given orientation, type, and key lock.
// Yaw is adjusted +1024 internally (vehicles use a different yaw convention).
// Creates a dynamic light if not daytime.
// uc_orig: VEH_create (fallen/Source/Vehicle.cpp)
THING_INDEX VEH_create(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG yaw,
    SLONG pitch,
    SLONG roll,
    SLONG type,
    UBYTE key,
    UBYTE colour)
{
    DrawMesh* dm;
    THING_INDEX ans = NULL;
    Thing* p_thing;
    int ii;

    ASSERT(WITHIN(type, 0, VEH_TYPE_NUMBER - 1));

    yaw += 1024;
    yaw &= 2047;

    dm = alloc_draw_mesh();

    if (dm) {
        ans = alloc_primary_thing(CLASS_VEHICLE);

        if (ans) {
            Vehicle* vp;

            p_thing = TO_THING(ans);

            p_thing->Class = CLASS_VEHICLE;
            p_thing->State = 0;
            p_thing->SubState = 0;
            p_thing->DrawType = DT_VEHICLE;
            p_thing->Flags = 0;
            p_thing->WorldPos.X = x;
            p_thing->WorldPos.Y = y;
            p_thing->WorldPos.Z = z;

            p_thing->Draw.Mesh = dm;

            dm->Angle = yaw;

            vp = p_thing->Genus.Vehicle = VEH_alloc();

            ASSERT(vp);

            vp->Angle = yaw;
            vp->Roll = roll;
            vp->Tilt = pitch;
            vp->Flags = 0;

            for (ii = 0; ii < 4; ii++) {
                vp->Spring[ii].Compression = MIN_COMPRESSION;
                vp->DY[ii] = 0;
            }

            vp->Type = type;

            if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME)) {
                vp->dlight = NIGHT_dlight_create(p_thing->WorldPos.X >> 8, p_thing->WorldPos.Y >> 8, p_thing->WorldPos.Z >> 8, 200, 35, 32, 10);
            } else {
                vp->dlight = 0;
            }

            reinit_vehicle(p_thing);

            set_state_function(p_thing, STATE_FDRIVING);

            p_thing->Genus.Vehicle->Roll = 0;
            p_thing->Genus.Vehicle->Tilt = 0;
            p_thing->Genus.Vehicle->key = key;

            add_thing_to_map(p_thing);
        } else {
            ASSERT(0);
        }
    } else {
        ASSERT(0);
    }

    return ans;
}

// Resets all dynamic vehicle state: velocities=0, health=300, damage[6]=0,
// springs at resting compression=4300. Snaps Y to map height + spring extension (107 units).
// uc_orig: reinit_vehicle (fallen/Source/Vehicle.cpp)
void reinit_vehicle(Thing* p_thing)
{
    Vehicle* vp = p_thing->Genus.Vehicle;

    vp->VelX = 0;
    vp->VelY = 0;
    vp->VelZ = 0;
    vp->VelR = 0;
    vp->Dir = 0;
    vp->WheelAngle = 0;
    vp->Skid = 0;
    vp->Stable = 0;
    vp->Smokin = 0;
    vp->OnRoadFlags = 0;
    vp->Health = 300;
    vp->Siren = 0;
    vp->GrabAction = 0;

    for (int ii = 0; ii < 6; ii++) {
        vp->damage[ii] = 0;
    }

    SLONG height;

    for (int ii = 0; ii < 4; ii++) {
        vp->DY[ii] = 0;
        vp->Spring[ii].Compression = 4300;
    }

    vp->Tilt = 0;
    vp->Roll = 0;
    height = PAP_calc_map_height_at(p_thing->WorldPos.X >> 8, p_thing->WorldPos.Z >> 8);
    // 107 = spring rest extension: 128 * 0.84 (84% compressed at rest)
    p_thing->WorldPos.Y = (height + 107) << 8;
}

// uc_orig: free_vehicle (fallen/Source/Vehicle.cpp)
void free_vehicle(Thing* p_thing)
{
    ASSERT(0);
    if (p_thing->Genus.Vehicle) {
        VEH_dealloc(p_thing->Genus.Vehicle);
    }

    free_draw_mesh(p_thing->Draw.Mesh);

    free_thing(p_thing);
}

// Forward declarations for calc functions defined in chunk 3.
// uc_orig: calc_car_collision_turn (fallen/Source/Vehicle.cpp)
SLONG calc_car_collision_turn(Thing* p_car, SLONG angle, SLONG tilt, SLONG roll);
// uc_orig: calc_car_normal (fallen/Source/Vehicle.cpp)
void calc_car_normal(SLONG* p, SLONG* dy, SLONG* nx, SLONG* ny, SLONG* nz);
// uc_orig: calc_car_vect (fallen/Source/Vehicle.cpp)
void calc_car_vect(SLONG p1, SLONG p2, SLONG* dy, SLONG* vx, SLONG* vy, SLONG* vz);
// uc_orig: normalise_val256 (fallen/Source/Vehicle.cpp)
void normalise_val256(SLONG* vx, SLONG* vy, SLONG* vz);

// animate_car: dead stub — body starts with ASSERT(0) and returns immediately.
// The actual vehicle animation is handled by draw_car via MESH_draw_poly.
// uc_orig: animate_car (fallen/Source/Vehicle.cpp)
void animate_car(Thing* p_car)
{
    SLONG tween_step;
    DrawTween* draw_info;
    ASSERT(0);
    return;

    draw_info = &p_car->Genus.Vehicle->Draw;
    tween_step = draw_info->CurrentFrame->TweenStep << 1;

    tween_step = (tween_step * TICK_RATIO) >> TICK_SHIFT;
    if (tween_step == 0)
        tween_step = 1;
    draw_info->AnimTween += tween_step;

    if (p_car->Genus.Vehicle->Draw.AnimTween > 256) {
        p_car->Genus.Vehicle->Draw.AnimTween -= 256;

        SLONG advance_keyframe(DrawTween * draw_info);
        advance_keyframe(&p_car->Genus.Vehicle->Draw);
    }
}

// Renders the vehicle body mesh, wheels, lights, smoke, skidmarks, and shadow.
// Steps: crumple deform → body mesh → shadow → headlights → flashing lights →
//        engine smoke → brakelights → wheel meshes → tyre smoke + skidmarks.
// uc_orig: draw_car (fallen/Source/Vehicle.cpp)
void draw_car(Thing* p_car)
{
    SLONG x[8], y[8], z[8];
    SLONG vector[3];
    SLONG dx, dy, dz;
    SLONG c0 = 0;
    struct VehInfo* info;
    SLONG tilt;
    Vehicle* vp;

    vp = p_car->Genus.Vehicle;
    info = &veh_info[p_car->Genus.Vehicle->Type];

    make_car_matrix(p_car->Genus.Vehicle);
    if (0) {
        extern void ANIM_obj_draw(Thing * p_thing, DrawTween * dt);

        p_car->WorldPos.Y -= info->BodyOffset;

        p_car->Genus.Vehicle->Draw.Angle = p_car->Genus.Vehicle->Angle;
        p_car->Genus.Vehicle->Draw.Tilt = p_car->Genus.Vehicle->Tilt;
        p_car->Genus.Vehicle->Draw.Roll = p_car->Genus.Vehicle->Roll;

        ANIM_obj_draw(p_car, &p_car->Genus.Vehicle->Draw);

        p_car->WorldPos.Y += info->BodyOffset;
    } else {
        MESH_set_crumple(info->VertexAssignments, p_car->Genus.Vehicle->damage);

        if (MESH_draw_poly(
                info->BodyPrim,
                p_car->WorldPos.X >> 8,
                p_car->WorldPos.Y - get_vehicle_body_offset(p_car->Genus.Vehicle->Type) >> 8,
                p_car->WorldPos.Z >> 8,
                p_car->Genus.Vehicle->Angle,
                p_car->Genus.Vehicle->Tilt,
                p_car->Genus.Vehicle->Roll,
                NULL,
                0xff,
                -1)
            == NULL)
        {
            // body culled — could skip wheels here, but doesn't
        }

        OVAL_add(
            p_car->WorldPos.X >> 8,
            p_car->WorldPos.Y - get_vehicle_body_offset(p_car->Genus.Vehicle->Type) >> 8,
            p_car->WorldPos.Z >> 8,
            float(veh_info[p_car->Genus.Vehicle->Type].shad_size) * 2.0F,
            float(veh_info[p_car->Genus.Vehicle->Type].shad_elongate) * (1.0F / 64.0F),
            float(p_car->Genus.Vehicle->Angle) * (2.0F * PI / 2048.0F),
            OVAL_TYPE_SQUARE);

        FMATRIX_TRANSPOSE(car_matrix);
        if (p_car->Genus.Vehicle->Flags & FLAG_FURN_DRIVING) {
            // headlights — only at night
            if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME)) {
                static SLONG xyz[5][3] = { -255, 0, 0, -254, 12, 16, -251, 24, 32, -247, 36, 48, -243, 48, 60 };

                dz = xyz[p_car->Genus.Vehicle->damage[1]][0];
                dy = xyz[p_car->Genus.Vehicle->damage[1]][1];
                dx = xyz[p_car->Genus.Vehicle->damage[1]][2];
                FMATRIX_MUL(car_matrix, dx, dy, dz);

                vector[2] = info->HLX;
                vector[1] = info->HLY;
                vector[0] = info->HLZ;
                FMATRIX_MUL(car_matrix, vector[0], vector[1], vector[2]);
                BLOOM_draw((p_car->WorldPos.X >> 8) + vector[0], (p_car->WorldPos.Y >> 8) + vector[1], (p_car->WorldPos.Z >> 8) + vector[2], dx, dy, dz, 0x606040, BLOOM_FLENSFLARE | BLOOM_BEAM);

                dz = xyz[p_car->Genus.Vehicle->damage[0]][0];
                dy = -xyz[p_car->Genus.Vehicle->damage[0]][1];
                dx = -xyz[p_car->Genus.Vehicle->damage[0]][2];
                FMATRIX_MUL(car_matrix, dx, dy, dz);

                vector[2] = info->HLX;
                vector[1] = info->HLY;
                vector[0] = -info->HLZ;
                FMATRIX_MUL(car_matrix, vector[0], vector[1], vector[2]);
                BLOOM_draw((p_car->WorldPos.X >> 8) + vector[0], (p_car->WorldPos.Y >> 8) + vector[1], (p_car->WorldPos.Z >> 8) + vector[2], dx, dy, dz, 0x606040, BLOOM_FLENSFLARE | BLOOM_BEAM);
            }
            // flashing emergency lights (police, ambulance, meatwagon)
            if (info->FLZ && (vp->Siren == 1)) {
                SLONG rx, rz;

                rx = SIN((SLONG(p_car) + (GAME_TURN << 7)) & 2047) >> 8;
                rz = COS((SLONG(p_car) + (GAME_TURN << 7)) & 2047) >> 8;

                vector[2] = info->FLX + (rx >> 6);
                vector[1] = info->FLY;
                vector[0] = info->FLZ + (rz >> 6);
                FMATRIX_MUL(car_matrix, vector[0], vector[1], vector[2]);

                SLONG colour = info->FLRED ? 0xDF0000 : 0x0000DF;

                BLOOM_draw((p_car->WorldPos.X >> 8) + vector[0], (p_car->WorldPos.Y >> 8) + vector[1], (p_car->WorldPos.Z >> 8) + vector[2], rx, 0, rz, colour, 0);

                vector[2] = info->FLX + (rx >> 6);
                vector[1] = info->FLY;
                vector[0] = -info->FLZ + (rz >> 6);
                FMATRIX_MUL(car_matrix, vector[0], vector[1], vector[2]);

                BLOOM_draw((p_car->WorldPos.X >> 8) + vector[0], (p_car->WorldPos.Y >> 8) + vector[1], (p_car->WorldPos.Z >> 8) + vector[2], rz, 0, rx, 0xDF0000, 0);
            }
        }

        // engine smoke — probability increases as health drops
        {
            if ((Random() & 0x7f) > p_car->Genus.Vehicle->Health) {
                vector[2] = info->HLX * 0.7f;
                vector[1] = info->HLY;
                vector[0] = 0;
                FMATRIX_MUL(car_matrix, vector[0], vector[1], vector[2]);
                PARTICLE_Add(
                    p_car->WorldPos.X + (vector[0] << 8) + (Random() & 0x3fff) - 0x1fff,
                    p_car->WorldPos.Y + (vector[1] << 8),
                    p_car->WorldPos.Z + (vector[2] << 8) + (Random() & 0x3fff) - 0x1fff,
                    vp->VelX + (Random() & 0xff) - 0x7f,
                    0x3ff + (Random() & 0xff),
                    vp->VelZ + (Random() & 0xff) - 0x7f,
                    POLY_PAGE_SMOKECLOUD, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                    PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE | PFLAG_DAMPING,
                    75, 40 + (rand() & 31), 1, 4, 1);

                if (p_car->Genus.Vehicle->Health <= 0) {
                    // additional smoke from rear when health exhausted
                    PARTICLE_Add(
                        p_car->WorldPos.X - (vector[0] << 8) + (Random() & 0x7fff) - 0x3fff,
                        p_car->WorldPos.Y - (vector[1] << 8),
                        p_car->WorldPos.Z - (vector[2] << 8) + (Random() & 0x7fff) - 0x3fff,
                        vp->VelX + (Random() & 0xff) - 0x7f,
                        0x3ff + (Random() & 0xff),
                        vp->VelZ + (Random() & 0xff) - 0x7f,
                        POLY_PAGE_SMOKECLOUD, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE | PFLAG_DAMPING,
                        75, 40 + (rand() & 31), 1, 4, 1);
                }
            }
        }

        // brakelights — shown day and night
        if (info->BLZ) {
            SLONG colour = 0;

            switch (p_car->Genus.Vehicle->Dir) {
            case -1:
            case 1:
                p_car->Genus.Vehicle->Brakelight = is_driven_by_player(p_car) ? 1 : 10;
                break;

            case -2:
                p_car->Genus.Vehicle->Brakelight = 0;
                if (p_car->Genus.Vehicle->DControl & VEH_DECEL)
                    colour = 0x303030;
                break;

            case 2:
                p_car->Genus.Vehicle->Brakelight = 0;
                break;

            case 0:
                break;
            }

            if (p_car->Genus.Vehicle->Brakelight) {
                p_car->Genus.Vehicle->Brakelight--;
                colour = 0x600000;
            }

            if (colour) {
                dx = 0;
                dy = 0;
                dz = 255;
                FMATRIX_MUL(car_matrix, dx, dy, dz);

                vector[2] = info->BLX;
                vector[1] = info->BLY;
                vector[0] = info->BLZ;
                FMATRIX_MUL(car_matrix, vector[0], vector[1], vector[2]);
                BLOOM_draw((p_car->WorldPos.X >> 8) + vector[0], (p_car->WorldPos.Y >> 8) + vector[1], (p_car->WorldPos.Z >> 8) + vector[2], dx, dy, dz, colour, 0);

                vector[2] = info->BLX;
                vector[1] = info->BLY;
                vector[0] = -info->BLZ;
                FMATRIX_MUL(car_matrix, vector[0], vector[1], vector[2]);
                BLOOM_draw((p_car->WorldPos.X >> 8) + vector[0], (p_car->WorldPos.Y >> 8) + vector[1], (p_car->WorldPos.Z >> 8) + vector[2], dx, dy, dz, colour, 0);
            }
        }

        FMATRIX_TRANSPOSE(car_matrix);
    }

    tilt = p_car->Genus.Vehicle->Spin;

    void AENG_set_bike_wheel_rotation(UWORD rot, UBYTE prim);
    // Despite the name, AENG_set_bike_wheel_rotation is used for all vehicle wheel rotation.
    AENG_set_bike_wheel_rotation(tilt, info->WheelPrim);

    for (c0 = 0; c0 < 4; c0++) {
        SLONG wx, wy, wz;
        SLONG angle;

        wx = info->DX[c0];
        wy = 51 - (((128 << 8) - p_car->Genus.Vehicle->Spring[c0].Compression) >> 8);
        wz = info->DZ[c0];

        apply_car_matrix(&wx, &wy, &wz);

        if (c0 >= 2) {
            // front wheels steered by WheelAngle
            angle = p_car->Genus.Vehicle->Angle - p_car->Genus.Vehicle->WheelAngle;
            angle = (angle + 2048) & 2047;
        } else {
            angle = p_car->Genus.Vehicle->Angle;
        }

        MESH_draw_poly(
            info->WheelPrim,
            (p_car->WorldPos.X >> 8) + wx,
            (p_car->WorldPos.Y >> 8) + wy,
            (p_car->WorldPos.Z >> 8) + wz,
            angle,
            0,
            p_car->Genus.Vehicle->Roll,
            NULL,
            0);
    }

    if (vp->Smokin) {
        vp->Smokin = 0; // must be re-set each tick to stay active

        for (c0 = 0; c0 < 4; c0++) {
            SLONG wx, wy, wz;
            SLONG speed;

            wx = info->DX[c0];
            wy = 51 - (((128 << 8) - vp->Spring[c0].Compression) >> 8);
            wz = info->DZ[c0];

            apply_car_matrix(&wx, &wy, &wz);

            wx = p_car->WorldPos.X + (wx << 8);
            wy = p_car->WorldPos.Y + (wy << 8);
            wz = p_car->WorldPos.Z + (wz << 8);

            PARTICLE_Add(wx, wy, wz,
                (rand() & 7) - 3, 20, (rand() & 7) - 3,
                POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), 0x7Fc9B7A3, PFLAG_FADE | PFLAG_RESIZE | PFLAG_SPRITEANI | PFLAG_SPRITELOOP, 30, 60, 1, 16, 10);
            speed = ((vp->VelX >> CAR_VEL_SHIFT) * (vp->VelX >> CAR_VEL_SHIFT))
                + ((vp->VelZ >> CAR_VEL_SHIFT) * (vp->VelZ >> CAR_VEL_SHIFT));

            if ((speed > 200) && (vp->Skid == SKID_START)) {
                if (speed > 300000)
                    MFX_play_thing(THING_NUMBER(p_car), S_SKID_START, MFX_MOVING, p_car);
                else if (speed > 100000)
                    MFX_play_thing(THING_NUMBER(p_car), S_SKID_START + 1, MFX_MOVING, p_car);
                else
                    MFX_play_thing(THING_NUMBER(p_car), S_SKID_END, MFX_MOVING, p_car);
            }

            if ((speed > 200) && (GAME_TURN & 1)) {
                SLONG dx, dz;
                if (vp->oldX[c0] && vp->oldZ[c0]) {
                    dx = (vp->oldX[c0] - wx) >> 8;
                    dz = (vp->oldZ[c0] - wz) >> 8;
                    TRACKS_Add(wx, (PAP_calc_map_height_at(wx >> 8, wz >> 8) + 5) << 8, wz, dx, 0, dz, TRACK_TYPE_TYRE_SKID, 0);
                }
                vp->oldX[c0] = wx;
                vp->oldZ[c0] = wz;
            }
        }
    } else {
        vp->oldX[0] = vp->oldX[1] = vp->oldX[2] = vp->oldX[3] = 0;
        vp->oldZ[0] = vp->oldZ[1] = vp->oldZ[2] = vp->oldZ[3] = 0;
    }
}

// Unused gravity constant for wheel fall simulation — present in original but not referenced.
// uc_orig: WHEEL_GRAV (fallen/Source/Vehicle.cpp)
#define WHEEL_GRAV (5 << 8)

// Externs for collision detection functions (defined in collide.cpp / ob.cpp).
extern SLONG there_is_a_los_car(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);
extern SLONG should_i_collide_against_this_anim_prim(Thing* p_animprim);

// Fills arctan_table[] using Arctan() for each wheel position.
// Called once from VEH_init_vehinfo(). Idempotent (arctan_table_ok guard).
// uc_orig: init_arctans (fallen/Source/Vehicle.cpp)
static void init_arctans(void)
{
    int ii;

    if (arctan_table_ok)
        return;

    for (ii = 0; ii <= WHEELTIME; ii++) {
        arctan_table[WHEELTIME - ii] = -Arctan(ii, -WHEELRATIO) & 2047;
        arctan_table[WHEELTIME + ii] = +Arctan(ii, -WHEELRATIO) & 2047;
    }

    for (ii = 0; ii <= 2 * WHEELTIME + 1; ii++) {
        if (arctan_table[ii] > 1024)
            arctan_table[ii] -= 2048;
    }

    arctan_table_ok = 1;
}

// Returns 4 world-space corner positions of the vehicle bounding box.
// step: 0 = current position, TICK_RATIO = predicted next position (for collision lookahead).
// Corners: [FL, FR, RR, RL] matching prim bounds (minx/maxx, minz/maxz).
// uc_orig: GetCarPoints (fallen/Source/Vehicle.cpp)
static inline void GetCarPoints(Thing* p_car, SLONG* x, SLONG* y, SLONG* z, SLONG step)
{
    Vehicle* veh;
    PrimInfo* pinfo;
    int ii;

    veh = p_car->Genus.Vehicle;
    pinfo = get_prim_info(veh_info[veh->Type].BodyPrim);

    x[0] = pinfo->minx;
    x[1] = pinfo->maxx;
    x[2] = pinfo->maxx;
    x[3] = pinfo->minx;

    y[0] = pinfo->miny;
    y[1] = pinfo->miny;
    y[2] = pinfo->miny;
    y[3] = pinfo->miny;

    z[0] = pinfo->minz;
    z[1] = pinfo->minz;
    z[2] = pinfo->maxz;
    z[3] = pinfo->maxz;

    make_car_matrix_p((veh->Angle + ((veh->VelR * step) >> TICK_SHIFT)) & 2047, 0, 0);

    for (ii = 0; ii < 4; ii++) {
        apply_car_matrix(&x[ii], &y[ii], &z[ii]);

        x[ii] += (p_car->WorldPos.X + ((veh->VelX * step) >> TICK_SHIFT)) >> 8;
        y[ii] += (p_car->WorldPos.Y + ((veh->VelY * step) >> TICK_SHIFT)) >> 8;
        z[ii] += (p_car->WorldPos.Z + ((veh->VelZ * step) >> TICK_SHIFT)) >> 8;
    }
}

// =============================================================================
// CHUNK 2: Collision system and main vehicle tick (VEH_driving)
// =============================================================================

// Fills VEH_col[] with all Things and OB prims the vehicle may collide with
// this frame. Used by CollideCar to run the actual collision resolution.
// Search sphere is (x,y,z) with given radius (in >>8 world units).
// ignore: THING_INDEX of the querying vehicle (to skip self).
// ignore_prims: skip OB prim scan (used when fully on road, for performance).
// uc_orig: VEH_collide_find_things (fallen/Source/Vehicle.cpp)
void VEH_collide_find_things(SLONG x, SLONG y, SLONG z, SLONG radius, SLONG ignore, SLONG ignore_prims)
{
    static UWORD found[VEH_MAX_COL];

    SLONG i;
    SLONG num;
    SLONG prim;
    SLONG dx;
    SLONG dz;
    SLONG dist;

    Thing* p_found;
    VEH_Col* vc;
    PrimInfo* pi;
    AnimPrimBbox* apb;
    OB_Info* oi;

    // Bike system is unfinished — only check vehicles, anim prims, and bats (Balrog).
    ULONG collide_types = (1 << CLASS_VEHICLE) | (1 << CLASS_ANIM_PRIM) | (1 << CLASS_BAT);

    num = THING_find_sphere(
        x, y, z,
        radius + 0x200,
        found,
        VEH_MAX_COL,
        collide_types);

    VEH_col_upto = 0;

    for (i = 0; i < num; i++) {
        if (found[i] == ignore)
            continue;

        ASSERT(WITHIN(VEH_col_upto, 0, VEH_MAX_COL - 1));

        p_found = TO_THING(found[i]);

        switch (p_found->Class) {
        case CLASS_VEHICLE:

            prim = get_vehicle_body_prim(p_found->Genus.Vehicle->Type);
            pi = get_prim_info(prim);

            dx = abs((p_found->WorldPos.X >> 8) - x);
            dz = abs((p_found->WorldPos.Z >> 8) - z);

            dist = QDIST2(dx, dz);

            if (dist <= radius + pi->radius + 0x10) {
                vc = &VEH_col[VEH_col_upto++];

                vc->type = VEH_COL_TYPE_BBOX;
                vc->ob_index = NULL;
                vc->veh = p_found;
                vc->mid_x = p_found->WorldPos.X >> 8;
                vc->mid_y = p_found->WorldPos.Y >> 8;
                vc->mid_z = p_found->WorldPos.Z >> 8;
                vc->height = pi->maxy;
                vc->min_x = pi->minx;
                vc->min_z = pi->minz;
                vc->max_x = pi->maxx;
                vc->max_z = pi->maxz;

                vc->radius_or_yaw = p_found->Genus.Vehicle->Angle;
            }

            break;

        case CLASS_ANIM_PRIM:
            break;

        case CLASS_BAT:

            if (p_found->Genus.Bat->type == BAT_TYPE_BALROG) {
                vc = &VEH_col[VEH_col_upto++];

                vc->type = VEH_COL_TYPE_CYLINDER;
                vc->ob_index = NULL;
                vc->veh = p_found;
                vc->mid_x = p_found->WorldPos.X >> 8;
                vc->mid_y = p_found->WorldPos.Y >> 8;
                vc->mid_z = p_found->WorldPos.Z >> 8;
                vc->height = 0x100;

                vc->radius_or_yaw = 0x40;
            }

            break;

        default:
            ASSERT(0);
            break;
        }
    }

    if (!ignore_prims) {
        SLONG mx1 = x - radius >> PAP_SHIFT_LO;
        SLONG mz1 = z - radius >> PAP_SHIFT_LO;
        SLONG mx2 = x + radius >> PAP_SHIFT_LO;
        SLONG mz2 = z + radius >> PAP_SHIFT_LO;

        SATURATE(mx1, 0, PAP_SIZE_LO - 1);
        SATURATE(mz1, 0, PAP_SIZE_LO - 1);
        SATURATE(mx2, 0, PAP_SIZE_LO - 1);
        SATURATE(mz2, 0, PAP_SIZE_LO - 1);

        for (SLONG mx = mx1; mx <= mx2; mx++) {
            for (SLONG mz = mz1; mz <= mz2; mz++) {
                for (oi = OB_find(mx, mz); oi->prim; oi++) {
                    if (VEH_col_upto >= VEH_MAX_COL)
                        return;

                    if (oi->y >= y + 0x180)
                        continue;

                    switch (prim_get_collision_model(oi->prim)) {
                    case PRIM_COLLIDE_BOX:
                    case PRIM_COLLIDE_SMALLBOX:

                        pi = get_prim_info(oi->prim);

                        dx = abs(oi->x - x);
                        dz = abs(oi->z - z);

                        dist = QDIST2(dx, dz);

                        if (dist <= radius + pi->radius + 0x10) {
                            vc = &VEH_col[VEH_col_upto++];

                            vc->type = VEH_COL_TYPE_BBOX;
                            vc->ob_index = oi->index;
                            vc->veh = NULL;
                            vc->mid_x = oi->x;
                            vc->mid_y = oi->y;
                            vc->mid_z = oi->z;
                            vc->height = pi->maxy;
                            vc->min_x = pi->minx;
                            vc->min_z = pi->minz;
                            vc->max_x = pi->maxx;
                            vc->max_z = pi->maxz;

                            vc->radius_or_yaw = oi->yaw;
                        }

                        break;

                    case PRIM_COLLIDE_NONE:
                        break;

                    case PRIM_COLLIDE_CYLINDER:

                        pi = get_prim_info(oi->prim);

                        dx = abs(oi->x - x);
                        dz = abs(oi->z - z);

                        dist = QDIST2(dx, dz);

                        if (dist <= radius + 0x40) {
                            vc = &VEH_col[VEH_col_upto++];

                            vc->type = VEH_COL_TYPE_CYLINDER;
                            vc->ob_index = oi->index;
                            vc->veh = NULL;
                            vc->mid_x = oi->x;
                            vc->mid_y = oi->y;
                            vc->mid_z = oi->z;
                            vc->height = pi->maxy;

                            vc->radius_or_yaw = 0x30;
                        }

                        break;

                    default:
                        ASSERT(0);
                        break;
                    }
                }
            }
        }
    }
}

// Animates fence facets on the edge of the given map square (hi-res coords).
// Called when a vehicle's edge hits a wall or fence during collision.
// uc_orig: VEH_shake_fences (fallen/Source/Vehicle.cpp)
void VEH_shake_fences(SLONG mx, SLONG mz)
{
    SLONG exit;
    SLONG facet;
    SLONG f_list;

    DFacet* df;

    f_list = PAP_2LO(mx >> 2, mz >> 2).ColVectHead;

    if (f_list) {
        exit = UC_FALSE;

        while (1) {
            ASSERT(WITHIN(f_list, 1, next_facet_link - 1));

            facet = facet_links[f_list];

            if (facet < 0) {
                facet = -facet;
                exit = UC_TRUE;
            }

            ASSERT(WITHIN(facet, 1, next_dfacet - 1));

            df = &dfacets[facet];

            if (df->FacetType == STOREY_TYPE_FENCE || df->FacetType == STOREY_TYPE_FENCE_FLAT || df->FacetType == STOREY_TYPE_FENCE_BRICK) {
                if (df->x[0] == df->x[1]) {
                    if (df->x[0] == mx || df->x[0] == mx + 1) {
                        df->Shake = 255;
                    }
                } else {
                    ASSERT(df->z[0] == df->z[1]);

                    if (df->z[0] == mz || df->z[0] == mz + 1) {
                        df->Shake = 255;
                    }
                }
            }

            if (exit) {
                return;
            }

            f_list += 1;
        }
    }
}

// Returns the index (0-5) of the crumple zone on 'car' closest to (x,y,z).
// Samples 6 points along the car bbox (left/right at front/mid/rear).
// uc_orig: find_closest_car_point (fallen/Source/Vehicle.cpp)
static UBYTE find_closest_car_point(SLONG x, SLONG y, SLONG z, Thing* car)
{
    Vehicle* v = car->Genus.Vehicle;

    make_car_matrix(v);

    PrimInfo* inf;
    SLONG xx[6], yy[6], zz[6];

    inf = get_prim_info(veh_info[car->Genus.Vehicle->Type].BodyPrim);

    xx[0] = inf->minx;
    xx[1] = inf->maxx;
    xx[2] = inf->minx;
    xx[3] = inf->maxx;
    xx[4] = inf->minx;
    xx[5] = inf->maxx;

    yy[0] = inf->miny;
    yy[1] = inf->miny;
    yy[2] = inf->miny;
    yy[3] = inf->miny;
    yy[4] = inf->miny;
    yy[5] = inf->miny;

    zz[0] = inf->minz;
    zz[1] = inf->minz;
    zz[2] = (inf->minz + inf->maxz) / 2;
    zz[3] = (inf->minz + inf->maxz) / 2;
    zz[4] = inf->maxz;
    zz[5] = inf->maxz;

    SLONG best_manhattan_distance = 0x7FFFFFFF;
    SLONG nearest = -1;

    for (int ii = 0; ii < 6; ii++) {
        apply_car_matrix(&xx[ii], &yy[ii], &zz[ii]);
        SLONG manhattan = abs(xx[ii] + (car->WorldPos.X >> 8) - x) + abs(zz[ii] + (car->WorldPos.Z >> 8) - z);
        if (manhattan < best_manhattan_distance) {
            best_manhattan_distance = manhattan;
            nearest = ii;
        }
    }

    ASSERT(nearest != -1);

    return nearest;
}

// Handles vehicle-vehicle collision damage.
// Only deals crumple damage if at least one car is player-driven (NPC-NPC collisions ignored).
// Damage is scaled by relative velocities; the slower vehicle takes more crumple damage.
// Also imparts velocity and angular impulse to the hit vehicle and forces it into skid.
// uc_orig: VEH_co_damage (fallen/Source/Vehicle.cpp)
void VEH_co_damage(Thing* v1, Thing* v2)
{
    if (!is_driven_by_player(v1) && !is_driven_by_player(v2))
        return;

    UBYTE c1 = find_closest_car_point(v2->WorldPos.X >> 8, v2->WorldPos.Y >> 8, v2->WorldPos.Z >> 8, v1);
    UBYTE c2 = find_closest_car_point(v1->WorldPos.X >> 8, v1->WorldPos.Y >> 8, v1->WorldPos.Z >> 8, v2);
    SLONG damage;

    MFX_play_thing(THING_NUMBER(v1), SOUND_Range(S_CAR_SMASH_START, S_CAR_SMASH_END), 0, v1);
    // Give most damage to the slower vehicle (the faster one is already penalised by physics).
    if (abs(v1->Velocity) > abs(v2->Velocity)) {
        VEH_add_damage(v1->Genus.Vehicle, c1, (abs(v1->Velocity) + abs(v2->Velocity)) / 1024);
        VEH_add_damage(v2->Genus.Vehicle, c2, abs(v1->Velocity) / 512);
        VEH_bounce(v1->Genus.Vehicle, c1, abs(v1->Velocity));
        VEH_bounce(v2->Genus.Vehicle, c2, abs(v1->Velocity));
    } else {
        VEH_add_damage(v1->Genus.Vehicle, c1, abs(v2->Velocity) / 512);
        VEH_add_damage(v2->Genus.Vehicle, c2, (abs(v1->Velocity) + abs(v2->Velocity)) / 1024);
        VEH_bounce(v1->Genus.Vehicle, c1, abs(v2->Velocity));
        VEH_bounce(v2->Genus.Vehicle, c2, abs(v2->Velocity));
    }

    // Set second vehicle moving from impact impulse.
    Vehicle* vv1 = v1->Genus.Vehicle;
    Vehicle* vv2 = v2->Genus.Vehicle;

    vv2->VelX = -vv2->VelX / 8 + vv1->VelX / 4;
    vv2->VelZ = -vv2->VelZ / 8 + vv1->VelZ / 4;

    SLONG torque = abs(vv1->VelX) + abs(vv1->VelZ);
    torque >>= 10;

    switch (c2) {
    case 0:
    case 5:
        vv2->VelR -= torque;
        break;

    case 1:
    case 4:
        vv2->VelR += torque;
        break;
    }

    vv2->Skid = SKID_START;
    vv2->Stable = 0;
    v2->StateFn = VEH_driving;
}

// Applies damage and plays a sound after a collision with a VEH_Col entry.
// For vehicle targets: damages health and calls VEH_co_damage for crumple zones.
// For OB prim targets: calls OB_damage if speed > 20000; sound threshold varies by speed.
// uc_orig: DoDamage (fallen/Source/Vehicle.cpp)
static void DoDamage(Thing* p_car, VEH_Col* col)
{
    if (col->veh && col->veh->Class == CLASS_VEHICLE) {
        {
            SLONG speed = p_car->Velocity >> 5;

            speed -= 16;

            if (speed > 0) {
                col->veh->Genus.Vehicle->Health -= speed >> 1;
            }
        }

        VEH_co_damage(p_car, col->veh);
    }
    if (col->ob_index) {
        Vehicle* veh = p_car->Genus.Vehicle;
        SLONG vel = Root(veh->VelX * veh->VelX + veh->VelZ * veh->VelZ);

        if (vel > 20000) {
            OB_damage(col->ob_index,
                p_car->WorldPos.X >> 8, p_car->WorldPos.Y >> 8,
                col->mid_x, col->mid_z,
                get_vehicle_driver(p_car));
            MFX_play_thing(THING_NUMBER(p_car), SOUND_Range(S_CAR_SMASH_START, S_CAR_SMASH_END), 0, p_car);
        } else {
            if (vel > 10000)
                MFX_play_thing(THING_NUMBER(p_car), SOUND_Range(S_CAR_SMASH_START, S_CAR_SMASH_END), 0, p_car);
            else if (vel > 1000)
                MFX_play_thing(THING_NUMBER(p_car), SOUND_Range(S_CAR_SMASH_START, S_CAR_SMASH_START + 1), 0, p_car);
            else if (vel > 250)
                MFX_play_thing(THING_NUMBER(p_car), S_CAR_BUMP, 0, p_car);
        }
    }
}

// Resolves wall collisions by nudging the car along its edge perpendicular
// to the colliding corner(s). flags bits 0-3 = which edge is in contact;
// bits 4-5 = X/Z wall axis (for velocity component selection).
// neg: if set, nudge in the opposite direction (post-bounce correction).
// uc_orig: nudge_car (fallen/Source/Vehicle.cpp)
void nudge_car(Thing* p_car, SLONG flags, SLONG* x, SLONG* z, SLONG neg)
{
    SLONG dx = 0, dz = 0;
    switch (flags & 15) {
    case 1 + 2:
        dx = x[3] - x[1];
        dz = z[3] - z[1];
        break;
    case 2 + 4:
        dx = x[0] - x[2];
        dz = z[0] - z[2];
        break;
    case 4 + 8:
        dx = x[1] - x[3];
        dz = z[1] - z[3];
        break;
    case 8 + 1:
        dx = x[2] - x[0];
        dz = z[2] - z[0];
        break;

    case 1:
    case 1 + 8 + 2:
        // front
        dx = x[2] - x[1];
        dz = z[2] - z[1];
        break;

    case 2:
    case 2 + 1 + 4:
        // rhs
        dx = x[0] - x[1];
        dz = z[0] - z[1];
        break;

    case 4:
    case 4 + 2 + 8:
        // back
        dx = x[1] - x[2];
        dz = z[1] - z[2];
        break;

    case 8:
    case 8 + 4 + 1:
        // lhs
        dx = x[1] - x[0];
        dz = z[1] - z[0];
        break;
    }

    if (neg) {
        dx = -dx;
        dz = -dz;
    }

    if (dx || dz) {
        GameCoord new_pos;

        new_pos.X = p_car->WorldPos.X + dx;
        new_pos.Y = p_car->WorldPos.Y;
        new_pos.Z = p_car->WorldPos.Z + dz;

        move_thing_on_map(p_car, &new_pos);
    }
}

// Collision result codes — combined corner/edge contact pattern → response direction.
// uc_orig: COLL_NONE (fallen/Source/Vehicle.cpp)
#define COLL_NONE  0x00
// uc_orig: COLL_FL (fallen/Source/Vehicle.cpp)
#define COLL_FL    0x01
// uc_orig: COLL_FR (fallen/Source/Vehicle.cpp)
#define COLL_FR    0x02
// uc_orig: COLL_BR (fallen/Source/Vehicle.cpp)
#define COLL_BR    0x03
// uc_orig: COLL_BL (fallen/Source/Vehicle.cpp)
#define COLL_BL    0x04
// uc_orig: COLL_FRONT (fallen/Source/Vehicle.cpp)
#define COLL_FRONT 0x05
// uc_orig: COLL_BACK (fallen/Source/Vehicle.cpp)
#define COLL_BACK  0x06
// uc_orig: COLL_LEFT (fallen/Source/Vehicle.cpp)
#define COLL_LEFT  0x07
// uc_orig: COLL_RIGHT (fallen/Source/Vehicle.cpp)
#define COLL_RIGHT 0x08
// uc_orig: COLL_ALL (fallen/Source/Vehicle.cpp)
#define COLL_ALL   0x09

extern UBYTE last_mav_square_x;
extern UBYTE last_mav_square_z;
extern SBYTE last_mav_dx;
extern SBYTE last_mav_dz;

// Main collision detection and response for one vehicle tick.
// Steps: run kerb correction, suspension dry-run, generate corner points,
// test edges vs walls, test corners vs VEH_col[], apply velocity response.
// Returns 1 if collision occurred, 0 if clear.
// uc_orig: CollideCar (fallen/Source/Vehicle.cpp)
static SLONG CollideCar(Thing* p_car, SLONG step)
{
    Vehicle* veh = p_car->Genus.Vehicle;
    VehInfo* vinfo = &veh_info[veh->Type];

    SLONG x[4], y[4], z[4];
    int ii;

    car_hit_flags = 0;

    CollideWithKerb(p_car);

    // Run suspension now to get VelY for this frame, then restore Y.
    SLONG old_y = p_car->WorldPos.Y;
    process_car(p_car);
    veh->VelY = p_car->WorldPos.Y - old_y;
    p_car->WorldPos.Y = old_y;

    GetCarPoints(p_car, x, y, z, step);

    UBYTE flags = 0, pflags = 0;
    static UBYTE flags_to_code[16] = {
        COLL_NONE, COLL_FRONT, COLL_NONE, COLL_FR,
        COLL_BACK, COLL_LEFT, COLL_BR, COLL_RIGHT,
        COLL_NONE, COLL_FL, COLL_FRONT, COLL_FRONT,
        COLL_BL, COLL_LEFT, COLL_BACK, COLL_ALL
    };

    // Test each edge against walls.
    if (!VEH_collide_line_ignore_walls) {
        for (ii = 0; ii < 4; ii++) {
            int jj = (ii + 1) & 3;
            int aa;

            if (aa = there_is_a_los_car(x[ii], y[ii], z[ii], x[jj], y[jj], z[jj])) {
                flags |= 1 << ii;

                if (aa == 1)
                    flags |= 16;
                if (aa == 2)
                    flags |= 32;

                if (veh->Scrapin < 10)
                    veh->Scrapin++;

                VEH_shake_fences(last_mav_square_x, last_mav_square_z);
            }
        }
        nudge_car(p_car, flags, x, z, 0);

        if (((flags & 15) == 8) || ((flags & 15) == 2) || ((flags & 15) == 10)) {
            flags |= 1;
        }
    }
    if (veh->Scrapin > 5) {
        SLONG vel = (veh->VelX >> CAR_VEL_SHIFT) * (veh->VelX >> CAR_VEL_SHIFT) + (veh->VelZ >> CAR_VEL_SHIFT) * (veh->VelZ >> CAR_VEL_SHIFT);
        if (vel > 300)
            MFX_play_thing(THING_NUMBER(p_car), SOUND_Range(S_CAR_SCRAPE_START, S_CAR_SCRAPE_END), MFX_MOVING, p_car);
        if (veh->Scrapin > 0)
            veh->Scrapin -= 2;

        if (flags & 15) {
            SLONG px = 0;
            SLONG pz = 0;
            SLONG div = 0;

            for (ii = 0; ii < 4; ii++) {
                int jj = (ii + 1) & 3;

                if (flags & (1 << ii)) {
                    px += x[ii] + x[jj];
                    pz += z[ii] + z[jj];
                    div += 2;
                }
            }

            px /= div;
            pz /= div;

            DIRT_new_sparks(px, y[0], pz, 2);
        }
    }

    // Test corners against all VEH_col[] entries (vehicles and OB prims).
    for (ii = 0; ii < VEH_col_upto; ii++) {
        VEH_Col* vc = &VEH_col[ii];
        int inside = 1;

        if (vc->type == VEH_COL_TYPE_BBOX) {
            int jj;

            for (jj = 0; jj < 4; jj++) {
                int kk = (jj + 1) & 3;

                if (collide_box_with_line(vc->mid_x,
                        vc->mid_z,
                        vc->min_x,
                        vc->min_z,
                        vc->max_x,
                        vc->max_z,
                        vc->radius_or_yaw,
                        x[jj], z[jj],
                        x[kk], z[kk])) {
                    pflags |= 1 << jj;

                    pflags |= 48;

                    DoDamage(p_car, vc);
                }

                SLONG x1 = x[kk] - x[jj];
                SLONG z1 = z[kk] - z[jj];
                SLONG x2 = vc->mid_x - x[jj];
                SLONG z2 = vc->mid_z - z[jj];

                if (x2 * z1 > x1 * z2)
                    inside = 0;
            }
        } else {
            int jj;

            ASSERT(vc->type == VEH_COL_TYPE_CYLINDER);

            for (jj = 0; jj < 4; jj++) {
                int kk = (jj + 1) & 3;

                if (distance_to_line(x[jj], z[jj],
                        x[kk], z[kk],
                        vc->mid_x,
                        vc->mid_z)
                    < vc->radius_or_yaw) {
                    pflags |= 1 << jj;

                    pflags |= 48;

                    DoDamage(p_car, vc);

                    if (vc->veh && vc->veh->Class == CLASS_BAT) {
                        // Car hit a Balrog — scare the driver away and make Balrog hostile.
                        if (veh->Driver) {
                            PCOM_make_driver_run_away(TO_THING(veh->Driver), vc->veh);

                            vc->veh->Genus.Bat->target = veh->Driver;
                            vc->veh->Genus.Bat->timer = 0;
                        }
                    }
                }

                SLONG x1 = x[kk] - x[jj];
                SLONG z1 = z[kk] - z[jj];
                SLONG x2 = vc->mid_x - x[jj];
                SLONG z2 = vc->mid_z - z[jj];

                if (x2 * z1 > x1 * z2)
                    inside = 0;
            }
        }

        if ((((flags | pflags) & 15) == 8) || (((flags | pflags) & 15) == 2) || (((flags | pflags) & 15) == 10)) {
            SLONG dsf = ((x[0] + x[1]) / 2 - vc->mid_x) * ((x[0] + x[1]) / 2 - vc->mid_x) + ((z[0] + z[1]) / 2 - vc->mid_z) * ((z[0] + z[1]) / 2 - vc->mid_z);
            SLONG dsb = ((x[2] + x[3]) / 2 - vc->mid_x) * ((x[2] + x[3]) / 2 - vc->mid_x) + ((z[2] + z[3]) / 2 - vc->mid_z) * ((z[2] + z[3]) / 2 - vc->mid_z);
            if (dsf < dsb)
                pflags |= 1;
            else
                pflags |= 4;
        }

        // Prim is inside car — bounce off.
        if (!(flags | pflags) && inside) {
            pflags = 49;
            DoDamage(p_car, vc);
        }
    }

    if (pflags) {
        if (flags)
            nudge_car(p_car, flags, x, z, 1);
    }
    flags |= pflags;

    if (!flags)
        return 0;

    // Damage car health based on speed at impact.
    {
        SLONG speed = p_car->Velocity >> 5;

        speed -= 16;

        if (speed > 0) {
            veh->Health -= speed >> 1;
        }
    }

    UBYTE code = flags_to_code[flags & 15];
    SLONG torque;

    if (!is_driven_by_player(p_car))
        veh->Wheel = 0;

    torque = 0;
    if (flags & 16)
        torque += abs(veh->VelX);
    if (flags & 32)
        torque += abs(veh->VelZ);
    torque >>= 9;

    switch (code) {
    case COLL_FRONT:
    case COLL_BACK:
        p_car->Flags |= FLAGS_COLLIDED;
        // Fallthrough!

    case COLL_LEFT:
    case COLL_RIGHT:
        if (flags & 16)
            veh->VelX = -veh->VelX / 4;
        if (flags & 32)
            veh->VelZ = -veh->VelZ / 4;
        veh->Skid = SKID_START;
        break;

    case COLL_FL:
    case COLL_BR:
        p_car->Flags |= FLAGS_COLLIDED;
        veh->VelR -= torque;
        if (veh->VelR > -10)
            veh->VelR = -10;
        if (flags & 16)
            veh->VelX = -veh->VelX / 8;
        if (flags & 32)
            veh->VelZ = -veh->VelZ / 8;
        break;

    case COLL_FR:
    case COLL_BL:
        p_car->Flags |= FLAGS_COLLIDED;
        veh->VelR += torque;
        if (veh->VelR < +10)
            veh->VelR = +10;
        if (flags & 16)
            veh->VelX = -veh->VelX / 8;
        if (flags & 32)
            veh->VelZ = -veh->VelZ / 8;
        break;

    case COLL_ALL:
        veh->VelX = 0;
        veh->VelZ = 0;
        veh->VelR = 0;
        p_car->Velocity = 0;
        veh->Skid = SKID_START;
        break;
    }

    // Reduce velocity to prevent bounce oscillation.
    p_car->Velocity >>= 1;

    // Regenerate corner points after position adjustment.
    GetCarPoints(p_car, x, y, z, step);

    // Re-test walls — if still colliding after bounce, halt completely.
    if (!VEH_collide_line_ignore_walls) {
        for (ii = 0; ii < 4; ii++) {
            int jj = (ii + 1) & 3;

            if (there_is_a_los_car(x[ii], y[ii], z[ii], x[jj], y[jj], z[jj])) {
                veh->VelX = 0;
                veh->VelY = 0;
                veh->VelZ = 0;
                veh->VelR = 0;
                p_car->Velocity = 0;
                veh->Skid = SKID_START;

                return 1;
            }
        }
    }

    // Re-test prim collisions — slide or halt if still penetrating.
    for (ii = 0; ii < VEH_col_upto; ii++) {
        VEH_Col* vc = &VEH_col[ii];

        if (vc->type == VEH_COL_TYPE_BBOX) {
            int jj;
            bool slide;

            slide = false;

            do {
                for (jj = 0; jj < 4; jj++) {
                    int kk = (jj + 1) & 3;

                    if (collide_box_with_line(vc->mid_x,
                            vc->mid_z,
                            vc->min_x + 0x10,
                            vc->min_z + 0x10,
                            vc->max_x - 0x10,
                            vc->max_z - 0x10,
                            vc->radius_or_yaw,
                            x[jj], z[jj],
                            x[kk], z[kk])) {
                        if (!slide) {
                            veh->VelX = ((p_car->WorldPos.X >> 8) - vc->mid_x) >> 4;
                            veh->VelZ = ((p_car->WorldPos.Z >> 8) - vc->mid_z) >> 4;
                            veh->VelR = 0;
                            slide = true;
                            veh->Skid = SKID_START;
                            break;
                        } else {
                            veh->VelX = 0;
                            veh->VelZ = 0;
                            veh->VelR = 0;
                            p_car->Velocity = 0;
                            return 1;
                        }
                    }
                }
                if (jj == 4)
                    break;

                if (slide) {
                    GetCarPoints(p_car, x, y, z, step);
                }
            } while (slide);
        } else {
            int jj;
            bool slide;

            ASSERT(vc->type == VEH_COL_TYPE_CYLINDER);

            slide = false;

            do {
                for (jj = 0; jj < 4; jj++) {
                    int kk = (jj + 1) & 3;

                    if (distance_to_line(x[jj], z[jj],
                            x[kk], z[kk],
                            vc->mid_x,
                            vc->mid_z)
                        < vc->radius_or_yaw - 0x10) {
                        if (!slide) {
                            SLONG vel = Root((veh->VelX >> 4) * (veh->VelX >> 4) + (veh->VelZ >> 4) * (veh->VelZ >> 4));
                            veh->VelR = 0;
                            veh->VelX = (vel * SIN(veh->Angle & 2047)) >> 12;
                            veh->VelZ = (vel * COS(veh->Angle & 2047)) >> 12;
                            slide = true;
                            break;
                        } else {
                            veh->VelX = 0;
                            veh->VelZ = 0;
                            veh->VelR = 0;
                            p_car->Velocity = 0;
                            return 1;
                        }
                    }
                }
                if (jj == 4)
                    break;

                if (slide) {
                    GetCarPoints(p_car, x, y, z, step);
                }
            } while (slide);
        }
    }

    return 1;
}

// Nudges the car back toward the road when a wheel transitions kerb/road boundary
// while no steering input is active (DControl==0). Applies a small angular correction.
// uc_orig: CollideWithKerb (fallen/Source/Vehicle.cpp)
static void CollideWithKerb(Thing* p_car)
{
    Vehicle* veh = p_car->Genus.Vehicle;
    VehInfo* vinfo = &veh_info[veh->Type];

    make_car_matrix_p((veh->Angle + ((veh->VelR * TICK_RATIO) >> TICK_SHIFT)) & 2047, 0, 0);

    UBYTE on_road = 0;

    for (SLONG wheel = 0; wheel < 4; wheel++) {
        SLONG wx = vinfo->DX[wheel];
        SLONG wy = 0;
        SLONG wz = vinfo->DZ[wheel];

        apply_car_matrix(&wx, &wy, &wz);

        SLONG papx = wx + (p_car->WorldPos.X >> 8);
        SLONG papz = wz + (p_car->WorldPos.Z >> 8);

        if (ROAD_is_road(papx >> 8, papz >> 8))
            on_road |= (1 << wheel);
    }

    UBYTE change = (on_road ^ veh->OnRoadFlags);

    if (change && !veh->DControl) {
#define KERB_TURN 16
        // Snap toward nearest cardinal axis (top 3 bits of angle).
        static SLONG towards_table[8] = { 0x000, 0x200, 0x200, 0x400, 0x400, 0x600, 0x600, 0x800 };
        SLONG towards = towards_table[veh->Angle >> 8];

        if ((towards - veh->Angle) < -(KERB_TURN * TICK_RATIO) >> TICK_SHIFT) {
            veh->VelR -= KERB_TURN;
        } else if ((towards - veh->Angle) > (KERB_TURN * TICK_RATIO) >> TICK_SHIFT) {
            veh->VelR += KERB_TURN;
        }
    }
    veh->OnRoadFlags = on_road;
}

// Run-over HP calculation: dot product of car velocity with direction from
// car centre to person. Larger component means vehicle moving more directly
// into the person, dealing more damage. Minimum 10 HP.
// uc_orig: GetRunoverHP (fallen/Source/Vehicle.cpp)
SLONG GetRunoverHP(Thing* p_car, Thing* p_person)
{
    SLONG tx = (p_person->WorldPos.X - p_car->WorldPos.X) >> 8;
    SLONG tz = (p_person->WorldPos.Z - p_car->WorldPos.Z) >> 8;

    SLONG tt = Root(tx * tx + tz * tz) * 200;

    SLONG hp = abs(p_car->Genus.Vehicle->VelX * tx + p_car->Genus.Vehicle->VelZ * tz) / tt;

    if (hp < 10)
        hp = 10;

    return hp;
}

// Ejects a person from a vehicle and knocks them down (called on crash or explosion).
// uc_orig: VEH_throw_out_person (fallen/Source/Vehicle.cpp)
void VEH_throw_out_person(Thing* p_person, Thing* p_vehicle)
{
    set_person_exit_vehicle(p_person);

    knock_person_down(
        p_person,
        30,
        p_vehicle->WorldPos.X >> 8,
        p_vehicle->WorldPos.Z >> 8,
        NULL);
}

// Main per-tick vehicle state function. Called for each active CLASS_VEHICLE Thing.
// Processes: health/explosion, stall override, idle detection, input, road optimization,
// collision find+resolve, position update, runover detection, environment effects.
// If stable and stopped: sets StateFn=NULL to stop ticking.
// uc_orig: VEH_driving (fallen/Source/Vehicle.cpp)
void VEH_driving(Thing* p_thing)
{
    DrawMesh* dm = p_thing->Draw.Mesh;
    Vehicle* veh = p_thing->Genus.Vehicle;
    SLONG dx, dy, dz;
    SLONG coltype;
    SLONG dwheel;

    dy = 0;

    ASSERT(p_thing->Class == CLASS_VEHICLE);

    // Forget the "been shot at" flag periodically.
    if ((GAME_TURN & 0x1f) == 0) {
        veh->Flags &= ~FLAG_VEH_SHOT_AT;
    }

    if (veh->Health <= 0) {
        veh->Steering = 0;
        veh->DControl = 0;

        if (p_thing->State != STATE_DEAD) {
            // Explode: spawn fireball, play sound, throw out occupants.
            {
                Thing* pyro;
                SLONG wave;
                pyro = PYRO_create(p_thing->WorldPos, PYRO_FIREBOMB);
                if (pyro)
                    pyro->Genus.Pyro->Flags |= PYRO_FLAGS_WAVE;
                wave = S_EXPLODE_MEDIUM;
                if (!(Random() & 3))
                    wave++;
                MFX_play_xyz(THING_NUMBER(p_thing), wave, 0, p_thing->WorldPos.X, p_thing->WorldPos.Y, p_thing->WorldPos.Z);
            }
            VEH_bounce(veh, 0, 4000);

            veh->damage[0] = 4;
            veh->damage[1] = 4;
            veh->damage[2] = 4;
            veh->damage[3] = 4;
            veh->damage[4] = 4;
            veh->damage[5] = 4;

            p_thing->State = STATE_DEAD;
            p_thing->Genus.Person->Action = ACTION_DEAD;
            veh->Health = 0;

            if (veh->Driver) {
                VEH_throw_out_person(TO_THING(veh->Driver), p_thing);
            }

            while (veh->Passenger) {
                VEH_throw_out_person(TO_THING(veh->Passenger), p_thing);
            }
        } else {
            // Health doubles as a countdown timer after death.
            if (!(p_thing->Flags & FLAGS_IN_VIEW)) {
                veh->Health -= 1;

                if (veh->Health < -200) {
                    remove_thing_from_map(p_thing);

                    p_thing->StateFn = NULL;

                    return;
                }
            }
        }
    }

    if (veh->Flags & FLAG_VEH_STALLED) {
        // Stalled: can only decelerate.
        veh->Steering = 0;

        if (p_thing->Velocity > 0) {
            veh->DControl = VEH_DECEL;
        } else {
            veh->DControl = 0;
        }
    }

    if (!(veh->Flags & FLAG_FURN_DRIVING) && !p_thing->Genus.Vehicle->VelX && !p_thing->Genus.Vehicle->VelZ && (veh->Stable >= STABLE_COUNT)) {
        if (p_thing->State != STATE_DEAD) {
            siren(veh, 0);
            p_thing->StateFn = NULL;
        }

        return;
    }

    do_car_input(p_thing);

    // Skip wall collision when fully on road — performance optimisation for NPC cars.
    VEH_collide_line_ignore_walls = VEH_on_road(p_thing, 0) && VEH_on_road(p_thing, TICK_RATIO);

    SLONG ignore_prims;

    if (GAME_FLAGS & GF_CARS_WITH_ROAD_PRIMS) {
        ignore_prims = UC_FALSE;
    } else {
        ignore_prims = VEH_collide_line_ignore_walls;
    }

    // Player always gets full collision.
    {
        Thing* p_driver;
        p_driver = TO_THING(p_thing->Genus.Vehicle->Driver);
        if (p_driver->Class == CLASS_PERSON) {
            if (p_driver->Genus.Person->PlayerID) {
                ignore_prims = 0;
                VEH_collide_line_ignore_walls = 0;
            }
        }
    }

    p_thing->Flags &= ~FLAGS_COLLIDED;

    SLONG vel = Root((veh->VelX >> 4) * (veh->VelX >> 4) + (veh->VelZ >> 4) * (veh->VelZ >> 4)) >> 4;

    VEH_collide_find_things(
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Y >> 8,
        p_thing->WorldPos.Z >> 8,
        0x180 + vel,
        THING_NUMBER(p_thing),
        ignore_prims);

    coltype = CollideCar(p_thing, TICK_RATIO);

    // Apply velocities to position and angle.
    GameCoord new_pos;

    new_pos.X = p_thing->WorldPos.X + ((veh->VelX * TICK_RATIO) >> TICK_SHIFT);
    new_pos.Y = p_thing->WorldPos.Y + veh->VelY;
    new_pos.Z = p_thing->WorldPos.Z + ((veh->VelZ * TICK_RATIO) >> TICK_SHIFT);

    move_thing_on_map(p_thing, &new_pos);
    veh->Angle += (veh->VelR * TICK_RATIO) >> TICK_SHIFT;
    veh->Angle &= 2047;

    // Runover detection: find persons in the vehicle's bounding box and knock them down.
    if ((veh->VelX || veh->VelZ) && veh->Driver) {
        SLONG i;

#define MAX_RUNOVER 8

        UWORD people[MAX_RUNOVER];
        SLONG num;

        SLONG box_valid = UC_FALSE;
        SLONG miny;
        SLONG maxy;
        SLONG prim;
        SLONG useangle;
        SLONG sin_yaw;
        SLONG cos_yaw;
        SLONG matrix[4];

        SLONG tx;
        SLONG tz;

        SLONG rx;
        SLONG rz;

        PrimInfo* pi;
        Thing* p_found;

        num = THING_find_sphere(
            p_thing->WorldPos.X >> 8,
            p_thing->WorldPos.Y >> 8,
            p_thing->WorldPos.Z >> 8,
            0x200,
            people,
            MAX_RUNOVER,
            1 << CLASS_PERSON);

        if (num) {
            prim = get_vehicle_body_prim(p_thing->Genus.Vehicle->Type);
            pi = get_prim_info(prim);
            miny = (p_thing->WorldPos.Y - get_vehicle_body_offset(p_thing->Genus.Vehicle->Type) >> 8) + pi->miny - 0x80;
            maxy = (p_thing->WorldPos.Y - get_vehicle_body_offset(p_thing->Genus.Vehicle->Type) >> 8) + pi->maxy - 0x80;
        }

        for (i = 0; i < num; i++) {
            p_found = TO_THING(people[i]);

            if (p_found->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                // Can't run yourself over.
            } else if (p_found->OnFace) {
                // Ignore people on faces (might be dangling on this car).
            } else if (!WITHIN((p_found->WorldPos.Y >> 8), miny, maxy)) {
                // Out of vertical range.
            } else {
                if (!box_valid) {
                    useangle = -p_thing->Genus.Vehicle->Angle;
                    useangle &= 2047;

                    sin_yaw = SIN(useangle);
                    cos_yaw = COS(useangle);

                    matrix[0] = cos_yaw;
                    matrix[1] = sin_yaw;
                    matrix[2] = -sin_yaw;
                    matrix[3] = cos_yaw;

                    box_valid = UC_TRUE;
                }

                // Rotate person into vehicle local space.
                tx = p_found->WorldPos.X - p_thing->WorldPos.X >> 8;
                tz = p_found->WorldPos.Z - p_thing->WorldPos.Z >> 8;

                rx = MUL64(tx, matrix[0]) + MUL64(tz, matrix[1]);
                rz = MUL64(tx, matrix[2]) + MUL64(tz, matrix[3]);

                if (WITHIN(rx, pi->minx - 0x18, pi->maxx + 0x18) && WITHIN(rz, pi->minz - 0x18, pi->maxz + 0x18)) {
                    Thing* p_driver = get_vehicle_driver(p_thing);

                    if (p_found->State == STATE_DEAD || p_found->State == STATE_DYING) {
                        // Ran over a corpse or knocked-down person.
                        if (is_person_ko_and_lay_down(p_found)) {
                            SLONG anim;

                            switch (person_is_lying_on_what(p_found)) {
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

                            if (p_found->Draw.Tweened->CurrentAnim != anim || (p_found->Draw.Tweened->CurrentFrame->Flags & ANIM_FLAG_LAST_FRAME))
                                set_person_ko_recoil(p_found, anim, 0);
                        }

                    } else {
                        if ((!VIOLENCE) && (p_found->Genus.Person->PersonType == PERSON_COP || p_found->Genus.Person->PersonType == PERSON_CIV)) {
                            // Censored mode: flip instead of knock down.
                            if (p_found->SubState != SUB_STATE_FLIPING) {
                                set_person_flip(p_found, Random() & 0x1);
                            }

                        } else {
                            knock_person_down(p_found,
                                GetRunoverHP(p_thing, p_found),
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Z >> 8,
                                p_driver);

                            MFX_play_thing(THING_NUMBER(p_found), S_THUMP_SQUISH, MFX_REPLACE, p_found);
                        }
                    }
                }
            }
        }
    }

    // Disturb leaves and mist.
    DIRT_gust(
        p_thing,
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Z >> 8,
        new_pos.X >> 8,
        new_pos.Z >> 8);

    MIST_gust(
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Z >> 8,
        new_pos.X >> 8,
        new_pos.Z >> 8);

    // Knock down barrels.
    {
        SLONG prim = get_vehicle_body_prim(p_thing->Genus.Vehicle->Type);

        BARREL_hit_with_prim(
            prim,
            p_thing->WorldPos.X >> 8,
            p_thing->WorldPos.Y - get_vehicle_body_offset(p_thing->Genus.Vehicle->Type) >> 8,
            p_thing->WorldPos.Z >> 8,
            p_thing->Genus.Vehicle->Angle);
    }

    // Scare nearby pedestrians when moving fast.
    if (p_thing->Velocity > 500) {
        dx = -SIN(p_thing->Genus.Vehicle->Angle) << 1;
        dz = -COS(p_thing->Genus.Vehicle->Angle) << 1;

        PCOM_oscillate_tympanum(
            PCOM_SOUND_VAN,
            NULL,
            p_thing->WorldPos.X + dx >> 8,
            p_thing->WorldPos.Y >> 8,
            p_thing->WorldPos.Z + dz >> 8);
    }
}

// =============================================================================
// CHUNK 3: Steering input, suspension physics, VEH_reduce_health, wheel query API
// =============================================================================

// Plays or stops the siren loop (police, ambulance, meatwagon only).
// veh->Siren tracks current state to avoid redundant MFX calls.
// uc_orig: siren (fallen/Source/Vehicle.cpp)
static void siren(Vehicle* veh, UBYTE play)
{
    if (veh->Siren == play)
        return;

    if ((veh->Type == VEH_TYPE_POLICE) || (veh->Type == VEH_TYPE_AMBULANCE) || (veh->Type == VEH_TYPE_MEATWAGON)) {
        if (play)
            MFX_play_ambient(VEHICLE_NUMBER(veh), S_CAR_SIREN1, MFX_LOOPED);
        else
            MFX_stop(VEHICLE_NUMBER(veh), S_CAR_SIREN1);
    }

    veh->Siren = play;
}

// Updates the steering wheel position (veh->Wheel) from Steering input and velocity.
// Digital input: increments/decrements Wheel by TICK_RATIO per frame, speed-scaled.
// Analog input: maps Steering directly to Wheel (inverse-speed-scaled for control at speed).
// Converts Wheel to WheelAngle via arctan LUT.
// uc_orig: steering_wheel (fallen/Source/Vehicle.cpp)
void steering_wheel(Vehicle* veh, SLONG velocity, bool player)
{
    SLONG inc = TICK_RATIO;

    if (!(veh->Flags & FLAG_FURN_DRIVING) || (veh->Flags & FLAG_VEH_IN_AIR)) {
        // No driver or airborne: bring wheel toward centre.
        if (veh->Wheel > inc)
            veh->Wheel -= inc;
        else if (veh->Wheel < -inc)
            veh->Wheel += inc;
        else
            veh->Wheel = 0;
    } else {
        if (player) {
            velocity = abs(velocity);

            if (velocity > 1000)
                velocity = 1000;

            if (veh->IsAnalog) {
                veh->Wheel = (veh->Steering * (700 - (velocity >> 1))) >> (3);

                if (veh->Wheel > (WHEELTIME << TICK_SHIFT)) {
                    veh->Wheel = WHEELTIME << TICK_SHIFT;
                } else if (veh->Wheel < -(WHEELTIME << TICK_SHIFT)) {
                    veh->Wheel = -(WHEELTIME << TICK_SHIFT);
                }
                goto steering_done;
            } else {
                inc = (inc * (256 - velocity / 8)) >> 7;
            }
        }

        {
            if (veh->Steering > 0) {
                if (veh->Wheel < 0)
                    veh->Wheel = 0;
                else
                    veh->Wheel += inc;

                if (veh->Wheel > (WHEELTIME << TICK_SHIFT))
                    veh->Wheel = WHEELTIME << TICK_SHIFT;
            } else if (veh->Steering < 0) {
                if (veh->Wheel > 0)
                    veh->Wheel = 0;
                else
                    veh->Wheel -= inc;

                if (veh->Wheel < -(WHEELTIME << TICK_SHIFT))
                    veh->Wheel = -(WHEELTIME << TICK_SHIFT);
            } else {
                if (veh->Wheel > 0)
                    veh->Wheel >>= 1;
                else if (veh->Wheel < 0)
                    veh->Wheel = (veh->Wheel + 1) >> 1;
            }
        }
    steering_done:;
    }

    veh->WheelAngle = arctan_table[(veh->Wheel >> TICK_SHIFT) + WHEELTIME];
}

// Translates DControl bitmask to friction, acceleration, Dir, and Skid each tick.
// Also manages siren toggle and wheelspin smoke.
// uc_orig: pedals (fallen/Source/Vehicle.cpp)
static inline void pedals(Vehicle* veh, VehInfo* vinfo, SLONG velocity, UBYTE& friction, UWORD& move_cancel, SWORD& accel, Thing* p_thing)
{

    if (veh->DControl & VEH_ACCEL)
        veh->Skid = 0;

    // Gate Darci's exit condition.
    if (veh->DControl & (VEH_ACCEL | VEH_DECEL)) {
        veh->GrabAction = 1;
    } else if (!(veh->DControl & VEH_FASTER)) {
        veh->GrabAction = 0;
    }

    if (!(veh->Flags & FLAG_FURN_DRIVING)) {
        // No driver: coast to stop with strong friction.
        siren(veh, 0);
        veh->Dir = 0;
        friction -= 4;
    } else if (veh->DControl & VEH_ACCEL) {

        if (veh->Dir < 0) {
            // Braking while in reverse.
            veh->Dir = -1;
            friction -= 4;
            accel = vinfo->SoftBrake;
            move_cancel = INPUT_CAR_ACCELERATE;
        } else {
            veh->Dir = +2;
            accel = vinfo->FwdAccel;
            if (veh->DControl & VEH_FASTER) {
                if (velocity < VEH_SPEED_LIMIT)
                    accel <<= 1;
                else if (velocity < VEH_SPEED_LIMIT * 2)
                    accel += (accel >> 1);
            } else {
                if (velocity >= VEH_SPEED_LIMIT)
                    accel = 0;
            }

            if ((velocity < -200) || ((veh->DControl & VEH_FASTER) && (velocity < 400))) {
                veh->Smokin = 1;
            }
        }
    } else if (veh->DControl & VEH_DECEL) {
        if ((veh->Dir > 0) || (velocity > 200)) {
            // Braking while going forwards.
            veh->Dir = +1;
            friction -= 4;
            accel = -vinfo->SoftBrake;
            move_cancel = INPUT_CAR_DECELERATE;

            if (!veh->Skid) {
                if ((veh->DControl & VEH_FASTER) && (velocity > 1600))
                    veh->Skid = 1;
            } else {
                veh->Skid++;
                if (veh->Skid == SKID_START) {
                    if (!veh->VelR) {
                        if (Random() & 128)
                            veh->VelR = velocity >> 6;
                        else
                            veh->VelR = -velocity >> 6;
                    } else {
                        if (veh->VelR > 0)
                            veh->VelR = velocity >> 5;
                        else
                            veh->VelR = -velocity >> 5;
                    }
                }
            }
        } else {
            // Accelerating backwards.
            veh->Dir = -2;
            accel = -vinfo->BkAccel;
        }
    } else {
        // Engine braking only.
        friction -= 1;

        if ((veh->Dir == -1) || (veh->Dir == +1))
            veh->Dir = 0;
    }

    // Siren toggle.
    if (veh->DControl & VEH_SIREN) {
        siren(veh, !veh->Siren);
        move_cancel = INPUT_CAR_SIREN;
    }
}

// Full per-tick input and physics update for one vehicle.
// Non-skid: applies friction + accel to Velocity scalar, converts to VelX/VelZ/VelR via
//   turning circle geometry. Skid: damps VelX/VelZ directly, exits when car aligns.
// uc_orig: do_car_input (fallen/Source/Vehicle.cpp)
static void do_car_input(Thing* p_thing)
{
    Vehicle* veh = p_thing->Genus.Vehicle;
    VehInfo* vinfo = &veh_info[p_thing->Genus.Vehicle->Type];

    if (!(veh->Flags & FLAG_FURN_DRIVING) && !veh->VelX && !veh->VelZ) {
        siren(veh, 0);
        veh->VelR = 0;
        return;
    }

    steering_wheel(veh, p_thing->Velocity, is_driven_by_player(p_thing));

    if (veh->Flags & FLAG_VEH_IN_AIR) {
        // Do nothing while airborne.
    } else if (veh->Skid < SKID_START) {
        UBYTE friction;
        UWORD move_cancel;
        SWORD accel;

        pedals(veh, vinfo, p_thing->Velocity, friction = 7, move_cancel = 0, accel = 0, p_thing);

        if (move_cancel == INPUT_CAR_SIREN && p_thing->Genus.Vehicle->Driver) {
            Thing* p_driver;
            p_driver = TO_THING(p_thing->Genus.Vehicle->Driver);
            if (p_driver->Genus.Person->PlayerID) {
                Thing* p_player;
                p_player = NET_PLAYER(p_driver->Genus.Person->PlayerID - 1);
                p_player->Genus.Player->InputDone = move_cancel;
            }
        }

        SLONG oldvel = p_thing->Velocity;
        SLONG oldmag = abs(oldvel);

        p_thing->Velocity = ((SLONG(p_thing->Velocity) << friction) - SLONG(p_thing->Velocity)) >> friction;
        p_thing->Velocity += accel;

        SLONG newvel = p_thing->Velocity;
        SLONG newmag = abs(newvel);

        SLONG realacc = newvel - oldvel;

        // Apply forces to suspension from acceleration.
        if (realacc > 0) {
            if (abs(veh->DY[0]) < (realacc << 4))
                veh->DY[0] -= realacc << 2;
            if (abs(veh->DY[1]) < (realacc << 4))
                veh->DY[1] -= realacc << 2;
        } else {
            if (abs(veh->DY[0]) < (-realacc << 2))
                veh->DY[0] += -realacc;
            if (abs(veh->DY[1]) < (-realacc << 2))
                veh->DY[1] += -realacc;
        }

        bool stopped = false;

        if ((veh->Dir == +2) || (veh->Dir == -2)) {
            if (!accel && (newmag < 10))
                stopped = true;
        } else if ((veh->Dir == +1) || (veh->Dir == -1)) {
            if ((newmag < 10) || (newmag > oldmag))
                stopped = true;
        } else {
            ASSERT(accel == 0);
            if (newmag < 10)
                stopped = true;
        }

        if (stopped) {
            p_thing->Velocity = 0;
            veh->Dir = 0;

            if (move_cancel && p_thing->Genus.Vehicle->Driver) {
                Thing* p_driver;
                p_driver = TO_THING(p_thing->Genus.Vehicle->Driver);
                if (p_driver->Genus.Person->PlayerID) {
                    Thing* p_player;
                    p_player = NET_PLAYER(p_driver->Genus.Person->PlayerID - 1);
                    p_player->Genus.Player->InputDone = move_cancel;
                }
            }
            if (!(veh->Flags & FLAG_FURN_DRIVING)) {
                p_thing->Velocity = 0;
                veh->Wheel = 0;
                veh->WheelAngle = 0;
            }

            veh->VelX = 0;
            veh->VelZ = 0;
            veh->VelR = 0;

            return;
        }
    } else {
        // Skidding: damp VelX/VelZ directly.
        veh->VelX = ((SLONG(veh->VelX) << 4) - SLONG(veh->VelX)) >> 4;
        veh->VelZ = ((SLONG(veh->VelZ) << 4) - SLONG(veh->VelZ)) >> 4;
        if (veh->VelR > 0)
            veh->VelR = ((SLONG(veh->VelR) << 4) - SLONG(veh->VelR)) >> 4;
        else
            veh->VelR = veh->VelR - (veh->VelR >> 4);

        if (veh->Steering < 0) {
            if (veh->VelR < 32)
                veh->VelR += (abs(veh->VelX) + abs(veh->VelZ)) >> 12;
        } else if (veh->Steering > 0) {
            if (veh->VelR > -32)
                veh->VelR -= (abs(veh->VelX) + abs(veh->VelZ)) >> 12;
        }

        if (abs(veh->VelX) < 2048)
            veh->VelX = 0;
        if (abs(veh->VelZ) < 2048)
            veh->VelZ = 0;

        if (!veh->VelX && !veh->VelZ)
            veh->VelR = (veh->VelR + 1) >> 1;

        p_thing->Velocity = (p_thing->Velocity + 1) >> 1;

        if (abs(p_thing->Velocity) < 10)
            p_thing->Velocity = 0;

        if (abs(veh->VelR) < 16) {
            // Check if skid angle aligns with car direction (cos^2 > 15/16).
            SLONG dx = veh->VelX >> (8 - CAR_VEL_SHIFT);
            SLONG dz = veh->VelZ >> (8 - CAR_VEL_SHIFT);

            SLONG mvx = dx * dx + dz * dz;
            SLONG dp = (dx * -SIN(veh->Angle) + dz * COS(veh->Angle)) >> 16;

            if ((dp > 0) && (dp * dp > (mvx - (mvx >> 2)))) {
                p_thing->Velocity = Root(mvx);
                veh->Skid = 0;
            }
        }

        if (!veh->VelX && !veh->VelZ && !p_thing->Velocity) {
            veh->Skid = 0;
        }

        veh->Dir = (veh->DControl & VEH_DECEL) ? +1 : 0;

        veh->Smokin = 1;
    }

    // Steer the car: compute VelX/VelZ/VelR from turning circle geometry.
    SLONG dangle;
    SLONG dx, dz;

    if (veh->WheelAngle && !(veh->Flags & FLAG_VEH_IN_AIR)) {
        SLONG tcx, tcz, tcr;
        SLONG dx1, dz1;

        if (veh->WheelAngle < 0) {
            SLONG angle = -veh->WheelAngle;
            SLONG wheelbase = vinfo->DZ[0] - vinfo->DZ[2];

            tcr = DIV64(wheelbase, SIN(angle));
            tcx = vinfo->DX[0] - (tcr * COS(angle) >> 16);
            tcz = vinfo->DZ[0];
        } else {
            SLONG angle = veh->WheelAngle;
            SLONG wheelbase = vinfo->DZ[1] - vinfo->DZ[3];

            tcr = DIV64(wheelbase, SIN(angle));
            tcx = vinfo->DX[1] + (tcr * COS(angle) >> 16);
            tcz = vinfo->DZ[1];
        }

        SLONG radangle = DIV64(p_thing->Velocity << (8 - CAR_VEL_SHIFT), tcr << 8);

        // Convert radians to 0-2047 game angle units (divide by 2*PI = multiply by 10430 then >>5).
        SLONG angle = MUL64(radangle, 10430);
        angle >>= 5;

        if (!angle)
            angle = 1;

        // Apply lateral suspension forces from cornering.
        if (veh->WheelAngle > 0) {
            if (abs(veh->DY[1]) < (angle << 2))
                veh->DY[1] += angle << 1;
            if (abs(veh->DY[3]) < (angle << 2))
                veh->DY[3] += angle << 1;

            dangle = -angle;
        } else {
            if (abs(veh->DY[0]) < (angle << 2))
                veh->DY[0] += angle << 1;
            if (abs(veh->DY[2]) < (angle << 2))
                veh->DY[2] += angle << 1;

            dangle = +angle;
        }

        dangle &= 2047;

        if (angle > 2) {
            if (veh->WheelAngle < 0) {
                SLONG c = COS(dangle) - 65536;
                SLONG s = SIN(dangle);

                dx1 = (-tcx * c - -tcz * s) >> (16 - CAR_VEL_SHIFT);
                dz1 = -(-tcx * s + -tcz * c) >> (16 - CAR_VEL_SHIFT);
            } else {
                SLONG c = COS(dangle) - 65536;
                SLONG s = -SIN(dangle);

                dx1 = -(tcx * c - -tcz * s) >> (16 - CAR_VEL_SHIFT);
                dz1 = -(tcx * s + -tcz * c) >> (16 - CAR_VEL_SHIFT);
            }
        } else {
            dx1 = 0;
            dz1 = -SLONG(p_thing->Velocity);
        }

        dx = (SIN(veh->Angle) * dz1 - COS(veh->Angle) * dx1) >> (8 + CAR_VEL_SHIFT);
        dz = (COS(veh->Angle) * dz1 + SIN(veh->Angle) * dx1) >> (8 + CAR_VEL_SHIFT);
        dangle = (dangle & 1024) ? dangle - 2048 : dangle;
    } else {
        // Straight ahead.
        SLONG dx1 = 0;
        SLONG dz1 = -SLONG(p_thing->Velocity);

        dx = (SIN(veh->Angle) * dz1 - COS(veh->Angle) * dx1) >> (8 + CAR_VEL_SHIFT);
        dz = (COS(veh->Angle) * dz1 + SIN(veh->Angle) * dx1) >> (8 + CAR_VEL_SHIFT);
        dangle = 0;
    }

    // Detect skid onset from lateral force.
    if ((veh->VelX || veh->VelZ) && (veh->Skid < SKID_START)) {
        SLONG ax, az;
        SLONG vx, vz, vv;
        SLONG av;

        ax = dx - veh->VelX;
        az = dz - veh->VelZ;

        vx = veh->VelX;
        vz = veh->VelZ;
        vv = vx * vx + vz * vz;
        if (vv) {
            vv = Root(vv);

            av = (ax * vz) - (az * vx);

            if (abs(av) > SKID_FORCE * vv)
                veh->Skid = SKID_START;
            else if (abs(av) > ((NEAR_SKID_FORCE * vinfo->FwdAccel) >> 5) * vv)
                MFX_play_thing(THING_NUMBER(p_thing), SOUND_Range(S_SKID_SLOW_START, S_SKID_SLOW_END), MFX_MOVING, p_thing);
        }
    }

    if (veh->Skid < SKID_START) {
        veh->VelX = dx;
        veh->VelZ = dz;
        veh->VelR = dangle;
    }

    // Spin wheels.
    veh->Spin = (veh->Spin + (p_thing->Velocity >> 2)) & 2047;

// Engine sound state constants (used by do_car_input sound state machine).
// uc_orig: VEH_FWD_ACCEL (fallen/Source/Vehicle.cpp)
#define VEH_FWD_ACCEL (1)
// uc_orig: VEH_FWD_DECEL (fallen/Source/Vehicle.cpp)
#define VEH_FWD_DECEL (2)
// uc_orig: VEH_REV_ACCEL (fallen/Source/Vehicle.cpp)
#define VEH_REV_ACCEL (3)
// uc_orig: VEH_REV_DECEL (fallen/Source/Vehicle.cpp)
#define VEH_REV_DECEL (4)

    if (is_driven_by_player(p_thing)) {
        UBYTE state = 0;

        if (veh->Dir >= 0) {
            if ((veh->DControl & VEH_ACCEL) && !veh->Skid)
                state = VEH_FWD_ACCEL;
            else
                state = VEH_FWD_DECEL;
        } else {
            if ((veh->DControl & VEH_DECEL) && !veh->Skid)
                state = VEH_REV_ACCEL;
            else
                state = VEH_REV_DECEL;
        }

        if (state != veh->LastSoundState) {
            MFX_stop(THING_NUMBER(p_thing), S_CARX_START);
            MFX_stop(THING_NUMBER(p_thing), S_CARX_CRUISE);
            MFX_stop(THING_NUMBER(p_thing), S_CARX_DECEL);
            MFX_stop(THING_NUMBER(p_thing), S_CARX_IDLE);
            MFX_stop(THING_NUMBER(p_thing), S_CAR_REVERSE_START);
            MFX_stop(THING_NUMBER(p_thing), S_CAR_REVERSE_LOOP);
            MFX_stop(THING_NUMBER(p_thing), S_CAR_REVERSE_END);
            switch (state) {
            case VEH_FWD_ACCEL:
                MFX_play_ambient(THING_NUMBER(p_thing), S_CARX_START, MFX_MOVING | MFX_EARLY_OUT);
                MFX_play_ambient(THING_NUMBER(p_thing), S_CARX_CRUISE, MFX_MOVING | MFX_QUEUED | MFX_SHORT_QUEUE | MFX_LOOPED);
                break;
            case VEH_FWD_DECEL:
                MFX_play_ambient(THING_NUMBER(p_thing), S_CARX_DECEL, MFX_MOVING | MFX_EARLY_OUT);
                MFX_play_ambient(THING_NUMBER(p_thing), S_CARX_IDLE, MFX_MOVING | MFX_QUEUED | MFX_SHORT_QUEUE | MFX_LOOPED);
                break;
            case VEH_REV_ACCEL:
                MFX_play_ambient(THING_NUMBER(p_thing), S_CAR_REVERSE_START, MFX_MOVING | MFX_EARLY_OUT);
                MFX_play_ambient(THING_NUMBER(p_thing), S_CAR_REVERSE_LOOP, MFX_MOVING | MFX_QUEUED | MFX_SHORT_QUEUE | MFX_LOOPED);
                break;
            case VEH_REV_DECEL:
                MFX_play_ambient(THING_NUMBER(p_thing), S_CAR_REVERSE_END, MFX_MOVING | MFX_EARLY_OUT);
                MFX_play_ambient(THING_NUMBER(p_thing), S_CARX_IDLE, MFX_MOVING | MFX_QUEUED | MFX_SHORT_QUEUE | MFX_LOOPED);
                break;
            }
        }

        veh->LastSoundState = state;
    }
}

// One sub-step of spring physics for a single wheel contact point.
// penetrate_dist: how far below ground the wheel is (positive = penetrating).
// Damps velocity, updates compression, computes quadratic spring restoring force.
// Returns new DY velocity for this wheel.
// uc_orig: apply_thrust_to_suspension (fallen/Source/Vehicle.cpp)
inline static SLONG apply_thrust_to_suspension(Suspension* p_sus, SLONG velocity, SLONG penetrate_dist)
{
    SLONG acc;
    SLONG compression;

    velocity = ((velocity << 4) - velocity) >> 4;

    compression = p_sus->Compression - (velocity - penetrate_dist);

    if (compression < MIN_COMPRESSION)
        compression = MIN_COMPRESSION;
    else if (compression > MAX_COMPRESSION)
        compression = MAX_COMPRESSION;

    // Non-linear spring (quadratic, not Hooke's law — linear was unstable in testing).
    acc = (compression >> 5) * (compression >> 5) >> 9;

    velocity += GRAVITY + acc;

    // If maximally compressed (stepping up), keep wheel above surface.
    if (compression == MAX_COMPRESSION)
        velocity += penetrate_dist;

    p_sus->Compression = compression;

    return velocity;
}

// Allows the spring to extend toward MIN_COMPRESSION when the wheel is in air.
// size: distance from wheel to ground (scaled to 3/4 to limit extension rate).
// uc_orig: expand_suspension (fallen/Source/Vehicle.cpp)
inline static void expand_suspension(Suspension* p_sus, SLONG size)
{
    ASSERT(size >= 0);

    size -= size >> 2;

    if (p_sus->Compression - size < MIN_COMPRESSION)
        p_sus->Compression = MIN_COMPRESSION;
    else
        p_sus->Compression -= size;
}

// Forward declaration for do_car_fall_and_tilt (defined below process_car).
// uc_orig: do_car_fall_and_tilt (fallen/Source/Vehicle.cpp)
static void do_car_fall_and_tilt(Thing* p_car, SLONG* wx, SLONG* wy, SLONG* wz, SLONG* dy);

// Main suspension and orientation update. Called from CollideCar and VEH_driving.
// Runs TICK_LOOP (4) sub-steps for each of 4 wheels per tick.
// Also manages in-air flag, suspension sounds, and headlight position.
// uc_orig: process_car (fallen/Source/Vehicle.cpp)
static void process_car(Thing* p_car)
{
    SLONG count;
    SLONG wheel;
    SLONG c0;
    SLONG wx[4], wy[4], wz[4];
    SLONG dy[4];
    VehInfo* info;
    Vehicle* vp;
    BOOL squeaky = 0;
    BOOL crunchy = 0;

    vp = p_car->Genus.Vehicle;
    info = &veh_info[vp->Type];

    if (vp->Flags & FLAG_VEH_ANIMATING) {
        // Reserved for animated car sequences (not active in this build).
    }

    make_car_matrix(vp);

    UBYTE on_road = 0;

    {
        int in_air = 0;

        for (wheel = 0; wheel < 4; wheel++) {
            SLONG y;
            SLONG height;

            y = p_car->WorldPos.Y;

            dy[wheel] = 0;

            ASSERT(p_car->Genus.Vehicle->Spring[wheel].Compression >= MIN_COMPRESSION);

            wx[wheel] = info->DX[wheel];
            wy[wheel] = 0;
            wz[wheel] = info->DZ[wheel];

            apply_car_matrix(&wx[wheel], &wy[wheel], &wz[wheel]);

            SLONG papx = wx[wheel] + (p_car->WorldPos.X >> 8);
            SLONG papz = wz[wheel] + (p_car->WorldPos.Z >> 8);

            height = PAP_calc_map_height_at(papx, papz) << 8;

            if (ROAD_is_road(papx >> 8, papz >> 8))
                on_road |= (1 << wheel);

            SLONG y_pos;

            count = TICK_LOOP;
            while (--count) {
                SLONG size;

                // Cap DY to prevent runaway velocity.
                if (vp->DY[wheel] > 1536)
                    vp->DY[wheel] = 1536;

                size = (128 << 8) - vp->Spring[wheel].Compression;

                y_pos = y + (wy[wheel] << 8) - size;

                if (y_pos <= height) {
                    vp->DY[wheel] = apply_thrust_to_suspension(&vp->Spring[wheel], vp->DY[wheel], height - y_pos);
                    squeaky += abs(vp->DY[wheel]);
                    crunchy += vp->DY[wheel];
                } else {
                    if (vp->Spring[wheel].Compression > MIN_COMPRESSION) {
                        expand_suspension(&vp->Spring[wheel], y_pos - height);

                        if (vp->Spring[wheel].Compression > MIN_COMPRESSION) {
                            vp->DY[wheel] = apply_thrust_to_suspension(&vp->Spring[wheel], vp->DY[wheel], 0);
                        }
                    }
                    vp->DY[wheel] += GRAVITY;
                }

                y += vp->DY[wheel];
                dy[wheel] += vp->DY[wheel];
            }

            SLONG size = (128 << 8) - vp->Spring[wheel].Compression;

            y_pos = y + (wy[wheel] << 8) - size;

            if (y_pos - height > 1024)
                in_air++;
        }
        if (squeaky > 1600)
            MFX_play_thing(THING_NUMBER(p_car), SOUND_Range(S_CAR_SUSPENSION_START, S_CAR_SUSPENSION_END), MFX_MOVING, p_car);
        if (crunchy < -4000)
            MFX_play_thing(THING_NUMBER(p_car), S_CAR_SMASH_START + 1, MFX_MOVING, p_car);

        do_car_fall_and_tilt(p_car, &wx[0], &wy[0], &wz[0], &dy[0]);

        if (in_air == 4)
            vp->Flags |= FLAG_VEH_IN_AIR;
        else
            vp->Flags &= ~FLAG_VEH_IN_AIR;
    }

    // Update dynamic night driving light position.
    if (vp->dlight) {
        SLONG dx = -car_matrix[6] << 1;
        SLONG dy = 0x2000;
        SLONG dz = -car_matrix[8] << 1;

        SLONG lx;
        SLONG ly;
        SLONG lz;

        lx = p_car->WorldPos.X + dx >> 8;
        ly = p_car->WorldPos.Y + dy >> 8;
        lz = p_car->WorldPos.Z + dz >> 8;

        NIGHT_dlight_move(p_car->Genus.Vehicle->dlight, lx, ly, lz);
    }
}

// Fast square root (uses FPU sqrt on PC — fastest available on Intel chips at the time).
// uc_orig: fast_root (fallen/Source/Vehicle.cpp)
static inline SLONG fast_root(SLONG num)
{
    return (SLONG)sqrt((double)num);
}

// Normalizes a 3D integer vector to magnitude 256 (fixed-point unit vector).
// uc_orig: normalise_val256 (fallen/Source/Vehicle.cpp)
static inline void normalise_val256(SLONG* vx, SLONG* vy, SLONG* vz)
{
    SLONG len;

    len = *vx * *vx + *vy * *vy + *vz * *vz;

    len = fast_root(len);

    if (len)
        len = 65536 / len;
    else
        len = 65536;

    *vx = (*vx * len) >> 8;
    *vy = (*vy * len) >> 8;
    *vz = (*vz * len) >> 8;
}

// Builds a 3x3 rotation matrix from (across, nose, normal) vectors and extracts
// tilt and roll angles via FMATRIX_find_angles.
// uc_orig: calc_tilt_n_roll_with_matrix (fallen/Source/Vehicle.cpp)
static inline void calc_tilt_n_roll_with_matrix(SLONG across_x, SLONG across_y, SLONG across_z, SLONG nose_x, SLONG nose_y, SLONG nose_z, SLONG nx, SLONG ny, SLONG nz, SLONG* angle, SLONG* tilt, SLONG* roll)
{
    SLONG matrix[9];

    matrix[0] = (across_x) << 8;
    matrix[1] = (across_y) << 8;
    matrix[2] = (across_z) << 8;

    matrix[3] = (nx) << 8;
    matrix[4] = (ny) << 8;
    matrix[5] = (nz) << 8;

    matrix[6] = (nose_x) << 8;
    matrix[7] = (nose_y) << 8;
    matrix[8] = (nose_z) << 8;

    FMATRIX_find_angles(matrix, angle, tilt, roll);
}

// Computes car body Tilt and Roll from the 4 wheel world positions.
// Averages tilt/roll from all 4 wheel corner combinations; clamps to ±312 game angle units.
// uc_orig: calc_tilt_and_roll (fallen/Source/Vehicle.cpp)
static void calc_tilt_and_roll(SLONG* tilt, SLONG* roll, SLONG* whx, SLONG* why, SLONG* whz, SLONG angle)
{
    SLONG nx, ny, nz;

    SLONG vx, vy, vz;
    SLONG wx, wy, wz;
    SLONG wheel;

    SLONG x02, y02, z02;
    SLONG x10, y10, z10;
    SLONG x31, y31, z31;
    SLONG x23, y23, z23;

    SLONG tt = 0;
    SLONG tr = 0;

    x02 = whx[0] - whx[2];
    y02 = -(why[0] - why[2]);
    z02 = whz[0] - whz[2];

    normalise_val256(&x02, &y02, &z02);

    x10 = whx[1] - whx[0];
    y10 = -(why[1] - why[0]);
    z10 = whz[1] - whz[0];

    normalise_val256(&x10, &y10, &z10);

    x31 = whx[3] - whx[1];
    y31 = -(why[3] - why[1]);
    z31 = whz[3] - whz[1];

    normalise_val256(&x31, &y31, &z31);

    x23 = whx[2] - whx[3];
    y23 = -(why[2] - why[3]);
    z23 = whz[2] - whz[3];

    normalise_val256(&x23, &y23, &z23);

    for (wheel = 0; wheel < 4; wheel++) {
        switch (wheel) {
        case 0:
            vx = x02;
            vy = y02;
            vz = z02;
            wx = x10;
            wy = y10;
            wz = z10;

            nx = (vy * wz - vz * wy) >> 8;
            ny = (vz * wx - vx * wz) >> 8;
            nz = (vx * wy - vy * wx) >> 8;

            calc_tilt_n_roll_with_matrix(wx, wy, wz, vx, vy, vz, nx, ny, nz, &angle, tilt, roll);
            break;

        case 1:
            vx = x10;
            vy = y10;
            vz = z10;
            wx = x31;
            wy = y31;
            wz = z31;

            nx = (vy * wz - vz * wy) >> 8;
            ny = (vz * wx - vx * wz) >> 8;
            nz = (vx * wy - vy * wx) >> 8;

            calc_tilt_n_roll_with_matrix(vx, vy, vz, -wx, -wy, -wz, nx, ny, nz, &angle, tilt, roll);
            break;

        case 2:
            vx = x23;
            vy = y23;
            vz = z23;
            wx = x02;
            wy = y02;
            wz = z02;

            nx = (vy * wz - vz * wy) >> 8;
            ny = (vz * wx - vx * wz) >> 8;
            nz = (vx * wy - vy * wx) >> 8;

            calc_tilt_n_roll_with_matrix(-vx, -vy, -vz, wx, wy, wz, nx, ny, nz, &angle, tilt, roll);
            break;

        case 3:
            vx = x31;
            vy = y31;
            vz = z31;
            wx = x23;
            wy = y23;
            wz = z23;

            nx = (vy * wz - vz * wy) >> 8;
            ny = (vz * wx - vx * wz) >> 8;
            nz = (vx * wy - vy * wx) >> 8;

            calc_tilt_n_roll_with_matrix(-wx, -wy, -wz, -vx, -vy, -vz, nx, ny, nz, &angle, tilt, roll);
            break;
        }

        if (*roll > 1024)
            *roll = *roll - 2048;
        if (*tilt > 1024)
            *tilt = *tilt - 2048;

        tt += *tilt;
        tr += *roll;
    }

    tt /= 4;
    tr /= 4;

    if (tt < -312)
        tt = -312;
    else if (tt > 312)
        tt = 312;

    if (tr < -312)
        tr = -312;
    if (tr > 312)
        tr = 312;

    *tilt = -tt & 2047;
    *roll = -tr & 2047;
}

// Computes the car body's aggregate vertical movement from 4 wheel DY vectors,
// applies it to WorldPos.Y, then calculates Tilt and Roll from wheel height differentials.
// Also updates the Stable counter.
// uc_orig: do_car_fall_and_tilt (fallen/Source/Vehicle.cpp)
static void do_car_fall_and_tilt(Thing* car, SLONG* wx, SLONG* wy, SLONG* wz, SLONG* dy)
{
    SLONG min_dy, max_dy;
    SLONG c0, pos_count, neg_count;
    SLONG remove;
    SLONG tilt, roll;

    min_dy = 0x7FFFFFFF;
    max_dy = 0x80000000;
    pos_count = 0;
    neg_count = 0;

    for (c0 = 0; c0 < 4; c0++) {
        if (dy[c0] > 0)
            pos_count++;
        if (dy[c0] < 0)
            neg_count++;
        if (dy[c0] > max_dy)
            max_dy = dy[c0];
        if (dy[c0] < min_dy)
            min_dy = dy[c0];
    }

    if (pos_count == 4) {
        car->WorldPos.Y += max_dy;
        remove = max_dy;
    } else if (pos_count == 0) {
        car->WorldPos.Y += min_dy;
        remove = min_dy;
    } else {
        remove = 0;
    }

    dy[0] -= remove;
    dy[1] -= remove;
    dy[2] -= remove;
    dy[3] -= remove;

    wy[0] += dy[0] >> 8;
    wy[1] += dy[1] >> 8;
    wy[2] += dy[2] >> 8;
    wy[3] += dy[3] >> 8;

    calc_tilt_and_roll(&tilt, &roll, wx, wy, wz, car->Genus.Vehicle->Angle);

    if ((abs(car->Genus.Vehicle->Tilt - tilt) < 16) && (abs(car->Genus.Vehicle->Roll - roll) < 16) && !remove) {
        if (car->Genus.Vehicle->Stable != STABLE_COUNT)
            car->Genus.Vehicle->Stable++;
    } else {
        car->Genus.Vehicle->Stable = 0;
    }

    car->Genus.Vehicle->Tilt = tilt;
    car->Genus.Vehicle->Roll = roll;
}

// Applies damage to a vehicle from an external source (e.g. gunfire).
// Causer is used for murder tracking and red-mark assignment on police car destruction.
// Triggers explosion processing by reactivating VEH_driving as StateFn.
// uc_orig: VEH_reduce_health (fallen/Source/Vehicle.cpp)
void VEH_reduce_health(Thing* p_car, Thing* p_person, SLONG damage)
{
    ASSERT(p_car->Class == CLASS_VEHICLE);

    p_car->Genus.Vehicle->Health -= damage >> 1;

    if (p_car->Genus.Vehicle->Health <= 0) {
        if (p_car->Genus.Vehicle->Driver) {
            if (p_person) {
                p_person->Genus.Person->Flags2 |= FLAG2_PERSON_IS_MURDERER;
            }
        }

        if (p_car->Genus.Vehicle->Type == VEH_TYPE_POLICE || p_car->Genus.Vehicle->Type == VEH_TYPE_MEATWAGON) {
            if (p_person && p_person->Class == CLASS_PERSON && p_person->Genus.Person->PlayerID) {
                NET_PLAYER(0)->Genus.Player->RedMarks += 1;
            }
        }
    }

    p_car->StateFn = VEH_driving;
}

// Initializes the wheel position query state for p_vehicle.
// Must be called before vehicle_wheel_pos_get().
// uc_orig: vehicle_wheel_pos_init (fallen/Source/Vehicle.cpp)
void vehicle_wheel_pos_init(Thing* p_vehicle)
{
    vehicle_wheel_pos_vehicle = p_vehicle;
    vehicle_wheel_pos_info = &veh_info[p_vehicle->Genus.Vehicle->Type];

    make_car_matrix(p_vehicle->Genus.Vehicle);
}

// Returns the world-space position of wheel 'which' (0-3), accounting for suspension compression.
// uc_orig: vehicle_wheel_pos_get (fallen/Source/Vehicle.cpp)
void vehicle_wheel_pos_get(
    SLONG which,
    SLONG* wx,
    SLONG* wy,
    SLONG* wz)
{
    SLONG wheel_x;
    SLONG wheel_y;
    SLONG wheel_z;

    wheel_x = vehicle_wheel_pos_info->DX[which];
    wheel_y = 51 - (((128 << 8) - vehicle_wheel_pos_vehicle->Genus.Vehicle->Spring[which].Compression) >> 8);
    wheel_z = vehicle_wheel_pos_info->DZ[which];

    apply_car_matrix(
        &wheel_x,
        &wheel_y,
        &wheel_z);

    *wx = wheel_x + (vehicle_wheel_pos_vehicle->WorldPos.X >> 8);
    *wy = wheel_y + (vehicle_wheel_pos_vehicle->WorldPos.Y >> 8);
    *wz = wheel_z + (vehicle_wheel_pos_vehicle->WorldPos.Z >> 8);
}
