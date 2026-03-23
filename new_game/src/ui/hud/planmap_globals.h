#ifndef UI_HUD_PLANMAP_GLOBALS_H
#define UI_HUD_PLANMAP_GLOBALS_H

#include "core/types.h"

// Pointer to screen memory for direct pixel drawing.
// uc_orig: screenmem (fallen/DDEngine/Source/planmap.cpp)
extern UBYTE* screenmem;

// Clipping rectangle for the planmap drawing area.
// uc_orig: clip_left (fallen/DDEngine/Source/planmap.cpp)
extern SLONG clip_left;
// uc_orig: clip_right (fallen/DDEngine/Source/planmap.cpp)
extern SLONG clip_right;
// uc_orig: clip_top (fallen/DDEngine/Source/planmap.cpp)
extern SLONG clip_top;
// uc_orig: clip_bot (fallen/DDEngine/Source/planmap.cpp)
extern SLONG clip_bot;

// Screen layout parameters for the planmap view.
// uc_orig: screen_x (fallen/DDEngine/Source/planmap.cpp)
extern UWORD screen_x;
// uc_orig: screen_y (fallen/DDEngine/Source/planmap.cpp)
extern UWORD screen_y;
// uc_orig: screen_width (fallen/DDEngine/Source/planmap.cpp)
extern UWORD screen_width;
// uc_orig: screen_height (fallen/DDEngine/Source/planmap.cpp)
extern UWORD screen_height;
// uc_orig: block_size (fallen/DDEngine/Source/planmap.cpp)
extern UWORD block_size;
// uc_orig: screen_mx (fallen/DDEngine/Source/planmap.cpp)
extern UWORD screen_mx;
// uc_orig: screen_mz (fallen/DDEngine/Source/planmap.cpp)
extern UWORD screen_mz;
// uc_orig: screen_pitch (fallen/DDEngine/Source/planmap.cpp)
extern SLONG screen_pitch;

#endif // UI_HUD_PLANMAP_GLOBALS_H
