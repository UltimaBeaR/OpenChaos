#ifndef ACTORS_ANIMALS_LEAD_H
#define ACTORS_ANIMALS_LEAD_H

#include "core/types.h"

// Dog-lead system. Each lead connects a world attachment point to a Thing (the dog).
// The rope is simulated as a chain of LEAD_Point particles; LEAD_process() runs Verlet integration.
// Global state (arrays and counters) is in lead_globals.h.

// ---- Lead structure ----

// uc_orig: LEAD_Lead (fallen/Headers/lead.h)
typedef struct
{
    UBYTE p_num;        // Number of rope points
    UBYTE p_index;      // Index into the LEAD_point array (first point of this lead)
    UWORD attach_x;     // World attachment point X
    UWORD attach_y;
    UWORD attach_z;
    UWORD attach_thing; // Thing index on the other end of the lead

} LEAD_Lead;

// uc_orig: LEAD_MAX_LEADS (fallen/Headers/lead.h)
#define LEAD_MAX_LEADS 32

// ---- Rope point structure ----

// uc_orig: LEAD_Point (fallen/Headers/lead.h)
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    SLONG dx;   // Velocity X
    SLONG dy;   // Velocity Y
    SLONG dz;   // Velocity Z

} LEAD_Point;

// uc_orig: LEAD_MAX_POINTS (fallen/Headers/lead.h)
#define LEAD_MAX_POINTS 256

// ---- Lead length constants ----

// uc_orig: LEAD_LEN_SHORT (fallen/Headers/lead.h)
#define LEAD_LEN_SHORT  1
// uc_orig: LEAD_LEN_MEDIUM (fallen/Headers/lead.h)
#define LEAD_LEN_MEDIUM 2
// uc_orig: LEAD_LEN_LONG (fallen/Headers/lead.h)
#define LEAD_LEN_LONG   3

// ---- Functions ----

// Removes all leads and resets the arrays.
// uc_orig: LEAD_init (fallen/Headers/lead.h)
void LEAD_init(void);

// Creates a new lead of the given length, attached at the world point (x, y, z).
// uc_orig: LEAD_create (fallen/Headers/lead.h)
void LEAD_create(SLONG len, SLONG world_x, SLONG world_y, SLONG world_z);

// Scans nearby lamp posts and dogs, attaches leads to them.
// uc_orig: LEAD_attach (fallen/Headers/lead.h)
void LEAD_attach(void);

// Simulates all active leads (rope physics).
// uc_orig: LEAD_process (fallen/Headers/lead.h)
void LEAD_process(void);

#endif // ACTORS_ANIMALS_LEAD_H
