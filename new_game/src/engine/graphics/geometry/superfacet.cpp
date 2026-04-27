#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

#include "game/game_types.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypoint.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "map/supermap.h"
#include "map/level_pools.h"
#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "engine/core/matrix.h"
#include "engine/graphics/geometry/superfacet.h"
#include "engine/graphics/geometry/superfacet_globals.h"

// POLY_set_local_rotation_none is defined as a no-op for the PC code path.
// On Dreamcast, this reset the local rotation matrix before DrawIndexedPrimitive.
// uc_orig: POLY_set_local_rotation_none (fallen/DDEngine/Source/superfacet.cpp)
#define POLY_set_local_rotation_none() \
    {                                  \
    }

// Defines for array bounds using the runtime-determined sizes (set in SUPERFACET_init).
#define SUPERFACET_MAX_LVERTS (SUPERFACET_max_lverts)
#define SUPERFACET_MAX_INDICES (SUPERFACET_max_indices)
#define SUPERFACET_MAX_FACETS SUPERFACET_max_facets

// facet.cpp entities used by superfacet rendering.
#include "engine/graphics/geometry/facet.h"
#include "engine/graphics/geometry/facet_globals.h"

// uc_orig: SUPERFACET_init (fallen/DDEngine/Headers/superfacet.h)
// Allocates and initialises the supefacet batch buffers. Call after all assets are loaded.
void SUPERFACET_init()
{
    SUPERFACET_max_facets = next_dfacet;
    SUPERFACET_max_lverts = 8192;
    SUPERFACET_max_indices = SUPERFACET_max_lverts * 6 / 4;

    SUPERFACET_lvert_buffer = (UBYTE*)MemAlloc(sizeof(GEVertexLit) * SUPERFACET_MAX_LVERTS + 32);
    ASSERT(SUPERFACET_lvert_buffer != NULL);
    SUPERFACET_index = (UWORD*)MemAlloc(sizeof(UWORD) * SUPERFACET_MAX_INDICES);
    ASSERT(SUPERFACET_index != NULL);
    SUPERFACET_facet = (SUPERFACET_Facet*)MemAlloc(sizeof(SUPERFACET_Facet) * SUPERFACET_MAX_FACETS);
    ASSERT(SUPERFACET_facet != NULL);

    memset(SUPERFACET_lvert_buffer, 0, sizeof(GEVertexLit) * SUPERFACET_MAX_LVERTS + 32);
    memset(SUPERFACET_index, 0, sizeof(UWORD) * SUPERFACET_MAX_INDICES);
    memset(SUPERFACET_call, 0, sizeof(SUPERFACET_call));
    memset(SUPERFACET_facet, 0, sizeof(SUPERFACET_Facet) * SUPERFACET_MAX_FACETS);
    memset(SUPERFACET_queue, 0, sizeof(SUPERFACET_queue));

    SUPERFACET_queue_start = 0;
    SUPERFACET_queue_end = 0;

    SUPERFACET_free_range_start = 0;
    SUPERFACET_free_range_end = SUPERFACET_MAX_LVERTS;

    // Align the lvert pointer to a 32-byte boundary for SIMD-friendly access.
    SUPERFACET_lvert = (GEVertexLit*)(((uintptr_t)(SUPERFACET_lvert_buffer) + 31) & ~(uintptr_t)0x1f);

    SUPERFACET_lvert_upto = 0;
    SUPERFACET_index_upto = 0;
    SUPERFACET_call_upto = 0;
    SUPERFACET_queue_start = 0;
    SUPERFACET_queue_end = 0;

    SUPERFACET_matrix = (GEMatrix*)(((uintptr_t)(SUPERFACET_matrix_buffer) + 31) & ~(uintptr_t)0x1f);

    // Build the four cardinal direction matrices (0°, 90°, 180°, 270°).
    SLONG i;

    for (i = 0; i < 4; i++) {
        MATRIX_calc(
            SUPERFACET_direction_matrix[i],
            float(i) * (PI / 2),
            0.0F,
            0.0F);
    }
}

// uc_orig: SUPERFACET_start_frame (fallen/DDEngine/Headers/superfacet.h)
// Clears the frame's batch buffers. Call once per frame before drawing.
void SUPERFACET_start_frame()
{
    POLY_set_local_rotation_none();
    POLY_flush_local_rot();
}

// uc_orig: SUPERFACET_fini (fallen/DDEngine/Headers/superfacet.h)
// Frees supefacet batch memory. Call at program shutdown.
void SUPERFACET_fini()
{
    MemFree(SUPERFACET_lvert_buffer);
    MemFree(SUPERFACET_index);
    MemFree(SUPERFACET_facet);
}
