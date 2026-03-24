#ifndef ENGINE_GRAPHICS_LIGHTING_CRINKLE_H
#define ENGINE_GRAPHICS_LIGHTING_CRINKLE_H

#include "engine/core/types.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/aeng.h"
#include "assets/file_clump.h"

// Crinkle = per-surface bump-mapping approximation using pre-computed normal meshes.
// Each crinkle is a dense triangle mesh that is extruded from a flat quad at render time
// to simulate surface relief. Loaded from .sex files or pre-built .txc clumps.

// uc_orig: CRINKLE_Handle (fallen/DDEngine/Headers/crinkle.h)
// Index into the crinkle pool. 0 = invalid/null.
typedef UWORD CRINKLE_Handle;

// uc_orig: CRINKLE_init (fallen/DDEngine/Headers/crinkle.h)
// Resets the crinkle pool. Call at startup.
void CRINKLE_init(void);

// uc_orig: CRINKLE_load (fallen/DDEngine/Headers/crinkle.h)
// Loads a crinkle from a .sex ASCII file. Returns CRINKLE_NULL if not found.
CRINKLE_Handle CRINKLE_load(CBYTE* sex_filename);

// uc_orig: CRINKLE_read_bin (fallen/DDEngine/Headers/crinkle.h)
// Reads a pre-built binary crinkle from a FileClump archive.
CRINKLE_Handle CRINKLE_read_bin(FileClump* tclump, int id);

// uc_orig: CRINKLE_write_bin (fallen/DDEngine/Headers/crinkle.h)
// Writes a crinkle to a FileClump archive (used by level tools).
void CRINKLE_write_bin(FileClump* tclump, CRINKLE_Handle hnd, int id);

// uc_orig: CRINKLE_light (fallen/DDEngine/Headers/crinkle.h)
// Sets the view-space light direction for crinkle shading.
void CRINKLE_light(float dx, float dy, float dz);

// uc_orig: CRINKLE_skew (fallen/DDEngine/Headers/crinkle.h)
// Passes the camera's aspect ratio and lens factor so crinkle projection matches the frustum.
void CRINKLE_skew(float aspect, float lens);

// uc_orig: CRINKLE_do (fallen/DDEngine/Headers/crinkle.h)
// Draws a crinkle extruded by 'amount' (0.0..1.0) between the four corner points pp[0..3].
void CRINKLE_do(
    CRINKLE_Handle crinkle,
    SLONG page,
    float amount,
    POLY_Point* pp[4],
    SLONG flip);

// uc_orig: CRINKLE_project (fallen/DDEngine/Headers/crinkle.h)
// Projects an SMAP shadow over the crinkle geometry.
void CRINKLE_project(
    CRINKLE_Handle crinkle,
    float amount,
    SVector_F poly[4],
    SLONG flip);

#endif // ENGINE_GRAPHICS_LIGHTING_CRINKLE_H
