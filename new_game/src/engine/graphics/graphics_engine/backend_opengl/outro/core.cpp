// Outro graphics engine — OpenGL 4.1 backend.
// Minimal implementation: textures + draw via game backend's TL path.
// Dual-texture (OS_DRAW_TEX_MUL) not yet supported — needs dedicated outro shader.

#include "engine/graphics/graphics_engine/outro_graphics_engine.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "engine/graphics/ui_coords.h"
#include "engine/platform/uc_common.h"
#include <string.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Internal texture type
// ---------------------------------------------------------------------------

struct os_texture {
    GLuint      gl_id;
    char        name[260];
    uint8_t     format;
    uint8_t     inverted;
    uint8_t     has_alpha;
    uint16_t    size;
    os_texture* next;
};

static os_texture* s_texture_head = nullptr;

// Bound textures (stage 0 and 1).
static os_texture* s_bound[2] = { nullptr, nullptr };

// ---------------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------------

// Configure outro's two-level viewport for the current scene FBO size.
// Idempotent and cheap — re-runnable per frame, which we need because
// the user can resize the window at any time during the outro and
// ge_resize_display will update ScreenWidth/Height + ui_coords WITHOUT
// notifying the outro. Computing the viewport from live globals keeps
// outro draws aligned to the centred 4:3 framed region after a resize.
//
// Outro pipeline draws in pixel-space coords scaled by OS_screen_width /
// OS_screen_height; the TL vertex shader maps those through u_viewport
// into NDC. Two-level viewport: shader sees (0, 0, frame_w, frame_h) so
// coords 0..frame_w map to NDC -1..1 cleanly, while GL-side glViewport /
// glScissor target the centred framed-area rect so the NDC quad lands
// in the framed region instead of the corner.
static void apply_outro_viewport()
{
    ui_coords::recompute(ge_get_screen_width(), ge_get_screen_height());
    const int frame_w  = (int)(ui_coords::g_frame_w_px + 0.5f);
    const int frame_h  = (int)(ui_coords::g_frame_h_px + 0.5f);
    const int origin_x = (int)((ui_coords::g_screen_w_px - ui_coords::g_frame_w_px) * 0.5f + 0.5f);
    const int origin_y = (int)((ui_coords::g_screen_h_px - ui_coords::g_frame_h_px) * 0.5f + 0.5f);

    OS_screen_width  = (float)frame_w;
    OS_screen_height = (float)frame_h;

    // Shader-side viewport (used by tl_vert.glsl for pixel→NDC math).
    ge_set_viewport(0, 0, frame_w, frame_h);

    // GL-side viewport — override what ge_set_viewport just set so the
    // framed NDC quad actually lands in the centred screen rect.
    const int screen_h = ge_get_screen_height();
    const int screen_w = ge_get_screen_width();
    glViewport(origin_x, screen_h - origin_y - frame_h, frame_w, frame_h);

    // Open up scissor to the whole render target. Outro has scrolling /
    // motion-trail effects (e.g. the sliding bar transition) that draw
    // shapes overlapping the framed-area boundary; if scissor clipped
    // them to the framed rect, partial fragments at the edge couldn't
    // be cleared the next frame, leaving a thin static strip. Viewport
    // alone is enough to confine rasterization to the framed region.
    glScissor(0, 0, screen_w, screen_h);
}

void oge_init()
{
    apply_outro_viewport();
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

OGETexture oge_texture_create(const char* name, int32_t width, int32_t height,
                              uint32_t flags, const uint8_t* pixels, int32_t invert)
{
    // Dedup: return existing if name+invert match.
    for (os_texture* ot = s_texture_head; ot; ot = ot->next) {
        if (strcmp(name, ot->name) == 0 && ot->inverted == invert)
            return ot;
    }

    os_texture* ot = (os_texture*)calloc(1, sizeof(os_texture));
    if (!ot) return nullptr;

    strncpy(ot->name, name, sizeof(ot->name) - 1);
    ot->inverted = (uint8_t)invert;
    ot->has_alpha = (flags & OGE_TEX_HAS_ALPHA) ? 1 : 0;
    ot->size = (uint16_t)width;

    // Make a mutable copy for inversion.
    int32_t pixel_count = width * height;
    uint8_t* data = (uint8_t*)malloc(pixel_count * 4);
    if (!data) { free(ot); return nullptr; }
    memcpy(data, pixels, pixel_count * 4);

    if (invert) {
        for (int32_t i = 0; i < pixel_count * 4; i++)
            data[i] = 255 - data[i];
    }

    // Upload as BGRA → GL texture.
    glGenTextures(1, &ot->gl_id);
    glBindTexture(GL_TEXTURE_2D, ot->gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // CLAMP_TO_EDGE so bilinear sampling at UV=1.0 doesn't wrap to the
    // opposite-colour leftmost texel — that wrap was visible as a thin
    // vertical strip on the right edge of outro elements at higher
    // resolutions (more pronounced when stretched into a smaller
    // framed area on widescreen).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(data);

    ot->next = s_texture_head;
    s_texture_head = ot;

    return ot;
}

// ---------------------------------------------------------------------------
// Render state
// ---------------------------------------------------------------------------

void oge_change_renderstate(uint32_t flags)
{
    // Blend mode. D3D6 required explicit ALPHABLENDENABLE; in GL we must call
    // ge_set_blend_enabled(true) — setting factors alone does not enable blending.
    if (flags & OS_DRAW_ADD) {
        ge_set_blend_enabled(true);
        ge_set_blend_factors(GEBlendFactor::One, GEBlendFactor::One);
    } else if (flags & OS_DRAW_ALPHABLEND) {
        ge_set_blend_enabled(true);
        ge_set_blend_factors(GEBlendFactor::SrcAlpha, GEBlendFactor::InvSrcAlpha);
    } else if (flags & OS_DRAW_TRANSPARENT) {
        ge_set_blend_enabled(true);
        ge_set_blend_factors(GEBlendFactor::Zero, GEBlendFactor::One);
    } else if (flags & OS_DRAW_MULBYONE) {
        ge_set_blend_enabled(true);
        ge_set_blend_factors(GEBlendFactor::DstColor, GEBlendFactor::Zero);
    } else {
        ge_set_blend_enabled(false);
    }

    // Depth.
    if (flags & OS_DRAW_NOZWRITE) {
        ge_set_depth_mode(GEDepthMode::ReadOnly);
    }
    if (flags & OS_DRAW_ZALWAYS) {
        ge_set_depth_func(GECompareFunc::Always);
    } else if (flags & OS_DRAW_ZREVERSE) {
        ge_set_depth_func(GECompareFunc::GreaterEqual);
    }

    // Cull.
    if (flags & OS_DRAW_DOUBLESIDED) {
        ge_set_cull_mode(GECullMode::None);
    }

    // Texture address.
    if (flags & OS_DRAW_CLAMP) {
        ge_set_texture_address(GETextureAddress::Clamp);
    }

    // Texture blend mode.
    // D3D6 ALPHABLEND set ALPHAOP=MODULATE (tex.a × vtx.a) — use ModulateAlpha.
    // Default Modulate uses ALPHAOP=SELECTARG1 (tex.a only).
    if (flags & OS_DRAW_DECAL) {
        ge_set_texture_blend(GETextureBlend::Decal);
    } else if (flags & OS_DRAW_ALPHABLEND) {
        ge_set_texture_blend(GETextureBlend::ModulateAlpha);
    } else if (flags & OS_DRAW_TEX_NONE) {
        ge_set_texture_blend(GETextureBlend::Modulate);
    } else {
        ge_set_texture_blend(GETextureBlend::Modulate);
    }
}

void oge_undo_renderstate_changes()
{
    ge_set_blend_enabled(false);
    ge_set_depth_mode(GEDepthMode::ReadWrite);
    ge_set_depth_func(GECompareFunc::LessEqual);
    ge_set_cull_mode(GECullMode::CCW);
    ge_set_texture_address(GETextureAddress::Wrap);
    ge_set_texture_blend(GETextureBlend::Modulate);
}

// ---------------------------------------------------------------------------
// Texture binding
// ---------------------------------------------------------------------------

void oge_bind_texture(int32_t stage, OGETexture tex)
{
    if (stage < 0 || stage > 1) return;
    s_bound[stage] = tex;

    // Only bind stage 0 to game engine (stage 1 = dual-texture, not yet supported).
    if (stage == 0 && tex && tex->gl_id) {
        ge_bind_texture((GETextureHandle)(uintptr_t)tex->gl_id);
        // Outro textures are not in the game texture pool, so ge_bind_texture
        // can't find their alpha flag. Override it from our own metadata.
        ge_set_bound_texture_has_alpha(tex->has_alpha != 0);
    }
}

// ---------------------------------------------------------------------------
// Drawing — convert 40-byte outro vertex to 32-byte game vertex, use TL path
// ---------------------------------------------------------------------------

// Outro vertex layout (40 bytes):
//   float sx, sy, sz (12), float rhw (4), uint32 colour (4), uint32 specular (4),
//   float tu1, tv1 (8), float tu2, tv2 (8)
// Game vertex layout (32 bytes):
//   float x, y, z (12), float rhw (4), uint32 color (4), uint32 specular (4),
//   float u, v (8)

void oge_draw_indexed(const void* verts, int32_t num_verts,
                      const uint16_t* indices, int32_t num_indices)
{
    if (!verts || num_verts <= 0 || !indices || num_indices <= 0) return;

    // Convert outro vertices to game TL vertices (drop texcoord2).
    GEVertexTL* tl = (GEVertexTL*)malloc(num_verts * sizeof(GEVertexTL));
    if (!tl) return;

    const uint8_t* src = (const uint8_t*)verts;
    for (int32_t i = 0; i < num_verts; i++) {
        // Copy first 32 bytes (position, rhw, color, specular, texcoord0).
        memcpy(&tl[i], src, 32);
        src += 40; // skip texcoord1 (8 bytes)
    }

    ge_draw_indexed_primitive(GEPrimitiveType::TriangleList,
                              tl, num_verts, indices, num_indices);
    free(tl);
}

// ---------------------------------------------------------------------------
// Scene (delegates to game graphics engine)
// ---------------------------------------------------------------------------

void oge_clear_screen()
{
    ge_clear(true, true);
}

void oge_scene_begin()
{
    // Re-apply the outro viewport every frame. Cheap (a few ge_get_*
    // queries + glViewport / glScissor) and lets a live window resize
    // during the outro sequence stay correct: ge_resize_display updates
    // ScreenWidth/Height + ui_coords but does NOT notify the outro
    // pipeline, so without this call the viewport set in oge_init
    // would still be sized for the pre-resize FBO and the outro would
    // render into the wrong rect.
    apply_outro_viewport();

    ge_begin_scene();
    // Outro's main loop calls OS_clear_screen exactly once at startup; the
    // design relied on BACK_draw fully repainting the framebuffer every
    // frame. In our framed setup BACK_draw doesn't always cover the last
    // pixel of the framed region (rounding / motion-effect partial draws),
    // and stale fragments from the sliding-bar transition leave a thin
    // visible strip that never gets erased. Clear here every frame so
    // anything BACK_draw misses ends up black instead of stale.
    ge_clear(true, true);
}

void oge_scene_end()
{
    ge_end_scene();
}

void oge_show()
{
    ge_flip();
}
