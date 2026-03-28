#ifndef ENGINE_GRAPHICS_GEOMETRY_FARFACET_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_FARFACET_GLOBALS_H

#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"   // GEVertexLit, GEMatrix
#include "engine/core/types.h"
#include "map/pap.h"

// uc_orig: FARFACET_RATIO (fallen/DDEngine/Source/farfacet.cpp)
#define FARFACET_RATIO (4)
// uc_orig: FARFACET_SIZE (fallen/DDEngine/Source/farfacet.cpp)
#define FARFACET_SIZE (PAP_SIZE_LO / FARFACET_RATIO)

// Per-square prebuilt indexed geometry record.
// uc_orig: FARFACET_Square (fallen/DDEngine/Source/farfacet.cpp)
typedef struct
{
    UWORD lvert;
    UWORD lvertcount;
    UWORD index;
    UWORD indexcount;

} FARFACET_Square;

// uc_orig: FARFACET_lvert_buffer (fallen/DDEngine/Source/farfacet.cpp)
extern GEVertexLit* FARFACET_lvert_buffer;
// uc_orig: FARFACET_lvert (fallen/DDEngine/Source/farfacet.cpp)
extern GEVertexLit* FARFACET_lvert;
// uc_orig: FARFACET_lvert_max (fallen/DDEngine/Source/farfacet.cpp)
extern SLONG FARFACET_lvert_max;
// uc_orig: FARFACET_lvert_upto (fallen/DDEngine/Source/farfacet.cpp)
extern SLONG FARFACET_lvert_upto;

// uc_orig: FARFACET_index (fallen/DDEngine/Source/farfacet.cpp)
extern UWORD* FARFACET_index;
// uc_orig: FARFACET_index_max (fallen/DDEngine/Source/farfacet.cpp)
extern SLONG FARFACET_index_max;
// uc_orig: FARFACET_index_upto (fallen/DDEngine/Source/farfacet.cpp)
extern SLONG FARFACET_index_upto;

// uc_orig: FARFACET_square (fallen/DDEngine/Source/farfacet.cpp)
extern FARFACET_Square FARFACET_square[FARFACET_SIZE][FARFACET_SIZE];

// uc_orig: FARFACET_renderstate (fallen/DDEngine/Source/farfacet.cpp)
extern RenderState FARFACET_renderstate;
// uc_orig: FARFACET_default_renderstate (fallen/DDEngine/Source/farfacet.cpp)
extern RenderState FARFACET_default_renderstate;

// uc_orig: FARFACET_matrix_buffer (fallen/DDEngine/Source/farfacet.cpp)
extern UBYTE FARFACET_matrix_buffer[sizeof(GEMatrix) + 32];
// uc_orig: FARFACET_matrix (fallen/DDEngine/Source/farfacet.cpp)
extern GEMatrix* FARFACET_matrix;

// uc_orig: FARFACET_num_squares_drawn (fallen/DDEngine/Source/farfacet.cpp)
extern SLONG FARFACET_num_squares_drawn;

// Intermediate outline record used only during FARFACET_init. Freed after init.
// uc_orig: FARFACET_Outline (fallen/DDEngine/Source/farfacet.cpp)
typedef struct
{
    UBYTE x1;
    SBYTE y1;
    UBYTE z1;

    UBYTE x2;
    SBYTE y2;
    UBYTE z2;

} FARFACET_Outline;

// uc_orig: FARFACET_MAX_OUTLINES (fallen/DDEngine/Source/farfacet.cpp)
#define FARFACET_MAX_OUTLINES (8192 / ((32 / FARFACET_RATIO) * (32 / FARFACET_RATIO)) + 256)

// uc_orig: FARFACET_outline (fallen/DDEngine/Source/farfacet.cpp)
extern FARFACET_Outline* FARFACET_outline;
// uc_orig: FARFACET_outline_upto (fallen/DDEngine/Source/farfacet.cpp)
extern SLONG FARFACET_outline_upto;

#endif // ENGINE_GRAPHICS_GEOMETRY_FARFACET_GLOBALS_H
