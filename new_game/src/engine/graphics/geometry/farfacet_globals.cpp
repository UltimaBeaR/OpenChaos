#include "engine/graphics/geometry/farfacet_globals.h"

// uc_orig: FARFACET_lvert_buffer (fallen/DDEngine/Source/farfacet.cpp)
GEVertexLit* FARFACET_lvert_buffer;
// uc_orig: FARFACET_lvert (fallen/DDEngine/Source/farfacet.cpp)
GEVertexLit* FARFACET_lvert;
// uc_orig: FARFACET_lvert_max (fallen/DDEngine/Source/farfacet.cpp)
SLONG FARFACET_lvert_max;
// uc_orig: FARFACET_lvert_upto (fallen/DDEngine/Source/farfacet.cpp)
SLONG FARFACET_lvert_upto;

// uc_orig: FARFACET_index (fallen/DDEngine/Source/farfacet.cpp)
UWORD* FARFACET_index;
// uc_orig: FARFACET_index_max (fallen/DDEngine/Source/farfacet.cpp)
SLONG FARFACET_index_max;
// uc_orig: FARFACET_index_upto (fallen/DDEngine/Source/farfacet.cpp)
SLONG FARFACET_index_upto;

// uc_orig: FARFACET_square (fallen/DDEngine/Source/farfacet.cpp)
FARFACET_Square FARFACET_square[FARFACET_SIZE][FARFACET_SIZE];

// uc_orig: FARFACET_renderstate (fallen/DDEngine/Source/farfacet.cpp)
RenderState FARFACET_renderstate;
// uc_orig: FARFACET_default_renderstate (fallen/DDEngine/Source/farfacet.cpp)
RenderState FARFACET_default_renderstate;

// uc_orig: FARFACET_matrix_buffer (fallen/DDEngine/Source/farfacet.cpp)
UBYTE FARFACET_matrix_buffer[sizeof(GEMatrix) + 32];
// uc_orig: FARFACET_matrix (fallen/DDEngine/Source/farfacet.cpp)
GEMatrix* FARFACET_matrix;

// uc_orig: FARFACET_num_squares_drawn (fallen/DDEngine/Source/farfacet.cpp)
SLONG FARFACET_num_squares_drawn;

// uc_orig: FARFACET_outline (fallen/DDEngine/Source/farfacet.cpp)
FARFACET_Outline* FARFACET_outline;
// uc_orig: FARFACET_outline_upto (fallen/DDEngine/Source/farfacet.cpp)
SLONG FARFACET_outline_upto;
