// Graphics engine — Direct3D 6 implementation.
// All D3D/DDraw code lives in this d3d/ folder (adapter + internal D3D modules).

#include "engine/graphics/graphics_engine/graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_directx6/display_macros.h"
#include "engine/graphics/graphics_engine/backend_directx6/d3d_texture.h"
#include "engine/graphics/graphics_engine/backend_directx6/display_globals.h"
#include "engine/graphics/graphics_engine/backend_directx6/vertex_buffer.h"
#include "engine/graphics/graphics_engine/backend_directx6/d3d_texture_globals.h"
#include "engine/graphics/graphics_engine/backend_directx6/vertex_buffer.h"
#include "engine/graphics/graphics_engine/backend_directx6/vertex_buffer_globals.h"
#include "engine/graphics/graphics_engine/backend_directx6/dd_manager_globals.h"
#include "engine/io/file.h"
#include "engine/core/memory.h"
#include "assets/formats/level_loader.h" // DATA_DIR

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ge_init()
{
    // Display is already initialized by OpenDisplay() before ge_init() is called.
    // NOTE: Do NOT set TSS (D3DTSS_COLOROP etc.) here — in D3D6, explicit TSS disables
    // the legacy D3DRENDERSTATE_TEXTUREMAPBLEND that the entire POLY pipeline uses.
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

static DWORD ge_blend_factor_to_d3d(GEBlendFactor f)
{
    switch (f) {
    case GEBlendFactor::Zero:         return D3DBLEND_ZERO;
    case GEBlendFactor::One:          return D3DBLEND_ONE;
    case GEBlendFactor::SrcAlpha:     return D3DBLEND_SRCALPHA;
    case GEBlendFactor::InvSrcAlpha:  return D3DBLEND_INVSRCALPHA;
    case GEBlendFactor::SrcColor:     return D3DBLEND_SRCCOLOR;
    case GEBlendFactor::InvSrcColor:  return D3DBLEND_INVSRCCOLOR;
    case GEBlendFactor::DstColor:     return D3DBLEND_DESTCOLOR;
    case GEBlendFactor::InvDstColor:  return D3DBLEND_INVDESTCOLOR;
    case GEBlendFactor::DstAlpha:     return D3DBLEND_DESTALPHA;
    case GEBlendFactor::InvDstAlpha:  return D3DBLEND_INVDESTALPHA;
    default:                          return D3DBLEND_ONE;
    }
}

void ge_set_blend_factors(GEBlendFactor src, GEBlendFactor dst)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, ge_blend_factor_to_d3d(src));
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, ge_blend_factor_to_d3d(dst));
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
}

void ge_set_blend_enabled(bool enabled)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, enabled ? TRUE : FALSE);
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
    // Uses legacy D3DRENDERSTATE_TEXTUREMAPBLEND — same as original code
    // (RenderState::InitScene / RenderState::SetChanged).
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

void ge_set_depth_bias(int32_t bias)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZBIAS, bias);
}

void ge_set_fog_enabled(bool enabled)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGENABLE, enabled ? TRUE : FALSE);
}

void ge_set_color_key_enabled(bool enabled)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_COLORKEYENABLE, enabled ? TRUE : FALSE);
}

void ge_set_alpha_test_enabled(bool enabled)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHATESTENABLE, enabled ? TRUE : FALSE);
}

void ge_set_alpha_ref(uint32_t ref)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHAREF, ref);
}

void ge_set_alpha_func(GECompareFunc func)
{
    DWORD d3d_func;
    switch (func) {
    case GECompareFunc::Always:       d3d_func = D3DCMP_ALWAYS; break;
    case GECompareFunc::Less:         d3d_func = D3DCMP_LESS; break;
    case GECompareFunc::LessEqual:    d3d_func = D3DCMP_LESSEQUAL; break;
    case GECompareFunc::Greater:      d3d_func = D3DCMP_GREATER; break;
    case GECompareFunc::GreaterEqual: d3d_func = D3DCMP_GREATEREQUAL; break;
    case GECompareFunc::NotEqual:     d3d_func = D3DCMP_NOTEQUAL; break;
    default:                          d3d_func = D3DCMP_ALWAYS; break;
    }
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHAFUNC, d3d_func);
}

static inline DWORD FloatAsDword(float f) { DWORD d; memcpy(&d, &f, sizeof(d)); return d; }

void ge_set_fog_params(uint32_t color, float near_dist, float far_dist)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGCOLOR, color);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLESTART, FloatAsDword(near_dist));
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEEND, FloatAsDword(far_dist));
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEDENSITY, FloatAsDword(0.5f));

    // Alpha test setup (used globally in the original).
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHAREF, 0x07);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER);
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
// Transforms
// ---------------------------------------------------------------------------

static_assert(sizeof(GEMatrix) == sizeof(D3DMATRIX), "GEMatrix must match D3DMATRIX layout");

void ge_set_transform(GETransform type, const GEMatrix* matrix)
{
    D3DTRANSFORMSTATETYPE d3d_type;
    switch (type) {
    case GETransform::World:      d3d_type = D3DTRANSFORMSTATE_WORLD; break;
    case GETransform::View:       d3d_type = D3DTRANSFORMSTATE_VIEW; break;
    case GETransform::Projection: d3d_type = D3DTRANSFORMSTATE_PROJECTION; break;
    default:                      d3d_type = D3DTRANSFORMSTATE_WORLD; break;
    }
    the_display.lp_D3D_Device->SetTransform(d3d_type, const_cast<D3DMATRIX*>(reinterpret_cast<const D3DMATRIX*>(matrix)));
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
    // Use screen-space clip values (matches original D3D6 code).
    vp.dvClipX = static_cast<float>(x);
    vp.dvClipY = static_cast<float>(y);
    vp.dvClipWidth = static_cast<float>(w);
    vp.dvClipHeight = static_cast<float>(h);
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    the_display.lp_D3D_Viewport->SetViewport2(&vp);

    // Update ViewportRect for Clear() calls.
    the_display.ViewportRect.x1 = x;
    the_display.ViewportRect.y1 = y;
    the_display.ViewportRect.x2 = x + w;
    the_display.ViewportRect.y2 = y + h;
}

void ge_set_viewport_3d(int32_t x, int32_t y, int32_t w, int32_t h,
                        float clip_x, float clip_y, float clip_w, float clip_h)
{
    D3DVIEWPORT2 vp = {};
    vp.dwSize = sizeof(D3DVIEWPORT2);
    vp.dwX = x;
    vp.dwY = y;
    vp.dwWidth = w;
    vp.dwHeight = h;
    vp.dvClipX = clip_x;
    vp.dvClipY = clip_y;
    vp.dvClipWidth = clip_w;
    vp.dvClipHeight = clip_h;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    the_display.lp_D3D_Viewport->SetViewport2(&vp);

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

// ---------------------------------------------------------------------------
// Screen buffer access
// ---------------------------------------------------------------------------

void* ge_lock_screen()
{
    return the_display.screen_lock();
}

void ge_unlock_screen()
{
    the_display.screen_unlock();
}

uint8_t* ge_get_screen_buffer()
{
    return the_display.screen;
}

int32_t ge_get_screen_pitch()
{
    return the_display.screen_pitch;
}

int32_t ge_get_screen_width()
{
    return the_display.screen_width;
}

int32_t ge_get_screen_height()
{
    return the_display.screen_height;
}

int32_t ge_get_screen_bpp()
{
    return the_display.screen_bbp;
}

void ge_plot_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    the_display.PlotPixel(x, y, r, g, b);
}

void ge_plot_formatted_pixel(int32_t x, int32_t y, uint32_t color)
{
    the_display.PlotFormattedPixel(x, y, color);
}

void ge_get_pixel(int32_t x, int32_t y, uint8_t* r, uint8_t* g, uint8_t* b)
{
    the_display.GetPixel(x, y, r, g, b);
}

uint32_t ge_get_formatted_pixel(uint8_t r, uint8_t g, uint8_t b)
{
    return the_display.GetFormattedPixel(r, g, b);
}

void ge_blit_back_buffer()
{
    the_display.blit_back_buffer();
}

// ---------------------------------------------------------------------------
// Device capabilities
// ---------------------------------------------------------------------------

bool ge_supports_dest_inv_src_color()
{
    return the_display.GetDeviceInfo()->DestInvSourceColourSupported();
}

bool ge_supports_modulate_alpha()
{
    return the_display.GetDeviceInfo()->ModulateAlphaSupported();
}

bool ge_supports_adami_lighting()
{
    return the_display.GetDeviceInfo()->AdamiLightingSupported();
}

bool ge_is_hardware()
{
    return the_display.CurrDevice->IsHardware();
}

bool ge_is_fullscreen()
{
    return the_display.IsFullScreen();
}

// ---------------------------------------------------------------------------
// Background surface
// ---------------------------------------------------------------------------

void ge_create_background_surface(uint8_t* pixels)
{
    the_display.create_background_surface(pixels);
}

void ge_blit_background_surface()
{
    the_display.blit_background_surface();
}

void ge_destroy_background_surface()
{
    the_display.destroy_background_surface();
}

void ge_init_back_image(const char* name)
{
    InitBackImage((CBYTE*)name);
}

void ge_show_back_image(bool in_3d_frame)
{
    ShowBackImage(in_3d_frame);
}

void ge_reset_back_image()
{
    ResetBackImage();
}

// ---------------------------------------------------------------------------
// Cutscene
// ---------------------------------------------------------------------------

void ge_run_cutscene(int32_t id)
{
    the_display.RunCutscene(id);
}

// ---------------------------------------------------------------------------
// Driver info
// ---------------------------------------------------------------------------

bool ge_is_primary_driver()
{
    return the_display.GetDriverInfo()->IsPrimary();
}

// ---------------------------------------------------------------------------
// Gamma
// ---------------------------------------------------------------------------

void ge_set_gamma(int32_t black, int32_t white)
{
    the_display.SetGamma(black, white);
}

void ge_get_gamma(int32_t* black, int32_t* white)
{
    int b, w;
    the_display.GetGamma(&b, &w);
    *black = b;
    *white = w;
}

bool ge_is_gamma_available()
{
    return the_display.IsGammaAvailable();
}

// ---------------------------------------------------------------------------
// Vertex buffer pool
// ---------------------------------------------------------------------------

void ge_reclaim_vertex_buffers()
{
    extern VertexBufferPool* TheVPool;
    if (TheVPool) TheVPool->ReclaimBuffers();
}

void ge_dump_vpool_info(void* fd)
{
    extern VertexBufferPool* TheVPool;
    if (TheVPool) TheVPool->DumpInfo((FILE*)fd);
}

// ---------------------------------------------------------------------------
// Texture pixel access
// ---------------------------------------------------------------------------

bool ge_lock_texture_pixels(int32_t page, uint16_t** bitmap, int32_t* pitch)
{
    SLONG p;
    HRESULT res = TEXTURE_texture[page].LockUser((UWORD**)bitmap, &p);
    *pitch = p;
    return SUCCEEDED(res);
}

void ge_unlock_texture_pixels(int32_t page)
{
    TEXTURE_texture[page].UnlockUser();
}

void ge_get_texture_pixel_format(int32_t page,
    int32_t* mask_r, int32_t* mask_g, int32_t* mask_b, int32_t* mask_a,
    int32_t* shift_r, int32_t* shift_g, int32_t* shift_b, int32_t* shift_a)
{
    D3DTexture& t = TEXTURE_texture[page];
    *mask_r  = t.mask_red;   *mask_g  = t.mask_green;
    *mask_b  = t.mask_blue;  *mask_a  = t.mask_alpha;
    *shift_r = t.shift_red;  *shift_g = t.shift_green;
    *shift_b = t.shift_blue; *shift_a = t.shift_alpha;
}

bool ge_is_texture_loaded(int32_t page)
{
    return TEXTURE_texture[page].Type != GE_TEXTURE_TYPE_UNUSED;
}

void ge_debug_paint_block(uint32_t color)
{
    DDSURFACEDESC2 mine;
    InitStruct(mine);
    mine.dwFlags = DDSD_PITCH;
    HRESULT res = the_display.lp_DD_FrontSurface->Lock(NULL, &mine, DDLOCK_WAIT, NULL);
    if (FAILED(res)) return;
    char* dest = (char*)mine.lpSurface;
    for (int i = 0; i < 50; i++) {
        DWORD* dest1 = (DWORD*)dest;
        dest += mine.lPitch;
        for (int j = 0; j < 50; j++) *dest1++ = color;
    }
    the_display.lp_DD_FrontSurface->Unlock(NULL);
}

// ---------------------------------------------------------------------------
// Display mode management
// ---------------------------------------------------------------------------

void ge_to_gdi()
{
    the_display.toGDI();
}

void ge_from_gdi()
{
    the_display.fromGDI();
}

void ge_restore_all_surfaces()
{
    if (the_display.lp_DD4) {
        the_display.lp_DD4->RestoreAllSurfaces();
    }
}

bool ge_is_display_changed()
{
    return the_display.IsDisplayChanged();
}

void ge_clear_display_changed()
{
    the_display.DisplayChangedOff();
}

void ge_update_display_rect(void* hwnd, bool fullscreen)
{
    HWND hWnd = (HWND)hwnd;
    if (fullscreen) {
        SetRect(&the_display.DisplayRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    } else {
        GetClientRect(hWnd, &the_display.DisplayRect);
        ClientToScreen(hWnd, (LPPOINT)&the_display.DisplayRect);
        ClientToScreen(hWnd, (LPPOINT)&the_display.DisplayRect + 1);
    }
}

// ---------------------------------------------------------------------------
// Texture management
// ---------------------------------------------------------------------------

void ge_remove_all_loaded_textures()
{
    the_display.RemoveAllLoadedTextures();
}

// ---------------------------------------------------------------------------
// Surface blitting
// ---------------------------------------------------------------------------

void ge_blit_texture_to_backbuffer(int32_t texture_page, int32_t src_w, int32_t src_h)
{
    RECT rcSource;
    rcSource.left   = 0;
    rcSource.top    = 0;
    rcSource.right  = src_w;
    rcSource.bottom = src_h;
    TEXTURE_texture[texture_page].GetSurface()->Blt(NULL, the_display.lp_DD_BackSurface, &rcSource, DDBLT_WAIT, NULL);
}

// ---------------------------------------------------------------------------
// Screen surfaces (menu backgrounds)
// ---------------------------------------------------------------------------

GEScreenSurface ge_create_screen_surface(uint8_t* image_data)
{
    DDSURFACEDESC2 back;
    DDSURFACEDESC2 mine;
    LPDIRECTDRAWSURFACE4 surface = NULL;

    InitStruct(back);
    the_display.lp_DD_BackSurface->GetSurfaceDesc(&back);

    InitStruct(mine);
    mine.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    mine.dwWidth = back.dwWidth;
    mine.dwHeight = back.dwHeight;
    mine.ddpfPixelFormat = back.ddpfPixelFormat;
    mine.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    HRESULT result = the_display.lp_DD4->CreateSurface(&mine, &surface, NULL);
    if (FAILED(result)) {
        return GE_SCREEN_SURFACE_NONE;
    }

    CopyBackground(image_data, surface);
    return reinterpret_cast<GEScreenSurface>(surface);
}

GEScreenSurface ge_load_screen_surface(const char* filename)
{
    // Delegate to the file-loading logic (reads raw 640x480 BGR, bottom-up).
    CBYTE fname[200];
    sprintf(fname, "%sdata\\%s", DATA_DIR, filename);

    UBYTE* image_data = (UBYTE*)MemAlloc(640 * 480 * 3);
    if (!image_data) return GE_SCREEN_SURFACE_NONE;

    MFFileHandle image_file = FileOpen(fname);
    if (image_file != nullptr) {
        FileSeek(image_file, SEEK_MODE_BEGINNING, 18);
        UBYTE* image = image_data + (640 * 479 * 3);
        for (SLONG height = 480; height; height--, image -= (640 * 3)) {
            FileRead(image_file, image, 640 * 3);
        }
        FileClose(image_file);
    }

    GEScreenSurface surface = ge_create_screen_surface(image_data);
    MemFree(image_data);
    return surface;
}

void ge_destroy_screen_surface(GEScreenSurface surface)
{
    if (surface != GE_SCREEN_SURFACE_NONE) {
        reinterpret_cast<LPDIRECTDRAWSURFACE4>(surface)->Release();
    }
}

void ge_restore_screen_surface(GEScreenSurface surface)
{
    if (surface != GE_SCREEN_SURFACE_NONE) {
        reinterpret_cast<LPDIRECTDRAWSURFACE4>(surface)->Restore();
    }
}

void ge_set_background_override(GEScreenSurface surface)
{
    the_display.lp_DD_Background_use_instead = reinterpret_cast<LPDIRECTDRAWSURFACE4>(surface);
}

GEScreenSurface ge_get_background_override()
{
    return reinterpret_cast<GEScreenSurface>(the_display.lp_DD_Background_use_instead);
}

void ge_blit_surface_to_backbuffer(GEScreenSurface surface, int32_t x, int32_t y, int32_t w, int32_t h)
{
    RECT rc;
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + w;
    rc.bottom = y + h;
    the_display.lp_DD_BackSurface->Blt(&rc, reinterpret_cast<LPDIRECTDRAWSURFACE4>(surface), &rc, DDBLT_WAIT, 0);
}

// ---------------------------------------------------------------------------
// Texture loading lifecycle
// ---------------------------------------------------------------------------

void ge_texture_loading_done()
{
    NotGoingToLoadTexturesForAWhileNowSoYouCanCleanUpABit();
}

bool ge_is_ntsc()
{
    return eDisplayType == DT_NTSC;
}

// ---------------------------------------------------------------------------
// Display driver enumeration
// ---------------------------------------------------------------------------

void ge_enumerate_drivers(GEDriverEnumCallback callback, void* ctx)
{
    DDDriverInfo* current = the_manager.CurrDriver;
    DDDriverInfo* driver = the_manager.DriverList;
    while (driver) {
        callback(driver->szName, driver->IsPrimary() != 0, driver == current, ctx);
        driver = driver->Next;
    }
}

// ---------------------------------------------------------------------------
// Texture page management
// ---------------------------------------------------------------------------

void ge_texture_loading_begin()
{
    D3DTexture::BeginLoading();
}

void ge_texture_load_tga(int32_t page, const char* path, bool can_shrink)
{
    TEXTURE_texture[page].LoadTextureTGA(const_cast<CBYTE*>(path), page, can_shrink ? UC_TRUE : UC_FALSE);
}

void ge_texture_create_user_page(int32_t page, int32_t size, bool alpha_fill)
{
    TEXTURE_texture[page].CreateUserPage(size, alpha_fill ? UC_TRUE : UC_FALSE);
}

void ge_texture_destroy(int32_t page)
{
    TEXTURE_texture[page].Destroy();
}

void ge_texture_free_all()
{
    FreeAllD3DPages();
}

void ge_texture_change_tga(int32_t page, const char* path)
{
    TEXTURE_texture[page].ChangeTextureTGA(const_cast<CBYTE*>(path));
}

void ge_texture_font_on(int32_t page)
{
    TEXTURE_texture[page].FontOn();
}

void ge_texture_font2_on(int32_t page)
{
    TEXTURE_texture[page].Font2On();
}

void ge_texture_set_greyscale(int32_t page, bool greyscale)
{
    TEXTURE_texture[page].set_greyscale(greyscale);
}

void ge_get_texture_offset(int32_t page, float* uScale, float* uOffset, float* vScale, float* vOffset)
{
    TEXTURE_texture[page].GetTexOffsetAndScale(uScale, uOffset, vScale, vOffset);
}

int32_t ge_texture_get_size(int32_t page)
{
    return TEXTURE_texture[page].size;
}

int32_t ge_texture_get_type(int32_t page)
{
    return TEXTURE_texture[page].Type;
}

void ge_texture_set_type(int32_t page, int32_t type)
{
    TEXTURE_texture[page].Type = type;
}

GETextureHandle ge_get_texture_handle(int32_t page)
{
    return reinterpret_cast<GETextureHandle>(TEXTURE_texture[page].GetD3DTexture());
}
