// Fast prim renderer: caches vertex and index buffers per-prim to avoid rebuilding geometry
// each frame. Uses multi-matrix draw for opaque/tinted prims, indexed draw for alpha and
// environment-mapped ones.

#include "engine/graphics/geometry/fastprim.h"
#include "engine/graphics/geometry/fastprim_globals.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "buildings/prim_types.h"    // PrimFace3/4, PrimObject, FACE_FLAG_*, PRIM_FLAG_*

// uc_orig: FASTPRIM_init (fallen/DDEngine/Source/fastprim.cpp)
void FASTPRIM_init()
{
    FASTPRIM_lvert_max = 4096;

    FASTPRIM_lvert_buffer = (GEVertexLit*)MemAlloc(sizeof(GEVertexLit) * FASTPRIM_lvert_max + 31);
    FASTPRIM_lvert = (GEVertexLit*)((((uintptr_t)FASTPRIM_lvert_buffer) + 31) & ~(uintptr_t)0x1f);
    FASTPRIM_lvert_upto = 0;
    FASTPRIM_lvert_free_end = FASTPRIM_lvert_max;
    FASTPRIM_lvert_free_unused = FASTPRIM_lvert_max;

    FASTPRIM_index_max = FASTPRIM_lvert_max * 3;
    FASTPRIM_index = (UWORD*)MemAlloc(sizeof(UWORD) * FASTPRIM_index_max);
    FASTPRIM_index_upto = 0;
    FASTPRIM_index_free_end = FASTPRIM_index_max;
    FASTPRIM_index_free_unused = FASTPRIM_index_max;

    FASTPRIM_call_upto = 0;

    FASTPRIM_queue_start = 0;
    FASTPRIM_queue_end = 0;

    memset(FASTPRIM_lvert, 0, sizeof(GEVertexLit) * FASTPRIM_lvert_max);
    memset(FASTPRIM_index, 0, sizeof(UWORD) * FASTPRIM_index_max);
    memset(FASTPRIM_call, 0, sizeof(FASTPRIM_call));
    memset(FASTPRIM_prim, 0, sizeof(FASTPRIM_prim));
    memset(FASTPRIM_queue, 0, sizeof(FASTPRIM_queue));

    FASTPRIM_matrix = (GEMatrix*)(((uintptr_t)(FASTPRIM_matrix_buffer) + 31) & ~(uintptr_t)0x1f);

    // Car wheels have rotating texture coordinates and cannot be cached.
    FASTPRIM_prim[PRIM_OBJ_CAR_WHEEL].flag = FASTPRIM_PRIM_FLAG_INVALID;
}

// uc_orig: FASTPRIM_fini (fallen/DDEngine/Source/fastprim.cpp)
void FASTPRIM_fini()
{
    MemFree(FASTPRIM_lvert_buffer);
    MemFree(FASTPRIM_index);

    FASTPRIM_lvert_buffer = NULL;
    FASTPRIM_index = NULL;
}
