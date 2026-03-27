// Graphics engine — Direct3D 6 implementation.
// This file is the ONLY place where D3D/DDraw headers should be included
// (besides the internal graphics_api/ files that this implementation wraps).

#include "engine/graphics/graphics_engine/graphics_engine.h"
#include "engine/graphics/graphics_api/display_macros.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ge_init()
{
    // Display is already initialized by OpenDisplay() before ge_init() is called.
    // Nothing extra needed for D3D path.
}

void ge_shutdown()
{
    // Display cleanup is handled by CloseDisplay().
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void ge_begin_scene()
{
    BEGIN_SCENE;
}

void ge_end_scene()
{
    END_SCENE;
}

void ge_clear(bool color, bool depth)
{
    DWORD flags = 0;
    if (color) flags |= D3DCLEAR_TARGET;
    if (depth) flags |= D3DCLEAR_ZBUFFER;
    if (flags) {
        the_display.lp_D3D_Viewport->Clear(1, &the_display.ViewportRect, flags);
    }
}

void ge_flip()
{
    FLIP(NULL, DDFLIP_WAIT);
}

// ---------------------------------------------------------------------------
// Render state
// ---------------------------------------------------------------------------

void ge_set_blend_mode(GEBlendMode mode)
{
    switch (mode) {
    case GEBlendMode::Opaque:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
        break;
    case GEBlendMode::Alpha:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        break;
    case GEBlendMode::Additive:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
        break;
    case GEBlendMode::Modulate:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
        break;
    case GEBlendMode::InvModulate:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCCOLOR);
        break;
    }
}

void ge_set_depth_mode(GEDepthMode mode)
{
    switch (mode) {
    case GEDepthMode::Off:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZENABLE, FALSE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
        break;
    case GEDepthMode::ReadOnly:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZENABLE, TRUE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
        break;
    case GEDepthMode::WriteOnly:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZENABLE, FALSE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
        break;
    case GEDepthMode::ReadWrite:
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZENABLE, TRUE);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
        break;
    }
}

void ge_set_depth_func(GECompareFunc func)
{
    D3DCMPFUNC d3d_func;
    switch (func) {
    case GECompareFunc::Always:       d3d_func = D3DCMP_ALWAYS; break;
    case GECompareFunc::Less:         d3d_func = D3DCMP_LESS; break;
    case GECompareFunc::LessEqual:    d3d_func = D3DCMP_LESSEQUAL; break;
    case GECompareFunc::Greater:      d3d_func = D3DCMP_GREATER; break;
    case GECompareFunc::GreaterEqual: d3d_func = D3DCMP_GREATEREQUAL; break;
    case GECompareFunc::NotEqual:     d3d_func = D3DCMP_NOTEQUAL; break;
    default:                          d3d_func = D3DCMP_LESSEQUAL; break;
    }
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZFUNC, d3d_func);
}

void ge_set_cull_mode(GECullMode mode)
{
    D3DCULL d3d_cull;
    switch (mode) {
    case GECullMode::None: d3d_cull = D3DCULL_NONE; break;
    case GECullMode::CW:   d3d_cull = D3DCULL_CW; break;
    case GECullMode::CCW:  d3d_cull = D3DCULL_CCW; break;
    default:               d3d_cull = D3DCULL_CCW; break;
    }
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_CULLMODE, d3d_cull);
}

void ge_set_texture_filter(GETextureFilter mag, GETextureFilter min)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAG,
        mag == GETextureFilter::Nearest ? D3DFILTER_NEAREST : D3DFILTER_LINEAR);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMIN,
        min == GETextureFilter::Nearest ? D3DFILTER_NEAREST : D3DFILTER_LINEAR);
}

void ge_set_texture_blend(GETextureBlend mode)
{
    D3DTEXTUREBLEND d3d_blend;
    switch (mode) {
    case GETextureBlend::Modulate:      d3d_blend = D3DTBLEND_MODULATE; break;
    case GETextureBlend::ModulateAlpha: d3d_blend = D3DTBLEND_MODULATEALPHA; break;
    case GETextureBlend::Decal:         d3d_blend = D3DTBLEND_DECAL; break;
    default:                            d3d_blend = D3DTBLEND_MODULATE; break;
    }
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAPBLEND, d3d_blend);
}

void ge_set_texture_address(GETextureAddress mode)
{
    REALLY_SET_TEXTURE_STATE(0, D3DTSS_ADDRESS,
        mode == GETextureAddress::Wrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);
}

void ge_set_fog_enabled(bool enabled)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGENABLE, enabled ? TRUE : FALSE);
}

void ge_set_specular_enabled(bool enabled)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SPECULARENABLE, enabled ? TRUE : FALSE);
}

void ge_set_perspective_correction(bool enabled)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREPERSPECTIVE, enabled ? TRUE : FALSE);
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

void ge_bind_texture(GETextureHandle tex)
{
    // For now, TextureHandle is a cast of LPDIRECT3DTEXTURE2.
    // This will be properly abstracted when texture management is migrated.
    REALLY_SET_TEXTURE(reinterpret_cast<LPDIRECT3DTEXTURE2>(static_cast<uintptr_t>(tex)));
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

// D3D6 draw flags used by the original code.
static constexpr DWORD GE_D3D_DRAW_FLAGS = D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT;

static_assert(sizeof(GEVertexTL) == sizeof(D3DTLVERTEX), "GEVertexTL must match D3DTLVERTEX layout");
static_assert(sizeof(GEVertexLit) == sizeof(D3DLVERTEX), "GEVertexLit must match D3DLVERTEX layout");
static_assert(sizeof(GEVertex) == sizeof(D3DVERTEX), "GEVertex must match D3DVERTEX layout");

void ge_draw_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t count)
{
    D3DPRIMITIVETYPE d3d_type = (type == GEPrimitiveType::TriangleList) ? D3DPT_TRIANGLELIST : D3DPT_TRIANGLEFAN;
    DRAW_PRIMITIVE(d3d_type, D3DFVF_TLVERTEX, (void*)verts, count, GE_D3D_DRAW_FLAGS);
}

void ge_draw_indexed_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t vert_count,
                               const uint16_t* indices, uint32_t index_count)
{
    D3DPRIMITIVETYPE d3d_type = (type == GEPrimitiveType::TriangleList) ? D3DPT_TRIANGLELIST : D3DPT_TRIANGLEFAN;
    DRAW_INDEXED_PRIMITIVE(d3d_type, D3DFVF_TLVERTEX, (void*)verts, vert_count,
                           (LPWORD)indices, index_count, GE_D3D_DRAW_FLAGS);
}

void ge_draw_indexed_primitive_lit(GEPrimitiveType type, const GEVertexLit* verts, uint32_t vert_count,
                                   const uint16_t* indices, uint32_t index_count)
{
    D3DPRIMITIVETYPE d3d_type = (type == GEPrimitiveType::TriangleList) ? D3DPT_TRIANGLELIST : D3DPT_TRIANGLEFAN;
    DRAW_INDEXED_PRIMITIVE(d3d_type, D3DFVF_LVERTEX, (void*)verts, vert_count,
                           (LPWORD)indices, index_count, GE_D3D_DRAW_FLAGS);
}

void ge_draw_indexed_primitive_unlit(GEPrimitiveType type, const GEVertex* verts, uint32_t vert_count,
                                     const uint16_t* indices, uint32_t index_count)
{
    D3DPRIMITIVETYPE d3d_type = (type == GEPrimitiveType::TriangleList) ? D3DPT_TRIANGLELIST : D3DPT_TRIANGLEFAN;
    DRAW_INDEXED_PRIMITIVE(d3d_type, D3DFVF_VERTEX, (void*)verts, vert_count,
                           (LPWORD)indices, index_count, GE_D3D_DRAW_FLAGS);
}

// ---------------------------------------------------------------------------
// Viewport
// ---------------------------------------------------------------------------

void ge_set_viewport(int32_t x, int32_t y, int32_t w, int32_t h)
{
    D3DVIEWPORT2 vp = {};
    vp.dwSize = sizeof(D3DVIEWPORT2);
    vp.dwX = x;
    vp.dwY = y;
    vp.dwWidth = w;
    vp.dwHeight = h;
    vp.dvClipX = -1.0f;
    vp.dvClipY = static_cast<float>(h) / static_cast<float>(w);
    vp.dvClipWidth = 2.0f;
    vp.dvClipHeight = 2.0f * static_cast<float>(h) / static_cast<float>(w);
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    the_display.lp_D3D_Viewport->SetViewport2(&vp);

    // Update ViewportRect for Clear() calls.
    the_display.ViewportRect.x1 = x;
    the_display.ViewportRect.y1 = y;
    the_display.ViewportRect.x2 = x + w;
    the_display.ViewportRect.y2 = y + h;
}

// ---------------------------------------------------------------------------
// Background
// ---------------------------------------------------------------------------

void ge_set_background(GEBackground bg)
{
    switch (bg) {
    case GEBackground::Black: SET_BLACK_BACKGROUND; break;
    case GEBackground::White: SET_WHITE_BACKGROUND; break;
    case GEBackground::User:  SET_USER_BACKGROUND; break;
    }
}

void ge_set_background_color(uint8_t r, uint8_t g, uint8_t b)
{
    the_display.SetUserColour(r, g, b);
}
