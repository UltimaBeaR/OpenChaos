#include <MFStdLib.h>
#include <tchar.h>
#include "engine/graphics/graphics_api/dd_manager.h"
#include "engine/graphics/graphics_api/dd_manager_globals.h"
#include "engine/graphics/graphics_api/gd_display.h"  // DEFAULT_WIDTH/HEIGHT/DEPTH
#include "engine/io/env.h"                              // ENV_get_value_number, ENV_set_value_number
#include "core/memory.h"                                // MemAlloc, MemFree

// Forward declaration: defined in engine/graphics/resources/d3d_texture.cpp.
// Avoids a graphics_api/ → graphics/resources/ include dependency.
// uc_orig: OS_calculate_mask_and_shift (fallen/DDLibrary/Source/D3DTexture.cpp)
extern void OS_calculate_mask_and_shift(ULONG bitmask, SLONG* mask, SLONG* shift);

// ---------------------------------------------------------------------------
// DirectDraw enumeration callbacks
// ---------------------------------------------------------------------------

// uc_orig: DriverEnumCallback (fallen/DDLibrary/Source/DDManager.cpp)
// Called by DirectDrawEnumerate() for each DirectDraw driver found.
// Creates a DDDriverInfo node and adds it to the global the_manager list.
BOOL WINAPI DriverEnumCallback(
    GUID FAR* lpGuid,
    LPTSTR    lpDesc,
    LPTSTR    lpName,
    LPVOID    lpExtra)
{
    CallbackInfo* the_info;
    DDDriverInfo* new_driver;
    HRESULT result;

    if (!lpExtra)
        return DDENUMRET_OK;

    the_info = (CallbackInfo*)lpExtra;

    new_driver = MFnew<DDDriverInfo>();
    if (!new_driver)
        return DDENUMRET_OK;

    result = new_driver->Create(lpGuid, lpName, lpDesc);
    if (FAILED(result))
        return DDENUMRET_OK;

    result = the_manager.AddDriver(new_driver);
    if (FAILED(result))
        return DDENUMRET_OK;

    the_info->Count++;

    return DDENUMRET_OK;
}

// uc_orig: ModeEnumCallback (fallen/DDLibrary/Source/DDManager.cpp)
// Called by IDirectDraw4::EnumDisplayModes() for each available video mode.
HRESULT WINAPI ModeEnumCallback(
    LPDDSURFACEDESC2 lpDDSurfDesc,
    LPVOID           lpExtra)
{
    CallbackInfo* the_info;
    DDDriverInfo* the_driver;
    DDModeInfo*   new_mode;
    HRESULT result;

    if (!lpExtra)
        return DDENUMRET_OK;

    the_info   = (CallbackInfo*)lpExtra;
    the_driver = (DDDriverInfo*)the_info->Extra;

    if (!the_driver)
        return DDENUMRET_OK;

    if (!lpDDSurfDesc)
        return DDENUMRET_CANCEL;

    ASSERT(lpDDSurfDesc->dwSize == sizeof(*lpDDSurfDesc));

    new_mode = MFnew<DDModeInfo>();
    if (!new_mode)
        return DDENUMRET_OK;

    new_mode->ddSurfDesc = *lpDDSurfDesc;

    result = the_driver->AddMode(new_mode);
    if (FAILED(result))
        return DDENUMRET_OK;

    the_info->Count++;

    return DDENUMRET_OK;
}

// uc_orig: DeviceEnumCallback (fallen/DDLibrary/Source/DDManager.cpp)
// Called by IDirect3D3::EnumDevices() for each Direct3D device on a driver.
HRESULT WINAPI DeviceEnumCallback(
    LPGUID        lpGuid,
    LPTSTR        lpName,
    LPTSTR        lpDesc,
    LPD3DDEVICEDESC lpHalDevice,
    LPD3DDEVICEDESC lpHelDevice,
    LPVOID        lpExtra)
{
    CallbackInfo*  the_info;
    D3DDeviceInfo* new_device;
    DDDriverInfo*  the_driver;
    HRESULT result;

    if (!lpExtra)
        return DDENUMRET_OK;

    the_info   = (CallbackInfo*)lpExtra;
    the_driver = (DDDriverInfo*)the_info->Extra;

    if (!the_driver)
        return DDENUMRET_OK;

    new_device = MFnew<D3DDeviceInfo>();
    if (!new_device)
        return DDENUMRET_OK;

    result = new_device->Create(lpGuid, lpName, lpDesc, lpHalDevice, lpHelDevice);
    if (FAILED(result))
        return DDENUMRET_OK;

    if (new_device->IsHardware()) {
        the_driver->hardware_guid = *lpGuid;

        if (lpHalDevice->dwDeviceRenderBitDepth & DDBD_16)
            the_driver->DriverFlags |= DD_DRIVER_RENDERS_TO_16BIT;
        if (lpHalDevice->dwDeviceRenderBitDepth & DDBD_32)
            the_driver->DriverFlags |= DD_DRIVER_RENDERS_TO_32BIT;
    }

    result = the_driver->AddDevice(new_device);
    if (FAILED(result))
        return DDENUMRET_OK;

    the_info->Count++;

    return DDENUMRET_OK;
}

// uc_orig: TextureFormatEnumCallback (fallen/DDLibrary/Source/DDManager.cpp)
// Called by IDirect3DDevice3::EnumTextureFormats() for each supported texture pixel format.
HRESULT WINAPI TextureFormatEnumCallback(
    LPDDPIXELFORMAT lpTextureFormat,
    LPVOID          lpExtra)
{
    CallbackInfo*  the_info;
    D3DDeviceInfo* the_device;
    DDModeInfo*    new_format;
    HRESULT result;

    if (!lpExtra)
        return DDENUMRET_OK;

    the_info   = (CallbackInfo*)lpExtra;
    the_device = (D3DDeviceInfo*)the_info->Extra;

    if (!the_device)
        return DDENUMRET_OK;

    if (!lpTextureFormat)
        return DDENUMRET_CANCEL;

    ASSERT(lpTextureFormat->dwSize == sizeof(*lpTextureFormat));

    new_format = MFnew<DDModeInfo>();
    if (!new_format)
        return DDENUMRET_OK;

    InitStruct(new_format->ddSurfDesc);
    new_format->ddSurfDesc.dwFlags           = DDSD_PIXELFORMAT;
    new_format->ddSurfDesc.ddpfPixelFormat   = *lpTextureFormat;

    result = the_device->AddFormat(new_format);
    if (FAILED(result)) {
        MFdelete(new_format);
        return DDENUMRET_OK;
    }

    the_info->Count++;

    return DDENUMRET_OK;
}

// uc_orig: ZFormatEnumCallback (fallen/DDLibrary/Source/DDManager.cpp)
// Called by IDirect3D3::EnumZBufferFormats(). Picks a 16-bit Z-buffer with no stencil.
HRESULT WINAPI ZFormatEnumCallback(LPDDPIXELFORMAT lpZFormat, LPVOID lpExtra)
{
    CallbackInfo*  the_info;
    D3DDeviceInfo* the_device;

    if (!lpExtra)
        return DDENUMRET_OK;

    the_info   = (CallbackInfo*)lpExtra;
    the_device = (D3DDeviceInfo*)the_info->Extra;

    if (!the_device)
        return DDENUMRET_OK;

    if (!lpZFormat)
        return DDENUMRET_CANCEL;

    // Fallback: accept first format in case no 16-bit format exists.
    if (!the_info->Count)
        the_device->SetZFormat(lpZFormat);

    // Prefer 16-bit Z-buffer with no stencil.
    if (lpZFormat->dwZBufferBitDepth == 16 && lpZFormat->dwStencilBitDepth == 0)
        the_device->SetZFormat(lpZFormat);

    the_info->Count++;

    return DDENUMRET_OK;
}

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------

// uc_orig: FlagsToBitDepth (fallen/DDLibrary/Source/DDManager.cpp)
// Converts a DDBD_* bit-depth flag to the actual bits-per-pixel value.
SLONG FlagsToBitDepth(SLONG flags)
{
    if (flags & DDBD_1)  return 1;
    if (flags & DDBD_2)  return 2L;
    if (flags & DDBD_4)  return 4L;
    if (flags & DDBD_8)  return 8L;
    if (flags & DDBD_16) return 16L;
    if (flags & DDBD_24) return 24L;
    if (flags & DDBD_32) return 32L;
    return 0L;
}

// uc_orig: FlagsToMask (fallen/DDLibrary/Source/DDManager.cpp)
// Returns a bitmask covering the number of bits indicated by a DDBD_* flag.
ULONG FlagsToMask(SLONG flags)
{
    if (flags & DDBD_1)  return 0x01;
    if (flags & DDBD_2)  return 0x03;
    if (flags & DDBD_4)  return 0x07;
    if (flags & DDBD_8)  return 0xFF;
    if (flags & DDBD_16) return 0xFFFF;
    if (flags & DDBD_24) return 0xFFFFFF;
    if (flags & DDBD_32) return 0xFFFFFFFF;
    return 0;
}

// uc_orig: BitDepthToFlags (fallen/DDLibrary/Source/DDManager.cpp)
// Converts a bits-per-pixel value to the corresponding DDBD_* flag.
SLONG BitDepthToFlags(SLONG bpp)
{
    switch (bpp) {
    case 1:  return DDBD_1;
    case 2:  return DDBD_2;
    case 4:  return DDBD_4;
    case 8:  return DDBD_8;
    case 16: return DDBD_16;
    case 24: return DDBD_24;
    case 32: return DDBD_32;
    default: return 0;
    }
}

// uc_orig: IsPalettized (fallen/DDLibrary/Source/DDManager.cpp)
// Returns TRUE if the given pixel format uses a palette index.
BOOL IsPalettized(LPDDPIXELFORMAT lp_dd_pf)
{
    if (!lp_dd_pf)
        return FALSE;

    if (lp_dd_pf->dwFlags & DDPF_PALETTEINDEXED1) return TRUE;
    if (lp_dd_pf->dwFlags & DDPF_PALETTEINDEXED2) return TRUE;
    if (lp_dd_pf->dwFlags & DDPF_PALETTEINDEXED4) return TRUE;
    if (lp_dd_pf->dwFlags & DDPF_PALETTEINDEXED8) return TRUE;

    return FALSE;
}

// ---------------------------------------------------------------------------
// Driver/device/mode lookup utilities
// ---------------------------------------------------------------------------

// uc_orig: GetDesktopMode (fallen/DDLibrary/Source/DDManager.cpp)
// Finds a display mode and D3D device matching the current desktop resolution.
BOOL GetDesktopMode(
    DDDriverInfo*  the_driver,
    LPGUID         D3D_guid,
    DDModeInfo**   the_mode,
    D3DDeviceInfo** the_device)
{
    SLONG w, h, bpp;
    HDC   hdc;
    HWND  hDesktop;
    DDModeInfo*    new_mode;
    D3DDeviceInfo* new_device;
    D3DDeviceInfo* next_best_device;

    if (!the_driver || !the_mode || !the_device)
        return FALSE;

    hDesktop = GetDesktopWindow();
    hdc      = GetDC(hDesktop);

    w   = GetDeviceCaps(hdc, HORZRES);
    h   = GetDeviceCaps(hdc, VERTRES);
    bpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);

    ReleaseDC(hDesktop, hdc);

    new_mode = the_driver->FindMode(w, h, bpp, 0, NULL);
    if (!new_mode)
        return FALSE;

    new_device = the_driver->FindDeviceSupportsMode(D3D_guid, new_mode, &next_best_device);
    if (!new_device) {
        if (!next_best_device)
            return FALSE;
        new_device = next_best_device;
    }

    *the_mode   = new_mode;
    *the_device = new_device;

    return TRUE;
}

// uc_orig: GetFullscreenMode (fallen/DDLibrary/Source/DDManager.cpp)
// Finds a display mode and D3D device for the requested fullscreen resolution.
// Falls back to DEFAULT_WIDTH/HEIGHT/DEPTH if w/h/bpp are 0.
BOOL GetFullscreenMode(
    DDDriverInfo*  the_driver,
    GUID*          D3D_guid,
    SLONG w, SLONG h, SLONG bpp, SLONG refresh,
    DDModeInfo**   the_mode,
    D3DDeviceInfo** the_device)
{
    D3DDeviceInfo* new_device;
    D3DDeviceInfo* next_best_device;
    DDModeInfo*    new_mode;
    DDModeInfo*    next_best_mode;

    if (!the_driver || !the_mode || !the_device)
        return FALSE;

    new_device = the_driver->FindDevice(D3D_guid, &next_best_device);
    if (!new_device) {
        if (!next_best_device)
            return FALSE;
        new_device = next_best_device;
    }

    if ((w == 0) || (h == 0) || (bpp == 0)) {
        w   = DEFAULT_WIDTH;
        h   = DEFAULT_HEIGHT;
        bpp = DEFAULT_DEPTH;
    }

    new_mode = the_driver->FindModeSupportsDevice(w, h, bpp, refresh, new_device, &next_best_mode);
    if (!new_mode) {
        if (!next_best_mode)
            return FALSE;
        new_mode = next_best_mode;
    }

    *the_mode   = new_mode;
    *the_device = new_device;

    return TRUE;
}

// ---------------------------------------------------------------------------
// Validate functions — find the best matching driver/device/mode for given criteria
// ---------------------------------------------------------------------------

// uc_orig: ValidateDriver (fallen/DDLibrary/Source/DDManager.cpp)
// Returns the DDDriverInfo matching DD_guid, or the best available alternative.
DDDriverInfo* ValidateDriver(GUID* DD_guid)
{
    DDDriverInfo* new_driver;
    DDDriverInfo* next_best_driver;

    new_driver = the_manager.FindDriver(DD_guid, &next_best_driver);
    if (new_driver)
        return new_driver;

    return next_best_driver;
}

// uc_orig: ValidateDevice (fallen/DDLibrary/Source/DDManager.cpp)
// Returns the D3DDeviceInfo matching D3D_guid on the_driver, or the best alternative.
D3DDeviceInfo* ValidateDevice(
    DDDriverInfo*  the_driver,
    GUID*          D3D_guid,
    DDModeInfo*    the_filter)
{
    D3DDeviceInfo* new_device;
    D3DDeviceInfo* next_best_device;

    if (!the_driver)
        return FALSE;

    if (!the_filter) {
        new_device = the_driver->FindDevice(D3D_guid, &next_best_device);
    } else {
        // Filter by mode support — exact path is unused in this build.
        new_device = NULL;
    }

    if (new_device)
        return new_device;

    return next_best_device;
}

// uc_orig: ValidateMode (fallen/DDLibrary/Source/DDManager.cpp)
// Returns the DDModeInfo matching w/h/bpp/refresh on the_driver, or the best alternative.
DDModeInfo* ValidateMode(
    DDDriverInfo*  the_driver,
    DWORD w, DWORD h, DWORD bpp, DWORD refresh,
    D3DDeviceInfo* the_filter)
{
    DDModeInfo* new_mode;
    DDModeInfo* next_best_mode;

    if (!the_driver)
        return FALSE;

    if (!the_filter)
        new_mode = the_driver->FindMode(w, h, bpp, refresh, &next_best_mode);
    else
        new_mode = the_driver->FindModeSupportsDevice(w, h, bpp, refresh, the_filter, &next_best_mode);

    if (new_mode)
        return new_mode;

    return next_best_mode;
}

// ---------------------------------------------------------------------------
// DDModeInfo
// ---------------------------------------------------------------------------

// uc_orig: DDModeInfo::DDModeInfo (fallen/DDLibrary/Source/DDManager.cpp)
DDModeInfo::DDModeInfo()
{
    InitStruct(ddSurfDesc);
    Prev = NULL;
    Next = NULL;
}

// uc_orig: DDModeInfo::DDModeInfo (fallen/DDLibrary/Source/DDManager.cpp)
DDModeInfo::DDModeInfo(const DDSURFACEDESC& ddDesc)
{
    CopyMemory(&ddSurfDesc, (const void*)&ddDesc, sizeof(ddSurfDesc));
    ASSERT(ddSurfDesc.dwSize == sizeof(ddSurfDesc));
    Prev = NULL;
    Next = NULL;
}

// uc_orig: DDModeInfo::~DDModeInfo (fallen/DDLibrary/Source/DDManager.cpp)
DDModeInfo::~DDModeInfo()
{
    Prev = NULL;
    Next = NULL;
}

// uc_orig: DDModeInfo::GetWidth (fallen/DDLibrary/Source/DDManager.cpp)
SLONG DDModeInfo::GetWidth(void)
{
    ASSERT(ddSurfDesc.dwSize == sizeof(ddSurfDesc));
    if (!(ddSurfDesc.dwFlags & DDSD_WIDTH))
        return 0;
    return ddSurfDesc.dwWidth;
}

// uc_orig: DDModeInfo::GetHeight (fallen/DDLibrary/Source/DDManager.cpp)
SLONG DDModeInfo::GetHeight(void)
{
    ASSERT(ddSurfDesc.dwSize == sizeof(ddSurfDesc));
    if (!(ddSurfDesc.dwFlags & DDSD_HEIGHT))
        return 0L;
    return ddSurfDesc.dwHeight;
}

// uc_orig: DDModeInfo::GetBPP (fallen/DDLibrary/Source/DDManager.cpp)
SLONG DDModeInfo::GetBPP(void)
{
    ASSERT(ddSurfDesc.dwSize == sizeof(ddSurfDesc));
    ASSERT(ddSurfDesc.ddpfPixelFormat.dwSize == sizeof(ddSurfDesc.ddpfPixelFormat));
    if (!(ddSurfDesc.dwFlags & DDSD_PIXELFORMAT))
        return 0;
    return ddSurfDesc.ddpfPixelFormat.dwRGBBitCount;
}

// uc_orig: DDModeInfo::GetMode (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDModeInfo::GetMode(SLONG* w, SLONG* h, SLONG* bpp, SLONG* refresh)
{
    ASSERT(ddSurfDesc.dwSize == sizeof(ddSurfDesc));
    ASSERT(ddSurfDesc.ddpfPixelFormat.dwSize == sizeof(ddSurfDesc.ddpfPixelFormat));

    if (!(ddSurfDesc.dwFlags & DDSD_WIDTH))      return DDERR_GENERIC;
    if (!(ddSurfDesc.dwFlags & DDSD_HEIGHT))     return DDERR_GENERIC;
    if (!(ddSurfDesc.dwFlags & DDSD_PIXELFORMAT)) return DDERR_GENERIC;

    *w       = (SLONG)ddSurfDesc.dwWidth;
    *h       = (SLONG)ddSurfDesc.dwHeight;
    *bpp     = (SLONG)ddSurfDesc.ddpfPixelFormat.dwRGBBitCount;
    *refresh = 0L;

    return DD_OK;
}

// uc_orig: DDModeInfo::ModeSupported (fallen/DDLibrary/Source/DDManager.cpp)
// Returns TRUE if the_device can render at this mode's bit depth.
BOOL DDModeInfo::ModeSupported(D3DDeviceInfo* the_device)
{
    SLONG bpp, depths, depth_flags;

    if (!the_device)
        return FALSE;

    bpp         = GetBPP();
    depth_flags = BitDepthToFlags(bpp);
    depths      = 0;

    if (the_device->IsHardware())
        depths = the_device->d3dHalDesc.dwDeviceRenderBitDepth;
    else
        depths = the_device->d3dHelDesc.dwDeviceRenderBitDepth;

    return (depths & depth_flags) ? TRUE : FALSE;
}

// uc_orig: DDModeInfo::Match (fallen/DDLibrary/Source/DDManager.cpp)
// Returns TRUE if this mode matches the given width, height, and bits-per-pixel.
BOOL DDModeInfo::Match(SLONG w, SLONG h, SLONG bpp)
{
    ASSERT(ddSurfDesc.dwSize == sizeof(ddSurfDesc));

    if ((ddSurfDesc.dwWidth  == (DWORD)w) &&
        (ddSurfDesc.dwHeight == (DWORD)h)) {
        if (ddSurfDesc.ddpfPixelFormat.dwRGBBitCount == (DWORD)bpp) {
            if (bpp <= 8 && !IsPalettized(&ddSurfDesc.ddpfPixelFormat))
                return FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

// uc_orig: DDModeInfo::Match (fallen/DDLibrary/Source/DDManager.cpp)
// Returns TRUE if this mode's bits-per-pixel matches bpp (overload — no w/h check).
BOOL DDModeInfo::Match(SLONG bpp)
{
    ASSERT(ddSurfDesc.dwSize == sizeof(ddSurfDesc));

    if (ddSurfDesc.ddpfPixelFormat.dwRGBBitCount == (DWORD)bpp) {
        if (bpp <= 8 && !IsPalettized(&ddSurfDesc.ddpfPixelFormat))
            return FALSE;
        return TRUE;
    }
    return FALSE;
}

// ---------------------------------------------------------------------------
// D3DDeviceInfo
// ---------------------------------------------------------------------------

// uc_orig: D3DDeviceInfo::D3DDeviceInfo (fallen/DDLibrary/Source/DDManager.cpp)
D3DDeviceInfo::D3DDeviceInfo()
{
    D3DFlags = 0;
    szName   = NULL;
    szDesc   = NULL;

    InitStruct(d3dHalDesc);
    InitStruct(d3dHelDesc);

    FormatCount   = 0;
    FormatList    = NULL;
    FormatListEnd = NULL;
    OpaqueTexFmt  = NULL;
    AlphaTexFmt   = NULL;

    CanDoModulateAlpha       = false;
    CanDoDestInvSourceColour = false;
    CanDoAdamiLighting       = false;
    CanDoForsythLighting     = false;

    Prev = NULL;
    Next = NULL;

    memset(&ZFormat, 0, sizeof(ZFormat));
}

// uc_orig: D3DDeviceInfo::~D3DDeviceInfo (fallen/DDLibrary/Source/DDManager.cpp)
D3DDeviceInfo::~D3DDeviceInfo()
{
    Destroy();
}

// uc_orig: D3DDeviceInfo::Create (fallen/DDLibrary/Source/DDManager.cpp)
// Initialises the device from enumeration data: copies GUID, name, desc, D3D caps.
HRESULT D3DDeviceInfo::Create(
    LPGUID          lpD3DGuid,
    LPTSTR          lpD3DName,
    LPTSTR          lpD3DDesc,
    LPD3DDEVICEDESC lpD3DHal,
    LPD3DDEVICEDESC lpD3DHel)
{
    ULONG  str_len, str_size;
    LPTSTR szTemp;

    if (!lpD3DGuid)
        return DDERR_INVALIDPARAMS;
    guid = *lpD3DGuid;

    szTemp   = lpD3DName ? lpD3DName : (LPTSTR)TEXT("UNKNOWN");
    str_len  = _tcslen(szTemp);
    str_size = (str_len + 1) * sizeof(TCHAR);
    szName   = (LPTSTR)MemAlloc(str_size);
    if (szName)
        _tcscpy(szName, szTemp);

    szTemp   = lpD3DDesc ? lpD3DDesc : (LPTSTR)TEXT("UNKNOWN");
    str_len  = _tcslen(szTemp);
    str_size = (str_len + 1) * sizeof(TCHAR);
    szDesc   = (LPTSTR)MemAlloc(str_size);
    if (szDesc)
        _tcscpy(szDesc, szTemp);

    if (lpD3DHal) d3dHalDesc = *lpD3DHal;
    if (lpD3DHel) d3dHelDesc = *lpD3DHel;

    FormatCount = 0;

    ValidOn();

    return DD_OK;
}

// uc_orig: D3DDeviceInfo::Destroy (fallen/DDLibrary/Source/DDManager.cpp)
void D3DDeviceInfo::Destroy(void)
{
    if (szDesc) { MemFree(szDesc); szDesc = NULL; }
    if (szName) { MemFree(szName); szName = NULL; }

    Prev = NULL;
    Next = NULL;
}

// uc_orig: D3DDeviceInfo::CheckCaps (fallen/DDLibrary/Source/DDManager.cpp)
// Queries D3D device caps and populates the CanDo* capability flags.
// Also reads the "Adami_lighting" env setting to allow disabling that path.
void D3DDeviceInfo::CheckCaps(LPDIRECT3DDEVICE3 the_device)
{
    D3DDEVICEDESC hw, sw;
    HRESULT rc;

    InitStruct(hw);
    InitStruct(sw);

    rc = the_device->GetCaps(&hw, &sw);
    if (FAILED(rc))
        return;

    CanDoModulateAlpha =
        (hw.dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATEALPHA) != 0;

    CanDoDestInvSourceColour =
        (hw.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR) != 0;

    CanDoAdamiLighting =
        (hw.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_SRCCOLOR) &&
        (hw.dpcTriCaps.dwSrcBlendCaps  & D3DPBLENDCAPS_DESTCOLOR);

    // Allow user to disable Adami lighting via env config.
    SLONG adami_lighting = ENV_get_value_number("Adami_lighting", -1, "Render");
    if (adami_lighting == -1) {
        ENV_set_value_number("Adami_lighting", 1, "Render");
        adami_lighting = 1;
    }
    if (adami_lighting == 0)
        CanDoAdamiLighting = false;
}

// uc_orig: D3DDeviceInfo::LoadFormats (fallen/DDLibrary/Source/DDManager.cpp)
// Enumerates texture formats for this device and picks best opaque/alpha formats.
HRESULT D3DDeviceInfo::LoadFormats(LPDIRECT3DDEVICE3 the_d3d_device)
{
    CallbackInfo callback_info;
    HRESULT result;

    if (!FormatsLoaded()) {
        if (!the_d3d_device)
            return DDERR_GENERIC;

        callback_info.Result = TRUE;
        callback_info.Count  = 0L;
        callback_info.Extra  = (void*)this;

        result = the_d3d_device->EnumTextureFormats(TextureFormatEnumCallback, (void*)&callback_info);
        if (FAILED(result))
            return result;

        if (!callback_info.Result || callback_info.Count == 0 || FormatCount != callback_info.Count)
            return DDERR_GENERIC;

        FindOpaqueTexFmt();
        FindAlphaTexFmt();

        TurnFormatsLoadedOn();
    }

    return DD_OK;
}

// uc_orig: D3DDeviceInfo::FindOpaqueTexFmt (fallen/DDLibrary/Source/DDManager.cpp)
// Selects the preferred opaque (no-alpha) texture format: highest-scoring 16-bit+ RGB format.
void D3DDeviceInfo::FindOpaqueTexFmt()
{
    OpaqueTexFmt = NULL;
    SLONG best_score = 0;

    for (DDModeInfo* mi = FormatList; mi; mi = mi->Next) {
        if (mi->ddSurfDesc.ddpfPixelFormat.dwFlags & DDPF_RGB) {
            if (mi->ddSurfDesc.ddpfPixelFormat.dwRGBBitCount >= 16) {
                SLONG score = 0x100 - mi->ddSurfDesc.ddpfPixelFormat.dwRGBBitCount;
                if (mi->ddSurfDesc.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS)
                    score -= 1;
                if (score > best_score) {
                    best_score   = score;
                    OpaqueTexFmt = mi;
                }
            }
        }
    }

    ASSERT(OpaqueTexFmt);
}

// uc_orig: D3DDeviceInfo::FindAlphaTexFmt (fallen/DDLibrary/Source/DDManager.cpp)
// Selects the preferred alpha texture format: 16-bit+ RGBA, preferring equal bit-counts per channel.
void D3DDeviceInfo::FindAlphaTexFmt()
{
    AlphaTexFmt = NULL;
    SLONG best_score = 0;

    for (DDModeInfo* mi = FormatList; mi; mi = mi->Next) {
        if ((mi->ddSurfDesc.ddpfPixelFormat.dwFlags & DDPF_RGB) &&
            (mi->ddSurfDesc.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) &&
            (mi->ddSurfDesc.ddpfPixelFormat.dwRGBBitCount >= 16)) {

            SLONG try_shift_alpha, try_shift_red, try_shift_green, try_shift_blue;
            SLONG try_mask_alpha, try_mask_red, try_mask_green, try_mask_blue;

            OS_calculate_mask_and_shift(mi->ddSurfDesc.ddpfPixelFormat.dwRGBAlphaBitMask, &try_mask_alpha, &try_shift_alpha);
            OS_calculate_mask_and_shift(mi->ddSurfDesc.ddpfPixelFormat.dwRBitMask,        &try_mask_red,   &try_shift_red);
            OS_calculate_mask_and_shift(mi->ddSurfDesc.ddpfPixelFormat.dwGBitMask,        &try_mask_green, &try_shift_green);
            OS_calculate_mask_and_shift(mi->ddSurfDesc.ddpfPixelFormat.dwBBitMask,        &try_mask_blue,  &try_shift_blue);

            SLONG score;
            if (try_mask_alpha == try_mask_red && try_mask_alpha == try_mask_green && try_mask_alpha == try_mask_blue)
                score = 0x400;
            else if (try_mask_red == try_mask_green && try_mask_red == try_mask_blue)
                score = 0x300;
            else
                score = 0x200;

            if (try_mask_alpha == 7) // only 1 bit of alpha
                score = 0x100;

            score -= mi->ddSurfDesc.ddpfPixelFormat.dwRGBBitCount;

            if (score > best_score) {
                best_score  = score;
                AlphaTexFmt = mi;
            }
        }
    }

    ASSERT(AlphaTexFmt);
}

// uc_orig: D3DDeviceInfo::LoadZFormats (fallen/DDLibrary/Source/DDManager.cpp)
// Enumerates Z-buffer formats; ZFormatEnumCallback picks a 16-bit no-stencil format.
HRESULT D3DDeviceInfo::LoadZFormats(LPDIRECT3D3 d3d)
{
    CallbackInfo callback_info;
    HRESULT result;

    callback_info.Result = TRUE;
    callback_info.Count  = 0;
    callback_info.Extra  = (void*)this;

    result = d3d->EnumZBufferFormats(guid, ZFormatEnumCallback, (void*)&callback_info);
    if (FAILED(result))
        return result;

    if (!callback_info.Result || !callback_info.Count)
        return DDERR_GENERIC;

    return DD_OK;
}

// uc_orig: D3DDeviceInfo::DestroyFormats (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT D3DDeviceInfo::DestroyFormats(void)
{
    DDModeInfo *current_format, *next_format;

    if (FormatsLoaded()) {
        current_format = FormatList;
        while (current_format) {
            next_format = current_format->Next;
            MFdelete(current_format);
            current_format = next_format;
        }

        FormatCount   = 0;
        FormatList    = NULL;
        FormatListEnd = NULL;

        TurnFormatsLoadedOff();
    }

    return DD_OK;
}

// uc_orig: D3DDeviceInfo::AddFormat (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT D3DDeviceInfo::AddFormat(DDModeInfo* the_format)
{
    if (!the_format)
        return DDERR_INVALIDPARAMS;

    the_format->Prev = FormatListEnd;
    the_format->Next = NULL;

    if (FormatListEnd)
        FormatListEnd->Next = the_format;
    FormatListEnd = the_format;

    if (!FormatList)
        FormatList = the_format;

    FormatCount++;

    return DD_OK;
}

// uc_orig: D3DDeviceInfo::DelFormat (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT D3DDeviceInfo::DelFormat(DDModeInfo* the_format)
{
    return DD_OK;
}

// uc_orig: D3DDeviceInfo::IsHardware (fallen/DDLibrary/Source/DDManager.cpp)
// Returns TRUE if this device is a hardware (HAL) device rather than software (HEL/MMX/RGB).
BOOL D3DDeviceInfo::IsHardware(void)
{
    return d3dHalDesc.dcmColorModel ? TRUE : FALSE;
}

// uc_orig: D3DDeviceInfo::Match (fallen/DDLibrary/Source/DDManager.cpp)
BOOL D3DDeviceInfo::Match(GUID* the_guid)
{
    if (!the_guid) return FALSE;
    if (!IsValid()) return FALSE;
    if (*the_guid != guid) return FALSE;
    return TRUE;
}

// uc_orig: D3DDeviceInfo::FindFormat (fallen/DDLibrary/Source/DDManager.cpp)
// Finds a texture format with the given bpp; next_best_format receives the first entry as fallback.
DDModeInfo* D3DDeviceInfo::FindFormat(
    SLONG        bpp,
    DDModeInfo** next_best_format,
    DDModeInfo*  start)
{
    DDModeInfo* current_format = start ? start : FormatList;

    if (next_best_format)
        *next_best_format = current_format;

    while (current_format) {
        if (current_format->Match(bpp))
            return current_format;
        current_format = current_format->Next;
    }

    return NULL;
}

// ---------------------------------------------------------------------------
// DDDriverInfo
// ---------------------------------------------------------------------------

// uc_orig: DDDriverInfo::DDDriverInfo (fallen/DDLibrary/Source/DDManager.cpp)
DDDriverInfo::DDDriverInfo()
{
    DriverFlags = 0;

    InitStruct(ddHalCaps);
    InitStruct(ddHelCaps);

    ModeCount   = 0;
    ModeList    = NULL;
    ModeListEnd = NULL;

    DeviceCount   = 0;
    DeviceList    = NULL;
    DeviceListEnd = NULL;

    Next = NULL;
    Prev = NULL;
}

// uc_orig: DDDriverInfo::~DDDriverInfo (fallen/DDLibrary/Source/DDManager.cpp)
DDDriverInfo::~DDDriverInfo()
{
    Destroy();
}

// uc_orig: DDDriverInfo::Create (fallen/DDLibrary/Source/DDManager.cpp)
// Enumerates modes and devices for this DirectDraw driver; sets DriverFlags capability bits.
HRESULT DDDriverInfo::Create(
    GUID*  lpGuid,
    LPTSTR lpDriverName,
    LPTSTR lpDriverDesc)
{
    ULONG   str_len, str_size;
    HRESULT result;
    LPDIRECTDRAW  lpDD  = NULL;
    LPDIRECTDRAW4 lpDD4 = NULL;
    LPDIRECT3D3   lpD3D = NULL;
    LPTSTR szTemp;

    if (IsValid())
        return FALSE;

    if (!lpGuid)
        PrimaryOn();
    else
        guid = *lpGuid;

    szTemp   = lpDriverName ? lpDriverName : (LPTSTR)TEXT("UNKNOWN");
    str_len  = _tcslen(szTemp);
    str_size = (str_len + 1) * sizeof(TCHAR);
    szName   = (LPTSTR)MemAlloc(str_size);
    if (szName) _tcscpy(szName, szTemp);

    szTemp   = lpDriverDesc ? lpDriverDesc : (LPTSTR)TEXT("UNKNOWN");
    str_len  = _tcslen(szTemp);
    str_size = (str_len + 1) * sizeof(TCHAR);
    szDesc   = (LPTSTR)MemAlloc(str_size);
    if (szDesc) _tcscpy(szDesc, szTemp);

    result = DirectDrawCreate(lpGuid, &lpDD, NULL);
    if (FAILED(result))
        goto cleanup;

    result = lpDD->QueryInterface(IID_IDirectDraw4, (void**)&lpDD4);
    if (FAILED(result))
        goto cleanup;

    result = lpDD->QueryInterface(IID_IDirect3D3, (void**)&lpD3D);
    if (FAILED(result))
        goto cleanup;

    result = lpDD4->GetCaps(&ddHalCaps, &ddHelCaps);
    if (FAILED(result))
        goto cleanup;

    DeviceCount = 0;
    result = LoadDevices(lpD3D);
    if (FAILED(result))
        goto cleanup;

    ModeCount = 0;
    result = LoadModes(lpDD4);
    if (FAILED(result))
        goto cleanup;

    // Flag drivers with less than 5 MB of video memory.
    {
        DDCAPS ddcaps;
        memset(&ddcaps, 0, sizeof(ddcaps));
        ddcaps.dwSize = sizeof(ddcaps);
        lpDD4->GetCaps(&ddcaps, NULL);
        if (ddcaps.dwVidMemTotal < 5 * 1024 * 1024)
            DriverFlags |= DD_DRIVER_LOW_MEMORY;
    }

    ValidOn();
    result = DD_OK;

cleanup:
    if (lpD3D) { lpD3D->Release(); lpD3D = NULL; }
    if (lpDD4) { lpDD4->Release(); lpDD4 = NULL; }
    if (lpDD)  { lpDD->Release();  lpDD  = NULL; }

    return result;
}

// uc_orig: DDDriverInfo::Destroy (fallen/DDLibrary/Source/DDManager.cpp)
void DDDriverInfo::Destroy(void)
{
    DestroyDevices();
    DestroyModes();

    if (szDesc) { MemFree(szDesc); szDesc = NULL; }
    if (szName) { MemFree(szName); szName = NULL; }

    Prev = NULL;
    Next = NULL;

    ValidOff();
}

// uc_orig: DDDriverInfo::Match (fallen/DDLibrary/Source/DDManager.cpp)
// Returns TRUE if this driver matches the_guid (or is the primary driver when the_guid is NULL).
BOOL DDDriverInfo::Match(GUID* the_guid)
{
    if (!IsValid()) return FALSE;

    if (!the_guid) {
        if (IsPrimary()) return TRUE;
    } else {
        if (*the_guid == guid) return TRUE;
    }

    return FALSE;
}

// uc_orig: DDDriverInfo::LoadModes (fallen/DDLibrary/Source/DDManager.cpp)
// Enumerates all DirectDraw display modes for this driver via IDirectDraw4::EnumDisplayModes.
HRESULT DDDriverInfo::LoadModes(LPDIRECTDRAW4 lpDD4)
{
    CallbackInfo callback_info;
    HRESULT result;

    if (!ModesLoaded()) {
        if (!lpDD4)
            return DDERR_GENERIC;

        callback_info.Result = TRUE;
        callback_info.Count  = 0L;
        callback_info.Extra  = (void*)this;

        result = lpDD4->EnumDisplayModes(0L, NULL, &callback_info, ModeEnumCallback);
        if (FAILED(result))
            return result;

        if (!callback_info.Result || callback_info.Count == 0 || ModeCount != callback_info.Count)
            return DDERR_GENERIC;

        TurnModesLoadedOn();
    }

    return DD_OK;
}

// uc_orig: DDDriverInfo::DestroyModes (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverInfo::DestroyModes(void)
{
    DDModeInfo *current_mode, *next_mode;

    current_mode = ModeList;
    while (current_mode) {
        next_mode = current_mode->Next;
        MFdelete(current_mode);
        current_mode = next_mode;
    }

    ModeCount   = 0;
    ModeList    = NULL;
    ModeListEnd = NULL;

    return DD_OK;
}

// uc_orig: DDDriverInfo::AddMode (fallen/DDLibrary/Source/DDManager.cpp)
// Appends a mode to the list; updates DriverFlags resolution and bit-depth support bits.
HRESULT DDDriverInfo::AddMode(DDModeInfo* the_mode)
{
    if (!the_mode)
        return DDERR_INVALIDPARAMS;

    the_mode->Prev = ModeListEnd;
    the_mode->Next = NULL;

    if (ModeListEnd) ModeListEnd->Next = the_mode;
    ModeListEnd = the_mode;

    if (!ModeList) ModeList = the_mode;

    if (the_mode->GetBPP() == 16 && (DriverFlags & DD_DRIVER_RENDERS_TO_16BIT))
        DriverFlags |= DD_DRIVER_SUPPORTS_16BIT;
    if (the_mode->GetBPP() == 32 && (DriverFlags & DD_DRIVER_RENDERS_TO_32BIT))
        DriverFlags |= DD_DRIVER_SUPPORTS_32BIT;

    if (the_mode->GetWidth() == 320  && the_mode->GetHeight() == 240)  DriverFlags |= DD_DRIVER_MODE_320;
    if (the_mode->GetWidth() == 512  && the_mode->GetHeight() == 384)  DriverFlags |= DD_DRIVER_MODE_512;
    if (the_mode->GetWidth() == 640  && the_mode->GetHeight() == 480)  DriverFlags |= DD_DRIVER_MODE_640;
    if (the_mode->GetWidth() == 800  && the_mode->GetHeight() == 600)  DriverFlags |= DD_DRIVER_MODE_800;
    if (the_mode->GetWidth() == 1024 && the_mode->GetHeight() == 768)  DriverFlags |= DD_DRIVER_MODE_1024;

    ModeCount++;

    return DD_OK;
}

// uc_orig: DDDriverInfo::DeleteMode (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverInfo::DeleteMode(DDModeInfo* the_mode)
{
    return DD_OK;
}

// uc_orig: DDDriverInfo::FindMode (fallen/DDLibrary/Source/DDManager.cpp)
// Finds a mode matching w/h/bpp; falls back to 640x480x8 as next_best.
DDModeInfo* DDDriverInfo::FindMode(
    SLONG w, SLONG h, SLONG bpp, SLONG refresh,
    DDModeInfo** next_best,
    DDModeInfo*  start_mode)
{
    DDModeInfo* current_mode = start_mode ? start_mode : ModeList;

    if (next_best)
        *next_best = current_mode;

    while (current_mode) {
        if (current_mode->Match(w, h, bpp))
            return current_mode;
        else if (current_mode->Match(DEFAULT_WIDTH, DEFAULT_HEIGHT, 8)) {
            if (next_best)
                *next_best = current_mode;
        }
        current_mode = current_mode->Next;
    }

    return NULL;
}

// uc_orig: DDDriverInfo::LoadDevices (fallen/DDLibrary/Source/DDManager.cpp)
// Enumerates all Direct3D devices for this driver via IDirect3D3::EnumDevices.
HRESULT DDDriverInfo::LoadDevices(LPDIRECT3D3 lpD3D3)
{
    CallbackInfo callback_info;
    HRESULT result;

    if (!DevicesLoaded()) {
        if (!lpD3D3)
            return DDERR_GENERIC;

        callback_info.Result = TRUE;
        callback_info.Count  = 0L;
        callback_info.Extra  = (void*)this;

        result = lpD3D3->EnumDevices(DeviceEnumCallback, &callback_info);
        if (FAILED(result))
            return result;

        if (!callback_info.Result || callback_info.Count == 0 || DeviceCount != callback_info.Count)
            return DDERR_GENERIC;

        TurnDevicesLoadedOn();
    }

    return DD_OK;
}

// uc_orig: DDDriverInfo::DestroyDevices (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverInfo::DestroyDevices(void)
{
    D3DDeviceInfo *current_device, *next_device;

    current_device = DeviceList;
    while (current_device) {
        next_device = current_device->Next;
        MFdelete(current_device);
        current_device = next_device;
    }

    DeviceCount   = 0;
    DeviceList    = NULL;
    DeviceListEnd = NULL;

    return DD_OK;
}

// uc_orig: DDDriverInfo::AddDevice (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverInfo::AddDevice(D3DDeviceInfo* the_device)
{
    if (!the_device)
        return DDERR_INVALIDPARAMS;

    the_device->Prev = DeviceListEnd;
    the_device->Next = NULL;

    if (DeviceListEnd) DeviceListEnd->Next = the_device;
    DeviceListEnd = the_device;

    if (!DeviceList) DeviceList = the_device;

    DeviceCount++;

    return DD_OK;
}

// uc_orig: DDDriverInfo::DeleteDevice (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverInfo::DeleteDevice(D3DDeviceInfo* the_device)
{
    return DD_OK;
}

// uc_orig: DDDriverInfo::FindDevice (fallen/DDLibrary/Source/DDManager.cpp)
// Finds the device matching the_guid; prefers hardware over MMX over RGB as next_best.
D3DDeviceInfo* DDDriverInfo::FindDevice(
    GUID*          the_guid,
    D3DDeviceInfo** next_best,
    D3DDeviceInfo*  start_device)
{
    D3DDeviceInfo* current_device    = start_device ? start_device : DeviceList;
    D3DDeviceInfo* first_device      = current_device;
    D3DDeviceInfo* hardware_device   = NULL;
    D3DDeviceInfo* mmx_device        = NULL;
    D3DDeviceInfo* rgb_device        = NULL;

    if (next_best) *next_best = NULL;

    while (current_device) {
        if (current_device->Match(the_guid))
            return current_device;

        if (current_device->IsHardware() && !hardware_device)
            hardware_device = current_device;

        if (current_device->guid == IID_IDirect3DRGBDevice && !rgb_device)
            rgb_device = current_device;

        if (current_device->guid == IID_IDirect3DMMXDevice && !mmx_device)
            mmx_device = current_device;

        current_device = current_device->Next;
    }

    if (next_best) {
        if (hardware_device)       *next_best = hardware_device;
        else if (rgb_device)       *next_best = rgb_device;
        else if (mmx_device)       *next_best = mmx_device;
        else if (first_device)     *next_best = first_device;
    }

    return NULL;
}

// uc_orig: DDDriverInfo::FindDeviceSupportsMode (fallen/DDLibrary/Source/DDManager.cpp)
// Finds a device matching the_guid that also supports the_mode; favours hardware.
D3DDeviceInfo* DDDriverInfo::FindDeviceSupportsMode(
    GUID*           the_guid,
    DDModeInfo*     the_mode,
    D3DDeviceInfo** next_best_device,
    D3DDeviceInfo*  start_device)
{
    D3DDeviceInfo* current_device;

    if (!the_mode) {
        if (next_best_device) *next_best_device = NULL;
        return NULL;
    }

    current_device = start_device ? start_device : DeviceList;

    if (next_best_device) {
        if (the_mode->ModeSupported(current_device))
            *next_best_device = current_device;
    }

    while (current_device) {
        if (current_device->Match(the_guid)) {
            if (the_mode->ModeSupported(current_device))
                return current_device;
        } else if (current_device->IsHardware()) {
            if (next_best_device && the_mode->ModeSupported(current_device))
                *next_best_device = current_device;
        } else if (the_mode->ModeSupported(current_device)) {
            if (next_best_device)
                *next_best_device = current_device;
        }
        current_device = current_device->Next;
    }

    return NULL;
}

// uc_orig: DDDriverInfo::FindModeSupportsDevice (fallen/DDLibrary/Source/DDManager.cpp)
// Finds a mode with the given w/h/bpp that is supported by the_device; fallback to any supported mode.
DDModeInfo* DDDriverInfo::FindModeSupportsDevice(
    SLONG          w, SLONG h, SLONG bpp, SLONG refresh,
    D3DDeviceInfo* the_device,
    DDModeInfo**   next_best,
    DDModeInfo*    start_mode)
{
    DDModeInfo* current_mode;

    if (!the_device) {
        if (next_best) *next_best = NULL;
        return NULL;
    }

    current_mode = start_mode ? start_mode : ModeList;

    if (next_best && current_mode->ModeSupported(the_device))
        *next_best = current_mode;

    while (current_mode) {
        if (current_mode->Match(w, h, bpp)) {
            if (current_mode->ModeSupported(the_device))
                return current_mode;
        } else if (current_mode->Match(DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_DEPTH)) {
            if (next_best && current_mode->ModeSupported(the_device))
                *next_best = current_mode;
        } else if (current_mode->ModeSupported(the_device)) {
            if (next_best)
                *next_best = current_mode;
        }
        current_mode = current_mode->Next;
    }

    return NULL;
}

// uc_orig: DDDriverInfo::GetGuid (fallen/DDLibrary/Source/DDManager.cpp)
// Returns NULL for the primary driver (no GUID), or the driver's GUID otherwise.
GUID* DDDriverInfo::GetGuid(void)
{
    return IsPrimary() ? NULL : &guid;
}

// ---------------------------------------------------------------------------
// DDDriverManager
// ---------------------------------------------------------------------------

// uc_orig: DDDriverManager::DDDriverManager (fallen/DDLibrary/Source/DDManager.cpp)
DDDriverManager::DDDriverManager()
{
    ManagerFlags = 0;
    DriverCount  = 0;
    DriverList   = NULL;
    DriverListEnd= NULL;

    CurrDriver        = NULL;
    CurrMode          = NULL;
    CurrDevice        = NULL;
    CurrTextureFormat = NULL;
}

// uc_orig: DDDriverManager::~DDDriverManager (fallen/DDLibrary/Source/DDManager.cpp)
DDDriverManager::~DDDriverManager()
{
    Fini();
}

// uc_orig: DDDriverManager::Init (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverManager::Init(void)
{
    HRESULT result;

    if (!IsInitialised()) {
        result = LoadDrivers();
        if (FAILED(result))
            return result;
        InitOn();
    }
    return DD_OK;
}

// uc_orig: DDDriverManager::Fini (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverManager::Fini(void)
{
    if (IsInitialised()) {
        DestroyDrivers();
        InitOff();
    }
    return DD_OK;
}

// uc_orig: DDDriverManager::LoadDrivers (fallen/DDLibrary/Source/DDManager.cpp)
// Calls DirectDrawEnumerate to fill the driver list.
HRESULT DDDriverManager::LoadDrivers(void)
{
    CallbackInfo callback_info;
    HRESULT result;

    callback_info.Result = TRUE;
    callback_info.Count  = 0L;
    callback_info.Extra  = NULL;

    result = DirectDrawEnumerate(DriverEnumCallback, &callback_info);
    if (FAILED(result))
        return result;

    if (!callback_info.Result || callback_info.Count == 0 || DriverCount != callback_info.Count)
        return DDERR_GENERIC;

    return DD_OK;
}

// uc_orig: DDDriverManager::DestroyDrivers (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverManager::DestroyDrivers(void)
{
    DDDriverInfo *current_driver, *next_driver;

    current_driver = DriverList;
    while (current_driver) {
        next_driver = current_driver->Next;
        MFdelete(current_driver);
        current_driver = next_driver;
    }
    return DD_OK;
}

// uc_orig: DDDriverManager::AddDriver (fallen/DDLibrary/Source/DDManager.cpp)
HRESULT DDDriverManager::AddDriver(DDDriverInfo* the_driver)
{
    if (!the_driver)
        return DDERR_INVALIDPARAMS;

    the_driver->Prev = DriverListEnd;
    the_driver->Next = NULL;

    if (DriverListEnd) DriverListEnd->Next = the_driver;
    DriverListEnd = the_driver;

    if (!DriverList) DriverList = the_driver;

    DriverCount++;

    return DD_OK;
}

// uc_orig: DDDriverManager::FindDriver (fallen/DDLibrary/Source/DDManager.cpp)
// Finds the driver matching the_guid; for NULL guid prefers the primary driver.
// In both NDEBUG and DEBUG builds, prefers the non-primary (dedicated) device when a GUID is given.
DDDriverInfo* DDDriverManager::FindDriver(GUID* the_guid, DDDriverInfo** next_best, DDDriverInfo* start_driver)
{
    DDDriverInfo* current_driver = start_driver ? start_driver : the_manager.DriverList;

    if (next_best) *next_best = current_driver;

    DDDriverInfo* driver[10];
    SLONG driver_upto = 0;

    while (current_driver) {
        ASSERT(driver_upto < 10);
        driver[driver_upto++] = current_driver;
        current_driver = current_driver->Next;
    }

    DDDriverInfo* choice1 = NULL;
    DDDriverInfo* choice2 = NULL;

    for (SLONG i = 0; i < driver_upto; i++) {
        if (the_guid) {
            if (driver[i]->Match(the_guid))
                choice1 = driver[i];
            else
                choice2 = driver[i];
        } else {
            if (driver[i]->IsPrimary())
                choice1 = driver[i];
            else
                choice2 = driver[i];
        }
    }

    if (!choice1) choice1 = choice2;
    if (!choice2) choice2 = choice1;

    if (next_best) *next_best = choice2;

    return choice1;
}
