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
