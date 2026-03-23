#ifndef WORLD_MAP_PAP_H
#define WORLD_MAP_PAP_H

// Lower-resolution memory map (PAP = "Player Accessibility Point" or similar).
// Two-tier heightfield: 128x128 hi-res cells and 32x32 lo-res cells.
// Each hi-res cell covers 256 world units; each lo-res cell covers 1024 world units (4x4 hi-res).

#include "core/types.h"

struct Thing;

// uc_orig: PAP_SIZE_HI (fallen/Headers/pap.h)
// Hi-res map dimension: 128x128 cells, one cell = 256 world units.
#define PAP_SIZE_HI 128

// uc_orig: PAP_SIZE_LO (fallen/Headers/pap.h)
// Lo-res map dimension: 32x32 cells, one cell = 1024 world units.
#define PAP_SIZE_LO 32

// uc_orig: PAP_BLOCKS (fallen/Headers/pap.h)
// Number of hi-res cells per lo-res cell along one axis.
#define PAP_BLOCKS (PAP_SIZE_HI / PAP_SIZE_LO)

// Flags for the hi-res map cell (PAP_Hi.Flags, UWORD = 16 bits).
// Note: bit 9 and bit 14 are context-dependent (see ANIM_TMAP/ROOF_EXISTS and WANDER/FLAT_ROOF).

// uc_orig: PAP_FLAG_SHADOW_1 (fallen/Headers/pap.h)
#define PAP_FLAG_SHADOW_1 (1 << 0)
// uc_orig: PAP_FLAG_SHADOW_2 (fallen/Headers/pap.h)
#define PAP_FLAG_SHADOW_2 (1 << 1)
// uc_orig: PAP_FLAG_SHADOW_3 (fallen/Headers/pap.h)
#define PAP_FLAG_SHADOW_3 (1 << 2)
// uc_orig: PAP_FLAG_REFLECTIVE (fallen/Headers/pap.h)
#define PAP_FLAG_REFLECTIVE (1 << 3)
// uc_orig: PAP_FLAG_HIDDEN (fallen/Headers/pap.h)
#define PAP_FLAG_HIDDEN (1 << 4)
// uc_orig: PAP_FLAG_SINK_SQUARE (fallen/Headers/pap.h)
// Lowers the floor square to create a curb.
#define PAP_FLAG_SINK_SQUARE (1 << 5)
// uc_orig: PAP_FLAG_SINK_POINT (fallen/Headers/pap.h)
// Transform the point on the lower level.
#define PAP_FLAG_SINK_POINT (1 << 6)
// uc_orig: PAP_FLAG_NOUPPER (fallen/Headers/pap.h)
// Don't transform the point on the upper level.
#define PAP_FLAG_NOUPPER (1 << 7)
// uc_orig: PAP_FLAG_NOGO (fallen/Headers/pap.h)
// A square nobody is allowed onto.
#define PAP_FLAG_NOGO (1 << 8)
// uc_orig: PAP_FLAG_ANIM_TMAP (fallen/Headers/pap.h)
// Bit 9 context 1: animated texture on this square.
#define PAP_FLAG_ANIM_TMAP (1 << 9)
// uc_orig: PAP_FLAG_ROOF_EXISTS (fallen/Headers/pap.h)
// Bit 9 context 2: a roof exists above this hi-res square.
#define PAP_FLAG_ROOF_EXISTS (1 << 9)
// uc_orig: PAP_FLAG_ZONE1 (fallen/Headers/pap.h)
#define PAP_FLAG_ZONE1 (1 << 10)
// uc_orig: PAP_FLAG_ZONE2 (fallen/Headers/pap.h)
#define PAP_FLAG_ZONE2 (1 << 11)
// uc_orig: PAP_FLAG_ZONE3 (fallen/Headers/pap.h)
#define PAP_FLAG_ZONE3 (1 << 12)
// uc_orig: PAP_FLAG_ZONE4 (fallen/Headers/pap.h)
#define PAP_FLAG_ZONE4 (1 << 13)
// uc_orig: PAP_FLAG_WANDER (fallen/Headers/pap.h)
// Bit 14 context 1: AI can wander here.
#define PAP_FLAG_WANDER (1 << 14)
// uc_orig: PAP_FLAG_FLAT_ROOF (fallen/Headers/pap.h)
// Bit 14 context 2: roof is flat.
#define PAP_FLAG_FLAT_ROOF (1 << 14)
// uc_orig: PAP_FLAG_WATER (fallen/Headers/pap.h)
#define PAP_FLAG_WATER (1 << 15)

// Roof "hidden index" encoding: negative face indices in this range encode a hidden hi-res mapsquare.
// uc_orig: ROOF_HIDDEN_INDEX (fallen/Headers/pap.h)
#define ROOF_HIDDEN_INDEX (-10000)
// uc_orig: IS_ROOF_HIDDEN_FACE (fallen/Headers/pap.h)
#define IS_ROOF_HIDDEN_FACE(face) (((face) < ROOF_HIDDEN_INDEX) ? 1 : 0)
// uc_orig: ROOF_HIDDEN_X (fallen/Headers/pap.h)
#define ROOF_HIDDEN_X(face) (((-(face)) + ROOF_HIDDEN_INDEX) & 127)
// uc_orig: ROOF_HIDDEN_Z (fallen/Headers/pap.h)
#define ROOF_HIDDEN_Z(face) ((((-(face)) + ROOF_HIDDEN_INDEX) >> 7) & 127)
// uc_orig: ROOF_HIDDEN_GET_FACE (fallen/Headers/pap.h)
#define ROOF_HIDDEN_GET_FACE(x, z) (-((x) + (z) * 128) + ROOF_HIDDEN_INDEX)

// uc_orig: PAP_ALT_SHIFT (fallen/Headers/pap.h)
// Scale for PAP_Hi.Alt: real_height = Alt << PAP_ALT_SHIFT (i.e. Alt * 8).
#define PAP_ALT_SHIFT 3

// uc_orig: PAP_LO_NO_WATER (fallen/Headers/pap.h)
// Sentinel: no water in this lo-res square.
#define PAP_LO_NO_WATER (-127)

// uc_orig: PAP_LO_FLAG_WAREHOUSE (fallen/Headers/pap.h)
// Lo-res flag: this square has a warehouse (affects height calculation).
#define PAP_LO_FLAG_WAREHOUSE (1 << 0)

// uc_orig: PAP_Lo (fallen/Headers/pap.h)
// Lo-res map cell (32x32). Covers a 4x4 area of hi-res cells.
typedef struct
{
    UWORD MapWho;       // THING_INDEX of first thing in this lo-res cell (linked list)
    SWORD Walkable;     // Head of walkable face linked list: +ve = DFacet, -ve = roof quad
    UWORD ColVectHead;  // Head of collision vector linked list for this cell
    SBYTE water;        // Water height; PAP_LO_NO_WATER (-127) = no water
    UBYTE Flag;         // PAP_LO_FLAG_*

} PAP_Lo;

// uc_orig: PAP_Hi (fallen/Headers/pap.h)
// Hi-res map cell (128x128). 6 bytes.
typedef struct
{
    UWORD Texture;  // Floor texture index (3 spare bits)
    UWORD Flags;    // PAP_FLAG_* bitfield
    SBYTE Alt;      // Corner height; real_height = Alt << PAP_ALT_SHIFT
    SBYTE Height;   // Unused padding (reserved for future use)

} PAP_Hi;

// uc_orig: MEM_PAP_Lo (fallen/Headers/pap.h)
typedef PAP_Lo MEM_PAP_Lo[PAP_SIZE_LO];
// uc_orig: MEM_PAP_Hi (fallen/Headers/pap.h)
typedef PAP_Hi MEM_PAP_Hi[PAP_SIZE_HI];

// uc_orig: PAP_SHIFT_LO (fallen/Headers/pap.h)
// Shift to convert world coord to lo-res cell index: coord >> PAP_SHIFT_LO.
#define PAP_SHIFT_LO 10
// uc_orig: PAP_SHIFT_HI (fallen/Headers/pap.h)
// Shift to convert world coord to hi-res cell index: coord >> PAP_SHIFT_HI.
#define PAP_SHIFT_HI 8

// uc_orig: PAP_2LO (fallen/Headers/pap.h)
// Access lo-res cell at already-shifted indices (x >> PAP_SHIFT_LO, z >> PAP_SHIFT_LO).
#define PAP_2LO(x, z) (PAP_lo[(x)][(z)])
// uc_orig: PAP_2HI (fallen/Headers/pap.h)
// Access hi-res cell at already-shifted indices (x >> PAP_SHIFT_HI, z >> PAP_SHIFT_HI).
#define PAP_2HI(x, z) (PAP_hi[(x)][(z)])

// uc_orig: ON_PAP_LO (fallen/Headers/pap.h)
// Check if lo-res indices (x,z) are within map bounds.
#define ON_PAP_LO(x, z) ((x) >= 0 && (z) >= 0 && (x) < PAP_SIZE_LO && (z) < PAP_SIZE_LO)

// Returns whether the lo-res index is on the map.
// uc_orig: PAP_on_map_lo (fallen/Source/pap.cpp)
SLONG PAP_on_map_lo(SLONG x, SLONG z);

// Returns whether the hi-res index is on the map.
// uc_orig: PAP_on_map_hi (fallen/Source/pap.cpp)
SLONG PAP_on_map_hi(SLONG x, SLONG z);

// Clears all map data to zero.
// uc_orig: PAP_clear (fallen/Source/pap.cpp)
void PAP_clear(void);

// Returns the height at the corner of a hi-res cell. map_x, map_z are already-shifted indices.
// uc_orig: PAP_calc_height_at_point (fallen/Source/pap.cpp)
SLONG PAP_calc_height_at_point(SLONG map_x, SLONG map_z);

// Returns bilinearly interpolated terrain height at world coordinates (x, z).
// Returns -32767 if GF_NO_FLOOR, 0 if outside map bounds.
// uc_orig: PAP_calc_height_at (fallen/Source/pap.cpp)
SLONG PAP_calc_height_at(SLONG x, SLONG z);

// Returns terrain height at (x,z) adjusted for things like warehouses.
// uc_orig: PAP_calc_height_at_thing (fallen/Source/pap.cpp)
SLONG PAP_calc_height_at_thing(Thing* p_thing, SLONG x, SLONG z);

// Returns terrain height at (x,z) accounting for PAP_FLAG_HIDDEN (building interiors).
// uc_orig: PAP_calc_map_height_at (fallen/Source/pap.cpp)
SLONG PAP_calc_map_height_at(SLONG x, SLONG z);

// Returns terrain height ignoring road curbs and sink-square modifiers.
// uc_orig: PAP_calc_height_noroads (fallen/Source/pap.cpp)
SLONG PAP_calc_height_noroads(SLONG x, SLONG z);

// Returns the maximum terrain height among the four neighbors of (x,z), used for placement.
// uc_orig: PAP_calc_map_height_near (fallen/Source/pap.cpp)
SLONG PAP_calc_map_height_near(SLONG x, SLONG z);

// Returns true if the rectangular region is roughly flat (height difference < 16 units).
// uc_orig: PAP_is_flattish (fallen/Source/pap.cpp)
SLONG PAP_is_flattish(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// Returns slope magnitude at (x,z) and the slope direction angle in *angle.
// Used for walking and AI slope rejection.
// uc_orig: PAP_on_slope (fallen/Source/pap.cpp)
SLONG PAP_on_slope(SLONG x, SLONG z, SLONG* angle);

#endif // WORLD_MAP_PAP_H
