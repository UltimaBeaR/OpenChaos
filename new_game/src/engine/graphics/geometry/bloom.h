#ifndef ENGINE_GRAPHICS_GEOMETRY_BLOOM_H
#define ENGINE_GRAPHICS_GEOMETRY_BLOOM_H

#include "core/types.h"

// Option flags for BLOOM_draw.
// uc_orig: BLOOM_GLOW_ALWAYS (fallen/DDEngine/Headers/DrawXtra.h)
#define BLOOM_GLOW_ALWAYS 1
// uc_orig: BLOOM_LENSFLARE (fallen/DDEngine/Headers/DrawXtra.h)
#define BLOOM_LENSFLARE 2
// uc_orig: BLOOM_BEAM (fallen/DDEngine/Headers/DrawXtra.h)
#define BLOOM_BEAM 4
// uc_orig: BLOOM_FAINT (fallen/DDEngine/Headers/DrawXtra.h)
#define BLOOM_FAINT 8
// uc_orig: BLOOM_NOGLOW (fallen/DDEngine/Headers/DrawXtra.h)
#define BLOOM_NOGLOW 16
// uc_orig: BLOOM_RAISE_LOS (fallen/DDEngine/Headers/DrawXtra.h)
#define BLOOM_RAISE_LOS 32

// Convenience composite: faint lens flare (half brightness, no beam).
// uc_orig: BLOOM_FLENSFLARE (fallen/DDEngine/Headers/DrawXtra.h)
#define BLOOM_FLENSFLARE (BLOOM_LENSFLARE | BLOOM_FAINT)

// Draws a lens-flare effect at the given world position.
// str is overall brightness (0-255). Flare is composed of 27 coloured quads
// spread along the screen-space axis between the light and the screen centre.
// uc_orig: BLOOM_flare_draw (fallen/DDEngine/Source/drawxtra.cpp)
void BLOOM_flare_draw(SLONG x, SLONG y, SLONG z, SLONG str);

// Draws a light bloom (glow + optional lens flare + optional beam) at a world position.
// dx/dy/dz is the light's direction normal scaled so 1.0 == 256; pass (0,0,0) for omni lights.
// col is 0x00RRGGBB. opts is a combination of BLOOM_* flags.
// uc_orig: BLOOM_draw (fallen/DDEngine/Source/drawxtra.cpp)
void BLOOM_draw(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, SLONG col, UBYTE opts = BLOOM_LENSFLARE | BLOOM_BEAM);

#endif // ENGINE_GRAPHICS_GEOMETRY_BLOOM_H
