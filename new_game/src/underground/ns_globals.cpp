#include "game/game_types.h"
#include "underground/ns_globals.h"
#include "engine/core/heap.h"

// === Map arrays ===

// uc_orig: NS_texture (fallen/Source/ns.cpp)
NS_Texture NS_texture[NS_MAX_TEXTURES] = {
    { { 0, 32, 0, 32 }, { 0, 0, 32, 32 } },

    { { 0, 32, 0, 32 }, { 0, 0, 8, 8 } },
    { { 0, 32, 0, 32 }, { 0, 0, 12, 12 } },
    { { 0, 32, 0, 32 }, { 12, 12, 32, 32 } },
    { { 0, 32, 0, 24 }, { 0, 0, 12, 12 } },
    { { 0, 32, 8, 32 }, { 0, 0, 12, 12 } },
    { { 0, 24, 0, 12 }, { 12, 12, 32, 32 } },
    { { 8, 32, 20, 32 }, { 12, 12, 32, 32 } },
    { { 0, 0, 12, 0 }, { 12, 0, 12, 12 } },
    { { 0, 8, 0, 32 }, { 12, 12, 32, 32 } },
    { { 8, 0, 32, 0 }, { 12, 12, 32, 32 } },

    { { 0, 16, 0, 16 }, { 0, 0, 32, 32 } },
    { { 16, 32, 16, 32 }, { 0, 0, 32, 32 } },
    { { 0, 32, 0, 32 }, { 0, 0, 16, 16 } },
    { { 0, 32, 0, 32 }, { 16, 16, 32, 32 } },

    { { 0, 16, 0, 12 }, { 0, 0, 16, 12 } },
    { { 16, 32, 20, 32 }, { 0, 0, 12, 16 } },
    { { 0, 12, 0, 16 }, { 16, 20, 32, 32 } },
    { { 20, 32, 16, 32 }, { 20, 16, 32, 32 } },

    { { 0, 20, 0, 16 }, { 0, 20, 32, 32 } },
    { { 0, 32, 20, 32 }, { 0, 0, 20, 16 } },
    { { 0, 32, 0, 12 }, { 0, 0, 16, 20 } },
    { { 12, 32, 16, 32 }, { 20, 0, 32, 32 } },
    { { 0, 16, 0, 20 }, { 0, 0, 32, 12 } },
    { { 20, 32, 0, 32 }, { 12, 16, 32, 32 } },
    { { 0, 12, 0, 32 }, { 16, 12, 32, 32 } },
    { { 16, 32, 12, 32 }, { 0, 0, 12, 32 } },

    { { 16, 0, 24, 0 }, { 32, 32, 12, 0 } },
    { { 32, 24, 32, 0 }, { 8, 12, 0, 0 } },

    { { 32, 16, 32, 8 }, { 32, 32, 0, 12 } },
    { { 8, 0, 32, 0 }, { 12, 8, 0, 0 } },
};

// Number of pre-defined texture slots. New textures can be added up to NS_MAX_TEXTURES.
// Number of pre-defined texture entries in the static initializer above.
// NS_TEXTURE_NUMBER is defined in ns_globals.h.
// uc_orig: NS_texture_upto (fallen/Source/ns.cpp)
SLONG NS_texture_upto = NS_TEXTURE_NUMBER;

// Texture atlas page numbers for each sewer surface type.
// uc_orig: NS_page (fallen/Source/ns.cpp)
NS_Page NS_page[NS_PAGE_NUMBER] = {
    { 149 }, // Rock
    { 187 }, // Sewer
    { 189 }, // Stone
    { 187 }, // Sewer walls
    { 37 },  // Grating
};

// uc_orig: NS_lo (fallen/Source/ns.cpp)
NS_Lo NS_lo[PAP_SIZE_LO][PAP_SIZE_LO];
// uc_orig: NS_hi (fallen/Source/ns.cpp)
NS_Hi NS_hi[PAP_SIZE_HI][PAP_SIZE_HI];

// uc_orig: NS_cache (fallen/Source/ns.cpp)
NS_Cache NS_cache[NS_MAX_CACHES];
// uc_orig: NS_cache_free (fallen/Source/ns.cpp)
UBYTE NS_cache_free;

// uc_orig: NS_fall (fallen/Source/ns.cpp)
NS_Fall NS_fall[NS_MAX_FALLS];
// uc_orig: NS_fall_free (fallen/Source/ns.cpp)
UBYTE NS_fall_free;

// uc_orig: NS_st (fallen/Source/ns.cpp)
NS_St NS_st[NS_MAX_STS];
// uc_orig: NS_st_free (fallen/Source/ns.cpp)
UBYTE NS_st_free;

// === Scratch buffer pointers (into HEAP_pad) ===

// Both scratch arrays live in HEAP_pad: points in the first half, faces in the second.
// uc_orig: NS_scratch_point (fallen/Source/ns.cpp)
NS_Point* NS_scratch_point = (NS_Point*)(&HEAP_pad[0]);
// uc_orig: NS_scratch_face (fallen/Source/ns.cpp)
NS_Face* NS_scratch_face = (NS_Face*)(&HEAP_pad[HEAP_PAD_SIZE / 2]);

// uc_orig: NS_scratch_point_upto (fallen/Source/ns.cpp)
SLONG NS_scratch_point_upto = 0;
// uc_orig: NS_scratch_face_upto (fallen/Source/ns.cpp)
SLONG NS_scratch_face_upto = 0;

// uc_orig: NS_scratch_origin_x (fallen/Source/ns.cpp)
SLONG NS_scratch_origin_x;
// uc_orig: NS_scratch_origin_z (fallen/Source/ns.cpp)
SLONG NS_scratch_origin_z;

// uc_orig: NS_slight (fallen/Source/ns.cpp)
NS_Slight NS_slight[NS_MAX_SLIGHTS];
// uc_orig: NS_slight_upto (fallen/Source/ns.cpp)
SLONG NS_slight_upto;

// uc_orig: NS_search_start (fallen/Source/ns.cpp)
SLONG NS_search_start;
// uc_orig: NS_search_end (fallen/Source/ns.cpp)
SLONG NS_search_end;

// uc_orig: NS_los_fail_x (fallen/Source/ns.cpp)
SLONG NS_los_fail_x;
// uc_orig: NS_los_fail_y (fallen/Source/ns.cpp)
SLONG NS_los_fail_y;
// uc_orig: NS_los_fail_z (fallen/Source/ns.cpp)
SLONG NS_los_fail_z;
