#include "engine/graphics/pipeline/aeng_globals.h"
#include "assets/compression.h" // Temporary: COMP_Frame

// RMAX_PRIM_POINTS = 65000 (from fallen/Headers/building.h)
#define RMAX_PRIM_POINTS 65000

// uc_orig: AENG_dx_prim_points (fallen/DDEngine/Source/aeng.cpp)
SVector_F AENG_dx_prim_points[RMAX_PRIM_POINTS];

// uc_orig: Lines (fallen/DDEngine/Source/aeng.cpp)
StoreLine Lines[MAX_LINES];

// uc_orig: next_line (fallen/DDEngine/Source/aeng.cpp)
SLONG next_line = 0;

// uc_orig: AENG_cone (fallen/DDEngine/Source/aeng.cpp)
AENG_ConePoint AENG_cone[5];

// uc_orig: aeng_draw_cloud_flag (fallen/DDEngine/Source/aeng.cpp)
UBYTE aeng_draw_cloud_flag = 1;

// uc_orig: light_inside (fallen/DDEngine/Source/aeng.cpp)
UWORD light_inside = 0;

// uc_orig: fade_black (fallen/DDEngine/Source/aeng.cpp)
UWORD fade_black = 1;

// uc_orig: cloud_data (fallen/DDEngine/Source/aeng.cpp)
UBYTE cloud_data[32][32];

// uc_orig: cloud_x (fallen/DDEngine/Source/aeng.cpp)
SLONG cloud_x;

// uc_orig: cloud_z (fallen/DDEngine/Source/aeng.cpp)
SLONG cloud_z;

// uc_orig: FirstPersonAlpha (fallen/DDEngine/Source/aeng.cpp)
SLONG FirstPersonAlpha = 255;

// uc_orig: AENG_total_polys_drawn (fallen/DDEngine/Source/aeng.cpp)
int AENG_total_polys_drawn;

// uc_orig: AENG_detail_stars (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_stars = 1;

// uc_orig: AENG_detail_shadows (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_shadows = 1;

// uc_orig: AENG_detail_moon_reflection (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_moon_reflection = 1;

// uc_orig: AENG_detail_people_reflection (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_people_reflection = 1;

// uc_orig: AENG_detail_puddles (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_puddles = 1;

// uc_orig: AENG_detail_dirt (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_dirt = 1;

// uc_orig: AENG_detail_mist (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_mist = 1;

// uc_orig: AENG_detail_rain (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_rain = 1;

// uc_orig: AENG_detail_skyline (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_skyline = 1;

// uc_orig: AENG_detail_filter (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_filter = 1;

// uc_orig: AENG_detail_perspective (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_perspective = 1;

// uc_orig: AENG_detail_crinkles (fallen/DDEngine/Source/aeng.cpp)
int AENG_detail_crinkles = 1;

// uc_orig: AENG_estimate_detail_levels (fallen/DDEngine/Source/aeng.cpp)
int AENG_estimate_detail_levels = 1;

// uc_orig: AENG_cur_fc_cam (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_cur_fc_cam;

// uc_orig: global_b (fallen/DDEngine/Source/aeng.cpp)
SLONG global_b = 0;

// uc_orig: AENG_lens (fallen/DDEngine/Source/aeng.cpp)
float AENG_lens = 4.0F;

// uc_orig: CurDrawDistance (fallen/DDEngine/Source/aeng.cpp)
SLONG CurDrawDistance = 22 << 8;

// uc_orig: AENG_cam_x (fallen/DDEngine/Source/aeng.cpp)
float AENG_cam_x;

// uc_orig: AENG_cam_y (fallen/DDEngine/Source/aeng.cpp)
float AENG_cam_y;

// uc_orig: AENG_cam_z (fallen/DDEngine/Source/aeng.cpp)
float AENG_cam_z;

// uc_orig: AENG_cam_yaw (fallen/DDEngine/Source/aeng.cpp)
float AENG_cam_yaw;

// uc_orig: AENG_cam_pitch (fallen/DDEngine/Source/aeng.cpp)
float AENG_cam_pitch;

// uc_orig: AENG_cam_roll (fallen/DDEngine/Source/aeng.cpp)
float AENG_cam_roll;

// uc_orig: AENG_cam_matrix (fallen/DDEngine/Source/aeng.cpp)
float AENG_cam_matrix[9];

// uc_orig: AENG_cam_vec (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_cam_vec[3];

// uc_orig: AENG_sewer_page (fallen/DDEngine/Source/aeng.cpp)
// Temporary: POLY_PAGE_WATER and SEWER_PAGE_NUMBER come from not-yet-migrated headers.
#include "fallen/Headers/sewer.h"
#include "engine/graphics/pipeline/poly_globals.h"
SLONG AENG_sewer_page[SEWER_PAGE_NUMBER] = {
    3,
    4,
    5,
    POLY_PAGE_WATER
};

// uc_orig: AENG_movie_data (fallen/DDEngine/Source/aeng.cpp)
UBYTE AENG_movie_data[AENG_MAX_MOVIE_DATA];

// uc_orig: AENG_movie_upto (fallen/DDEngine/Source/aeng.cpp)
UBYTE* AENG_movie_upto;

// uc_orig: AENG_frame_one (fallen/DDEngine/Source/aeng.cpp)
COMP_Frame AENG_frame_one;

// uc_orig: AENG_frame_two (fallen/DDEngine/Source/aeng.cpp)
COMP_Frame AENG_frame_two;

// uc_orig: AENG_frame_last (fallen/DDEngine/Source/aeng.cpp)
COMP_Frame* AENG_frame_last = &AENG_frame_one;

// uc_orig: AENG_frame_next (fallen/DDEngine/Source/aeng.cpp)
COMP_Frame* AENG_frame_next = &AENG_frame_two;

// uc_orig: AENG_frame_count (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_frame_count;

// uc_orig: AENG_frame_tick (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_frame_tick;

// uc_orig: AENG_frame_number (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_frame_number;

// uc_orig: AENG_gamut_lo_xmin (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_gamut_lo_xmin;

// uc_orig: AENG_gamut_lo_xmax (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_gamut_lo_xmax;

// uc_orig: AENG_gamut_lo_zmin (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_gamut_lo_zmin;

// uc_orig: AENG_gamut_lo_zmax (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_gamut_lo_zmax;

// uc_orig: AENG_project_offset_u (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_offset_u;

// uc_orig: AENG_project_offset_v (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_offset_v;

// uc_orig: AENG_project_mul_u (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_mul_u;

// uc_orig: AENG_project_mul_v (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_mul_v;

// uc_orig: AENG_project_lit_light_x (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_lit_light_x;

// uc_orig: AENG_project_lit_light_y (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_lit_light_y;

// uc_orig: AENG_project_lit_light_z (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_lit_light_z;

// uc_orig: AENG_project_lit_light_range (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_lit_light_range;

// uc_orig: AENG_project_fadeout_x (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_fadeout_x;

// uc_orig: AENG_project_fadeout_z (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_fadeout_z;
