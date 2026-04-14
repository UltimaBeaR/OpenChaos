#ifndef NAVIGATION_WMOVE_H
#define NAVIGATION_WMOVE_H

#include "engine/core/types.h"

struct Thing;

// Moving walkable faces: virtual walkable triangles attached to vehicles and platforms.
// Allows characters to stand on top of moving vehicles and elevators.
// Each WMOVE_Face tracks the previous-frame positions for relative motion transfer.

// A single point in a WMOVE face (packed coordinates).
// uc_orig: WMOVE_Point (fallen/Headers/wmove.h)
typedef struct {
    UWORD x;
    SWORD y;
    UWORD z;
} WMOVE_Point;

// State for one moving walkable face.
// uc_orig: WMOVE_Face (fallen/Headers/wmove.h)
typedef struct {
    UWORD face4;       // Index into prim_faces4[] for the walkable quad.
    WMOVE_Point last[3]; // Positions of the face's three corner points last frame.
    UWORD thing;       // Owner Thing index.
    UWORD number;      // Sub-index when a thing owns multiple faces (e.g. car roof).
} WMOVE_Face;

// Maximum WMOVE faces (driven by save table; hard limit is RWMOVE_MAX_FACES).
// uc_orig: RWMOVE_MAX_FACES (fallen/Headers/wmove.h)
#define RWMOVE_MAX_FACES 192
// uc_orig: WMOVE_MAX_FACES (fallen/Headers/wmove.h)
#define WMOVE_MAX_FACES (save_table[SAVE_TABLE_WMOVE].Maximum)

// uc_orig: WMOVE_face (fallen/Source/wmove.cpp)
extern WMOVE_Face* WMOVE_face;
// uc_orig: WMOVE_face_upto (fallen/Source/wmove.cpp)
extern SLONG WMOVE_face_upto;

// Cache for the last computed rotation matrix (avoids recomputing per-face on same tick).
// uc_orig: WMOVE_matrix (fallen/Source/wmove.cpp)
extern SLONG WMOVE_matrix[9];
// uc_orig: WMOVE_matrix_thing (fallen/Source/wmove.cpp)
extern UWORD WMOVE_matrix_thing;
// uc_orig: WMOVE_matrix_turn (fallen/Source/wmove.cpp)
extern UWORD WMOVE_matrix_turn;

// Reset the pool.
// uc_orig: WMOVE_init (fallen/Headers/wmove.h)
void WMOVE_init(void);

// Allocate prim faces for a thing that needs moving walkable surfaces.
// uc_orig: WMOVE_create (fallen/Headers/wmove.h)
void WMOVE_create(Thing* p_thing);

// Update all active WMOVE faces to match current owner positions.
// uc_orig: WMOVE_process (fallen/Headers/wmove.h)
void WMOVE_process(void);

// Given a character's last-frame position on a WMOVE face, compute the new position
// after the face has moved (used by person movement logic).
// uc_orig: WMOVE_relative_pos (fallen/Headers/wmove.h)
void WMOVE_relative_pos(
    UBYTE wmove_index,
    SLONG last_x,
    SLONG last_y,
    SLONG last_z,
    SLONG* now_x,
    SLONG* now_y,
    SLONG* now_z,
    SLONG* now_dangle);

// Bulk removal: drop every WMOVE_Face whose owner is of the given Thing class
// (used during level transitions to clear stale walkable surfaces).
// uc_orig: WMOVE_remove (fallen/Source/wmove.cpp)
void WMOVE_remove(UBYTE which_class);

// Debug-only: draw outlines of all active WMOVE faces.
// uc_orig: WMOVE_draw (fallen/Headers/wmove.h)
void WMOVE_draw(void);

#endif // NAVIGATION_WMOVE_H
