#ifndef ENGINE_GRAPHICS_GEOMETRY_MESH_H
#define ENGINE_GRAPHICS_GEOMETRY_MESH_H

#include "engine/core/types.h"
#include "engine/graphics/lighting/night.h"

// uc_orig: MESH_colour_and (fallen/DDEngine/Source/mesh.cpp)
// When a face has FACE_FLAG_TINT set, all vertex colours are ANDed with this value.
extern ULONG MESH_colour_and;

// uc_orig: MESH_init (fallen/DDEngine/Headers/mesh.h)
// Precomputes random crumple (damage deformation) offsets for generic crumple levels 0..4.
// Must be called once before any MESH_draw_poly call with crumple > 0.
void MESH_init(void);

// uc_orig: MESH_set_crumple (fallen/DDEngine/Headers/mesh.h)
// Sets car zone crumple parameters for the next MESH_draw_poly call with crumple=-1.
// assignments: per-vertex zone index array (0..5). crumples: per-zone damage level array (0..4).
void MESH_set_crumple(UBYTE* assignments, UBYTE* crumples);

// uc_orig: MESH_draw_poly (fallen/DDEngine/Headers/mesh.h)
// Draws a static prim mesh at a world position with yaw/pitch/roll rotation.
// lpc: per-vertex lighting array, or NULL to use uniform ambient light at position.
// fade: alpha value 0..255 packed into high byte.
// crumple: 0=undamaged, 1..4=increasing damage levels, -1=use car zone deformation.
NIGHT_Colour* MESH_draw_poly(
    SLONG prim,
    SLONG at_x,
    SLONG at_y,
    SLONG at_z,
    SLONG i_yaw,
    SLONG i_pitch,
    SLONG i_roll,
    NIGHT_Colour* lpc,
    UBYTE fade,
    SLONG crumple = 0);

// uc_orig: MESH_draw_poly_inv_matrix (fallen/DDEngine/Headers/mesh.h)
// Same as MESH_draw_poly but uses a non-transposed rotation matrix.
// Used for helicopters where the standard D3D transpose convention is skipped.
NIGHT_Colour* MESH_draw_poly_inv_matrix(
    SLONG prim,
    SLONG at_x,
    SLONG at_y,
    SLONG at_z,
    SLONG i_yaw,
    SLONG i_pitch,
    SLONG i_roll,
    NIGHT_Colour* lpc);

// uc_orig: MESH_init_reflections (fallen/DDEngine/Headers/mesh.h)
// Clears all cached reflection geometry. Must be called when loading a new level.
void MESH_init_reflections(void);

// uc_orig: MESH_draw_reflection (fallen/DDEngine/Headers/mesh.h)
// Draws a reflection of the given prim on a water surface at the bottom of the prim.
void MESH_draw_reflection(
    SLONG prim,
    SLONG at_x,
    SLONG at_y,
    SLONG at_z,
    SLONG at_yaw,
    NIGHT_Colour* lpc);

// uc_orig: MESH_draw_morph (fallen/DDEngine/Headers/mesh.h)
// Draws a mesh using keyframe vertex morphing: vertices are interpolated between
// morph frame morph1 and morph2 by blend factor tween (0..256 = 0.0..1.0).
// Face data (UVs, texture pages) comes from the base prim geometry, not the morph data.
void MESH_draw_morph(
    SLONG prim,
    UBYTE morph1,
    UBYTE morph2,
    UWORD tween,
    SLONG at_x,
    SLONG at_y,
    SLONG at_z,
    SLONG i_yaw,
    SLONG i_pitch,
    SLONG i_roll,
    NIGHT_Colour* lpc);

#endif // ENGINE_GRAPHICS_GEOMETRY_MESH_H
