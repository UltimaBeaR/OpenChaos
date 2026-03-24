#ifndef UNDERGROUND_NS_H
#define UNDERGROUND_NS_H

// Sewer/cavern subsystem — geometry, caching, and query functions for underground areas.
// The "NS" (New Sewer) system manages a separate low-res/high-res grid describing sewer tunnels
// beneath the city map.

#include "engine/core/types.h"
#include "map/pap.h"

// === High-res sewer map cell ===

// uc_orig: NS_HI_PACKED_TYPE (fallen/Headers/ns.h)
#define NS_HI_PACKED_TYPE (0x07)
// uc_orig: NS_HI_PACKED_FLAG (fallen/Headers/ns.h)
#define NS_HI_PACKED_FLAG (0x38)
// uc_orig: NS_HI_PACKED_DIR (fallen/Headers/ns.h)
#define NS_HI_PACKED_DIR (0xc0)

// uc_orig: NS_HI_TYPE (fallen/Headers/ns.h)
#define NS_HI_TYPE(nh) ((nh)->packed & NS_HI_PACKED_TYPE)
// uc_orig: NS_HI_TYPE_SET (fallen/Headers/ns.h)
#define NS_HI_TYPE_SET(nh, t)               \
    {                                       \
        (nh)->packed &= ~NS_HI_PACKED_TYPE; \
        (nh)->packed |= (t);                \
    }

// uc_orig: NS_HI_TYPE_ROCK (fallen/Headers/ns.h)
#define NS_HI_TYPE_ROCK 0
// uc_orig: NS_HI_TYPE_SEWER (fallen/Headers/ns.h)
#define NS_HI_TYPE_SEWER 1
// uc_orig: NS_HI_TYPE_STONE (fallen/Headers/ns.h)
#define NS_HI_TYPE_STONE 2
// uc_orig: NS_HI_TYPE_NOTHING (fallen/Headers/ns.h)
#define NS_HI_TYPE_NOTHING 3
// uc_orig: NS_HI_TYPE_CURVE (fallen/Headers/ns.h)
#define NS_HI_TYPE_CURVE 4 // Private — set by NS_precalculate for edge smoothing

// uc_orig: NS_HI_FLAG_GRATE (fallen/Headers/ns.h)
#define NS_HI_FLAG_GRATE (1 << 5)   // Hole through which water can pour
// uc_orig: NS_HI_FLAG_LOCKTOP (fallen/Headers/ns.h)
#define NS_HI_FLAG_LOCKTOP (1 << 6) // Private — top height is locked
// uc_orig: NS_HI_FLAG_TOPUSED (fallen/Headers/ns.h)
#define NS_HI_FLAG_TOPUSED (1 << 7) // Private — top height has been computed

// uc_orig: NS_Hi (fallen/Headers/ns.h)
typedef struct
{
    // Heights in units of 1/8 map-square, from 32 squares below ground (value 0 = -32 squares).
    // 'bot' must be on mapsquare boundaries (multiples of 8).
    // 'top' is computed by NS_precalculate.
    UBYTE bot;
    UBYTE top;

    UBYTE water; // Height of water, or 0 if none. No water allowed on rock.
    UBYTE packed; // bits [2:0] = type, bits [5:3] = flags

} NS_Hi;

// uc_orig: NS_hi (fallen/Headers/ns.h)
extern NS_Hi NS_hi[PAP_SIZE_HI][PAP_SIZE_HI];

// === Textures ===

// uc_orig: NS_Texture (fallen/Headers/ns.h)
typedef struct
{
    UBYTE u[4];
    UBYTE v[4];

} NS_Texture;

// uc_orig: NS_MAX_TEXTURES (fallen/Headers/ns.h)
#define NS_MAX_TEXTURES 256

// uc_orig: NS_texture (fallen/Headers/ns.h)
extern NS_Texture NS_texture[NS_MAX_TEXTURES];
// uc_orig: NS_texture_upto (fallen/Headers/ns.h)
extern SLONG NS_texture_upto;

// === Pages (one per texture atlas page) ===

// uc_orig: NS_Page (fallen/Headers/ns.h)
typedef struct
{
    UWORD page;
} NS_Page;

// uc_orig: NS_PAGE_ROCK (fallen/Headers/ns.h)
#define NS_PAGE_ROCK 0
// uc_orig: NS_PAGE_SEWER (fallen/Headers/ns.h)
#define NS_PAGE_SEWER 1
// uc_orig: NS_PAGE_STONE (fallen/Headers/ns.h)
#define NS_PAGE_STONE 2
// uc_orig: NS_PAGE_SWALL (fallen/Headers/ns.h)
#define NS_PAGE_SWALL 3
// uc_orig: NS_PAGE_GRATE (fallen/Headers/ns.h)
#define NS_PAGE_GRATE 4
// uc_orig: NS_PAGE_NUMBER (fallen/Headers/ns.h)
#define NS_PAGE_NUMBER 5

// uc_orig: NS_page (fallen/Headers/ns.h)
extern NS_Page NS_page[NS_PAGE_NUMBER];

// === Waterfalls ===

// uc_orig: NS_Fall (fallen/Headers/ns.h)
typedef struct
{
    UBYTE x;
    UBYTE z;
    SBYTE dx; // Vector from source direction
    SBYTE dz;
    UBYTE top;
    UBYTE bot;
    UBYTE counter;
    UBYTE next;
} NS_Fall;

// uc_orig: NS_MAX_FALLS (fallen/Headers/ns.h)
#define NS_MAX_FALLS 32

// uc_orig: NS_fall (fallen/Headers/ns.h)
extern NS_Fall NS_fall[NS_MAX_FALLS];
// uc_orig: NS_fall_free (fallen/Headers/ns.h)
extern UBYTE NS_fall_free;

// === Scratch buffer element types ===

// uc_orig: NS_Point (fallen/Headers/ns.h)
typedef struct
{
    UBYTE x;
    UBYTE z;
    UBYTE y; // In 1/8 mapsquare units from 32 squares underground
    UBYTE bright;
} NS_Point;

// uc_orig: NS_Face (fallen/Headers/ns.h)
typedef struct
{
    UBYTE p[4];
    UBYTE page;
    UBYTE texture;
} NS_Face;

// === Cache ===

// uc_orig: NS_Cache (fallen/Headers/ns.h)
typedef struct
{
    UBYTE next;
    UBYTE used;
    UBYTE map_x;
    UBYTE map_z;
    UBYTE* memory;
    UWORD num_points;
    UWORD num_faces; // Face data starts immediately after point data
    UBYTE fall;      // Linked list index of first waterfall in this square
    UBYTE padding;
} NS_Cache;

// uc_orig: NS_MAX_CACHES (fallen/Headers/ns.h)
#define NS_MAX_CACHES 128

// uc_orig: NS_cache (fallen/Headers/ns.h)
extern NS_Cache NS_cache[NS_MAX_CACHES];
// uc_orig: NS_cache_free (fallen/Headers/ns.h)
extern UBYTE NS_cache_free;

// === Sewer things (ladders, prims, bridges) ===

// uc_orig: NS_ST_TYPE_UNUSED (fallen/Headers/ns.h)
#define NS_ST_TYPE_UNUSED 0
// uc_orig: NS_ST_TYPE_PRIM (fallen/Headers/ns.h)
#define NS_ST_TYPE_PRIM 1
// uc_orig: NS_ST_TYPE_LADDER (fallen/Headers/ns.h)
#define NS_ST_TYPE_LADDER 2
// uc_orig: NS_ST_TYPE_BRIDGE (fallen/Headers/ns.h)
#define NS_ST_TYPE_BRIDGE 3
// uc_orig: NS_ST_TYPE_PLATFORM (fallen/Headers/ns.h)
#define NS_ST_TYPE_PLATFORM 4

// uc_orig: NS_St (fallen/Headers/ns.h)
typedef struct
{
    UBYTE type;
    UBYTE next;

    union {
        struct
        {
            UBYTE prim;
            UBYTE yaw;
            UBYTE x; // (x<<3, z<<3) relative to the lo-res mapsquare
            UBYTE z;
            UBYTE y;
        } prim;

        struct
        {
            UBYTE x1; // hi-res mapsquare coordinates
            UBYTE z1;
            UBYTE x2;
            UBYTE z2;
            UBYTE height;
        } ladder;
    };
} NS_St;

// uc_orig: NS_MAX_STS (fallen/Headers/ns.h)
#define NS_MAX_STS 64

// uc_orig: NS_st (fallen/Headers/ns.h)
extern NS_St NS_st[NS_MAX_STS];
// uc_orig: NS_st_free (fallen/Headers/ns.h)
extern UBYTE NS_st_free;

// === Lo-res sewer map ===

// uc_orig: NS_Lo (fallen/Headers/ns.h)
typedef struct
{
    UBYTE cache;
    UBYTE st; // Head of linked list of sewer things above this square

    // Light position for this lo-res square. y==0 means no light.
    UBYTE light_x; // (x<<3, z<<3) relative to lo-res square
    UBYTE light_z; // 0 = no light
    UBYTE light_y; // In 1/8 mapsquare units from 32 squares underground
} NS_Lo;

// uc_orig: NS_lo (fallen/Headers/ns.h)
extern NS_Lo NS_lo[PAP_SIZE_LO][PAP_SIZE_LO];

// === Editor functions ===

// Initialises sewer grid to all-rock.
// uc_orig: NS_init (fallen/Source/ns.cpp)
void NS_init(void);

// Computes top heights of rock cells and marks curve cells at sewer edges.
// uc_orig: NS_precalculate (fallen/Source/ns.cpp)
void NS_precalculate(void);

// Adds a ladder to the sewer map. All coordinates in hi-res mapsquare units, clockwise order.
// uc_orig: NS_add_ladder (fallen/Source/ns.cpp)
void NS_add_ladder(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG height);

// Adds a prim to the sewer map. (x,z) in hi-res fixed-8 coords, y in sewer Y units.
// uc_orig: NS_add_prim (fallen/Source/ns.cpp)
void NS_add_prim(SLONG prim, SLONG yaw, SLONG x, SLONG y, SLONG z);

// Serialise/deserialise sewer map.
// uc_orig: NS_save (fallen/Headers/ns.h)
void NS_save(CBYTE* fname);
// uc_orig: NS_load (fallen/Headers/ns.h)
void NS_load(CBYTE* fname);

// === Sewer queries ===

// Returns sewer floor height at world (x,z) in 1/8 mapsquare units.
// uc_orig: NS_calc_height_at (fallen/Source/ns.cpp)
SLONG NS_calc_height_at(SLONG x, SLONG z);

// Returns water surface height (or floor height if no water) at world (x,z).
// uc_orig: NS_calc_splash_height_at (fallen/Source/ns.cpp)
SLONG NS_calc_splash_height_at(SLONG x, SLONG z);

// Slides a movement vector to avoid penetrating sewer geometry.
// Takes positions in 16-bit fixed point. radius is 8-bit fixed point.
// uc_orig: NS_slide_along (fallen/Source/ns.cpp)
void NS_slide_along(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG radius);

// Returns UC_TRUE if (x,y,z) is inside the sewer system.
// uc_orig: NS_inside (fallen/Source/ns.cpp)
SLONG NS_inside(SLONG x, SLONG y, SLONG z);

// uc_orig: NS_los_fail_x (fallen/Source/ns.cpp)
extern SLONG NS_los_fail_x;
// uc_orig: NS_los_fail_y (fallen/Source/ns.cpp)
extern SLONG NS_los_fail_y;
// uc_orig: NS_los_fail_z (fallen/Source/ns.cpp)
extern SLONG NS_los_fail_z;

// Returns UC_TRUE if the line from (x1,y1,z1) to (x2,y2,z2) has clear line-of-sight through sewers.
// uc_orig: NS_there_is_a_los (fallen/Source/ns.cpp)
SLONG NS_there_is_a_los(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2);

// === Cache management ===

// Initialises the HEAP and sewer cache. Call once at level load.
// uc_orig: NS_cache_init (fallen/Source/ns.cpp)
void NS_cache_init(void);

// Builds geometry cache for mapsquare (mx,mz). Returns cache index.
// uc_orig: NS_cache_create (fallen/Source/ns.cpp)
SLONG NS_cache_create(UBYTE mx, UBYTE mz);

// Frees the geometry cache for the given cache slot.
// uc_orig: NS_cache_destroy (fallen/Source/ns.cpp)
void NS_cache_destroy(UBYTE cache);

// Frees all cache entries and reinitialises the HEAP.
// uc_orig: NS_cache_fini (fallen/Source/ns.cpp)
void NS_cache_fini(void);

// Returns index of an unused NS_St slot, or asserts if pool is full.
// uc_orig: NS_get_unused_st (fallen/Source/ns.cpp)
SLONG NS_get_unused_st(void);

#endif // UNDERGROUND_NS_H
