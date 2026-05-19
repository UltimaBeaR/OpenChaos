#ifndef ENGINE_GRAPHICS_LIGHTING_SMAP_H
#define ENGINE_GRAPHICS_LIGHTING_SMAP_H

#include "engine/core/types.h"
// SVector_F is used in the public API of SMAP_project_onto_poly.
#include "engine/graphics/pipeline/aeng.h"

struct Thing;

// A clipped polygon link used when projecting a shadow map onto world geometry.
// Points form a NULL-terminated singly-linked list.
// uc_orig: smap_link (fallen/DDEngine/Headers/smap.h)
typedef struct smap_link {
    float wx;
    float wy;
    float wz;
    float u;
    float v;

    struct smap_link* next;

    ULONG clip; // Private — used during clipping

} SMAP_Link;

// Milestone 1E (Phase B): GPU character shadow silhouette. Self-contained
// (own bone walk, works in any situation incl. cutscenes), sets the
// SMAP_* projection globals (so SMAP_project_onto_poly stays consistent)
// from a STABLE box (model bounding radius, pelvis-centred —
// pose-independent), and renders the silhouette on the GPU into texture
// page `tex_page` sub-rect (off_x,off_y,res,res). Returns false (doing
// nothing) if the model is not ready that frame (no shadow that frame —
// rare/transient; there is NO CPU fallback, shadows are 100% GPU).
// uc_orig: none (replaced the removed CPU SMAP_person).
bool SMAP_person_gpu(
    Thing* person,
    SLONG tex_page,
    SLONG off_x,
    SLONG off_y,
    UBYTE res,
    SLONG light_dx,
    SLONG light_dy,
    SLONG light_dz);

// Milestone 1E: destroy all cached per-prim shadow meshes. MUST be called
// when the prim pools are reloaded (clear_prims) — stale GPU meshes after
// a level load were a corruption source. uc_orig: none.
void SMAP_shadow_prim_cache_reset(void);

// Projects the shadow map rendered by SMAP_person_gpu() onto a convex polygon in world space.
// poly must be given in clockwise order. Returns NULL if the polygon is behind the light
// or outside the shadow map's bounds. The returned linked list is valid only until the
// next call to this function.
// uc_orig: SMAP_project_onto_poly (fallen/DDEngine/Headers/smap.h)
SMAP_Link* SMAP_project_onto_poly(
    SVector_F poly[],
    SLONG num_points);

#endif // ENGINE_GRAPHICS_LIGHTING_SMAP_H
