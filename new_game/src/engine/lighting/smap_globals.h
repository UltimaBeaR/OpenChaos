#ifndef ENGINE_LIGHTING_SMAP_GLOBALS_H
#define ENGINE_LIGHTING_SMAP_GLOBALS_H

#include "engine/core/types.h"
#include "engine/lighting/smap.h"

// Internal point structure used during shadow map construction.
// uc_orig: SMAP_Point (fallen/DDEngine/Source/smap.cpp)
typedef struct {
    float along_u;
    float along_v;
    float along_n;

    float world_x;
    float world_y;
    float world_z;

    SLONG u; // Coordinates on the bitmap in fixed-point 16.
    SLONG v;

} SMAP_Point;

// uc_orig: SMAP_MAX_POINTS (fallen/DDEngine/Source/smap.cpp)
#define SMAP_MAX_POINTS 2048

// uc_orig: SMAP_plane_ux (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_ux;
// uc_orig: SMAP_plane_uy (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_uy;
// uc_orig: SMAP_plane_uz (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_uz;

// uc_orig: SMAP_plane_vx (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_vx;
// uc_orig: SMAP_plane_vy (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_vy;
// uc_orig: SMAP_plane_vz (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_vz;

// uc_orig: SMAP_plane_nx (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_nx;
// uc_orig: SMAP_plane_ny (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_ny;
// uc_orig: SMAP_plane_nz (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_plane_nz;

// uc_orig: SMAP_u_min (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_u_min;
// uc_orig: SMAP_u_max (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_u_max;

// uc_orig: SMAP_v_min (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_v_min;
// uc_orig: SMAP_v_max (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_v_max;

// uc_orig: SMAP_n_min (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_n_min;
// uc_orig: SMAP_n_max (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_n_max;

// uc_orig: SMAP_u_map_mul_float (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_u_map_mul_float;
// uc_orig: SMAP_v_map_mul_float (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_v_map_mul_float;

// uc_orig: SMAP_u_map_add_float (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_u_map_add_float;
// uc_orig: SMAP_v_map_add_float (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_v_map_add_float;

// uc_orig: SMAP_u_map_mul_slong (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_u_map_mul_slong;
// uc_orig: SMAP_v_map_mul_slong (fallen/DDEngine/Source/smap.cpp)
extern float SMAP_v_map_mul_slong;

// uc_orig: SMAP_bitmap (fallen/DDEngine/Source/smap.cpp)
extern UBYTE* SMAP_bitmap;
// uc_orig: SMAP_res_u (fallen/DDEngine/Source/smap.cpp)
extern SLONG SMAP_res_u;
// uc_orig: SMAP_res_v (fallen/DDEngine/Source/smap.cpp)
extern SLONG SMAP_res_v;

// uc_orig: SMAP_point (fallen/DDEngine/Source/smap.cpp)
extern SMAP_Point SMAP_point[SMAP_MAX_POINTS];
// uc_orig: SMAP_point_upto (fallen/DDEngine/Source/smap.cpp)
extern SLONG SMAP_point_upto;

// uc_orig: SMAP_MAX_LINKS (fallen/DDEngine/Source/smap.cpp)
#define SMAP_MAX_LINKS 16

// uc_orig: SMAP_link (fallen/DDEngine/Source/smap.cpp)
extern SMAP_Link SMAP_link[SMAP_MAX_LINKS];
// uc_orig: SMAP_link_upto (fallen/DDEngine/Source/smap.cpp)
extern SLONG SMAP_link_upto;

#endif // ENGINE_LIGHTING_SMAP_GLOBALS_H
