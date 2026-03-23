#ifndef ACTORS_VEHICLES_VEHICLE_H
#define ACTORS_VEHICLES_VEHICLE_H

// Vehicle physics system — ground vehicles (CLASS_VEHICLE).
// Covers physics, collision, rendering, damage model, and enter/exit sequencing.

#include "core/types.h"
#include "actors/core/state.h"
#include "actors/core/drawtype.h"

struct Thing;

// uc_orig: FLAG_VEH_DRIVING (fallen/Headers/Vehicle.h)
#define FLAG_VEH_DRIVING (1 << 0)
// uc_orig: FLAG_VEH_WHEEL1_GRIP (fallen/Headers/Vehicle.h)
#define FLAG_VEH_WHEEL1_GRIP (1 << 1)
// uc_orig: FLAG_VEH_WHEEL2_GRIP (fallen/Headers/Vehicle.h)
#define FLAG_VEH_WHEEL2_GRIP (1 << 2)
// uc_orig: FLAG_VEH_WHEEL3_GRIP (fallen/Headers/Vehicle.h)
#define FLAG_VEH_WHEEL3_GRIP (1 << 3)
// uc_orig: FLAG_VEH_WHEEL4_GRIP (fallen/Headers/Vehicle.h)
#define FLAG_VEH_WHEEL4_GRIP (1 << 4)
// uc_orig: FLAG_VEH_ANIMATING (fallen/Headers/Vehicle.h)
#define FLAG_VEH_ANIMATING (1 << 5)
// uc_orig: FLAG_VEH_FX_STATE (fallen/Headers/Vehicle.h)
#define FLAG_VEH_FX_STATE (1 << 6)
// uc_orig: FLAG_VEH_SHOT_AT (fallen/Headers/Vehicle.h)
#define FLAG_VEH_SHOT_AT (1 << 7)
// uc_orig: FLAG_VEH_STALLED (fallen/Headers/Vehicle.h)
#define FLAG_VEH_STALLED (1 << 8)
// uc_orig: FLAG_VEH_IN_AIR (fallen/Headers/Vehicle.h)
#define FLAG_VEH_IN_AIR (1 << 9)

// Per-wheel spring suspension data.
// uc_orig: Suspension (fallen/Headers/Vehicle.h)
typedef struct
{
    UWORD Compression; // current spring compression
    UWORD Length;      // current spring length
} Suspension;

// DControl button bitmask values.
// uc_orig: VEH_ACCEL (fallen/Headers/Vehicle.h)
#define VEH_ACCEL 1
// uc_orig: VEH_DECEL (fallen/Headers/Vehicle.h)
#define VEH_DECEL 2
// uc_orig: VEH_FASTER (fallen/Headers/Vehicle.h)
#define VEH_FASTER 4
// uc_orig: VEH_SIREN (fallen/Headers/Vehicle.h)
#define VEH_SIREN 8

// Maximum forward/reverse speed (world units per tick).
// uc_orig: VEH_SPEED_LIMIT (fallen/Headers/Vehicle.h)
#define VEH_SPEED_LIMIT 750
// uc_orig: VEH_REVERSE_SPEED (fallen/Headers/Vehicle.h)
#define VEH_REVERSE_SPEED 300

// Steering geometry: ticks to reach full wheel lock; width/depth ratio for arctan.
// uc_orig: WHEELTIME (fallen/Source/Vehicle.cpp)
#define WHEELTIME 35
// uc_orig: WHEELRATIO (fallen/Source/Vehicle.cpp)
#define WHEELRATIO 45

// uc_orig: Vehicle (fallen/Headers/Vehicle.h)
typedef struct
{
    DrawTween Draw;

    Suspension Spring[4]; // 4-wheel suspension
    SLONG DY[4];          // vertical velocity per wheel point

    SLONG Angle; // yaw (0-2047)
    SLONG Tilt;
    SLONG Roll;

    SBYTE Steering;    // -64..+64 or -1..+1
    UBYTE IsAnalog;    // if set, steering uses full -64..+64 range
    UBYTE DControl;    // VEH_ACCEL / VEH_DECEL / VEH_FASTER / VEH_SIREN bitmask
    UBYTE GrabAction;  // action key was grabbed this frame

    SWORD Wheel;      // steering wheel position
    UWORD Allocated;  // 0 if slot is free

    UWORD Flags;
    UWORD Driver;    // THING_INDEX of driver, 0 = none

    UWORD Passenger; // linked list of passengers
    UWORD Type;      // VEH_TYPE_*

    UBYTE still;       // ticks at zero velocity (for stability detection)
    UBYTE dlight;      // dynamic night light handle
    UBYTE key;         // key required to enter, or SPECIAL_NONE
    UBYTE Brakelight;  // brakelight timer

    UBYTE damage[6]; // crumple zone damage (0=intact, 4=max)
    SWORD Spin;      // wheel rotation accumulator

    SLONG VelX;
    SLONG VelY;
    SLONG VelZ;
    SLONG VelR; // rotational velocity

    SWORD WheelAngle;
    UBYTE Siren;          // siren active
    UBYTE LastSoundState; // previous sound state for change detection

    SBYTE Dir;   // +2=fwd, +1=fwd braking, -1/-2=reverse variants, 0=stopped
    UBYTE Skid;  // 0=no skid; >=SKID_START=full skid
    UBYTE Stable;
    UBYTE Smokin; // tyre smoke active

    UBYTE Scrapin;      // scraping metal against wall
    UBYTE OnRoadFlags;  // per-wheel on-road bitmask
    SWORD Health;       // 300=full; 0=explode

    SLONG oldX[4], oldZ[4]; // previous wheel positions for skidmark rendering

} Vehicle;

typedef Vehicle* VehiclePtr;

// Vehicle pool size limits.
// uc_orig: RMAX_VEHICLES (fallen/Headers/Vehicle.h)
#define RMAX_VEHICLES 40
// uc_orig: MAX_VEHICLES (fallen/Headers/Vehicle.h)
#define MAX_VEHICLES (save_table[SAVE_TABLE_VEHICLE].Maximum)
// uc_orig: VEH_NULL (fallen/Headers/Vehicle.h)
#define VEH_NULL 65000

// Vehicle type constants (Vehicle.Type).
// uc_orig: VEH_TYPE_VAN (fallen/Headers/Vehicle.h)
#define VEH_TYPE_VAN 0
// uc_orig: VEH_TYPE_CAR (fallen/Headers/Vehicle.h)
#define VEH_TYPE_CAR 1
// uc_orig: VEH_TYPE_TAXI (fallen/Headers/Vehicle.h)
#define VEH_TYPE_TAXI 2
// uc_orig: VEH_TYPE_POLICE (fallen/Headers/Vehicle.h)
#define VEH_TYPE_POLICE 3
// uc_orig: VEH_TYPE_AMBULANCE (fallen/Headers/Vehicle.h)
#define VEH_TYPE_AMBULANCE 4
// uc_orig: VEH_TYPE_JEEP (fallen/Headers/Vehicle.h)
#define VEH_TYPE_JEEP 5
// uc_orig: VEH_TYPE_MEATWAGON (fallen/Headers/Vehicle.h)
#define VEH_TYPE_MEATWAGON 6
// uc_orig: VEH_TYPE_SEDAN (fallen/Headers/Vehicle.h)
#define VEH_TYPE_SEDAN 7
// uc_orig: VEH_TYPE_WILDCATVAN (fallen/Headers/Vehicle.h)
#define VEH_TYPE_WILDCATVAN 8
// uc_orig: VEH_TYPE_NUMBER (fallen/Headers/Vehicle.h)
#define VEH_TYPE_NUMBER 9

// Collision candidate descriptor — used by VEH_collide_find_things / CollideCar.
// uc_orig: VEH_COL_TYPE_BBOX (fallen/Headers/Vehicle.h)
#define VEH_COL_TYPE_BBOX 0
// uc_orig: VEH_COL_TYPE_CYLINDER (fallen/Headers/Vehicle.h)
#define VEH_COL_TYPE_CYLINDER 1

// uc_orig: VEH_Col (fallen/Headers/Vehicle.h)
typedef struct
{
    UWORD type;       // VEH_COL_TYPE_BBOX or VEH_COL_TYPE_CYLINDER
    UWORD ob_index;   // OB index if from an OB object, else 0
    Thing* veh;       // pointer to vehicle/Balrog Thing, or NULL
    UWORD mid_x;
    UWORD mid_y;
    UWORD mid_z;
    UWORD height;
    SWORD min_x;
    SWORD max_x;
    SWORD min_z;
    SWORD max_z;
    UWORD radius_or_yaw; // yaw for BBOX; radius for CYLINDER

} VEH_Col;

// uc_orig: VEH_MAX_COL (fallen/Headers/Vehicle.h)
#define VEH_MAX_COL 8

// Vehicle state machine update — called each tick for CLASS_VEHICLE Things.
// uc_orig: VEH_driving (fallen/Headers/Vehicle.h)
void VEH_driving(Thing*);

// Init all vehicle pool slots to free.
// uc_orig: init_vehicles (fallen/Headers/Vehicle.h)
void init_vehicles(void);

// Spawn a vehicle at world position (x,y,z) with given orientation and type.
// yaw is adjusted internally (+1024, vehicles use a different convention).
// uc_orig: VEH_create (fallen/Headers/Vehicle.h)
THING_INDEX VEH_create(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG yaw,
    SLONG pitch,
    SLONG roll,
    SLONG type,
    UBYTE key,
    UBYTE colour);

// Returns the body prim index for a given vehicle type.
// uc_orig: get_vehicle_body_prim (fallen/Headers/Vehicle.h)
UWORD get_vehicle_body_prim(SLONG type);

// Returns the Y offset (in large coords) of the body prim relative to vehicle origin.
// uc_orig: get_vehicle_body_offset (fallen/Headers/Vehicle.h)
SLONG get_vehicle_body_offset(SLONG type);

// Finds Things the vehicle is about to run over. Fills thing_index[] up to max_number.
// uc_orig: VEH_find_runover_things (fallen/Headers/Vehicle.h)
SLONG VEH_find_runover_things(Thing* p_vehicle, UWORD thing_index[], SLONG max_number, SLONG dangle);

// Returns the world-space X/Z position of the driver or passenger door.
// uc_orig: VEH_find_door (fallen/Headers/Vehicle.h)
void VEH_find_door(Thing* p_vehicle, SLONG i_am_a_passenger, SLONG* door_x, SLONG* door_z);

// Returns the driver Thing, or NULL if no driver.
// uc_orig: get_vehicle_driver (fallen/Headers/Vehicle.h)
Thing* get_vehicle_driver(Thing* p_vehicle);

// Initialises per-type crumple zone vertex assignments. Must be called after prims are loaded.
// uc_orig: VEH_init_vehinfo (fallen/Headers/Vehicle.h)
void VEH_init_vehinfo();

// Returns vertex-to-crumple-zone assignment array for a given body prim.
// uc_orig: VEH_get_assignments (fallen/Headers/Vehicle.h)
UBYTE* VEH_get_assignments(ULONG prim);

// Resets all dynamic vehicle state (velocities, health, damage). Called after teleport or creation.
// uc_orig: reinit_vehicle (fallen/Headers/Vehicle.h)
void reinit_vehicle(Thing* p_thing);

// Wheel position query — init for p_vehicle, then call vehicle_wheel_pos_get per wheel.
// uc_orig: vehicle_wheel_pos_init (fallen/Headers/Vehicle.h)
void vehicle_wheel_pos_init(Thing* p_vehicle);
// uc_orig: vehicle_wheel_pos_get (fallen/Headers/Vehicle.h)
void vehicle_wheel_pos_get(SLONG which, SLONG* wx, SLONG* wy, SLONG* wz);

// Builds the VEH_col[] collision candidate list for the given sphere.
// uc_orig: VEH_collide_find_things (fallen/Headers/Vehicle.h)
void VEH_collide_find_things(
    SLONG x, SLONG y, SLONG z,
    SLONG radius,
    SLONG ignore_thing_index,
    SLONG ignore_prims = UC_FALSE);

// Apply damage to a crumple zone (area 0-5, hp 0-2).
// uc_orig: VEH_add_damage (fallen/Source/Vehicle.cpp)
void VEH_add_damage(Vehicle* vp, UBYTE area, UBYTE hp);

// Apply upward impulse to one or two wheels on impact.
// uc_orig: VEH_bounce (fallen/Source/Vehicle.cpp)
void VEH_bounce(Vehicle* vp, UBYTE area, SLONG amount);

// Run-over HP calculation for a person hit by this vehicle.
// uc_orig: GetRunoverHP (fallen/Source/Vehicle.cpp)
SLONG GetRunoverHP(Thing* p_car, Thing* p_person);

// Throw a person out of a vehicle (eject on crash or rollover).
// uc_orig: VEH_throw_out_person (fallen/Source/Vehicle.cpp)
void VEH_throw_out_person(Thing* p_person, Thing* p_vehicle);

// Shake nearby fence objects when vehicle passes through.
// uc_orig: VEH_shake_fences (fallen/Source/Vehicle.cpp)
void VEH_shake_fences(SLONG mx, SLONG mz);

// Apply combined damage from two vehicles colliding.
// uc_orig: VEH_co_damage (fallen/Source/Vehicle.cpp)
void VEH_co_damage(Thing* v1, Thing* v2);

// Reduce vehicle health; triggers explosion at 0.
// Causer is the person who inflicted the damage (for murder/red-mark tracking), or NULL.
// uc_orig: VEH_reduce_health (fallen/Source/Vehicle.cpp)
void VEH_reduce_health(
    Thing* p_car,
    Thing* p_person,
    SLONG damage);

// Returns UC_TRUE if all 4 corners of the vehicle are on road tiles.
// uc_orig: VEH_on_road (fallen/Source/Vehicle.cpp)
SLONG VEH_on_road(Thing* p_vehicle, SLONG step);

// Get/set DX offsets for door animation (used when entering vehicle).
// uc_orig: get_car_door_offsets (fallen/Source/Vehicle.cpp)
void get_car_door_offsets(SLONG type, SLONG door, SLONG* dx, SLONG* dz);

// Update steering wheel angle based on input direction.
// uc_orig: steering_wheel (fallen/Source/Vehicle.cpp)
void steering_wheel(Vehicle* veh, SLONG velocity, bool player);

// Apply a nudge to the car position for collision response.
// uc_orig: nudge_car (fallen/Source/Vehicle.cpp)
void nudge_car(Thing* p_car, SLONG flags, SLONG* x, SLONG* z, SLONG neg);

// Draw the vehicle body and wheels for the current frame.
// uc_orig: draw_car (fallen/Source/Vehicle.cpp)
void draw_car(Thing* p_car);

#endif // ACTORS_VEHICLES_VEHICLE_H
