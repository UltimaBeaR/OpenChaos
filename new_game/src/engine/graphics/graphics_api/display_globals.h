#ifndef ENGINE_GRAPHICS_GRAPHICS_API_DISPLAY_GLOBALS_H
#define ENGINE_GRAPHICS_GRAPHICS_API_DISPLAY_GLOBALS_H

#include "engine/graphics/graphics_api/gd_display.h"

// uc_orig: VideoRes (fallen/DDLibrary/Source/GDisplay.cpp)
// Resolution index: 0=320x240, 1=512x384, 2=640x480, 3=800x600, 4=1024x768, -1=unknown.
extern int VideoRes;

// uc_orig: image_mem (fallen/DDLibrary/Source/GDisplay.cpp)
// Heap buffer for the background image. Also accessed by planmap.cpp for map screenshots.
extern UBYTE* image_mem;

// uc_orig: image (fallen/DDLibrary/Source/GDisplay.cpp)
// Pointer into image_mem used during background image loading.
extern UBYTE* image;

// uc_orig: m_lpLastBackground (fallen/DDLibrary/Source/GDisplay.cpp)
// Last background surface used by the display; reset on mode change.
extern LPDIRECTDRAWSURFACE4 m_lpLastBackground;

// uc_orig: clumpfile (fallen/DDLibrary/Source/GDisplay.cpp)
// TGA clump filename, saved by SetLastClumpfile for ReloadTextures after mode change.
extern char clumpfile[MAX_PATH];
// uc_orig: clumpsize (fallen/DDLibrary/Source/GDisplay.cpp)
// Size of the TGA clump in bytes.
extern size_t clumpsize;

// Non-static standalone functions from GDisplay.cpp not declared in gd_display.h.
// uc_orig: SetLastClumpfile (fallen/DDLibrary/Source/GDisplay.cpp)
// Records the TGA clump filename so ReloadTextures() can reopen it after a mode change.
void SetLastClumpfile(char* file, size_t size);
// uc_orig: CopyBackground (fallen/DDLibrary/Source/GDisplay.cpp)
// Copies a 640x480x24 RGB image into a DirectDraw surface (scales to match surface size).
void CopyBackground(UBYTE* image_data, IDirectDrawSurface4* surface);
// uc_orig: CopyBackground32 (fallen/DDLibrary/Source/GDisplay.cpp)
// Same as CopyBackground but always uses 32-bit pixel format path.
void CopyBackground32(UBYTE* image_data, IDirectDrawSurface4* surface);
// uc_orig: PlayQuickMovie (fallen/DDLibrary/Source/GDisplay.cpp)
// Plays a Bink video: type=0 → intro sequence, type>0 → cutscene #type.
void PlayQuickMovie(SLONG type, SLONG language_ignored, bool bIgnored);

#endif // ENGINE_GRAPHICS_GRAPHICS_API_DISPLAY_GLOBALS_H
