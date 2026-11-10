#ifndef EFFECTS_ENVIRONMENT_SNOWFLAKE_H
#define EFFECTS_ENVIRONMENT_SNOWFLAKE_H

#include "world_objects/dirt.h"

// Orientation of a snowflake triangle (DIRT_TYPE_SNOW).
//
// Snowflakes share the leaf code, which draws a triangle lying flat on the
// ground (tilted by pitch/roll while airborne). Seen from a low camera a
// falling flake is then a thin sliver instead of a flake. This replaces the
// three world-space corners so that a flake in the air faces the camera, and
// in the last moments of its fall tips over until it lies flat on the ground.
// Resting flakes lie flat.
//
// size - half-extent of the triangle, as used by the leaf code.
// up   - the leaf code's height offset above the dirt position.
// wx/wy/wz - the three corners, overwritten in the leaf code's vertex order.
void SNOWFLAKE_calc_corners(const DIRT_Dirt* dd, float size, float up, float wx[3], float wy[3], float wz[3]);

#endif // EFFECTS_ENVIRONMENT_SNOWFLAKE_H
