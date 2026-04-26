#include "engine/graphics/geometry/superfacet_globals.h"

// uc_orig: SUPERFACET_max_lverts (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_max_lverts = 0;

// uc_orig: SUPERFACET_lvert_buffer (fallen/DDEngine/Source/superfacet.cpp)
UBYTE* SUPERFACET_lvert_buffer = nullptr;

// uc_orig: SUPERFACET_lvert (fallen/DDEngine/Source/superfacet.cpp)
GEVertexLit* SUPERFACET_lvert = nullptr;

// uc_orig: SUPERFACET_lvert_upto (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_lvert_upto = 0;

// uc_orig: SUPERFACET_max_indices (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_max_indices = 0;

// uc_orig: SUPERFACET_index (fallen/DDEngine/Source/superfacet.cpp)
UWORD* SUPERFACET_index = nullptr;

// uc_orig: SUPERFACET_index_upto (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_index_upto = 0;

// uc_orig: SUPERFACET_free_range_start (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_free_range_start = 0;

// uc_orig: SUPERFACET_free_range_end (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_free_range_end = 0;

// uc_orig: SUPERFACET_call (fallen/DDEngine/Source/superfacet.cpp)
SUPERFACET_Call SUPERFACET_call[SUPERFACET_MAX_CALLS] = {};

// uc_orig: SUPERFACET_call_upto (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_call_upto = 0;

// uc_orig: SUPERFACET_max_facets (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_max_facets = 0;

// uc_orig: SUPERFACET_facet (fallen/DDEngine/Source/superfacet.cpp)
SUPERFACET_Facet* SUPERFACET_facet = nullptr;

// uc_orig: SUPERFACET_queue (fallen/DDEngine/Source/superfacet.cpp)
UWORD SUPERFACET_queue[SUPERFACET_QUEUE_SIZE] = {};

// uc_orig: SUPERFACET_queue_start (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_queue_start = 0;

// uc_orig: SUPERFACET_queue_end (fallen/DDEngine/Source/superfacet.cpp)
SLONG SUPERFACET_queue_end = 0;

// uc_orig: SUPERFACET_matrix_buffer (fallen/DDEngine/Source/superfacet.cpp)
UBYTE SUPERFACET_matrix_buffer[sizeof(GEMatrix) + 32] = {};

// uc_orig: SUPERFACET_matrix (fallen/DDEngine/Source/superfacet.cpp)
GEMatrix* SUPERFACET_matrix = nullptr;

// uc_orig: SUPERFACET_direction_matrix (fallen/DDEngine/Source/superfacet.cpp)
float SUPERFACET_direction_matrix[4][9] = {};
