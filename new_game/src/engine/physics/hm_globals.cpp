// uc_orig: hm_globals (fallen/Source/hm.cpp)
// Global variable definitions for the Hypermatter physics system.

#include "engine/physics/hm.h"
#include "engine/physics/hm_globals.h"

// uc_orig: HM_object (fallen/Source/hm.cpp)
HM_Object HM_object[HM_MAX_OBJECTS];


// uc_orig: HM_primgrid (fallen/Source/hm.cpp)
HM_Primgrid HM_primgrid[HM_MAX_PRIMGRIDS];

// uc_orig: HM_primgrid_upto (fallen/Source/hm.cpp)
SLONG HM_primgrid_upto;

// uc_orig: HM_default_primgrid (fallen/Source/hm.cpp)
// Fallback 2x2x2 config. z_dgrav = -0.25 gives mild forward-tipping tendency.
HM_Primgrid HM_default_primgrid = {
    0,
    2, 2, 2,
    { 0, 0x10000 },
    { 0, 0x10000 },
    { 0, 0x10000 },
    0.00F,
    0.00F,
    -0.25F
};
