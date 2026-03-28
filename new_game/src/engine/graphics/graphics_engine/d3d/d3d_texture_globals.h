#ifndef ENGINE_GRAPHICS_GRAPHICS_API_D3D_TEXTURE_GLOBALS_H
#define ENGINE_GRAPHICS_GRAPHICS_API_D3D_TEXTURE_GLOBALS_H

#include "engine/graphics/graphics_engine/d3d/d3d_texture.h"

// D3DTexture array — the core texture storage. Game code uses ge_* / TEXTURE_get_handle().
// uc_orig: TEXTURE_texture (fallen/DDEngine/Source/texture.cpp)
#define TEXTURE_NUM_STANDARD (22 * 64)
#define TEXTURE_MAX_TEXTURES_D3D (TEXTURE_NUM_STANDARD + 160)
extern D3DTexture TEXTURE_texture[TEXTURE_MAX_TEXTURES_D3D];

// Source D3DTexture being embedded into a page during the current load batch.
// uc_orig: EmbedSource (fallen/DDLibrary/Source/D3DTexture.cpp)
extern D3DTexture* EmbedSource;

// DirectDraw surface of the embed target page.
// uc_orig: EmbedSurface (fallen/DDLibrary/Source/D3DTexture.cpp)
extern LPDIRECTDRAWSURFACE4 EmbedSurface;

// Direct3D texture interface of the embed target page.
// uc_orig: EmbedTexture (fallen/DDLibrary/Source/D3DTexture.cpp)
extern LPDIRECT3DTEXTURE2 EmbedTexture;

// Current subtexture slot offset being written into the embed page.
// uc_orig: EmbedOffset (fallen/DDLibrary/Source/D3DTexture.cpp)
extern UBYTE EmbedOffset;

// Allocated size in bytes of the fast-load scratch buffer.
// uc_orig: dwSizeOfFastLoadBuffer (fallen/DDLibrary/Source/D3DTexture.cpp)
extern DWORD dwSizeOfFastLoadBuffer;

// Pointer to the fast-load scratch buffer (VirtualAlloc'd, grows on demand).
// uc_orig: pvFastLoadBuffer (fallen/DDLibrary/Source/D3DTexture.cpp)
extern void* pvFastLoadBuffer;

// True once the texture page system has been initialised.
// uc_orig: m_bTexturePagesInitialised (fallen/DDLibrary/Source/D3DTexture.cpp)
extern bool m_bTexturePagesInitialised;

#endif // ENGINE_GRAPHICS_GRAPHICS_API_D3D_TEXTURE_GLOBALS_H
