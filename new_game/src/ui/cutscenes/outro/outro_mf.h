#ifndef UI_CUTSCENES_OUTRO_OUTRO_MF_H
#define UI_CUTSCENES_OUTRO_OUTRO_MF_H

#include "ui/cutscenes/outro/outro_os.h"
#include "ui/cutscenes/outro/outro_imp.h"

// Maximum number of shared vertices in a single mesh draw call.
// uc_orig: MF_MAX_SVERTS (fallen/outro/mf.cpp)
#define MF_MAX_SVERTS 16384

// Loads all textures referenced by the mesh materials (ot_tex, ot_bpos, ot_bneg).
// uc_orig: MF_load_textures (fallen/outro/mf.h)
void MF_load_textures(IMP_Mesh* im);

// Allocates backup copies of vert[] and svert[] so the mesh can be rotated/scaled
// non-destructively using MF_rotate_mesh().
// uc_orig: MF_backup (fallen/outro/mf.h)
void MF_backup(IMP_Mesh* im);

// Rotates and scales the mesh in-place using a yaw/pitch/roll angle set plus a scale factor
// and translation. Requires MF_backup() to have been called first.
// uc_orig: MF_rotate_mesh (fallen/outro/mf.h)
void MF_rotate_mesh(
    IMP_Mesh* im,
    float yaw,
    float pitch = 0.0F,
    float roll = 0.0F,
    float scale = 1.0F,
    float pos_x = 0.0F,
    float pos_y = 0.0F,
    float pos_z = 0.0F);

// Overload: rotates using a pre-built 3x3 matrix instead of angles.
// Matrix must be normalised or normals will be distorted.
// uc_orig: MF_rotate_mesh (fallen/outro/mf.h)
void MF_rotate_mesh(
    IMP_Mesh* im,
    float pos_x,
    float pos_y,
    float pos_z,
    float matrix[9]);

// Transforms all mesh vertices into OS_trans[] using the current camera.
// uc_orig: MF_transform_points (fallen/outro/mf.h)
void MF_transform_points(IMP_Mesh* im);

// Inverts the Z values in OS_trans[] so that 0=far, 1=near (for reversed z-buffer).
// Call after MF_transform_points().
// uc_orig: MF_invert_zeds (fallen/outro/mf.h)
void MF_invert_zeds(IMP_Mesh* im);

// Computes per-vertex ambient + directional lighting colour into svert[].colour.
// Light vector does not need to be normalised.
// uc_orig: MF_ambient (fallen/outro/mf.h)
void MF_ambient(
    IMP_Mesh* im,
    float light_dx,
    float light_dy,
    float light_dz,
    SLONG light_r,
    SLONG light_g,
    SLONG light_b,
    SLONG amb_r,
    SLONG amb_g,
    SLONG amb_b);

// Computes diffuse spotlight UVs (lu, lv on IMP_Vert) and gouraud + bumpmap offsets
// (colour, du, dv on IMP_Svert) for the given spotlight.
// uc_orig: MF_diffuse_spotlight (fallen/outro/mf.h)
void MF_diffuse_spotlight(
    IMP_Mesh* im,
    float light_x,
    float light_y,
    float light_z,
    float light_matrix[9],
    float light_lens);

// Computes specular spotlight UVs (lu, lv on IMP_Svert) and highlight colour.
// uc_orig: MF_specular_spotlight (fallen/outro/mf.h)
void MF_specular_spotlight(
    IMP_Mesh* im,
    float light_x,
    float light_y,
    float light_z,
    float light_matrix[9],
    float light_lens);

// Submits mesh triangles using per-material textures with per-vertex colour.
// uc_orig: MF_add_triangles_normal (fallen/outro/mf.h)
void MF_add_triangles_normal(IMP_Mesh* im, ULONG draw = OS_DRAW_NORMAL);

// Same as MF_add_triangles_normal but overrides all vertex colours with a constant.
// uc_orig: MF_add_triangles_normal_colour (fallen/outro/mf.h)
void MF_add_triangles_normal_colour(IMP_Mesh* im, ULONG draw = OS_DRAW_NORMAL, ULONG colour = 0xffffff);

// Submits mesh triangles using lightmap coordinates (lu/lv from IMP_Vert) and a single texture.
// uc_orig: MF_add_triangles_light (fallen/outro/mf.h)
void MF_add_triangles_light(IMP_Mesh* im, OS_Texture* ot, ULONG draw = OS_DRAW_ADD | OS_DRAW_CLAMP);

// Two-pass lightmap with bumpmap: first pass draws bump offset UVs, second pass draws additive.
// uc_orig: MF_add_triangles_light_bumpmapped (fallen/outro/mf.h)
void MF_add_triangles_light_bumpmapped(IMP_Mesh* im, OS_Texture* ot, ULONG draw = OS_DRAW_ADD | OS_DRAW_CLAMP);

// Submits mesh triangles using specular spotlight UVs (lu/lv from IMP_Svert).
// uc_orig: MF_add_triangles_specular (fallen/outro/mf.h)
void MF_add_triangles_specular(IMP_Mesh* im, OS_Texture* ot, ULONG draw = OS_DRAW_ADD | OS_DRAW_CLAMP);

// Single-pass specular with bumpmap: submits materials with bumpmaps using multiply blend.
// uc_orig: MF_add_triangles_specular_bumpmapped (fallen/outro/mf.h)
void MF_add_triangles_specular_bumpmapped(IMP_Mesh* im, OS_Texture* ot, ULONG draw = OS_DRAW_ADD | OS_DRAW_CLAMP);

// Draws specular highlight shadowed by a diffuse spotlight. Uses two textures.
// uc_orig: MF_add_triangles_specular_shadowed (fallen/outro/mf.h)
void MF_add_triangles_specular_shadowed(IMP_Mesh* im, OS_Texture* ot_specdot, OS_Texture* ot_diffdot, ULONG draw = OS_DRAW_ADD | OS_DRAW_CLAMP | OS_DRAW_TEX_MUL);

// Draws visible mesh edges as 3D lines using OS_buffer_add_line_3d.
// uc_orig: MF_add_wireframe (fallen/outro/mf.h)
void MF_add_wireframe(IMP_Mesh* im, OS_Texture* ot, ULONG colour, float width = 0.002F, ULONG draw = OS_DRAW_ADD | OS_DRAW_NOZWRITE);

// Two-pass bumpmap: pass 0 shifts UVs by +du/dv (positive bumpmap), pass 1 by -du/dv (negative).
// Only processes faces with materials that have bumpmaps.
// uc_orig: MF_add_triangles_bumpmapped_pass (fallen/outro/mf.h)
void MF_add_triangles_bumpmapped_pass(IMP_Mesh* im, SLONG pass, ULONG draw = OS_DRAW_NORMAL);

// Draws the final colour texture after bumpmap passes. Uses per-material draw flags
// (double-sided, multiply+decal for bumpmapped materials).
// uc_orig: MF_add_triangles_texture_after_bumpmap (fallen/outro/mf.h)
void MF_add_triangles_texture_after_bumpmap(IMP_Mesh* im);

#endif // UI_CUTSCENES_OUTRO_OUTRO_MF_H
