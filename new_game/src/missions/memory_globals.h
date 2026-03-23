#ifndef MISSIONS_MEMORY_GLOBALS_H
#define MISSIONS_MEMORY_GLOBALS_H

#include "core/types.h"
#include "fallen/Headers/structs.h"   // Temporary: SVector (needed by building.h for PrimNormal typedef)
#include "world/map/supermap.h"
#include "world/navigation/inside2.h"
#include "world/navigation/inside2_globals.h"
#include "fallen/Headers/building.h"  // Temporary: BoundBox, PrimPoint, PrimFace4, PrimFace3, PrimObject, PrimMultiObject, PrimNormal, RMAX_PRIM_POINTS, RMAX_PRIM_FACES4, MAX_PRIM_FACES3, MAX_PRIM_OBJECTS, MAX_PRIM_MOBJECTS, MAX_PRIM_POINTS, RoofFace4, MAX_ROOF_BOUND
#include "fallen/Headers/Prim.h"      // Temporary: PrimInfo
#include "fallen/Headers/Game.h"      // Temporary: MemTable (needed for save_table[] extern declaration)

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
    ULONG ticks;
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

// uc_orig: psx_remap (fallen/Source/memory.cpp)
extern UWORD* psx_remap;

// Facet links: connectivity between facets (shared edge links).
// uc_orig: facet_links (fallen/Source/memory.cpp)
extern SWORD* facet_links;

// Dynamic building geometry arrays (allocated at level load time).
// uc_orig: dbuildings (fallen/Source/memory.cpp)
extern struct DBuilding* dbuildings;
// uc_orig: dfacets (fallen/Source/memory.cpp)
extern struct DFacet* dfacets;
// uc_orig: dwalkables (fallen/Source/memory.cpp)
extern struct DWalkable* dwalkables;
// uc_orig: dstyles (fallen/Source/memory.cpp)
extern SWORD* dstyles;
// uc_orig: dstoreys (fallen/Source/memory.cpp)
extern struct DStorey* dstoreys;

// Paint data for textured buildings.
// uc_orig: paint_mem (fallen/Source/memory.cpp)
extern UBYTE* paint_mem;

// Anim mid-points: a small table of reference positions used to pack animation origin offsets.
// Each entry is a world-space position that animations can be expressed relative to.
// uc_orig: anim_mids (fallen/Source/memory.cpp)
extern struct PrimPoint* anim_mids;

// uc_orig: next_anim_mids (fallen/Source/memory.cpp)
extern ULONG next_anim_mids;

// Inside-building geometry arrays.
// uc_orig: inside_storeys (fallen/Source/memory.cpp)
extern struct InsideStorey* inside_storeys;
// uc_orig: inside_stairs (fallen/Source/memory.cpp)
extern struct Staircase* inside_stairs;
// uc_orig: inside_block (fallen/Source/memory.cpp)
extern UBYTE* inside_block;
// uc_orig: inside_tex (fallen/Source/memory.cpp)
extern UBYTE inside_tex[64][16];

// Bounding boxes for roof sections.
// uc_orig: roof_bounds (fallen/Source/memory.cpp)
extern struct BoundBox* roof_bounds;

// Prim geometry arrays — the vertex and face pools for all static world geometry.
// uc_orig: prim_points (fallen/Source/memory.cpp)
extern struct PrimPoint* prim_points;
// uc_orig: prim_faces4 (fallen/Source/memory.cpp)
extern struct PrimFace4* prim_faces4;
// uc_orig: prim_faces3 (fallen/Source/memory.cpp)
extern struct PrimFace3* prim_faces3;
// uc_orig: prim_objects (fallen/Source/memory.cpp)
extern struct PrimObject* prim_objects;
// uc_orig: prim_multi_objects (fallen/Source/memory.cpp)
extern struct PrimMultiObject* prim_multi_objects;

// Vertex normals parallel to prim_points[].
// uc_orig: prim_normal (fallen/Source/memory.cpp)
extern PrimNormal* prim_normal;

// Per-prim metadata computed from geometry at load time.
// Defined in Prim.cpp (not yet migrated); declared here because it is referenced
// by save_table[] and many geometry systems that include memory_globals.h.
// uc_orig: prim_info (fallen/Source/Prim.cpp)
extern PrimInfo* prim_info;

// Roof face data (a secondary pool for walkable roof geometry).
// uc_orig: next_roof_face4 (fallen/Source/memory.cpp)
extern UWORD next_roof_face4;
// uc_orig: roof_faces4 (fallen/Source/memory.cpp)
extern struct RoofFace4* roof_faces4;

// Darci-specific normal lookup table (packed 15-bit normals).
// uc_orig: darci_normal (fallen/Source/memory.cpp)
extern UWORD* darci_normal;
// uc_orig: darci_normal_count (fallen/Source/memory.cpp)
extern UWORD darci_normal_count;

// Whether a quick save exists and can be loaded.
// uc_orig: MEMORY_quick_avaliable (fallen/Source/memory.cpp)
extern SLONG MEMORY_quick_avaliable;

// Central allocation table used by init_memory(), save_whole_game(), load_whole_game().
// Defined in memory_globals.cpp (also declared in fallen/Headers/Game.h — Temporary).
// uc_orig: save_table (fallen/Source/memory.cpp)
extern struct MemTable save_table[];

#endif // MISSIONS_MEMORY_GLOBALS_H
