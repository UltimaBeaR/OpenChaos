#ifndef ENGINE_LIGHTING_SMAP_H
#define ENGINE_LIGHTING_SMAP_H

#include "core/types.h"
// Temporary: engine/lighting/ -> engine/graphics/pipeline/ DAG violation.
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

// Generates a shadow bitmap for a person's body at the given light direction.
// bitmap: caller-allocated UBYTE[u_res*v_res] — 0 = transparent, 255 = opaque.
// uc_orig: SMAP_person (fallen/DDEngine/Headers/smap.h)
void SMAP_person(
    Thing* person,
    UBYTE* bitmap,
    UBYTE u_res,
    UBYTE v_res,
    SLONG light_dx,
    SLONG light_dy,
    SLONG light_dz);

// SMAP_bike is declared here for completeness but was never implemented.
// Dead code: no definition exists in the original or new codebase.
// uc_orig: SMAP_bike (fallen/DDEngine/Headers/smap.h)
void SMAP_bike(
    Thing* person,
    UBYTE* bitmap,
    UBYTE u_res,
    UBYTE v_res,
    SLONG light_dx,
    SLONG light_dy,
    SLONG light_dz);

// Projects the shadow map computed by SMAP_person() onto a convex polygon in world space.
// poly must be given in clockwise order. Returns NULL if the polygon is behind the light
// or outside the shadow map's bounds. The returned linked list is valid only until the
// next call to this function.
// uc_orig: SMAP_project_onto_poly (fallen/DDEngine/Headers/smap.h)
SMAP_Link* SMAP_project_onto_poly(
    SVector_F poly[],
    SLONG num_points);

#endif // ENGINE_LIGHTING_SMAP_H
