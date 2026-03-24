#ifndef ENGINE_LIGHTING_NGAMUT_GLOBALS_H
#define ENGINE_LIGHTING_NGAMUT_GLOBALS_H

#include "engine/platform/uc_common.h"
#include "ngamut.h"

// uc_orig: NGAMUT_gamut (fallen/DDEngine/Source/NGamut.cpp)
extern NGAMUT_Gamut NGAMUT_gamut[NGAMUT_SIZE];
// uc_orig: NGAMUT_xmin (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_xmin;
// uc_orig: NGAMUT_zmin (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_zmin;
// uc_orig: NGAMUT_zmax (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_zmax;

// uc_orig: NGAMUT_point_gamut (fallen/DDEngine/Source/NGamut.cpp)
extern NGAMUT_Gamut NGAMUT_point_gamut[NGAMUT_SIZE];
// uc_orig: NGAMUT_point_zmin (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_point_zmin;
// uc_orig: NGAMUT_point_zmax (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_point_zmax;

// uc_orig: NGAMUT_out_gamut (fallen/DDEngine/Source/NGamut.cpp)
extern NGAMUT_Gamut NGAMUT_out_gamut[NGAMUT_SIZE];
// uc_orig: NGAMUT_out_zmin (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_out_zmin;
// uc_orig: NGAMUT_out_zmax (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_out_zmax;

// uc_orig: NGAMUT_lo_gamut (fallen/DDEngine/Source/NGamut.cpp)
extern NGAMUT_Gamut NGAMUT_lo_gamut[NGAMUT_SIZE_LO];
// uc_orig: NGAMUT_lo_zmin (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_lo_zmin;
// uc_orig: NGAMUT_lo_zmax (fallen/DDEngine/Source/NGamut.cpp)
extern SLONG NGAMUT_lo_zmax;

#endif // ENGINE_LIGHTING_NGAMUT_GLOBALS_H
