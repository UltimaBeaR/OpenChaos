#ifndef ENGINE_GRAPHICS_GRAPHICS_API_D3D_TEXTURE_H
#define ENGINE_GRAPHICS_GRAPHICS_API_D3D_TEXTURE_H

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "engine/io/file.h"

// uc_orig: NotGoingToLoadTexturesForAWhileNowSoYouCanCleanUpABit (fallen/DDLibrary/Headers/D3DTexture.h)
// Releases the fast-load scratch buffer. Call after a batch of texture loads to reclaim memory.
void NotGoingToLoadTexturesForAWhileNowSoYouCanCleanUpABit(void);

// uc_orig: FastLoadFileSomewhere (fallen/DDLibrary/Headers/D3DTexture.h)
// Reads dwSize bytes from handle into the fast-load scratch buffer and returns a pointer to it.
// The returned pointer is valid only until the next call to this function.
void* FastLoadFileSomewhere(MFFileHandle handle, DWORD dwSize);

// Frees all D3D texture pages and resets render states.
// Defined in d3d_texture.cpp; declared here so migrated callers can include this header.
// uc_orig: FreeAllD3DPages (fallen/DDLibrary/Source/D3DTexture.cpp)
void FreeAllD3DPages(void);

// uc_orig: OS_calculate_mask_and_shift (fallen/DDLibrary/Source/D3DTexture.cpp)
// Given a pixel format bitmask, computes the right-shift mask and left-shift needed to
// pack an 8-bit colour component into that field.
// Formula: pixel_field = (component >> mask) << shift
void OS_calculate_mask_and_shift(ULONG bitmask, SLONG* mask, SLONG* shift);

// Font/Char structs now defined in game_graphics_engine.h (GEFont/GEFontChar with aliases).
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

// Texture flags stored in D3DTexture::TextureFlags.
// uc_orig: D3D_TEXTURE_FONT (fallen/DDLibrary/Headers/D3DTexture.h)
#define D3D_TEXTURE_FONT (1 << 0)
// uc_orig: D3D_TEXTURE_FONT2 (fallen/DDLibrary/Headers/D3DTexture.h)
// Texture is a font with red border pixels that should be made transparent.
#define D3D_TEXTURE_FONT2 (1 << 1)

// Values for D3DTexture::Type.
// uc_orig: D3DTEXTURE_TYPE_UNUSED (fallen/DDLibrary/Headers/D3DTexture.h)
#define D3DTEXTURE_TYPE_UNUSED 0
// uc_orig: D3DTEXTURE_TYPE_TGA (fallen/DDLibrary/Headers/D3DTexture.h)
#define D3DTEXTURE_TYPE_TGA    1
// uc_orig: D3DTEXTURE_TYPE_USER (fallen/DDLibrary/Headers/D3DTexture.h)
// The caller writes pixel data directly; no backing file.
#define D3DTEXTURE_TYPE_USER   2

// Values for D3DTexture::bPageType / D3DPage::bPageType.
// First number: subtexture size in pixels. Second: grid layout (NxN subtextures per page).
// _3X3 variants have 32-pixel padding gaps; _4X4 variants are tightly packed.
// uc_orig: D3DPAGE_NONE (fallen/DDLibrary/Headers/D3DTexture.h)
#define D3DPAGE_NONE    0
// uc_orig: D3DPAGE_64_3X3 (fallen/DDLibrary/Headers/D3DTexture.h)
#define D3DPAGE_64_3X3  1
// uc_orig: D3DPAGE_64_4X4 (fallen/DDLibrary/Headers/D3DTexture.h)
#define D3DPAGE_64_4X4  2
// uc_orig: D3DPAGE_32_3X3 (fallen/DDLibrary/Headers/D3DTexture.h)
#define D3DPAGE_32_3X3  3
// uc_orig: D3DPAGE_32_4X4 (fallen/DDLibrary/Headers/D3DTexture.h)
#define D3DPAGE_32_4X4  4

// uc_orig: D3DTexture (fallen/DDLibrary/Headers/D3DTexture.h)
// Manages a single DirectDraw/Direct3D texture surface. Supports TGA file-backed textures,
// procedural user-written textures, and packed texture pages. Also handles font extraction.
class D3DTexture {
public:
    CBYTE texture_name[256];
    ULONG ID;             // File clump texture ID.
    BOOL bCanShrink;      // Allow the texture to be downscaled for faster loading.
    SLONG TextureFlags;
    Font* FontList;
    LPDIRECT3DTEXTURE2 lp_Texture;
    LPDIRECTDRAWSURFACE4 lp_Surface;
    HRESULT Reload_TGA(void);
    HRESULT Reload_user(void);
    BOOL GreyScale;
    BOOL UserWantsAlpha;  // The user page requires an alpha channel.
    UBYTE bPagePos;       // Subtexture slot index within a texture page.
    UBYTE bPageType;      // One of D3DPAGE_xxx.
    WORD wPageNum;        // Index of the D3DPage this texture belongs to; -1 if none.

    D3DTexture* NextTexture;

    D3DTexture()
    {
        TextureFlags = 0;
        FontList = NULL;
        NextTexture = NULL;
        lp_Surface = NULL;
        lp_Texture = NULL;
        Type = D3DTEXTURE_TYPE_UNUSED;
        wPageNum = -1;
        bPagePos = 0;
        bPageType = D3DPAGE_NONE;
        GreyScale = UC_FALSE;
        UserWantsAlpha = UC_FALSE;
        ID = -1;
        bCanShrink = UC_FALSE;
    }

    // Pixel format masks and shifts for packing RGB(A) components into the surface pixel format.
    SLONG mask_red = 0;
    SLONG mask_green = 0;
    SLONG mask_blue = 0;
    SLONG mask_alpha = 0;

    SLONG shift_red = 0;
    SLONG shift_green = 0;
    SLONG shift_blue = 0;
    SLONG shift_alpha = 0;

    SLONG Type;
    SLONG size;           // Width (and height) of the texture page in pixels.
    SLONG ContainsAlpha;

    // Load a TGA file as a managed texture. texid is the file-clump ID (-1 if none).
    HRESULT LoadTextureTGA(CBYTE* tga_file, ULONG texid, BOOL bCanShrink = UC_TRUE);

    // Replace the current TGA texture with a different file, reloading immediately.
    HRESULT ChangeTextureTGA(CBYTE* tga_file);

    // Create a blank user-writable texture of the given size (power of two, 32–256).
    HRESULT CreateUserPage(SLONG size, BOOL i_want_an_alpha_channel);

    // Lock the surface for direct pixel writes. pitch is in bytes.
    HRESULT LockUser(UWORD** bitmap, SLONG* pitch);
    void UnlockUser(void);

    // Reload (or re-create) the underlying DirectDraw surface from source data.
    HRESULT Reload(void);

    // Release all DirectDraw/Direct3D resources. Does not free the D3DTexture object itself.
    HRESULT Destroy(void);

    // Return the Font at position id in the linked list (0 = first font).
    Font* GetFont(SLONG id);

    // Reset per-session embedding state before a new batch of texture loads.
    static void BeginLoading();

    // Enable or disable greyscale conversion. If the texture is already loaded, it is reloaded.
    void set_greyscale(BOOL is_greyscale);

    LPDIRECT3DTEXTURE2 GetD3DTexture() { return lp_Texture; }
    LPDIRECTDRAWSURFACE4 GetSurface(void) { return lp_Surface; }

    // Compute UV scale and offset into the packed page for this subtexture slot.
    void GetTexOffsetAndScale(float* pfUScale, float* pfUOffset, float* pfVScale, float* pfVOffset);

    HRESULT SetColorKey(SLONG flags, LPDDCOLORKEY key)
    {
        if (lp_Surface) {
            return lp_Surface->SetColorKey(flags, key);
        } else {
            return DDERR_GENERIC;
        }
    }

    inline BOOL IsFont(void) { return TextureFlags & D3D_TEXTURE_FONT; }
    inline void FontOn(void) { TextureFlags |= D3D_TEXTURE_FONT; }
    inline void FontOff(void) { TextureFlags &= ~D3D_TEXTURE_FONT; }

    inline BOOL IsFont2(void) { return TextureFlags & D3D_TEXTURE_FONT2; }
    inline void Font2On(void) { TextureFlags |= D3D_TEXTURE_FONT2; }
    inline void Font2Off(void) { TextureFlags &= ~D3D_TEXTURE_FONT2; }
};

// uc_orig: D3DPage (fallen/DDLibrary/Headers/D3DTexture.h)
// Represents a texture page that packs multiple subtextures. Demand-loads the underlying
// D3DTexture when first needed and unloads it on shutdown.
class D3DPage {
public:
    UBYTE bPageType;          // One of D3DPAGE_xxx.
    UBYTE bNumTextures;       // Number of subtextures packed into this page.
    char* pcDirectory;        // Page directory path. Points to static storage — do not free.
    char* pcFilename;         // Page TGA filename. Points to static storage — do not free.
    char** ppcTextureList;    // Array of pointers to subtexture name strings.
    D3DTexture* pTex;         // The texture page surface itself.

    D3DPage(void)
    {
        bPageType = 0;
        bNumTextures = 0;
        pcDirectory = NULL;
        pcFilename = NULL;
        pTex = NULL;
        ppcTextureList = NULL;
    }

    // Demand-load the page texture if it has not been loaded yet.
    void D3DPage::EnsureLoaded(void);

    // Release the page texture. Call during a full texture unload.
    void D3DPage::Unload(void);
};

#endif // ENGINE_GRAPHICS_GRAPHICS_API_D3D_TEXTURE_H
