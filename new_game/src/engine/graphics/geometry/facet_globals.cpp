#include "engine/graphics/geometry/facet_globals.h"

// uc_orig: FACET_direction_matrix (fallen/DDEngine/Source/facet.cpp)
float FACET_direction_matrix[9];

// uc_orig: dfacets_drawn_this_gameturn (fallen/DDEngine/Source/facet.cpp)
SLONG dfacets_drawn_this_gameturn;

// uc_orig: facet_seed (fallen/DDEngine/Source/facet.cpp)
ULONG facet_seed = 0x12345678;

// uc_orig: flip (fallen/DDEngine/Source/facet.cpp)
SLONG flip;

// uc_orig: door_poly (fallen/DDEngine/Source/facet.cpp)
// Vertex index table for the 3-panel door geometry (indices into POLY_buffer).
UBYTE door_poly[3][4] = {
    { 0, 1, 7, 6 },
    { 1, 2, 6, 5 },
    { 2, 3, 5, 4 }
};

// uc_orig: FacetRows (fallen/DDEngine/Source/facet.cpp)
SWORD FacetRows[100];

// uc_orig: FacetDiffY (fallen/DDEngine/Source/facet.cpp)
float FacetDiffY[128];
