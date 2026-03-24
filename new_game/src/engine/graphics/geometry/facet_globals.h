#ifndef ENGINE_GRAPHICS_GEOMETRY_FACET_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_FACET_GLOBALS_H

#include "engine/core/types.h"

// uc_orig: FACET_direction_matrix (fallen/DDEngine/Source/facet.cpp)
extern float FACET_direction_matrix[9];

// uc_orig: dfacets_drawn_this_gameturn (fallen/DDEngine/Source/facet.cpp)
extern SLONG dfacets_drawn_this_gameturn;

// uc_orig: iNumFacets (fallen/DDEngine/Source/facet.cpp)
extern int iNumFacets;

// uc_orig: iNumFacetTextures (fallen/DDEngine/Source/facet.cpp)
extern int iNumFacetTextures;

// uc_orig: facet_seed (fallen/DDEngine/Source/facet.cpp)
extern ULONG facet_seed;

// uc_orig: flip (fallen/DDEngine/Source/facet.cpp)
// Global side-effect channel between texture_quad() and FillFacetPoints().
extern SLONG flip;

// uc_orig: door_poly (fallen/DDEngine/Source/facet.cpp)
extern UBYTE door_poly[3][4];

// uc_orig: FacetRows (fallen/DDEngine/Source/facet.cpp)
extern SWORD FacetRows[100];

// uc_orig: FacetDiffY (fallen/DDEngine/Source/facet.cpp)
extern float FacetDiffY[128];

#endif // ENGINE_GRAPHICS_GEOMETRY_FACET_GLOBALS_H
