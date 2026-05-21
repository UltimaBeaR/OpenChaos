#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "engine/graphics/postprocess/wibble.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"

#include <cmath>

// Display globals (defined in d3d/display_globals.cpp).
extern SLONG ScreenWidth;
extern SLONG ScreenHeight;

namespace {

// 3×3 inverse of MATRIX_MUL's effective operator on POLY_cam_matrix
// (m[r*3+c] is row r, col c). POLY_cam_matrix is poorly conditioned —
// MATRIX_skew bakes 1/view_dist × aspect × lens into row scales, so
// entries are ~1e-4 and det ~1e-12. Computed in double precision; the
// final float cast keeps the inverse stable per-pixel for the shader's
// ray-cast.
bool mat3_invert_operator(const float* m, float* out)
{
    const double a = m[0], b = m[1], c = m[2];
    const double d = m[3], e = m[4], f = m[5];
    const double g = m[6], h = m[7], i = m[8];

    const double A =  (e * i - f * h);
    const double B = -(d * i - f * g);
    const double C =  (d * h - e * g);
    const double D = -(b * i - c * h);
    const double E =  (a * i - c * g);
    const double F = -(a * h - b * g);
    const double G =  (b * f - c * e);
    const double H = -(a * f - c * d);
    const double I =  (a * e - b * d);

    const double det = a * A + b * B + c * C;
    if (std::fabs(det) < 1e-20) return false;
    const double inv_det = 1.0 / det;

    out[0] = float(A * inv_det); out[1] = float(D * inv_det); out[2] = float(G * inv_det);
    out[3] = float(B * inv_det); out[4] = float(E * inv_det); out[5] = float(H * inv_det);
    out[6] = float(C * inv_det); out[7] = float(F * inv_det); out[8] = float(I * inv_det);
    return true;
}

} // namespace

// uc_orig: WIBBLE_simple (fallen/DDEngine/Source/wibble.cpp)
// Original CPU implementation per-row shifted backbuffer pixels via the
// locked screen surface — that's the glReadPixels + writeback hang
// pattern we eliminated in Stage 3. Now a thin wrapper: scale rect
// from game-virtual (640×480) to framebuffer pixels, populate POLY-
// pipeline state needed for the shader's world-XZ ray-cast (issue #67
// fix), then delegate to the GPU pass in the backend.
void WIBBLE_simple(
    SLONG x1, SLONG y1,
    SLONG x2, SLONG y2,
    UBYTE wibble_y1,
    UBYTE wibble_y2,
    UBYTE wibble_g1,
    UBYTE wibble_g2,
    UBYTE wibble_s1,
    UBYTE wibble_s2)
{
    // Uniform scale by vertical on both axes — matches the s_YScale used
    // to render the world (see PolyPage::SetScaling in game_mode_changed).
    // Scaling X by ScreenWidth/DisplayWidth works only when the monitor
    // aspect is 4:3; on widescreen the horizontal ratio diverges from
    // the vertical one and the wibble bbox drifts off the puddle.
    x1 = x1 * ScreenHeight / DisplayHeight;
    x2 = x2 * ScreenHeight / DisplayHeight;
    y1 = y1 * ScreenHeight / DisplayHeight;
    y2 = y2 * ScreenHeight / DisplayHeight;

    GEWibbleParams p {};
    p.wibble_y1 = wibble_y1;
    p.wibble_y2 = wibble_y2;
    p.wibble_g1 = wibble_g1;
    p.wibble_g2 = wibble_g2;
    p.wibble_s1 = wibble_s1;
    p.wibble_s2 = wibble_s2;
    p.game_turn = VISUAL_TURN;

    // POLY-aware ray-cast support (issue #67 fix). Inverse of MATRIX_MUL
    // operator on POLY_cam_matrix in double precision; camera position
    // and perspective constants follow. Used by the fragment shader to
    // map each pixel to its world-XZ point on the ground plane (Y=0
    // anchor — see shader header for why a fixed plane, not per-puddle
    // water height).
    if (!mat3_invert_operator(POLY_cam_matrix, p.inv_poly_cam)) {
        // Degenerate cam matrix — should never happen mid-frame. Skip.
        return;
    }
    p.poly_cam_pos[0] = POLY_cam_x;
    p.poly_cam_pos[1] = POLY_cam_y;
    p.poly_cam_pos[2] = POLY_cam_z;
    // POLY_screen_mid_*/mul_* live in the game's virtual 480-tall canvas;
    // gl_FragCoord in the shader is in framebuffer pixels. Pre-scale here
    // so the shader's pixel→view inverse math operates in one scale.
    const float fbo_scale = float(ScreenHeight) / float(DisplayHeight);
    p.poly_screen_mid[0] = POLY_screen_mid_x * fbo_scale;
    p.poly_screen_mid[1] = POLY_screen_mid_y * fbo_scale;
    p.poly_screen_mul[0] = POLY_screen_mul_x * fbo_scale;
    p.poly_screen_mul[1] = POLY_screen_mul_y * fbo_scale;
    p.poly_zclip         = POLY_ZCLIP_PLANE;
    // Viewport for gl_FragCoord.y bottom-up → POLY top-down flip in shader.
    p.viewport[0] = 0.0f;
    p.viewport[1] = 0.0f;
    p.viewport[2] = float(ScreenWidth);
    p.viewport[3] = float(ScreenHeight);

    ge_apply_wibble(x1, y1, x2, y2, p);
}
