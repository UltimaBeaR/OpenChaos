#ifndef ENGINE_PHYSICS_HM_GLOBALS_H
#define ENGINE_PHYSICS_HM_GLOBALS_H

// uc_orig: hm_globals (fallen/Source/hm.cpp)
// External declarations of the global state for the Hypermatter physics system.

#include "engine/physics/hm.h"

// uc_orig: HM_object (fallen/Source/hm.cpp)
// Pool of 8 HM object slots. Each slot has a 'used' flag.
extern HM_Object HM_object[HM_MAX_OBJECTS];

// uc_orig: HM_object_upto (fallen/Source/hm.cpp)
// Unused counter (kept for binary compatibility — original never used it either).
extern SLONG HM_object_upto;

// uc_orig: HM_primgrid (fallen/Source/hm.cpp)
// Per-prim spring lattice config table, populated by HM_load().
extern HM_Primgrid HM_primgrid[HM_MAX_PRIMGRIDS];

// uc_orig: HM_primgrid_upto (fallen/Source/hm.cpp)
// Number of valid entries in HM_primgrid[].
extern SLONG HM_primgrid_upto;

// uc_orig: HM_default_primgrid (fallen/Source/hm.cpp)
// Fallback 2x2x2 lattice config used when no editor-defined grid exists.
extern HM_Primgrid HM_default_primgrid;

#endif // ENGINE_PHYSICS_HM_GLOBALS_H
