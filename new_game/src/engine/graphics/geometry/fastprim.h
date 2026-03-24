#ifndef ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_H
#define ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_H

// Fast prim renderer: caches D3D vertex/index buffers per prim object and renders them
// with DrawIndPrimMM or DrawIndexedPrimitive, skipping per-frame geometry rebuild.

#include "engine/platform/platform.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"

// uc_orig: FASTPRIM_init (fallen/DDEngine/Headers/fastprim.h)
void FASTPRIM_init(void);

// uc_orig: FASTPRIM_draw (fallen/DDEngine/Headers/fastprim.h)
SLONG FASTPRIM_draw(
    SLONG prim,
    float x,
    float y,
    float z,
    float matrix[9],
    NIGHT_Colour* lpc);

// uc_orig: FASTPRIM_fini (fallen/DDEngine/Headers/fastprim.h)
void FASTPRIM_fini(void);

#endif // ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_H
