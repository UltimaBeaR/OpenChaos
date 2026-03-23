#ifndef WORLD_ENVIRONMENT_SEWER_H
#define WORLD_ENVIRONMENT_SEWER_H

#include "core/types.h"

// Sewer system: underground network the player can enter via ladders.
// Sewers are 128x128 grid squares deep by SEWER_DEPTH units.

// uc_orig: SEWER_SIZE (fallen/Headers/sewer.h)
#define SEWER_SIZE 128
// uc_orig: SEWER_DEPTH (fallen/Headers/sewer.h)
#define SEWER_DEPTH 0x100

// Edge directions for sewer ladders.
// uc_orig: SEWER_EDGE_XS (fallen/Headers/sewer.h)
#define SEWER_EDGE_XS 0
// uc_orig: SEWER_EDGE_XL (fallen/Headers/sewer.h)
#define SEWER_EDGE_XL 1
// uc_orig: SEWER_EDGE_ZS (fallen/Headers/sewer.h)
#define SEWER_EDGE_ZS 2
// uc_orig: SEWER_EDGE_ZL (fallen/Headers/sewer.h)
#define SEWER_EDGE_ZL 3

// uc_orig: SEWER_init (fallen/Headers/sewer.h)
void SEWER_init(void);
// uc_orig: SEWER_square_on (fallen/Headers/sewer.h)
void SEWER_square_on(SLONG x, SLONG z);
// uc_orig: SEWER_ladder_on (fallen/Headers/sewer.h)
void SEWER_ladder_on(SLONG x, SLONG z, SLONG edge);
// uc_orig: SEWER_pillar_on (fallen/Headers/sewer.h)
void SEWER_pillar_on(SLONG x, SLONG z);
// uc_orig: SEWER_precalc (fallen/Headers/sewer.h)
void SEWER_precalc(void);
// uc_orig: SEWER_save (fallen/Headers/sewer.h)
void SEWER_save(CBYTE* filename);
// uc_orig: SEWER_load (fallen/Headers/sewer.h)
void SEWER_load(CBYTE* filename);

// Returns true if the given square is an entrance into the sewer system.
// uc_orig: SEWER_can_i_enter (fallen/Headers/sewer.h)
SLONG SEWER_can_i_enter(UBYTE x, UBYTE z);

// Insert/remove collision vectors around the sewer entrances.
// uc_orig: SEWER_colvects_insert (fallen/Headers/sewer.h)
void SEWER_colvects_insert(void);
// uc_orig: SEWER_colvects_remove (fallen/Headers/sewer.h)
void SEWER_colvects_remove(void);

// Returns the floor height of the sewer at (x, z).
// uc_orig: SEWER_calc_height_at (fallen/Headers/sewer.h)
SLONG SEWER_calc_height_at(SLONG x, SLONG z);

// Sewer face page indices used for rendering.
// uc_orig: SEWER_PAGE_FLOOR (fallen/Headers/sewer.h)
#define SEWER_PAGE_FLOOR   0
// uc_orig: SEWER_PAGE_WALL (fallen/Headers/sewer.h)
#define SEWER_PAGE_WALL    1
// uc_orig: SEWER_PAGE_PILLAR (fallen/Headers/sewer.h)
#define SEWER_PAGE_PILLAR  2
// uc_orig: SEWER_PAGE_WATER (fallen/Headers/sewer.h)
#define SEWER_PAGE_WATER   3
// uc_orig: SEWER_PAGE_NUMBER (fallen/Headers/sewer.h)
#define SEWER_PAGE_NUMBER  4

// A single renderable quad face of the sewer geometry.
// uc_orig: SEWER_Face (fallen/Headers/sewer.h)
typedef struct
{
    SLONG x[4];
    SLONG y[4];
    SLONG z[4];
    SLONG u[4]; // 16-bit fixed point, range 0.0–256.0
    SLONG v[4]; // 16-bit fixed point, range 0.0–256.0
    ULONG c[4];

    UBYTE page; // Sewer page index (SEWER_PAGE_*), NOT the texture page.

} SEWER_Face;

// Iterates sewer faces for rendering. Call SEWER_get_start, then SEWER_get_next
// until it returns NULL. SEWER_get_water returns only water surface faces.
// uc_orig: SEWER_get_start (fallen/Headers/sewer.h)
void SEWER_get_start(SLONG x, SLONG z);
// uc_orig: SEWER_get_next (fallen/Headers/sewer.h)
SEWER_Face* SEWER_get_next(void);
// uc_orig: SEWER_get_water (fallen/Headers/sewer.h)
SEWER_Face* SEWER_get_water(void);

#endif // WORLD_ENVIRONMENT_SEWER_H
