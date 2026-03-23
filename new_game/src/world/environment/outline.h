#ifndef WORLD_ENVIRONMENT_OUTLINE_H
#define WORLD_ENVIRONMENT_OUTLINE_H

// Grid outline computation utility — used by building construction to test polygon overlap.
// An outline is a set of orthogonal line segments on a 2D integer grid.
// Originally from the editor codebase; used at runtime for building placement validation.

#include "core/types.h"

// Opaque handle to an outline instance.
// uc_orig: OUTLINE_Outline (fallen/Editor/Headers/outline.h)
typedef struct outline_outline OUTLINE_Outline;

// Allocates and initialises an empty outline for a grid with num_z_squares rows.
// uc_orig: OUTLINE_create (fallen/Editor/Source/outline.cpp)
OUTLINE_Outline* OUTLINE_create(SLONG num_z_squares);

// Adds an orthogonal line segment from (x1,z1) to (x2,z2) to the outline.
// Either x1==x2 (vertical) or z1==z2 (horizontal). The shape must be closed
// before calling OUTLINE_overlap or OUTLINE_intersects.
// uc_orig: OUTLINE_add_line (fallen/Editor/Source/outline.cpp)
void OUTLINE_add_line(
    OUTLINE_Outline* oo,
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2);

// Frees all memory used by the outline (links and outline struct itself).
// uc_orig: OUTLINE_free (fallen/Editor/Source/outline.cpp)
void OUTLINE_free(OUTLINE_Outline* oo);

// Returns UC_TRUE if the two closed outlines overlap (share interior area).
// uc_orig: OUTLINE_overlap (fallen/Editor/Source/outline.cpp)
SLONG OUTLINE_overlap(
    OUTLINE_Outline* oo1,
    OUTLINE_Outline* oo2);

// Returns UC_TRUE if the given orthogonal line segment crosses the outline boundary.
// uc_orig: OUTLINE_intersects (fallen/Editor/Source/outline.cpp)
SLONG OUTLINE_intersects(
    OUTLINE_Outline* oo,
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2);

#endif // WORLD_ENVIRONMENT_OUTLINE_H
