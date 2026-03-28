// Outro graphics engine — D3D6 backend implementation.
// Implements the oge_* contract from outro_graphics_engine.h.
// Uses the shared D3D device from common/ (the_display).

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <d3dtypes.h>
#include <string.h>
#include <stdio.h>

#include "engine/graphics/graphics_engine/outro_graphics_engine.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_directx6/common/display_macros.h"
#include "engine/graphics/graphics_engine/backend_directx6/common/d3d_texture.h"
#include "engine/graphics/graphics_engine/backend_directx6/common/gd_display.h"
#include "engine/platform/uc_common.h"
#include "outro/core/outro_os_globals.h"
#include "outro/core/outro_os.h"
#include "outro/core/outro_tga.h"

// ========================================================
// INTERNAL TYPES (D3D-specific, not exposed in contract)
// ========================================================

// uc_orig: OS_Framework (fallen/outro/os.cpp)
typedef class {
public:
    LPDIRECTDRAW4      direct_draw;
    LPDIRECT3DDEVICE3  direct_3d;

    LPDIRECTDRAW4     GetDirectDraw() { return direct_draw; }
    LPDIRECT3DDEVICE3 GetD3DDevice()  { return direct_3d; }
} OGE_Framework;

// uc_orig: OS_Tformat (fallen/outro/os.cpp)
typedef struct {
    SLONG valid;
    DDPIXELFORMAT ddpf;
    SLONG mask_r, mask_g, mask_b, mask_a;
    SLONG shift_r, shift_g, shift_b, shift_a;
} OGE_Tformat;

// Full definition of OS_Texture (opaque via OGETexture in the contract).
// uc_orig: os_texture (fallen/outro/os.cpp)
struct os_texture {
    CBYTE                  name[_MAX_PATH];
    UBYTE                  format;
    UBYTE                  inverted;
    UWORD                  size;
    DDSURFACEDESC2         ddsd;
    LPDIRECTDRAWSURFACE4   ddsurface;
    LPDIRECT3DTEXTURE2     ddtx;
    OS_Texture*            next;
};

// Pre-transformed + lit vertex format for DrawIndexedPrimitive.
// uc_orig: OS_Flert (fallen/outro/os.cpp)
typedef struct {
    float sx, sy, sz;
    float rhw;
    ULONG colour;
    ULONG specular;
    float tu1, tv1;
    float tu2, tv2;
} OGE_Flert;

// uc_orig: OS_FLERT_FORMAT (fallen/outro/os.cpp)
#define OGE_FLERT_FORMAT (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)

// ========================================================
// INTERNAL GLOBALS
// ========================================================

// uc_orig: OS_frame (fallen/outro/os.cpp)
static OGE_Framework oge_frame;

// uc_orig: OS_tformat (fallen/outro/os.cpp)
static OGE_Tformat oge_tformat[OGE_TEXTURE_FORMAT_NUMBER] = {};

// uc_orig: OS_texture_head (fallen/outro/os.cpp)
static OS_Texture* oge_texture_head = NULL;

// uc_orig: OS_pipeline_method_mul (fallen/outro/os.cpp)
static SLONG oge_pipeline_method_mul = 0;

// Texture directory prefix.
// uc_orig: OS_TEXTURE_DIR (fallen/outro/os.cpp)
#define OGE_TEXTURE_DIR "Textures\\"

// Number of hardware multitexture methods tried before giving up.
// uc_orig: OS_METHOD_NUMBER_MUL (fallen/outro/os.cpp)
#define OGE_METHOD_NUMBER_MUL 2

// Debug output helper.
static void oge_string(const char* fmt, ...)
{
    char message[512];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);
    OutputDebugString(message);
}

// ========================================================
// TEXTURE FORMAT ENUMERATION
// ========================================================

// uc_orig: OS_bit_count (fallen/outro/os.cpp)
static SLONG oge_bit_count(ULONG mask)
{
    SLONG ans;
    for (ans = 0; mask; mask &= mask - 1, ans += 1)
        ;
    return ans;
}

// uc_orig: OS_texture_enumerate_pixel_formats (fallen/outro/os.cpp)
static HRESULT CALLBACK oge_texture_enumerate_pixel_formats(
    LPDDPIXELFORMAT lpddpf,
    LPVOID context)
{
    OGE_Tformat* otf = (OGE_Tformat*)malloc(sizeof(OGE_Tformat));

    if (otf == NULL) {
        return D3DENUMRET_CANCEL;
    }

    if (lpddpf->dwFlags & DDPF_RGB) {
        if (lpddpf->dwRGBBitCount == 16) {
            if (lpddpf->dwFlags & DDPF_ALPHAPIXELS) {
                SLONG alphabits = oge_bit_count(lpddpf->dwRGBAlphaBitMask);

                if (alphabits == 1) {
                    oge_tformat[OGE_TEXTURE_FORMAT_1555].valid = UC_TRUE;
                    oge_tformat[OGE_TEXTURE_FORMAT_1555].ddpf  = *lpddpf;
                } else if (alphabits == 4) {
                    oge_tformat[OGE_TEXTURE_FORMAT_4444].valid = UC_TRUE;
                    oge_tformat[OGE_TEXTURE_FORMAT_4444].ddpf  = *lpddpf;
                }
            } else {
                oge_tformat[OGE_TEXTURE_FORMAT_RGB].valid = UC_TRUE;
                oge_tformat[OGE_TEXTURE_FORMAT_RGB].ddpf  = *lpddpf;
            }
        }
    }

    free(otf);

    return D3DENUMRET_OK;
}

// ========================================================
// INIT / SHUTDOWN
// ========================================================

// uc_orig: OS_hack init part (fallen/outro/os.cpp)
void oge_init()
{
    // Grab D3D device from the main engine's display object.
    oge_frame.direct_draw = the_display.lp_DD4;
    oge_frame.direct_3d   = the_display.lp_D3D_Device;

    // Enumerate texture pixel formats for this device.
    memset(oge_tformat, 0, sizeof(oge_tformat));
    oge_frame.GetD3DDevice()->EnumTextureFormats(oge_texture_enumerate_pixel_formats, NULL);

    for (int i = 0; i < OGE_TEXTURE_FORMAT_NUMBER; i++) {
        OGE_Tformat* otf = &oge_tformat[i];

        if (i == OGE_TEXTURE_FORMAT_8) {
            continue;
        }

        if (otf->valid) {
            OS_calculate_mask_and_shift(otf->ddpf.dwRBitMask, &otf->mask_r, &otf->shift_r);
            OS_calculate_mask_and_shift(otf->ddpf.dwGBitMask, &otf->mask_g, &otf->shift_g);
            OS_calculate_mask_and_shift(otf->ddpf.dwBBitMask, &otf->mask_b, &otf->shift_b);

            if (otf->ddpf.dwFlags & DDPF_ALPHAPIXELS) {
                OS_calculate_mask_and_shift(otf->ddpf.dwRGBAlphaBitMask, &otf->mask_a, &otf->shift_a);
            }
        }
    }

    // Get screen dimensions from the main game's display.
    {
        extern SLONG RealDisplayWidth;
        extern SLONG RealDisplayHeight;

        OS_screen_width  = float(RealDisplayWidth);
        OS_screen_height = float(RealDisplayHeight);
    }

    oge_calculate_pipeline();
}

void oge_shutdown()
{
    // Walk texture list and release DDraw surfaces.
    OS_Texture* ot = oge_texture_head;
    while (ot) {
        OS_Texture* next = ot->next;
        if (ot->ddtx) ot->ddtx->Release();
        if (ot->ddsurface) ot->ddsurface->Release();
        free(ot);
        ot = next;
    }
    oge_texture_head = NULL;
}

// ========================================================
// TEXTURES
// ========================================================

// uc_orig: OS_texture_create (from TGA) (fallen/outro/os.cpp)
OGETexture oge_texture_create_from_tga(const char* fname, int32_t invert)
{
    SLONG format;

    OS_Texture*       ot;
    OGE_Tformat*      best_otf;
    OUTRO_TGA_Info    ti;
    OUTRO_TGA_Pixel*  data;
    CBYTE             fullpath[_MAX_PATH];

    for (ot = oge_texture_head; ot; ot = ot->next) {
        if (strcmp(fname, ot->name) == 0) {
            if (ot->inverted == invert) {
                return ot;
            }
        }
    }

    data = (OUTRO_TGA_Pixel*)malloc(256 * 256 * sizeof(OUTRO_TGA_Pixel));

    if (data == NULL) {
        return NULL;
    }

    sprintf(fullpath, OGE_TEXTURE_DIR "%s", fname);

    ti = OUTRO_TGA_load(fullpath, 256, 256, data);

    if (!ti.valid) {
        free(data);
        return NULL;
    }

    if (ti.width != ti.height) {
        free(data);
        return NULL;
    }

    if (ti.flag & TGA_FLAG_CONTAINS_ALPHA) {
        if (ti.flag & TGA_FLAG_ONE_BIT_ALPHA) {
            format = OGE_TEXTURE_FORMAT_1555;
        } else {
            format = OGE_TEXTURE_FORMAT_4444;
        }
    } else if (ti.flag & TGA_FLAG_GRAYSCALE) {
        if (oge_tformat[OGE_TEXTURE_FORMAT_8].valid) {
            format = OGE_TEXTURE_FORMAT_8;
        } else {
            format = OGE_TEXTURE_FORMAT_RGB;
        }
    } else {
        format = OGE_TEXTURE_FORMAT_RGB;
    }

    best_otf = &oge_tformat[format];

    if (!best_otf->valid) {
        free(data);
        return NULL;
    }

    ot = (OS_Texture*)malloc(sizeof(OS_Texture));

    if (ot == NULL) {
        free(data);
        return NULL;
    }

    strncpy(ot->name, fname, _MAX_PATH);
    ot->format   = format;
    ot->inverted = invert;

    memset(&ot->ddsd, 0, sizeof(ot->ddsd));

    ot->ddsd.dwSize             = sizeof(DDSURFACEDESC2);
    ot->ddsd.dwWidth            = ti.width;
    ot->ddsd.dwHeight           = ti.height;
    ot->ddsd.dwMipMapCount      = 1;
    ot->ddsd.dwFlags            = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_MIPMAPCOUNT | DDSD_PIXELFORMAT;
    ot->ddsd.ddsCaps.dwCaps     = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    ot->ddsd.ddsCaps.dwCaps2    = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_HINTSTATIC;
    ot->ddsd.ddpfPixelFormat    = best_otf->ddpf;

    HRESULT res = oge_frame.GetDirectDraw()->CreateSurface(
        &ot->ddsd,
        &ot->ddsurface,
        NULL);

    ASSERT(res == DD_OK);

    if (invert) {
        SLONG i;
        SLONG j;
        OUTRO_TGA_Pixel* tp = data;

        for (i = 0; i < ti.width; i++) {
            for (j = 0; j < ti.height; j++) {
                tp->alpha = 255 - tp->alpha;
                tp->red   = 255 - tp->red;
                tp->green = 255 - tp->green;
                tp->blue  = 255 - tp->blue;
                tp += 1;
            }
        }
    }

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    VERIFY(ot->ddsurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) == DD_OK);

    if (format != OGE_TEXTURE_FORMAT_8) {
        SLONG i;
        SLONG j;
        UWORD pixel_our;
        OUTRO_TGA_Pixel pixel_tga;
        UWORD* wscreen = (UWORD*)ddsd.lpSurface;

        for (i = 0; i < ti.width; i++) {
            for (j = 0; j < ti.height; j++) {
                pixel_tga = data[i + j * ti.width];
                pixel_our = 0;

                pixel_our |= (pixel_tga.red   >> best_otf->mask_r) << best_otf->shift_r;
                pixel_our |= (pixel_tga.green >> best_otf->mask_g) << best_otf->shift_g;
                pixel_our |= (pixel_tga.blue  >> best_otf->mask_b) << best_otf->shift_b;

                if (best_otf->ddpf.dwFlags & DDPF_ALPHAPIXELS) {
                    pixel_our |= (pixel_tga.alpha >> best_otf->mask_a) << best_otf->shift_a;
                }

                wscreen[i + j * (ddsd.lPitch >> 1)] = pixel_our;
            }
        }
    } else {
        SLONG i;
        SLONG j;
        UBYTE* wscreen = (UBYTE*)ddsd.lpSurface;

        for (i = 0; i < ti.width; i++) {
            for (j = 0; j < ti.height; j++) {
                wscreen[i + j * ddsd.lPitch] = data[i + j * ti.width].red;
            }
        }
    }

    ot->ddsurface->Unlock(NULL);

    VERIFY(ot->ddsurface->QueryInterface(IID_IDirect3DTexture2, (void**)&ot->ddtx) == DD_OK);

    ot->next         = oge_texture_head;
    oge_texture_head = ot;
    ot->size         = ti.width;

    free(data);

    return ot;
}

// uc_orig: OS_texture_create (blank) (fallen/outro/os.cpp)
OGETexture oge_texture_create_blank(int32_t size, int32_t format)
{
    OS_Texture* ot;
    OGE_Tformat* otf;

    {
        D3DDEVICEDESC dh;
        D3DDEVICEDESC ds;

        memset(&dh, 0, sizeof(dh));
        memset(&ds, 0, sizeof(ds));
        dh.dwSize = sizeof(dh);
        ds.dwSize = sizeof(ds);

        VERIFY(oge_frame.GetD3DDevice()->GetCaps(&dh, &ds) == D3D_OK);

        if (dh.dwFlags == 0) {
            dh = ds;
        }

        if (size > (SLONG)dh.dwMaxTextureWidth || size > (SLONG)dh.dwMaxTextureHeight) {
            return NULL;
        }
    }

    if (!oge_tformat[format].valid) {
        switch (format) {
        case OGE_TEXTURE_FORMAT_8:
            format = OGE_TEXTURE_FORMAT_RGB;
            break;
        case OGE_TEXTURE_FORMAT_1555:
            format = OGE_TEXTURE_FORMAT_4444;
            break;
        case OGE_TEXTURE_FORMAT_4444:
            format = OGE_TEXTURE_FORMAT_1555;
            break;
        }

        if (!oge_tformat[format].valid) {
            return NULL;
        }
    }

    otf = &oge_tformat[format];

    ot = (OS_Texture*)malloc(sizeof(OS_Texture));

    if (ot == NULL) {
        return NULL;
    }

    sprintf(ot->name, "Generated");
    ot->format = format;
    ot->size   = size;

    memset(&ot->ddsd, 0, sizeof(ot->ddsd));

    ot->ddsd.dwSize          = sizeof(DDSURFACEDESC2);
    ot->ddsd.dwWidth         = size;
    ot->ddsd.dwHeight        = size;
    ot->ddsd.dwMipMapCount   = 1;
    ot->ddsd.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_MIPMAPCOUNT | DDSD_PIXELFORMAT;
    ot->ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    ot->ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_HINTDYNAMIC;
    ot->ddsd.ddpfPixelFormat = otf->ddpf;

    if (oge_frame.GetDirectDraw()->CreateSurface(
            &ot->ddsd,
            &ot->ddsurface,
            NULL)
        != DD_OK) {
        free(ot);
        return NULL;
    }

    VERIFY(ot->ddsurface->QueryInterface(IID_IDirect3DTexture2, (void**)&ot->ddtx) == DD_OK);

    ot->next        = oge_texture_head;
    oge_texture_head = ot;

    return ot;
}

// uc_orig: OS_texture_finished_creating (fallen/outro/os.cpp)
void oge_texture_finished_creating()
{
    // Stub — body commented out in original.
}

// uc_orig: OS_texture_size (fallen/outro/os.cpp)
int32_t oge_texture_size(OGETexture tex)
{
    return tex->size;
}

// uc_orig: OS_texture_lock (fallen/outro/os.cpp)
void oge_texture_lock(OGETexture tex)
{
    OGE_Tformat* otf;
    HRESULT     res;
    DDSURFACEDESC2 ddsd;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    VERIFY((res = tex->ddsurface->Lock(
                NULL,
                &ddsd,
                DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK,
                NULL))
        == DD_OK);

    ASSERT(WITHIN(tex->format, 0, OGE_TEXTURE_FORMAT_NUMBER - 1));

    otf = &oge_tformat[tex->format];

    if (tex->format == OGE_TEXTURE_FORMAT_8) {
        OS_bitmap_ubyte_screen = (UBYTE*)ddsd.lpSurface;
        OS_bitmap_ubyte_pitch  = ddsd.lPitch;
        OS_bitmap_uword_screen = NULL;
        OS_bitmap_uword_pitch  = 0;
    } else {
        OS_bitmap_ubyte_screen = NULL;
        OS_bitmap_ubyte_pitch  = 0;
        OS_bitmap_uword_screen = (UWORD*)ddsd.lpSurface;
        OS_bitmap_uword_pitch  = ddsd.lPitch >> 1;
    }

    OS_bitmap_format  = tex->format;
    OS_bitmap_width   = ddsd.dwWidth;
    OS_bitmap_height  = ddsd.dwHeight;
    OS_bitmap_mask_r  = otf->mask_r;
    OS_bitmap_mask_g  = otf->mask_g;
    OS_bitmap_mask_b  = otf->mask_b;
    OS_bitmap_mask_a  = otf->mask_a;
    OS_bitmap_shift_r = otf->shift_r;
    OS_bitmap_shift_g = otf->shift_g;
    OS_bitmap_shift_b = otf->shift_b;
    OS_bitmap_shift_a = otf->shift_a;
}

// uc_orig: OS_texture_unlock (fallen/outro/os.cpp)
void oge_texture_unlock(OGETexture tex)
{
    tex->ddsurface->Unlock(NULL);
}

// ========================================================
// RENDER STATES
// ========================================================

// uc_orig: OS_init_renderstates (fallen/outro/os.cpp)
void oge_init_renderstates()
{
    LPDIRECT3DDEVICE3 d3d = oge_frame.GetD3DDevice();

    d3d->SetRenderState(D3DRENDERSTATE_SHADEMODE,         D3DSHADE_GOURAUD);
    d3d->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, UC_TRUE);
    d3d->SetRenderState(D3DRENDERSTATE_SPECULARENABLE,     UC_TRUE);
    d3d->SetRenderState(D3DRENDERSTATE_ZENABLE,            UC_TRUE);
    d3d->SetRenderState(D3DRENDERSTATE_ZFUNC,              D3DCMP_LESSEQUAL);
    d3d->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE,       UC_TRUE);
    d3d->SetRenderState(D3DRENDERSTATE_CULLMODE,           D3DCULL_CCW);
    d3d->SetRenderState(D3DRENDERSTATE_FOGENABLE,          UC_FALSE);
    d3d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,   UC_FALSE);
    d3d->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,    UC_FALSE);

    d3d->SetRenderState(D3DRENDERSTATE_ANTIALIAS, D3DANTIALIAS_NONE);

    d3d->SetTextureStageState(0, D3DTSS_COLOROP,       D3DTOP_MODULATE);
    d3d->SetTextureStageState(0, D3DTSS_COLORARG1,     D3DTA_TEXTURE);
    d3d->SetTextureStageState(0, D3DTSS_COLORARG2,     D3DTA_DIFFUSE);
    d3d->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

    d3d->SetTextureStageState(1, D3DTSS_COLOROP,       D3DTOP_DISABLE);
    d3d->SetTextureStageState(1, D3DTSS_COLORARG1,     D3DTA_TEXTURE);
    d3d->SetTextureStageState(1, D3DTSS_COLORARG2,     D3DTA_CURRENT);
    d3d->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);

    d3d->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);

    d3d->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFG_LINEAR);
    d3d->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    d3d->SetTextureStageState(0, D3DTSS_ADDRESS,   D3DTADDRESS_WRAP);

    d3d->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFG_LINEAR);
    d3d->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);

    d3d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    d3d->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    d3d->SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

// uc_orig: OS_pipeline_calculate (fallen/outro/os.cpp)
void oge_calculate_pipeline()
{
    ULONG num_passes;

    LPDIRECT3DDEVICE3 d3d = oge_frame.GetD3DDevice();

    oge_pipeline_method_mul = 0;

    OS_Texture* ot1 = oge_texture_create_blank(32, OGE_TEXTURE_FORMAT_RGB);
    OS_Texture* ot2 = oge_texture_create_blank(32, OGE_TEXTURE_FORMAT_RGB);

    while (1) {
        oge_init_renderstates();

        d3d->SetTexture(0, ot1->ddtx);
        d3d->SetTexture(1, ot2->ddtx);

        switch (oge_pipeline_method_mul) {
        case 1:
            d3d->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            d3d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d3d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            d3d->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            d3d->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d3d->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
            break;

        case 0:
            d3d->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
            d3d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d3d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
            d3d->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            d3d->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d3d->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
            d3d->SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            d3d->SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
            d3d->SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);
            break;

        default:
            break;
        }

        if (oge_pipeline_method_mul == OGE_METHOD_NUMBER_MUL) {
            break;
        }

        if (d3d->ValidateDevice(&num_passes) == D3D_OK) {
            if (num_passes != 0) {
                oge_string("Validated %d with %d passes\n", oge_pipeline_method_mul, num_passes);
                break;
            }
        }

        oge_pipeline_method_mul += 1;
    }

    oge_string("Multitexture method %d\n", oge_pipeline_method_mul);
}

// uc_orig: OS_change_renderstate_for_type (fallen/outro/os.cpp)
void oge_change_renderstate(uint32_t draw)
{
    LPDIRECT3DDEVICE3 d3d = oge_frame.GetD3DDevice();

    if (draw & OS_DRAW_ADD) {
        d3d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
        d3d->SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_ONE);
        d3d->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
    }

    if (draw & OS_DRAW_MULTIPLY) {
        d3d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
        d3d->SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_DESTCOLOR);
        d3d->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
    }

    if (draw & OS_DRAW_MULBYONE) {
        d3d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
        d3d->SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_DESTCOLOR);
        d3d->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
    }

    if (draw & OS_DRAW_CLAMP) {
        d3d->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
    }

    if (draw & OS_DRAW_DECAL) {
        d3d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    }

    if (draw & OS_DRAW_TRANSPARENT) {
        d3d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
        d3d->SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_ZERO);
        d3d->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
    }

    if (draw & OS_DRAW_DOUBLESIDED) {
        d3d->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    }

    if (draw & OS_DRAW_NOZWRITE) {
        d3d->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, UC_FALSE);
    }

    if (draw & OS_DRAW_ALPHAREF) {
        d3d->SetRenderState(D3DRENDERSTATE_ALPHAFUNC,       D3DCMP_NOTEQUAL);
        d3d->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, UC_TRUE);
        d3d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        d3d->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
    }

    if (draw & OS_DRAW_ZREVERSE) {
        d3d->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_GREATEREQUAL);
    }

    if (draw & OS_DRAW_ZALWAYS) {
        d3d->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
    }

    if (draw & OS_DRAW_CULLREVERSE) {
        d3d->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
    }

    if (draw & OS_DRAW_ALPHABLEND) {
        d3d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
        d3d->SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
        d3d->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        d3d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        d3d->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        d3d->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
    }

    if (draw & OS_DRAW_TEX_NONE) {
        d3d->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
        d3d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    }

    if (draw & OS_DRAW_TEX_MUL) {
        switch (oge_pipeline_method_mul) {
        case 1:
            if (draw & OS_DRAW_DECAL) {
                d3d->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                d3d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                d3d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
            } else {
                d3d->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                d3d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                d3d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            }
            d3d->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            d3d->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d3d->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
            break;

        case 0:
            d3d->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
            d3d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d3d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
            d3d->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            d3d->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d3d->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
            if (draw & OS_DRAW_DECAL) {
                // No diffuse multiply
            } else {
                d3d->SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                d3d->SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                d3d->SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);
            }
            break;

        default:
            break;
        }
    }

    if (draw & OS_DRAW_NOFILTER) {
        d3d->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFG_POINT);
        d3d->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_POINT);
        d3d->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFG_POINT);
        d3d->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFG_POINT);
    }
}

// uc_orig: OS_undo_renderstate_type_changes (fallen/outro/os.cpp)
void oge_undo_renderstate_changes()
{
    LPDIRECT3DDEVICE3 d3d = oge_frame.GetD3DDevice();

    d3d->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    d3d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d3d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    d3d->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    d3d->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d3d->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

    d3d->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);

    d3d->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

    d3d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_FALSE);
    d3d->SetRenderState(D3DRENDERSTATE_CULLMODE,         D3DCULL_CCW);
    d3d->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE,     UC_TRUE);
    d3d->SetRenderState(D3DRENDERSTATE_ZFUNC,            D3DCMP_LESSEQUAL);
    d3d->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  UC_FALSE);

    d3d->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFG_LINEAR);
    d3d->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);

    d3d->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFG_LINEAR);
    d3d->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
}

// ========================================================
// DRAWING
// ========================================================

// uc_orig: part of OS_buffer_draw (fallen/outro/os.cpp)
void oge_bind_texture(int32_t stage, OGETexture tex)
{
    LPDIRECT3DDEVICE3 d3d = oge_frame.GetD3DDevice();
    d3d->SetTexture(stage, tex ? tex->ddtx : NULL);
}

// uc_orig: part of OS_buffer_draw (fallen/outro/os.cpp)
void oge_draw_indexed(const void* verts, int32_t num_verts,
                      const uint16_t* indices, int32_t num_indices)
{
    LPDIRECT3DDEVICE3 d3d = oge_frame.GetD3DDevice();

    {
        ULONG num_passes;
        if (d3d->ValidateDevice(&num_passes) != D3D_OK) {
            oge_string("Validation failed\n");
        }
    }

    d3d->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        OGE_FLERT_FORMAT,
        (void*)verts,
        num_verts,
        (WORD*)indices,
        num_indices,
        D3DDP_DONOTUPDATEEXTENTS);
}

// ========================================================
// SCENE
// ========================================================

// uc_orig: OS_clear_screen (fallen/outro/os.cpp)
void oge_clear_screen()
{
    CLEAR_VIEWPORT;
}

// uc_orig: OS_scene_begin (fallen/outro/os.cpp)
void oge_scene_begin()
{
    ge_begin_scene();
    oge_init_renderstates();
}

// uc_orig: OS_scene_end (fallen/outro/os.cpp)
void oge_scene_end()
{
    ge_end_scene();
}

// uc_orig: OS_show (fallen/outro/os.cpp)
void oge_show()
{
    FLIP(NULL, DDFLIP_WAIT);
}
