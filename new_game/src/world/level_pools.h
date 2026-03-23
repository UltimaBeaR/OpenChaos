#ifndef WORLD_LEVEL_POOLS_H
#define WORLD_LEVEL_POOLS_H

// Level geometry pools: extern declarations for all large arrays allocated at level load time.
// These arrays are the primary storage for world geometry and building data. They are
// allocated as a single flat block by init_memory() and partitioned at load time.
//
// All types are defined in world/environment/prim_types.h and world/map/supermap.h.
// The actual allocations and definitions live in missions/memory.cpp (memory_globals.cpp).

#include "core/types.h"
#include "core/vector.h"                        // SVector
#include "world/environment/prim_types.h"       // PrimPoint, PrimFace3/4, PrimObject, PrimMultiObject, PrimNormal, PrimInfo, RoofFace4
#include "world/environment/building_types.h"   // BoundBox
#include "world/map/supermap.h"                 // DBuilding, DFacet, DWalkable, DStorey, DInsideRect
#include "world/navigation/inside2.h"           // InsideStorey, Staircase
#include "world/navigation/inside2_globals.h"   // next_inside_storey, etc.

// ---- Prim geometry pools (static world geometry: vertices and faces) ----

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
// uc_orig: prim_info (fallen/Source/Prim.cpp)
extern PrimInfo* prim_info;

// ---- Dynamic building geometry pools ----

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

// Facet connectivity: shared-edge links between dfacets[].
// uc_orig: facet_links (fallen/Source/memory.cpp)
extern SWORD* facet_links;

// ---- Roof face pool ----

// Roof face data (secondary pool for walkable roof geometry).
// uc_orig: roof_faces4 (fallen/Source/memory.cpp)
extern struct RoofFace4* roof_faces4;
// uc_orig: next_roof_face4 (fallen/Source/memory.cpp)
extern UWORD next_roof_face4;

// Bounding boxes for roof sections.
// uc_orig: roof_bounds (fallen/Source/memory.cpp)
extern struct BoundBox* roof_bounds;

// ---- Inside-building geometry pools ----

// uc_orig: inside_storeys (fallen/Source/memory.cpp)
extern struct InsideStorey* inside_storeys;
// uc_orig: inside_stairs (fallen/Source/memory.cpp)
extern struct Staircase* inside_stairs;
// uc_orig: inside_block (fallen/Source/memory.cpp)
extern UBYTE* inside_block;
// uc_orig: inside_tex (fallen/Source/memory.cpp)
extern UBYTE inside_tex[64][16];

// ---- Animation origin pool ----

// Anim mid-points: reference positions used to pack animation origin offsets.
// uc_orig: anim_mids (fallen/Source/memory.cpp)
extern struct PrimPoint* anim_mids;
// uc_orig: next_anim_mids (fallen/Source/memory.cpp)
extern ULONG next_anim_mids;

// ---- Darci normals ----

// Darci-specific compressed 15-bit normal lookup table.
// uc_orig: darci_normal (fallen/Source/memory.cpp)
extern UWORD* darci_normal;
// uc_orig: darci_normal_count (fallen/Source/memory.cpp)
extern UWORD darci_normal_count;

#endif // WORLD_LEVEL_POOLS_H
