// Outro graphics engine — OpenGL 4.1 backend.
// Minimal implementation: textures + draw via game backend's TL path.
// Dual-texture (OS_DRAW_TEX_MUL) not yet supported — needs dedicated outro shader.

#include "engine/graphics/graphics_engine/outro_graphics_engine.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
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
    uint16_t    size;
    os_texture* next;
};

static os_texture* s_texture_head = nullptr;

// Bound textures (stage 0 and 1).
static os_texture* s_bound[2] = { nullptr, nullptr };

// ---------------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------------

void oge_init()
{
    OS_screen_width = (float)ge_get_screen_width();
    OS_screen_height = (float)ge_get_screen_height();
}

void oge_shutdown()
{
    os_texture* ot = s_texture_head;
    while (ot) {
        os_texture* next = ot->next;
        if (ot->gl_id) glDeleteTextures(1, &ot->gl_id);
        free(ot);
        ot = next;
    }
    s_texture_head = nullptr;
    s_bound[0] = s_bound[1] = nullptr;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(data);

    ot->next = s_texture_head;
    s_texture_head = ot;

    return ot;
}

OGETexture oge_texture_create_blank(int32_t size, int32_t format)
{
    os_texture* ot = (os_texture*)calloc(1, sizeof(os_texture));
    if (!ot) return nullptr;

    ot->size = (uint16_t)size;
    ot->format = (uint8_t)format;

    glGenTextures(1, &ot->gl_id);
    glBindTexture(GL_TEXTURE_2D, ot->gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    ot->next = s_texture_head;
    s_texture_head = ot;

    return ot;
}

void oge_texture_finished_creating() {}

int32_t oge_texture_size(OGETexture tex)
{
    return tex ? tex->size : 0;
}

void oge_texture_lock(OGETexture) {}
void oge_texture_unlock(OGETexture) {}

// ---------------------------------------------------------------------------
// Render state
// ---------------------------------------------------------------------------

void oge_init_renderstates() {}
void oge_calculate_pipeline() {}

void oge_change_renderstate(uint32_t flags)
{
    // Blend mode.
    if (flags & OS_DRAW_ADD) {
        ge_set_blend_factors(GEBlendFactor::One, GEBlendFactor::One);
    } else if (flags & OS_DRAW_ALPHABLEND) {
        ge_set_blend_factors(GEBlendFactor::SrcAlpha, GEBlendFactor::InvSrcAlpha);
    } else if (flags & OS_DRAW_TRANSPARENT) {
        ge_set_blend_factors(GEBlendFactor::Zero, GEBlendFactor::One);
    } else if (flags & OS_DRAW_MULBYONE) {
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
    if (flags & OS_DRAW_DECAL) {
        ge_set_texture_blend(GETextureBlend::Decal);
    } else if (flags & OS_DRAW_TEX_NONE) {
        ge_set_texture_blend(GETextureBlend::Modulate); // vertex color only, no texture bound
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
    ge_begin_scene();
}

void oge_scene_end()
{
    ge_end_scene();
}

void oge_show()
{
    ge_flip();
}
