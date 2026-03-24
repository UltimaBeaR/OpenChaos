#ifndef ENGINE_GRAPHICS_LIGHTING_GAMUT_H
#define ENGINE_GRAPHICS_LIGHTING_GAMUT_H

#include "engine/core/types.h"

// View frustum gamut system — divides the world into a radial grid of cells
// to quickly determine which map squares are visible from the current camera.
// Used by AENG_calc_gamut to cull distant geometry.

// uc_orig: MAX_GAMUT_RADIUS (fallen/DDEngine/Headers/Gamut.h)
// Radius in grid cells of the view gamut (24 = 24x24 cell grid around camera).
#define MAX_GAMUT_RADIUS (24)

// uc_orig: GamutElement (fallen/DDEngine/Headers/Gamut.h)
// One entry in the gamut table: grid offset (DX,DZ) and corresponding angle.
typedef struct
{
    SBYTE DX,
        DZ;
    SWORD Angle;
} GamutElement;

// uc_orig: gamut_ele_pool (fallen/DDEngine/Source/Gamut.cpp)
// Pool of all GamutElement entries, sized for MAX_GAMUT_RADIUS*4 rings.
extern GamutElement gamut_ele_pool[];

// uc_orig: gamut_ele_ptr (fallen/DDEngine/Source/Gamut.cpp)
// Pointer table: gamut_ele_ptr[r] → first element in ring r.
extern GamutElement* gamut_ele_ptr[];

// uc_orig: build_gamut_table (fallen/DDEngine/Source/Gamut.cpp)
// Populates gamut_ele_pool and gamut_ele_ptr for the current MAX_GAMUT_RADIUS.
void build_gamut_table(void);

// uc_orig: draw_gamut (fallen/DDEngine/Source/Gamut.cpp)
// Debug visualisation: draws the gamut grid centred at screen (x, y).
void draw_gamut(SLONG x, SLONG y);

#endif // ENGINE_GRAPHICS_LIGHTING_GAMUT_H
