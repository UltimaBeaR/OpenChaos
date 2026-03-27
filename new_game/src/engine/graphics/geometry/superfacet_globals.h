#ifndef ENGINE_GRAPHICS_GEOMETRY_SUPERFACET_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_SUPERFACET_GLOBALS_H

#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_engine/graphics_engine.h"
#include "engine/graphics/graphics_engine/d3d/dd_manager.h"   // D3DLVERTEX, D3DMATRIX, LPDIRECT3DTEXTURE2
#include "engine/core/types.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"

// uc_orig: SUPERFACET_CALL_FLAG_USED (fallen/DDEngine/Source/superfacet.cpp)
#define SUPERFACET_CALL_FLAG_USED (1 << 0)
// uc_orig: SUPERFACET_CALL_FLAG_2PASS (fallen/DDEngine/Source/superfacet.cpp)
#define SUPERFACET_CALL_FLAG_2PASS (1 << 1)

// uc_orig: SUPERFACET_Call (fallen/DDEngine/Source/superfacet.cpp)
// Per-texture draw call descriptor accumulated during facet caching.
typedef struct
{
    UBYTE flag;         // SUPERFACET_CALL_FLAG_* bits
    UBYTE dir;          // Facing direction of the facet (0-3)
    UWORD quads;        // Number of quads in this call
    UWORD lvert;        // Start index into SUPERFACET_lvert[]
    UWORD lvertcount;   // Number of lit vertices used
    UWORD index;        // Start index into SUPERFACET_index[]
    UWORD indexcount;   // Number of indices used
    UWORD index2;       // For 2-pass textures (second index list start)

    LPDIRECT3DTEXTURE2 texture;       // Primary texture
    LPDIRECT3DTEXTURE2 texture_2pass; // Secondary texture for 2-pass pages

} SUPERFACET_Call;

// uc_orig: SUPERFACET_MAX_CALLS (fallen/DDEngine/Source/superfacet.cpp)
#define SUPERFACET_MAX_CALLS 2048

// uc_orig: SUPERFACET_Facet (fallen/DDEngine/Source/superfacet.cpp)
// Per-facet index into the call list; tracks which calls belong to this facet.
typedef struct
{
    UWORD call; // Index into SUPERFACET_call[] of the first call for this facet
    UWORD num;  // Number of calls for this facet

} SUPERFACET_Facet;

// uc_orig: SUPERFACET_QUEUE_SIZE (fallen/DDEngine/Source/superfacet.cpp)
#define SUPERFACET_QUEUE_SIZE 512

// uc_orig: SUPERFACET_max_lverts (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_max_lverts;
// uc_orig: SUPERFACET_lvert_buffer (fallen/DDEngine/Source/superfacet.cpp)
extern UBYTE* SUPERFACET_lvert_buffer;
// uc_orig: SUPERFACET_lvert (fallen/DDEngine/Source/superfacet.cpp)
extern D3DLVERTEX* SUPERFACET_lvert;
// uc_orig: SUPERFACET_lvert_upto (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_lvert_upto;

// uc_orig: SUPERFACET_max_indices (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_max_indices;
// uc_orig: SUPERFACET_index (fallen/DDEngine/Source/superfacet.cpp)
extern UWORD* SUPERFACET_index;
// uc_orig: SUPERFACET_index_upto (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_index_upto;

// uc_orig: SUPERFACET_free_range_start (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_free_range_start;
// uc_orig: SUPERFACET_free_range_end (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_free_range_end;

// uc_orig: SUPERFACET_call (fallen/DDEngine/Source/superfacet.cpp)
extern SUPERFACET_Call SUPERFACET_call[SUPERFACET_MAX_CALLS];
// uc_orig: SUPERFACET_call_upto (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_call_upto;

// uc_orig: SUPERFACET_max_facets (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_max_facets;
// uc_orig: SUPERFACET_facet (fallen/DDEngine/Source/superfacet.cpp)
extern SUPERFACET_Facet* SUPERFACET_facet;

// uc_orig: SUPERFACET_queue (fallen/DDEngine/Source/superfacet.cpp)
extern UWORD SUPERFACET_queue[SUPERFACET_QUEUE_SIZE];
// uc_orig: SUPERFACET_queue_start (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_queue_start;
// uc_orig: SUPERFACET_queue_end (fallen/DDEngine/Source/superfacet.cpp)
extern SLONG SUPERFACET_queue_end;

// uc_orig: SUPERFACET_matrix_buffer (fallen/DDEngine/Source/superfacet.cpp)
extern UBYTE SUPERFACET_matrix_buffer[sizeof(D3DMATRIX) + 32];
// uc_orig: SUPERFACET_matrix (fallen/DDEngine/Source/superfacet.cpp)
extern D3DMATRIX* SUPERFACET_matrix;

// uc_orig: SUPERFACET_direction_matrix (fallen/DDEngine/Source/superfacet.cpp)
extern float SUPERFACET_direction_matrix[4][9];

// uc_orig: SUPERFACET_colour_base (fallen/DDEngine/Source/superfacet.cpp)
// Pointer to the base of the NIGHT_Colour array for the current facet being processed.
// Used to compute vertex index (pp->user = col - base) for lighting updates.
extern NIGHT_Colour* SUPERFACET_colour_base;

// uc_orig: m_iFacetDirection (fallen/DDEngine/Source/superfacet.cpp)
// Cached direction index for the facet currently being processed (0-3).
extern int m_iFacetDirection;

#endif // ENGINE_GRAPHICS_GEOMETRY_SUPERFACET_GLOBALS_H
