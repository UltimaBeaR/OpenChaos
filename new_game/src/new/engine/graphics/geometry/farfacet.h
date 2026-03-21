#ifndef ENGINE_GRAPHICS_GEOMETRY_FARFACET_H
#define ENGINE_GRAPHICS_GEOMETRY_FARFACET_H

#include "core/types.h"

// FarFacet: LOD renderer for distant building walls.
// At init time, building wall (DFacet) geometry is merged and pre-built into
// indexed D3D vertex buffers organized in a 2D grid of "far squares".
// At draw time only the squares in the view frustum that are beyond the standard
// draw distance are rendered, using pre-built DrawIndexedPrimitive calls.

// uc_orig: FARFACET_init (fallen/DDEngine/Headers/farfacet.h)
// Allocates memory, builds geometry from the currently loaded facet/map data.
// Call once after all building assets are loaded.
void FARFACET_init(void);

// uc_orig: FARFACET_draw (fallen/DDEngine/Headers/farfacet.h)
// Draws all far-facet squares visible from the given camera position and orientation.
void FARFACET_draw(
    float x,
    float y,
    float z,
    float yaw,
    float pitch,
    float roll,
    float draw_dist,
    float lens);

// uc_orig: FARFACET_fini (fallen/DDEngine/Headers/farfacet.h)
// Frees all memory allocated by FARFACET_init.
void FARFACET_fini(void);

#endif // ENGINE_GRAPHICS_GEOMETRY_FARFACET_H
