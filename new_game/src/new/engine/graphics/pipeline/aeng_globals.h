#ifndef ENGINE_GRAPHICS_PIPELINE_AENG_GLOBALS_H
#define ENGINE_GRAPHICS_PIPELINE_AENG_GLOBALS_H

#include "core/types.h"
#include "engine/graphics/pipeline/aeng.h"
#include "assets/compression.h" // Temporary: COMP_Frame type used in movie playback

// uc_orig: StoreLine (fallen/DDEngine/Source/aeng.cpp)
// One entry in the debug line draw list.
struct StoreLine
{
    SLONG x1, y1, z1, x2, y2, z2;
    ULONG col;
};

// uc_orig: MAX_LINES (fallen/DDEngine/Source/aeng.cpp)
#define MAX_LINES 100

// uc_orig: Lines (fallen/DDEngine/Source/aeng.cpp)
extern StoreLine Lines[MAX_LINES];

// uc_orig: next_line (fallen/DDEngine/Source/aeng.cpp)
extern SLONG next_line;

// Renamed from anonymous struct to AENG_ConePoint to allow external linkage.
// uc_orig: AENG_cone (fallen/DDEngine/Source/aeng.cpp)
struct AENG_ConePoint
{
    float x;
    float y;
    float z;
};

// uc_orig: AENG_cone (fallen/DDEngine/Source/aeng.cpp)
// Five corners of the view frustum pyramid in world space (cam origin + 4 far corners).
extern AENG_ConePoint AENG_cone[5];

// uc_orig: aeng_draw_cloud_flag (fallen/DDEngine/Source/aeng.cpp)
// When non-zero, cloud shadowing is applied to vertices. Can be disabled via debug keys.
extern UBYTE aeng_draw_cloud_flag;

// uc_orig: light_inside (fallen/DDEngine/Source/aeng.cpp)
// Tracks which indoor section (INDOORS_INDEX) was used to build the last cached lighting.
extern UWORD light_inside;

// uc_orig: fade_black (fallen/DDEngine/Source/aeng.cpp)
// When non-zero, POLY_frame_draw applies a full-screen black fade-out.
extern UWORD fade_black;

// uc_orig: cloud_data (fallen/DDEngine/Source/aeng.cpp)
// 32x32 greyscale cloud-shadow texture loaded from data\cloud.raw.
extern UBYTE cloud_data[32][32];

// uc_orig: cloud_x (fallen/DDEngine/Source/aeng.cpp)
// Cloud scroll offset on the X axis (accumulates each frame via move_clouds).
extern SLONG cloud_x;

// uc_orig: cloud_z (fallen/DDEngine/Source/aeng.cpp)
// Cloud scroll offset on the Z axis.
extern SLONG cloud_z;

// uc_orig: FirstPersonAlpha (fallen/DDEngine/Source/aeng.cpp)
// Opacity of the first-person view fade overlay (255 = opaque, 0 = transparent).
extern SLONG FirstPersonAlpha;

// uc_orig: AENG_total_polys_drawn (fallen/DDEngine/Source/aeng.cpp)
// Running count of polygons submitted this frame (debug display).
extern int AENG_total_polys_drawn;

// uc_orig: AENG_detail_stars (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_stars;

// uc_orig: AENG_detail_shadows (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_shadows;

// uc_orig: AENG_detail_moon_reflection (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_moon_reflection;

// uc_orig: AENG_detail_people_reflection (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_people_reflection;

// uc_orig: AENG_detail_puddles (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_puddles;

// uc_orig: AENG_detail_dirt (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_dirt;

// uc_orig: AENG_detail_mist (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_mist;

// uc_orig: AENG_detail_rain (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_rain;

// uc_orig: AENG_detail_skyline (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_skyline;

// uc_orig: AENG_detail_filter (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_filter;

// uc_orig: AENG_detail_perspective (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_perspective;

// uc_orig: AENG_detail_crinkles (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_crinkles;

// uc_orig: AENG_estimate_detail_levels (fallen/DDEngine/Source/aeng.cpp)
// When non-zero, detail levels are auto-adjusted based on frame performance.
extern int AENG_estimate_detail_levels;

// uc_orig: AENG_cur_fc_cam (fallen/DDEngine/Source/aeng.cpp)
// Index of the currently active FC_cam (0 = player 1, 1 = player 2 in splitscreen).
extern SLONG AENG_cur_fc_cam;

// uc_orig: global_b (fallen/DDEngine/Source/aeng.cpp)
// Pre-computed cloud shadow intensity at the last calc_global_cloud position (0..255).
extern SLONG global_b;

// uc_orig: AENG_lens (fallen/DDEngine/Source/aeng.cpp)
// Camera lens factor controlling field of view (higher = narrower FOV).
extern float AENG_lens;

// uc_orig: CurDrawDistance (fallen/DDEngine/Source/aeng.cpp)
// Current draw distance in map units (fixed-point, shifted left by 8).
extern SLONG CurDrawDistance;

// uc_orig: AENG_cam_x (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_cam_x;

// uc_orig: AENG_cam_y (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_cam_y;

// uc_orig: AENG_cam_z (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_cam_z;

// uc_orig: AENG_cam_yaw (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_cam_yaw;

// uc_orig: AENG_cam_pitch (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_cam_pitch;

// uc_orig: AENG_cam_roll (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_cam_roll;

// uc_orig: AENG_cam_matrix (fallen/DDEngine/Source/aeng.cpp)
// 3x3 rotation matrix built from AENG_cam_yaw/pitch/roll.
extern float AENG_cam_matrix[9];

// uc_orig: AENG_cam_vec (fallen/DDEngine/Source/aeng.cpp)
// Camera forward vector in 16.16 fixed-point.
extern SLONG AENG_cam_vec[3];

// uc_orig: AENG_sewer_page (fallen/DDEngine/Source/aeng.cpp)
// Texture page indices used for sewer geometry.
extern SLONG AENG_sewer_page[];

// uc_orig: AENG_MAX_MOVIE_DATA (fallen/DDEngine/Source/aeng.cpp)
// Maximum size of the pre-rendered movie video data buffer (512 KB).
#define AENG_MAX_MOVIE_DATA (512 * 1024)

// uc_orig: AENG_movie_data (fallen/DDEngine/Source/aeng.cpp)
// Buffer for the pre-rendered "movie" video texture data (bond.mmv).
extern UBYTE AENG_movie_data[AENG_MAX_MOVIE_DATA];

// uc_orig: AENG_movie_upto (fallen/DDEngine/Source/aeng.cpp)
// Read pointer into AENG_movie_data for the current playback position.
extern UBYTE* AENG_movie_upto;

// uc_orig: AENG_frame_count (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_frame_count;

// uc_orig: AENG_frame_tick (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_frame_tick;

// uc_orig: AENG_frame_number (fallen/DDEngine/Source/aeng.cpp)
// Total number of frames in the loaded movie (0 = no movie loaded).
extern SLONG AENG_frame_number;

// uc_orig: AENG_gamut_lo_xmin (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_gamut_lo_xmin;

// uc_orig: AENG_gamut_lo_xmax (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_gamut_lo_xmax;

// uc_orig: AENG_gamut_lo_zmin (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_gamut_lo_zmin;

// uc_orig: AENG_gamut_lo_zmax (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_gamut_lo_zmax;

// uc_orig: AENG_project_offset_u (fallen/DDEngine/Source/aeng.cpp)
// UV offset applied to projected shadow polygons (set by AENG_add_projected_*).
extern float AENG_project_offset_u;

// uc_orig: AENG_project_offset_v (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_offset_v;

// uc_orig: AENG_project_mul_u (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_mul_u;

// uc_orig: AENG_project_mul_v (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_mul_v;

// uc_orig: AENG_project_lit_light_x (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_lit_light_x;

// uc_orig: AENG_project_lit_light_y (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_lit_light_y;

// uc_orig: AENG_project_lit_light_z (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_lit_light_z;

// uc_orig: AENG_project_lit_light_range (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_lit_light_range;

// uc_orig: AENG_project_fadeout_x (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_fadeout_x;

// uc_orig: AENG_project_fadeout_z (fallen/DDEngine/Source/aeng.cpp)
extern float AENG_project_fadeout_z;

// uc_orig: AENG_frame_one (fallen/DDEngine/Source/aeng.cpp)
extern COMP_Frame AENG_frame_one;

// uc_orig: AENG_frame_two (fallen/DDEngine/Source/aeng.cpp)
extern COMP_Frame AENG_frame_two;

// uc_orig: AENG_frame_last (fallen/DDEngine/Source/aeng.cpp)
extern COMP_Frame* AENG_frame_last;

// uc_orig: AENG_frame_next (fallen/DDEngine/Source/aeng.cpp)
extern COMP_Frame* AENG_frame_next;

// uc_orig: GetShadowPixelMapping (fallen/DDEngine/Source/aeng.cpp)
// Returns a 256-entry UBYTE->UWORD table mapping shadow density values to
// the correct 16-bit pixel format for the shadow texture surface.
UWORD* GetShadowPixelMapping();

// uc_orig: move_clouds (fallen/DDEngine/Source/aeng.cpp)
// Advances the cloud scroll offsets by a fixed amount each call.
void move_clouds(void);

// uc_orig: AENG_movie_init (fallen/DDEngine/Source/aeng.cpp)
void AENG_movie_init(void);

// uc_orig: AENG_movie_update (fallen/DDEngine/Source/aeng.cpp)
void AENG_movie_update(void);

// uc_orig: AENG_calc_gamut (fallen/DDEngine/Source/aeng.cpp)
// Computes the full view frustum gamut for camera-based culling.
void AENG_calc_gamut(float x, float y, float z,
    float yaw, float pitch, float roll,
    float draw_dist, float lens);

// uc_orig: AENG_calc_gamut_lo_only (fallen/DDEngine/Source/aeng.cpp)
// Lightweight gamut computation for the lo-res skyline map (bounding box only).
void AENG_calc_gamut_lo_only(float x, float y, float z,
    float yaw, float pitch, float roll,
    float draw_dist, float lens);

// uc_orig: AENG_do_cached_lighting_old (fallen/DDEngine/Source/aeng.cpp)
void AENG_do_cached_lighting_old(void);

// uc_orig: AENG_mark_night_squares_as_deleteme (fallen/DDEngine/Source/aeng.cpp)
void AENG_mark_night_squares_as_deleteme(void);

// uc_orig: AENG_ensure_appropriate_caching (fallen/DDEngine/Source/aeng.cpp)
void AENG_ensure_appropriate_caching(SLONG ware);

// uc_orig: AENG_get_rid_of_deleteme_squares (fallen/DDEngine/Source/aeng.cpp)
void AENG_get_rid_of_deleteme_squares(void);

#endif // ENGINE_GRAPHICS_PIPELINE_AENG_GLOBALS_H
