#ifndef UNDERGROUND_NS_GLOBALS_H
#define UNDERGROUND_NS_GLOBALS_H

// Global state for the sewer (NS) subsystem.
// Includes the map arrays, scratch buffers, and per-cell light definitions.

#include "underground/ns.h"
#include "engine/core/heap.h"

// === Map arrays ===

// uc_orig: NS_texture (fallen/Source/ns.cpp)
extern NS_Texture NS_texture[NS_MAX_TEXTURES];
// uc_orig: NS_texture_upto (fallen/Source/ns.cpp)
extern SLONG NS_texture_upto;

// uc_orig: NS_page (fallen/Source/ns.cpp)
extern NS_Page NS_page[NS_PAGE_NUMBER];

// uc_orig: NS_lo (fallen/Source/ns.cpp)
extern NS_Lo NS_lo[PAP_SIZE_LO][PAP_SIZE_LO];
// uc_orig: NS_hi (fallen/Source/ns.cpp)
extern NS_Hi NS_hi[PAP_SIZE_HI][PAP_SIZE_HI];

// uc_orig: NS_cache (fallen/Source/ns.cpp)
extern NS_Cache NS_cache[NS_MAX_CACHES];
// uc_orig: NS_cache_free (fallen/Source/ns.cpp)
extern UBYTE NS_cache_free;

// uc_orig: NS_fall (fallen/Source/ns.cpp)
extern NS_Fall NS_fall[NS_MAX_FALLS];
// uc_orig: NS_fall_free (fallen/Source/ns.cpp)
extern UBYTE NS_fall_free;

// uc_orig: NS_st (fallen/Source/ns.cpp)
extern NS_St NS_st[NS_MAX_STS];
// uc_orig: NS_st_free (fallen/Source/ns.cpp)
extern UBYTE NS_st_free;

// === Scratch buffer pointers (into HEAP_pad) ===

// uc_orig: NS_scratch_point (fallen/Source/ns.cpp)
extern NS_Point* NS_scratch_point;
// uc_orig: NS_scratch_face (fallen/Source/ns.cpp)
extern NS_Face* NS_scratch_face;

// uc_orig: NS_scratch_point_upto (fallen/Source/ns.cpp)
extern SLONG NS_scratch_point_upto;
// uc_orig: NS_scratch_face_upto (fallen/Source/ns.cpp)
extern SLONG NS_scratch_face_upto;

// World origin of the scratch buffer (lo-res mapsquare bottom-left corner).
// uc_orig: NS_scratch_origin_x (fallen/Source/ns.cpp)
extern SLONG NS_scratch_origin_x;
// uc_orig: NS_scratch_origin_z (fallen/Source/ns.cpp)
extern SLONG NS_scratch_origin_z;

// === Per-square lights used when computing vertex brightness ===

// Internal light source for vertex lighting in sewer geometry.
// uc_orig: NS_Slight (fallen/Source/ns.cpp)
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
} NS_Slight;

// uc_orig: NS_MAX_SLIGHTS (fallen/Source/ns.cpp)
#define NS_MAX_SLIGHTS 9

// uc_orig: NS_slight (fallen/Source/ns.cpp)
extern NS_Slight NS_slight[NS_MAX_SLIGHTS];
// uc_orig: NS_slight_upto (fallen/Source/ns.cpp)
extern SLONG NS_slight_upto;

// === Internal constants shared across ns.cpp chunks ===

// Curve type indices stored in NS_Hi.water for rock cells at sewer edges.
// uc_orig: NS_HI_CURVE_XS (fallen/Source/ns.cpp)
#define NS_HI_CURVE_XS  0
// uc_orig: NS_HI_CURVE_XL (fallen/Source/ns.cpp)
#define NS_HI_CURVE_XL  1
// uc_orig: NS_HI_CURVE_ZS (fallen/Source/ns.cpp)
#define NS_HI_CURVE_ZS  2
// uc_orig: NS_HI_CURVE_ZL (fallen/Source/ns.cpp)
#define NS_HI_CURVE_ZL  3
// uc_orig: NS_HI_CURVE_ASS (fallen/Source/ns.cpp)
#define NS_HI_CURVE_ASS 4
// uc_orig: NS_HI_CURVE_ALS (fallen/Source/ns.cpp)
#define NS_HI_CURVE_ALS 5
// uc_orig: NS_HI_CURVE_ASL (fallen/Source/ns.cpp)
#define NS_HI_CURVE_ASL 6
// uc_orig: NS_HI_CURVE_ALL (fallen/Source/ns.cpp)
#define NS_HI_CURVE_ALL 7
// uc_orig: NS_HI_CURVE_OSS (fallen/Source/ns.cpp)
#define NS_HI_CURVE_OSS 8
// uc_orig: NS_HI_CURVE_OLS (fallen/Source/ns.cpp)
#define NS_HI_CURVE_OLS 9
// uc_orig: NS_HI_CURVE_OSL (fallen/Source/ns.cpp)
#define NS_HI_CURVE_OSL 10
// uc_orig: NS_HI_CURVE_OLL (fallen/Source/ns.cpp)
#define NS_HI_CURVE_OLL 11

// Pre-defined texture slot names (indices into NS_texture[]).
// uc_orig: NS_TEXTURE_FULL (fallen/Source/ns.cpp)
#define NS_TEXTURE_FULL     0
// uc_orig: NS_TEXTURE_HSTRIP1 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP1  1
// uc_orig: NS_TEXTURE_HSTRIP2 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP2  2
// uc_orig: NS_TEXTURE_HSTRIP3 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP3  3
// uc_orig: NS_TEXTURE_HSTRIP4 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP4  4
// uc_orig: NS_TEXTURE_HSTRIP5 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP5  5
// uc_orig: NS_TEXTURE_HSTRIP6 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP6  6
// uc_orig: NS_TEXTURE_HSTRIP7 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP7  7
// uc_orig: NS_TEXTURE_HSTRIP8 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP8  8
// uc_orig: NS_TEXTURE_HSTRIP9 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP9  9
// uc_orig: NS_TEXTURE_HSTRIP10 (fallen/Source/ns.cpp)
#define NS_TEXTURE_HSTRIP10 10
// uc_orig: NS_TEXTURE_CT1 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT1      11
// uc_orig: NS_TEXTURE_CT2 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT2      12
// uc_orig: NS_TEXTURE_CT3 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT3      13
// uc_orig: NS_TEXTURE_CT4 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT4      14
// uc_orig: NS_TEXTURE_CT5 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT5      15
// uc_orig: NS_TEXTURE_CT6 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT6      16
// uc_orig: NS_TEXTURE_CT7 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT7      17
// uc_orig: NS_TEXTURE_CT8 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT8      18
// uc_orig: NS_TEXTURE_CT9 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT9      19
// uc_orig: NS_TEXTURE_CT10 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT10     20
// uc_orig: NS_TEXTURE_CT11 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT11     21
// uc_orig: NS_TEXTURE_CT12 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT12     22
// uc_orig: NS_TEXTURE_CT13 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT13     23
// uc_orig: NS_TEXTURE_CT14 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT14     24
// uc_orig: NS_TEXTURE_CT15 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT15     25
// uc_orig: NS_TEXTURE_CT16 (fallen/Source/ns.cpp)
#define NS_TEXTURE_CT16     26
// uc_orig: NS_TEXTURE_EL1 (fallen/Source/ns.cpp)
#define NS_TEXTURE_EL1      27
// uc_orig: NS_TEXTURE_EL2 (fallen/Source/ns.cpp)
#define NS_TEXTURE_EL2      28
// uc_orig: NS_TEXTURE_ER1 (fallen/Source/ns.cpp)
#define NS_TEXTURE_ER1      29
// uc_orig: NS_TEXTURE_ER2 (fallen/Source/ns.cpp)
#define NS_TEXTURE_ER2      30
// uc_orig: NS_TEXTURE_NUMBER (fallen/Source/ns.cpp)
#define NS_TEXTURE_NUMBER   31

// Normal direction codes for NS_add_point vertex lighting.
// uc_orig: NS_NORM_XL (fallen/Source/ns.cpp)
#define NS_NORM_XL    0
// uc_orig: NS_NORM_XS (fallen/Source/ns.cpp)
#define NS_NORM_XS    1
// uc_orig: NS_NORM_ZL (fallen/Source/ns.cpp)
#define NS_NORM_ZL    2
// uc_orig: NS_NORM_ZS (fallen/Source/ns.cpp)
#define NS_NORM_ZS    3
// uc_orig: NS_NORM_YL (fallen/Source/ns.cpp)
#define NS_NORM_YL    4
// uc_orig: NS_NORM_DUNNO (fallen/Source/ns.cpp)
#define NS_NORM_DUNNO 5
// uc_orig: NS_NORM_BLACK (fallen/Source/ns.cpp)
#define NS_NORM_BLACK 6
// uc_orig: NS_NORM_GREY (fallen/Source/ns.cpp)
#define NS_NORM_GREY  7

// Maximum scratch buffer capacity.
// uc_orig: NS_MAX_SCRATCH_POINTS (fallen/Source/ns.cpp)
#define NS_MAX_SCRATCH_POINTS ((HEAP_PAD_SIZE / 2) / sizeof(NS_Point))
// uc_orig: NS_MAX_SCRATCH_FACES (fallen/Source/ns.cpp)
#define NS_MAX_SCRATCH_FACES  ((HEAP_PAD_SIZE / 2) / sizeof(NS_Face))

// Internal helper: adds a lit vertex to the scratch buffer.
// uc_orig: NS_add_point (fallen/Source/ns.cpp)
void NS_add_point(SLONG x, SLONG y, SLONG z, SLONG norm);

// Internal helper: appends a quad face to the scratch face buffer.
// uc_orig: NS_add_face (fallen/Source/ns.cpp)
void NS_add_face(SLONG p[4], UBYTE page, UBYTE texture);

// Internal helper: finds or creates a scratch point for wall strip generation.
// uc_orig: NS_create_wallstrip_point (fallen/Source/ns.cpp)
SLONG NS_create_wallstrip_point(SLONG x, SLONG y, SLONG z, SLONG norm);
// uc_orig: NS_create_wallstrip_point (fallen/Source/ns.cpp)
SLONG NS_create_wallstrip_point(SLONG x, SLONG y, SLONG z);

// Internal: generates floor/ceiling geometry for a lo-res mapsquare.
// uc_orig: NS_cache_create_floors (fallen/Source/ns.cpp)
void NS_cache_create_floors(UBYTE mx, UBYTE mz);

// === Wall-strip point search range ===

// Search window for NS_create_wallstrip_point to reuse existing scratch points.
// uc_orig: NS_search_start (fallen/Source/ns.cpp)
extern SLONG NS_search_start;
// uc_orig: NS_search_end (fallen/Source/ns.cpp)
extern SLONG NS_search_end;

// === LOS failure position ===

// uc_orig: NS_los_fail_x (fallen/Source/ns.cpp)
extern SLONG NS_los_fail_x;
// uc_orig: NS_los_fail_y (fallen/Source/ns.cpp)
extern SLONG NS_los_fail_y;
// uc_orig: NS_los_fail_z (fallen/Source/ns.cpp)
extern SLONG NS_los_fail_z;

#endif // UNDERGROUND_NS_GLOBALS_H
