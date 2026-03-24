// Outro OS platform layer (complete migration).
// Covers: OS_Framework, joystick, texture management, render states,
// Windows helpers, camera+transform, vertex buffers, render loop, and sound.

#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>
#include <d3d.h>
#include <d3dtypes.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0700
#endif
#include <dinput.h>
#include <stdarg.h>
#include <string.h>

#include "ui/cutscenes/outro/outro_os.h"
#include "ui/cutscenes/outro/outro_os_globals.h"
#include "ui/cutscenes/outro/outro_key.h"
#include "ui/cutscenes/outro/outro_matrix.h"
#include "ui/cutscenes/outro/outro_tga.h"
#include "engine/platform/platform.h"
#include "engine/graphics/graphics_api/display_macros.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/keyboard.h"
#include "engine/audio/mfx.h"
#include "engine/audio/music.h"
#include "assets/sound_id.h"

// OS_calculate_mask_and_shift is defined in engine/graphics/resources/d3d_texture.cpp
// and declared here for use in OS_texture_lock and OS_hack.
#include "engine/graphics/resources/d3d_texture.h"

// Entry point from outro system.
extern void MAIN_main(void);

// OS_Framework, OS_frame, OS_Tformat, OS_tformat, OS_texture_head are
// defined/declared in outro_os_globals.h / outro_os_globals.cpp.

// ========================================================
// TEXTURE INTERNAL TYPES
// ========================================================

// Texture directory prefix.
// uc_orig: OS_TEXTURE_DIR (fallen/outro/os.cpp)
#define OS_TEXTURE_DIR "Textures\\"

// Full definition of OS_Texture (opaque in the header).
// uc_orig: os_texture (fallen/outro/os.cpp)
typedef struct os_texture {
    CBYTE                  name[_MAX_PATH];
    UBYTE                  format;
    UBYTE                  inverted;
    UWORD                  size;
    DDSURFACEDESC2         ddsd;
    LPDIRECTDRAWSURFACE4   ddsurface;
    LPDIRECT3DTEXTURE2     ddtx;
    OS_Texture*            next;
} OS_Texture;

// ========================================================
// JOYSTICK
// ========================================================

// uc_orig: OS_joy_poll (fallen/outro/os.cpp)
void OS_joy_poll(void)
{
    HRESULT hr;

    if (OS_joy_direct_input == NULL || OS_joy_input_device == NULL || OS_joy_input_device2 == NULL) {
        OS_joy_x          = 0.0F;
        OS_joy_y          = 0.0F;
        OS_joy_button     = 0;
        OS_joy_button_down = 0;
        OS_joy_button_up  = 0;
        return;
    }

    hr = OS_joy_input_device->Acquire();

    if (hr == DI_OK) {
        DIJOYSTATE js;

        OS_joy_input_device2->Poll();

        hr = OS_joy_input_device->GetDeviceState(sizeof(js), &js);

        if (!FAILED(hr)) {
            SLONG dx = OS_joy_x_range_max - OS_joy_x_range_min;
            SLONG dy = OS_joy_y_range_max - OS_joy_y_range_min;

            OS_joy_x = 0.0F;
            OS_joy_y = 0.0F;

            if (dx) {
                OS_joy_x = float(js.lX - OS_joy_x_range_min) * 2.0F / float(dx) - 1.0F;
            }
            if (dy) {
                OS_joy_y = float(js.lY - OS_joy_y_range_min) * 2.0F / float(dy) - 1.0F;
            }

            SLONG i;
            ULONG last = OS_joy_button;
            ULONG now  = 0;

            for (i = 0; i < 32; i++) {
                if (js.rgbButtons[i] & 0x80) {
                    now |= 1 << i;
                }
            }

            OS_joy_button      = now;
            OS_joy_button_down = now & ~last;
            OS_joy_button_up   = last & ~now;
        }

        OS_joy_input_device->Unacquire();
    }
}

// ========================================================
// TEXTURE UTILITIES
// ========================================================

// Counts the number of set bits in a bitmask — used to identify alpha bit depth (1 or 4).
// uc_orig: OS_bit_count (fallen/outro/os.cpp)
SLONG OS_bit_count(ULONG mask)
{
    SLONG ans;
    for (ans = 0; mask; mask &= mask - 1, ans += 1)
        ;
    return ans;
}

// DirectDraw texture format enumeration callback — selects the best pixel format for each
// OS_TEXTURE_FORMAT_* slot based on the formats the hardware supports.
// uc_orig: OS_texture_enumerate_pixel_formats (fallen/outro/os.cpp)
HRESULT CALLBACK OS_texture_enumerate_pixel_formats(
    LPDDPIXELFORMAT lpddpf,
    LPVOID context)
{
    OS_Tformat* otf = (OS_Tformat*)malloc(sizeof(OS_Tformat));

    if (otf == NULL) {
        return D3DENUMRET_CANCEL;
    }

    if (lpddpf->dwFlags & DDPF_RGB) {
        if (lpddpf->dwRGBBitCount == 16) {
            if (lpddpf->dwFlags & DDPF_ALPHAPIXELS) {
                SLONG alphabits = OS_bit_count(lpddpf->dwRGBAlphaBitMask);

                if (alphabits == 1) {
                    OS_tformat[OS_TEXTURE_FORMAT_1555].valid = UC_TRUE;
                    OS_tformat[OS_TEXTURE_FORMAT_1555].ddpf  = *lpddpf;
                } else if (alphabits == 4) {
                    OS_tformat[OS_TEXTURE_FORMAT_4444].valid = UC_TRUE;
                    OS_tformat[OS_TEXTURE_FORMAT_4444].ddpf  = *lpddpf;
                }
            } else {
                OS_tformat[OS_TEXTURE_FORMAT_RGB].valid = UC_TRUE;
                OS_tformat[OS_TEXTURE_FORMAT_RGB].ddpf  = *lpddpf;
            }
        }
    }

    free(otf);

    return D3DENUMRET_OK;
}

// Computes the pixel mask (how many bits to discard) and shift (where to place the channel)
// from a DirectDraw bitmask. Used to pack R, G, B, A into the target pixel format.
// Note: OS_calculate_mask_and_shift is implemented in engine/graphics/resources/d3d_texture.cpp;
// the prototype is included from there.

// Creates a texture from a TGA file. Returns an existing texture if the same file+invert
// combination was already loaded (deduplication by filename).
// uc_orig: OS_texture_create (fallen/outro/os.cpp)
OS_Texture* OS_texture_create(CBYTE* fname, SLONG invert)
{
    SLONG format;

    OS_Texture*       ot;
    OS_Tformat*       best_otf;
    OUTRO_TGA_Info    ti;
    OUTRO_TGA_Pixel*  data;
    CBYTE             fullpath[_MAX_PATH];

    for (ot = OS_texture_head; ot; ot = ot->next) {
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

    sprintf(fullpath, OS_TEXTURE_DIR "%s", fname);

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
            format = OS_TEXTURE_FORMAT_1555;
        } else {
            format = OS_TEXTURE_FORMAT_4444;
        }
    } else if (ti.flag & TGA_FLAG_GRAYSCALE) {
        if (OS_tformat[OS_TEXTURE_FORMAT_8].valid) {
            format = OS_TEXTURE_FORMAT_8;
        } else {
            format = OS_TEXTURE_FORMAT_RGB;
        }
    } else {
        format = OS_TEXTURE_FORMAT_RGB;
    }

    best_otf = &OS_tformat[format];

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

    HRESULT res = OS_frame.GetDirectDraw()->CreateSurface(
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

    if (format != OS_TEXTURE_FORMAT_8) {
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

    ot->next         = OS_texture_head;
    OS_texture_head  = ot;
    ot->size         = ti.width;

    free(data);

    return ot;
}

// Creates a blank (uninitialized) texture of a given size and format.
// Used to create writable dynamic textures (e.g. for font glyphs).
// uc_orig: OS_texture_create (fallen/outro/os.cpp)
OS_Texture* OS_texture_create(SLONG size, SLONG format)
{
    OS_Texture* ot;
    OS_Tformat* otf;

    {
        D3DDEVICEDESC dh;
        D3DDEVICEDESC ds;

        memset(&dh, 0, sizeof(dh));
        memset(&ds, 0, sizeof(ds));
        dh.dwSize = sizeof(dh);
        ds.dwSize = sizeof(ds);

        VERIFY(OS_frame.GetD3DDevice()->GetCaps(&dh, &ds) == D3D_OK);

        if (dh.dwFlags == 0) {
            dh = ds;
        }

        if (size > dh.dwMaxTextureWidth || size > dh.dwMaxTextureHeight) {
            return NULL;
        }
    }

    if (!OS_tformat[format].valid) {
        switch (format) {
        case OS_TEXTURE_FORMAT_8:
            format = OS_TEXTURE_FORMAT_RGB;
            break;
        case OS_TEXTURE_FORMAT_1555:
            format = OS_TEXTURE_FORMAT_4444;
            break;
        case OS_TEXTURE_FORMAT_4444:
            format = OS_TEXTURE_FORMAT_1555;
            break;
        }

        if (!OS_tformat[format].valid) {
            return NULL;
        }
    }

    otf = &OS_tformat[format];

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

    if (OS_frame.GetDirectDraw()->CreateSurface(
            &ot->ddsd,
            &ot->ddsurface,
            NULL)
        != DD_OK) {
        free(ot);
        return NULL;
    }

    VERIFY(ot->ddsurface->QueryInterface(IID_IDirect3DTexture2, (void**)&ot->ddtx) == DD_OK);

    ot->next        = OS_texture_head;
    OS_texture_head = ot;

    return ot;
}

// Called after all textures are loaded to hint the driver to promote them to VRAM.
// The body is commented out in the original — the function is a no-op stub.
// uc_orig: OS_texture_finished_creating (fallen/outro/os.cpp)
void OS_texture_finished_creating()
{
    // Stub — body commented out in original.
}

// uc_orig: OS_texture_size (fallen/outro/os.cpp)
SLONG OS_texture_size(OS_Texture* ot)
{
    return ot->size;
}

// Locks the texture surface for CPU write access. Populates the OS_bitmap_* globals
// with the surface pointer, pitch, dimensions, and channel masks.
// uc_orig: OS_texture_lock (fallen/outro/os.cpp)
void OS_texture_lock(OS_Texture* ot)
{
    OS_Tformat* otf;
    HRESULT     res;
    DDSURFACEDESC2 ddsd;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    VERIFY((res = ot->ddsurface->Lock(
                NULL,
                &ddsd,
                DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK,
                NULL))
        == DD_OK);

    ASSERT(WITHIN(ot->format, 0, OS_TEXTURE_FORMAT_NUMBER - 1));

    otf = &OS_tformat[ot->format];

    if (ot->format == OS_TEXTURE_FORMAT_8) {
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

    OS_bitmap_format  = ot->format;
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
void OS_texture_unlock(OS_Texture* ot)
{
    ot->ddsurface->Unlock(NULL);
}

// ========================================================
// RENDER STATE
// ========================================================

// Sets all D3D render states to the default: Gouraud shaded, zbuffered, CCW cull,
// no alpha blending, one texture stage.
// uc_orig: OS_init_renderstates (fallen/outro/os.cpp)
void OS_init_renderstates()
{
    LPDIRECT3DDEVICE3 d3d = OS_frame.GetD3DDevice();

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

    if (KEY_on[KEY_A]) {
        d3d->SetRenderState(D3DRENDERSTATE_ANTIALIAS, D3DANTIALIAS_SORTINDEPENDENT);
    } else {
        d3d->SetRenderState(D3DRENDERSTATE_ANTIALIAS, D3DANTIALIAS_NONE);
    }

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

// Number of hardware multitexture methods tried before giving up.
// uc_orig: OS_METHOD_NUMBER_MUL (fallen/outro/os.cpp)
#define OS_METHOD_NUMBER_MUL 2

// Probes which multitexture blending method the current D3D device supports.
// Sets OS_pipeline_method_mul to 0 (3-stage), 1 (2-stage), or 2 (none).
// uc_orig: OS_pipeline_calculate (fallen/outro/os.cpp)
void OS_pipeline_calculate()
{
    ULONG num_passes;

    LPDIRECT3DDEVICE3 d3d = OS_frame.GetD3DDevice();

    OS_pipeline_method_mul = 0;

    OS_Texture* ot1 = OS_texture_create(32, OS_TEXTURE_FORMAT_RGB);
    OS_Texture* ot2 = OS_texture_create(32, OS_TEXTURE_FORMAT_RGB);

    while (1) {
        OS_init_renderstates();

        d3d->SetTexture(0, ot1->ddtx);
        d3d->SetTexture(1, ot2->ddtx);

        switch (OS_pipeline_method_mul) {
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

        if (OS_pipeline_method_mul == OS_METHOD_NUMBER_MUL) {
            break;
        }

        if (d3d->ValidateDevice(&num_passes) == D3D_OK) {
            if (num_passes != 0) {
                OS_string("Validated %d with %d passes\n", OS_pipeline_method_mul, num_passes);
                break;
            }
        }

        OS_pipeline_method_mul += 1;
    }

    OS_string("Multitexture method %d\n", OS_pipeline_method_mul);
}

// Applies D3D render state overrides based on OS_DRAW_* flag combinations.
// Called before DrawIndexedPrimitive when the draw mode is not OS_DRAW_NORMAL.
// uc_orig: OS_change_renderstate_for_type (fallen/outro/os.cpp)
void OS_change_renderstate_for_type(ULONG draw)
{
    LPDIRECT3DDEVICE3 d3d = OS_frame.GetD3DDevice();

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
        switch (OS_pipeline_method_mul) {
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

// Restores the default render states for stages 0, 1, 2 after a custom draw call.
// uc_orig: OS_undo_renderstate_type_changes (fallen/outro/os.cpp)
void OS_undo_renderstate_type_changes(void)
{
    LPDIRECT3DDEVICE3 d3d = OS_frame.GetD3DDevice();

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
// WINDOWS UTILITIES
// ========================================================

// uc_orig: OS_string (fallen/outro/os.cpp)
void OS_string(CBYTE* fmt, ...)
{
    CBYTE message[512];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);
    OutputDebugString(message);
}

// uc_orig: OS_ticks (fallen/outro/os.cpp)
SLONG OS_ticks(void)
{
    return GetTickCount() - OS_game_start_tick_count;
}

// uc_orig: OS_ticks_reset (fallen/outro/os.cpp)
void OS_ticks_reset()
{
    OS_game_start_tick_count = GetTickCount();
}

// ========================================================
// MOUSE
// ========================================================

// uc_orig: OS_mouse_get (fallen/outro/os.cpp)
void OS_mouse_get(SLONG* x, SLONG* y)
{
    POINT point;
    GetCursorPos(&point);
    *x = point.x;
    *y = point.y;
}

// uc_orig: OS_mouse_set (fallen/outro/os.cpp)
void OS_mouse_set(SLONG x, SLONG y)
{
    SetCursorPos(x, y);
}

// Processes Windows messages and maps keyboard/joystick input.
// Note: the function returns early at OS_CARRY_ON before the message pump —
// the message pump code below is unreachable dead code from the original.
// uc_orig: OS_process_messages (fallen/outro/os.cpp)
SLONG OS_process_messages()
{
    SHELL_ACTIVE;

    if (Keys[KB_ESC]) {
        Keys[KB_ESC] = 0;
        KEY_on[KEY_ESCAPE] = UC_TRUE;
    }

    return OS_CARRY_ON;
}

// ========================================================
// CAMERA AND TRANSFORM
// ========================================================

// uc_orig: OS_camera_set (fallen/outro/os.cpp)
void OS_camera_set(
    float world_x, float world_y, float world_z,
    float view_dist,
    float yaw, float pitch, float roll,
    float lens,
    float screen_x1, float screen_y1,
    float screen_x2, float screen_y2)
{
    OS_cam_screen_x1 = screen_x1 * OS_screen_width;
    OS_cam_screen_y1 = screen_y1 * OS_screen_height;
    OS_cam_screen_x2 = screen_x2 * OS_screen_width;
    OS_cam_screen_y2 = screen_y2 * OS_screen_height;

    OS_cam_screen_width  = OS_cam_screen_x2 - OS_cam_screen_x1;
    OS_cam_screen_height = OS_cam_screen_y2 - OS_cam_screen_y1;
    OS_cam_screen_mid_x  = OS_cam_screen_x1 + OS_cam_screen_width  * 0.5F;
    OS_cam_screen_mid_y  = OS_cam_screen_y1 + OS_cam_screen_height * 0.5F;
    OS_cam_screen_mul_x  = OS_cam_screen_width  * 0.5F / OS_ZCLIP_PLANE;
    OS_cam_screen_mul_y  = OS_cam_screen_height * 0.5F / OS_ZCLIP_PLANE;

    OS_cam_x = world_x;
    OS_cam_y = world_y;
    OS_cam_z = world_z;

    OS_cam_lens           = lens;
    OS_cam_view_dist      = view_dist;
    OS_cam_over_view_dist = 1.0F / view_dist;
    OS_cam_aspect         = OS_cam_screen_height / OS_cam_screen_width;

    MATRIX_calc(OS_cam_matrix, yaw, pitch, roll);

    memcpy(OS_cam_view_matrix, OS_cam_matrix, sizeof(OS_cam_view_matrix));

    MATRIX_skew(
        OS_cam_matrix,
        OS_cam_aspect,
        OS_cam_lens,
        OS_cam_over_view_dist);
}

// OS_trans[] and OS_trans_upto defined in outro_os_globals.cpp.

// Projects a world-space point into screen-space and stores it in an OS_Trans slot.
// Sets OS_CLIP_* flags based on the result.
// uc_orig: OS_transform (fallen/outro/os.cpp)
void OS_transform(float world_x, float world_y, float world_z, OS_Trans* os)
{
    os->x = world_x - OS_cam_x;
    os->y = world_y - OS_cam_y;
    os->z = world_z - OS_cam_z;

    MATRIX_MUL(OS_cam_matrix, os->x, os->y, os->z);

    os->clip = OS_CLIP_ROTATED;

    if (os->z < OS_ZCLIP_PLANE) {
        os->clip |= OS_CLIP_NEAR;
        return;
    } else if (os->z > 1.0F) {
        os->clip |= OS_CLIP_FAR;
        return;
    } else {
        os->Z = OS_ZCLIP_PLANE / os->z;
        os->X = OS_cam_screen_mid_x + OS_cam_screen_mul_x * os->x * os->Z;
        os->Y = OS_cam_screen_mid_y - OS_cam_screen_mul_y * os->y * os->Z;

        os->clip |= OS_CLIP_TRANSFORMED;

        if (os->X < 0.0F) {
            os->clip |= OS_CLIP_LEFT;
        } else if (os->X > OS_screen_width) {
            os->clip |= OS_CLIP_RIGHT;
        }

        if (os->Y < 0.0F) {
            os->clip |= OS_CLIP_TOP;
        } else if (os->Y > OS_screen_height) {
            os->clip |= OS_CLIP_BOTTOM;
        }

        return;
    }
}

// ========================================================
// RENDER LOOP
// ========================================================

// uc_orig: OS_clear_screen (fallen/outro/os.cpp)
void OS_clear_screen(UBYTE r, UBYTE g, UBYTE b, float z)
{
    CLEAR_VIEWPORT;

    /*
    ULONG colour = (r << 16) | (g << 8) | (b << 0);
    HRESULT ret = OS_frame.GetViewport()->Clear2(...);
    */
}

// uc_orig: OS_scene_begin (fallen/outro/os.cpp)
void OS_scene_begin()
{
    OS_frame.GetD3DDevice()->BeginScene();
    OS_init_renderstates();
}

// uc_orig: OS_scene_end (fallen/outro/os.cpp)
void OS_scene_end()
{
    OS_frame.GetD3DDevice()->EndScene();
}

// Draws a framerate indicator as a row of ticks at the top of the screen.
// Uses function-local statics for fps tracking.
// uc_orig: OS_fps (fallen/outro/os.cpp)
void OS_fps()
{
    SLONG i;
    static SLONG fps = 0;
    static SLONG last_time = 0;
    static SLONG last_frame_count = 0;
    static SLONG frame_count = 0;

    float x1;
    float y1;
    float x2;
    float y2;
    float tick;
    SLONG now;

    now = OS_ticks();
    frame_count += 1;

    if (now >= last_time + 1000) {
        fps = frame_count - last_frame_count;
        last_frame_count = frame_count;
        last_time = now;
    }

    OS_Buffer* ob = OS_buffer_new();

    for (i = 0; i < fps; i++) {
        switch ((i + 1) % 10) {
        case 0:
            tick = 8.0F;
            break;
        case 5:
            tick = 5.0F;
            break;
        default:
            tick = 3.0F;
            break;
        }

        x1 = float(i)       / 100.0F;
        x2 = float(i + 1)   / 100.0F - 0.001F;
        y1 = 1.0F - tick    / OS_screen_height;
        y2 = 1.0F;

        OS_buffer_add_sprite(
            ob,
            x1, y1,
            x2, y2,
            0.0F, 1.0F,
            0.0F, 1.0F,
            0.0F,
            0x00ffffff,
            0x00000000,
            OS_FADE_BOTTOM);
    }

    OS_buffer_draw(ob, NULL);
}

// uc_orig: OS_show (fallen/outro/os.cpp)
void OS_show()
{
    FLIP(NULL, DDFLIP_WAIT);
}

// ========================================================
// VERTEX BUFFERS
// ========================================================

// Pre-transformed + lit vertex format for DrawIndexedPrimitive.
// uc_orig: OS_Flert (fallen/outro/os.cpp)
typedef struct {
    float sx, sy, sz;
    float rhw;
    ULONG colour;
    ULONG specular;
    float tu1, tv1;
    float tu2, tv2;
} OS_Flert;

// uc_orig: OS_FLERT_FORMAT (fallen/outro/os.cpp)
#define OS_FLERT_FORMAT (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)

// Full definition of OS_Buffer (opaque in the header).
// uc_orig: os_buffer (fallen/outro/os.cpp)
typedef struct os_buffer {
    SLONG       num_flerts;
    SLONG       num_indices;
    SLONG       max_flerts;
    SLONG       max_indices;
    OS_Flert*   flert;
    UWORD*      index;
    OS_Buffer*  next;
} OS_Buffer;

// OS_buffer_free defined in outro_os_globals.cpp.

// uc_orig: OS_buffer_create (fallen/outro/os.cpp)
OS_Buffer* OS_buffer_create(void)
{
    OS_Buffer* ob = (OS_Buffer*)malloc(sizeof(OS_Buffer));

    ob->max_flerts  = 256;
    ob->max_indices = 1024;
    ob->num_flerts  = 0;
    ob->num_indices = 1;
    ob->flert       = (OS_Flert*)malloc(sizeof(OS_Flert) * ob->max_flerts);
    ob->index       = (UWORD*)malloc(sizeof(UWORD)      * ob->max_indices);

    memset(ob->flert,  0, sizeof(OS_Flert) * ob->max_flerts);
    memset(ob->index,  0, sizeof(UWORD)    * ob->max_indices);

    ob->next = NULL;

    return ob;
}

// Returns a buffer from the free list or creates a new one.
// uc_orig: OS_buffer_get (fallen/outro/os.cpp)
OS_Buffer* OS_buffer_get(void)
{
    OS_Buffer* ans;

    if (OS_buffer_free) {
        ans              = OS_buffer_free;
        OS_buffer_free   = OS_buffer_free->next;
        ans->next        = NULL;
    } else {
        ans = OS_buffer_create();
    }

    return ans;
}

// Returns a buffer to the free list.
// uc_orig: OS_buffer_give (fallen/outro/os.cpp)
void OS_buffer_give(OS_Buffer* ob)
{
    ob->next       = OS_buffer_free;
    OS_buffer_free = ob;
}

// uc_orig: OS_buffer_new (fallen/outro/os.cpp)
OS_Buffer* OS_buffer_new(void)
{
    OS_Buffer* ob = OS_buffer_get();
    ob->num_indices = 0;
    ob->num_flerts  = 1;
    return ob;
}

// uc_orig: OS_buffer_grow_flerts (fallen/outro/os.cpp)
void OS_buffer_grow_flerts(OS_Buffer* ob)
{
    ob->max_flerts *= 2;
    ob->flert = (OS_Flert*)realloc(ob->flert, ob->max_flerts * sizeof(OS_Flert));
}

// uc_orig: OS_buffer_grow_indices (fallen/outro/os.cpp)
void OS_buffer_grow_indices(OS_Buffer* ob)
{
    ob->max_indices *= 2;
    ob->index = (UWORD*)realloc(ob->index, ob->max_indices * sizeof(UWORD));
}

// Adds a single vertex to the buffer, converting it from OS_Vert + OS_Trans format
// to the pre-transformed OS_Flert format used by DrawIndexedPrimitive.
// uc_orig: OS_buffer_add_vert (fallen/outro/os.cpp)
void OS_buffer_add_vert(OS_Buffer* ob, OS_Vert* ov)
{
    OS_Trans* ot;
    OS_Flert* of;

    if (ob->num_flerts >= ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    ASSERT(WITHIN(ov->trans, 0, OS_MAX_TRANS - 1));

    of = &ob->flert[ob->num_flerts];
    ot = &OS_trans[ov->trans];

    of->sx       = ot->X;
    of->sy       = ot->Y;
    of->sz       = 1.0F - ot->Z;
    of->rhw      = ot->Z;
    of->colour   = ov->colour;
    of->specular = ov->specular;
    of->tu1      = ov->u1;
    of->tv1      = ov->v1;
    of->tu2      = ov->u2;
    of->tv2      = ov->v2;

    ov->index = ob->num_flerts++;
}

// Adds a triangle to the buffer. Performs trivial reject if all three vertices
// are off the same side of the screen; skips near/far-clipped triangles.
// uc_orig: OS_buffer_add_triangle (fallen/outro/os.cpp)
void OS_buffer_add_triangle(OS_Buffer* ob, OS_Vert* ov1, OS_Vert* ov2, OS_Vert* ov3)
{
    ULONG clip_and = OS_trans[ov1->trans].clip & OS_trans[ov2->trans].clip & OS_trans[ov3->trans].clip;

    if (clip_and & OS_CLIP_TRANSFORMED) {
        if (clip_and & OS_CLIP_OFFSCREEN) {
            return;
        } else {
            if (ov1->index == NULL) {
                OS_buffer_add_vert(ob, ov1);
            }
            if (ov2->index == NULL) {
                OS_buffer_add_vert(ob, ov2);
            }
            if (ov3->index == NULL) {
                OS_buffer_add_vert(ob, ov3);
            }

            if (ob->num_indices + 3 > ob->max_indices) {
                OS_buffer_grow_indices(ob);
            }

            ob->index[ob->num_indices++] = ov1->index;
            ob->index[ob->num_indices++] = ov2->index;
            ob->index[ob->num_indices++] = ov3->index;

            return;
        }
    } else {
        ULONG clip_or = OS_trans[ov1->trans].clip | OS_trans[ov2->trans].clip | OS_trans[ov3->trans].clip;

        if (clip_or & OS_CLIP_TRANSFORMED) {
            // Needs z-clipping — not implemented, skip.
            return;
        } else {
            // Entirely outside both clip planes.
            return;
        }
    }
}

// Adds a 2D screen-space sprite as two triangles. The four corners are arranged:
//   2-----0
//   |     |
//   3-----1
// Coordinates are normalised (0..1) and scaled by screen dimensions.
// uc_orig: OS_buffer_add_sprite (fallen/outro/os.cpp)
void OS_buffer_add_sprite(
    OS_Buffer* ob,
    float x1, float y1,
    float x2, float y2,
    float u1, float v1,
    float u2, float v2,
    float z,
    ULONG colour,
    ULONG specular,
    ULONG fade)
{
    SLONG i;
    OS_Flert* of;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        of->sx       = ((i & 1) ? x1 : x2) * OS_screen_width;
        of->sy       = ((i & 2) ? y1 : y2) * OS_screen_height;
        of->sz       = z;
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = ((i & 1) ? u1 : u2);
        of->tv1      = ((i & 2) ? v1 : v2);
        of->tu2      = ((i & 1) ? u1 : u2);
        of->tv2      = ((i & 2) ? v1 : v2);
    }

    if (fade) {
        if (fade & OS_FADE_TOP) {
            ob->flert[ob->num_flerts + 2].colour = 0x00000000;
            ob->flert[ob->num_flerts + 3].colour = 0x00000000;
        }
        if (fade & OS_FADE_BOTTOM) {
            ob->flert[ob->num_flerts + 0].colour = 0x00000000;
            ob->flert[ob->num_flerts + 1].colour = 0x00000000;
        }
        if (fade & OS_FADE_LEFT) {
            ob->flert[ob->num_flerts + 1].colour = 0x00000000;
            ob->flert[ob->num_flerts + 3].colour = 0x00000000;
        }
        if (fade & OS_FADE_RIGHT) {
            ob->flert[ob->num_flerts + 0].colour = 0x00000000;
            ob->flert[ob->num_flerts + 2].colour = 0x00000000;
        }
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// Adds a rotated square sprite. The 'size' parameter is the radius as a fraction
// of screen width; 'angle' is in radians. The Y axis is scaled by 1.33 to compensate
// for the non-square pixel ratio of the screen.
// uc_orig: OS_buffer_add_sprite_rot (fallen/outro/os.cpp)
void OS_buffer_add_sprite_rot(
    OS_Buffer* ob,
    float x_mid, float y_mid,
    float size,
    float angle,
    float u1, float v1,
    float u2, float v2,
    float z,
    ULONG colour,
    ULONG specular,
    float tu1, float tv1,
    float tu2, float tv2)
{
    SLONG i;
    OS_Flert* of;

    float dx = sin(angle) * size;
    float dy = cos(angle) * size;
    float x;
    float y;

    x_mid *= OS_screen_width;
    y_mid *= OS_screen_height;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        x = 0.0F;
        y = 0.0F;

        if (i & 1) { x -= dx; y -= dy; }
        else        { x += dx; y += dy; }

        if (i & 2) { x += -dy; y += +dx; }
        else        { x -= -dy; y -= +dx; }

        x *= OS_screen_width;
        y *= OS_screen_height * 1.33F;

        x += x_mid;
        y += y_mid;

        of->sx       = x;
        of->sy       = y;
        of->sz       = z;
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = ((i & 1) ? u1  : u2);
        of->tv1      = ((i & 2) ? v1  : v2);
        of->tu2      = ((i & 1) ? tu1 : tu2);
        of->tv2      = ((i & 2) ? tv1 : tv2);
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// Adds a quad with four independently specified corners. Useful for perspective
// warped or non-rectangular sprites (e.g. opening/closing doors in credits).
//    0-------1
//   /         \
//  2-----------3
// uc_orig: OS_buffer_add_sprite_arbitrary (fallen/outro/os.cpp)
void OS_buffer_add_sprite_arbitrary(
    OS_Buffer* ob,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    float x4, float y4,
    float u1, float v0,
    float u2, float v2,
    float u3, float v3,
    float u4, float v4,
    float z,
    ULONG colour,
    ULONG specular)
{
    SLONG i;
    float x, y, u, v;
    OS_Flert* of;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        switch (i) {
        case 0: x = x1; y = y1; u = u1; v = v0; break;
        case 1: x = x2; y = y2; u = u2; v = v2; break;
        case 2: x = x3; y = y3; u = u3; v = v3; break;
        case 3: x = x4; y = y4; u = u4; v = v4; break;
        default: ASSERT(0); break;
        }

        x *= OS_screen_width;
        y *= OS_screen_height;

        of->sx       = x;
        of->sy       = y;
        of->sz       = z;
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = u;
        of->tv1      = v;
        of->tu2      = u;
        of->tv2      = v;
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// Adds a screen-space line with given 3D-projected endpoints (in real screen pixels)
// as a quad of two triangles. The line width is a percentage of screen width.
// uc_orig: OS_buffer_add_line_3d (fallen/outro/os.cpp)
void OS_buffer_add_line_3d(
    OS_Buffer* ob,
    float X1, float Y1,
    float X2, float Y2,
    float width,
    float u1, float v1,
    float u2, float v2,
    float z1, float z2,
    ULONG colour,
    ULONG specular)
{
    SLONG i;
    OS_Flert* of;
    float dx, dy, len, overlen;
    float x, y;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    dx = X2 - X1;
    dy = Y2 - Y1;

    len = qdist2(fabs(dx), fabs(dy));
    overlen = width * OS_screen_width / len;

    dx *= overlen;
    dy *= overlen;

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        x = 0.0F;
        y = 0.0F;

        if (i & 1) { x += -dy; y += +dx; }
        else        { x -= -dy; y -= +dx; }

        if (i & 2) { x += X1;  y += Y1;  }
        else        { x += X2;  y += Y2;  }

        of->sx       = x;
        of->sy       = y;
        of->sz       = ((i & 2) ? z1 : z2);
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = ((i & 1) ? u1 : u2);
        of->tv1      = ((i & 2) ? v1 : v2);
        of->tu2      = ((i & 1) ? u1 : u2);
        of->tv2      = ((i & 2) ? v1 : v2);
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// Adds a normalised-coordinate 2D line as a quad.
// uc_orig: OS_buffer_add_line_2d (fallen/outro/os.cpp)
void OS_buffer_add_line_2d(
    OS_Buffer* ob,
    float x1, float y1,
    float x2, float y2,
    float width,
    float u1, float v1,
    float u2, float v2,
    float z,
    ULONG colour,
    ULONG specular)
{
    SLONG i;
    OS_Flert* of;
    float dx, dy, len, overlen;
    float x, y;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    x1 *= OS_screen_width;
    y1 *= OS_screen_height;
    x2 *= OS_screen_width;
    y2 *= OS_screen_height;

    dx = x2 - x1;
    dy = y2 - y1;

    len = qdist2(fabs(dx), fabs(dy));
    overlen = width * OS_screen_width / len;

    dx *= overlen;
    dy *= overlen;

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        x = 0.0F;
        y = 0.0F;

        if (i & 1) { x += -dy; y += +dx; }
        else        { x -= -dy; y -= +dx; }

        if (i & 2) { x += x1; y += y1; }
        else        { x += x2; y += y2; }

        of->sx       = x;
        of->sy       = y;
        of->sz       = z;
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = ((i & 1) ? u1 : u2);
        of->tv1      = ((i & 2) ? v1 : v2);
        of->tu2      = ((i & 1) ? u1 : u2);
        of->tv2      = ((i & 2) ? v1 : v2);
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// Submits the buffer for rendering then returns it to the free pool.
// Applies texture bindings and render state overrides based on the draw flags.
// uc_orig: OS_buffer_draw (fallen/outro/os.cpp)
void OS_buffer_draw(OS_Buffer* ob, OS_Texture* ot1, OS_Texture* ot2, ULONG draw)
{
    LPDIRECT3DDEVICE3 d3d = OS_frame.GetD3DDevice();

    if (ob->num_flerts == 0) {
        OS_buffer_give(ob);
        return;
    }

    if (ot1 == NULL) {
        draw |= OS_DRAW_TEX_NONE;
    } else {
        d3d->SetTexture(0, ot1->ddtx);
    }

    if (ot2) {
        d3d->SetTexture(1, ot2->ddtx);
    }

    if (draw != OS_DRAW_NORMAL) {
        OS_change_renderstate_for_type(draw);
    }

    {
        ULONG num_passes;
        if (d3d->ValidateDevice(&num_passes) != D3D_OK) {
            OS_string("Validation failed: draw = 0x%x\n", draw);
        }
    }

    d3d->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        OS_FLERT_FORMAT,
        ob->flert,
        ob->num_flerts,
        ob->index,
        ob->num_indices,
        D3DDP_DONOTUPDATEEXTENTS);

    if (draw != OS_DRAW_NORMAL) {
        OS_undo_renderstate_type_changes();
    }

    OS_buffer_give(ob);
}

// ========================================================
// SOUND
// ========================================================

// MIDAS-based sound functions are fully commented out in the original.
// The active implementation uses the main game's MFX/MUSIC system.

// uc_orig: OS_sound_init (fallen/outro/os.cpp)
void OS_sound_init()
{
    // Stub — MIDAS implementation was removed.
}

// uc_orig: OS_sound_start (fallen/outro/os.cpp)
void OS_sound_start(void)
{
    // Stub — MIDAS implementation was removed.
}

// uc_orig: OS_sound_volume (fallen/outro/os.cpp)
void OS_sound_volume(float vol)
{
    // Stub — MIDAS implementation was removed.
}

// Selects a random music track and initialises the D3D/DD pointers from the main
// game's display object. Called once at outro startup.
// uc_orig: OS_hack (fallen/outro/os.cpp)
void OS_hack(void)
{
    MUSIC_mode(0);
    MUSIC_mode_process();

    switch (rand() % 5) {
    case 0: sound = S_TUNE_COMBAT_TRAINING;   break;
    case 1: sound = S_TUNE_DRIVING_TRAINING;  break;
    case 2: sound = S_TUNE_CLUB_START;        break;
    case 3: sound = S_TUNE_CLUB_END;          break;
    case 4: sound = S_TUNE_ASSAULT_TRAINING;  break;
    }

    sound = S_CREDITS_LOOP;

    OS_frame.direct_draw = the_display.lp_DD4;
    OS_frame.direct_3d   = the_display.lp_D3D_Device;

    // Enumerate texture pixel formats for this device.
    {
        int i;
        OS_Tformat* otf;

        OS_frame.GetD3DDevice()->EnumTextureFormats(OS_texture_enumerate_pixel_formats, NULL);

        for (i = 0; i < OS_TEXTURE_FORMAT_NUMBER; i++) {
            otf = &OS_tformat[i];

            if (i == OS_TEXTURE_FORMAT_8) {
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
    }

    // Get screen dimensions from the main game's display.
    {
        extern SLONG RealDisplayWidth;
        extern SLONG RealDisplayHeight;

        OS_screen_width  = float(RealDisplayWidth);
        OS_screen_height = float(RealDisplayHeight);
    }

    OS_pipeline_calculate();

    OS_game_start_tick_count = GetTickCount();

    KEY_on[KEY_ESCAPE] = 0;

    MAIN_main();

    // DO NO CLEANUP!

    MFX_stop(0, sound);
}

// uc_orig: OS_sound_loop_start (fallen/outro/os.cpp)
void OS_sound_loop_start()
{
    MFX_play_stereo(0, sound, MFX_LOOPED);
}

// uc_orig: OS_sound_loop_process (fallen/outro/os.cpp)
void OS_sound_loop_process()
{
    MUSIC_mode_process();
    MFX_update();
}
