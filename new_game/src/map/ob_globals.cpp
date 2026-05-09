#include "map/ob_globals.h"

// These pointers are allocated in memory.cpp (MemAlloc calls during game init).

// uc_orig: OB_mapwho (fallen/Source/ob.cpp)
OB_workaround* OB_mapwho = nullptr;

// uc_orig: OB_ob (fallen/Source/ob.cpp)
OB_Ob* OB_ob = nullptr;

// uc_orig: OB_ob_upto (fallen/Source/ob.cpp)
SLONG OB_ob_upto = 1;

// uc_orig: OB_found (fallen/Source/ob.cpp)
OB_Info OB_found[OB_MAX_PER_SQUARE + 1];

// uc_orig: OB_return (fallen/Source/ob.cpp)
OB_Info OB_return;

// uc_orig: OB_hydrant (fallen/Source/ob.cpp)
OB_Hydrant OB_hydrant[OB_MAX_HYDRANTS];

// uc_orig: OB_hydrant_last (fallen/Source/ob.cpp)
UBYTE OB_hydrant_last = 0;

// Extended-objects fix: storage for per-object info, per-cell index
// references, and a per-cell linked-list head. Reset at level load via
// OB_extended_build() (which clears everything before re-populating).
OB_Extended g_ob_extended[OB_MAX_EXTENDED];
SLONG g_ob_extended_count = 0;
OB_ExtRef g_ob_ext_refs[OB_MAX_EXT_REFS];
SLONG g_ob_ext_refs_count = 0;
UWORD g_ob_ext_cell_first[OB_SIZE][OB_SIZE];
SLONG g_ob_ext_draws_this_frame = 0;
