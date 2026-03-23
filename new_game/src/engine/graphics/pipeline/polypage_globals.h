#ifndef ENGINE_GRAPHICS_PIPELINE_POLYPAGE_GLOBALS_H
#define ENGINE_GRAPHICS_PIPELINE_POLYPAGE_GLOBALS_H

#include "core/types.h"

// Shared index buffer used when submitting polygons to D3D.
// Large static array — shared across all PolyPage::Render and DrawSinglePoly calls.
// uc_orig: IxBuffer (fallen/DDEngine/Source/polypage.cpp)
extern UWORD IxBuffer[65536];

// Non-private copies of PolyPage scaling factors, exposed for external readers
// (e.g. figure rendering). Set by PolyPage::SetScaling alongside the static members.
// uc_orig: not_private_smiley_xscale (fallen/DDEngine/Source/polypage.cpp)
extern float not_private_smiley_xscale;
// uc_orig: not_private_smiley_yscale (fallen/DDEngine/Source/polypage.cpp)
extern float not_private_smiley_yscale;


#endif // ENGINE_GRAPHICS_PIPELINE_POLYPAGE_GLOBALS_H
