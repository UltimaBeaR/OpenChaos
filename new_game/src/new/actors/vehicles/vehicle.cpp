// uc_orig: vehicle.cpp (fallen/Source/Vehicle.cpp)
// Vehicle physics, collision, rendering, damage, and enter/exit logic.
// Chunk 1 of 3: VehInfo table, alloc/dealloc, draw_car, and utility functions.

#include <math.h>

#include "fallen/Headers/Game.h"
#include "fallen/DDEngine/Headers/Matrix.h"
#include "fallen/DDEngine/Headers/poly.h"
#include "fallen/DDEngine/Headers/oval.h"
#include "fallen/DDLibrary/Headers/DDLib.h"
#include "fallen/Headers/pap.h"
#include "fallen/Headers/fmatrix.h"
#include "fallen/Headers/statedef.h"
#include "fallen/Headers/prim.h"
#include "fallen/Headers/animate.h"
#include "fallen/Headers/sound.h"
#include "fallen/Headers/interact.h"
#include "fallen/Headers/ob.h"
#include "fallen/Headers/night.h"
#include "fallen/DDEngine/Headers/DrawXtra.h"
#include "fallen/DDEngine/Headers/aeng.h"
#include "fallen/DDEngine/Headers/mesh.h"
#include "fallen/Headers/pow.h"
#include "fallen/Headers/interfac.h"
#include "fallen/Headers/dirt.h"
#include "fallen/Headers/mist.h"
#include "fallen/Headers/barrel.h"
#include "fallen/Headers/road.h"
#include "fallen/DDEngine/Headers/font2d.h" // Temporary: font2d lives in DDEngine, not Headers/
#include "fallen/Headers/memory.h"
#include "engine/audio/mfx.h"
#include "effects/tracks.h"
#include "engine/effects/psystem.h"
#include "actors/vehicles/vehicle.h"
#include "actors/vehicles/vehicle_globals.h"

// Temporary: pcom.h needed for PCOM_set_person_ai_flee_person (used in VEH_throw_out_person)
#include "fallen/Headers/pcom.h"
// Temporary: psystem.h included via engine/effects but also needs old flags
#include "fallen/Headers/psystem.h"

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
            TRUE);
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

// Returns TRUE if all 4 edges of the vehicle (7 sample points each) lie on road tiles.
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
                return FALSE;
            }

            cx += dx;
            cz += dz;
        }
    }

    return TRUE;
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
