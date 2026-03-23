#ifndef ACTORS_ITEMS_SPECIAL_GLOBALS_H
#define ACTORS_ITEMS_SPECIAL_GLOBALS_H

#include "actors/items/special.h"
// Temporary: dirt.h needed for DIRT_Info in special_di declaration
#include "fallen/Headers/dirt.h"

// Master table: item type index -> display name, mesh prim ID, and group flag.
// uc_orig: SPECIAL_info (fallen/Source/Special.cpp)
extern SPECIAL_Info SPECIAL_info[SPECIAL_NUM_TYPES];

// Dead global from original; declared but never referenced anywhere.
// uc_orig: special_di (fallen/Source/Special.cpp)
extern DIRT_Info special_di;

#endif // ACTORS_ITEMS_SPECIAL_GLOBALS_H
