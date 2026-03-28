// Display class implementation and related standalone functions.
// Manages the DirectDraw/Direct3D device lifecycle, surfaces, viewport,
// mode changes, and pixel-level framebuffer access.

#include <windows.h>
#include <string.h>
#include <stdio.h>
// MFStdLib before gd_display.h: MFStdLib.h declares 'extern SLONG DisplayWidth/Height'
// which would conflict with '#define DisplayWidth 640' from gd_display.h.
#include "engine/platform/uc_common.h"                  // ASSERT, InitStruct
#include <mmstream.h>  // IMultiMediaStream, IMediaStream
#include <amstream.h>  // IAMMultiMediaStream, CLSID_AMMultiMediaStream
#include <ddstream.h>  // IDirectDrawMediaStream, IDirectDrawStreamSample

#include "engine/graphics/graphics_engine/backend_directx6/gd_display.h"
#include "engine/graphics/graphics_engine/backend_directx6/display_globals.h"
#include "engine/graphics/graphics_engine/backend_directx6/dd_manager.h"
#include "engine/graphics/graphics_engine/backend_directx6/dd_manager_globals.h"
#include "engine/graphics/graphics_engine/backend_directx6/display_macros.h"   // dd_error, d3d_error macros
#include "engine/io/env.h"             // ENV_get_value_number, ENV_set_value_number
#include "assets/formats/tga.h"                // OpenTGAClump, CloseTGAClump
#include "engine/core/memory.h"               // MemAlloc, MemFree
#include "engine/io/file.h"            // FileOpen, FileRead, FileSeek, FileClose

#include "engine/graphics/graphics_engine/backend_directx6/vertex_buffer.h"
#include "engine/graphics/graphics_engine/graphics_engine.h"

extern CBYTE DATA_DIR[];

// uc_orig: calculate_mask_and_shift (fallen/DDLibrary/Source/GDisplay.cpp)
static void calculate_mask_and_shift(ULONG bitmask, SLONG* mask, SLONG* shift);

// Callbacks registered by game code.
static GEPreFlipCallback s_pre_flip_callback = nullptr;
static GEModeChangeCallback s_mode_change_callback = nullptr;

void ge_set_pre_flip_callback(GEPreFlipCallback callback)
{
    s_pre_flip_callback = callback;
}

void ge_set_mode_change_callback(GEModeChangeCallback callback)
{
    s_mode_change_callback = callback;
}


// RGB 5-6-5 and 5-5-5 pixel format structs used by LoadBackImage.
// uc_orig: RGB_565 (fallen/DDLibrary/Source/GDisplay.cpp)
struct RGB_565 {
    UWORD R : 5,
        G : 6,
        B : 5;
};
// uc_orig: RGB_555 (fallen/DDLibrary/Source/GDisplay.cpp)
struct RGB_555 {
    UWORD R : 5,
        G : 5,
        B : 5;
};

// ---------------------------------------------------------------------------
// Standalone display helper functions
// ---------------------------------------------------------------------------

// uc_orig: InitBackImage (fallen/DDLibrary/Source/GDisplay.cpp)
// Loads a 640x480x24 background image from the data directory and uploads it to the display.
void InitBackImage(CBYTE* name)
{
    MFFileHandle image_file;
    SLONG height;
    CBYTE fname[200];

    sprintf(fname, "%sdata\\%s", DATA_DIR, name);

    if (image_mem == 0) {
        image_mem = (UBYTE*)MemAlloc(640 * 480 * 3);
    }

    if (image_mem) {
        image_file = FileOpen(fname);
        if (image_file != nullptr) {
            FileSeek(image_file, SEEK_MODE_BEGINNING, 18);
            image = image_mem + (640 * 479 * 3);
            for (height = 480; height; height--, image -= (640 * 3)) {
                FileRead(image_file, image, 640 * 3);
            }
            FileClose(image_file);
        }
        the_display.create_background_surface(image_mem);
    }
}

// uc_orig: UseBackSurface (fallen/DDLibrary/Source/GDisplay.cpp)
void UseBackSurface(LPDIRECTDRAWSURFACE4 use)
{
    the_display.use_this_background_surface(use);
}

// uc_orig: ResetBackImage (fallen/DDLibrary/Source/GDisplay.cpp)
void ResetBackImage(void)
{
    the_display.destroy_background_surface();
    if (image_mem) {
        MemFree(image_mem);
        image_mem = 0;
    }
}

// uc_orig: ShowBackImage (fallen/DDLibrary/Source/GDisplay.cpp)
void ShowBackImage(bool b3DInFrame)
{
    the_display.blit_background_surface(b3DInFrame);
}

// uc_orig: OpenDisplay (fallen/DDLibrary/Source/GDisplay.cpp)
// Opens DirectDraw/D3D, chooses the display mode based on VideoRes from the .env file,
// and calls SetDisplay to bring up the chosen resolution.
SLONG OpenDisplay(ULONG width, ULONG height, ULONG depth, ULONG flags)
{
    HRESULT result;

    result = the_manager.Init();
    if (FAILED(result)) {
        return -1;
    }

    extern HINSTANCE hGlobalThisInst;

    VideoRes = ENV_get_value_number("video_res", -1, "Render");

    VideoRes = max(min(VideoRes, 4), 0);

    depth = 32;
    switch (VideoRes) {
    case 0:
        width = 320;
        height = 240;
        break;
    case 1:
        width = 512;
        height = 384;
        break;
    case 2:
        width = 640;
        height = 480;
        break;
    case 3:
        width = 800;
        height = 600;
        break;
    case 4:
        width = 1024;
        height = 768;
        break;
    }

    if (flags & FLAGS_USE_3D)
        the_display.Use3DOn();

    if (flags & FLAGS_USE_WORKSCREEN)
        the_display.UseWorkOn();

    the_display.FullScreenOff();

    result = SetDisplay(width, height, depth);

    return result;
}

// uc_orig: CloseDisplay (fallen/DDLibrary/Source/GDisplay.cpp)
SLONG CloseDisplay(void)
{
    the_display.Fini();
    the_manager.Fini();

    return 1;
}

// uc_orig: SetDisplay (fallen/DDLibrary/Source/GDisplay.cpp)
// Changes the display mode and notifies the polygon engine of the new scaling factors.
SLONG SetDisplay(ULONG width, ULONG height, ULONG depth)
{
    HRESULT result;

    RealDisplayWidth = width;
    RealDisplayHeight = height;
    DisplayBPP = depth;
    result = the_display.ChangeMode(width, height, depth, 0);
    if (FAILED(result))
        return -1;

    if (s_mode_change_callback) {
        s_mode_change_callback(width, height);
    }

    return 0;
}

// uc_orig: ClearDisplay (fallen/DDLibrary/Source/GDisplay.cpp)
SLONG ClearDisplay(UBYTE r, UBYTE g, UBYTE b)
{
    DDBLTFX dd_bltfx;

    dd_bltfx.dwSize = sizeof(dd_bltfx);
    dd_bltfx.dwFillColor = 0;

    the_display.lp_DD_FrontSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &dd_bltfx);

    return 0;
}

// uc_orig: LoadBackImage (fallen/DDLibrary/Source/GDisplay.cpp)
// Copies image_data (raw 16-bit RGB) into the back buffer (asserts and aborts immediately).
void LoadBackImage(UBYTE* image_data)
{
    ASSERT(0);

    UWORD pixel,
        *surface_mem;
    SLONG try_count,
        height,
        pitch,
        width;
    DDSURFACEDESC2 dd_sd;
    HRESULT result;

    {
        InitStruct(dd_sd);
        try_count = 0;
    do_the_lock:
        result = the_display.lp_DD_BackSurface->Lock(NULL, &dd_sd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);
        switch (result) {
        case DD_OK:
            pitch = dd_sd.lPitch >> 1;
            surface_mem = (UWORD*)dd_sd.lpSurface;
            for (height = 0; (unsigned)height < dd_sd.dwHeight; height++, surface_mem += pitch) {
                for (width = 0; (unsigned)width < dd_sd.dwWidth; width++) {
                    pixel = the_display.GetFormattedPixel(*(image_data + 2), *(image_data + 1), *(image_data + 0));
                    *(surface_mem + width) = pixel;
                    image_data += 3;
                }
            }
            try_count = 0;
        do_the_unlock:
            result = the_display.lp_DD_BackSurface->Unlock(NULL);
            switch (result) {
            case DDERR_SURFACELOST:
                try_count++;
                if (try_count < 10) {
                    the_display.Restore();
                    goto do_the_unlock;
                }
                break;
            }
            break;
        case DDERR_SURFACELOST:
            try_count++;
            if (try_count < 10) {
                the_display.Restore();
                goto do_the_lock;
            }
            break;
        default:
            try_count++;
            if (try_count < 10)
                goto do_the_lock;
        }
    }
}

// uc_orig: PlayQuickMovie (fallen/DDLibrary/Source/GDisplay.cpp)
void PlayQuickMovie(SLONG type, SLONG language_ignored, bool bIgnored)
{
    // Stub — Bink video subsystem removed.
}

// ---------------------------------------------------------------------------
// Display class constructor / destructor
// ---------------------------------------------------------------------------

// uc_orig: Display::Display (fallen/DDLibrary/Source/GDisplay.cpp)
Display::Display()
{
    DisplayFlags = 0;
    CurrDevice = NULL;
    CurrDriver = NULL;
    CurrMode = NULL;

    CreateZBufferOn();

    lp_DD_FrontSurface = NULL;
    lp_DD_BackSurface = NULL;
    lp_DD_WorkSurface = NULL;
    lp_DD_ZBuffer = NULL;
    lp_D3D_Black = NULL;
    lp_D3D_White = NULL;
    lp_D3D_User = NULL;
    lp_DD_GammaControl = NULL;

    BackColour = BK_COL_BLACK;
    TextureList = NULL;
}

// uc_orig: Display::~Display (fallen/DDLibrary/Source/GDisplay.cpp)
Display::~Display()
{
    Fini();
}

// ---------------------------------------------------------------------------
// Initialisation / finalisation
// ---------------------------------------------------------------------------

// uc_orig: Display::Init (fallen/DDLibrary/Source/GDisplay.cpp)
// Creates the full DirectDraw/D3D surface chain. Safe to call multiple times — skips
// if already initialised. Plays the intro FMV on first call.
HRESULT Display::Init(void)
{
    HRESULT result;
    static bool run_fmv = false;

    if (!IsInitialised()) {
        if ((!hDDLibWindow) || (!IsWindow(hDDLibWindow))) {
            result = DDERR_GENERIC;
            return result;
        }

        result = InitInterfaces();
        if (FAILED(result))
            goto cleanup;

        result = InitWindow();
        if (FAILED(result))
            goto cleanup;

        result = InitFullscreenMode();
        if (FAILED(result))
            goto cleanup;

        result = InitFront();
        if (FAILED(result))
            goto cleanup;

        result = InitBack();
        if (FAILED(result))
            goto cleanup;

        if (background_image_mem) {
            create_background_surface(background_image_mem);
        }

        InitOn();

        if (!run_fmv) {
            RunFMV();
            run_fmv = true;
        }

        return DD_OK;

    cleanup:
        Fini();
        return DDERR_GENERIC;
    }
    return DD_OK;
}

// uc_orig: Display::Fini (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::Fini(void)
{
    toGDI();

    if (lp_DD_Background) {
        lp_DD_Background->Release();
        lp_DD_Background = NULL;
    }
    FiniBack();
    FiniFront();
    FiniFullscreenMode();
    FiniWindow();
    FiniInterfaces();

    InitOff();
    return DD_OK;
}

// uc_orig: Display::GenerateDefaults (fallen/DDLibrary/Source/GDisplay.cpp)
// Picks a default driver/mode/device. Prefers the requested resolution;
// falls back to 640x480 if no driver validates the current mode.
HRESULT Display::GenerateDefaults(void)
{
    D3DDeviceInfo* new_device;
    DDDriverInfo* new_driver;
    DDModeInfo* new_mode;
    HRESULT result;

    new_driver = ValidateDriver(NULL);
    if (!new_driver) {
        result = DDERR_GENERIC;
        return result;
    }

    if (IsFullScreen()) {
        if (
            !GetFullscreenMode(
                new_driver,
                NULL,
                DEFAULT_WIDTH,
                DEFAULT_HEIGHT,
                DEFAULT_DEPTH,
                0,
                &new_mode,
                &new_device)) {
            result = DDERR_GENERIC;
            return result;
        }
    } else {
        if (
            !GetDesktopMode(
                new_driver,
                NULL,
                &new_mode,
                &new_device)) {
            result = DDERR_GENERIC;
            return result;
        }
    }

    CurrDriver = new_driver;
    CurrMode = new_mode;
    CurrDevice = new_device;

    return DD_OK;
}

// uc_orig: Display::InitInterfaces (fallen/DDLibrary/Source/GDisplay.cpp)
// Creates the IDirectDraw, IDirectDraw4, and IDirect3D3 COM interfaces.
// Displays an error box if DirectX 6.0 or later is not installed.
HRESULT Display::InitInterfaces(void)
{
    GUID* the_guid;
    HRESULT result;

    if (!CurrDriver) {
        CurrDriver = ValidateDriver(NULL);
        if (!CurrDriver) {
            result = DDERR_GENERIC;
            return result;
        }
    }

    the_guid = CurrDriver->GetGuid();

    result = DirectDrawCreate(the_guid, &lp_DD, NULL);
    if (FAILED(result)) {
        goto cleanup;
    }

    result = lp_DD->QueryInterface((REFIID)IID_IDirectDraw4, (void**)&lp_DD4);
    if (FAILED(result)) {
        MessageBox(hDDLibWindow, TEXT("Need DirectX 6.0 or greater to run"), TEXT("Error"), MB_OK);
        goto cleanup;
    }

    result = lp_DD4->QueryInterface((REFIID)IID_IDirect3D3, (void**)&lp_D3D);
    if (FAILED(result)) {
        goto cleanup;
    }

    TurnValidInterfaceOn();

    return DD_OK;

cleanup:
    FiniInterfaces();

    return result;
}

// uc_orig: Display::FiniInterfaces (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::FiniInterfaces(void)
{
    TurnValidInterfaceOff();

    if (lp_D3D) {
        lp_D3D->Release();
        lp_D3D = NULL;
    }

    if (lp_DD4) {
        lp_DD4->Release();
        lp_DD4 = NULL;
    }

    if (lp_DD) {
        lp_DD->Release();
        lp_DD = NULL;
    }

    return DD_OK;
}

// uc_orig: Display::InitWindow (fallen/DDLibrary/Source/GDisplay.cpp)
// Sets DirectDraw cooperative level (exclusive fullscreen or normal windowed),
// then initialises the vertex buffer pool.
HRESULT Display::InitWindow(void)
{
    HRESULT result;
    SLONG flags;

    if ((!hDDLibWindow) || (!IsWindow(hDDLibWindow))) {
        result = DDERR_GENERIC;
        return result;
    }

    if (IsFullScreen()) {
        flags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_FPUSETUP;
    } else {
        flags = DDSCL_NORMAL | DDSCL_FPUSETUP;
    }

    result = lp_DD4->SetCooperativeLevel(hDDLibWindow, flags);
    if (FAILED(result)) {
        return result;
    }

    VB_Init();
    TheVPool->Create(lp_D3D, !CurrDevice->IsHardware());

    return DD_OK;
}

// uc_orig: Display::FiniWindow (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::FiniWindow(void)
{
    HRESULT result;

    VB_Term();

    if (lp_DD4) {
        result = lp_DD4->SetCooperativeLevel(hDDLibWindow, DDSCL_NORMAL | DDSCL_FPUSETUP);
        if (FAILED(result)) {
            return result;
        }
    }

    return DD_OK;
}

// uc_orig: Display::RunFMV (fallen/DDLibrary/Source/GDisplay.cpp)
// Plays the intro FMV sequence unless suppressed by the "play_movie" env key.
void Display::RunFMV()
{
    if (!hDDLibWindow)
        return;

    if (!ENV_get_value_number("play_movie", 1, "Movie"))
        return;

    PlayQuickMovie(0, 0, UC_TRUE);
}

// uc_orig: Display::RunCutscene (fallen/DDLibrary/Source/GDisplay.cpp)
void Display::RunCutscene(int which, int language, bool bAllowButtonsToExit)
{
    PlayQuickMovie(which, language, bAllowButtonsToExit);
}

// uc_orig: Display::InitFullscreenMode (fallen/DDLibrary/Source/GDisplay.cpp)
// Sets the display mode. In windowed mode it resizes the window; in fullscreen mode
// it calls SetDisplayMode and falls back to 640x480x16 if the requested mode fails.
HRESULT Display::InitFullscreenMode(void)
{
    SLONG flags = 0,
          style,
          w, h, bpp, refresh;
    HRESULT result;

    if ((!CurrMode) || (!lp_DD4)) {
        result = DDERR_GENERIC;
        return result;
    }

    if (!IsFullScreen()) {
        style = GetWindowStyle(hDDLibWindow);
        style &= ~WS_POPUP;
        style |= WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX;
        SetWindowLong(hDDLibWindow, GWL_STYLE, style);

        DisplayRect.left = 0;
        DisplayRect.top = 0;
        DisplayRect.right = RealDisplayWidth;
        DisplayRect.bottom = RealDisplayHeight;

        AdjustWindowRectEx(
            &DisplayRect,
            GetWindowStyle(hDDLibWindow),
            GetMenu(hDDLibWindow) != NULL,
            GetWindowExStyle(hDDLibWindow));
        SetWindowPos(
            hDDLibWindow,
            NULL,
            0, 0,
            DisplayRect.right - DisplayRect.left,
            DisplayRect.bottom - DisplayRect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);

        GetClientRect(hDDLibWindow, &the_display.DisplayRect);
        ClientToScreen(hDDLibWindow, (LPPOINT)&the_display.DisplayRect);
        ClientToScreen(hDDLibWindow, (LPPOINT)&the_display.DisplayRect + 1);

        TurnValidFullscreenOn();
        return DD_OK;
    }

    hDDLibMenu = GetMenu(hDDLibWindow);
    hDDLibStyle = GetWindowLong(hDDLibWindow, GWL_STYLE);
    hDDLibStyleEx = GetWindowLong(hDDLibWindow, GWL_EXSTYLE);

    CurrMode->GetMode(&w, &h, &bpp, &refresh);

    if ((w == 320) && (h == 200) && (bpp == 8)) {
        flags = DDSDM_STANDARDVGAMODE;
    }

    result = lp_DD4->SetDisplayMode(w, h, bpp, refresh, flags);
    if (SUCCEEDED(result)) {
        DisplayRect.left = 0;
        DisplayRect.top = 0;
        DisplayRect.right = w;
        DisplayRect.bottom = h;

        TurnValidFullscreenOn();
        return result;
    }

    if ((w != DEFAULT_WIDTH || h != DEFAULT_HEIGHT)) {
        w = DEFAULT_WIDTH;
        h = DEFAULT_HEIGHT;

        CurrMode = ValidateMode(CurrDriver, w, h, bpp, 0, CurrDevice);
        if (CurrMode) {
            result = lp_DD4->SetDisplayMode(w, h, bpp, 0, 0);
            if (SUCCEEDED(result)) {
                DisplayRect.left = 0;
                DisplayRect.top = 0;
                DisplayRect.right = w;
                DisplayRect.bottom = h;

                TurnValidFullscreenOn();
                return result;
            }
        }
    }

    if (bpp != DEFAULT_DEPTH) {
        bpp = DEFAULT_DEPTH;

        CurrMode = ValidateMode(CurrDriver, w, h, bpp, 0, CurrDevice);
        if (CurrMode) {
            result = lp_DD4->SetDisplayMode(w, h, bpp, 0, 0);
            if (SUCCEEDED(result)) {
                DisplayRect.left = 0;
                DisplayRect.top = 0;
                DisplayRect.right = w;
                DisplayRect.bottom = h;

                TurnValidFullscreenOn();
                return result;
            }
        }
    }

    return result;
}

// uc_orig: Display::FiniFullscreenMode (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::FiniFullscreenMode(void)
{
    TurnValidFullscreenOff();

    if (lp_DD4)
        lp_DD4->RestoreDisplayMode();

    return DD_OK;
}

// uc_orig: calculate_mask_and_shift (fallen/DDLibrary/Source/GDisplay.cpp)
// Given a colour bitmask, computes the right-shift (mask) and left-shift (shift) needed
// to pack an 8-bit component into that position. Formula: pixel = ((c >> mask) << shift).
static void calculate_mask_and_shift(
    ULONG bitmask,
    SLONG* mask,
    SLONG* shift)
{
    SLONG i;
    SLONG b;
    SLONG num_bits = 0;
    SLONG first_bit = -1;

    for (i = 0, b = 1; i < 32; i++, b <<= 1) {
        if (bitmask & b) {
            num_bits += 1;

            if (first_bit == -1) {
                first_bit = i;
            }
        }
    }

    if (first_bit == -1 || num_bits == 0) {
        *mask = 0;
        *shift = 0;

        return;
    }

    *mask = 8 - num_bits;
    *shift = first_bit;

    if (*mask < 0) {
        *shift -= *mask;
        *mask = 0;
    }
}

// uc_orig: Display::InitFront (fallen/DDLibrary/Source/GDisplay.cpp)
// Creates the primary surface (with back buffer in fullscreen hardware mode).
// Also sets up gamma correction and reads the pixel format masks/shifts.
HRESULT Display::InitFront(void)
{
    DDSURFACEDESC2 dd_sd;
    HRESULT result;

    if ((!CurrMode) || (!lp_DD4)) {
        result = DDERR_GENERIC;
        return result;
    }

    InitStruct(dd_sd);

    if (IsFullScreen() && CurrDevice->IsHardware()) {
        dd_sd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        dd_sd.ddsCaps.dwCaps = DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
        dd_sd.dwBackBufferCount = 1;
    } else {
        dd_sd.dwFlags = DDSD_CAPS;
        dd_sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    }

    result = lp_DD4->CreateSurface(&dd_sd, &lp_DD_FrontSurface, NULL);
    if (FAILED(result)) {
        return result;
    }

    result = lp_DD_FrontSurface->QueryInterface(IID_IDirectDrawGammaControl, (void**)&lp_DD_GammaControl);
    if (FAILED(result)) {
        lp_DD_GammaControl = NULL;
    } else {
        int black, white;

        GetGamma(&black, &white);
        SetGamma(black, white);
    }

    TurnValidFrontOn();

    InitStruct(dd_sd);

    lp_DD_FrontSurface->GetSurfaceDesc(&dd_sd);

    ASSERT(dd_sd.ddpfPixelFormat.dwFlags & DDPF_RGB);

    calculate_mask_and_shift(dd_sd.ddpfPixelFormat.dwRBitMask, &mask_red, &shift_red);
    calculate_mask_and_shift(dd_sd.ddpfPixelFormat.dwGBitMask, &mask_green, &shift_green);
    calculate_mask_and_shift(dd_sd.ddpfPixelFormat.dwBBitMask, &mask_blue, &shift_blue);

    return DD_OK;
}

// uc_orig: Display::FiniFront (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::FiniFront(void)
{
    TurnValidFrontOff();

    if (lp_DD_GammaControl) {
        lp_DD_GammaControl->Release();
        lp_DD_GammaControl = NULL;
    }

    if (lp_DD_FrontSurface) {
        lp_DD_FrontSurface->Release();
        lp_DD_FrontSurface = NULL;
    }

    return DD_OK;
}

// uc_orig: Display::InitBack (fallen/DDLibrary/Source/GDisplay.cpp)
// Creates back buffer, Z-buffer, D3D device, viewport, and work screen.
// In hardware fullscreen the back buffer is fetched from the flip chain.
HRESULT Display::InitBack(void)
{
    SLONG mem_type,
        w, h;
    DDSCAPS2 dd_scaps;
    DDSURFACEDESC2 dd_sd;
    HRESULT result;
    LPD3DDEVICEDESC device_desc;

    if (
        (!hDDLibWindow) || (!IsWindow(hDDLibWindow)) || (!CurrDevice) || (!CurrMode) || (!lp_DD4) || (!lp_D3D) || (!lp_DD_FrontSurface)) {
        result = DDERR_GENERIC;
        return result;
    }

    w = abs(DisplayRect.right - DisplayRect.left);
    h = abs(DisplayRect.bottom - DisplayRect.top);

    if (IsFullScreen() && CurrDevice->IsHardware()) {
        memset(&dd_scaps, 0, sizeof(dd_scaps));
        dd_scaps.dwCaps = DDSCAPS_BACKBUFFER;
        result = lp_DD_FrontSurface->GetAttachedSurface(&dd_scaps, &lp_DD_BackSurface);
        if (FAILED(result)) {
            return result;
        }
    } else {
        InitStruct(dd_sd);
        dd_sd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        dd_sd.dwWidth = w;
        dd_sd.dwHeight = h;
        dd_sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;

        if (!CurrDevice->IsHardware()) {
            dd_sd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        }

        result = lp_DD4->CreateSurface(&dd_sd, &lp_DD_BackSurface, NULL);
        if (FAILED(result)) {
            return result;
        }
    }

    if (IsUse3D()) {
        if (CurrDevice->IsHardware()) {
            device_desc = &(CurrDevice->d3dHalDesc);
            mem_type = DDSCAPS_VIDEOMEMORY;
        } else {
            device_desc = &(CurrDevice->d3dHelDesc);
            mem_type = DDSCAPS_SYSTEMMEMORY;
        }

        result = CurrDevice->LoadZFormats(lp_D3D);
        if (FAILED(result)) {
        }

        if (IsCreateZBuffer() && device_desc && device_desc->dwDeviceZBufferBitDepth) {
            InitStruct(dd_sd);
            dd_sd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            dd_sd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | mem_type;
            dd_sd.dwWidth = w;
            dd_sd.dwHeight = h;
            memcpy(&dd_sd.ddpfPixelFormat, CurrDevice->GetZFormat(), sizeof(DDPIXELFORMAT));
            result = lp_DD4->CreateSurface(&dd_sd, &lp_DD_ZBuffer, NULL);
            if (FAILED(result)) {
                dd_error(result);
            } else {
                result = lp_DD_BackSurface->AddAttachedSurface(lp_DD_ZBuffer);
                if (FAILED(result)) {

                    if (lp_DD_ZBuffer) {
                        lp_DD_ZBuffer->Release();
                        lp_DD_ZBuffer = NULL;
                    }
                }
            }
        }

        result = lp_D3D->CreateDevice(CurrDevice->guid, lp_DD_BackSurface, &lp_D3D_Device, NULL);
        if (FAILED(result)) {
            d3d_error(result);
            return result;
        }

        result = CurrDevice->LoadFormats(lp_D3D_Device);
        if (FAILED(result)) {
        }

        result = InitViewport();
        if (FAILED(result)) {
            return result;
        }

        CurrDevice->CheckCaps(lp_D3D_Device);
    }

    if (IsUseWork()) {
        if (CurrDevice->IsHardware()) {
            device_desc = &(CurrDevice->d3dHalDesc);
        } else {
            device_desc = &(CurrDevice->d3dHelDesc);
        }

        if (device_desc) {
            InitStruct(dd_sd);
            dd_sd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
            dd_sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
            dd_sd.dwWidth = w;
            dd_sd.dwHeight = h;
            result = lp_DD4->CreateSurface(&dd_sd, &lp_DD_WorkSurface, NULL);
            if (FAILED(result)) {
                dd_error(result);
                return result;
            }
            WorkScreenPixelWidth = w;
            WorkScreenWidth = w;
            WorkScreenHeight = h;
        }
    }

    TurnValidBackOn();

    return DD_OK;
}

// uc_orig: Display::FiniBack (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::FiniBack(void)
{
    TurnValidBackOff();

    if (IsUseWork()) {
        if (lp_DD_WorkSurface) {
            lp_DD_WorkSurface->Release();
            lp_DD_WorkSurface = NULL;
        }
    }

    if (IsUse3D()) {
        FiniViewport();

        if (lp_D3D_Device) {
            lp_D3D_Device->Release();
            lp_D3D_Device = NULL;
        }

        if (lp_DD_ZBuffer) {
            if (lp_DD_BackSurface)
                lp_DD_BackSurface->DeleteAttachedSurface(0L, lp_DD_ZBuffer);

            lp_DD_ZBuffer->Release();
            lp_DD_ZBuffer = NULL;
        }
    }

    if (lp_DD_BackSurface) {
        lp_DD_BackSurface->Release();
        lp_DD_BackSurface = NULL;
    }

    return DD_OK;
}

// uc_orig: Display::InitViewport (fallen/DDLibrary/Source/GDisplay.cpp)
// Creates the D3D viewport and attaches it to the device.
// Creates black, white, and user background materials.
HRESULT Display::InitViewport(void)
{
    D3DMATERIAL material;
    HRESULT result;

    if ((!lp_D3D) || (!lp_D3D_Device)) {
        result = DDERR_GENERIC;
        return result;
    }

    result = lp_D3D->CreateViewport(&lp_D3D_Viewport, NULL);
    if (FAILED(result)) {
        return result;
    }

    result = lp_D3D_Device->AddViewport(lp_D3D_Viewport);
    if (FAILED(result)) {
        lp_D3D_Viewport->Release();
        lp_D3D_Viewport = NULL;

        return result;
    }

    result = lp_D3D->CreateMaterial(&lp_D3D_Black, NULL);
    if (FAILED(result)) {
        return result;
    }

    InitStruct(material);
    material.diffuse.r = D3DVAL(0.0);
    material.diffuse.g = D3DVAL(0.0);
    material.diffuse.b = D3DVAL(0.0);
    material.diffuse.a = D3DVAL(1.0);
    material.ambient.r = D3DVAL(0.0);
    material.ambient.g = D3DVAL(0.0);
    material.ambient.b = D3DVAL(0.0);
    material.dwRampSize = 0;

    result = lp_D3D_Black->SetMaterial(&material);
    if (FAILED(result)) {
        lp_D3D_Black->Release();
        lp_D3D_Black = NULL;
        return result;
    }
    result = lp_D3D_Black->GetHandle(lp_D3D_Device, &black_handle);
    if (FAILED(result)) {
        lp_D3D_Black->Release();
        lp_D3D_Black = NULL;
        return result;
    }

    result = lp_D3D->CreateMaterial(&lp_D3D_White, NULL);
    if (FAILED(result)) {
        return result;
    }
    material.diffuse.r = D3DVAL(1.0);
    material.diffuse.g = D3DVAL(1.0);
    material.diffuse.b = D3DVAL(1.0);
    material.diffuse.a = D3DVAL(1.0);
    material.ambient.r = D3DVAL(1.0);
    material.ambient.g = D3DVAL(1.0);
    material.ambient.b = D3DVAL(1.0);
    material.dwRampSize = 0;

    result = lp_D3D_White->SetMaterial(&material);
    if (FAILED(result)) {
        lp_D3D_White->Release();
        lp_D3D_White = NULL;
        return result;
    }
    result = lp_D3D_White->GetHandle(lp_D3D_Device, &white_handle);
    if (FAILED(result)) {
        lp_D3D_White->Release();
        lp_D3D_White = NULL;
        return result;
    }

    SetUserColour(255, 150, 255);

    result = UpdateViewport();
    if (FAILED(result)) {
        lp_D3D_Device->DeleteViewport(lp_D3D_Viewport);
        lp_D3D_Viewport->Release();
        lp_D3D_Viewport = NULL;

        return result;
    }

    TurnValidViewportOn();

    return DD_OK;
}

// uc_orig: Display::SetUserColour (fallen/DDLibrary/Source/GDisplay.cpp)
// Creates (or recreates) the user-defined D3D background material with the given RGB colour.
void Display::SetUserColour(UBYTE red, UBYTE green, UBYTE blue)
{
    D3DMATERIAL material;
    HRESULT result;

    float r = (1.0F / 255.0F) * float(red);
    float g = (1.0F / 255.0F) * float(green);
    float b = (1.0F / 255.0F) * float(blue);

    if (lp_D3D_User) {
        lp_D3D_User->Release();
        lp_D3D_User = NULL;
        user_handle = NULL;
    }

    result = lp_D3D->CreateMaterial(&lp_D3D_User, NULL);

    ASSERT(!FAILED(result));

    InitStruct(material);
    material.diffuse.r = D3DVAL(r);
    material.diffuse.g = D3DVAL(g);
    material.diffuse.b = D3DVAL(b);
    material.diffuse.a = D3DVAL(1.0);
    material.ambient.r = D3DVAL(r);
    material.ambient.g = D3DVAL(g);
    material.ambient.b = D3DVAL(b);
    material.dwRampSize = 0;

    result = lp_D3D_User->SetMaterial(&material);

    ASSERT(!FAILED(result));

    result = lp_D3D_User->GetHandle(lp_D3D_Device, &user_handle);

    ASSERT(!FAILED(result));
}

// uc_orig: Display::FiniViewport (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::FiniViewport(void)
{
    TurnValidViewportOff();

    FreeLoadedTextures();

    if (lp_D3D_Black) {
        lp_D3D_Black->Release();
        lp_D3D_Black = NULL;
        black_handle = NULL;
    }

    if (lp_D3D_White) {
        lp_D3D_White->Release();
        lp_D3D_White = NULL;
        white_handle = NULL;
    }

    if (lp_D3D_User) {
        lp_D3D_User->Release();
        lp_D3D_User = NULL;
        user_handle = NULL;
    }

    if (lp_D3D_Viewport) {
        lp_D3D_Device->DeleteViewport(lp_D3D_Viewport);
        lp_D3D_Viewport->Release();
        lp_D3D_Viewport = NULL;
    }

    return DD_OK;
}

// uc_orig: Display::UpdateViewport (fallen/DDLibrary/Source/GDisplay.cpp)
// Resizes the D3D viewport to match DisplayRect, reloads textures, and sets the background colour.
HRESULT Display::UpdateViewport(void)
{
    SLONG s_w, s_h;
    HRESULT result;
    D3DVIEWPORT2 d3d_viewport;

    if ((!lp_D3D_Device) || (!lp_D3D_Viewport)) {
        result = DDERR_GENERIC;
        return result;
    }

    s_w = abs(DisplayRect.right - DisplayRect.left);
    s_h = abs(DisplayRect.bottom - DisplayRect.top);

    InitStruct(d3d_viewport);
    d3d_viewport.dwX = 0;
    d3d_viewport.dwY = 0;
    d3d_viewport.dwWidth = s_w;
    d3d_viewport.dwHeight = s_h;
    d3d_viewport.dvClipX = 0.0f;
    d3d_viewport.dvClipY = 0.0f;
    d3d_viewport.dvClipWidth = (float)s_w;
    d3d_viewport.dvClipHeight = (float)s_h;
    d3d_viewport.dvMinZ = 1.0f;
    d3d_viewport.dvMaxZ = 0.0f;

    result = lp_D3D_Viewport->SetViewport2(&d3d_viewport);
    if (FAILED(result)) {
        return result;
    }

    result = lp_D3D_Device->SetCurrentViewport(lp_D3D_Viewport);
    if (FAILED(result)) {
        return result;
    }

    ReloadTextures();

    ViewportRect.x1 = 0;
    ViewportRect.y1 = 0;
    ViewportRect.x2 = s_w;
    ViewportRect.y2 = s_h;

    switch (the_display.BackColour) {
    case BK_COL_NONE:
        break;
    case BK_COL_BLACK:
        return the_display.SetBlackBackground();
    case BK_COL_WHITE:
        return the_display.SetWhiteBackground();
    case BK_COL_USER:
        return the_display.SetUserBackground();
    }

    return DD_OK;
}

// uc_orig: Display::ChangeMode (fallen/DDLibrary/Source/GDisplay.cpp)
// Changes to a new display resolution. Destroys the back/front surfaces and recreates them
// at the new size. On failure, attempts to restore the previous mode.
HRESULT Display::ChangeMode(
    SLONG w,
    SLONG h,
    SLONG bpp,
    SLONG refresh)
{
    HRESULT result;
    DDDriverInfo* old_driver;
    DDModeInfo *new_mode,
        *old_mode;
    D3DDeviceInfo *new_device,
        *next_best_device,
        *old_device;

    if ((!hDDLibWindow) || (!IsWindow(hDDLibWindow))) {
        result = DDERR_GENERIC;
        return result;
    }

    if (!IsInitialised()) {
        result = GenerateDefaults();
        if (FAILED(result)) {
            result = DDERR_GENERIC;
            return result;
        }

        result = Init();
        if (FAILED(result)) {
            result = DDERR_GENERIC;
            return result;
        }
    }

    old_driver = CurrDriver;
    old_mode = CurrMode;
    old_device = CurrDevice;

    new_mode = old_driver->FindMode(w, h, bpp, 0, NULL);
    if (!new_mode) {
        result = DDERR_GENERIC;
        return result;
    }

    if (new_mode->ModeSupported(old_device)) {
        new_device = NULL;
    } else {
        new_device = old_driver->FindDeviceSupportsMode(&old_device->guid, new_mode, &next_best_device);
        if (!new_device) {
            if (!next_best_device) {
                result = DDERR_GENERIC;
                return result;
            }
            new_device = next_best_device;
        }
    }

    FiniBack();
    FiniFront();

    CurrMode = new_mode;
    if (new_device)
        CurrDevice = new_device;

    result = InitFullscreenMode();
    if (FAILED(result))
        return result;

    result = InitFront();
    if (FAILED(result)) {

        CurrMode = old_mode;
        CurrDevice = old_device;

        InitFullscreenMode();
        InitFront();
        InitBack();

        return result;
    }

    result = InitBack();
    if (FAILED(result)) {

        FiniFront();

        CurrMode = old_mode;
        CurrDevice = old_device;

        InitFullscreenMode();
        InitFront();
        InitBack();

        return result;
    }

    if (background_image_mem) {
        create_background_surface(background_image_mem);
    }

    DisplayChangedOn();

    return DD_OK;
}

// uc_orig: Display::Restore (fallen/DDLibrary/Source/GDisplay.cpp)
// Restores any lost DirectDraw surfaces (called after an alt-tab or mode change).
HRESULT Display::Restore(void)
{
    HRESULT result;

    if (!IsValid()) {
        result = DDERR_GENERIC;
        return result;
    }

    if (lp_DD_FrontSurface) {
        result = lp_DD_FrontSurface->IsLost();
        if (FAILED(result)) {
            result = lp_DD_FrontSurface->Restore();
            if (FAILED(result))
                return result;
        }
    }

    if (lp_DD_ZBuffer) {
        result = lp_DD_ZBuffer->IsLost();
        if (FAILED(result)) {
            result = lp_DD_ZBuffer->Restore();
            if (FAILED(result))
                return result;
        }
    }

    if (lp_DD_BackSurface) {
        result = lp_DD_BackSurface->IsLost();
        if (FAILED(result)) {
            result = lp_DD_BackSurface->Restore();
            if (FAILED(result))
                return result;
        }
    }

    return DD_OK;
}

// ---------------------------------------------------------------------------
// Texture list management
// ---------------------------------------------------------------------------

// uc_orig: Display::AddLoadedTexture (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::AddLoadedTexture(D3DTexture* the_texture)
{
    the_texture->NextTexture = TextureList;
    TextureList = the_texture;

    return DD_OK;
}

// uc_orig: Display::RemoveAllLoadedTextures (fallen/DDLibrary/Source/GDisplay.cpp)
void Display::RemoveAllLoadedTextures(void)
{
    TextureList = NULL;
}

// uc_orig: Display::FreeLoadedTextures (fallen/DDLibrary/Source/GDisplay.cpp)
// Destroys all D3DTexture objects in the TextureList. Bounded to prevent infinite loops.
HRESULT Display::FreeLoadedTextures(void)
{
    D3DTexture* current_texture;

    int iCountdown = 10000;

    current_texture = TextureList;
    while (current_texture) {
        D3DTexture* next_texture = current_texture->NextTexture;
        current_texture->Destroy();
        current_texture = next_texture;
        iCountdown--;
        if (iCountdown == 0) {
            ASSERT(UC_FALSE);
            break;
        }
    }

    return DD_OK;
}

// uc_orig: SetLastClumpfile (fallen/DDLibrary/Source/GDisplay.cpp)
// Saves the TGA clump filename so ReloadTextures() can reopen it after a device reset.
void SetLastClumpfile(char* file, size_t size)
{
    strcpy(clumpfile, file);
    clumpsize = size;
}

// uc_orig: Display::ReloadTextures (fallen/DDLibrary/Source/GDisplay.cpp)
// Reopens the TGA clump (if any) and calls Reload() on every tracked texture.
HRESULT Display::ReloadTextures(void)
{
    D3DTexture* current_texture;

    if (clumpfile[0]) {
        OpenTGAClump(clumpfile, clumpsize, true);
    }

    current_texture = TextureList;
    while (current_texture) {
        D3DTexture* next_texture = current_texture->NextTexture;
        current_texture->Reload();
        current_texture = next_texture;
    }

    if (clumpfile[0]) {
        CloseTGAClump();
    }

    return DD_OK;
}

// ---------------------------------------------------------------------------
// GDI / screen lock / pixel access
// ---------------------------------------------------------------------------

// uc_orig: Display::toGDI (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::toGDI(void)
{
    HRESULT result;

    if (lp_DD4) {
        result = lp_DD4->FlipToGDISurface();
        if (FAILED(result)) {
            return result;
        }
    }

    if ((hDDLibWindow) && (IsWindow(hDDLibWindow))) {
        DrawMenuBar(hDDLibWindow);
        RedrawWindow(hDDLibWindow, NULL, NULL, RDW_FRAME);
    }

    return DD_OK;
}

// uc_orig: Display::fromGDI (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::fromGDI(void)
{
    return DD_OK;
}

// uc_orig: Display::ShowWorkScreen (fallen/DDLibrary/Source/GDisplay.cpp)
HRESULT Display::ShowWorkScreen(void)
{
    return lp_DD_FrontSurface->Blt(&DisplayRect, lp_DD_WorkSurface, NULL, DDBLT_WAIT, NULL);
}

// uc_orig: Display::screen_lock (fallen/DDLibrary/Source/GDisplay.cpp)
// Locks the back surface for direct pixel access. Sets screen, screen_width/height/pitch/bbp.
// Returns NULL if the lock fails or the screen is already locked.
void* Display::screen_lock(void)
{
    if (DisplayFlags & DISPLAY_LOCKED) {
        // Already locked — don't lock twice.
    } else {
        DDSURFACEDESC2 ddsdesc;
        HRESULT ret;

        InitStruct(ddsdesc);

        ret = lp_DD_BackSurface->Lock(NULL, &ddsdesc, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);

        if (SUCCEEDED(ret)) {
            screen_width = ddsdesc.dwWidth;
            screen_height = ddsdesc.dwHeight;
            screen_pitch = ddsdesc.lPitch;
            screen_bbp = ddsdesc.ddpfPixelFormat.dwRGBBitCount;
            screen = (UBYTE*)ddsdesc.lpSurface;

            DisplayFlags |= DISPLAY_LOCKED;
        } else {
            d3d_error(ret);

            screen = NULL;
        }
    }

    return screen;
}

// uc_orig: Display::screen_unlock (fallen/DDLibrary/Source/GDisplay.cpp)
void Display::screen_unlock(void)
{
    if (DisplayFlags & DISPLAY_LOCKED) {
        lp_DD_BackSurface->Unlock(NULL);
    }

    screen = NULL;
    DisplayFlags &= ~DISPLAY_LOCKED;
}

// uc_orig: Display::PlotPixel (fallen/DDLibrary/Source/GDisplay.cpp)
// Writes a pixel to (x, y) in the locked back buffer. No-op if the screen is not locked.
void Display::PlotPixel(SLONG x, SLONG y, UBYTE red, UBYTE green, UBYTE blue)
{
    if (DisplayFlags & DISPLAY_LOCKED) {
        if (WITHIN(x, 0, screen_width - 1) && WITHIN(y, 0, screen_height - 1)) {
            if (CurrMode->GetBPP() == 16) {
                UWORD* dest;

                UWORD pixel = GetFormattedPixel(red, green, blue);
                SLONG index = x + x + y * screen_pitch;

                dest = (UWORD*)(&(screen[index]));
                dest[0] = pixel;
            } else {
                ULONG* dest;
                ULONG pixel = GetFormattedPixel(red, green, blue);
                SLONG index = x * 4 + y * screen_pitch;

                dest = (ULONG*)(screen + index);
                dest[0] = pixel;
            }
        }
    }
}

// uc_orig: Display::PlotFormattedPixel (fallen/DDLibrary/Source/GDisplay.cpp)
// Same as PlotPixel but takes an already-packed pixel value.
void Display::PlotFormattedPixel(SLONG x, SLONG y, ULONG colour)
{
    if (DisplayFlags & DISPLAY_LOCKED) {
        if (WITHIN(x, 0, screen_width - 1) && WITHIN(y, 0, screen_height - 1)) {
            if (CurrMode->GetBPP() == 16) {
                UWORD* dest;
                SLONG index = x + x + y * screen_pitch;

                dest = (UWORD*)(&(screen[index]));
                dest[0] = colour;
            } else {
                ULONG* dest;
                SLONG index = x * 4 + y * screen_pitch;
                dest = (ULONG*)(screen + index);
                dest[0] = colour;
            }
        }
    }
}

// uc_orig: Display::GetPixel (fallen/DDLibrary/Source/GDisplay.cpp)
// Reads back a pixel from the locked back buffer.
void Display::GetPixel(SLONG x, SLONG y, UBYTE* red, UBYTE* green, UBYTE* blue)
{
    ULONG colour;

    *red = 0;
    *green = 0;
    *blue = 0;

    if (DisplayFlags & DISPLAY_LOCKED) {
        if (WITHIN(x, 0, screen_width - 1) && WITHIN(y, 0, screen_height - 1)) {
            if (CurrMode->GetBPP() == 16) {
                UWORD* dest;
                SLONG index = x + x + y * screen_pitch;

                dest = (UWORD*)(&(screen[index]));
                colour = dest[0];
            } else {
                ULONG* dest;
                SLONG index = 4 * x + y * screen_pitch;
                dest = (ULONG*)(screen + index);
                colour = dest[0];
            }

            *red = ((colour >> shift_red) << mask_red) & 0xff;
            *green = ((colour >> shift_green) << mask_green) & 0xff;
            *blue = ((colour >> shift_blue) << mask_blue) & 0xff;
        }
    }
}

// uc_orig: Display::blit_back_buffer (fallen/DDLibrary/Source/GDisplay.cpp)
// Blits the back buffer to the front surface. In windowed mode, maps client coords to screen.
void Display::blit_back_buffer()
{
    POINT clientpos;
    RECT dest;

    HRESULT res;

    if (the_display.IsFullScreen()) {
        res = lp_DD_FrontSurface->Blt(NULL, lp_DD_BackSurface, NULL, DDBLT_WAIT, 0);
    } else {
        clientpos.x = 0;
        clientpos.y = 0;

        ClientToScreen(
            hDDLibWindow,
            &clientpos);

        GetClientRect(
            hDDLibWindow,
            &dest);

        dest.top += clientpos.y;
        dest.left += clientpos.x;
        dest.right += clientpos.x;
        dest.bottom += clientpos.y;

        res = lp_DD_FrontSurface->Blt(&dest, lp_DD_BackSurface, NULL, DDBLT_WAIT, 0);
    }

    if (FAILED(res)) {
        dd_error(res);
    }
}

// uc_orig: CopyBackground32 (fallen/DDLibrary/Source/GDisplay.cpp)
// Copies a 640x480x24 source image into a DirectDraw surface, scaling to fit.
// Uses bilinear-style nearest-neighbour line repetition to avoid redundant row copies.
void CopyBackground32(UBYTE* image_data, IDirectDrawSurface4* surface)
{
    DDSURFACEDESC2 mine;
    HRESULT res;

    InitStruct(mine);
    res = surface->Lock(NULL, &mine, DDLOCK_WAIT, NULL);
    if (FAILED(res))
        return;

    SLONG pitch = mine.lPitch >> 2;
    ULONG* mem = (ULONG*)mine.lpSurface;
    SLONG width;
    SLONG height;

    SLONG sdx = 65536 * 640 / mine.dwWidth;
    SLONG sdy = 65536 * 480 / mine.dwHeight;

    SLONG lsy = -1;
    SLONG sy = 0;
    ULONG* lmem = NULL;

    for (height = 0; (unsigned)height < mine.dwHeight; height++) {
        UBYTE* src = image_data + 640 * 3 * (sy >> 16);

        if ((sy >> 16) == lsy) {
            memcpy(mem, lmem, mine.dwWidth * 4);
        } else {
            SLONG sx = 0;

            for (width = 0; (unsigned)width < mine.dwWidth; width++) {
                UBYTE* pp = src + 3 * (sx >> 16);

                mem[width] = the_display.GetFormattedPixel(pp[2], pp[1], pp[0]);

                sx += sdx;
            }
        }

        lmem = mem;
        lsy = sy >> 16;

        mem += pitch;
        sy += sdy;
    }

    surface->Unlock(NULL);
}

// uc_orig: CopyBackground (fallen/DDLibrary/Source/GDisplay.cpp)
void CopyBackground(UBYTE* image_data, IDirectDrawSurface4* surface)
{
    CopyBackground32(image_data, surface);
}

// uc_orig: Display::Flip (fallen/DDLibrary/Source/GDisplay.cpp)
// End-of-frame: runs pre-flip hooks, draws the screensaver, then flips (hardware) or blits (windowed).
HRESULT Display::Flip(LPDIRECTDRAWSURFACE4 alt, SLONG flags)
{
    if (s_pre_flip_callback) {
        s_pre_flip_callback();
    }

    if (IsFullScreen() && CurrDevice->IsHardware()) {
        return lp_DD_FrontSurface->Flip(alt, flags);
    } else {
        return lp_DD_FrontSurface->Blt(&DisplayRect, lp_DD_BackSurface, NULL, DDBLT_WAIT, NULL);
    }
}

// uc_orig: Display::use_this_background_surface (fallen/DDLibrary/Source/GDisplay.cpp)
void Display::use_this_background_surface(LPDIRECTDRAWSURFACE4 this_one)
{
    lp_DD_Background_use_instead = this_one;
}

// uc_orig: Display::create_background_surface (fallen/DDLibrary/Source/GDisplay.cpp)
// Allocates a system-memory surface matching the back buffer pixel format and copies image_data into it.
void Display::create_background_surface(UBYTE* image_data)
{
    DDSURFACEDESC2 back;
    DDSURFACEDESC2 mine;

    background_image_mem = image_data;

    if (lp_DD_Background) {
        lp_DD_Background->Release();
        lp_DD_Background = NULL;
    }

    lp_DD_Background_use_instead = NULL;

    InitStruct(back);

    lp_DD_BackSurface->GetSurfaceDesc(&back);

    InitStruct(mine);

    mine.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    mine.dwWidth = back.dwWidth;
    mine.dwHeight = back.dwHeight;
    mine.ddpfPixelFormat = back.ddpfPixelFormat;
    mine.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    HRESULT result = lp_DD4->CreateSurface(&mine, &lp_DD_Background, NULL);

    if (FAILED(result)) {
        lp_DD_Background = NULL;
        background_image_mem = NULL;
        return;
    }

    CopyBackground(image_data, lp_DD_Background);

    return;
}

// uc_orig: Display::blit_background_surface (fallen/DDLibrary/Source/GDisplay.cpp)
// Blits the saved background surface (or the override surface) into the back buffer.
void Display::blit_background_surface(bool b3DInFrame)
{
    LPDIRECTDRAWSURFACE4 lpBG = NULL;

    if (lp_DD_Background_use_instead != NULL) {
        lpBG = lp_DD_Background_use_instead;
    } else if (lp_DD_Background != NULL) {
        lpBG = lp_DD_Background;
    } else {
        return;
    }

    HRESULT result;

    {
        result = lp_DD_BackSurface->Blt(NULL, lpBG, NULL, DDBLT_WAIT, 0);

        if (FAILED(result)) {
            dd_error(result);
        }
    }
}

// uc_orig: Display::destroy_background_surface (fallen/DDLibrary/Source/GDisplay.cpp)
void Display::destroy_background_surface()
{
    if (lp_DD_Background) {
        lp_DD_Background->Release();
        lp_DD_Background = NULL;
    }

    background_image_mem = NULL;
}

// ---------------------------------------------------------------------------
// Gamma control
// ---------------------------------------------------------------------------

// uc_orig: Display::IsGammaAvailable (fallen/DDLibrary/Source/GDisplay.cpp)
bool Display::IsGammaAvailable()
{
    return (lp_DD_GammaControl != NULL);
}

// uc_orig: Display::SetGamma (fallen/DDLibrary/Source/GDisplay.cpp)
// Sets the hardware gamma ramp. black=0, white=256 is the default identity ramp.
// Clamps and persists the settings to the Gamma env section.
void Display::SetGamma(int black, int white)
{
    if (!lp_DD_GammaControl)
        return;

    if (black < 0)
        black = 0;
    if (white > 256)
        white = 256;
    if (black > white - 1)
        black = white - 1;

    ENV_set_value_number("BlackPoint", black, "Gamma");
    ENV_set_value_number("WhitePoint", white, "Gamma");

    DDGAMMARAMP ramp;
    int diff = white - black;

    black <<= 8;

    for (int ii = 0; ii < 256; ii++) {
        ramp.red[ii] = black;
        ramp.green[ii] = black;
        ramp.blue[ii] = black;

        black += diff;
    }

    lp_DD_GammaControl->SetGammaRamp(0, &ramp);
}

// uc_orig: Display::GetGamma (fallen/DDLibrary/Source/GDisplay.cpp)
void Display::GetGamma(int* black, int* white)
{
    *black = ENV_get_value_number("BlackPoint", 0, "Gamma");
    *white = ENV_get_value_number("WhitePoint", 256, "Gamma");
}
