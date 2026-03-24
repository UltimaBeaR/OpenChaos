#ifndef WORLD_ENVIRONMENT_TRIPWIRE_H
#define WORLD_ENVIRONMENT_TRIPWIRE_H

#include "engine/core/types.h"

// uc_orig: TRIP_X1_DEACTIVATED (fallen/Headers/trip.h)
#define TRIP_X1_DEACTIVATED 0xffff

// uc_orig: TRIP_Wire (fallen/Headers/trip.h)
typedef struct
{
    UBYTE along;
    UBYTE wait;
    UWORD counter;
    SWORD y;
    UWORD x1; // TRIP_X1_DEACTIVATED means the wire is inactive (not drawn or processed)
    UWORD z1;
    UWORD x2;
    UWORD z2;

} TRIP_Wire;

// uc_orig: TRIP_MAX_WIRES (fallen/Headers/trip.h)
#define TRIP_MAX_WIRES 32

// uc_orig: TRIP_Info (fallen/Headers/trip.h)
// Snapshot of a wire used for rendering (returned by TRIP_get_next).
typedef struct
{
    SLONG y;
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;
    UWORD counter;
    UBYTE along; // How far along the beam is blocked (0=start, 255=not triggered)
    UBYTE padding;

} TRIP_Info;

// uc_orig: TRIP_init (fallen/Headers/trip.h)
void TRIP_init(void);

// uc_orig: TRIP_create (fallen/Headers/trip.h)
// Creates a new tripwire from (x1,z1) to (x2,z2) at height y.
// Returns the wire index, or NULL if the pool is full.
// If an identical wire already exists, returns that index instead.
UBYTE TRIP_create(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: TRIP_process (fallen/Headers/trip.h)
void TRIP_process(void);

// uc_orig: TRIP_activated (fallen/Headers/trip.h)
SLONG TRIP_activated(UBYTE tripwire);

// uc_orig: TRIP_deactivate (fallen/Headers/trip.h)
void TRIP_deactivate(UBYTE tripwire);

// uc_orig: TRIP_get_start (fallen/Headers/trip.h)
void TRIP_get_start(void);

// uc_orig: TRIP_get_next (fallen/Headers/trip.h)
// Returns the next active wire's rendering info, or NULL when done.
TRIP_Info* TRIP_get_next(void);

#endif // WORLD_ENVIRONMENT_TRIPWIRE_H
