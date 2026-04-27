// Global variables for the polygon rendering pipeline.

#include "engine/graphics/pipeline/poly_globals.h"

// Per-page texture flags. Filled by POLY_load_texture_flags / POLY_reset_render_states.
// uc_orig: POLY_page_flag (fallen/DDEngine/Headers/poly.h)
UWORD POLY_page_flag[POLY_NUM_PAGES];

// Scratch buffer for transformed vertices (indexed by POLY_buffer_upto).
// uc_orig: POLY_buffer (fallen/DDEngine/Headers/poly.h)
POLY_Point POLY_buffer[POLY_BUFFER_SIZE];
// uc_orig: POLY_buffer_upto (fallen/DDEngine/Headers/poly.h)
SLONG POLY_buffer_upto;

// Camera world-space position, set by POLY_camera_set.
// uc_orig: POLY_cam_x (fallen/DDEngine/Headers/poly.h)
float POLY_cam_x;
// uc_orig: POLY_cam_y (fallen/DDEngine/Headers/poly.h)
float POLY_cam_y;
// uc_orig: POLY_cam_z (fallen/DDEngine/Headers/poly.h)
float POLY_cam_z;
// uc_orig: POLY_cam_matrix (fallen/DDEngine/Headers/poly.h)
// 3x3 camera rotation matrix (row-major), updated by POLY_camera_set each frame.
float POLY_cam_matrix[9];

// Screen viewport dimensions (pixels), set by POLY_camera_set.
// uc_orig: POLY_screen_width (fallen/DDEngine/Headers/poly.h)
float POLY_screen_width;
// uc_orig: POLY_screen_height (fallen/DDEngine/Headers/poly.h)
float POLY_screen_height;

// Colour channel mask applied to all polys (zero = unrestricted).
// uc_orig: POLY_colour_restrict (fallen/DDEngine/Headers/poly.h)
ULONG POLY_colour_restrict;
// When non-zero, all alpha-blend pages are rendered additive regardless of page flags.
// uc_orig: POLY_force_additive_alpha (fallen/DDEngine/Headers/poly.h)
ULONG POLY_force_additive_alpha;

// uc_orig: iPolyNumPagesRender (fallen/DDEngine/Source/poly.cpp)
int iPolyNumPagesRender = 0;

// Camera transform state (set each frame by POLY_camera_set).
// uc_orig: POLY_cam_aspect (fallen/DDEngine/Source/poly.cpp)
float POLY_cam_aspect = 1.0f;
// uc_orig: POLY_cam_lens (fallen/DDEngine/Source/poly.cpp)
float POLY_cam_lens = 1.0f;
// uc_orig: POLY_cam_view_dist (fallen/DDEngine/Source/poly.cpp)
float POLY_cam_view_dist = 1.0f;
// uc_orig: POLY_cam_over_view_dist (fallen/DDEngine/Source/poly.cpp)
float POLY_cam_over_view_dist = 1.0f;
// uc_orig: POLY_cam_matrix_comb (fallen/DDEngine/Source/poly.cpp)
float POLY_cam_matrix_comb[9];
// uc_orig: POLY_cam_off_x (fallen/DDEngine/Source/poly.cpp)
float POLY_cam_off_x;
// uc_orig: POLY_cam_off_y (fallen/DDEngine/Source/poly.cpp)
float POLY_cam_off_y;
// uc_orig: POLY_cam_off_z (fallen/DDEngine/Source/poly.cpp)
float POLY_cam_off_z;

// Screen geometry (set each frame by POLY_camera_set).
// uc_orig: POLY_screen_mid_x (fallen/DDEngine/Source/poly.cpp)
float POLY_screen_mid_x;
// uc_orig: POLY_screen_mid_y (fallen/DDEngine/Source/poly.cpp)
float POLY_screen_mid_y;
// uc_orig: POLY_screen_mul_x (fallen/DDEngine/Source/poly.cpp)
float POLY_screen_mul_x;
// uc_orig: POLY_screen_mul_y (fallen/DDEngine/Source/poly.cpp)
float POLY_screen_mul_y;

// Screen clip bounds (pixel coords).
// uc_orig: POLY_screen_clip_left (fallen/DDEngine/Source/poly.cpp)
float POLY_screen_clip_left = 0;
// uc_orig: POLY_screen_clip_right (fallen/DDEngine/Source/poly.cpp)
float POLY_screen_clip_right = 640;
// uc_orig: POLY_screen_clip_bottom (fallen/DDEngine/Source/poly.cpp)
float POLY_screen_clip_bottom = 480;
// uc_orig: POLY_screen_clip_top (fallen/DDEngine/Source/poly.cpp)
float POLY_screen_clip_top = 0;

// uc_orig: POLY_splitscreen (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_splitscreen;

// Wibble state.
// uc_orig: POLY_wibble_y1 (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_y1;
// uc_orig: POLY_wibble_y2 (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_y2;
// uc_orig: POLY_wibble_g1 (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_g1;
// uc_orig: POLY_wibble_g2 (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_g2;
// uc_orig: POLY_wibble_s1 (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_s1;
// uc_orig: POLY_wibble_s2 (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_s2;
// uc_orig: POLY_wibble_turn (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_turn;
// uc_orig: POLY_wibble_dangle1 (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_dangle1;
// uc_orig: POLY_wibble_dangle2 (fallen/DDEngine/Source/poly.cpp)
SLONG POLY_wibble_dangle2;

// uc_orig: DefRenderState (fallen/DDEngine/Source/poly.cpp)
RenderState DefRenderState;

// uc_orig: POLY_Page (fallen/DDEngine/Source/poly.cpp)
PolyPage POLY_Page[POLY_NUM_PAGES];

// D3D matrices.
// uc_orig: g_matProjection (fallen/DDEngine/Source/poly.cpp)
GEMatrix g_matProjection;
// uc_orig: g_viewData (fallen/DDEngine/Source/poly.cpp)
GEViewport g_viewData;
// uc_orig: g_dw3DStuffHeight (fallen/DDEngine/Source/poly.cpp)
DWORD g_dw3DStuffHeight;
// uc_orig: g_dw3DStuffY (fallen/DDEngine/Source/poly.cpp)
DWORD g_dw3DStuffY;
// uc_orig: g_matWorld (fallen/DDEngine/Source/poly.cpp)
GEMatrix g_matWorld;
