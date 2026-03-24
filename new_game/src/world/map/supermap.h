#ifndef WORLD_MAP_SUPERMAP_H
#define WORLD_MAP_SUPERMAP_H

#include "engine/core/types.h"
#include "engine/platform/uc_common.h"                          // MFFileHandle (used in load_super_map / save_super_map)
#include "world/environment/building_types.h"  // STOREY_TYPE_*, FACET_FLAG_*, FBuilding, FStorey, etc.
// Global state: allocation cursors, level list.
#include "world/map/supermap_globals.h"

// Supermap: pre-computed spatial index of all buildings, facets, and walkables.
// Loaded from the binary level file (.pam) via load_super_map().

// ---- Structures ----

// uc_orig: DStorey (fallen/Headers/supermap.h)
struct DStorey {
    UWORD Style;    // Replacement style index
    UWORD Index;    // Index into painted info
    SBYTE Count;    // Positive = style, negative = (unused)
    UBYTE BloodyPadding;
};

// uc_orig: DFacet (fallen/Headers/supermap.h)
struct DFacet {
    UBYTE FacetType;
    UBYTE Height;
    UBYTE x[2];         // Grid-based X coordinates (byte precision)
    SWORD Y[2];
    UBYTE z[2];         // Grid-based Z coordinates (byte precision)
    UWORD FacetFlags;
    UWORD StyleIndex;
    UWORD Building;
    UWORD DStorey;
    UBYTE FHeight;
    UBYTE BlockHeight;
    UBYTE Open;         // How open/closed a STOREY_TYPE_OUTSIDE_DOOR is
    UBYTE Dfcache;      // Index into NIGHT_dfcache[] or NULL
    UBYTE Shake;        // Set when a fence has been hit hard by something
    UBYTE CutHole;
    UBYTE Counter[2];
};

// uc_orig: DBuilding (fallen/Headers/supermap.h)
struct DBuilding {
    SLONG X, Y, Z;
    UWORD StartFacet;
    UWORD EndFacet;
    UWORD Walkable;
    UBYTE Counter[2];
    UWORD Padding;
    UBYTE Ware;     // If this building is a warehouse, index into WARE_ware[]
    UBYTE Type;
};

// uc_orig: DWalkable (fallen/Headers/supermap.h)
struct DWalkable {
    UWORD StartPoint;   // Unused
    UWORD EndPoint;     // Unused
    UWORD StartFace3;   // Unused
    UWORD EndFace3;     // Unused

    UWORD StartFace4;   // Indices into the roof faces
    UWORD EndFace4;

    UBYTE X1;
    UBYTE Z1;
    UBYTE X2;
    UBYTE Z2;
    UBYTE Y;
    UBYTE StoreyY;
    UWORD Next;
    UWORD Building;
};

// uc_orig: DInsideRect (fallen/Headers/supermap.h)
struct DInsideRect {
    UBYTE MapX;
    UBYTE MapZ;
    UBYTE Width;
    UBYTE Depth;
    UBYTE StoreyY;
    UBYTE Flags;        // Flags; also pads the struct to a nice size
    UWORD BitIndex;     // Index into block of data for inside buildings
};

// ---- Array limits ----

// uc_orig: MAX_FACET_LINK (fallen/Headers/supermap.h)
#define MAX_FACET_LINK 32000

// uc_orig: MAX_DBUILDINGS (fallen/Headers/supermap.h)
#define MAX_DBUILDINGS 1024
// uc_orig: MAX_DFACETS (fallen/Headers/supermap.h)
#define MAX_DFACETS 16384
// uc_orig: MAX_DWALKABLES (fallen/Headers/supermap.h)
#define MAX_DWALKABLES 2048
// uc_orig: MAX_DSTYLES (fallen/Headers/supermap.h)
#define MAX_DSTYLES 10000
// uc_orig: MAX_DSTOREYS (fallen/Headers/supermap.h)
#define MAX_DSTOREYS 10000
// uc_orig: MAX_PAINTMEM (fallen/Headers/supermap.h)
#define MAX_PAINTMEM 64000

// ---- Inside-room bit-packing macros ----
// Each inside ID byte packs: bits 0-3 = room ID (16 rooms), bits 4-5 = direction, bits 6-7 = type.

// uc_orig: CALC_INSIDE_ID (fallen/Headers/supermap.h)
#define CALC_INSIDE_ID(id) (id & 3)
// uc_orig: CALC_INSIDE_DIRECTION (fallen/Headers/supermap.h)
#define CALC_INSIDE_DIRECTION(id) ((id & 3) << 4)
// uc_orig: CALC_INSIDE_TYPE (fallen/Headers/supermap.h)
#define CALC_INSIDE_TYPE(id) ((id & 3) << 6)

// uc_orig: GET_INSIDE_ID (fallen/Headers/supermap.h)
#define GET_INSIDE_ID(id) (id & 3)
// uc_orig: GET_INSIDE_DIRECTION (fallen/Headers/supermap.h)
#define GET_INSIDE_DIRECTION(id) ((id >> 4) & 3)
// uc_orig: GET_INSIDE_TYPE (fallen/Headers/supermap.h)
#define GET_INSIDE_TYPE(id) ((id >> 6) & 3)

// ---- Functions ----

// uc_orig: load_super_map (fallen/Source/supermap.cpp)
void load_super_map(MFFileHandle handle, SLONG save_type);

// uc_orig: add_sewer_ladder (fallen/Source/supermap.cpp)
void add_sewer_ladder(SLONG x1, SLONG z1, SLONG x2, SLONG z2,
                      SLONG bottom, SLONG height, SLONG link);

// uc_orig: find_electric_fence_dbuilding (fallen/Source/supermap.cpp)
SLONG find_electric_fence_dbuilding(SLONG world_x, SLONG world_y, SLONG world_z, SLONG range);

// uc_orig: set_electric_fence_state (fallen/Source/supermap.cpp)
void set_electric_fence_state(SLONG dbuilding, SLONG onoroff);

// uc_orig: SUPERMAP_counter_increase (fallen/Source/supermap.cpp)
void SUPERMAP_counter_increase(UBYTE which);

// Returns level index for mission name, or 0 if not found. Sets DONT_load bitmask.
// uc_orig: get_level_no (fallen/Source/supermap.cpp)
SLONG get_level_no(CBYTE* name);

// Returns InsideStorey index (1-based) for world point (x,y,z), or 0 if outdoors.
// Also fills *room with the room ID within that InsideStorey.
// uc_orig: calc_inside_for_xyz (fallen/Source/supermap.cpp)
UWORD calc_inside_for_xyz(SLONG x, SLONG y, SLONG z, UWORD* room);

// Dev tool: iterates all levels, generates texture clump files, then exits.
// Called from game_startup() when TEXTURE_create_clump mode is active.
// uc_orig: make_all_clumps (fallen/Source/supermap.cpp)
void make_all_clumps(void);

// Serialises the current supermap to a file handle (editor only).
// uc_orig: save_super_map (fallen/Source/supermap.cpp)
void save_super_map(MFFileHandle handle);

// Editor helper: converts a single building into the supermap DBuilding[1] slot
// so ENTER/STAIR/ID modules can operate on editor buildings.
// uc_orig: create_super_dbuilding (fallen/Source/supermap.cpp)
void create_super_dbuilding(SLONG building);

#endif // WORLD_MAP_SUPERMAP_H
