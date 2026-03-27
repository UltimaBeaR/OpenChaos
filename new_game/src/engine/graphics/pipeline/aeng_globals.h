#ifndef ENGINE_GRAPHICS_PIPELINE_AENG_GLOBALS_H
#define ENGINE_GRAPHICS_PIPELINE_AENG_GLOBALS_H

#include "engine/core/types.h"
#include "engine/graphics/graphics_engine/graphics_engine.h"
#include "engine/graphics/pipeline/aeng.h"
#include "buildings/prim_types.h" // RMAX_PRIM_POINTS, PrimObject types
#include "engine/graphics/graphics_engine/d3d/render_state.h" // D3DMATRIX, D3DLVERTEX
#include "engine/graphics/pipeline/poly.h"          // POLY_Point
#include "engine/graphics/lighting/smap.h"                  // SMAP_Link
#include "engine/compression/compression.h"
#include "map/map.h"

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

// ---------------------------------------------------------------------------
// Globals for chunk 2: shadow projection, dirt/debris, pow (explosion sprites)
// ---------------------------------------------------------------------------

// uc_orig: AENG_MAX_DIRT_LVERTS (fallen/DDEngine/Source/aeng.cpp)
#define AENG_MAX_DIRT_LVERTS (64 * 3)

// uc_orig: AENG_MAX_DIRT_INDICES (fallen/DDEngine/Source/aeng.cpp)
#define AENG_MAX_DIRT_INDICES (AENG_MAX_DIRT_LVERTS * 4 / 3)

// uc_orig: AENG_dirt_lvert_buffer (fallen/DDEngine/Source/aeng.cpp)
// 32-byte aligned D3DLVERTEX scratch buffer for batched dirt/debris rendering.
extern D3DLVERTEX AENG_dirt_lvert_buffer[AENG_MAX_DIRT_LVERTS + 1];

// uc_orig: AENG_dirt_lvert (fallen/DDEngine/Source/aeng.cpp)
// Pointer into AENG_dirt_lvert_buffer aligned to 32 bytes.
extern D3DLVERTEX* AENG_dirt_lvert;

// uc_orig: AENG_dirt_lvert_upto (fallen/DDEngine/Source/aeng.cpp)
// Number of vertices currently written into AENG_dirt_lvert.
extern SLONG AENG_dirt_lvert_upto;

// uc_orig: AENG_dirt_index (fallen/DDEngine/Source/aeng.cpp)
// Index buffer for indexed triangle rendering of dirt quads/triangles.
extern UWORD AENG_dirt_index[AENG_MAX_DIRT_INDICES];

// uc_orig: AENG_dirt_index_upto (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_dirt_index_upto;

// uc_orig: AENG_dirt_matrix_buffer (fallen/DDEngine/Source/aeng.cpp)
// Raw storage for a 32-byte aligned D3DMATRIX.
extern UBYTE AENG_dirt_matrix_buffer[sizeof(D3DMATRIX) + 32];

// uc_orig: AENG_dirt_matrix (fallen/DDEngine/Source/aeng.cpp)
// Pointer into AENG_dirt_matrix_buffer, aligned to 32 bytes.
extern D3DMATRIX* AENG_dirt_matrix;

// uc_orig: AENG_MAX_DIRT_UVLOOKUP (fallen/DDEngine/Source/aeng.cpp)
// Number of precomputed UV entries for leaf/snowflake rotation table.
#define AENG_MAX_DIRT_UVLOOKUP 16

// uc_orig: AENG_dirt_uvlookup (fallen/DDEngine/Source/aeng.cpp)
// Precomputed UV coordinates for 16 rotation angles around the leaf/snowflake texture.
// Anonymous struct in original — named AENG_DirtUV here for linkage.
struct AENG_DirtUV
{
    float u;
    float v;
};
extern AENG_DirtUV AENG_dirt_uvlookup[AENG_MAX_DIRT_UVLOOKUP];

// uc_orig: AENG_dirt_uvlookup_valid (fallen/DDEngine/Source/aeng.cpp)
// Non-zero when AENG_dirt_uvlookup is populated for the current world_type.
extern SLONG AENG_dirt_uvlookup_valid;

// uc_orig: AENG_dirt_uvlookup_world_type (fallen/DDEngine/Source/aeng.cpp)
// world_type value for which the current UV lookup table was computed.
extern SWORD AENG_dirt_uvlookup_world_type;

// uc_orig: aeng_pow (fallen/DDEngine/Source/aeng.cpp)
// Projected screen-space entry for one explosion sprite, sorted into a depth bucket.
typedef struct aeng_pow
{
    SLONG frame;
    float sx;
    float sy;
    float sz;
    float Z;

    struct aeng_pow* next;

} AENG_Pow;

// uc_orig: AENG_MAX_POWS (fallen/DDEngine/Source/aeng.cpp)
#define AENG_MAX_POWS 256

// uc_orig: AENG_POW_NUM_BUCKETS (fallen/DDEngine/Source/aeng.cpp)
// Depth-sort bucket count for explosion sprites.
#define AENG_POW_NUM_BUCKETS 1024

// uc_orig: AENG_pow (fallen/DDEngine/Source/aeng.cpp)
// Pool of AENG_Pow entries allocated per frame.
extern AENG_Pow AENG_pow[AENG_MAX_POWS];

// uc_orig: AENG_pow_upto (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_pow_upto;

// uc_orig: AENG_pow_bucket (fallen/DDEngine/Source/aeng.cpp)
// Linked-list heads for each depth bucket used to sort POW sprites back-to-front.
extern AENG_Pow* AENG_pow_bucket[AENG_POW_NUM_BUCKETS];

// uc_orig: AENG_draw_rain_old (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_rain_old(float angle);

// uc_orig: AENG_draw_rain (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_rain(void);

// uc_orig: AENG_draw_drips (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_drips(UBYTE puddles_only);

// uc_orig: AENG_draw_bangs (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_bangs(void);

// uc_orig: AENG_draw_cloth (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_cloth(void);

// uc_orig: AENG_draw_fire (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_fire(void);

// uc_orig: AENG_draw_sparks (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_sparks(void);

// uc_orig: AENG_draw_hook (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_hook(void);

// uc_orig: AENG_add_projected_shadow_poly (fallen/DDEngine/Source/aeng.cpp)
void AENG_add_projected_shadow_poly(SMAP_Link* sl);

// uc_orig: AENG_add_projected_fadeout_shadow_poly (fallen/DDEngine/Source/aeng.cpp)
void AENG_add_projected_fadeout_shadow_poly(SMAP_Link* sl);

// uc_orig: AENG_add_projected_lit_shadow_poly (fallen/DDEngine/Source/aeng.cpp)
void AENG_add_projected_lit_shadow_poly(SMAP_Link* sl);

// uc_orig: AENG_colour_mult (fallen/DDEngine/Source/aeng.cpp)
// Component-wise multiply two packed RGB colours (each channel * channel / 256).
ULONG AENG_colour_mult(ULONG c1, ULONG c2);

// uc_orig: AENG_draw_dirt (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_dirt(void);

// uc_orig: AENG_draw_pows (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_pows(void);

// ---------------------------------------------------------------------------
// Globals for chunk 3: balloons, sky, rect/tri 2D draw, people messages,
//                      bike wheel, detail levels, floor tile infrastructure
// ---------------------------------------------------------------------------

// uc_orig: AENG_AA_LEFT (fallen/DDEngine/Source/aeng.cpp)
#define AENG_AA_LEFT 20
// uc_orig: AENG_AA_TOP (fallen/DDEngine/Source/aeng.cpp)
#define AENG_AA_TOP 20
// uc_orig: AENG_AA_PIX_SIZE (fallen/DDEngine/Source/aeng.cpp)
#define AENG_AA_PIX_SIZE 4
// uc_orig: AENG_AA_BUF_SIZE (fallen/DDEngine/Source/aeng.cpp)
#define AENG_AA_BUF_SIZE 32

// uc_orig: AENG_aa_buffer (fallen/DDEngine/Source/aeng.cpp)
// Scratch buffer for the anti-aliasing test (32x32 pixel coverage grid).
extern UBYTE AENG_aa_buffer[AENG_AA_BUF_SIZE][AENG_AA_BUF_SIZE];

// uc_orig: MAP_SIZE_TWEAK (fallen/DDEngine/Source/aeng.cpp)
// Extra padding on MAP_WIDTH/2 and MAP_HEIGHT/2 for vertex arrays.
#define MAP_SIZE_TWEAK 0

// uc_orig: AENG_upper (fallen/DDEngine/Source/aeng.cpp)
// Per-vertex lighting/UV data for the upper (hi-res) ground tile grid.
extern POLY_Point AENG_upper[MAP_WIDTH / 2 + MAP_SIZE_TWEAK][MAP_HEIGHT / 2 + MAP_SIZE_TWEAK];

// uc_orig: AENG_lower (fallen/DDEngine/Source/aeng.cpp)
// Per-vertex lighting/UV data for the lower ground tile grid.
extern POLY_Point AENG_lower[MAP_WIDTH / 2 + MAP_SIZE_TWEAK * 2][MAP_HEIGHT / 2 + MAP_SIZE_TWEAK * 2];

// uc_orig: AENG_SKY_TYPE_NIGHT (fallen/DDEngine/Source/aeng.cpp)
#define AENG_SKY_TYPE_NIGHT 0
// uc_orig: AENG_SKY_TYPE_DAY (fallen/DDEngine/Source/aeng.cpp)
#define AENG_SKY_TYPE_DAY 1

// uc_orig: AENG_torch_on (fallen/DDEngine/Source/aeng.cpp)
// Non-zero when the player's torch/flashlight is enabled.
extern SLONG AENG_torch_on;

// uc_orig: AENG_shadows_on (fallen/DDEngine/Source/aeng.cpp)
// Non-zero when shadow rendering is active.
extern SLONG AENG_shadows_on;

// uc_orig: AENG_sky_type (fallen/DDEngine/Source/aeng.cpp)
// Current sky type: AENG_SKY_TYPE_DAY or AENG_SKY_TYPE_NIGHT.
extern SLONG AENG_sky_type;

// uc_orig: AENG_sky_colour_bot (fallen/DDEngine/Source/aeng.cpp)
// Bottom horizon colour for the sky gradient (ARGB packed).
extern ULONG AENG_sky_colour_bot;

// uc_orig: AENG_sky_colour_top (fallen/DDEngine/Source/aeng.cpp)
// Top sky colour for the sky gradient (ARGB packed).
extern ULONG AENG_sky_colour_top;

// uc_orig: RRect (fallen/DDEngine/Source/aeng.cpp)
// Deferred screen-space rectangle entry, queued via AENG_draw_rectr, flushed by draw_all_boxes.
struct RRect
{
    SLONG x;
    SLONG y;
    SLONG w;
    SLONG h;
    SLONG col;
    SLONG layer;
    SLONG page;
};

// uc_orig: rrect (fallen/DDEngine/Source/aeng.cpp)
// Ring buffer of deferred rectangle draw calls.
extern RRect rrect[2000];

// uc_orig: next_rrect (fallen/DDEngine/Source/aeng.cpp)
// Next free slot in rrect[]; starts at 1, reset to 0 after flush.
extern SLONG next_rrect;

// uc_orig: MAX_WIDTH_DRAWN (fallen/DDEngine/Source/aeng.cpp)
#define MAX_WIDTH_DRAWN 100

// uc_orig: MAX_FLOOR_TILES_FOR_STRIPS (fallen/DDEngine/Source/aeng.cpp)
// Maximum ground tiles batched in one indexed primitive strip.
#define MAX_FLOOR_TILES_FOR_STRIPS 16

// uc_orig: MAX_VERTS_FOR_STRIPS (fallen/DDEngine/Source/aeng.cpp)
#define MAX_VERTS_FOR_STRIPS (MAX_FLOOR_TILES_FOR_STRIPS * 4)

// uc_orig: MAX_INDICES_FOR_STRIPS (fallen/DDEngine/Source/aeng.cpp)
#define MAX_INDICES_FOR_STRIPS (MAX_FLOOR_TILES_FOR_STRIPS * 5)

// uc_orig: IPRIM_COUNT (fallen/DDEngine/Source/aeng.cpp)
// Number of parallel indexed primitive groups used during floor tile rendering.
#define IPRIM_COUNT 5

// uc_orig: MAX_DRAW_WIDTH (fallen/DDEngine/Source/aeng.cpp)
#define MAX_DRAW_WIDTH 128

// uc_orig: FloorStore (fallen/DDEngine/Source/aeng.cpp)
// Cached per-vertex data for one map tile corner (colour, altitude, flags, texture).
struct FloorStore
{
    ULONG Colour;
    float Alt;
    UWORD Flags;
    UWORD Texture;
};

// uc_orig: GroupInfo (fallen/DDEngine/Source/aeng.cpp)
// Tracks which D3D texture page is bound in each of the IPRIM_COUNT strip groups.
struct GroupInfo
{
    LPDIRECT3DTEXTURE2 page;
};

// uc_orig: KERB_TILES (fallen/DDEngine/Source/aeng.cpp)
#define KERB_TILES 16
// uc_orig: KERB_VERTS (fallen/DDEngine/Source/aeng.cpp)
#define KERB_VERTS (4 * KERB_TILES)
// uc_orig: KERB_INDICIES (fallen/DDEngine/Source/aeng.cpp)
#define KERB_INDICIES (5 * KERB_TILES)

// uc_orig: m_vert_mem_block32 (fallen/DDEngine/Source/aeng.cpp)
// Raw allocation block for kerb + floor strip vertices; sized for 32-byte alignment.
extern UBYTE m_vert_mem_block32[sizeof(D3DLVERTEX) * KERB_VERTS + sizeof(D3DLVERTEX) * MAX_VERTS_FOR_STRIPS * IPRIM_COUNT + 32];

// uc_orig: m_indicies (fallen/DDEngine/Source/aeng.cpp)
// Index buffers for each of the IPRIM_COUNT floor strip groups.
extern UWORD m_indicies[IPRIM_COUNT][MAX_INDICES_FOR_STRIPS + 1];

// uc_orig: MAX_STEAM (fallen/DDEngine/Source/aeng.cpp)
// Maximum number of steam sources accumulated in one frame.
#define MAX_STEAM 20

// uc_orig: kerb_scaleu (fallen/DDEngine/Source/aeng.cpp)
extern float kerb_scaleu;
// uc_orig: kerb_scalev (fallen/DDEngine/Source/aeng.cpp)
extern float kerb_scalev;
// uc_orig: kerb_du (fallen/DDEngine/Source/aeng.cpp)
extern float kerb_du;
// uc_orig: kerb_dv (fallen/DDEngine/Source/aeng.cpp)
extern float kerb_dv;

// uc_orig: HALF_COL (fallen/DDEngine/Source/aeng.cpp)
// In-place halves each colour channel of a packed ARGB value.
#define HALF_COL(col) (col) = ((col) >> 2) & 0xff3f3f3f

// uc_orig: show_facet (fallen/DDEngine/Source/aeng.cpp)
// Evaluates sizeof(thing) for side-effects only — "bin this" debug marker.
#define show_facet(thing) sizeof(thing)

// uc_orig: show_gamut_lo (fallen/DDEngine/Source/aeng.cpp)
// Debug stub for visualising lo-res gamut cells.
void show_gamut_lo(SLONG x, SLONG z);

// uc_orig: show_gamut_hi (fallen/DDEngine/Source/aeng.cpp)
// Debug stub for visualising hi-res gamut cells.
void show_gamut_hi(SLONG x, SLONG z);

// Forward declaration — full definition in polypage.h (Temporary).
struct D3DMULTIMATRIX;

// uc_orig: cache_a_row (fallen/DDEngine/Source/aeng.cpp)
// Pre-fetches lighting and PAP data for a row of map tile corners.
void cache_a_row(SLONG x, SLONG z, struct FloorStore* p2, SLONG endx);

// uc_orig: add_kerb (fallen/DDEngine/Source/aeng.cpp)
// Emits four vertices for one kerb quad; returns UC_TRUE if the quad was added.
SLONG add_kerb(float alt1, float alt2, SLONG x, SLONG z, SLONG dx, SLONG dz, D3DLVERTEX* pv, UWORD* p_indicies, SLONG count, ULONG c1, ULONG c2, SLONG flip);

// uc_orig: draw_i_prim (fallen/DDEngine/Source/aeng.cpp)
// Flushes one indexed primitive strip group to the GPU using DrawIndPrimMM.
void draw_i_prim(LPDIRECT3DTEXTURE2 page, D3DLVERTEX* verts, UWORD* indicies, SLONG* vert_count, SLONG* index_count, D3DMULTIMATRIX* mm_draw_floor);

// uc_orig: general_steam (fallen/DDEngine/Source/aeng.cpp)
// Accumulates (mode=1), flushes (mode=2), or resets (mode=0) steam source positions.
void general_steam(SLONG x, SLONG z, UWORD texture, SLONG mode);

// uc_orig: draw_quick_floor (fallen/DDEngine/Source/aeng.cpp)
// Renders the ground tile mesh using indexed primitive strips.
void draw_quick_floor(SLONG warehouse);

// uc_orig: AENG_draw_released_balloons (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_released_balloons(void);

// uc_orig: AENG_draw_rectr (fallen/DDEngine/Source/aeng.cpp)
// Queues a deferred 2D rectangle to be flushed later by draw_all_boxes.
void AENG_draw_rectr(SLONG x, SLONG y, SLONG w, SLONG h, SLONG col, SLONG layer, SLONG page);

// uc_orig: draw_all_boxes (fallen/DDEngine/Source/aeng.cpp)
// Flushes all queued AENG_draw_rectr rectangles via AENG_draw_rect.
void draw_all_boxes(void);

// uc_orig: AENG_draw_people_messages (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_people_messages(void);

// uc_orig: AENG_set_bike_wheel_rotation (fallen/DDEngine/Source/aeng.cpp)
// Updates the UV coordinates on bike wheel prim faces to animate wheel spin.
void AENG_set_bike_wheel_rotation(UWORD rot, UBYTE prim);

// uc_orig: AENG_draw_some_polys (fallen/DDEngine/Source/aeng.cpp)
// Renders N test triangles using DrawIndexedPrimitive; returns elapsed time.
float AENG_draw_some_polys(bool large, bool blend);

// uc_orig: AENG_guess_detail_levels (fallen/DDEngine/Source/aeng.cpp)
// Auto-selects detail settings based on GPU speed; only runs once if estimate flag is set.
void AENG_guess_detail_levels(void);

// uc_orig: index_lookup (fallen/DDEngine/Source/aeng.cpp)
// Maps quad corner index 0-3 to face winding order for roof tile rendering inside AENG_draw_city.
extern UBYTE index_lookup[4];

// ---------------------------------------------------------------------------
// Globals for chunk 5a: far facets, warehouse renderer, screen shot, FPS, fade
// ---------------------------------------------------------------------------

// uc_orig: record_video (fallen/DDEngine/Source/aeng.cpp)
// When non-zero, AENG_screen_shot dumps a frame every game tick (debug video recording).
extern UBYTE record_video;

// uc_orig: AENG_draw_warehouse (fallen/DDEngine/Source/aeng.cpp)
// Renders interior warehouse/building scenes (migrated to new/aeng.cpp).
void AENG_draw_warehouse(void);

// ---------------------------------------------------------------------------
// Globals for chunk 5b: top-level draw, viewport, editor helpers, inside floor
// ---------------------------------------------------------------------------

// uc_orig: AENG_transparent_warehouses (fallen/DDEngine/Source/aeng.cpp)
// When non-zero, warehouse geometry is rendered on top of the city view (debug toggle).
extern UBYTE AENG_transparent_warehouses;

// uc_orig: AENG_drawing_a_warehouse (fallen/DDEngine/Source/aeng.cpp)
// Set to UC_TRUE during the warehouse rendering pass; consulted by facet.cpp
// to skip outdoor shadow rendering on warehouse geometry.
extern SLONG AENG_drawing_a_warehouse;

#endif // ENGINE_GRAPHICS_PIPELINE_AENG_GLOBALS_H
