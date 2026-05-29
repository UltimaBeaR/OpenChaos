#ifndef THINGS_VEHICLES_VEHICLE_H
#define THINGS_VEHICLES_VEHICLE_H

// Vehicle physics system — ground vehicles (CLASS_VEHICLE).
// Covers physics, collision, rendering, damage model, and enter/exit sequencing.

#include "engine/core/types.h"
#include "things/core/state.h"
#include "things/core/drawtype.h"

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
    UWORD Length; // current spring length
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
// OpenChaos: dedicated reverse (drive-backward) control. The original coupled
// brake and reverse on a single button (VEH_DECEL: brake at speed, reverse
// when slow/stopped). The OpenChaos control scheme splits them onto separate
// buttons — brake (Space / L1) stays VEH_DECEL, reverse (S / L2) gets this
// new bit. Not in the original.
#define VEH_REVERSE 16

// Speed magnitude (signed Velocity units) below which the car is treated as
// effectively stopped for direction decisions and stop detection. Matches the
// historical hard-coded threshold used in do_car_input's stop check.
// uc_orig: literal 10 in do_car_input (fallen/Source/Vehicle.cpp)
#define VEH_STOP_SPEED 10

// Forward-speed threshold separating "brake" from "reverse" in the brake/
// reverse pedal logic: above it the car is moving forward fast enough to brake;
// below it (and slow/stopped) the reverse key drives backwards instead.
// uc_orig: literal 200 in pedals (fallen/Source/Vehicle.cpp)
#define VEH_BRAKE_SPEED 200

// OpenChaos: friction shift drop for the brake key at LOW speed (no-skid).
// Smaller drop = higher friction value = gentler. The hard brake (with skid)
// keeps the original drop of 4. (friction starts at 7; new_vel = vel*(2^f-1)/2^f.)
#define VEH_SOFT_DECEL_FRICTION_DROP 3

// OpenChaos: deceleration curve for the THROTTLE opposing motion — reverse key
// slowing forward motion, gas key slowing a backward roll. Two INDEPENDENT
// knobs:
//   FRICTION_DROP: the percentage (∝ speed) term — controls how fast HIGH
//     speed bleeds off. Bigger drop = lower friction value = faster high-speed
//     slowdown. (friction starts 7; drop 3 -> f4 -> -1/16 of speed per tick.)
//   CONST: a flat per-tick decel on top — controls the firmness of the stop at
//     LOW speed (kills the long exponential tail). Bigger = quicker dead stop.
// Together: fast bleed when fast, firm stop when slow. Tune them separately to
// taste (high-speed feel = FRICTION_DROP, low-speed/stop feel = CONST).
#define VEH_THROTTLE_DECEL_FRICTION_DROP 3
#define VEH_THROTTLE_DECEL_CONST 15

// OpenChaos: when the reverse key brakes the car from forward motion to a full
// stop and the player keeps holding it, the car pauses this many physics ticks
// at zero before it starts driving backwards on its own. Releasing the key
// resets the pause (so release+re-press reverses immediately at zero speed).
// Symmetric for the gas key vs a backward roll. 5 ≈ 0.25 s at 20 Hz physics.
#define VEH_THROTTLE_HOLD_TICKS 5

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
    SLONG DY[4]; // vertical velocity per wheel point

    SLONG Angle; // yaw (0-2047)
    SLONG Tilt;
    SLONG Roll;

    SBYTE Steering; // -64..+64 or -1..+1
    UBYTE IsAnalog; // if set, steering uses full -64..+64 range
    UBYTE DControl; // VEH_ACCEL / VEH_DECEL / VEH_FASTER / VEH_SIREN bitmask
    UBYTE GrabAction; // action key was grabbed this frame

    SWORD Wheel; // steering wheel position
    UWORD Allocated; // 0 if slot is free

    UWORD Flags;
    UWORD Driver; // THING_INDEX of driver, 0 = none

    UWORD Passenger; // linked list of passengers
    UWORD Type; // VEH_TYPE_*

    UBYTE still; // ticks at zero velocity (for stability detection)
    UBYTE dlight; // dynamic night light handle
    UBYTE key; // key required to enter, or SPECIAL_NONE
    UBYTE Brakelight; // brakelight timer

    UBYTE damage[6]; // crumple zone damage (0=intact, 4=max)
    SWORD Spin; // wheel rotation accumulator

    SLONG VelX;
    SLONG VelY;
    SLONG VelZ;
    SLONG VelR; // rotational velocity

    SWORD WheelAngle;
    UBYTE Siren; // siren active
    UBYTE LastSoundState; // previous sound state for change detection

    SBYTE Dir; // +2=fwd, +1=fwd braking, -1/-2=reverse variants, 0=stopped
    UBYTE Skid; // 0=no skid; >=SKID_START=full skid

    // OpenChaos: "press twice" hold for the split reverse/gas keys. When the
    // reverse key brakes the car from forward motion down to a stop (or gas
    // brakes a backward roll to a stop), the throttle latches — holding it just
    // keeps the car stopped; the player must release and press again to drive
    // the other way. Implemented as an internal hold (NOT input-masking) so the
    // reverse key still fully overrides a held gas (input-masking would let the
    // gas leak through after the stop). Player-only; AI never latches (seamless
    // brake-then-reverse). Cleared the moment the key is released.
    UBYTE GasLatch; // gas held through a reverse->stop brake
    UBYTE RevLatch; // reverse held through a forward->stop brake
    UBYTE GasTimer; // ticks paused at zero before gas auto-drives forward
    UBYTE RevTimer; // ticks paused at zero before reverse auto-drives backward

    // OpenChaos: THING index of the person currently playing the ENTER
    // animation (0 = none). The enter reservation (FLAG_VEH_ANIMATING) is valid
    // only while this person is still actually entering; process_car releases
    // it the instant they stop (completed, killed, or any state change), so the
    // car can never be left permanently "occupied" by an interrupted animation.
    UWORD Animator;
    UBYTE Stable;
    UBYTE Smokin; // tyre smoke active

    UBYTE Scrapin; // scraping metal against wall
    UBYTE OnRoadFlags; // per-wheel on-road bitmask
    SWORD Health; // 300=full; 0=explode

    SLONG oldX[4], oldZ[4]; // previous wheel positions for skidmark rendering

    UBYTE LastSmokeSpawn; // wall-clock phase for engine smoke spawn edge-detect (FPS-independent)

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
    UWORD type; // VEH_COL_TYPE_BBOX or VEH_COL_TYPE_CYLINDER
    UWORD ob_index; // OB index if from an OB object, else 0
    Thing* veh; // pointer to vehicle/Balrog Thing, or NULL
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

// OpenChaos: world-space XZ teleport point for the vehicle-enter animation —
// front wheel of the door's side plus per-model hand-tuned offsets. See
// veh_enter_tune in vehicle_globals.h.
void get_car_enter_anim_xz(Thing* p_vehicle, SLONG door, SLONG* cx, SLONG* cz);

// OpenChaos: set the person's height for one physics tick of the enter
// animation — waits on the ground, then climbs to the door bottom
// (veh_enter_tune.upLeft/upRight) during a window of the animation. No-op when
// the side's up == 0.
void car_enter_anim_rise(Thing* p_person, Thing* p_vehicle);

// Update steering wheel angle based on input direction.
// uc_orig: steering_wheel (fallen/Source/Vehicle.cpp)
void steering_wheel(Vehicle* veh, SLONG velocity, bool player);

// Apply a nudge to the car position for collision response.
// uc_orig: nudge_car (fallen/Source/Vehicle.cpp)
void nudge_car(Thing* p_car, SLONG flags, SLONG* x, SLONG* z, SLONG neg);

// Draw the vehicle body and wheels for the current frame.
// uc_orig: draw_car (fallen/Source/Vehicle.cpp)
void draw_car(Thing* p_car);

#endif // THINGS_VEHICLES_VEHICLE_H
