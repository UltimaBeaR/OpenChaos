// Temporary: Game.h must be first — sets up cross-module types.
#include "fallen/Headers/Game.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"

// uc_orig: NIGHT_llight (fallen/Source/night.cpp)
NIGHT_Llight NIGHT_llight[NIGHT_MAX_LLIGHTS];
// uc_orig: NIGHT_llight_upto (fallen/Source/night.cpp)
SLONG NIGHT_llight_upto;

// uc_orig: NIGHT_slight (fallen/Source/night.cpp)
NIGHT_Slight* NIGHT_slight;
// uc_orig: NIGHT_slight_upto (fallen/Source/night.cpp)
SLONG NIGHT_slight_upto;

// uc_orig: NIGHT_smap (fallen/Source/night.cpp)
NIGHT_Smap_2d* NIGHT_smap;

// uc_orig: NIGHT_dlight (fallen/Source/night.cpp)
NIGHT_Dlight* NIGHT_dlight;
// uc_orig: NIGHT_dlight_free (fallen/Source/night.cpp)
UBYTE NIGHT_dlight_free;
// uc_orig: NIGHT_dlight_used (fallen/Source/night.cpp)
UBYTE NIGHT_dlight_used;

// uc_orig: NIGHT_square (fallen/Source/night.cpp)
NIGHT_Square NIGHT_square[NIGHT_MAX_SQUARES];
// uc_orig: NIGHT_square_free (fallen/Source/night.cpp)
UBYTE NIGHT_square_free;
// uc_orig: NIGHT_square_num_used (fallen/Source/night.cpp)
SLONG NIGHT_square_num_used;
// uc_orig: NIGHT_cache (fallen/Source/night.cpp)
UBYTE NIGHT_cache[PAP_SIZE_LO][PAP_SIZE_LO];

// uc_orig: NIGHT_dfcache (fallen/Source/night.cpp)
NIGHT_Dfcache NIGHT_dfcache[NIGHT_MAX_DFCACHES];
// uc_orig: NIGHT_dfcache_free (fallen/Source/night.cpp)
UBYTE NIGHT_dfcache_free;
// uc_orig: NIGHT_dfcache_used (fallen/Source/night.cpp)
UBYTE NIGHT_dfcache_used;

// uc_orig: NIGHT_flag (fallen/Source/night.cpp)
ULONG NIGHT_flag;
// uc_orig: NIGHT_lampost_radius (fallen/Source/night.cpp)
UBYTE NIGHT_lampost_radius;
// uc_orig: NIGHT_lampost_red (fallen/Source/night.cpp)
SBYTE NIGHT_lampost_red;
// uc_orig: NIGHT_lampost_green (fallen/Source/night.cpp)
SBYTE NIGHT_lampost_green;
// uc_orig: NIGHT_lampost_blue (fallen/Source/night.cpp)
SBYTE NIGHT_lampost_blue;
// uc_orig: NIGHT_sky_colour (fallen/Source/night.cpp)
NIGHT_Colour NIGHT_sky_colour;

// uc_orig: NIGHT_amb_d3d_colour (fallen/Source/night.cpp)
ULONG NIGHT_amb_d3d_colour;
// uc_orig: NIGHT_amb_d3d_specular (fallen/Source/night.cpp)
ULONG NIGHT_amb_d3d_specular;
// uc_orig: NIGHT_amb_red (fallen/Source/night.cpp)
SLONG NIGHT_amb_red;
// uc_orig: NIGHT_amb_green (fallen/Source/night.cpp)
SLONG NIGHT_amb_green;
// uc_orig: NIGHT_amb_blue (fallen/Source/night.cpp)
SLONG NIGHT_amb_blue;
// uc_orig: NIGHT_amb_norm_x (fallen/Source/night.cpp)
SLONG NIGHT_amb_norm_x;
// uc_orig: NIGHT_amb_norm_y (fallen/Source/night.cpp)
SLONG NIGHT_amb_norm_y;
// uc_orig: NIGHT_amb_norm_z (fallen/Source/night.cpp)
SLONG NIGHT_amb_norm_z;

// uc_orig: NIGHT_dfcache_counter (fallen/Source/night.cpp)
UBYTE NIGHT_dfcache_counter;

// uc_orig: NIGHT_old_amb_red (fallen/Source/night.cpp)
SLONG NIGHT_old_amb_red;
// uc_orig: NIGHT_old_amb_green (fallen/Source/night.cpp)
SLONG NIGHT_old_amb_green;
// uc_orig: NIGHT_old_amb_blue (fallen/Source/night.cpp)
SLONG NIGHT_old_amb_blue;
// uc_orig: NIGHT_old_amb_norm_x (fallen/Source/night.cpp)
SLONG NIGHT_old_amb_norm_x;
// uc_orig: NIGHT_old_amb_norm_y (fallen/Source/night.cpp)
SLONG NIGHT_old_amb_norm_y;
// uc_orig: NIGHT_old_amb_norm_z (fallen/Source/night.cpp)
SLONG NIGHT_old_amb_norm_z;

// uc_orig: NIGHT_found (fallen/Source/night.cpp)
NIGHT_Found NIGHT_found[NIGHT_MAX_FOUND];
// uc_orig: NIGHT_found_upto (fallen/Source/night.cpp)
SLONG NIGHT_found_upto;

// uc_orig: NIGHT_first_walkable_prim_point (fallen/Source/night.cpp)
SLONG NIGHT_first_walkable_prim_point;
// uc_orig: NIGHT_walkable (fallen/Source/night.cpp)
NIGHT_Colour NIGHT_walkable[NIGHT_MAX_WALKABLE];
// uc_orig: NIGHT_roof_walkable (fallen/Source/night.cpp)
// MAX_ROOF_FACE4 * 4 entries. MAX_ROOF_FACE4=10000 defined in fallen/Headers/memory.h.
NIGHT_Colour NIGHT_roof_walkable[10000 * 4]; // 10000 = MAX_ROOF_FACE4
