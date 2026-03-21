#ifndef ACTORS_VEHICLES_VEHICLE_GLOBALS_H
#define ACTORS_VEHICLES_VEHICLE_GLOBALS_H

// Global state for the vehicle system.

#include "core/types.h"
#include "actors/vehicles/vehicle.h"

// Per-type static vehicle parameters: wheel positions, engine tuning,
// prim indices, light positions, shadow size.
// uc_orig: VehInfo (fallen/Source/Vehicle.cpp)
struct VehInfo {
    SWORD DX[4], DZ[4];

    SBYTE FwdAccel;
    SBYTE BkAccel;
    SBYTE SoftBrake;
    SBYTE HardBrake;

    UWORD Reserved;
    SWORD BodyDy;
    UWORD WheelPrim;
    UWORD BodyPrim;
    SWORD BodyOffset;
    UWORD NumVertices;
    UBYTE* VertexAssignments;

    SWORD HLX, HLY, HLZ; // headlight position
    SWORD BLX, BLY, BLZ; // brakelight position (BLZ==0 = no brakelights)
    SWORD FLX, FLY, FLZ; // flashing light position (FLZ==0 = no flashing lights)
    SWORD FLRED;          // 1 = both lights red; 0 = red+blue

    UBYTE shad_size;
    UBYTE shad_elongate; // in 6-bit fixed point
};

// Master per-type data table, indexed by VEH_TYPE_*.
// uc_orig: veh_info (fallen/Source/Vehicle.cpp)
extern struct VehInfo veh_info[VEH_TYPE_NUMBER];

// Weighted list of civilian vehicle types for random NPC spawning (civilian types only).
// uc_orig: vehicle_random (fallen/Source/Vehicle.cpp)
extern UBYTE vehicle_random[];

// Collision candidate list filled by VEH_collide_find_things().
// uc_orig: VEH_col (fallen/Headers/Vehicle.h)
extern VEH_Col VEH_col[VEH_MAX_COL];

// Number of valid entries in VEH_col[].
// uc_orig: VEH_col_upto (fallen/Headers/Vehicle.h)
extern SLONG VEH_col_upto;

// Wheel position query state (set by vehicle_wheel_pos_init).
// uc_orig: vehicle_wheel_pos_vehicle (fallen/Source/Vehicle.cpp)
extern Thing* vehicle_wheel_pos_vehicle;

// 3x3 rotation matrix computed from vehicle yaw/tilt/roll each frame.
// uc_orig: car_matrix (fallen/Source/Vehicle.cpp)
extern SLONG car_matrix[9];

// Wheel-angle lookup table: maps wheel position → yaw offset (filled once by init_arctans).
// uc_orig: arctan_table (fallen/Source/Vehicle.cpp)
extern SLONG arctan_table[2 * WHEELTIME + 1];

// Set to 1 once arctan_table has been filled.
// uc_orig: arctan_table_ok (fallen/Source/Vehicle.cpp)
extern SLONG arctan_table_ok;

// State function dispatch table for CLASS_VEHICLE Things.
// uc_orig: VEH_statefunctions (fallen/Source/Vehicle.cpp)
extern StateFunction VEH_statefunctions[];

// When set, get_car_door_offsets() uses wider DX for enter-animation positioning.
// uc_orig: sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim (fallen/Source/Vehicle.cpp)
extern UBYTE sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim;

// When set, wall collision LOS checks are skipped in VEH_collide_find_things.
// uc_orig: VEH_collide_line_ignore_walls (fallen/Source/Vehicle.cpp)
extern SLONG VEH_collide_line_ignore_walls;

#endif // ACTORS_VEHICLES_VEHICLE_GLOBALS_H
