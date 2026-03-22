#ifndef ENGINE_GRAPHICS_GEOMETRY_SUPERFACET_H
#define ENGINE_GRAPHICS_GEOMETRY_SUPERFACET_H

#include "core/types.h"

// SuperFacet: optimized batched-facet renderer using DrawIndexedPrimitive.
// Accumulates building wall (DFacet) geometry into large indexed vertex buffers
// per texture page, then flushes all at once — reducing SetTexture call count.

// uc_orig: SUPERFACET_init (fallen/DDEngine/Headers/superfacet.h)
// Allocates memory for the supefacet batch buffers. Call after all assets are loaded.
void SUPERFACET_init(void);

// uc_orig: SUPERFACET_start_frame (fallen/DDEngine/Headers/superfacet.h)
// Clears the frame's batch buffers. Call once per frame before drawing.
void SUPERFACET_start_frame(void);

// uc_orig: SUPERFACET_draw (fallen/DDEngine/Headers/superfacet.h)
// Attempts to batch the given facet index into the super-fast path.
// Returns UC_FALSE if the facet cannot be handled (falls back to normal FACET_draw).
SLONG SUPERFACET_draw(SLONG facet);

// uc_orig: SUPERFACET_fini (fallen/DDEngine/Headers/superfacet.h)
// Frees supefacet batch memory. Call at program shutdown.
void SUPERFACET_fini(void);

#endif // ENGINE_GRAPHICS_GEOMETRY_SUPERFACET_H
