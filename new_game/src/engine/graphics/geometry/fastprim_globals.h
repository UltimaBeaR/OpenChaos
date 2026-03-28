#ifndef ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_GLOBALS_H

#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/core/types.h"

// D3D vertex/index buffer for all batched prim draws.
// uc_orig: FASTPRIM_lvert_buffer (fallen/DDEngine/Source/fastprim.cpp)
extern GEVertexLit* FASTPRIM_lvert_buffer;
// uc_orig: FASTPRIM_lvert (fallen/DDEngine/Source/fastprim.cpp)
extern GEVertexLit* FASTPRIM_lvert;
// uc_orig: FASTPRIM_lvert_max (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_lvert_max;
// uc_orig: FASTPRIM_lvert_upto (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_lvert_upto;
// uc_orig: FASTPRIM_lvert_free_end (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_lvert_free_end;
// uc_orig: FASTPRIM_lvert_free_unused (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_lvert_free_unused;

// uc_orig: FASTPRIM_index (fallen/DDEngine/Source/fastprim.cpp)
extern UWORD* FASTPRIM_index;
// uc_orig: FASTPRIM_index_max (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_index_max;
// uc_orig: FASTPRIM_index_upto (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_index_upto;
// uc_orig: FASTPRIM_index_free_end (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_index_free_end;
// uc_orig: FASTPRIM_index_free_unused (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_index_free_unused;

// 32-byte-aligned D3D matrix for DrawIndPrimMM calls.
// uc_orig: FASTPRIM_matrix_buffer (fallen/DDEngine/Source/fastprim.cpp)
extern UBYTE FASTPRIM_matrix_buffer[sizeof(GEMatrix) + 32];
// uc_orig: FASTPRIM_matrix (fallen/DDEngine/Source/fastprim.cpp)
extern GEMatrix* FASTPRIM_matrix;

// Call descriptor: one per texture/type combination within a single cached prim.
// uc_orig: FASTPRIM_Call (fallen/DDEngine/Source/fastprim.cpp)
struct FASTPRIM_Call {
    UWORD flag;
    UWORD type;
    UWORD lvert;
    UWORD lvertcount;
    UWORD index;
    UWORD indexcount;
    GETextureHandle texture;
};

// uc_orig: FASTPRIM_MAX_CALLS (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_MAX_CALLS 512

// uc_orig: FASTPRIM_call (fallen/DDEngine/Source/fastprim.cpp)
extern FASTPRIM_Call FASTPRIM_call[FASTPRIM_MAX_CALLS];
// uc_orig: FASTPRIM_call_upto (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_call_upto;

// Per-prim cache entry.
// uc_orig: FASTPRIM_Prim (fallen/DDEngine/Source/fastprim.cpp)
struct FASTPRIM_Prim {
    UWORD flag;
    UWORD call_index;
    UWORD call_count;
};

// uc_orig: FASTPRIM_MAX_PRIMS (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_MAX_PRIMS 256

// uc_orig: FASTPRIM_prim (fallen/DDEngine/Source/fastprim.cpp)
extern FASTPRIM_Prim FASTPRIM_prim[FASTPRIM_MAX_PRIMS];

// Circular queue of recently-cached prim indices for LRU eviction.
// uc_orig: FASTPRIM_MAX_QUEUE (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_MAX_QUEUE 128

// uc_orig: FASTPRIM_queue (fallen/DDEngine/Source/fastprim.cpp)
extern UBYTE FASTPRIM_queue[FASTPRIM_MAX_QUEUE];
// uc_orig: FASTPRIM_queue_start (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_queue_start;
// uc_orig: FASTPRIM_queue_end (fallen/DDEngine/Source/fastprim.cpp)
extern SLONG FASTPRIM_queue_end;

// Call type constants.
// uc_orig: FASTPRIM_CALL_TYPE_NORMAL (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_CALL_TYPE_NORMAL    0
// uc_orig: FASTPRIM_CALL_TYPE_COLOURAND (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_CALL_TYPE_COLOURAND 1
// uc_orig: FASTPRIM_CALL_TYPE_INDEXED (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_CALL_TYPE_INDEXED   2
// uc_orig: FASTPRIM_CALL_TYPE_ENVMAP (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_CALL_TYPE_ENVMAP    3

// uc_orig: FASTPRIM_CALL_FLAG_SELF_ILLUM (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_CALL_FLAG_SELF_ILLUM (1 << 0)

// uc_orig: FASTPRIM_PRIM_FLAG_CACHED (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_PRIM_FLAG_CACHED  (1 << 0)
// uc_orig: FASTPRIM_PRIM_FLAG_INVALID (fallen/DDEngine/Source/fastprim.cpp)
#define FASTPRIM_PRIM_FLAG_INVALID (1 << 1)

#endif // ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_GLOBALS_H
