#include "engine/graphics/graphics_engine/backend_directx6/common/d3d_texture_globals.h"

// uc_orig: TEXTURE_texture (fallen/DDEngine/Source/texture.cpp)
D3DTexture TEXTURE_texture[TEXTURE_MAX_TEXTURES_D3D];

// uc_orig: EmbedSource (fallen/DDLibrary/Source/D3DTexture.cpp)
D3DTexture* EmbedSource = nullptr;

// uc_orig: EmbedSurface (fallen/DDLibrary/Source/D3DTexture.cpp)
LPDIRECTDRAWSURFACE4 EmbedSurface = nullptr;

// uc_orig: EmbedTexture (fallen/DDLibrary/Source/D3DTexture.cpp)
LPDIRECT3DTEXTURE2 EmbedTexture = nullptr;

// uc_orig: EmbedOffset (fallen/DDLibrary/Source/D3DTexture.cpp)
UBYTE EmbedOffset = 0;

// uc_orig: dwSizeOfFastLoadBuffer (fallen/DDLibrary/Source/D3DTexture.cpp)
DWORD dwSizeOfFastLoadBuffer = 0;

// uc_orig: pvFastLoadBuffer (fallen/DDLibrary/Source/D3DTexture.cpp)
void* pvFastLoadBuffer = nullptr;

// uc_orig: m_bTexturePagesInitialised (fallen/DDLibrary/Source/D3DTexture.cpp)
bool m_bTexturePagesInitialised = false;
