#ifndef ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_H
#define ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_H

// Fast prim renderer: caches D3D vertex/index buffers per prim object and renders them
// with DrawIndPrimMM or DrawIndexedPrimitive, skipping per-frame geometry rebuild.

// uc_orig: FASTPRIM_init (fallen/DDEngine/Headers/fastprim.h)
void FASTPRIM_init(void);

// uc_orig: FASTPRIM_fini (fallen/DDEngine/Headers/fastprim.h)
void FASTPRIM_fini(void);

#endif // ENGINE_GRAPHICS_GEOMETRY_FASTPRIM_H
