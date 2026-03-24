#ifndef ASSETS_LEVEL_LOADER_GLOBALS_H
#define ASSETS_LEVEL_LOADER_GLOBALS_H

#include "engine/core/types.h"

// Snapshot of prim database cursors, saved by record_prim_status() for rollback.
// uc_orig: local_next_prim_point (fallen/Source/io.cpp)
extern UWORD local_next_prim_point;
// uc_orig: local_next_prim_face4 (fallen/Source/io.cpp)
extern UWORD local_next_prim_face4;
// uc_orig: local_next_prim_face3 (fallen/Source/io.cpp)
extern UWORD local_next_prim_face3;
// uc_orig: local_next_prim_object (fallen/Source/io.cpp)
extern UWORD local_next_prim_object;
// uc_orig: local_next_prim_multi_object (fallen/Source/io.cpp)
extern UWORD local_next_prim_multi_object;

#endif // ASSETS_LEVEL_LOADER_GLOBALS_H
