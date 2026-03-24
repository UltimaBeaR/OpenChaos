#ifndef ENGINE_GRAPHICS_GEOMETRY_CONE_H
#define ENGINE_GRAPHICS_GEOMETRY_CONE_H

#include "engine/core/types.h"

// Cone geometry: a cone shape defined by an origin point, a direction, length, and base radius.
// Cones can be clipped against planar polygons and intersected with the map's collision geometry.
// Used for light/spotlight effects rendered as triangle fans.

// Describes a single polygon used to clip a cone. The polygon is defined by an array of these.
// uc_orig: CONE_Poly (fallen/DDEngine/Headers/cone.h)
typedef struct
{
    float x;
    float y;
    float z;

} CONE_Poly;

// uc_orig: CONE_create (fallen/DDEngine/Headers/cone.h)
// Creates a new cone from origin (x,y,z) pointing in direction (dx,dy,dz).
// direction does not need to be normalized. detail is 0-256 and controls base tessellation.
void CONE_create(
    float x,
    float y,
    float z,
    float dx,
    float dy,
    float dz,
    float length,
    float radius,
    ULONG colour_start,
    ULONG colour_end,
    SLONG detail);

// uc_orig: CONE_clip (fallen/DDEngine/Headers/cone.h)
// Clips the last created cone against the given planar polygon.
// Points of the cone that penetrate the polygon are moved to the polygon surface.
void CONE_clip(
    CONE_Poly p[],
    SLONG num_points);

// uc_orig: CONE_intersect_with_map (fallen/DDEngine/Headers/cone.h)
// Walks the map cells along the cone axis and clips the cone against all collision facets found.
void CONE_intersect_with_map(void);

// uc_orig: CONE_draw (fallen/DDEngine/Headers/cone.h)
// Projects and draws the cone as a triangle fan. Uses additive blending.
void CONE_draw(void);

#endif // ENGINE_GRAPHICS_GEOMETRY_CONE_H
