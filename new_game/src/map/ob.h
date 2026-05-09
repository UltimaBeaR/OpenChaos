#ifndef MAP_OB_H
#define MAP_OB_H

#include "engine/core/types.h"
#include "map/pap.h"

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
    UWORD index : 11; // first entry in OB_ob[] for this square
    UWORD num : 5; // number of entries on this square
} OB_Mapwho;

// Full info about one placed object, as returned by OB_find().
// Fields are expanded from the compressed OB_Ob storage format.
// uc_orig: OB_Info (fallen/Headers/ob.h)
typedef struct {
    UWORD prim; // prim ID; 0 = end-of-array sentinel
    UWORD x;
    SWORD y;
    UWORD z;
    UWORD yaw;
    UWORD pitch;
    UWORD roll;
    UWORD index; // index into OB_ob[]
    UBYTE crumple; // visual damage scale 0-4 (for non-lean prims)
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

// ---- Extended-objects fix (OpenChaos addition) ----
//
// Problem: anchor cell of an object is its only registration in OB_mapwho.
// Long objects (e.g. PRIM 215 garlands strung between buildings) have a
// mesh that physically extends across multiple lo-cells, but the object is
// only "alive" via its anchor cell. When the anchor cell falls out of the
// gamut (camera angle hides the wall it's anchored to), the entire object
// disappears even though most of its mesh is still on-screen above other,
// visible cells.
//
// Fix: at level load, compute each object's mesh bbox in world space. If
// it extends past the anchor lo-cell, register the object as "extended"
// in a parallel structure that lists (per-lo-cell) which extended objects
// reach into that cell. At render time, each frame's pre-pass picks one
// "responsible" lo-cell per extended object — the anchor cell if it's in
// the gamut (normal path draws it), or the first bbox cell that's in the
// gamut (extended path draws it). Guarantees one draw per object per
// frame, by construction.
//
// FUTURE OPTIMIZATION (TODO if perf becomes an issue — currently ~1-3ms
// on Steam Deck per frame, acceptable):
//
//   1) Cache `r_world` (bounding-sphere radius) and AABB-derived diagonal
//      directly inside OB_Extended at OB_extended_build time. Right now
//      the render-block cull recomputes sqrt(2·half_xz² + half_y²) every
//      cull test — but the value is fully static per object. One-shot
//      build-time computation removes the per-frame sqrt.
//
//   2) Compute the POLY_cam_matrix scale factor (max row magnitude, used
//      to convert world-space radius to view-space) ONCE per frame at the
//      top of the render-pass instead of re-deriving it per cull test.
//      Saves 9 muls + 2 adds + sqrt × every guest.
//
//   3) Distance early-out in OB_extended_pre_pass. The current pre-pass
//      iterates ALL of g_ob_extended[] and scans the full gamut for each
//      anchor-out-of-gamut entry, picking the closest gamut cell. For
//      objects on the far side of the map this is wasted work — they
//      can't be visible anyway. Skip cheaply: if the Manhattan/Chebyshev
//      distance from anchor lo to camera lo is bigger than something
//      like (AENG_DRAW_DIST + a few cells), don't even bother scanning
//      the gamut for them.
//
// Caveat for any future caching: OB_remove() can remove an object from
// OB_ob[]/OB_mapwho at runtime (e.g. dustbin→barrel conversion at level
// start, or ad-hoc removals). The cached g_ob_extended[] entry is NOT
// invalidated — its ob_idx points into the now-empty OB_ob slot. In
// practice this is rare and the current code reads through ext->prim/
// ext->x/y/z fields cached at build time so it doesn't crash, but a
// removed object will keep "rendering" via ext until level reload.
// If this ever becomes user-visible, fix by either (a) calling
// OB_extended_build() again after OB_remove(), or (b) checking
// OB_ob[ext->ob_idx].prim != 0 in pre-pass and skipping if zero.

// Maximum number of objects tracked by the extended system. Sized to
// match the global OB pool so every static object can be a candidate —
// including FITS_IN_CELL ones (mesh fully inside anchor cell). Needed so
// e.g. PRIM 215 garlands hung high above ground get drawn through ext
// even when their mesh AABB sits entirely in the anchor lo-cell:
// without this the camera frustum sees them in the air, but the gamut
// (a ground-plane projection) excludes the anchor cell and the object
// vanishes. Pre-pass picks per-frame whether anchor or guest cell
// renders.
#define OB_MAX_EXTENDED OB_MAX_OBS
// Maximum total cell-references. Per frame at most one ref per ext obj
// (the responsible cell), so this matches OB_MAX_EXTENDED.
#define OB_MAX_EXT_REFS OB_MAX_OBS

// Per-extended-object record. Built once at level load.
typedef struct {
    UWORD prim;
    UWORD ob_idx; // index into OB_ob[]
    SLONG x; // world position (full)
    SLONG y;
    SLONG z;
    UWORD yaw;
    UWORD pitch;
    UWORD roll;
    UBYTE crumple;
    UBYTE InsideIndex;
    UBYTE flags;
    UBYTE anchor_lo_x; // lo-cell of the anchor (where the object sits in OB_mapwho)
    UBYTE anchor_lo_z;
    UBYTE bbox_lo_x_min; // bounding box of mesh in lo-cells (inclusive)
    UBYTE bbox_lo_x_max;
    UBYTE bbox_lo_z_min;
    UBYTE bbox_lo_z_max;
    SLONG center_x; // geometric centre of the rotated mesh, world coords.
    SLONG center_y; // Used by the bbox frustum check in the render loop
    SLONG center_z; // and by the diagnostic dump.
    SLONG half_xz; // yaw-conservative half-extent in X/Z (world units)
    SLONG min_wy; // world Y range of the mesh (anchor.y + local min/max)
    SLONG max_wy;
    // Pre-pass output (per frame). responsible_valid != 0 means draw via the
    // extended path in the (responsible_lo_x, responsible_lo_z) cell. When
    // the anchor is in the gamut this stays 0 — the normal PRIM-loop draws.
    UBYTE responsible_lo_x;
    UBYTE responsible_lo_z;
    UBYTE responsible_valid;
    UBYTE drawn_this_frame; // set=1 after frustum check passes + MESH_draw_poly call
} OB_Extended;

// One reference from a lo-cell to an OB_Extended. Linked list per cell.
typedef struct {
    UWORD ext_idx; // 1-based index into g_ob_extended[]; 0 unused
    UWORD next_in_cell; // 1-based linked list; 0 = end
} OB_ExtRef;

// ---- Globals (defined in ob_globals.cpp) ----

// uc_orig: OB_mapwho (fallen/Source/ob.cpp)
extern OB_workaround* OB_mapwho;

// uc_orig: OB_ob (fallen/Source/ob.cpp)
extern OB_Ob* OB_ob;

// uc_orig: OB_ob_upto (fallen/Source/ob.cpp)
extern SLONG OB_ob_upto;

// Extended-objects globals (OpenChaos addition; see comment above for rationale).
extern OB_Extended g_ob_extended[OB_MAX_EXTENDED];
extern SLONG g_ob_extended_count;
extern OB_ExtRef g_ob_ext_refs[OB_MAX_EXT_REFS];
extern SLONG g_ob_ext_refs_count;
extern UWORD g_ob_ext_cell_first[OB_SIZE][OB_SIZE]; // 1-based head; 0 = empty

// How many extended objects were drawn this frame through the extended
// path (anchor outside gamut, drawn from a "guest" cell that IS inside
// the gamut). Reset by OB_extended_pre_pass(); incremented by the
// extended-draw block in AENG_draw_city after a successful frustum cull.
extern SLONG g_ob_ext_draws_this_frame;

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

// Called from elev.cpp and Game.cpp to set environment mapping on special objects.
// uc_orig: envmap_specials (fallen/Source/ob.cpp)
void envmap_specials(void);

// Build the extended-objects table from current OB_ob[]/OB_mapwho state.
// Call once after a level finishes loading.
void OB_extended_build(void);

// Per-frame pre-pass: for each extended object, decide whether the anchor
// path will draw it (anchor in gamut) or the extended path will (and pick
// which cell). Must be called after gamut is computed and before the main
// PRIM-loop runs.
void OB_extended_pre_pass(void);

#endif // MAP_OB_H
