#ifndef WORLD_ENVIRONMENT_WATER_H
#define WORLD_ENVIRONMENT_WATER_H

#include "core/types.h"

// Water surface simulation.
// Water is stored as a mesh of points and faces; each map square can contain
// a stack of water faces linked by WATER_get_first_face/WATER_get_next_face.

// uc_orig: WATER_init (fallen/Headers/water.h)
void WATER_init(void);

// uc_orig: WATER_add (fallen/Headers/water.h)
// Creates a square of water at the given height at map coordinates.
void WATER_add(SLONG map_x, SLONG map_z, SLONG height);

// uc_orig: WATER_gush (fallen/Headers/water.h)
// Disturbs the water surface as if something is moving through it.
void WATER_gush(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: WATER_process (fallen/Headers/water.h)
void WATER_process(void);

// Per-frame helpers for tracking transformed point indices during rendering.
// uc_orig: WATER_point_index_clear_all (fallen/Headers/water.h)
void WATER_point_index_clear_all(void);
// uc_orig: WATER_point_index_get (fallen/Headers/water.h)
UWORD WATER_point_index_get(UWORD p_index);
// uc_orig: WATER_point_index_set (fallen/Headers/water.h)
void WATER_point_index_set(UWORD p_index, UWORD index);

// Iterate faces above a map square.
// uc_orig: WATER_get_first_face (fallen/Headers/water.h)
UWORD WATER_get_first_face(SLONG x, SLONG z);
// uc_orig: WATER_get_next_face (fallen/Headers/water.h)
UWORD WATER_get_next_face(UWORD f_index);

// uc_orig: WATER_get_face_points (fallen/Headers/water.h)
// Fills in the 4 point indices for the given face.
void WATER_get_face_points(UWORD f_index, UWORD p_index[4]);

// uc_orig: WATER_get_point_pos (fallen/Headers/water.h)
void WATER_get_point_pos(UWORD p_index, float* x, float* y, float* z);
// uc_orig: WATER_get_point_uvs (fallen/Headers/water.h)
void WATER_get_point_uvs(UWORD p_index, float* u, float* v, ULONG* colour);

#endif // WORLD_ENVIRONMENT_WATER_H
