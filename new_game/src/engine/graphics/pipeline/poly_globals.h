#ifndef ENGINE_GRAPHICS_PIPELINE_POLY_GLOBALS_H
#define ENGINE_GRAPHICS_PIPELINE_POLY_GLOBALS_H

#include "engine/graphics/graphics_engine/game_graphics_engine.h"   // GEMatrix, GEViewport
#include "engine/core/types.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypage.h"

// Camera state set once per frame by POLY_camera_set().

// uc_orig: POLY_cam_aspect (fallen/DDEngine/Source/poly.cpp)
// Aspect ratio: screen height / screen width.
extern float POLY_cam_aspect;

// uc_orig: POLY_cam_lens (fallen/DDEngine/Source/poly.cpp)
// FOV multiplier passed to POLY_camera_set.
extern float POLY_cam_lens;

// uc_orig: POLY_cam_view_dist (fallen/DDEngine/Source/poly.cpp)
// Maximum world-space view distance.
extern float POLY_cam_view_dist;

// uc_orig: POLY_cam_over_view_dist (fallen/DDEngine/Source/poly.cpp)
// Reciprocal of POLY_cam_view_dist (1/view_dist), precomputed.
extern float POLY_cam_over_view_dist;

// uc_orig: POLY_cam_matrix_comb (fallen/DDEngine/Source/poly.cpp)
// Combined camera+local-object rotation matrix (3x3, row-major).
// Precomputed by POLY_set_local_rotation to avoid per-vertex recomputation.
extern float POLY_cam_matrix_comb[9];

// uc_orig: POLY_cam_off_x (fallen/DDEngine/Source/poly.cpp)
extern float POLY_cam_off_x;
// uc_orig: POLY_cam_off_y (fallen/DDEngine/Source/poly.cpp)
extern float POLY_cam_off_y;
// uc_orig: POLY_cam_off_z (fallen/DDEngine/Source/poly.cpp)
// Object origin rotated into camera space (translation offset).
extern float POLY_cam_off_z;

// Screen geometry: derived from viewport each frame.

// uc_orig: POLY_screen_mid_x (fallen/DDEngine/Source/poly.cpp)
extern float POLY_screen_mid_x;
// uc_orig: POLY_screen_mid_y (fallen/DDEngine/Source/poly.cpp)
extern float POLY_screen_mid_y;
// uc_orig: POLY_screen_mul_x (fallen/DDEngine/Source/poly.cpp)
// Horizontal projection multiplier: pixels per unit at POLY_ZCLIP_PLANE.
extern float POLY_screen_mul_x;
// uc_orig: POLY_screen_mul_y (fallen/DDEngine/Source/poly.cpp)
extern float POLY_screen_mul_y;

// Screen clip boundaries (pixel coords). Set by POLY_camera_set for splitscreen.
// uc_orig: POLY_screen_clip_left (fallen/DDEngine/Source/poly.cpp)
extern float POLY_screen_clip_left;
// uc_orig: POLY_screen_clip_right (fallen/DDEngine/Source/poly.cpp)
extern float POLY_screen_clip_right;
// uc_orig: POLY_screen_clip_bottom (fallen/DDEngine/Source/poly.cpp)
extern float POLY_screen_clip_bottom;
// uc_orig: POLY_screen_clip_top (fallen/DDEngine/Source/poly.cpp)
extern float POLY_screen_clip_top;

// uc_orig: POLY_splitscreen (fallen/DDEngine/Source/poly.cpp)
// Current split-screen mode: POLY_SPLITSCREEN_NONE / TOP / BOTTOM.
extern SLONG POLY_splitscreen;

// Wibble (sinusoidal screen-space wobble) state.
// uc_orig: POLY_wibble_y1 (fallen/DDEngine/Source/poly.cpp)
extern SLONG POLY_wibble_y1;
// uc_orig: POLY_wibble_y2 (fallen/DDEngine/Source/poly.cpp)
extern SLONG POLY_wibble_y2;
// uc_orig: POLY_wibble_g1 (fallen/DDEngine/Source/poly.cpp)
extern SLONG POLY_wibble_g1;
// uc_orig: POLY_wibble_g2 (fallen/DDEngine/Source/poly.cpp)
extern SLONG POLY_wibble_g2;
// uc_orig: POLY_wibble_s1 (fallen/DDEngine/Source/poly.cpp)
extern SLONG POLY_wibble_s1;
// uc_orig: POLY_wibble_s2 (fallen/DDEngine/Source/poly.cpp)
extern SLONG POLY_wibble_s2;
// uc_orig: POLY_wibble_turn (fallen/DDEngine/Source/poly.cpp)
// Running phase accumulator advanced each frame.
extern SLONG POLY_wibble_turn;
// uc_orig: POLY_wibble_dangle1 (fallen/DDEngine/Source/poly.cpp)
extern SLONG POLY_wibble_dangle1;
// uc_orig: POLY_wibble_dangle2 (fallen/DDEngine/Source/poly.cpp)
extern SLONG POLY_wibble_dangle2;

// uc_orig: DefRenderState (fallen/DDEngine/Source/poly.cpp)
// Default Direct3D render state used at the start of each frame.
extern RenderState DefRenderState;

// uc_orig: POLY_Page (fallen/DDEngine/Source/poly.cpp)
// Array of per-texture-page polygon buckets. One bucket per texture page.
extern PolyPage POLY_Page[POLY_NUM_PAGES];

// uc_orig: iPolyNumPagesRender (fallen/DDEngine/Source/poly.cpp)
// Number of texture pages that need rendering this frame.
extern int iPolyNumPagesRender;

// Direct3D matrices and viewport data set by POLY_camera_set each frame.
// uc_orig: g_matProjection (fallen/DDEngine/Source/poly.cpp)
extern GEMatrix g_matProjection;
// uc_orig: g_viewData (fallen/DDEngine/Source/poly.cpp)
extern GEViewport g_viewData;
// uc_orig: g_dw3DStuffHeight (fallen/DDEngine/Source/poly.cpp)
// Height of the 3D viewport in scaled pixels.
extern DWORD g_dw3DStuffHeight;
// uc_orig: g_dw3DStuffY (fallen/DDEngine/Source/poly.cpp)
// Y offset of the 3D viewport in scaled pixels.
extern DWORD g_dw3DStuffY;
// uc_orig: g_matWorld (fallen/DDEngine/Source/poly.cpp)
// World matrix uploaded to D3D by POLY_set_local_rotation.
extern GEMatrix g_matWorld;

// uc_orig: POLY_clip_left (fallen/DDEngine/Source/poly.cpp)
// 2D clip rectangle used by POLY_clip_line_add.
extern float POLY_clip_left;
// uc_orig: POLY_clip_right (fallen/DDEngine/Source/poly.cpp)
extern float POLY_clip_right;
// uc_orig: POLY_clip_top (fallen/DDEngine/Source/poly.cpp)
extern float POLY_clip_top;
// uc_orig: POLY_clip_bottom (fallen/DDEngine/Source/poly.cpp)
extern float POLY_clip_bottom;

#endif // ENGINE_GRAPHICS_PIPELINE_POLY_GLOBALS_H
