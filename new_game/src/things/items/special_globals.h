#ifndef THINGS_ITEMS_SPECIAL_GLOBALS_H
#define THINGS_ITEMS_SPECIAL_GLOBALS_H

#include "things/items/special.h"
#include "effects/environment/dirt.h"

// Master table: item type index -> display name, mesh prim ID, and group flag.
// uc_orig: SPECIAL_info (fallen/Source/Special.cpp)
extern SPECIAL_Info SPECIAL_info[SPECIAL_NUM_TYPES];

// Dead global from original; declared but never referenced anywhere.
// uc_orig: special_di (fallen/Source/Special.cpp)
extern DIRT_Info special_di;

#endif // THINGS_ITEMS_SPECIAL_GLOBALS_H
