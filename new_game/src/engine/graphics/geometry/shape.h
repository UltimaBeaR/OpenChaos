#ifndef ENGINE_GRAPHICS_GEOMETRY_SHAPE_H
#define ENGINE_GRAPHICS_GEOMETRY_SHAPE_H

// Primitive shape rendering: spheres, lines, tripwires, shadows, balloons.
// All shapes are submitted as POLY_Point geometry through the standard POLY pipeline.

#include "core/types.h"
#include "engine/graphics/geometry/shape_globals.h"

// Temporary: OB_Info not yet migrated
#include "world/map/ob.h"
#include "world/map/ob_globals.h"

// uc_orig: SHAPE_MAX_SPARKY_POINTS (fallen/DDEngine/Headers/shape.h)
#define SHAPE_MAX_SPARKY_POINTS 16

// Draws a hemisphere: solid-colour cone of tris and quads, alpha-blended outward.
// Direction (dx,dy,dz) must be 256 units long. The apex points in that direction.
// uc_orig: SHAPE_semisphere (fallen/DDEngine/Headers/shape.h)
void SHAPE_semisphere(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG dx,
    SLONG dy,
    SLONG dz,
    SLONG radius,
    SLONG page,
    UBYTE red,
    UBYTE green,
    UBYTE blue);

// Textured variant of SHAPE_semisphere: UV coords computed from polar angle around the axis.
// uc_orig: SHAPE_semisphere_textured (fallen/DDEngine/Headers/shape.h)
void SHAPE_semisphere_textured(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG dx,
    SLONG dy,
    SLONG dz,
    SLONG radius,
    float u_mid,
    float v_mid,
    float uv_radius,
    SLONG page,
    UBYTE red,
    UBYTE green,
    UBYTE blue);

// Draws a full sphere, lit by ambient light. Flat-colour (POLY_PAGE_COLOUR).
// uc_orig: SHAPE_sphere (fallen/DDEngine/Headers/shape.h)
void SHAPE_sphere(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG radius,
    ULONG colour);

// Draws an alpha-blended sphere. Same as SHAPE_sphere but alpha channel comes from 'alpha'.
// uc_orig: SHAPE_alpha_sphere (fallen/DDEngine/Headers/shape.h)
void SHAPE_alpha_sphere(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG radius,
    ULONG colour,
    ULONG alpha);

// Draws a glowing line through a sequence of world points, rendered as screen-space quads.
// uc_orig: SHAPE_sparky_line (fallen/DDEngine/Headers/shape.h)
void SHAPE_sparky_line(
    SLONG num_points,
    SLONG px[],
    SLONG py[],
    SLONG pz[],
    ULONG colour,
    float width);

// Draws a sparkle/glitter sprite (elongated triangle) between two world points.
// uc_orig: SHAPE_glitter (fallen/DDEngine/Headers/shape.h)
void SHAPE_glitter(
    SLONG x1,
    SLONG y1,
    SLONG z1,
    SLONG x2,
    SLONG y2,
    SLONG z2,
    ULONG colour);

// Draws a tripwire as an animated quad between two world-space endpoints.
// 'along' is how far along [0..255] the wire extends (255 = full length).
// uc_orig: SHAPE_tripwire (fallen/DDEngine/Headers/shape.h)
void SHAPE_tripwire(
    SLONG x1,
    SLONG y1,
    SLONG z1,
    SLONG x2,
    SLONG y2,
    SLONG z2,
    SLONG width,
    ULONG colour,
    UWORD counter,
    UBYTE along);

// Draws a waterfall: three translucent horizontal quads falling away from the wall edge.
// uc_orig: SHAPE_waterfall (fallen/DDEngine/Headers/shape.h)
void SHAPE_waterfall(
    SLONG map_x,
    SLONG map_z,
    SLONG map_dx,
    SLONG map_dz,
    SLONG top,
    SLONG bot);

// Draws a water-droplet sprite (elongated triangle in screen space).
// uc_orig: SHAPE_droplet (fallen/DDEngine/Headers/shape.h)
void SHAPE_droplet(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG dx,
    SLONG dy,
    SLONG dz,
    ULONG colour,
    SLONG page);

// Draws a ground shadow under the prim described by the OB_Info.
// Shadow type (cylinder, box edge, four legs) is stored per-prim.
// uc_orig: SHAPE_prim_shadow (fallen/DDEngine/Headers/shape.h)
void SHAPE_prim_shadow(OB_Info* oi);

// Draws a balloon prim at the tip of the balloon's spring chain.
// Also draws the chain links as world lines.
// uc_orig: SHAPE_draw_balloon (fallen/DDEngine/Headers/shape.h)
void SHAPE_draw_balloon(SLONG balloon);

#endif // ENGINE_GRAPHICS_GEOMETRY_SHAPE_H
