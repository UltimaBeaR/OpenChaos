#include "engine/graphics/pipeline/aeng_globals.h"

// uc_orig: AENG_dx_prim_points (fallen/DDEngine/Source/aeng.cpp)
SVector_F AENG_dx_prim_points[RMAX_PRIM_POINTS];

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

// uc_orig: AENG_cur_fc_cam (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_cur_fc_cam;

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

// uc_orig: AENG_project_offset_u (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_offset_u;

// uc_orig: AENG_project_offset_v (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_offset_v;

// uc_orig: AENG_project_mul_u (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_mul_u;

// uc_orig: AENG_project_mul_v (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_mul_v;

// uc_orig: AENG_project_fadeout_x (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_fadeout_x;

// uc_orig: AENG_project_fadeout_z (fallen/DDEngine/Source/aeng.cpp)
float AENG_project_fadeout_z;

// ---------------------------------------------------------------------------
// Chunk 2 globals: dirt/debris, pow (explosion sprites)
// ---------------------------------------------------------------------------

// uc_orig: AENG_pow (fallen/DDEngine/Source/aeng.cpp)
AENG_Pow AENG_pow[AENG_MAX_POWS];

// uc_orig: AENG_pow_upto (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_pow_upto;

// uc_orig: AENG_pow_bucket (fallen/DDEngine/Source/aeng.cpp)
AENG_Pow* AENG_pow_bucket[AENG_POW_NUM_BUCKETS];

// ---------------------------------------------------------------------------
// Chunk 3 globals: sky, rect queue, floor tile infrastructure
// ---------------------------------------------------------------------------

// uc_orig: AENG_aa_buffer (fallen/DDEngine/Source/aeng.cpp)
UBYTE AENG_aa_buffer[AENG_AA_BUF_SIZE][AENG_AA_BUF_SIZE];

// uc_orig: AENG_upper (fallen/DDEngine/Source/aeng.cpp)
POLY_Point AENG_upper[MAP_WIDTH / 2 + MAP_SIZE_TWEAK][MAP_HEIGHT / 2 + MAP_SIZE_TWEAK];

// uc_orig: AENG_lower (fallen/DDEngine/Source/aeng.cpp)
POLY_Point AENG_lower[MAP_WIDTH / 2 + MAP_SIZE_TWEAK * 2][MAP_HEIGHT / 2 + MAP_SIZE_TWEAK * 2];

// uc_orig: AENG_torch_on (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_torch_on = UC_FALSE;

// uc_orig: AENG_shadows_on (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_shadows_on = UC_TRUE;

// uc_orig: rrect (fallen/DDEngine/Source/aeng.cpp)
RRect rrect[2000];

// uc_orig: next_rrect (fallen/DDEngine/Source/aeng.cpp)
SLONG next_rrect = 1;

// uc_orig: index_lookup (fallen/DDEngine/Source/aeng.cpp)
UBYTE index_lookup[] = { 0, 1, 3, 2 };

// uc_orig: record_video (fallen/DDEngine/Source/aeng.cpp)
UBYTE record_video = 0;

// uc_orig: AENG_transparent_warehouses (fallen/DDEngine/Source/aeng.cpp)
UBYTE AENG_transparent_warehouses = 0;

// uc_orig: AENG_drawing_a_warehouse (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_drawing_a_warehouse = 0;
