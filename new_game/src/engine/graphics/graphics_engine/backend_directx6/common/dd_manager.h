#ifndef ENGINE_GRAPHICS_GRAPHICS_API_DD_MANAGER_H
#define ENGINE_GRAPHICS_GRAPHICS_API_DD_MANAGER_H

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "engine/core/types.h"

// uc_orig: DD_DRIVER_INIT (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_INIT (1 << 0)

// DDDriverInfo flags
// uc_orig: DD_DRIVER_VALID (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_VALID          (1 << 0)
// uc_orig: DD_DRIVER_PRIMARY (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_PRIMARY        (1 << 1)
// uc_orig: DD_DRIVER_D3D (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_D3D            (1 << 2)
// uc_orig: DD_DRIVER_M_LOADED (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_M_LOADED       (1 << 3)
// uc_orig: DD_DRIVER_D_LOADED (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_D_LOADED       (1 << 4)
// uc_orig: DD_DRIVER_SUPPORTS_16BIT (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_SUPPORTS_16BIT (1 << 5)
// uc_orig: DD_DRIVER_SUPPORTS_32BIT (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_SUPPORTS_32BIT (1 << 6)
// uc_orig: DD_DRIVER_RENDERS_TO_16BIT (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_RENDERS_TO_16BIT (1 << 7)
// uc_orig: DD_DRIVER_RENDERS_TO_32BIT (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_RENDERS_TO_32BIT (1 << 8)
// uc_orig: DD_DRIVER_MODE_320 (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_MODE_320  (1 << 9)
// uc_orig: DD_DRIVER_MODE_512 (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_MODE_512  (1 << 10)
// uc_orig: DD_DRIVER_MODE_640 (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_MODE_640  (1 << 11)
// uc_orig: DD_DRIVER_MODE_800 (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_MODE_800  (1 << 12)
// uc_orig: DD_DRIVER_MODE_1024 (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_MODE_1024 (1 << 13)
// uc_orig: DD_DRIVER_LOW_MEMORY (fallen/DDLibrary/Headers/DDManager.h)
#define DD_DRIVER_LOW_MEMORY (1 << 14)

// D3DDeviceInfo flags
// uc_orig: D3D_DEVICE_VALID (fallen/DDLibrary/Headers/DDManager.h)
#define D3D_DEVICE_VALID    (1 << 0)
// uc_orig: D3D_DEVICE_F_LOADED (fallen/DDLibrary/Headers/DDManager.h)
#define D3D_DEVICE_F_LOADED (1 << 1)

// uc_orig: CallbackInfo (fallen/DDLibrary/Headers/DDManager.h)
// Passed as lpExtra to DirectDraw enumeration callbacks to accumulate results.
typedef struct
{
    BOOL  Result; // Success/failure flag
    DWORD Count;  // Number of items enumerated so far
    void* Extra;  // Currently enumerated driver/device/etc.
} CallbackInfo;

// Forward declarations
// uc_orig: DDModeInfo (fallen/DDLibrary/Headers/DDManager.h)
class DDModeInfo;
// uc_orig: D3DDeviceInfo (fallen/DDLibrary/Headers/DDManager.h)
class D3DDeviceInfo;
// uc_orig: DDDriverInfo (fallen/DDLibrary/Headers/DDManager.h)
class DDDriverInfo;
// uc_orig: DDDriverManager (fallen/DDLibrary/Headers/DDManager.h)
class DDDriverManager;

// Helper functions for converting between bit-depth flags and actual BPP values.
// uc_orig: FlagsToBitDepth (fallen/DDLibrary/Headers/DDManager.h)
SLONG FlagsToBitDepth(SLONG flags);
// uc_orig: FlagsToMask (fallen/DDLibrary/Headers/DDManager.h)
ULONG FlagsToMask(SLONG flags);
// uc_orig: BitDepthToFlags (fallen/DDLibrary/Headers/DDManager.h)
SLONG BitDepthToFlags(SLONG bpp);
// uc_orig: IsPalettized (fallen/DDLibrary/Headers/DDManager.h)
BOOL  IsPalettized(LPDDPIXELFORMAT lp_dd_pf);

// Driver/device/mode lookup utilities.
// uc_orig: GetDesktopMode (fallen/DDLibrary/Headers/DDManager.h)
BOOL GetDesktopMode(DDDriverInfo* the_driver, LPGUID D3D_guid, DDModeInfo** the_mode, D3DDeviceInfo** the_device);
// uc_orig: GetFullscreenMode (fallen/DDLibrary/Headers/DDManager.h)
BOOL GetFullscreenMode(DDDriverInfo* the_driver, GUID* D3D_guid, SLONG w, SLONG h, SLONG bpp, SLONG refresh, DDModeInfo** the_mode, D3DDeviceInfo** the_device);
// uc_orig: ValidateDriver (fallen/DDLibrary/Headers/DDManager.h)
DDDriverInfo*  ValidateDriver(GUID* DD_guid);
// uc_orig: ValidateDevice (fallen/DDLibrary/Headers/DDManager.h)
D3DDeviceInfo* ValidateDevice(DDDriverInfo* the_driver, GUID* D3D_guid, DDModeInfo* the_filter = NULL);
// uc_orig: ValidateMode (fallen/DDLibrary/Headers/DDManager.h)
DDModeInfo*    ValidateMode(DDDriverInfo* the_driver, DWORD w, DWORD h, DWORD bpp, DWORD refresh, D3DDeviceInfo* the_filter = NULL);

// uc_orig: DDModeInfo (fallen/DDLibrary/Headers/DDManager.h)
// Describes a single DirectDraw display mode (width, height, BPP, refresh rate).
// Stored as a linked list inside DDDriverInfo.
class DDModeInfo {
private:
protected:
public:
    DDSURFACEDESC2 ddSurfDesc;
    DDModeInfo *Next, *Prev;

    DDModeInfo();
    DDModeInfo(const DDSURFACEDESC& ddDesc);
    ~DDModeInfo();

    SLONG   GetWidth(void);
    SLONG   GetHeight(void);
    SLONG   GetBPP(void);
    HRESULT GetMode(SLONG* w, SLONG* h, SLONG* bpp, SLONG* refresh);
    BOOL    ModeSupported(D3DDeviceInfo* the_device);
    BOOL    Match(SLONG w, SLONG h, SLONG bpp);
    BOOL    Match(SLONG bpp);
};

// uc_orig: D3DDeviceInfo (fallen/DDLibrary/Headers/DDManager.h)
// Describes a Direct3D device including its capabilities, supported texture formats,
// and Z-buffer pixel format. Stored as a linked list inside DDDriverInfo.
class D3DDeviceInfo {
private:
protected:
public:
    ULONG D3DFlags;

    GUID    guid;
    LPTSTR  szName;
    LPTSTR  szDesc;
    D3DDEVICEDESC d3dHalDesc;
    D3DDEVICEDESC d3dHelDesc;

    ULONG      FormatCount;
    DDModeInfo *FormatList, *FormatListEnd;

    DDModeInfo* OpaqueTexFmt;
    DDModeInfo* AlphaTexFmt;

    DDPIXELFORMAT ZFormat;

    // Runtime capability flags queried from the device.
    bool CanDoModulateAlpha;
    bool CanDoDestInvSourceColour;
    bool CanDoAdamiLighting;
    bool CanDoForsythLighting;

    D3DDeviceInfo *Next, *Prev;

    D3DDeviceInfo();
    ~D3DDeviceInfo();

    HRESULT Create(LPGUID lpD3DGuid, LPTSTR lpD3DName, LPTSTR lpD3DDesc, LPD3DDEVICEDESC lpD3DHal, LPD3DDEVICEDESC lpD3DHel);
    void    Destroy(void);

    BOOL IsHardware(void);
    BOOL Match(GUID* the_guid);

    inline BOOL IsValid(void)   { return D3DFlags & D3D_DEVICE_VALID; }
    inline void ValidOn(void)   { D3DFlags |= D3D_DEVICE_VALID; }
    inline void ValidOff(void)  { D3DFlags &= ~D3D_DEVICE_VALID; }

    HRESULT  LoadFormats(LPDIRECT3DDEVICE3 the_d3d_device);
    HRESULT  DestroyFormats(void);
    HRESULT  AddFormat(DDModeInfo* the_format);
    HRESULT  DelFormat(DDModeInfo* the_format);

    void        FindOpaqueTexFmt();
    void        FindAlphaTexFmt();
    DDModeInfo* FindFormat(SLONG bpp, DDModeInfo** next_best_format, DDModeInfo* start = NULL);

    inline SLONG CountFormats(void)       { return FormatCount; }
    inline BOOL  FormatsLoaded(void)      { return ((D3DFlags & D3D_DEVICE_F_LOADED) ? UC_TRUE : UC_FALSE); }
    inline void  TurnFormatsLoadedOn(void)  { D3DFlags |= D3D_DEVICE_F_LOADED; }
    inline void  TurnFormatsLoadedOff(void) { D3DFlags &= ~D3D_DEVICE_F_LOADED; }

    HRESULT        LoadZFormats(LPDIRECT3D3 d3d);
    void           SetZFormat(LPDDPIXELFORMAT lpPF) { memcpy(&ZFormat, lpPF, sizeof(ZFormat)); }
    LPDDPIXELFORMAT GetZFormat() { return &ZFormat; }

    void CheckCaps(LPDIRECT3DDEVICE3 the_device);
    bool ModulateAlphaSupported()       { return CanDoModulateAlpha; }
    bool DestInvSourceColourSupported() { return CanDoDestInvSourceColour; }
    bool AdamiLightingSupported()       { return CanDoAdamiLighting; }
};

// uc_orig: DDDriverInfo (fallen/DDLibrary/Headers/DDManager.h)
// Describes a DirectDraw driver (display adapter). Contains linked lists of
// supported display modes and D3D devices for this driver.
class DDDriverInfo {
private:
protected:
public:
    ULONG DriverFlags;

    GUID   guid, hardware_guid;
    LPTSTR szName;
    LPTSTR szDesc;
    DDCAPS ddHalCaps;
    DDCAPS ddHelCaps;

    ULONG      ModeCount;
    DDModeInfo *ModeList, *ModeListEnd;

    ULONG         DeviceCount;
    D3DDeviceInfo *DeviceList, *DeviceListEnd;

    DDDriverInfo *Next, *Prev;

    DDDriverInfo();
    ~DDDriverInfo();

    HRESULT Create(GUID* lpGuid, LPTSTR lpDriverName, LPTSTR lpDriverDesc);
    void    Destroy(void);
    BOOL    Match(GUID* the_guid);
    GUID*   GetGuid(void);

    inline BOOL IsValid(void)   { return DriverFlags & DD_DRIVER_VALID; }
    inline void ValidOn(void)   { DriverFlags |= DD_DRIVER_VALID; }
    inline void ValidOff(void)  { DriverFlags &= ~DD_DRIVER_VALID; }

    inline BOOL IsPrimary(void)  { return DriverFlags & DD_DRIVER_PRIMARY; }
    inline void PrimaryOn(void)  { DriverFlags |= DD_DRIVER_PRIMARY; }
    inline void PrimaryOff(void) { DriverFlags &= ~DD_DRIVER_PRIMARY; }

    inline BOOL IsD3D(void) { return DriverFlags & DD_DRIVER_D3D; }
    inline void D3DOn(void) { DriverFlags |= DD_DRIVER_D3D; }
    inline void D3DOff(void){ DriverFlags &= ~DD_DRIVER_D3D; }

    HRESULT     LoadModes(LPDIRECTDRAW4 lpDD4);
    HRESULT     DestroyModes(void);
    HRESULT     AddMode(DDModeInfo* the_mode);
    HRESULT     DeleteMode(DDModeInfo* the_mode);
    DDModeInfo* FindMode(SLONG w, SLONG h, SLONG bpp, SLONG refresh, DDModeInfo** next_best = NULL, DDModeInfo* start_mode = NULL);

    inline SLONG CountModes(void)       { return ModeCount; }
    inline BOOL  ModesLoaded(void)      { return ((DriverFlags & DD_DRIVER_M_LOADED) ? UC_TRUE : UC_FALSE); }
    inline void  TurnModesLoadedOn(void)  { DriverFlags |= DD_DRIVER_M_LOADED; }
    inline void  TurnModesLoadedOff(void) { DriverFlags &= ~DD_DRIVER_M_LOADED; }

    HRESULT       LoadDevices(LPDIRECT3D3 lpD3D3);
    HRESULT       DestroyDevices(void);
    HRESULT       AddDevice(D3DDeviceInfo* the_device);
    HRESULT       DeleteDevice(D3DDeviceInfo* the_device);
    D3DDeviceInfo* FindDevice(GUID* the_guid, D3DDeviceInfo** next_best, D3DDeviceInfo* start_device = NULL);
    D3DDeviceInfo* FindDeviceSupportsMode(GUID* the_guid, DDModeInfo* the_mode, D3DDeviceInfo** next_best_device, D3DDeviceInfo* start_device = NULL);
    DDModeInfo*    FindModeSupportsDevice(SLONG w, SLONG h, SLONG bpp, SLONG refresh, D3DDeviceInfo* the_device, DDModeInfo** next_best, DDModeInfo* start_device = NULL);

    inline SLONG CountDevices(void)        { return DeviceCount; }
    inline BOOL  DevicesLoaded(void)       { return ((DriverFlags & DD_DRIVER_D_LOADED) ? UC_TRUE : UC_FALSE); }
    inline void  TurnDevicesLoadedOn(void)  { DriverFlags |= DD_DRIVER_D_LOADED; }
    inline void  TurnDevicesLoadedOff(void) { DriverFlags &= ~DD_DRIVER_D_LOADED; }
};

// uc_orig: DDDriverManager (fallen/DDLibrary/Headers/DDManager.h)
// Singleton that enumerates and manages all available DirectDraw drivers.
// Holds the currently selected driver, mode, and D3D device.
class DDDriverManager {
private:
protected:
public:
    ULONG         ManagerFlags, DriverCount;
    DDDriverInfo *DriverList, *DriverListEnd;

    DDDriverInfo* CurrDriver;
    DDModeInfo*   CurrMode;
    D3DDeviceInfo* CurrDevice;
    DDModeInfo*   CurrTextureFormat;

    DDDriverManager();
    ~DDDriverManager();

    HRESULT Init(void);
    HRESULT Fini(void);

    HRESULT      LoadDrivers(void);
    HRESULT      DestroyDrivers(void);
    HRESULT      AddDriver(DDDriverInfo* the_driver);
    DDDriverInfo* FindDriver(GUID* guid, DDDriverInfo** next_best, DDDriverInfo* start_driver = NULL);
    DDDriverInfo* FindDriver(DDCAPS* hal, DDCAPS* hel, DDDriverInfo** next_best, DDDriverInfo* start_driver = NULL);

    inline BOOL IsInitialised(void) { return ManagerFlags & DD_DRIVER_INIT; }
    inline void InitOn(void)  { ManagerFlags |= DD_DRIVER_INIT; }
    inline void InitOff(void) { ManagerFlags &= ~DD_DRIVER_INIT; }
};

// uc_orig: the_manager (fallen/DDLibrary/Headers/DDManager.h)
// Global DDDriverManager singleton — holds all enumerated DirectDraw drivers.
extern DDDriverManager the_manager;

// uc_orig: InitStruct (fallen/DDLibrary/Headers/DDManager.h)
// Zero-initialises a DirectX structure and sets its dwSize field.
#define InitStruct(s)                \
    {                                \
        ZeroMemory(&(s), sizeof(s)); \
        (s).dwSize = sizeof(s);      \
    }

#endif // ENGINE_GRAPHICS_GRAPHICS_API_DD_MANAGER_H
