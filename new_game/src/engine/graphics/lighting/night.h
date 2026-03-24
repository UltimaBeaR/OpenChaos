#ifndef ENGINE_GRAPHICS_LIGHTING_NIGHT_H
#define ENGINE_GRAPHICS_LIGHTING_NIGHT_H

#include "engine/platform/uc_common.h"              // UBYTE, SLONG, SBYTE, UWORD, SWORD, ULONG base types
#include "world/map/pap_globals.h"

// uc_orig: NIGHT_MAX_SLIGHTS (fallen/Headers/Night.h)
#define NIGHT_MAX_SLIGHTS 256

// uc_orig: NIGHT_LIGHT_MULTIPLIER (fallen/Headers/Night.h)
#define NIGHT_LIGHT_MULTIPLIER 1.0f

// uc_orig: NIGHT_Slight (fallen/Headers/Night.h)
// Static point light placed by the level editor (streetlamps, glows).
// x/z are UBYTE; top bit used as an "inside" flag.
// red/green/blue are SBYTE (signed colour tint). radius is UBYTE falloff distance.
typedef struct
{
    SWORD y;
    UBYTE x;
    UBYTE z;
    SBYTE red;
    SBYTE green;
    SBYTE blue;
    UBYTE radius;

} NIGHT_Slight;

// uc_orig: NIGHT_Smap (fallen/Headers/Night.h)
typedef struct
{
    UBYTE index;
    UBYTE number;

} NIGHT_Smap;

// uc_orig: NIGHT_Smap_2d (fallen/Headers/Night.h)
typedef NIGHT_Smap NIGHT_Smap_2d[PAP_SIZE_LO];

// uc_orig: NIGHT_DLIGHT_FLAG_USED (fallen/Headers/Night.h)
#define NIGHT_DLIGHT_FLAG_USED (1 << 0)
// uc_orig: NIGHT_DLIGHT_FLAG_REMOVE (fallen/Headers/Night.h)
#define NIGHT_DLIGHT_FLAG_REMOVE (1 << 1)

// uc_orig: NIGHT_Dlight (fallen/Headers/Night.h)
// Runtime point light (explosion flash, fire glow, muzzle flash, etc.).
// next: linked list index. Pool-based allocation, 64 max.
typedef struct
{
    UWORD x;
    SWORD y;
    UWORD z;
    UBYTE red;
    UBYTE green;
    UBYTE blue;
    UBYTE radius;
    UBYTE next;
    UBYTE flag;

} NIGHT_Dlight;

// uc_orig: NIGHT_MAX_DLIGHTS (fallen/Headers/Night.h)
#define NIGHT_MAX_DLIGHTS 64

// uc_orig: NIGHT_Colour (fallen/Headers/Night.h)
// Fundamental lighting colour type. Range 0..NIGHT_MAX_BRIGHT (0..63).
// On PC/DC: 3 separate UBYTE channels. On PSX: packed into UWORD (bit-fields below).
typedef struct
{
    UBYTE red;
    UBYTE green;
    UBYTE blue;

} NIGHT_Colour;

// uc_orig: get_red (fallen/Headers/Night.h)
#define get_red(col) (((col) >> 10) & 0x3f)
// uc_orig: get_green (fallen/Headers/Night.h)
#define get_green(col) (((col) >> 5) & 0x1f)
// uc_orig: get_blue (fallen/Headers/Night.h)
#define get_blue(col) (((col)) & 0x1f)

// uc_orig: set_red (fallen/Headers/Night.h)
#define set_red(col, red) col = (red) << 10
// uc_orig: set_green (fallen/Headers/Night.h)
#define set_green(col, red) col = (red) << 5
// uc_orig: set_blue (fallen/Headers/Night.h)
#define set_blue(col, red) col = (red)

// uc_orig: or_red (fallen/Headers/Night.h)
#define or_red(col, red) col |= (red) << 10
// uc_orig: or_green (fallen/Headers/Night.h)
#define or_green(col, red) col |= (red) << 5
// uc_orig: or_blue (fallen/Headers/Night.h)
#define or_blue(col, red) col |= (red)

// uc_orig: nor_red (fallen/Headers/Night.h)
#define nor_red(col, red) col = (col & (0x3f << 10)) | ((red) << 10)
// uc_orig: nor_green (fallen/Headers/Night.h)
#define nor_green(col, red) col = (col & (0x1f << 5)) | ((red) << 5)
// uc_orig: nor_blue (fallen/Headers/Night.h)
#define nor_blue(col, red) col = (col & (0x1f << 0)) | ((red) << 0)

// uc_orig: NIGHT_SQUARE_FLAG_USED (fallen/Headers/Night.h)
#define NIGHT_SQUARE_FLAG_USED (1 << 0)
// uc_orig: NIGHT_SQUARE_FLAG_WARE (fallen/Headers/Night.h)
#define NIGHT_SQUARE_FLAG_WARE (1 << 1)
// uc_orig: NIGHT_SQUARE_FLAG_DELETEME (fallen/Headers/Night.h)
#define NIGHT_SQUARE_FLAG_DELETEME (1 << 2)

// uc_orig: NIGHT_Square (fallen/Headers/Night.h)
// Cached vertex colours for one lo-res terrain map square.
// colour points to PAP_BLOCKS*PAP_BLOCKS entries for terrain, then per-prim point colours.
typedef struct
{
    UBYTE next;
    UBYTE flag;
    UBYTE lo_map_x;
    UBYTE lo_map_z;
    NIGHT_Colour* colour;
    ULONG sizeof_colour;

} NIGHT_Square;

// uc_orig: NIGHT_MAX_SQUARES (fallen/Headers/Night.h)
#define NIGHT_MAX_SQUARES 256

// uc_orig: NIGHT_Dfcache (fallen/Headers/Night.h)
// Cached vertex colours for DFacets (building faces).
typedef struct
{
    UBYTE next;
    UBYTE counter;
    UWORD dfacet;
    NIGHT_Colour* colour;
    UWORD sizeof_colour;

} NIGHT_Dfcache;

// uc_orig: NIGHT_MAX_DFCACHES (fallen/Headers/Night.h)
#define NIGHT_MAX_DFCACHES 256

// uc_orig: NIGHT_MAX_BRIGHT (fallen/Headers/Night.h)
// Max value of a NIGHT_Colour channel. Multiply by 4 to convert to 0-255 D3D range.
#define NIGHT_MAX_BRIGHT 64

// uc_orig: NIGHT_specular_enable (fallen/Source/Controls.cpp)
extern SLONG NIGHT_specular_enable;

// uc_orig: NIGHT_get_d3d_colour (fallen/Headers/Night.h)
// Convert NIGHT_Colour (6-bit per channel) to D3D ARGB (0xAARRGGBB).
// Values over 255 overflow into pseudo-specular if NIGHT_specular_enable is set.
inline void NIGHT_get_d3d_colour(NIGHT_Colour col, ULONG* colour, ULONG* specular)
{
    SLONG red = col.red;
    SLONG green = col.green;
    SLONG blue = col.blue;

    red *= (256 / NIGHT_MAX_BRIGHT);
    green *= (256 / NIGHT_MAX_BRIGHT);
    blue *= (256 / NIGHT_MAX_BRIGHT);

    if (NIGHT_specular_enable) {
        SLONG wred = 0;
        SLONG wgreen = 0;
        SLONG wblue = 0;

        if (red > 255) {
            wred = (red - 255) >> 1;
            red = 255;
            if (wred > 255) {
                wred = 255;
            }
        }
        if (green > 255) {
            wgreen = (green - 255) >> 1;
            green = 255;
            if (wgreen > 255) {
                wgreen = 255;
            }
        }
        if (blue > 255) {
            wblue = (blue - 255) >> 1;
            blue = 255;
            if (wblue > 255) {
                wblue = 255;
            }
        }

        *colour = (red << 16) | (green << 8) | (blue << 0);
        *specular = (wred << 16) | (wgreen << 8) | (wblue << 0);
        *specular |= 0xff000000;
    } else {
        if (red > 255) {
            red = 255;
        }
        if (green > 255) {
            green = 255;
        }
        if (blue > 255) {
            blue = 255;
        }

        *colour = (red << 16) | (green << 8) | (blue << 0);
        *specular = 0xff000000;
    }
    *colour |= 0xff000000;
}

// uc_orig: POLY_FADEOUT_START (fallen/Headers/Night.h)
// uc_orig: POLY_FADEOUT_END (fallen/Headers/Night.h)
#ifndef POLY_FADEOUT_START
#define POLY_FADEOUT_START (0.60F)
#define POLY_FADEOUT_END   (0.95F)
#endif

// uc_orig: NIGHT_get_d3d_colour_and_fade (fallen/Headers/Night.h)
// Like NIGHT_get_d3d_colour but fades out at depth range [POLY_FADEOUT_START, POLY_FADEOUT_END].
inline void NIGHT_get_d3d_colour_and_fade(NIGHT_Colour col, ULONG* colour, ULONG* specular, float z)
{
    SLONG red = col.red;
    SLONG green = col.green;
    SLONG blue = col.blue;
    SLONG multi = 255 - (SLONG)((z - POLY_FADEOUT_START) * (256.0F / (POLY_FADEOUT_END - POLY_FADEOUT_START)));

    if (multi > 255) {
        multi = 255;
    }
    if (multi < 0) {
        multi = 0;
    }
    multi *= 4;

    red = red * multi >> 8;
    green = green * multi >> 8;
    blue = blue * multi >> 8;

    {
        if (red > 255) {
            red = 255;
        }
        if (green > 255) {
            green = 255;
        }
        if (blue > 255) {
            blue = 255;
        }

        *colour = (red << 16) | (green << 8) | (blue << 0);
        *specular = 0xff000000;
    }
    *colour |= 0xff000000;
}

// uc_orig: NIGHT_get_d3d_colour_dim (fallen/Headers/Night.h)
// Like NIGHT_get_d3d_colour but halves the brightness (dim variant).
inline void NIGHT_get_d3d_colour_dim(NIGHT_Colour col, ULONG* colour, ULONG* specular)
{
    SLONG red = col.red;
    SLONG green = col.green;
    SLONG blue = col.blue;

    red *= (256 / NIGHT_MAX_BRIGHT);
    green *= (256 / NIGHT_MAX_BRIGHT);
    blue *= (256 / NIGHT_MAX_BRIGHT);

    red >>= 1;
    green >>= 1;
    blue >>= 1;

    if (NIGHT_specular_enable) {
        SLONG wred = 0;
        SLONG wgreen = 0;
        SLONG wblue = 0;

        if (red > 255) {
            wred = (red - 255) >> 1;
            red = 255;
            if (wred > 255) {
                wred = 255;
            }
        }
        if (green > 255) {
            wgreen = (green - 255) >> 1;
            green = 255;
            if (wgreen > 255) {
                wgreen = 255;
            }
        }
        if (blue > 255) {
            wblue = (blue - 255) >> 1;
            blue = 255;
            if (wblue > 255) {
                wblue = 255;
            }
        }

        *colour = (red << 16) | (green << 8) | (blue << 0);
        *specular = (wred << 16) | (wgreen << 8) | (wblue << 0);
        *specular |= 0xff000000;
    } else {
        if (red > 255) {
            red = 255;
        }
        if (green > 255) {
            green = 255;
        }
        if (blue > 255) {
            blue = 255;
        }

        *colour = (red << 16) | (green << 8) | (blue << 0);
        *specular = 0xff000000;
    }
    *colour |= 0xff000000;
}

// uc_orig: NIGHT_get_light_at (fallen/Headers/Night.h)
NIGHT_Colour NIGHT_get_light_at(SLONG x, SLONG y, SLONG z);

// uc_orig: NIGHT_Found (fallen/Headers/Night.h)
// Describes one light source that affects a given point: colour + direction vector.
typedef struct
{
    SLONG r;
    SLONG g;
    SLONG b;
    SLONG dx;
    SLONG dy;
    SLONG dz;

} NIGHT_Found;

// uc_orig: NIGHT_MAX_FOUND (fallen/Headers/Night.h)
#define NIGHT_MAX_FOUND 4

// uc_orig: NIGHT_find (fallen/Headers/Night.h)
void NIGHT_find(SLONG x, SLONG y, SLONG z);

// uc_orig: NIGHT_destroy_all_cached_info (fallen/Headers/Night.h)
void NIGHT_destroy_all_cached_info(void);

// uc_orig: NIGHT_Llight (fallen/Source/night.cpp)
// Position of a lampost light for lighting cache computation.
typedef struct
{
    UWORD x;
    SWORD y;
    UWORD z;

} NIGHT_Llight;

// uc_orig: NIGHT_MAX_LLIGHTS (fallen/Source/night.cpp)
#define NIGHT_MAX_LLIGHTS 16

// uc_orig: NIGHT_FLAG_INSIDE (fallen/Source/night.cpp)
// Internal flag: light source is inside a building, only affects inside points.
#define NIGHT_FLAG_INSIDE (1 << 0)

// uc_orig: NIGHT_FLAG_LIGHTS_UNDER_LAMPOSTS (fallen/Headers/Night.h)
#define NIGHT_FLAG_LIGHTS_UNDER_LAMPOSTS (1 << 0)
// uc_orig: NIGHT_FLAG_DARKEN_BUILDING_POINTS (fallen/Headers/Night.h)
#define NIGHT_FLAG_DARKEN_BUILDING_POINTS (1 << 1)
// uc_orig: NIGHT_FLAG_DAYTIME (fallen/Headers/Night.h)
// When set: it is daytime — cloud shadows active, lampost lights off.
#define NIGHT_FLAG_DAYTIME (1 << 2)

// uc_orig: NIGHT_load_ed_file (fallen/Headers/Night.h)
extern SLONG NIGHT_load_ed_file(CBYTE* name);

// uc_orig: NIGHT_ambient (fallen/Headers/Night.h)
// Set ambient (directional) light. normal should be normalised to length 256.
void NIGHT_ambient(UBYTE red, UBYTE green, UBYTE blue, SLONG norm_x, SLONG norm_y, SLONG norm_z);

// uc_orig: NIGHT_ambient_at_point (fallen/Headers/Night.h)
// Returns the ambient light contribution at a surface point with given normal (length 256).
NIGHT_Colour NIGHT_ambient_at_point(SLONG norm_x, SLONG norm_y, SLONG norm_z);

// uc_orig: NIGHT_slight_create (fallen/Headers/Night.h)
SLONG NIGHT_slight_create(SLONG x, SLONG y, SLONG z, UBYTE radius, SBYTE red, SBYTE green, SBYTE blue);

// uc_orig: NIGHT_slight_delete (fallen/Headers/Night.h)
void NIGHT_slight_delete(SLONG x, SLONG y, SLONG z, UBYTE radius, SBYTE red, SBYTE green, SBYTE blue);

// uc_orig: NIGHT_slight_delete_all (fallen/Headers/Night.h)
void NIGHT_slight_delete_all(void);

// uc_orig: NIGHT_dlight_create (fallen/Headers/Night.h)
UBYTE NIGHT_dlight_create(SLONG x, SLONG y, SLONG z, UBYTE radius, UBYTE red, UBYTE green, UBYTE blue);

// uc_orig: NIGHT_dlight_destroy (fallen/Headers/Night.h)
void NIGHT_dlight_destroy(UBYTE dlight_index);
// uc_orig: NIGHT_dlight_move (fallen/Headers/Night.h)
void NIGHT_dlight_move(UBYTE dlight_index, SLONG x, SLONG y, SLONG z);
// uc_orig: NIGHT_dlight_colour (fallen/Headers/Night.h)
void NIGHT_dlight_colour(UBYTE dlight_index, UBYTE red, UBYTE green, UBYTE blue);

// uc_orig: NIGHT_dlight_squares_up (fallen/Headers/Night.h)
void NIGHT_dlight_squares_up(void);
// uc_orig: NIGHT_dlight_squares_down (fallen/Headers/Night.h)
void NIGHT_dlight_squares_down(void);

// uc_orig: NIGHT_cache_recalc (fallen/Headers/Night.h)
void NIGHT_cache_recalc(void);
// uc_orig: NIGHT_cache_create (fallen/Headers/Night.h)
void NIGHT_cache_create(UBYTE lo_map_x, UBYTE lo_map_z, UBYTE inside_warehouse = UC_FALSE);
// uc_orig: NIGHT_cache_create_inside (fallen/Headers/Night.h)
void NIGHT_cache_create_inside(UBYTE lo_map_x, UBYTE lo_map_z, SLONG floor_y);
// uc_orig: NIGHT_cache_destroy (fallen/Headers/Night.h)
void NIGHT_cache_destroy(UBYTE square_index);

// uc_orig: NIGHT_dfcache_recalc (fallen/Headers/Night.h)
void NIGHT_dfcache_recalc(void);
// uc_orig: NIGHT_dfcache_create (fallen/Headers/Night.h)
UBYTE NIGHT_dfcache_create(UWORD dfacet_index);
// uc_orig: NIGHT_dfcache_destroy (fallen/Headers/Night.h)
void NIGHT_dfcache_destroy(UBYTE dfcache_index);

// uc_orig: NIGHT_MAX_WALKABLE (fallen/Headers/Night.h)
#define NIGHT_MAX_WALKABLE 15000

// uc_orig: NIGHT_WALKABLE_POINT (fallen/Headers/Night.h)
#define NIGHT_WALKABLE_POINT(p) (NIGHT_walkable[(p) - NIGHT_first_walkable_prim_point])

// uc_orig: NIGHT_ROOF_WALKABLE_POINT (fallen/Headers/Night.h)
#define NIGHT_ROOF_WALKABLE_POINT(f, p) (NIGHT_roof_walkable[f * 4 + p])

// uc_orig: NIGHT_generate_walkable_lighting (fallen/Headers/Night.h)
void NIGHT_generate_walkable_lighting(void);

// uc_orig: NIGHT_slight_init (fallen/Source/night.cpp)
void NIGHT_slight_init(void);
// uc_orig: NIGHT_dlight_init (fallen/Source/night.cpp)
void NIGHT_dlight_init(void);
// uc_orig: NIGHT_cache_init (fallen/Source/night.cpp)
void NIGHT_cache_init(void);
// uc_orig: NIGHT_dfcache_init (fallen/Source/night.cpp)
void NIGHT_dfcache_init(void);

// uc_orig: NIGHT_init (fallen/Source/night.cpp)
void NIGHT_init(void);

// uc_orig: NIGHT_light_mapsquare (fallen/Source/night.cpp)
void NIGHT_light_mapsquare(SLONG lo_map_x, SLONG lo_map_z, NIGHT_Colour* colour, SLONG floor_y, SLONG inside);

// uc_orig: NIGHT_get_facet_info (fallen/Source/night.cpp)
void NIGHT_get_facet_info(UWORD dfacet_index, SLONG* length, SLONG* height, SLONG* dx, SLONG* dy, SLONG* dz, ULONG* flags);

// uc_orig: NIGHT_light_prim (fallen/Source/night.cpp)
void NIGHT_light_prim(SLONG prim, SLONG prim_x, SLONG prim_y, SLONG prim_z, SLONG prim_yaw, SLONG prim_pitch, SLONG prim_roll, SLONG prim_inside, NIGHT_Colour* colour);

// uc_orig: NIGHT_generate_roof_walkable (fallen/Source/night.cpp)
void NIGHT_generate_roof_walkable(void);

#endif // ENGINE_GRAPHICS_LIGHTING_NIGHT_H
