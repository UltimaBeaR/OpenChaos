#ifndef ENGINE_GRAPHICS_LIGHTING_SHADOW_H
#define ENGINE_GRAPHICS_LIGHTING_SHADOW_H

#include "engine/core/types.h"

// Static shadow pre-computation for city geometry.
// Computes 3-bit shadow values for each map square and face,
// simulating a fixed directional light source from the top-right.
//
// Shadow value bits encode which adjacent squares are taller (casting shadow):
//   bit0 = left neighbour taller, bit1 = front neighbour taller,
//   bit2 = left-front neighbour taller → lookup into shadow[8] gradient table.

// uc_orig: SHADOW_do (fallen/Source/shadow.cpp)
// Pre-compute all static shadows across the current city: floor squares, roof tiles,
// roof faces, and walkable prim faces. Called once after the city is built.
void SHADOW_do(void);

// uc_orig: SHADOW_in (fallen/Source/shadow.cpp)
// Returns non-zero if the given world point is in shadow (i.e. blocked from the sun).
// Uses a line-of-sight ray cast toward the fixed light direction.
SLONG SHADOW_in(SLONG x, SLONG y, SLONG z);

#endif // ENGINE_GRAPHICS_LIGHTING_SHADOW_H
