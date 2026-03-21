#ifndef WORLD_MAP_OB_GLOBALS_H
#define WORLD_MAP_OB_GLOBALS_H

#include "world/map/ob.h"

// All file-scope global state for the OB (static map object) system.

// Spatial lookup table: 2D grid of OB_Mapwho, heap-allocated in memory.cpp.
// uc_orig: OB_mapwho (fallen/Source/ob.cpp)
extern OB_workaround* OB_mapwho;

// Flat array of all placed objects, heap-allocated in memory.cpp.
// uc_orig: OB_ob (fallen/Source/ob.cpp)
extern OB_Ob* OB_ob;

// Next free slot in OB_ob[]; index 0 is reserved as a null sentinel.
// uc_orig: OB_ob_upto (fallen/Source/ob.cpp)
extern SLONG OB_ob_upto;

// Tracks damaged fire hydrants for the particle water spray in OB_process().
// uc_orig: OB_Hydrant (fallen/Source/ob.cpp)
typedef struct {
    UWORD life;     // remaining ticks; 0 = slot unused
    UWORD index;    // index into OB_ob[] for the hydrant
    UWORD x;        // world-space x (fine coord)
    UWORD z;
} OB_Hydrant;

// uc_orig: OB_MAX_HYDRANTS (fallen/Source/ob.cpp)
#define OB_MAX_HYDRANTS 4

// Scratch array returned by OB_find() — holds the current mapsquare's objects.
// uc_orig: OB_found (fallen/Source/ob.cpp)
extern OB_Info OB_found[OB_MAX_PER_SQUARE + 1];

// Persistent copy used by OB_find_index() to keep a result after the loop ends.
// uc_orig: OB_return (fallen/Source/ob.cpp)
extern OB_Info OB_return;

// Active fire hydrant slots.
// uc_orig: OB_hydrant (fallen/Source/ob.cpp)
extern OB_Hydrant OB_hydrant[OB_MAX_HYDRANTS];

// Ring cursor for hydrant slot allocation.
// uc_orig: OB_hydrant_last (fallen/Source/ob.cpp)
extern UBYTE OB_hydrant_last;

#endif // WORLD_MAP_OB_GLOBALS_H
