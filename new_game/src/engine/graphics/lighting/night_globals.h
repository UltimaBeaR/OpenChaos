#ifndef ENGINE_GRAPHICS_LIGHTING_NIGHT_GLOBALS_H
#define ENGINE_GRAPHICS_LIGHTING_NIGHT_GLOBALS_H

#include "engine/graphics/lighting/night.h"

// uc_orig: NIGHT_llight (fallen/Source/night.cpp)
extern NIGHT_Llight NIGHT_llight[NIGHT_MAX_LLIGHTS];
// uc_orig: NIGHT_llight_upto (fallen/Source/night.cpp)
extern SLONG NIGHT_llight_upto;

// uc_orig: NIGHT_slight (fallen/Source/night.cpp)
extern NIGHT_Slight* NIGHT_slight;
// uc_orig: NIGHT_slight_upto (fallen/Source/night.cpp)
extern SLONG NIGHT_slight_upto;

// uc_orig: NIGHT_smap (fallen/Source/night.cpp)
extern NIGHT_Smap_2d* NIGHT_smap;

// uc_orig: NIGHT_dlight (fallen/Source/night.cpp)
extern NIGHT_Dlight* NIGHT_dlight;
// uc_orig: NIGHT_dlight_free (fallen/Source/night.cpp)
extern UBYTE NIGHT_dlight_free;
// uc_orig: NIGHT_dlight_used (fallen/Source/night.cpp)
extern UBYTE NIGHT_dlight_used;

// uc_orig: NIGHT_square (fallen/Source/night.cpp)
extern NIGHT_Square NIGHT_square[NIGHT_MAX_SQUARES];
// uc_orig: NIGHT_square_free (fallen/Source/night.cpp)
extern UBYTE NIGHT_square_free;
// uc_orig: NIGHT_square_num_used (fallen/Source/night.cpp)
extern SLONG NIGHT_square_num_used;
// uc_orig: NIGHT_cache (fallen/Source/night.cpp)
extern UBYTE NIGHT_cache[PAP_SIZE_LO][PAP_SIZE_LO];

// uc_orig: NIGHT_dfcache (fallen/Source/night.cpp)
extern NIGHT_Dfcache NIGHT_dfcache[NIGHT_MAX_DFCACHES];
// uc_orig: NIGHT_dfcache_free (fallen/Source/night.cpp)
extern UBYTE NIGHT_dfcache_free;
// uc_orig: NIGHT_dfcache_used (fallen/Source/night.cpp)
extern UBYTE NIGHT_dfcache_used;

// uc_orig: NIGHT_flag (fallen/Source/night.cpp)
extern ULONG NIGHT_flag;
// uc_orig: NIGHT_lampost_radius (fallen/Source/night.cpp)
extern UBYTE NIGHT_lampost_radius;
// uc_orig: NIGHT_lampost_red (fallen/Source/night.cpp)
extern SBYTE NIGHT_lampost_red;
// uc_orig: NIGHT_lampost_green (fallen/Source/night.cpp)
extern SBYTE NIGHT_lampost_green;
// uc_orig: NIGHT_lampost_blue (fallen/Source/night.cpp)
extern SBYTE NIGHT_lampost_blue;
// uc_orig: NIGHT_sky_colour (fallen/Source/night.cpp)
extern NIGHT_Colour NIGHT_sky_colour;

// uc_orig: NIGHT_amb_d3d_colour (fallen/Source/night.cpp)
extern ULONG NIGHT_amb_d3d_colour;
// uc_orig: NIGHT_amb_d3d_specular (fallen/Source/night.cpp)
extern ULONG NIGHT_amb_d3d_specular;
// uc_orig: NIGHT_amb_red (fallen/Source/night.cpp)
extern SLONG NIGHT_amb_red;
// uc_orig: NIGHT_amb_green (fallen/Source/night.cpp)
extern SLONG NIGHT_amb_green;
// uc_orig: NIGHT_amb_blue (fallen/Source/night.cpp)
extern SLONG NIGHT_amb_blue;
// uc_orig: NIGHT_amb_norm_x (fallen/Source/night.cpp)
extern SLONG NIGHT_amb_norm_x;
// uc_orig: NIGHT_amb_norm_y (fallen/Source/night.cpp)
extern SLONG NIGHT_amb_norm_y;
// uc_orig: NIGHT_amb_norm_z (fallen/Source/night.cpp)
extern SLONG NIGHT_amb_norm_z;

// uc_orig: NIGHT_dfcache_counter (fallen/Source/night.cpp)
extern UBYTE NIGHT_dfcache_counter;

// uc_orig: NIGHT_old_amb_red (fallen/Source/night.cpp)
extern SLONG NIGHT_old_amb_red;
// uc_orig: NIGHT_old_amb_green (fallen/Source/night.cpp)
extern SLONG NIGHT_old_amb_green;
// uc_orig: NIGHT_old_amb_blue (fallen/Source/night.cpp)
extern SLONG NIGHT_old_amb_blue;
// uc_orig: NIGHT_old_amb_norm_x (fallen/Source/night.cpp)
extern SLONG NIGHT_old_amb_norm_x;
// uc_orig: NIGHT_old_amb_norm_y (fallen/Source/night.cpp)
extern SLONG NIGHT_old_amb_norm_y;
// uc_orig: NIGHT_old_amb_norm_z (fallen/Source/night.cpp)
extern SLONG NIGHT_old_amb_norm_z;

// uc_orig: NIGHT_found (fallen/Source/night.cpp)
extern NIGHT_Found NIGHT_found[NIGHT_MAX_FOUND];
// uc_orig: NIGHT_found_upto (fallen/Source/night.cpp)
extern SLONG NIGHT_found_upto;

// uc_orig: NIGHT_first_walkable_prim_point (fallen/Source/night.cpp)
extern SLONG NIGHT_first_walkable_prim_point;
// uc_orig: NIGHT_walkable (fallen/Source/night.cpp)
extern NIGHT_Colour NIGHT_walkable[NIGHT_MAX_WALKABLE];
// uc_orig: NIGHT_roof_walkable (fallen/Source/night.cpp)
extern NIGHT_Colour NIGHT_roof_walkable[];

// uc_orig: hidden_roof_index (fallen/Source/night.cpp)
// Index into NIGHT_roof_walkable[] for each hi-map (x,z) square that has a hidden roof.
// Set by NIGHT_generate_roof_walkable; read by aeng.cpp for hidden-roof lighting lookup.
extern UWORD hidden_roof_index[128][128];

#endif // ENGINE_GRAPHICS_LIGHTING_NIGHT_GLOBALS_H
