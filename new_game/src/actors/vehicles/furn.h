#ifndef ACTORS_VEHICLES_FURN_H
#define ACTORS_VEHICLES_FURN_H

#include "core/types.h"
#include "actors/core/state.h"

struct Thing;

// Flags stored in Furniture.Flags (and Vehicle.Flags for cars).
// uc_orig: FLAG_FURN_DRIVING (fallen/Headers/Furn.h)
#define FLAG_FURN_DRIVING (1 << 0)
// uc_orig: FLAG_FURN_WHEEL1_GRIP (fallen/Headers/Furn.h)
#define FLAG_FURN_WHEEL1_GRIP (1 << 1)
// uc_orig: FLAG_FURN_WHEEL2_GRIP (fallen/Headers/Furn.h)
#define FLAG_FURN_WHEEL2_GRIP (1 << 2)
// uc_orig: FLAG_FURN_WHEEL3_GRIP (fallen/Headers/Furn.h)
#define FLAG_FURN_WHEEL3_GRIP (1 << 3)
// uc_orig: FLAG_FURN_WHEEL4_GRIP (fallen/Headers/Furn.h)
#define FLAG_FURN_WHEEL4_GRIP (1 << 4)

// Per-instance dynamic state for a moving furniture object (doors, cars, crates, etc.).
// Static furniture has no Furniture struct allocated — Thing.Genus.Furniture is NULL.
// Allocated when the object starts moving, freed when it stops.
// uc_orig: Furniture (fallen/Headers/Furn.h)
typedef struct
{
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dyaw;
    SLONG dpitch;
    SLONG droll;

    SWORD Wheel;           // steering wheel position
    SWORD RAngle;
    SWORD OverSteer;
    SWORD DeltaOverSteer;
    SWORD Compression[4];  // suspension extension *4
    SWORD SpringDY[4];
    UWORD Flags;
    UWORD Driver;

    UWORD Command;
    UWORD Waypoint;

    UWORD closed_angle;    // door resting angle when closed
    UWORD ajar;            // door resting angle when ajar
} Furniture;

// uc_orig: FurniturePtr (fallen/Headers/Furn.h)
typedef Furniture* FurniturePtr;

// Maximum number of simultaneously moving furniture objects.
// uc_orig: MAX_FURNITURE (fallen/Headers/Furn.h)
#define MAX_FURNITURE 10

// State-machine function table for all furniture things (one entry shared).
// uc_orig: FURN_statefunctions (fallen/Headers/Furn.h)
extern StateFunction FURN_statefunctions[];

// uc_orig: init_furniture (fallen/Headers/Furn.h)
void init_furniture(void);

// uc_orig: free_furniture (fallen/Headers/Furn.h)
void free_furniture(Thing* furniture_thing);

// uc_orig: FURN_alloc_furniture (fallen/Headers/Furn.h)
Furniture* FURN_alloc_furniture(void);

// Creates a static furniture Thing at the given world position with the specified prim mesh.
// uc_orig: FURN_create (fallen/Headers/Furn.h)
THING_INDEX FURN_create(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG yaw,
    SLONG pitch,
    SLONG roll,
    SLONG prim);

// Creates a vehicle Thing at the given position.
// uc_orig: VEHICLE_create (fallen/Headers/Furn.h)
THING_INDEX VEHICLE_create(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG angle,
    SLONG prim);

// Turns a furniture Thing into a door with the given closed/ajar angles.
// uc_orig: FURN_turn_into_door (fallen/Headers/Furn.h)
void FURN_turn_into_door(
    THING_INDEX furniture_thing,
    UWORD closed_angle,
    UWORD ajar,
    UBYTE am_i_locked);

// Slides a movement vector against the furniture's bounding box, returning the
// deflected endpoint. Returns UC_TRUE if collision occurred and dont_slide was set.
// uc_orig: FURN_slide_along (fallen/Headers/Furn.h)
SLONG FURN_slide_along(
    THING_INDEX thing,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG radius,
    SLONG dont_slide);

// Returns +1 or -1 — steering direction to avoid the furniture.
// uc_orig: FURN_avoid (fallen/Headers/Furn.h)
SLONG FURN_avoid(
    THING_INDEX thing,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2);

// Applies hypermatter physics simulation to the furniture object.
// uc_orig: FURN_hypermatterise (fallen/Headers/Furn.h)
void FURN_hypermatterise(THING_INDEX thing);

// Applies an impulse force to the furniture from direction (x1,y1,z1)→(x2,y2,z2).
// uc_orig: FURN_push (fallen/Headers/Furn.h)
void FURN_push(
    THING_INDEX thing,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2);

// Scans all furniture Things for walkable faces and registers them in the walkable grid.
// uc_orig: FURN_add_walkable (fallen/Headers/Furn.h)
void FURN_add_walkable(void);

#endif // ACTORS_VEHICLES_FURN_H
