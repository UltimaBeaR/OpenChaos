#ifndef MISSIONS_MEMORY_GLOBALS_H
#define MISSIONS_MEMORY_GLOBALS_H

#include <stdint.h>
#include "engine/core/types.h"
// All level geometry pools (prim_points, prim_faces4, dfacets, etc.) live here:
#include "map/level_pools.h"
#include "missions/save.h" // MemTable, save_table[] (moved out of Game.h)

// Types declared in old memory.h that have not yet moved to their own subsystem headers.

// uc_orig: MAP_Beacon (fallen/Headers/memory.h)
typedef struct {
    UBYTE used;
    UBYTE counter;
    UWORD track_thing;
    UWORD index;
    UWORD pad;
    SLONG wx;
    SLONG wz;
    uint64_t ticks; // sdl3_get_ticks() timestamp (was ULONG/GetTickCount)
} MAP_Beacon;

// uc_orig: MAP_MAX_BEACONS (fallen/Headers/memory.h)
#define MAP_MAX_BEACONS 32

// uc_orig: PSX_TEX (fallen/Headers/memory.h)
typedef UWORD PSX_TEX[5];

// uc_orig: MAX_ROOF_FACE4 (fallen/Headers/memory.h)
#define MAX_ROOF_FACE4 10000

// ---- Globals from memory.cpp ----

// uc_orig: MAP_beacon (fallen/Source/memory.cpp)
extern MAP_Beacon* MAP_beacon;

// uc_orig: psx_textures_xy (fallen/Source/memory.cpp)
extern PSX_TEX* psx_textures_xy;

// Single flat memory block that backs all level arrays (save_table entries).
// uc_orig: mem_all (fallen/Source/memory.cpp)
extern void* mem_all;

// uc_orig: mem_all_size (fallen/Source/memory.cpp)
extern ULONG mem_all_size;

// Whether a quick save exists and can be loaded.
// uc_orig: MEMORY_quick_avaliable (fallen/Source/memory.cpp)
extern SLONG MEMORY_quick_avaliable;

// Central allocation table used by init_memory(), save_whole_game(), load_whole_game().
// Defined in memory_globals.cpp (also declared in fallen/Headers/Game.h — Temporary).
// uc_orig: save_table (fallen/Source/memory.cpp)
extern struct MemTable save_table[];

#endif // MISSIONS_MEMORY_GLOBALS_H
