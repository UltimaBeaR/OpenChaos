#ifndef ACTORS_ANIMALS_LEAD_GLOBALS_H
#define ACTORS_ANIMALS_LEAD_GLOBALS_H

#include "core/types.h"
#include "actors/animals/lead.h"

// Global state for the dog-lead system.

// Active lead array.
// uc_orig: LEAD_lead (fallen/Headers/lead.h)
extern LEAD_Lead LEAD_lead[LEAD_MAX_LEADS];
// uc_orig: LEAD_lead_upto (fallen/Headers/lead.h)
extern SLONG LEAD_lead_upto;

// Active rope-point array.
// uc_orig: LEAD_point (fallen/Headers/lead.h)
extern LEAD_Point LEAD_point[LEAD_MAX_POINTS];
// uc_orig: LEAD_point_upto (fallen/Headers/lead.h)
extern SLONG LEAD_point_upto;

#endif // ACTORS_ANIMALS_LEAD_GLOBALS_H
