#ifndef BUILDINGS_BUILD_H
#define BUILDINGS_BUILD_H

// Forward declaration: Thing is defined in fallen/Headers/thing.h (not yet migrated).
struct Thing;

// Draws a building thing using its associated prim data and current lighting.
// Assumes POLY module and camera are set up before calling.
// uc_orig: BUILD_draw (fallen/DDEngine/Headers/build.h)
void BUILD_draw(Thing* building);

// Draws the interior of the building the player is currently inside.
// Currently a no-op (entire body is commented out in the pre-release codebase).
// uc_orig: BUILD_draw_inside (fallen/DDEngine/Headers/build.h)
void BUILD_draw_inside(void);

#endif // BUILDINGS_BUILD_H
