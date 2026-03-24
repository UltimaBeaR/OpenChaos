#ifndef ENGINE_GRAPHICS_GRAPHICS_API_GD_DISPLAY_H
#define ENGINE_GRAPHICS_GRAPHICS_API_GD_DISPLAY_H

// MFStdLib.h must be included before this header defines DisplayWidth/DisplayHeight
// as #define macros (640/480). MFStdLib declares them as extern SLONG — if the
// macros come first, that extern declaration becomes "extern SLONG 640" and fails.
#include "engine/platform/platform.h"
#include "engine/graphics/graphics_api/dd_manager.h"
#include "engine/graphics/resources/d3d_texture.h"

// uc_orig: SHELL_ACTIVE (fallen/DDLibrary/Headers/GDisplay.h)
#define SHELL_ACTIVE   (LibShellActive())
// uc_orig: SHELL_CHANGED (fallen/DDLibrary/Headers/GDisplay.h)
#define SHELL_CHANGED  (LibShellChanged())

// uc_orig: FLAGS_USE_3D (fallen/DDLibrary/Headers/GDisplay.h)
#define FLAGS_USE_3D       (1 << 1)
// uc_orig: FLAGS_USE_WORKSCREEN (fallen/DDLibrary/Headers/GDisplay.h)
#define FLAGS_USE_WORKSCREEN (1 << 2)

// uc_orig: DEFAULT_WIDTH (fallen/DDLibrary/Headers/GDisplay.h)
#define DEFAULT_WIDTH  (640)
// uc_orig: DEFAULT_HEIGHT (fallen/DDLibrary/Headers/GDisplay.h)
#define DEFAULT_HEIGHT (480)
// uc_orig: DEFAULT_DEPTH (fallen/DDLibrary/Headers/GDisplay.h)
#define DEFAULT_DEPTH  (16)

// uc_orig: enumDisplayType (fallen/DDLibrary/Headers/GDisplay.h)
// PAL/NTSC/VGA display output type.
enum enumDisplayType {
    DT_PAL,
    DT_NTSC,
    DT_VGA,
};

// uc_orig: eDisplayType (fallen/DDLibrary/Headers/GDisplay.h)
extern enumDisplayType eDisplayType;

// uc_orig: InitBackImage (fallen/DDLibrary/Headers/GDisplay.h)
// Loads a background image from file for use as the display background surface.
extern void InitBackImage(CBYTE* name);
// uc_orig: UseBackSurface (fallen/DDLibrary/Headers/GDisplay.h)
// Redirects the background to an externally provided DirectDraw surface.
extern void UseBackSurface(LPDIRECTDRAWSURFACE4 use);
// uc_orig: ResetBackImage (fallen/DDLibrary/Headers/GDisplay.h)
// Clears the background image; display will show black/cleared background.
extern void ResetBackImage(void);
// uc_orig: ShowBackImage (fallen/DDLibrary/Headers/GDisplay.h)
// Blits the current background image to the back buffer.
// b3DInFrame=UC_FALSE allows DC-level blits when no 3D rendering is active.
extern void ShowBackImage(bool b3DInFrame = UC_TRUE);

// uc_orig: OpenDisplay (fallen/DDLibrary/Headers/GDisplay.h)
// Opens the display with the requested resolution/depth/flags. Returns non-zero on success.
SLONG OpenDisplay(ULONG width, ULONG height, ULONG depth, ULONG flags);
// uc_orig: CloseDisplay (fallen/DDLibrary/Headers/GDisplay.h)
SLONG CloseDisplay(void);
// uc_orig: SetDisplay (fallen/DDLibrary/Headers/GDisplay.h)
// Changes display resolution without re-opening.
SLONG SetDisplay(ULONG width, ULONG height, ULONG depth);
// uc_orig: ClearDisplay (fallen/DDLibrary/Headers/GDisplay.h)
// Clears the display surface to the specified RGB colour.
SLONG ClearDisplay(UBYTE r, UBYTE g, UBYTE b);
// uc_orig: ShellPaused (fallen/DDLibrary/Headers/GDisplay.h)
// Returns true if the application is shell-paused (minimised/alt-tabbed).
void ShellPaused(void);
// uc_orig: ShellPauseOn (fallen/DDLibrary/Headers/GDisplay.h)
void ShellPauseOn(void);
// uc_orig: ShellPauseOff (fallen/DDLibrary/Headers/GDisplay.h)
void ShellPauseOff(void);

// Display state flags.
// uc_orig: DISPLAY_INIT (fallen/DDLibrary/Headers/GDisplay.h)
#define DISPLAY_INIT      (1 << 0)
// uc_orig: DISPLAY_PAUSE (fallen/DDLibrary/Headers/GDisplay.h)
#define DISPLAY_PAUSE     (1 << 1)
// uc_orig: DISPLAY_PAUSE_ACK (fallen/DDLibrary/Headers/GDisplay.h)
#define DISPLAY_PAUSE_ACK (1 << 2)
// uc_orig: DISPLAY_LOCKED (fallen/DDLibrary/Headers/GDisplay.h)
#define DISPLAY_LOCKED    (1 << 3)

// Background colour modes for the D3D viewport.
// uc_orig: BK_COL_NONE (fallen/DDLibrary/Headers/GDisplay.h)
#define BK_COL_NONE  0
// uc_orig: BK_COL_BLACK (fallen/DDLibrary/Headers/GDisplay.h)
#define BK_COL_BLACK 1
// uc_orig: BK_COL_WHITE (fallen/DDLibrary/Headers/GDisplay.h)
#define BK_COL_WHITE 2
// uc_orig: BK_COL_USER (fallen/DDLibrary/Headers/GDisplay.h)
#define BK_COL_USER  3

// uc_orig: Display (fallen/DDLibrary/Headers/GDisplay.h)
// The main display class: owns DirectDraw/Direct3D surfaces, viewport, and the render loop.
// Wraps all D3D API calls needed to draw a frame and manage window/fullscreen state.
class Display {
private:
protected:
    enum {
        DWF_FULLSCREEN      = (1 << 0),
        DWF_VISIBLE         = (1 << 1),
        DWF_ZBUFFER         = (1 << 2),
        DWF_ACTIVE          = (1 << 3),
        DWF_USE_3D          = (1 << 4),
        DWF_USE_WORK        = (1 << 5),
        DWF_DISPLAY_CHANGED = (1 << 6),
        DWF_TEXTURES_INVALID= (1 << 7)
    } Attributes;

    enum {
        DWF_VALID_INTERFACE  = (1 << 0),
        DWF_VALID_FULLSCREEN = (1 << 1),
        DWF_VALID_FRONT      = (1 << 2),
        DWF_VALID_BACK       = (1 << 3),
        DWF_VALID_WORK       = (1 << 4),
        DWF_VALID_VIEWPORT   = (1 << 6),

        DWF_VALID = DWF_VALID_INTERFACE | DWF_VALID_FULLSCREEN | DWF_VALID_FRONT | DWF_VALID_BACK
    } Validates;

    volatile SLONG AttribFlags, ValidFlags, PauseCount;

    inline void TurnValidInterfaceOn(void)   { ValidFlags |= DWF_VALID_INTERFACE; }
    inline void TurnValidInterfaceOff(void)  { ValidFlags &= ~DWF_VALID_INTERFACE; }
    inline void TurnValidFullscreenOn(void)  { ValidFlags |= DWF_VALID_FULLSCREEN; }
    inline void TurnValidFullscreenOff(void) { ValidFlags &= ~DWF_VALID_FULLSCREEN; }
    inline void TurnValidFrontOn(void)       { ValidFlags |= DWF_VALID_FRONT; }
    inline void TurnValidFrontOff(void)      { ValidFlags &= ~DWF_VALID_FRONT; }
    inline void TurnValidBackOn(void)        { ValidFlags |= DWF_VALID_BACK; }
    inline void TurnValidBackOff(void)       { ValidFlags &= ~DWF_VALID_BACK; }
    inline void TurnValidWorkOn(void)        { ValidFlags |= DWF_VALID_WORK; }
    inline void TurnValidWorkOff(void)       { ValidFlags &= ~DWF_VALID_WORK; }
    inline void TurnValidViewportOn(void)    { ValidFlags |= DWF_VALID_VIEWPORT; }
    inline void TurnValidViewportOff(void)   { ValidFlags &= ~DWF_VALID_VIEWPORT; }

    inline BOOL IsValidDefaults(void)   { return ((CurrDriver && CurrMode && CurrDevice) ? UC_TRUE : UC_FALSE); }
    inline BOOL IsValidInterface(void)  { return ((ValidFlags & DWF_VALID_INTERFACE)  ? UC_TRUE : UC_FALSE); }
    inline BOOL IsValidFullscreen(void) { return ((ValidFlags & DWF_VALID_FULLSCREEN) ? UC_TRUE : UC_FALSE); }
    inline BOOL IsValidFront(void)      { return ((ValidFlags & DWF_VALID_FRONT)      ? UC_TRUE : UC_FALSE); }
    inline BOOL IsValidBack(void)       { return ((ValidFlags & DWF_VALID_BACK)       ? UC_TRUE : UC_FALSE); }
    inline BOOL IsValidWork(void)       { return ((ValidFlags & DWF_VALID_WORK)       ? UC_TRUE : UC_FALSE); }
    inline BOOL IsValidViewport(void)   { return ((ValidFlags & DWF_VALID_VIEWPORT)   ? UC_TRUE : UC_FALSE); }

    UBYTE* background_image_mem;

public:
    ULONG BackColour;
    D3DDeviceInfo*   CurrDevice;
    D3DMATERIALHANDLE black_handle, white_handle, user_handle;
    D3DRECT           ViewportRect;
    D3DTexture*       TextureList;
    DDDriverInfo*     CurrDriver;
    DDModeInfo*       CurrMode;
    GUID*             DDGuid;
    LPDIRECT3D3       lp_D3D;
    LPDIRECT3DDEVICE3 lp_D3D_Device;
    LPDIRECT3DMATERIAL3  lp_D3D_Black, lp_D3D_White, lp_D3D_User;
    LPDIRECT3DVIEWPORT3  lp_D3D_Viewport;
    LPDIRECTDRAW         lp_DD;
    LPDIRECTDRAW4        lp_DD4;
    LPDIRECTDRAWSURFACE4 lp_DD_FrontSurface, lp_DD_BackSurface, lp_DD_WorkSurface, lp_DD_ZBuffer;
    LPDIRECTDRAWSURFACE4 lp_DD_Background, lp_DD_Background_use_instead;
    IDirectDrawGammaControl* lp_DD_GammaControl;
    RECT DisplayRect;

    // Pixel format info for direct framebuffer access (only valid when screen is locked).
    SLONG mask_red, mask_green, mask_blue;
    SLONG shift_red, shift_green, shift_blue;
    SLONG screen_width, screen_height, screen_pitch, screen_bbp;
    UBYTE* screen;

    volatile ULONG DisplayFlags;

    Display();
    ~Display();

    HRESULT Init(void);
    HRESULT Fini(void);

    HRESULT GenerateDefaults(void);

    HRESULT InitInterfaces(void);
    HRESULT FiniInterfaces(void);

    HRESULT InitWindow(void);
    HRESULT FiniWindow(void);

    HRESULT InitFullscreenMode(void);
    HRESULT FiniFullscreenMode(void);

    HRESULT InitFront(void);
    HRESULT FiniFront(void);

    HRESULT InitBack(void);
    HRESULT FiniBack(void);

    HRESULT InitViewport(void);
    HRESULT FiniViewport(void);
    HRESULT UpdateViewport(void);

    void RunFMV();
    void RunCutscene(int which, int language = 0, bool bAllowButtonsToExit = UC_TRUE);

    HRESULT ChangeMode(SLONG w, SLONG h, SLONG bpp, SLONG refresh);

    bool IsGammaAvailable();
    void SetGamma(int black = 0, int white = 256);
    void GetGamma(int* black, int* white);

    HRESULT Restore(void);

    // Texture tracking: the display keeps a list of loaded textures to restore after mode changes.
    HRESULT AddLoadedTexture(D3DTexture* the_texture);
    void    RemoveAllLoadedTextures(void);
    HRESULT FreeLoadedTextures(void);
    HRESULT ReloadTextures(void);

    HRESULT toGDI(void);
    HRESULT fromGDI(void);

    void* screen_lock(void);
    void  screen_unlock(void);

    HRESULT ShowWorkScreen(void);
    void    blit_back_buffer(void);

    void SetUserColour(UBYTE red, UBYTE green, UBYTE blue);

    inline HRESULT SetBlackBackground(void) {
        BackColour = BK_COL_BLACK;
        return lp_D3D_Viewport->SetBackground(black_handle);
    }
    inline HRESULT SetWhiteBackground(void) {
        BackColour = BK_COL_WHITE;
        return lp_D3D_Viewport->SetBackground(white_handle);
    }
    inline HRESULT SetUserBackground(void) {
        BackColour = BK_COL_USER;
        return lp_D3D_Viewport->SetBackground(user_handle);
    }
    inline HRESULT BeginScene(void)  { return lp_D3D_Device->BeginScene(); }
    inline HRESULT EndScene(void)    { return lp_D3D_Device->EndScene(); }
    inline HRESULT ClearViewport(void) {
        return lp_D3D_Viewport->Clear(1, &ViewportRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    }

    inline HRESULT SetRenderState(D3DRENDERSTATETYPE type, SLONG state) {
        return lp_D3D_Device->SetRenderState(type, state);
    }
    inline HRESULT SetTexture(LPDIRECT3DTEXTURE2 tex) {
        return lp_D3D_Device->SetTexture(0, tex);
    }
    inline HRESULT SetTextureState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD dw) {
        return lp_D3D_Device->SetTextureStageState(stage, type, dw);
    }

    HRESULT Flip(LPDIRECTDRAWSURFACE4 alt, SLONG flags);

    inline HRESULT DrawPrimitive(D3DPRIMITIVETYPE p_type, DWORD dwVertexDesc, LPVOID v, DWORD v_count, DWORD flags) {
        return lp_D3D_Device->DrawPrimitive(p_type, dwVertexDesc, v, v_count, flags);
    }
    inline HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE p_type, DWORD dwVertexDesc, LPVOID v, DWORD v_count, LPWORD i, DWORD i_count, DWORD flags) {
        return lp_D3D_Device->DrawIndexedPrimitive(p_type, dwVertexDesc, v, v_count, i, i_count, flags);
    }

    inline BOOL IsValid(void) {
        return (((ValidFlags & DWF_VALID) == DWF_VALID) ? UC_TRUE : UC_FALSE);
    }

    inline BOOL IsDisplayChanged(void)   { return (((AttribFlags & DWF_DISPLAY_CHANGED)  == DWF_DISPLAY_CHANGED)  ? UC_TRUE : UC_FALSE); }
    inline void DisplayChangedOn(void)   { AttribFlags |= DWF_DISPLAY_CHANGED; }
    inline void DisplayChangedOff(void)  { AttribFlags &= ~DWF_DISPLAY_CHANGED; }

    inline BOOL IsTexturesInvalid(void)   { return (((AttribFlags & DWF_TEXTURES_INVALID) == DWF_TEXTURES_INVALID) ? UC_TRUE : UC_FALSE); }
    inline void TexturesInvalidOn(void)   { AttribFlags |= DWF_TEXTURES_INVALID; }
    inline void TexturesInvalidOff(void)  { AttribFlags &= ~DWF_TEXTURES_INVALID; }

    inline BOOL IsFullScreen(void)  { return (AttribFlags & DWF_FULLSCREEN); }
    inline void FullScreenOn(void)  { AttribFlags |= DWF_FULLSCREEN; }
    inline void FullScreenOff(void) { AttribFlags &= ~DWF_FULLSCREEN; }

    inline BOOL IsCreateZBuffer(void)  { return (AttribFlags & DWF_ZBUFFER); }
    inline void CreateZBufferOn(void)  { AttribFlags |= DWF_ZBUFFER; }
    inline void CreateZBufferOff(void) { AttribFlags &= ~DWF_ZBUFFER; }

    inline BOOL IsUse3D(void)   { return (AttribFlags & DWF_USE_3D); }
    inline void Use3DOn(void)   { AttribFlags |= DWF_USE_3D; }
    inline void Use3DOff(void)  { AttribFlags &= ~DWF_USE_3D; }

    inline BOOL IsUseWork(void)   { return (AttribFlags & DWF_USE_WORK); }
    inline void UseWorkOn(void)   { AttribFlags |= DWF_USE_WORK; }
    inline void UseWorkOff(void)  { AttribFlags &= ~DWF_USE_WORK; }

    inline BOOL IsInitialised(void) { return DisplayFlags & DISPLAY_INIT; }
    inline void InitOn(void)  { DisplayFlags |= DISPLAY_INIT; }
    inline void InitOff(void) { DisplayFlags &= ~DISPLAY_INIT; }

    inline DDDriverInfo*   GetDriverInfo(void) { return CurrDriver; }
    inline DDModeInfo*     GetModeInfo(void)   { return CurrMode; }
    inline D3DDeviceInfo*  GetDeviceInfo(void) { return CurrDevice; }

    // Packs r,g,b into the current framebuffer pixel format.
    inline ULONG GetFormattedPixel(UBYTE r, UBYTE g, UBYTE b) {
        return ((r >> mask_red) << shift_red) | ((g >> mask_green) << shift_green) | ((b >> mask_blue) << shift_blue);
    }

    // Direct framebuffer pixel access — only valid when screen is locked via screen_lock().
    void PlotPixel(SLONG x, SLONG y, UBYTE red, UBYTE green, UBYTE blue);
    void PlotFormattedPixel(SLONG x, SLONG y, ULONG colour);
    void GetPixel(SLONG x, SLONG y, UBYTE* red, UBYTE* green, UBYTE* blue);

    // Background surface management.
    void create_background_surface(UBYTE* image_data);
    void use_this_background_surface(LPDIRECTDRAWSURFACE4 this_one);
    void blit_background_surface(bool b3DInFrame = UC_TRUE);
    void destroy_background_surface(void);
};

// uc_orig: DisplayWidth (fallen/DDLibrary/Headers/GDisplay.h)
// Fixed display width — always 640 in this build.
#define DisplayWidth  640
// uc_orig: DisplayHeight (fallen/DDLibrary/Headers/GDisplay.h)
// Fixed display height — always 480 in this build.
#define DisplayHeight 480

// uc_orig: RealDisplayWidth (fallen/DDLibrary/Headers/GDisplay.h)
extern SLONG RealDisplayWidth;
// uc_orig: RealDisplayHeight (fallen/DDLibrary/Headers/GDisplay.h)
extern SLONG RealDisplayHeight;
// uc_orig: DisplayBPP (fallen/DDLibrary/Headers/GDisplay.h)
extern SLONG DisplayBPP;
// uc_orig: the_display (fallen/DDLibrary/Headers/GDisplay.h)
// Global Display singleton. All rendering goes through this object.
extern Display the_display;
// uc_orig: hDDLibStyle (fallen/DDLibrary/Headers/GDisplay.h)
extern volatile SLONG hDDLibStyle;
// uc_orig: hDDLibStyleEx (fallen/DDLibrary/Headers/GDisplay.h)
extern volatile SLONG hDDLibStyleEx;
// uc_orig: hDDLibMenu (fallen/DDLibrary/Headers/GDisplay.h)
extern volatile HMENU hDDLibMenu;
// uc_orig: hDDLibWindow (fallen/DDLibrary/Headers/GDisplay.h)
// Handle to the application window.
extern volatile HWND hDDLibWindow;

#endif // ENGINE_GRAPHICS_GRAPHICS_API_GD_DISPLAY_H
