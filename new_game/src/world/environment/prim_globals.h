#ifndef WORLD_ENVIRONMENT_PRIM_GLOBALS_H
#define WORLD_ENVIRONMENT_PRIM_GLOBALS_H

#include "engine/core/types.h"
#include "engine/core/vector.h"                        // SVector
#include "world/environment/prim_types.h"       // PrimFace3/4, PrimObject, PrimPoint, PrimMultiObject, MAX_PRIM_OBJECTS

// ---- Internal constants ----

// Minimum bounding box dimension for reliable collision response.
// uc_orig: PRIM_MIN_BBOX (fallen/Source/Prim.cpp)
#define PRIM_MIN_BBOX 0x58

// Maximum points in a single prim object (used as scratch buffer size for normal calc).
// uc_orig: MAX_POINTS_PER_PRIM (fallen/Source/Prim.cpp)
#define MAX_POINTS_PER_PRIM 5000

// Maximum number of anim-prim Things collected by find_anim_prim sphere search.
// uc_orig: MAX_FIND_ANIM_PRIMS (fallen/Source/Prim.cpp)
#define MAX_FIND_ANIM_PRIMS 8

// ---- Globals ----

// Per-prim runtime metadata computed from geometry at load time.
// Originally defined in Prim.cpp; moved here per _globals rule.
// uc_orig: prim_info (fallen/Source/Prim.cpp)
// Note: PrimInfo* prim_info is also extern-declared in missions/memory_globals.h
// (needed by save_table[]). Both declarations refer to the same variable.
// PrimInfo is declared in world/environment/prim_types.h (included above via prim_types.h)
extern PrimInfo* prim_info;

// Names for each prim object slot (editor labels, not used at runtime).
// uc_orig: prim_names (fallen/Source/Prim.cpp)
extern CBYTE prim_names[MAX_PRIM_OBJECTS][32];

// Transformed vertex output buffer. Shared scratch space used by the DDEngine
// rendering layer to store projected screen-space coordinates.
// 15560 = max vertices across all prims in a single draw call.
// uc_orig: global_res (fallen/Source/Prim.cpp)
extern struct SVector global_res[15560];

// Per-vertex clip flags output buffer (parallel to global_res[]).
// uc_orig: global_flags (fallen/Source/Prim.cpp)
extern SLONG global_flags[15560];

// Per-vertex brightness output buffer (parallel to global_res[]).
// uc_orig: global_bright (fallen/Source/Prim.cpp)
extern UWORD global_bright[15560];

// Temporary scratch buffer for the per-vertex normal averaging pass.
// Each element counts how many faces reference the corresponding prim point.
// uc_orig: each_point (fallen/Source/Prim.cpp)
extern UBYTE each_point[MAX_POINTS_PER_PRIM];

// Result buffer for find_anim_prim() sphere search.
// uc_orig: found_aprim (fallen/Source/Prim.cpp)
extern UWORD found_aprim[MAX_FIND_ANIM_PRIMS];

#endif // WORLD_ENVIRONMENT_PRIM_GLOBALS_H
