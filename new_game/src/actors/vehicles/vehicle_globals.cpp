// uc_orig: vehicle_globals.cpp (fallen/Source/Vehicle.cpp)
// Global state definitions for the vehicle system.

#include "fallen/Headers/Game.h"
#include "world/environment/prim_types.h" // PRIM_OBJ_* constants for veh_info initializer
#include "fallen/Headers/statedef.h"  // STATE_* constants for VEH_statefunctions
#include "actors/vehicles/vehicle.h"
#include "actors/vehicles/vehicle_globals.h"

// Shorthand macros for veh_info[] wheel-base and engine parameters.
// Expand to: DX[4] offsets (x axis), DZ[4] offsets (z axis).
// uc_orig: WHEELBASE_VAN (fallen/Source/Vehicle.cpp)
#define WHEELBASE_VAN { -120, 120, -120, 120 }, { 150, 150, -165, -165 }
// uc_orig: WHEELBASE_CAR (fallen/Source/Vehicle.cpp)
#define WHEELBASE_CAR { -85, 85, -85, 85 }, { 160, 160, -120, -120 }

// Expands to: FwdAccel, BkAccel, SoftBrake, HardBrake.
// uc_orig: ENGINE_LGV (fallen/Source/Vehicle.cpp)
#define ENGINE_LGV 17, 10, 4, 8
// uc_orig: ENGINE_CAR (fallen/Source/Vehicle.cpp)
#define ENGINE_CAR 21, 10, 4, 8
// uc_orig: ENGINE_PIG (fallen/Source/Vehicle.cpp)
#define ENGINE_PIG 25, 15, 5, 10
// uc_orig: ENGINE_AMB (fallen/Source/Vehicle.cpp)
#define ENGINE_AMB 25, 10, 5, 8

// Per-type vehicle parameters: row order matches VEH_TYPE_* constants.
// uc_orig: veh_info (fallen/Source/Vehicle.cpp)
struct VehInfo veh_info[VEH_TYPE_NUMBER] = {
    { WHEELBASE_VAN, ENGINE_LGV, 0, 30, PRIM_OBJ_VAN_WHEEL, PRIM_OBJ_VAN_BODY, 0x6800, 0, NULL, -248, 15, 90, 0, 0, 0, 0, 0, 0, 0, 185, 100 },
    { WHEELBASE_CAR, ENGINE_CAR, 0, 30, PRIM_OBJ_CAR_WHEEL, PRIM_OBJ_CAR_BODY, 0x4000, 0, NULL, -205, 0, 60, 235, -8, 60, 0, 0, 0, 0, 125, 160 },
    { WHEELBASE_CAR, ENGINE_CAR, 0, 30, PRIM_OBJ_CAR_WHEEL, PRIM_OBJ_TAXI_BODY, 0x4000, 0, NULL, -225, 0, 60, 245, -8, 60, 0, 0, 0, 0, 125, 160 },
    { WHEELBASE_CAR, ENGINE_PIG, 0, 30, PRIM_OBJ_CAR_WHEEL, PRIM_OBJ_POLICE_BODY, 0x4000, 0, NULL, -225, 0, 60, 205, -24, 60, 0, 70, 40, 0, 125, 160 },
    { WHEELBASE_VAN, ENGINE_AMB, 0, 30, PRIM_OBJ_VAN_WHEEL, PRIM_OBJ_AMBULANCE_BODY, 0x6800, 0, NULL, -248, 15, 90, 240, 15, 90, -40, 210, 50, 1, 185, 100 },
    { WHEELBASE_VAN, ENGINE_CAR, 0, 30, PRIM_OBJ_VAN_WHEEL, PRIM_OBJ_JEEP_BODY, 0x6800, 0, NULL, -225, -10, 80, 240, -5, 90, 0, 0, 0, 0, 185, 100 },
    { WHEELBASE_VAN, ENGINE_AMB, 0, 30, PRIM_OBJ_VAN_WHEEL, PRIM_OBJ_MEATWAGON_BODY, 0x6800, 0, NULL, -240, -10, 80, 240, 15, 90, 50, 140, 40, 0, 185, 100 },
    { WHEELBASE_CAR, ENGINE_PIG, 0, 30, PRIM_OBJ_CAR_WHEEL, PRIM_OBJ_SEDAN_BODY, 0x4000, 0, NULL, -225, -20, 60, 205, -24, 70, 0, 0, 0, 0, 125, 160 },
    { WHEELBASE_VAN, ENGINE_LGV, 0, 30, PRIM_OBJ_VAN_WHEEL, PRIM_OBJ_WILDCATVAN_BODY, 0x6800, 0, NULL, -248, 15, 90, 0, 0, 0, 0, 0, 0, 0, 185, 100 },
};

// Weighted list of civilian vehicle types for random NPC spawning.
// Police/Ambulance/etc. are intentionally absent — only civilian types spawn randomly.
// uc_orig: vehicle_random (fallen/Source/Vehicle.cpp)
UBYTE vehicle_random[] = {
    VEH_TYPE_VAN, VEH_TYPE_CAR, VEH_TYPE_TAXI, VEH_TYPE_JEEP,
    VEH_TYPE_SEDAN, VEH_TYPE_VAN, VEH_TYPE_CAR, VEH_TYPE_TAXI,
    VEH_TYPE_JEEP, VEH_TYPE_SEDAN, VEH_TYPE_VAN, VEH_TYPE_CAR,
    VEH_TYPE_TAXI, VEH_TYPE_VAN, VEH_TYPE_CAR, VEH_TYPE_TAXI
};

// Collision candidate list, filled by VEH_collide_find_things each tick.
// uc_orig: VEH_col (fallen/Headers/Vehicle.h)
VEH_Col VEH_col[VEH_MAX_COL];

// Number of valid entries in VEH_col[].
// uc_orig: VEH_col_upto (fallen/Headers/Vehicle.h)
SLONG VEH_col_upto;

// Bitmask set by CollideCar each frame; communicates hit-side results to VEH_driving.
// uc_orig: car_hit_flags (fallen/Source/Vehicle.cpp)
SLONG car_hit_flags;

// Wheel position query state — set by vehicle_wheel_pos_init, used by vehicle_wheel_pos_get.
// uc_orig: vehicle_wheel_pos_vehicle (fallen/Source/Vehicle.cpp)
Thing* vehicle_wheel_pos_vehicle;
// uc_orig: vehicle_wheel_pos_info (fallen/Source/Vehicle.cpp)
VehInfo* vehicle_wheel_pos_info;

// 3x3 rotation matrix computed from vehicle yaw/tilt/roll each frame.
// uc_orig: car_matrix (fallen/Source/Vehicle.cpp)
SLONG car_matrix[9];

// Wheel-angle lookup table: maps wheel position → yaw offset (filled once by init_arctans).
// uc_orig: arctan_table (fallen/Source/Vehicle.cpp)
SLONG arctan_table[2 * WHEELTIME + 1];

// Set to 1 once arctan_table has been filled.
// uc_orig: arctan_table_ok (fallen/Source/Vehicle.cpp)
SLONG arctan_table_ok = 0;

// State function dispatch table: maps state IDs to handler functions for CLASS_VEHICLE Things.
// uc_orig: VEH_statefunctions (fallen/Source/Vehicle.cpp)
StateFunction VEH_statefunctions[] = {
    { STATE_INIT, NULL },
    { STATE_NORMAL, NULL },
    { STATE_COLLISION, NULL },
    { STATE_ABOUT_TO_REMOVE, NULL },
    { STATE_REMOVE_ME, NULL },
    { STATE_MOVEING, NULL },
    { STATE_FDRIVING, VEH_driving },
    { STATE_FDOOR, NULL }
};

// When set, get_car_door_offsets() uses wider DX for enter-animation positioning.
// uc_orig: sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim (fallen/Source/Vehicle.cpp)
UBYTE sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim;

// When set, wall collision LOS checks are skipped in VEH_collide_find_things.
// uc_orig: VEH_collide_line_ignore_walls (fallen/Source/Vehicle.cpp)
SLONG VEH_collide_line_ignore_walls = 0;
