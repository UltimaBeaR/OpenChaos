// Core software polygon renderer: camera transform, clipping, and D3D submission.
// All functions here operate on POLY_Point vertices and PolyPage buckets.
// In the new renderer this whole file will be replaced by a GPU pipeline.

#include <platform.h>
#include "engine/graphics/graphics_api/display_macros.h" // BEGIN_SCENE, END_SCENE, REALLY_SET_*, DRAW_INDEXED_PRIMITIVE, the_display
#include <math.h>
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/poly_render.h"
#include "engine/graphics/pipeline/poly_render_globals.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/pipeline/render_state.h"
#include "engine/graphics/pipeline/vertex_buffer.h"
#include "engine/graphics/geometry/superfacet.h"
#include "engine/lighting/crinkle.h"
#include "core/matrix.h"
#include "assets/texture.h"
#include "missions/game_types.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"
#include "missions/eway.h"


// draw_3d is defined in aeng.cpp (not yet migrated).
// uc_orig: draw_3d (fallen/DDEngine/Headers/poly.h)
extern SLONG draw_3d;

// fade_black is defined in controls.cpp.
// uc_orig: fade_black (fallen/DDEngine/Source/poly.cpp)
extern UWORD fade_black;

// (globals moved to poly_globals.cpp)

// COMBO_* constants control the Direct3D vertex buffer combo state.
// uc_orig: COMBO_FALSE (fallen/DDEngine/Source/poly.cpp)
#define COMBO_FALSE 0
// uc_orig: COMBO_TRUE (fallen/DDEngine/Source/poly.cpp)
#define COMBO_TRUE 1
// uc_orig: COMBO_DIRTY (fallen/DDEngine/Source/poly.cpp)
#define COMBO_DIRTY 2
// uc_orig: m_iCurrentCombo (fallen/DDEngine/Source/poly.cpp)
// Tracks whether a D3D vertex buffer combo (lock) is currently active.
static int m_iCurrentCombo = COMBO_DIRTY;

// Near-clip plane distance used throughout all clipping functions.
// uc_orig: POLY_Z_NEARPLANE (fallen/DDEngine/Source/poly.cpp)
#define POLY_Z_NEARPLANE POLY_ZCLIP_PLANE

// Current clip-plane bitmask (STD or STD|TOP/BOTTOM for splitscreen).
// uc_orig: s_ClipMask (fallen/DDEngine/Source/poly.cpp)
static UBYTE s_ClipMask;

// uc_orig: STD_CLIPMASK (fallen/DDEngine/Source/poly.cpp)
#define STD_CLIPMASK (POLY_CLIP_LEFT | POLY_CLIP_RIGHT | POLY_CLIP_TOP | POLY_CLIP_BOTTOM | POLY_CLIP_NEAR)

// uc_orig: POLY_SWAP (fallen/DDEngine/Source/poly.cpp)
// Swaps two POLY_Point pointers.
#define POLY_SWAP(pp1, pp2)   \
    {                         \
        POLY_Point* pp_spare; \
        pp_spare = (pp1);     \
        (pp1) = (pp2);        \
        (pp2) = pp_spare;     \
    }

// uc_orig: fade_point_more (fallen/DDEngine/Headers/poly.h)
// Extended fade calculation using Y component. Not the normal fog path.
SLONG fade_point_more(POLY_Point* pp)
{
    SLONG fade;

    fade = (((SLONG)(pp->y)) >> 0);

    return (fade);
}

// uc_orig: POLY_init (fallen/DDEngine/Headers/poly.h)
// One-time init for the polygon renderer. No-op on PC — all state initialised via camera_set.
void POLY_init(void)
{
}

// uc_orig: POLY_ClearAllPages (fallen/DDEngine/Source/poly.cpp)
// Clears all polygon page vertex/index buffers.
void POLY_ClearAllPages(void)
{
    for (int i = 0; i < POLY_NUM_PAGES; i++) {
        POLY_Page[i].Clear();
    }
}

// uc_orig: POLY_wibble_y1 (fallen/DDEngine/Source/poly.cpp)
// — globals now in poly_globals.cpp, but wibble values are set here:

// uc_orig: POLY_set_wibble (fallen/DDEngine/Headers/poly.h)
// Sets wibble (sinusoidal water-shimmer) parameters and advances the phase accumulator.
void POLY_set_wibble(
    UBYTE wibble_y1,
    UBYTE wibble_y2,
    UBYTE wibble_g1,
    UBYTE wibble_g2,
    UBYTE wibble_s1,
    UBYTE wibble_s2)
{
    POLY_wibble_y1 = wibble_y1;
    POLY_wibble_y2 = wibble_y2;
    POLY_wibble_g1 = wibble_g1;
    POLY_wibble_g2 = wibble_g2;
    POLY_wibble_s1 = wibble_s1;
    POLY_wibble_s2 = wibble_s2;

    POLY_wibble_turn += 256 * TICK_RATIO >> TICK_SHIFT;

    POLY_wibble_dangle1 = POLY_wibble_turn * POLY_wibble_g1 >> 9;
    POLY_wibble_dangle2 = POLY_wibble_turn * POLY_wibble_g2 >> 9;
}

// uc_orig: POLY_page_is_masked_self_illuminating (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the texture page uses 2-pass masked self-illumination rendering.
SLONG POLY_page_is_masked_self_illuminating(SLONG page)
{
    if (WITHIN(page, 0, POLY_NUM_PAGES - 1) && (POLY_page_flag[page] & POLY_PAGE_FLAG_2PASS)) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: POLY_camera_set (fallen/DDEngine/Headers/poly.h)
// Sets up camera projection and viewport for the current frame.
// Computes the 3x3 rotation matrix from yaw/pitch/roll, scales it by view_dist and aspect,
// and uploads view/projection/viewport to Direct3D.
// lens: FOV multiplier (higher = more zoom). splitscreen: POLY_SPLITSCREEN_NONE/TOP/BOTTOM.
void POLY_camera_set(
    float x,
    float y,
    float z,
    float yaw,
    float pitch,
    float roll,
    float view_dist,
    float lens,
    SLONG splitscreen)
{
    POLY_splitscreen = splitscreen;

    POLY_screen_width = float(DisplayWidth);
    POLY_screen_clip_left = 0.0;
    POLY_screen_clip_right = POLY_screen_width - 00.0;
    POLY_screen_mid_x = POLY_screen_width * 0.5F;
    POLY_screen_mul_x = POLY_screen_width * 0.5F / POLY_ZCLIP_PLANE;

    switch (POLY_splitscreen) {
    case POLY_SPLITSCREEN_NONE:
        POLY_screen_height = float(DisplayHeight);

        POLY_screen_clip_top = 00.0;
        POLY_screen_clip_bottom = POLY_screen_height - 00.0;

        {
            // Letterbox effect: narrow the viewport vertically during cutscenes.
            // Commented-out smooth version is intentionally kept (matches original).
            static float wideify = 0.0f;

            /*

            if (EWAY_stop_player_moving())
            {
                    wideify += (80.0F - wideify) * 0.125F;
            }
            else
            {
                    wideify += (0.0F - wideify) * 0.125F;
            }

            if (wideify < 1)	wideify = 0;

            */

            if (EWAY_stop_player_moving()) {
                wideify = 80.0F;
            } else {
                wideify = 0.0F;
            }

            POLY_screen_clip_top += wideify;
            POLY_screen_clip_bottom -= wideify;

            POLY_screen_height -= wideify * 2.0F;
        }

        POLY_screen_mid_y = POLY_screen_height * 0.5F + POLY_screen_clip_top;
        POLY_screen_mul_y = POLY_screen_height * 0.5F / POLY_ZCLIP_PLANE;

        s_ClipMask = STD_CLIPMASK;
        break;

    case POLY_SPLITSCREEN_TOP:
        POLY_screen_height = float(DisplayHeight >> 1);

        POLY_screen_clip_top = 0.0F;
        POLY_screen_clip_bottom = POLY_screen_height;

        POLY_screen_mid_y = POLY_screen_height * 0.50F;
        POLY_screen_mul_y = POLY_screen_height * 0.50F / POLY_ZCLIP_PLANE;

        s_ClipMask = STD_CLIPMASK | POLY_CLIP_BOTTOM;
        break;

    case POLY_SPLITSCREEN_BOTTOM:
        POLY_screen_height = float(DisplayHeight >> 1);

        POLY_screen_clip_top = POLY_screen_height;
        POLY_screen_clip_bottom = float(DisplayHeight);

        POLY_screen_mid_y = POLY_screen_height * 1.50F;
        POLY_screen_mul_y = POLY_screen_height * 0.50F / POLY_ZCLIP_PLANE;

        s_ClipMask = STD_CLIPMASK | POLY_CLIP_TOP;
        break;

    default:
        ASSERT(0);
        break;
    }

    POLY_cam_x = x;
    POLY_cam_y = y;
    POLY_cam_z = z;

    POLY_cam_lens = lens;
    POLY_cam_view_dist = view_dist;
    POLY_cam_over_view_dist = 1.0F / view_dist;
    POLY_cam_aspect = POLY_screen_height / POLY_screen_width;

    MATRIX_calc(
        POLY_cam_matrix,
        yaw,
        pitch,
        roll);

    {
        // Inform the crinkle (bump-mapping) system about the current light direction in view space.
        float dx = float(NIGHT_amb_norm_x) * (1.0F / 256.0F);
        float dy = float(NIGHT_amb_norm_y) * (1.0F / 256.0F);
        float dz = float(NIGHT_amb_norm_z) * (1.0F / 256.0F);

        CRINKLE_light(dx, dy, dz);
    }

    {
        // Inform the crinkle module about the view-space aspect/FOV skewing.
        CRINKLE_skew(
            POLY_cam_aspect,
            POLY_cam_lens);
    }

    // Bake view_dist and aspect into the camera matrix so that Z maps to 0..1 range.
    MATRIX_skew(
        POLY_cam_matrix,
        POLY_cam_aspect,
        POLY_cam_lens,
        POLY_cam_over_view_dist);

    HRESULT hres;

    // View matrix is identity — all transforms are concatenated into the world matrix.
    D3DMATRIX matTemp;
    matTemp._11 = 1.0f;
    matTemp._21 = 0.0f;
    matTemp._31 = 0.0f;
    matTemp._41 = 0.0f;
    matTemp._12 = 0.0f;
    matTemp._22 = 1.0f;
    matTemp._32 = 0.0f;
    matTemp._42 = 0.0f;
    matTemp._13 = 0.0f;
    matTemp._23 = 0.0f;
    matTemp._33 = 1.0f;
    matTemp._43 = 0.0f;
    matTemp._14 = 0.0f;
    matTemp._24 = 0.0f;
    matTemp._34 = 0.0f;
    matTemp._44 = 1.0f;
    hres = (the_display.lp_D3D_Device)->SetTransform(D3DTRANSFORMSTATE_VIEW, &matTemp);

    // Projection matrix: identity-ish, with Z shifted by POLY_ZCLIP_PLANE.
    g_matProjection._11 = -1.0f;
    g_matProjection._21 = 0.0f;
    g_matProjection._31 = 0.0f;
    g_matProjection._41 = 0.0f;
    g_matProjection._12 = 0.0f;
    g_matProjection._22 = 1.0f;
    g_matProjection._32 = 0.0f;
    g_matProjection._42 = 0.0f;
    g_matProjection._13 = 0.0f;
    g_matProjection._23 = 0.0f;
    g_matProjection._33 = 1.0f;
    g_matProjection._43 = -POLY_ZCLIP_PLANE;
    g_matProjection._14 = 0.0f;
    g_matProjection._24 = 0.0f;
    g_matProjection._34 = 1.0f;
    g_matProjection._44 = 0.0f;

    hres = (the_display.lp_D3D_Device)->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_matProjection);

    // Viewport: maps clip-space [-1,1] to pixel coordinates.
    memset(&g_viewData, 0, sizeof(D3DVIEWPORT2));
    g_viewData.dwSize = sizeof(D3DVIEWPORT2);
    float fMyMulX = POLY_screen_mul_x * POLY_ZCLIP_PLANE;
    float fMyMulY = POLY_screen_mul_y * POLY_ZCLIP_PLANE;
    g_dw3DStuffHeight = fMyMulY * PolyPage::s_YScale * 2;
    g_dw3DStuffY = (POLY_screen_mid_y - fMyMulY) * PolyPage::s_YScale;
    g_viewData.dwWidth = fMyMulX * PolyPage::s_XScale * 2;
    g_viewData.dwHeight = fMyMulY * PolyPage::s_YScale * 2;
    g_viewData.dwX = (POLY_screen_mid_x - fMyMulX) * PolyPage::s_XScale;
    g_viewData.dwY = (POLY_screen_mid_y - fMyMulY) * PolyPage::s_YScale;
    g_viewData.dvClipX = -1.0f;
    g_viewData.dvClipY = 1.0;
    g_viewData.dvClipWidth = 2.0f;
    g_viewData.dvClipHeight = 2.0f;
    g_viewData.dvMinZ = 0.0f;
    g_viewData.dvMaxZ = 1.0f;

    hres = (the_display.lp_D3D_Viewport)->SetViewport2(&g_viewData);

    SUPERFACET_start_frame();
}

// uc_orig: POLY_setclip (fallen/DDEngine/Headers/poly.h)
// Sets clip flags on an already-projected POLY_Point.
// Must NOT just set clip = TRANSFORMED — must preserve existing near/far bits.
extern inline void POLY_setclip(POLY_Point* pt)
{
    pt->clip = POLY_CLIP_TRANSFORMED;

    if (pt->X < POLY_screen_clip_left)
        pt->clip |= POLY_CLIP_LEFT;
    else if (pt->X > POLY_screen_clip_right)
        pt->clip |= POLY_CLIP_RIGHT;

    if (pt->Y < POLY_screen_clip_top)
        pt->clip |= POLY_CLIP_TOP;
    else if (pt->Y > POLY_screen_clip_bottom)
        pt->clip |= POLY_CLIP_BOTTOM;
}

// uc_orig: POLY_perspective (fallen/DDEngine/Headers/poly.h)
// Perspective-projects a view-space POLY_Point: fills X, Y (screen pixels), Z (1/z), and clip flags.
// wibble_key: non-zero applies sinusoidal horizontal shimmer (water effect).
inline void POLY_perspective(POLY_Point* pt, UBYTE wibble_key)
{
    if (pt->z < POLY_Z_NEARPLANE) {
        pt->clip = POLY_CLIP_NEAR;
    } else if (pt->z > 1.0F) {
        pt->clip = POLY_CLIP_FAR;
    } else {
        pt->Z = POLY_ZCLIP_PLANE / pt->z;

        pt->X = POLY_screen_mid_x - POLY_screen_mul_x * pt->x * pt->Z;
        pt->Y = POLY_screen_mid_y - POLY_screen_mul_y * pt->y * pt->Z;

        if (wibble_key) {
            SLONG offset;
            SLONG angle1;
            SLONG angle2;

            angle1 = wibble_key * POLY_wibble_y1 >> 2;
            angle2 = wibble_key * POLY_wibble_y2 >> 2;
            angle1 += POLY_wibble_dangle1;
            angle2 += POLY_wibble_dangle2;

            angle1 &= 2047;
            angle2 &= 2047;

            offset = SIN(angle1) * POLY_wibble_s1 >> 19;
            offset += COS(angle2) * POLY_wibble_s2 >> 19;

            pt->X += offset;
        }

        POLY_setclip(pt);
    }
}

// uc_orig: POLY_transform_c_saturate_z (fallen/DDEngine/Headers/poly.h)
// Like POLY_transform but clamps Z to the near plane rather than discarding the point.
// Used for objects that must always render even when partly behind the camera.
void POLY_transform_c_saturate_z(
    float world_x,
    float world_y,
    float world_z,
    POLY_Point* pt)
{
    pt->x = world_x - POLY_cam_x;
    pt->y = world_y - POLY_cam_y;
    pt->z = world_z - POLY_cam_z;

    MATRIX_MUL(
        POLY_cam_matrix,
        pt->x,
        pt->y,
        pt->z);

    if (pt->z < POLY_Z_NEARPLANE) {
        pt->clip = POLY_CLIP_NEAR;
    } else {
        pt->Z = POLY_ZCLIP_PLANE / pt->z;

        pt->X = POLY_screen_mid_x - POLY_screen_mul_x * pt->x * pt->Z;
        pt->Y = POLY_screen_mid_y - POLY_screen_mul_y * pt->y * pt->Z;

        pt->clip = POLY_CLIP_TRANSFORMED;

        if (pt->X < POLY_screen_clip_left)
            pt->clip |= POLY_CLIP_LEFT;
        else if (pt->X > POLY_screen_clip_right)
            pt->clip |= POLY_CLIP_RIGHT;

        if (pt->Y < POLY_screen_clip_top)
            pt->clip |= POLY_CLIP_TOP;
        else if (pt->Y > POLY_screen_clip_bottom)
            pt->clip |= POLY_CLIP_BOTTOM;
    }
}

// uc_orig: POLY_transform_from_view_space (fallen/DDEngine/Headers/poly.h)
// Projects a point that is already in camera/view space (x,y,z already set by caller).
void POLY_transform_from_view_space(POLY_Point* pt)
{
    if (pt->z < POLY_Z_NEARPLANE) {
        pt->clip = POLY_CLIP_NEAR;
    } else {
        pt->Z = POLY_ZCLIP_PLANE / pt->z;

        pt->X = POLY_screen_mid_x - POLY_screen_mul_x * pt->x * pt->Z;
        pt->Y = POLY_screen_mid_y - POLY_screen_mul_y * pt->y * pt->Z;

        pt->clip = POLY_CLIP_TRANSFORMED;

        if (pt->X < POLY_screen_clip_left) {
            pt->clip |= POLY_CLIP_LEFT;
        } else if (pt->X > POLY_screen_clip_right) {
            pt->clip |= POLY_CLIP_RIGHT;
        }

        if (pt->Y < POLY_screen_clip_top) {
            pt->clip |= POLY_CLIP_TOP;
        } else if (pt->Y > POLY_screen_clip_bottom) {
            pt->clip |= POLY_CLIP_BOTTOM;
        }
    }
}

// uc_orig: POLY_transform_abs (fallen/DDEngine/Headers/poly.h)
// Transforms a world-space point without subtracting the camera origin.
// Used for objects already positioned relative to the camera.
void POLY_transform_abs(
    float world_x,
    float world_y,
    float world_z,
    POLY_Point* pt)
{
    pt->x = world_x;
    pt->y = world_y;
    pt->z = world_z;

    MATRIX_MUL(
        POLY_cam_matrix,
        pt->x,
        pt->y,
        pt->z);

    POLY_perspective(pt);
}

// uc_orig: POLY_get_screen_pos (fallen/DDEngine/Headers/poly.h)
// Projects a world-space point directly to screen coordinates without creating a POLY_Point.
// Returns UC_TRUE if the point is in front of the camera (z > near plane).
SLONG POLY_get_screen_pos(
    float world_x,
    float world_y,
    float world_z,
    float* screen_x,
    float* screen_y)
{
    float vx = world_x - POLY_cam_x;
    float vy = world_y - POLY_cam_y;
    float vz = world_z - POLY_cam_z;

    MATRIX_MUL(
        POLY_cam_matrix,
        vx,
        vy,
        vz);

    if (vz < POLY_Z_NEARPLANE) {
        return UC_FALSE;
    } else {
        float Z = POLY_ZCLIP_PLANE / vz;

        *screen_x = POLY_screen_mid_x - POLY_screen_mul_x * vx * Z;
        *screen_y = POLY_screen_mid_y - POLY_screen_mul_y * vy * Z;

        return UC_TRUE;
    }
}

// uc_orig: POLY_set_local_rotation (fallen/DDEngine/Headers/poly.h)
// Precomputes the combined camera+object transform for a mesh.
// Call once per mesh, before transforming its vertices with POLY_transform_using_local_rotation.
// off_x/y/z: object world position. matrix[9]: object's 3x3 rotation (row-major).
void POLY_set_local_rotation(
    float off_x,
    float off_y,
    float off_z,
    float matrix[9])
{
    POLY_cam_off_x = off_x - POLY_cam_x;
    POLY_cam_off_y = off_y - POLY_cam_y;
    POLY_cam_off_z = off_z - POLY_cam_z;

    MATRIX_MUL(
        POLY_cam_matrix,
        POLY_cam_off_x,
        POLY_cam_off_y,
        POLY_cam_off_z);

    MATRIX_3x3mul(
        POLY_cam_matrix_comb,
        POLY_cam_matrix,
        matrix);

    // Upload the combined world transform to Direct3D.
    g_matWorld._11 = POLY_cam_matrix_comb[0];
    g_matWorld._21 = POLY_cam_matrix_comb[1];
    g_matWorld._31 = POLY_cam_matrix_comb[2];
    g_matWorld._41 = POLY_cam_off_x;
    g_matWorld._12 = POLY_cam_matrix_comb[3];
    g_matWorld._22 = POLY_cam_matrix_comb[4];
    g_matWorld._32 = POLY_cam_matrix_comb[5];
    g_matWorld._42 = POLY_cam_off_y;
    g_matWorld._13 = POLY_cam_matrix_comb[6];
    g_matWorld._23 = POLY_cam_matrix_comb[7];
    g_matWorld._33 = POLY_cam_matrix_comb[8];
    g_matWorld._43 = POLY_cam_off_z;
    g_matWorld._14 = 0.0f;
    g_matWorld._24 = 0.0f;
    g_matWorld._34 = 0.0f;
    g_matWorld._44 = 1.0f;
    HRESULT hres = (the_display.lp_D3D_Device)->SetTransform(D3DTRANSFORMSTATE_WORLD, &g_matWorld);
}

// uc_orig: POLY_set_local_rotation_none (fallen/DDEngine/Headers/poly.h)
// Uploads the camera-only transform to D3D. Equivalent to POLY_set_local_rotation with identity object matrix.
void POLY_set_local_rotation_none(void)
{
    POLY_cam_off_x = -POLY_cam_x;
    POLY_cam_off_y = -POLY_cam_y;
    POLY_cam_off_z = -POLY_cam_z;

    MATRIX_MUL(
        POLY_cam_matrix,
        POLY_cam_off_x,
        POLY_cam_off_y,
        POLY_cam_off_z);

    g_matWorld._11 = POLY_cam_matrix[0];
    g_matWorld._21 = POLY_cam_matrix[1];
    g_matWorld._31 = POLY_cam_matrix[2];
    g_matWorld._41 = POLY_cam_off_x;
    g_matWorld._12 = POLY_cam_matrix[3];
    g_matWorld._22 = POLY_cam_matrix[4];
    g_matWorld._32 = POLY_cam_matrix[5];
    g_matWorld._42 = POLY_cam_off_y;
    g_matWorld._13 = POLY_cam_matrix[6];
    g_matWorld._23 = POLY_cam_matrix[7];
    g_matWorld._33 = POLY_cam_matrix[8];
    g_matWorld._43 = POLY_cam_off_z;
    g_matWorld._14 = 0.0f;
    g_matWorld._24 = 0.0f;
    g_matWorld._34 = 0.0f;
    g_matWorld._44 = 1.0f;
    HRESULT hres = (the_display.lp_D3D_Device)->SetTransform(D3DTRANSFORMSTATE_WORLD, &g_matWorld);
}

// uc_orig: POLY_transform_using_local_rotation_and_wibble (fallen/DDEngine/Headers/poly.h)
// Transforms a local-space vertex through the combined local+camera matrix, then applies wibble.
void POLY_transform_using_local_rotation_and_wibble(
    float local_x,
    float local_y,
    float local_z,
    POLY_Point* pt,
    UBYTE wibble_key)
{
    pt->x = local_x;
    pt->y = local_y;
    pt->z = local_z;

    MATRIX_MUL(
        POLY_cam_matrix_comb,
        pt->x,
        pt->y,
        pt->z);

    pt->x += POLY_cam_off_x;
    pt->y += POLY_cam_off_y;
    pt->z += POLY_cam_off_z;

    POLY_perspective(pt, wibble_key);
}

// uc_orig: POLY_sphere_visible (fallen/DDEngine/Headers/poly.h)
// Frustum cull: returns UC_TRUE if the sphere is (possibly) visible.
// radius is the sphere radius divided by view_dist (fractional, not world units).
SLONG POLY_sphere_visible(
    float world_x,
    float world_y,
    float world_z,
    float radius)
{
    float view_x;
    float view_y;
    float view_z;

    view_x = world_x - POLY_cam_x;
    view_y = world_y - POLY_cam_y;
    view_z = world_z - POLY_cam_z;

    MATRIX_MUL(
        POLY_cam_matrix,
        view_x,
        view_y,
        view_z);

    if (view_z + radius <= POLY_Z_NEARPLANE) {
        // Behind the view pyramid.
        return UC_FALSE;
    }

    if (view_x + radius < -view_z || view_x - radius > +view_z) {
        return UC_FALSE;
    }

    if (view_y + radius * 1.4F < -view_z || view_y - radius * 1.4F > +view_z) {
        return UC_FALSE;
    }

    return UC_TRUE;
}

// uc_orig: POLY_fadeout_buffer (fallen/DDEngine/Headers/poly.h)
// Applies distance fog to all vertices in POLY_buffer[0..POLY_buffer_upto].
void POLY_fadeout_buffer()
{
    SLONG i;

    for (i = 0; i < POLY_buffer_upto; i++) {
        POLY_fadeout_point(&POLY_buffer[i]);
    }
}

// uc_orig: POLY_frame_init (fallen/DDEngine/Headers/poly.h)
// Clears all polygon page buckets for the new frame and sets the default D3D fog colour.
// keep_shadow_page/keep_text_page: if UC_TRUE, preserves those pages' geometry from last frame.
void POLY_frame_init(SLONG keep_shadow_page, SLONG keep_text_page)
{
    SLONG i;

    for (i = 0; i < POLY_NUM_PAGES; i++) {
        if (keep_shadow_page && i == POLY_PAGE_SHADOW) {
            // Keep this stuff...
        } else if (keep_text_page && i == POLY_PAGE_TEXT) {
            // Keep this stuff...
        } else {
            POLY_Page[i].Clear();
        }
    }

    // Initialise render states and begin scene.
    POLY_init_render_states();

    ULONG fog_colour;
    if ((GAME_FLAGS & GF_SEWERS) || (GAME_FLAGS & GF_INDOORS)) {
        fog_colour = 0;
    } else {
        if (fade_black) {
            fog_colour = 0;
        } else {
            fog_colour = (NIGHT_sky_colour.red << 16) | (NIGHT_sky_colour.green << 8) | (NIGHT_sky_colour.blue << 0);
        }

        if (draw_3d) {
            SLONG white = NIGHT_sky_colour.red + NIGHT_sky_colour.green + NIGHT_sky_colour.blue;

            white /= 3;

            fog_colour = (white << 16) | (white << 8) | (white << 0);
        }
    }

    // set default render state
    DefRenderState.InitScene(fog_colour);

    BEGIN_SCENE;
}

// uc_orig: POLY_valid_triangle (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the triangle's vertices are valid (transformed) and not all off-screen.
SLONG POLY_valid_triangle(POLY_Point* pp[3])
{
    if (!pp[0]->MaybeValid())
        return UC_FALSE;
    if (!pp[1]->MaybeValid())
        return UC_FALSE;
    if (!pp[2]->MaybeValid())
        return UC_FALSE;

    if ((pp[0]->clip & pp[1]->clip & pp[2]->clip) & POLY_CLIP_OFFSCREEN) {
        return UC_FALSE;
    }

    return UC_TRUE;
}

// uc_orig: POLY_valid_quad (fallen/DDEngine/Headers/poly.h)
SLONG POLY_valid_quad(POLY_Point* pp[4])
{
    if (!pp[0]->MaybeValid())
        return UC_FALSE;
    if (!pp[1]->MaybeValid())
        return UC_FALSE;
    if (!pp[2]->MaybeValid())
        return UC_FALSE;
    if (!pp[3]->MaybeValid())
        return UC_FALSE;

    if ((pp[0]->clip & pp[1]->clip & pp[2]->clip & pp[3]->clip) & POLY_CLIP_OFFSCREEN) {
        return UC_FALSE;
    }

    return UC_TRUE;
}

// uc_orig: POLY_valid_line (fallen/DDEngine/Headers/poly.h)
SLONG POLY_valid_line(POLY_Point* p1, POLY_Point* p2)
{
    if (!p1->IsValid())
        return UC_FALSE;
    if (!p2->IsValid())
        return UC_FALSE;

    if ((p1->clip & p2->clip) & POLY_CLIP_OFFSCREEN) {
        return UC_FALSE;
    }

    return UC_TRUE;
}

// uc_orig: POLY_tri_backfacing (fallen/DDEngine/Source/poly.cpp)
// Returns true if the triangle (in view space) is back-facing relative to the camera.
// Computes cross product of two edges, dots with the eye vector at vertex 1.
inline bool POLY_tri_backfacing(POLY_Point* pp1, POLY_Point* pp2, POLY_Point* pp3)
{
    float x12, y12, z12;
    float x13, y13, z13;
    float cx, cy, cz;
    float dp;

    ASSERT(pp1->MaybeValid());
    ASSERT(pp2->MaybeValid());
    ASSERT(pp3->MaybeValid());

    x12 = pp2->x - pp1->x;
    y12 = pp2->y - pp1->y;
    z12 = pp2->z - pp1->z;

    x13 = pp3->x - pp1->x;
    y13 = pp3->y - pp1->y;
    z13 = pp3->z - pp1->z;

    cx = y12 * z13 - z12 * y13;
    cy = z12 * x13 - x12 * z13;
    cz = x12 * y13 - y12 * x13;

    dp = cx * pp1->x + cy * pp1->y + cz * pp1->z;

    return (dp < 0);
}

// uc_orig: TweenD3DColour (fallen/DDEngine/Source/poly.cpp)
// Linearly interpolates two ARGB D3D colours at 8-bit fraction lambda8 (0=c1, 256=c2).
// Processes R/B and A/G channel pairs together to reduce multiplications.
static inline ULONG TweenD3DColour(ULONG c1, ULONG c2, ULONG lambda8)
{
    if (c1 == c2)
        return c1;
    if (lambda8 == 0)
        return c1;
    if (lambda8 == 256)
        return c2;

    // lerp R & B
    SLONG rb1 = c1 & 0x00FF00FF;
    SLONG rb2 = c2 & 0x00FF00FF;

    if (rb1 != rb2) {
        rb2 -= rb1;
        rb2 *= lambda8;
        rb1 += rb2 >> 8;
        rb1 &= 0x00FF00FF;
    }

    // lerp A & G
    SLONG ag1 = (c1 >> 8) & 0x00FF00FF;
    SLONG ag2 = (c2 >> 8) & 0x00FF00FF;

    if (ag1 != ag2) {
        ag2 -= ag1;
        ag2 *= lambda8;
        ag1 += ag2 >> 8;
        ag1 &= 0x00FF00FF;
    }

    return rb1 + (ag1 << 8);
}

// Near-plane clip buffer: newly generated vertices from Sutherland-Hodgman clipping go here.
// uc_orig: s_PointBuffer (fallen/DDEngine/Source/poly.cpp)
static POLY_Point s_PointBuffer[32];
// uc_orig: s_PointBufferOffset (fallen/DDEngine/Source/poly.cpp)
static ULONG s_PointBufferOffset;

// uc_orig: NewTweenVertex3D (fallen/DDEngine/Source/poly.cpp)
// Creates a new vertex interpolated between p1 and p2 at the near clip plane (z = POLY_Z_NEARPLANE).
// Used during near-plane Sutherland-Hodgman clipping.
POLY_Point* NewTweenVertex3D(POLY_Point* p1, POLY_Point* p2, float lambda)
{
    ULONG lambda8;

    // Fast float->8-bit fraction via bit manipulation.
    *(float*)&lambda8 = lambda + 32768.0F;
    lambda8 &= 0x1FF;

    POLY_Point* np = &s_PointBuffer[s_PointBufferOffset++];

    np->x = p1->x + lambda * (p2->x - p1->x);
    np->y = p1->y + lambda * (p2->y - p1->y);
    np->z = POLY_Z_NEARPLANE; // clamp to near plane

    np->u = p1->u + lambda * (p2->u - p1->u);
    np->v = p1->v + lambda * (p2->v - p1->v);

    np->colour = TweenD3DColour(p1->colour, p2->colour, lambda8);
    np->specular = TweenD3DColour(p1->specular, p2->specular, lambda8);

    POLY_perspective(np);
    ASSERT(np->IsValid());

    return np;
}

// uc_orig: NewTweenVertex2D_X (fallen/DDEngine/Source/poly.cpp)
// Creates a new screen-space vertex clipped to a vertical screen edge (constant X).
POLY_Point* NewTweenVertex2D_X(POLY_Point* p1, POLY_Point* p2, float lambda, float xcoord)
{
    ULONG lambda8;

    *(float*)&lambda8 = lambda + 32768.0F;
    lambda8 &= 0x1FF;

    POLY_Point* np = &s_PointBuffer[s_PointBufferOffset++];

    np->X = xcoord;
    np->Y = p1->Y + lambda * (p2->Y - p1->Y);
    np->Z = p1->Z + lambda * (p2->Z - p1->Z);

    // Perspective-correct UV interpolation.
    float u1 = p1->u * p1->Z;
    float v1 = p1->v * p1->Z;
    float u2 = p2->u * p2->Z;
    float v2 = p2->v * p2->Z;
    float rz = 1.0F / np->Z;

    np->u = rz * (u1 + lambda * (u2 - u1));
    np->v = rz * (v1 + lambda * (v2 - v1));

    np->colour = TweenD3DColour(p1->colour, p2->colour, lambda8);
    np->specular = TweenD3DColour(p1->specular, p2->specular, lambda8);

    POLY_setclip(np);

    return np;
}

// uc_orig: NewTweenVertex2D_Y (fallen/DDEngine/Source/poly.cpp)
// Creates a new screen-space vertex clipped to a horizontal screen edge (constant Y).
POLY_Point* NewTweenVertex2D_Y(POLY_Point* p1, POLY_Point* p2, float lambda, float ycoord)
{
    ULONG lambda8;

    *(float*)&lambda8 = lambda + 32768.0F;
    lambda8 &= 0x1FF;

    POLY_Point* np = &s_PointBuffer[s_PointBufferOffset++];

    np->X = p1->X + lambda * (p2->X - p1->X);
    np->Y = ycoord;
    np->Z = p1->Z + lambda * (p2->Z - p1->Z);

    // Perspective-correct UV interpolation.
    float u1 = p1->u * p1->Z;
    float v1 = p1->v * p1->Z;
    float u2 = p2->u * p2->Z;
    float v2 = p2->v * p2->Z;
    float rz = 1.0F / np->Z;

    np->u = rz * (u1 + lambda * (u2 - u1));
    np->v = rz * (v1 + lambda * (v2 - v1));

    np->colour = TweenD3DColour(p1->colour, p2->colour, lambda8);
    np->specular = TweenD3DColour(p1->specular, p2->specular, lambda8);

    POLY_setclip(np);

    return np;
}

// uc_orig: POLY_clip_against_nearplane (fallen/DDEngine/Headers/poly.h)
// Sutherland-Hodgman clip against the near plane. rptr/dptr: input polygon and distances.
// Outputs to wbuf. Returns the number of output vertices.
SLONG POLY_clip_against_nearplane(POLY_Point** rptr, float* dptr, SLONG count, POLY_Point** wbuf)
{
    POLY_Point** wptr = wbuf;

    POLY_Point* p1;
    POLY_Point* p2;

    SLONG ii;
    for (ii = 0; ii < count - 1; ii++) {
        p1 = rptr[ii];
        p2 = rptr[ii + 1];

        if (dptr[ii] >= 0) {
            *wptr++ = p1;
            if (dptr[ii + 1] >= 0) {
                continue;
            }
        } else {
            if (dptr[ii + 1] < 0) {
                continue;
            }
        }

        *wptr++ = NewTweenVertex3D(p1, p2, dptr[ii] / (dptr[ii] - dptr[ii + 1]));
    }

    p1 = rptr[ii];
    p2 = rptr[0];

    if (dptr[ii] >= 0) {
        *wptr++ = p1;
        if (dptr[0] < 0) {
            *wptr++ = NewTweenVertex3D(p1, p2, dptr[ii] / (dptr[ii] - dptr[0]));
        }
    } else {
        if (dptr[0] >= 0) {
            *wptr++ = NewTweenVertex3D(p1, p2, dptr[ii] / (dptr[ii] - dptr[0]));
        }
    }

    return wptr - wbuf;
}

// uc_orig: POLY_clip_against_side_X (fallen/DDEngine/Headers/poly.h)
// Sutherland-Hodgman clip against a left or right screen edge (constant X).
SLONG POLY_clip_against_side_X(POLY_Point** rptr, float* dptr, SLONG count, POLY_Point** wbuf, float xcoord)
{
    POLY_Point** wptr = wbuf;

    POLY_Point* p1;
    POLY_Point* p2;

    SLONG ii;
    for (ii = 0; ii < count - 1; ii++) {
        p1 = rptr[ii];
        p2 = rptr[ii + 1];

        if (dptr[ii] >= 0) {
            *wptr++ = p1;
            if (dptr[ii + 1] >= 0) {
                continue;
            }
        } else {
            if (dptr[ii + 1] < 0) {
                continue;
            }
        }

        *wptr++ = NewTweenVertex2D_X(p1, p2, dptr[ii] / (dptr[ii] - dptr[ii + 1]), xcoord);
    }

    p1 = rptr[ii];
    p2 = rptr[0];

    if (dptr[ii] >= 0) {
        *wptr++ = p1;
        if (dptr[0] < 0) {
            *wptr++ = NewTweenVertex2D_X(p1, p2, dptr[ii] / (dptr[ii] - dptr[0]), xcoord);
        }
    } else {
        if (dptr[0] >= 0) {
            *wptr++ = NewTweenVertex2D_X(p1, p2, dptr[ii] / (dptr[ii] - dptr[0]), xcoord);
        }
    }

    return wptr - wbuf;
}

// uc_orig: POLY_clip_against_side_Y (fallen/DDEngine/Headers/poly.h)
// Sutherland-Hodgman clip against a top or bottom screen edge (constant Y).
SLONG POLY_clip_against_side_Y(POLY_Point** rptr, float* dptr, SLONG count, POLY_Point** wbuf, float ycoord)
{
    POLY_Point** wptr = wbuf;

    POLY_Point* p1;
    POLY_Point* p2;

    SLONG ii;
    for (ii = 0; ii < count - 1; ii++) {
        p1 = rptr[ii];
        p2 = rptr[ii + 1];

        if (dptr[ii] >= 0) {
            *wptr++ = p1;
            if (dptr[ii + 1] >= 0) {
                continue;
            }
        } else {
            if (dptr[ii + 1] < 0) {
                continue;
            }
        }

        *wptr++ = NewTweenVertex2D_Y(p1, p2, dptr[ii] / (dptr[ii] - dptr[ii + 1]), ycoord);
    }

    p1 = rptr[ii];
    p2 = rptr[0];

    if (dptr[ii] >= 0) {
        *wptr++ = p1;
        if (dptr[0] < 0) {
            *wptr++ = NewTweenVertex2D_Y(p1, p2, dptr[ii] / (dptr[ii] - dptr[0]), ycoord);
        }
    } else {
        if (dptr[0] >= 0) {
            *wptr++ = NewTweenVertex2D_Y(p1, p2, dptr[ii] / (dptr[ii] - dptr[0]), ycoord);
        }
    }

    return wptr - wbuf;
}

// Scratch distance/pointer buffers for clipping.
// uc_orig: s_DistBuffer (fallen/DDEngine/Source/poly.cpp)
static float s_DistBuffer[128];
// uc_orig: s_PtrBuffer (fallen/DDEngine/Source/poly.cpp)
static POLY_Point* s_PtrBuffer[128];

// uc_orig: POLY_add_nearclipped_triangle (fallen/DDEngine/Source/poly.cpp)
// Submits a triangle that straddles the near clip plane.
// Clips against the near plane using Sutherland-Hodgman, then fans the result into PolyPage.
void POLY_add_nearclipped_triangle(POLY_Point* pt[3], SLONG page, SLONG backface_cull)
{
    POLY_Point** rptr = pt;
    POLY_Point** wptr = s_PtrBuffer;

    s_PointBufferOffset = 0;

    {
        SLONG i;
        SLONG j;
        SLONG laura;
        SLONG poly_points;

        s_DistBuffer[0] = rptr[0]->z - POLY_Z_NEARPLANE;
        s_DistBuffer[1] = rptr[1]->z - POLY_Z_NEARPLANE;
        s_DistBuffer[2] = rptr[2]->z - POLY_Z_NEARPLANE;

        poly_points = POLY_clip_against_nearplane(rptr, s_DistBuffer, 3, wptr);

        if (!poly_points) {
            return;
        }

        rptr = wptr;

        SLONG clip_and = 0xffffffff;

        for (i = 0; i < poly_points; i++) {
            POLY_setclip(rptr[i]);

            clip_and &= rptr[i]->clip;
        }

        if (clip_and & POLY_CLIP_OFFSCREEN) {
            return;
        }

        if (backface_cull && POLY_tri_backfacing(rptr[0], rptr[1], rptr[2])) {
            return;
        }

    second_page:;

        PolyPage* pp = &POLY_Page[page];
        PolyPage* ppDrawn = pp->pTheRealPolyPage;

        // Fan-triangulate the clipped polygon: allocate 3 + (poly_points-3)*3 verts.
        PolyPoint2D* pv = ppDrawn->PointAlloc(3 + (poly_points - 3) * 3);

        POLY_Point* ppt;
        PolyPoint2D* pv_first = pv;

        ppt = rptr[0];

        pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
        pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
        pv->SetColour(ppt->colour);
        pv->SetSpecular(ppt->specular);

        pv++;

        laura = 2;

        while (1) {
            ppt = rptr[laura - 1];

            pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
            pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
            pv->SetColour(ppt->colour);
            pv->SetSpecular(ppt->specular);

            pv++;

            ppt = rptr[laura];

            pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
            pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
            pv->SetColour(ppt->colour);
            pv->SetSpecular(ppt->specular);

            laura++;

            if (laura >= poly_points) {
                break;
            }

            pv++;

            *pv = *pv_first;

            pv++;
        }

        if (POLY_page_flag[page] & POLY_PAGE_FLAG_2PASS) {
            page += 1;

            goto second_page;
        }
    }

    return;
}

// uc_orig: POLY_add_triangle_fast (fallen/DDEngine/Source/poly.cpp)
// Submits a triangle to the given texture page bucket.
// Skips offscreen triangles. Near-clips if necessary. Optionally backface-culls.
void POLY_add_triangle_fast(POLY_Point* pt[3], SLONG page, SLONG backface_cull, SLONG generate_clip_flags)
{
    if (generate_clip_flags) {
        POLY_setclip(pt[0]);
        POLY_setclip(pt[1]);
        POLY_setclip(pt[2]);
    }

    if (pt[0]->clip & pt[1]->clip & pt[2]->clip & POLY_CLIP_OFFSCREEN) {
        return;
    }

    if ((pt[0]->clip | pt[1]->clip | pt[2]->clip) & POLY_CLIP_NEAR) {
        POLY_add_nearclipped_triangle(pt, page, backface_cull);
        return;
    }

    if (backface_cull && POLY_tri_backfacing(pt[0], pt[1], pt[2])) {
        return;
    }

second_page:;

    PolyPage* pp = &POLY_Page[page];
    PolyPage* ppDrawn = pp->pTheRealPolyPage;

    PolyPoint2D* pv = ppDrawn->PointAlloc(3);

    POLY_Point* ppt;
    PolyPoly* ppoly = ppDrawn->PolyBufAlloc();
    if (!ppoly)
        return;
    ppoly->first_vertex = pv - ppDrawn->m_VertexPtr;
    ppoly->num_vertices = 3;

    ASSERT(pv);

    ppt = pt[0];

    pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
    pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
    pv->SetColour(ppt->colour);
    pv->SetSpecular(ppt->specular);

    pv++;

    ppt = pt[1];

    pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
    pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
    pv->SetColour(ppt->colour);
    pv->SetSpecular(ppt->specular);

    pv++;

    ppt = pt[2];

    pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
    pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
    pv->SetColour(ppt->colour);
    pv->SetSpecular(ppt->specular);

    if (POLY_page_flag[page] & POLY_PAGE_FLAG_2PASS) {
        page += 1;

        goto second_page;
    }
}

// uc_orig: POLY_add_quad_fast (fallen/DDEngine/Source/poly.cpp)
// Submits a quad (as two triangles) to the given texture page bucket.
// Near-clips if necessary. Handles backface cull per triangle.
void POLY_add_quad_fast(POLY_Point* pt[4], SLONG page, SLONG backface_cull, SLONG generate_clip_flags)
{
    if (generate_clip_flags) {
        POLY_setclip(pt[0]);
        POLY_setclip(pt[1]);
        POLY_setclip(pt[2]);
        POLY_setclip(pt[3]);
    }

    if (pt[0]->clip & pt[1]->clip & pt[2]->clip & pt[3]->clip & POLY_CLIP_OFFSCREEN) {
        return;
    }

    if ((pt[0]->clip | pt[1]->clip | pt[2]->clip | pt[3]->clip) & POLY_CLIP_NEAR) {
        POLY_Point* pt2[3] = { pt[1], pt[3], pt[2] };

        // Split quad into two triangles for near clipping.
        POLY_add_triangle(pt, page, backface_cull, UC_FALSE);
        POLY_add_triangle(pt2, page, backface_cull, UC_FALSE);

        return;
    }

    if (backface_cull) {
        SLONG cull;

        cull = 0;

        if (POLY_tri_backfacing(pt[0], pt[1], pt[2])) {
            cull |= 1;
        }
        if (POLY_tri_backfacing(pt[1], pt[3], pt[2])) {
            cull |= 2;
        }

        if (cull == 0) {
            // Draw the whole quad.
        } else if (cull == 3) {
            return;
        } else if (cull == 1) {
            // First triangle culled; draw only the second.
            POLY_add_triangle(pt + 1, page, UC_FALSE, UC_FALSE);
            return;
        } else if (cull == 2) {
            // Second triangle culled; draw only the first.
            POLY_add_triangle(pt, page, UC_FALSE, UC_FALSE);
            return;
        }
    }

second_page:;

    PolyPage* pp = &POLY_Page[page];
    PolyPage* ppDrawn = pp->pTheRealPolyPage;

    PolyPoint2D* pv = ppDrawn->PointAlloc(6);
    POLY_Point* ppt;
    PolyPoly* ppoly = ppDrawn->PolyBufAlloc();
    if (!ppoly)
        return;
    ppoly->first_vertex = pv - ppDrawn->m_VertexPtr;
    ppoly->num_vertices = 3;
    ppoly = pp->PolyBufAlloc();
    if (!ppoly)
        return;
    ppoly->first_vertex = pv - ppDrawn->m_VertexPtr + 3;
    ppoly->num_vertices = 3;

    ASSERT(pv);

    ppt = pt[0];

    pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
    pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
    pv->SetColour(ppt->colour);
    pv->SetSpecular(ppt->specular);

    pv++;

    ppt = pt[1];

    pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
    pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
    pv->SetColour(ppt->colour);
    pv->SetSpecular(ppt->specular);

    pv++;

    ppt = pt[2];

    pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
    pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
    pv->SetColour(ppt->colour);
    pv->SetSpecular(ppt->specular);

    pv++;

    ppt = pt[3];

    pv->SetSC(ppt->X * pp->s_XScale, ppt->Y * pp->s_YScale, 1.0F - ppt->Z);
    pv->SetUV2(ppt->u * pp->m_UScale + pp->m_UOffset, ppt->v * pp->m_VScale + pp->m_VOffset);
    pv->SetColour(ppt->colour);
    pv->SetSpecular(ppt->specular);

    pv[1] = pv[-1];
    pv[2] = pv[-2];

    if (POLY_page_flag[page] & POLY_PAGE_FLAG_2PASS) {
        page += 1;

        goto second_page;
    }
}

// uc_orig: POLY_add_quad (fallen/DDEngine/Headers/poly.h)
void POLY_add_quad(POLY_Point* pp[4], SLONG page, SLONG backface_cull, SLONG generate_clip_flags)
{
    {
        POLY_add_quad_fast(pp, page, backface_cull, generate_clip_flags);
    }
}

// uc_orig: POLY_add_triangle (fallen/DDEngine/Headers/poly.h)
void POLY_add_triangle(POLY_Point* pp[4], SLONG page, SLONG backface_cull, SLONG generate_clip_flags)
{
    {
        POLY_add_triangle_fast(pp, page, backface_cull, generate_clip_flags);
    }
}

// uc_orig: POLY_world_length_to_screen (fallen/DDEngine/Headers/poly.h)
// Converts a world-space length to approximate screen-space pixels at the current view distance.
float POLY_world_length_to_screen(float world_length)
{
    float view_length = world_length * POLY_cam_over_view_dist;
    float screen_length = view_length * POLY_screen_mul_x;

    return screen_length;
}

// uc_orig: POLY_approx_len (fallen/DDEngine/Headers/poly.h)
// Octagonal approximation of 2D vector length (faster than sqrt).
float POLY_approx_len(float dx, float dy)
{
    return (fabsf(dx) > fabsf(dy)) ? fabsf(dx) + 0.414F * fabsf(dy) : fabsf(dy) + 0.414F * fabsf(dx);
}

// uc_orig: POLY_create_cylinder_points (fallen/DDEngine/Headers/poly.h)
// Expands a screen-space line segment into 4 billboard quad vertices with given half-widths.
void POLY_create_cylinder_points(POLY_Point* p1, POLY_Point* p2, float width, POLY_Point* pout)
{
    float dx, dy;
    float len, overlen;
    float dx1, dy1;
    float dx2, dy2;

    ASSERT(p1->IsValid());
    ASSERT(p2->IsValid());

    width *= POLY_cam_over_view_dist;

    dx = p2->X - p1->X;
    dy = p2->Y - p1->Y;

    len = POLY_approx_len(dx, dy);
    overlen = 1.0F / len;

    dx *= overlen;
    dy *= overlen;

    dx1 = -dy * width * p1->Z;
    dy1 = dx * width * p1->Z;

    dx2 = -dy * width * p2->Z;
    dy2 = dx * width * p2->Z;

    pout[0] = *p1;
    pout[0].X += dx1;
    pout[0].Y += dy1;

    pout[1] = *p1;
    pout[1].X -= dx1;
    pout[1].Y -= dy1;

    pout[2] = *p2;
    pout[2].X += dx2;
    pout[2].Y += dy2;

    pout[3] = *p2;
    pout[3].X -= dx2;
    pout[3].Y -= dy2;
}

// uc_orig: POLY_add_line_tex_uv (fallen/DDEngine/Source/poly.cpp)
// Submits a 3D billboard line with full UV coordinates from the input POLY_Points.
void POLY_add_line_tex_uv(POLY_Point* p1, POLY_Point* p2, float width1, float width2, SLONG page, UBYTE sort_to_front)
{
    float dx;
    float dy;

    float dx1;
    float dy1;

    float dx2;
    float dy2;

    float vw1;
    float vw2;

    float sw1;
    float sw2;

    float len;
    float overlen;

    if (p1->NearClip() || p2->NearClip())
        return;

    ASSERT(p1->IsValid());
    ASSERT(p2->IsValid());

    POLY_Point pt[4];
    POLY_Point* ppt[4];

    dx = p2->X - p1->X;
    dy = p2->Y - p1->Y;

    len = (fabsf(dx) > fabsf(dy)) ? fabsf(dx) + 0.414F * fabsf(dy) : fabsf(dy) + 0.414F * fabsf(dx);
    overlen = 1.0F / len;

    dx *= overlen;
    dy *= overlen;

    vw1 = width1 * POLY_cam_over_view_dist;
    vw2 = width2 * POLY_cam_over_view_dist;

    sw1 = POLY_screen_mul_x * vw1 * p1->Z;
    sw2 = POLY_screen_mul_x * vw2 * p2->Z;

    dx1 = -dy * sw1;
    dy1 = +dx * sw1;

    dx2 = -dy * sw2;
    dy2 = +dx * sw2;

    if (sort_to_front) {
        p1->Z = 1.0F;
        p1->z = 0.0F;

        p2->Z = 1.0F;
        p2->z = 0.0F;
    }

    pt[0] = *p1;
    pt[1] = *p1;

    pt[0].X -= dx1;
    pt[0].Y -= dy1;

    pt[1].X += dx1;
    pt[1].Y += dy1;

    pt[0].u = p2->u;

    pt[2] = *p2;
    pt[3] = *p2;

    pt[2].X += dx2;
    pt[2].Y += dy2;

    pt[3].X -= dx2;
    pt[3].Y -= dx2;

    pt[2].u = p1->u;

    ppt[0] = &pt[0];
    ppt[1] = &pt[1];
    ppt[2] = &pt[3];
    ppt[3] = &pt[2];

    POLY_add_quad(ppt, page, UC_FALSE, UC_TRUE);
}

// uc_orig: POLY_add_line_tex (fallen/DDEngine/Headers/poly.h)
// Submits a 3D billboard line, setting UV to (0,0)-(1,1).
void POLY_add_line_tex(POLY_Point* p1, POLY_Point* p2, float width1, float width2, SLONG page, UBYTE sort_to_front)
{
    p1->u = p1->v = 0;
    p2->u = p2->v = 1;

    POLY_add_line_tex_uv(p1, p2, width1, width2, page, sort_to_front);
}

// uc_orig: POLY_add_line (fallen/DDEngine/Headers/poly.h)
// Submits a 3D billboard line without UV (uses POLY_PAGE_COLOUR or similar solid page).
void POLY_add_line(POLY_Point* p1, POLY_Point* p2, float width1, float width2, SLONG page, UBYTE sort_to_front)
{
    float dx;
    float dy;

    float dx1;
    float dy1;

    float dx2;
    float dy2;

    float vw1;
    float vw2;

    float sw1;
    float sw2;

    float len;
    float overlen;

    if (p1->NearClip() || p2->NearClip())
        return;

    ASSERT(p1->IsValid() & p2->IsValid());

    POLY_Point pt[4];
    POLY_Point* ppt[4];

    dx = p2->X - p1->X;
    dy = p2->Y - p1->Y;

    len = (fabsf(dx) > fabsf(dy)) ? fabsf(dx) + 0.414F * fabsf(dy) : fabsf(dy) + 0.414F * fabsf(dx);
    overlen = 1.0F / len;

    dx *= overlen;
    dy *= overlen;

    vw1 = width1 * POLY_cam_over_view_dist;
    vw2 = width2 * POLY_cam_over_view_dist;

    sw1 = POLY_screen_mul_x * vw1 * p1->Z;
    sw2 = POLY_screen_mul_x * vw2 * p2->Z;

    dx1 = -dy * sw1;
    dy1 = +dx * sw1;

    dx2 = -dy * sw2;
    dy2 = +dx * sw2;

    if (sort_to_front) {
        p1->Z = 1.0F;
        p1->z = 0.0F;

        p2->Z = 1.0F;
        p2->z = 0.0F;
    }

    pt[0] = *p1;
    pt[1] = *p1;
    pt[2] = *p2;
    pt[3] = *p2;

    for (int ii = 0; ii < 4; ii++) {
        pt[ii].u = 0;
        pt[ii].v = 0;
    }

    pt[0].X -= dx1;
    pt[0].Y -= dy1;
    pt[1].X += dx1;
    pt[1].Y += dy1;

    pt[2].X += dx2;
    pt[2].Y += dy2;
    pt[3].X -= dx2;
    pt[3].Y -= dy2;

    ppt[0] = &pt[0];
    ppt[1] = &pt[1];
    ppt[2] = &pt[3];
    ppt[3] = &pt[2];

    POLY_add_quad(ppt, page, UC_FALSE, UC_TRUE);
}

// uc_orig: POLY_add_rect (fallen/DDEngine/Source/poly.cpp)
// Submits a screen-space aligned rectangle centred on p1 (top-left corner).
void POLY_add_rect(POLY_Point* p1, SLONG width, SLONG height, SLONG page, UBYTE sort_to_front)
{
    if (p1->NearClip())
        return;

    ASSERT(p1->IsValid());

    POLY_Point pt[4];
    POLY_Point* ppt[4];

    if (sort_to_front) {
        p1->Z = 1.0F;
        p1->z = 0.0F;
    }

    pt[0] = *p1;
    pt[1] = *p1;
    pt[2] = *p1;
    pt[3] = *p1;

    for (int ii = 0; ii < 4; ii++) {
        pt[ii].u = 0;
        pt[ii].v = 0;
    }

    pt[1].X += (float)width;
    pt[3].Y += (float)height;
    pt[2].X += (float)width;
    pt[2].Y += (float)height;

    ppt[0] = &pt[0];
    ppt[1] = &pt[1];
    ppt[2] = &pt[3];
    ppt[3] = &pt[2];

    POLY_add_quad(ppt, page, UC_FALSE, UC_TRUE);
}

// uc_orig: POLY_add_line_2d (fallen/DDEngine/Headers/poly.h)
// Draws a screen-space (2D) line as a thin billboard quad to POLY_PAGE_COLOUR.
void POLY_add_line_2d(float sx1, float sy1, float sx2, float sy2, ULONG colour)
{
    float dx;
    float dy;

    float len;
    float overlen;

    POLY_Point pt[4];
    POLY_Point* ppt[4];

    dx = sx2 - sx1;
    dy = sy2 - sy1;

    len = (fabsf(dx) > fabsf(dy)) ? fabsf(dx) + 0.414F * fabsf(dy) : fabsf(dy) + 0.414F * fabsf(dx);
    overlen = 0.5F / len;

    dx *= overlen;
    dy *= overlen;

    for (int ii = 0; ii < 4; ii++) {
        pt[ii].u = 0;
        pt[ii].v = 0;
        pt[ii].colour = colour;
        pt[ii].specular = 0;
    }

    pt[0].X = sx1 - dy;
    pt[0].Y = sy1 + dx;
    pt[1].X = sx1 + dy;
    pt[1].Y = sy1 - dx;
    pt[2].X = sx2 + dy;
    pt[2].Y = sy2 - dx;
    pt[3].X = sx2 - dy;
    pt[3].Y = sy2 + dx;

    ppt[0] = &pt[0];
    ppt[1] = &pt[1];
    ppt[2] = &pt[3];
    ppt[3] = &pt[2];

    POLY_add_quad(ppt, POLY_PAGE_COLOUR, UC_FALSE, UC_TRUE);
}

// uc_orig: POLY_clip_line_box (fallen/DDEngine/Headers/poly.h)
// Sets the 2D rectangular clip box used by POLY_clip_line_add.
void POLY_clip_line_box(float sx1, float sy1, float sx2, float sy2)
{
    POLY_clip_left = sx1;
    POLY_clip_right = sx2;
    POLY_clip_top = sy1;
    POLY_clip_bottom = sy2;
}

// uc_orig: POLY_clip_line_add (fallen/DDEngine/Headers/poly.h)
// Draws a 2D line clipped to the current POLY_clip_line_box rectangle.
// Uses Cohen-Sutherland clipping.
void POLY_clip_line_add(float sx1, float sy1, float sx2, float sy2, ULONG colour)
{
    UBYTE clip1 = 0;
    UBYTE clip2 = 0;
    UBYTE clip_and;
    UBYTE clip_or;
    UBYTE clip_xor;

    float along;

    if (sx1 < POLY_clip_left) {
        clip1 |= POLY_CLIP_LEFT;
    } else if (sx1 > POLY_clip_right) {
        clip1 |= POLY_CLIP_RIGHT;
    }

    if (sx2 < POLY_clip_left) {
        clip2 |= POLY_CLIP_LEFT;
    } else if (sx2 > POLY_clip_right) {
        clip2 |= POLY_CLIP_RIGHT;
    }

    if (sy1 < POLY_clip_top) {
        clip1 |= POLY_CLIP_TOP;
    } else if (sy1 > POLY_clip_bottom) {
        clip1 |= POLY_CLIP_BOTTOM;
    }

    if (sy2 < POLY_clip_top) {
        clip2 |= POLY_CLIP_TOP;
    } else if (sy2 > POLY_clip_bottom) {
        clip2 |= POLY_CLIP_BOTTOM;
    }

    clip_and = clip1 & clip2;
    clip_or = clip1 | clip2;
    clip_xor = clip1 ^ clip2;

    if (clip_and) {
        return;
    }

#define SWAP_UB(q, w)     \
    {                     \
        UBYTE temp = (q); \
        (q) = (w);        \
        (w) = temp;       \
    }

    if (clip_or) {
        if (clip_xor & POLY_CLIP_LEFT) {
            if (clip2 & POLY_CLIP_LEFT) {
                SWAP_FL(sx1, sx2);
                SWAP_FL(sy1, sy2);
                SWAP_UB(clip1, clip2);
            }

            along = (POLY_clip_left - sx1) / (sx2 - sx1);
            sx1 = POLY_clip_left;
            sy1 = sy1 + along * (sy2 - sy1);
            clip1 &= ~POLY_CLIP_LEFT;

            if (sy1 < POLY_clip_top) {
                clip1 |= POLY_CLIP_TOP;
            } else if (sy1 > POLY_clip_bottom) {
                clip1 |= POLY_CLIP_BOTTOM;
            }

            if (clip1 & clip2) {
                return;
            }

            clip_xor = clip1 ^ clip2;
        }

        if (clip_xor & POLY_CLIP_RIGHT) {
            if (clip2 & POLY_CLIP_RIGHT) {
                SWAP_FL(sx1, sx2);
                SWAP_FL(sy1, sy2);
                SWAP_UB(clip1, clip2);
            }

            along = (POLY_clip_right - sx1) / (sx2 - sx1);
            sx1 = POLY_clip_right;
            sy1 = sy1 + along * (sy2 - sy1);
            clip1 &= ~POLY_CLIP_RIGHT;

            if (sy1 < POLY_clip_top) {
                clip1 |= POLY_CLIP_TOP;
            } else if (sy1 > POLY_clip_bottom) {
                clip1 |= POLY_CLIP_BOTTOM;
            }

            if (clip1 & clip2) {
                return;
            }

            clip_xor = clip1 ^ clip2;
        }

        if (clip_xor & POLY_CLIP_TOP) {
            if (clip2 & POLY_CLIP_TOP) {
                SWAP_FL(sx1, sx2);
                SWAP_FL(sy1, sy2);
                SWAP_UB(clip1, clip2);
            }

            along = (POLY_clip_top - sy1) / (sy2 - sy1);
            sx1 = sx1 + along * (sx2 - sx1);
            sy1 = POLY_clip_top;
            clip1 &= ~POLY_CLIP_TOP;

            if (sx1 < POLY_clip_left) {
                clip1 |= POLY_CLIP_LEFT;
            } else if (sx1 > POLY_clip_right) {
                clip1 |= POLY_CLIP_RIGHT;
            }

            if (clip1 & clip2) {
                return;
            }

            clip_xor = clip1 ^ clip2;
        }

        if (clip_xor & POLY_CLIP_BOTTOM) {
            if (clip2 & POLY_CLIP_BOTTOM) {
                SWAP_FL(sx1, sx2);
                SWAP_FL(sy1, sy2);
                SWAP_UB(clip1, clip2);
            }

            along = (POLY_clip_bottom - sy1) / (sy2 - sy1);
            sx1 = sx1 + along * (sx2 - sx1);
            sy1 = POLY_clip_bottom;
            clip1 &= ~POLY_CLIP_BOTTOM;

            if (sx1 < POLY_clip_left) {
                clip1 |= POLY_CLIP_LEFT;
            } else if (sx1 > POLY_clip_right) {
                clip1 |= POLY_CLIP_RIGHT;
            }

            if (clip1 & clip2) {
                return;
            }

            clip_xor = clip1 ^ clip2;
        }
    }

    if (clip1 | clip2) {
        return;
    }

    POLY_add_line_2d(sx1, sy1, sx2, sy2, colour);
}

// uc_orig: POLY_frame_draw (fallen/DDEngine/Headers/poly.h)
// Flushes all polygon buckets to Direct3D for the current frame.
// draw_shadow_page/draw_text_page: UC_FALSE to suppress those pages.
void POLY_frame_draw(SLONG draw_shadow_page, SLONG draw_text_page)
{
    SLONG i;
    SLONG j;
    SLONG k;

    PolyPage* pa;

    static int iPageNumberToClear = 0;

    // Draw sky page first (always rendered at the back).
    pa = &POLY_Page[POLY_PAGE_SKY];

    if (pa->NeedsRendering()) {
        pa->RS.SetChanged();
        pa->Render(the_display.lp_D3D_Device);
    }

    if (PolyPage::AlphaSortEnabled()) {
        // Alpha-sort path: draw opaque pages first, then sort and draw alpha pages by depth.

        for (i = 0; i <= iPolyNumPagesRender; i++) {
            k = PageOrder[i];

            // Draw POLY_PAGE_COLOUR first (needed for correct order).
            if (i == 0) {
                k = POLY_PAGE_COLOUR;
            } else {
                k = i - 1;

                if (k == POLY_PAGE_COLOUR) {
                    continue;
                }
            }

            pa = &POLY_Page[k];

            if (!pa->NeedsRendering()) {
                continue;
            }

            if (k == POLY_PAGE_TEXT && !draw_text_page) {
                continue;
            }

            if (!pa->RS.NeedsSorting() || (k == POLY_PAGE_PUDDLE)) {
                pa->RS.SetChanged();

                if (POLY_force_additive_alpha) {
                    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
                    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
                    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
                    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZBIAS, 2);
                }
                /*
                                                if(INDOORS_INDEX)
                                                {
                                                  //poo poo poo for fadeing current floor of building



                                                        SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAPBLEND,D3DTBLEND_MODULATEALPHA);
                                                        SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND,D3DBLEND_SRCALPHA);
                                                        SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND,D3DBLEND_INVSRCALPHA);
                                                        SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE,UC_TRUE);
                                                        SET_RENDER_STATE(D3DRENDERSTATE_FOGENABLE,UC_FALSE);
                                                }
                */

                pa->Render(the_display.lp_D3D_Device);
            }
        }

        // Alpha-sort phase: bucket polys by depth and render front-to-back.
        PolyPoly* buckets[2048];

        for (i = 0; i < 2048; i++)
            buckets[i] = NULL;

        for (i = 0; i < POLY_NUM_PAGES; i++) {
            pa = &POLY_Page[i];

            if (!pa->NeedsRendering())
                continue;
            if (i == POLY_PAGE_SHADOW && !draw_shadow_page)
                continue;
            if (!pa->RS.NeedsSorting())
                continue;
            if (i == POLY_PAGE_PUDDLE)
                continue;

            pa->AddToBuckets(buckets, 2048);
        }

        for (i = 0; i < 2048; i++) {
            PolyPoly* p = buckets[i];

            while (p) {
                p->page->RS.SetChanged();
                p->page->DrawSinglePoly(p, the_display.lp_D3D_Device);
                p = p->next;
            }
        }

        for (i = 0; i < POLY_NUM_PAGES; i++) {
            pa = &POLY_Page[i];

            if (pa->RS.NeedsSorting())
                pa->RS.ResetTempTransparent();
        }

    } else {
        // Non-sorted path: draw all pages in PageOrder sequence.
        for (i = 0; i < iPolyNumPagesRender; i++) {
            k = PageOrder[i];

            pa = &POLY_Page[k];

            if (!pa->NeedsRendering()) {
                continue;
            }

            pa->RS.SetChanged();

            pa->Render(the_display.lp_D3D_Device);
        }
    }

    END_SCENE;

    // Incrementally clear a few pages' VBs/IBs per frame to reclaim memory from inactive pages.
    for (i = 0; i < 3; i++) {
        iPageNumberToClear++;
        if (iPageNumberToClear >= POLY_NUM_PAGES) {
            iPageNumberToClear = 0;
        }
        POLY_Page[iPageNumberToClear].Clear();
    }

    /*

//
//	Guy Demo Dodge!!!
//

    if(GAME_STATE&GS_ATTRACT_MODE)
    {
extern	void	draw_text_at(float x,float y,CBYTE *message,SLONG font_id);
    extern BOOL  text_fudge;
    extern ULONG text_colour;

            text_fudge  = UC_FALSE;
            text_colour = 0x00ffffff;
            draw_text_at(200,150,"Press Anything To Play",0);

extern void	show_text(void);
            show_text();
    }

    */
}

// uc_orig: POLY_frame_draw_odd (fallen/DDEngine/Source/poly.cpp)
// Draws only the standard texture pages with MODULATEALPHA blend and no depth test.
// Used for the attract-mode 3D scene overlay.
void POLY_frame_draw_odd()
{
    SLONG i;
    SLONG j;

    PolyPage* pa;

    BEGIN_SCENE;

// Sets the actual hardware RS, and also keeps the cache informed.
#define FORCE_SET_RENDER_STATE(t, s)           \
    RenderState::s_State.SetRenderState(t, s); \
    REALLY_SET_RENDER_STATE(t, s)
#define FORCE_SET_TEXTURE(s)            \
    RenderState::s_State.SetTexture(s); \
    REALLY_SET_TEXTURE(s)

    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SPECULARENABLE, UC_TRUE);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_ZENABLE, UC_FALSE);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_ZWRITEENABLE, UC_FALSE);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREADDRESS, D3DTADDRESS_CLAMP);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
    FORCE_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);

    for (i = 0; i < TEXTURE_page_num_standard; i++) {
        ASSERT(WITHIN(i, 0, POLY_NUM_PAGES - 1));

        pa = &POLY_Page[i];

        if (!pa->NeedsRendering()) {
            continue;
        }

        FORCE_SET_TEXTURE(TEXTURE_get_handle(i));

        pa->Render(the_display.lp_D3D_Device);
    }

    if (POLY_Page[POLY_PAGE_SKY].NeedsRendering()) {
        FORCE_SET_RENDER_STATE(D3DRENDERSTATE_FOGENABLE, UC_FALSE);
        FORCE_SET_TEXTURE(TEXTURE_get_handle(TEXTURE_page_sky));

        POLY_Page[POLY_PAGE_SKY].Render(the_display.lp_D3D_Device);
    }

    if (POLY_Page[POLY_PAGE_MOON].NeedsRendering()) {
        FORCE_SET_RENDER_STATE(D3DRENDERSTATE_FOGENABLE, UC_FALSE);
        FORCE_SET_TEXTURE(TEXTURE_get_handle(TEXTURE_page_moon));

        POLY_Page[POLY_PAGE_MOON].Render(the_display.lp_D3D_Device);
    }

    END_SCENE;
}

// uc_orig: POLY_frame_draw_puddles (fallen/DDEngine/Source/poly.cpp)
// Draws only the puddle reflection texture page with its own render state.
void POLY_frame_draw_puddles()
{
    PolyPage* pp = &POLY_Page[POLY_PAGE_PUDDLE];
    PolyPage* ppDrawn = pp->pTheRealPolyPage;

    if (pp->NeedsRendering()) {
        BEGIN_SCENE;

        pp->RS.InitScene(0);

        pp->Render(the_display.lp_D3D_Device);

        END_SCENE;
    }
}

// uc_orig: POLY_get_sphere_circle (fallen/DDEngine/Headers/poly.h)
// Projects a world-space sphere to screen space, returning its approximate screen circle.
// Returns UC_FALSE if the sphere centre is behind the camera.
SLONG POLY_get_sphere_circle(
    float world_x,
    float world_y,
    float world_z,
    float world_radius,
    SLONG* screen_x,
    SLONG* screen_y,
    SLONG* screen_radius)
{
    float vw;
    float width;

    POLY_Point pp;

    POLY_transform(
        world_x,
        world_y,
        world_z,
        &pp);

    if (pp.IsValid()) {
        vw = world_radius * POLY_cam_over_view_dist;
        width = vw * pp.Z * POLY_screen_mul_x;

        *screen_x = SLONG(pp.X);
        *screen_y = SLONG(pp.Y);
        *screen_radius = SLONG(width);

        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: POLY_inside_quad (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the 2D screen point lies inside the parallelogram defined by quad[0..2].
// along_01 and along_02 receive the parametric coordinates along the two edge vectors.
SLONG POLY_inside_quad(
    float screen_x,
    float screen_y,
    POLY_Point* quad[3],
    float* along_01,
    float* along_02)
{
    float ax = quad[1]->X - quad[0]->X;
    float ay = quad[1]->Y - quad[0]->Y;

    float bx = quad[2]->X - quad[0]->X;
    float by = quad[2]->Y - quad[0]->Y;

    float dx = screen_x - quad[0]->X;
    float dy = screen_y - quad[0]->Y;

    float dproda = ax * dx + ay * dy;
    float dprodb = bx * dx + by * dy;

    float lena2 = ax * ax + ay * ay;
    float lenb2 = bx * bx + by * by;

    float alonga = dproda / lena2;
    float alongb = dprodb / lenb2;

    if (WITHIN(alonga, 0.0f, 1.0f) && WITHIN(alongb, 0.0f, 1.0f)) {
        *along_01 = alonga;
        *along_02 = alongb;

        return UC_TRUE;
    } else {
        *along_01 = alonga;
        *along_02 = alongb;

        return UC_FALSE;
    }
}

// uc_orig: POLY_transform (fallen/DDEngine/Headers/poly.h)
// Transforms a world-space point into view space and projects it.
// bUnused parameter is present for Intel compiler compatibility only.
void POLY_transform(
    float world_x,
    float world_y,
    float world_z,
    POLY_Point* pt,
    bool bUnused)
{
    pt->x = world_x - POLY_cam_x;
    pt->y = world_y - POLY_cam_y;
    pt->z = world_z - POLY_cam_z;

    MATRIX_MUL(
        POLY_cam_matrix,
        pt->x,
        pt->y,
        pt->z);

    POLY_perspective(pt);
}

// uc_orig: POLY_transform_using_local_rotation (fallen/DDEngine/Headers/poly.h)
// Transforms a local-space point through the combined local+camera matrix.
void POLY_transform_using_local_rotation(
    float local_x,
    float local_y,
    float local_z,
    POLY_Point* pt)
{
    pt->x = local_x;
    pt->y = local_y;
    pt->z = local_z;

    MATRIX_MUL(
        POLY_cam_matrix_comb,
        pt->x,
        pt->y,
        pt->z);

    pt->x += POLY_cam_off_x;
    pt->y += POLY_cam_off_y;
    pt->z += POLY_cam_off_z;

    POLY_perspective(pt);
}
