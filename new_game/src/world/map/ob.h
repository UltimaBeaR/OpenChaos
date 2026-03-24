#ifndef WORLD_MAP_OB_H
#define WORLD_MAP_OB_H

#include "engine/core/types.h"
#include "world/map/pap.h"

// Forward declaration — Thing is defined in fallen/Headers/Game.h (not yet migrated).
struct Thing;

// Static map objects (OBs) — decorative and interactive outdoor objects placed on the map
// grid (lamp posts, benches, hydrants, switches, weapon pickups, etc.).
// These are NOT Thing entities: they have no AI or physics, just visual + collision data.
// Exception: fire hydrants run a particle simulation in OB_process().
//
// Coordinates: worldPos>>10 = mapsquare index. OB_Ob stores subunit offsets (>>2) within cell.
// OB_find() expands back to fine coords (same scale as WorldPos>>8).

// ---- Constants ----

// uc_orig: OB_MAX_OBS (fallen/Headers/ob.h)
#define OB_MAX_OBS 2048

// uc_orig: OB_SIZE (fallen/Headers/ob.h)
#define OB_SIZE PAP_SIZE_LO

// uc_orig: OB_MAX_PER_SQUARE (fallen/Headers/ob.h)
#define OB_MAX_PER_SQUARE 31

// uc_orig: OB_FLAG_ON_FLOOR (fallen/Headers/ob.h)
#define OB_FLAG_ON_FLOOR (1 << 0)

// uc_orig: OB_FLAG_SEARCHABLE (fallen/Headers/ob.h)
#define OB_FLAG_SEARCHABLE (1 << 1)

// uc_orig: OB_FLAG_NOT_ON_PSX (fallen/Headers/ob.h)
#define OB_FLAG_NOT_ON_PSX (1 << 2)

// uc_orig: OB_FLAG_DAMAGED (fallen/Headers/ob.h)
#define OB_FLAG_DAMAGED (1 << 3)

// uc_orig: OB_FLAG_WAREHOUSE (fallen/Headers/ob.h)
#define OB_FLAG_WAREHOUSE (1 << 4)

// uc_orig: OB_FLAG_HIDDEN_ITEM (fallen/Headers/ob.h)
#define OB_FLAG_HIDDEN_ITEM (1 << 5)

// Bits 6-7 carry damage sub-type for lean/crumple computation.
// uc_orig: OB_FLAG_RESERVED1 (fallen/Headers/ob.h)
#define OB_FLAG_RESERVED1 (1 << 6)

// uc_orig: OB_FLAG_RESERVED2 (fallen/Headers/ob.h)
#define OB_FLAG_RESERVED2 (1 << 7)

// Special extended flags passed to OB_find_type() — values above 255 to distinguish
// from PRIM_FLAG_* bitmasks.
// uc_orig: FIND_OB_TRIPWIRE (fallen/Headers/ob.h)
#define FIND_OB_TRIPWIRE (1 << 9)

// uc_orig: FIND_OB_SWITCH_OR_VALVE (fallen/Headers/ob.h)
#define FIND_OB_SWITCH_OR_VALVE (1 << 10)

// ---- Structures ----

// Spatial index entry: maps one lo-res mapsquare to a contiguous slice of OB_ob[].
// uc_orig: OB_Mapwho (fallen/Headers/ob.h)
typedef struct {
    UWORD index : 11;   // first entry in OB_ob[] for this square
    UWORD num : 5;      // number of entries on this square
} OB_Mapwho;

// Full info about one placed object, as returned by OB_find().
// Fields are expanded from the compressed OB_Ob storage format.
// uc_orig: OB_Info (fallen/Headers/ob.h)
typedef struct {
    UWORD prim;         // prim ID; 0 = end-of-array sentinel
    UWORD x;
    SWORD y;
    UWORD z;
    UWORD yaw;
    UWORD pitch;
    UWORD roll;
    UWORD index;        // index into OB_ob[]
    UBYTE crumple;      // visual damage scale 0-4 (for non-lean prims)
    UBYTE InsideIndex;
    UBYTE Room;
    UBYTE flags;
} OB_Info;

// Compact per-object storage. x/z are subunit offsets (worldCoord>>2 & 0xff).
// yaw is compressed: stored>>3, so full yaw = oo->yaw << 3.
// uc_orig: OB_Ob (fallen/Headers/ob.h)
typedef struct {
    SWORD y;
    UBYTE x;
    UBYTE z;
    UBYTE prim;
    UBYTE yaw;
    UBYTE flags;
    UBYTE InsideIndex;
} OB_Ob;

// Array type for the 2D spatial index.
// uc_orig: OB_workaround (fallen/Headers/ob.h)
typedef OB_Mapwho OB_workaround[OB_SIZE];

// ---- Globals (defined in ob_globals.cpp) ----

// uc_orig: OB_mapwho (fallen/Source/ob.cpp)
extern OB_workaround* OB_mapwho;

// uc_orig: OB_ob (fallen/Source/ob.cpp)
extern OB_Ob* OB_ob;

// uc_orig: OB_ob_upto (fallen/Source/ob.cpp)
extern SLONG OB_ob_upto;

// ---- Functions ----

// uc_orig: OB_init (fallen/Source/ob.cpp)
void OB_init(void);

// uc_orig: OB_load_needed_prims (fallen/Source/ob.cpp)
void OB_load_needed_prims(void);

// uc_orig: OB_create (fallen/Source/ob.cpp)
void OB_create(SLONG x, SLONG y, SLONG z, SLONG yaw, SLONG pitch, SLONG roll,
               SLONG prim, UBYTE flag, UWORD Inside, UBYTE Room);

// Returns null-terminated array of OB_Info for all objects on mapsquare (x, z).
// uc_orig: OB_find (fallen/Source/ob.cpp)
OB_Info* OB_find(SLONG lo_map_x, SLONG lo_map_z);

// Like OB_find but filters by InsideIndex (room ID).
// uc_orig: OB_find_inside (fallen/Source/ob.cpp)
OB_Info* OB_find_inside(SLONG x, SLONG z, SLONG indoors);

// Search for the nearest OB matching prim_flags within max_range of (mid_x, mid_y, mid_z).
// prim_flags <= 255: PRIM_FLAG_* bitmask. prim_flags > 255: FIND_OB_* special type.
// uc_orig: OB_find_type (fallen/Source/ob.cpp)
SLONG OB_find_type(SLONG mid_x, SLONG mid_y, SLONG mid_z, SLONG max_range,
                   ULONG prim_flags,
                   SLONG* ob_x, SLONG* ob_y, SLONG* ob_z, SLONG* ob_yaw,
                   SLONG* ob_prim, SLONG* ob_index);

// uc_orig: OB_find_index (fallen/Source/ob.cpp)
OB_Info* OB_find_index(SLONG mid_x, SLONG mid_y, SLONG mid_z, SLONG max_range,
                       SLONG must_be_searchable);

// Avoidance helper: given movement (x1,z1)->(x2,z2), returns -1/+1 to steer around ob_prim.
// uc_orig: OB_avoid (fallen/Source/ob.cpp)
SLONG OB_avoid(SLONG ob_x, SLONG ob_y, SLONG ob_z, SLONG ob_yaw, SLONG ob_prim,
               SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: OB_remove (fallen/Source/ob.cpp)
void OB_remove(OB_Info* oi);

// uc_orig: OB_damage (fallen/Source/ob.cpp)
void OB_damage(SLONG index, SLONG from_dx, SLONG from_dz, SLONG ob_x, SLONG ob_z,
               Thing* p_aggressor);

// uc_orig: OB_inside_prim (fallen/Source/ob.cpp)
SLONG OB_inside_prim(SLONG x, SLONG y, SLONG z);

// Per-frame update for active fire hydrant particles.
// uc_orig: OB_process (fallen/Source/ob.cpp)
void OB_process(void);

// uc_orig: OB_convert_dustbins_to_barrels (fallen/Source/ob.cpp)
void OB_convert_dustbins_to_barrels(void);

// uc_orig: OB_make_all_the_switches_be_at_the_proper_height (fallen/Source/ob.cpp)
void OB_make_all_the_switches_be_at_the_proper_height(void);

// uc_orig: OB_add_walkable_faces (fallen/Source/ob.cpp)
void OB_add_walkable_faces(void);

// uc_orig: OB_find_min_y (fallen/Source/ob.cpp)
SLONG OB_find_min_y(SLONG prim);

// Called from elev.cpp and Game.cpp to set environment mapping on special objects.
// uc_orig: envmap_specials (fallen/Source/ob.cpp)
void envmap_specials(void);

#endif // WORLD_MAP_OB_H
