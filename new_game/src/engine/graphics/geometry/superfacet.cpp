#include "engine/platform/uc_common.h"

#include "game/game_types.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypoint.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "world/map/supermap.h"
#include "world/level_pools.h"
#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "engine/core/matrix.h"
#include "engine/graphics/geometry/superfacet.h"
#include "engine/graphics/geometry/superfacet_globals.h"

// POLY_set_local_rotation_none is defined as a no-op for the PC code path.
// On Dreamcast, this reset the local rotation matrix before DrawIndexedPrimitive.
// uc_orig: POLY_set_local_rotation_none (fallen/DDEngine/Source/superfacet.cpp)
#define POLY_set_local_rotation_none() \
    {                                  \
    }

// Defines for array bounds using the runtime-determined sizes (set in SUPERFACET_init).
#define SUPERFACET_MAX_LVERTS   (SUPERFACET_max_lverts)
#define SUPERFACET_MAX_INDICES  (SUPERFACET_max_indices)
#define SUPERFACET_MAX_FACETS   SUPERFACET_max_facets

// facet.cpp entities used by superfacet rendering.
#include "engine/graphics/geometry/facet.h"
#include "engine/graphics/geometry/facet_globals.h"

// uc_orig: SUPERFACET_free_end_of_queue (fallen/DDEngine/Source/superfacet.cpp)
// Frees the oldest facet in the circular queue, releasing its vertex memory range.
void SUPERFACET_free_end_of_queue(void)
{
    SLONG i;
    SLONG facet;

    SUPERFACET_Facet* sf;
    SUPERFACET_Call* sc;

    ASSERT(SUPERFACET_queue_end < SUPERFACET_queue_start);

    facet = SUPERFACET_queue[SUPERFACET_queue_end & (SUPERFACET_QUEUE_SIZE - 1)];

    ASSERT(WITHIN(facet, 0, SUPERFACET_MAX_FACETS - 1));

    sf = &SUPERFACET_facet[facet];

    // Free all the call structures belonging to this facet.
    for (i = 0; i < sf->num; i++) {
        ASSERT(WITHIN(sf->call + i, 0, SUPERFACET_MAX_CALLS - 1));

        sc = &SUPERFACET_call[sf->call + i];

        ASSERT(sc->flag & SUPERFACET_CALL_FLAG_USED);

        sc->flag = 0;
    }

    // Everything up to the end of the last call's vertex range is now free.
    SUPERFACET_free_range_end = sc->lvert + sc->lvertcount;

    ASSERT(WITHIN(SUPERFACET_free_range_end, 0, SUPERFACET_MAX_LVERTS));

    // If the next call is used and starts at the beginning of the lvert array,
    // all memory to the end of the array is free (the buffer has wrapped around).
    ASSERT(WITHIN(sf->call + sf->num, 0, SUPERFACET_MAX_CALLS - 1));

    sc = &SUPERFACET_call[sf->call + sf->num];

    if (sc->flag & SUPERFACET_CALL_FLAG_USED) {
        if (sc->lvert == 0) {
            SUPERFACET_free_range_end = SUPERFACET_MAX_LVERTS;
        }
    }

    // Pop the facet off the end of the queue.
    SUPERFACET_queue_end += 1;

    // Mark as unused.
    sf->call = 0;
    sf->num = 0;
}

// uc_orig: SUPERFACET_convert_texture (fallen/DDEngine/Source/superfacet.cpp)
// Converts UV coordinates from normalised [0,1] to atlas-page space and
// returns the D3D texture pointer for the given page number.
LPDIRECT3DTEXTURE2 SUPERFACET_convert_texture(SLONG page, POLY_Point* quad[4])
{
    SLONG i;

    ASSERT(WITHIN(page, 0, 511));

    PolyPage* pp = &POLY_Page[page];

    // Convert texture coordinates from normalised to atlas space.
    for (i = 0; i < 4; i++) {
        quad[i]->u = quad[i]->u * pp->m_UScale + pp->m_UOffset;
        quad[i]->v = quad[i]->v * pp->m_VScale + pp->m_VOffset;
    }

    return pp->RS.GetTexture();
}

// uc_orig: SUPERFACET_make_facet_points (fallen/DDEngine/Source/superfacet.cpp)
// Fills POLY_buffer[] with the untransformed world-space points of a wall facet.
// One point per (column, row) intersection; rows are separated by block_height units.
// Foundation=2 rows hug the terrain height from PAP_2HI.
void SUPERFACET_make_facet_points(
    float sx,
    float sy,
    float sz,
    float dx,
    float dz,
    float block_height,
    SLONG height,
    NIGHT_Colour* col,
    SLONG foundation,
    SLONG count,
    SLONG hug)
{
    SLONG hf = 0;

    POLY_buffer_upto = 0;

    ASSERT(WITHIN(block_height, 32.0F, 256.0F));

    while (height >= 0) {
        float x = sx;
        float y = sy;
        float z = sz;

        FacetRows[hf] = POLY_buffer_upto;

        for (SLONG c0 = count; c0 > 0; c0--) {
            float ty;

            ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

            POLY_Point* pp = &POLY_buffer[POLY_buffer_upto++];

            if (hug) {
                ty = float(PAP_2HI(SLONG(x) >> 8, SLONG(z) >> 8).Alt << 3);
                ty += y;
            } else if (foundation != 2) {
                ty = y;
            } else {
                ty = float(PAP_2HI(SLONG(x) >> 8, SLONG(z) >> 8).Alt << 3);

                FacetDiffY[POLY_buffer_upto - 1] = ((y - ty) * (1.0f / 256.0f)) + 1.0f;
            }

            pp->x = x;
            pp->y = ty;
            pp->z = z;

            // Index into the NIGHT_Colour[] array for this vertex (used by SUPERFACET_redo_lighting).
            pp->user = col - SUPERFACET_colour_base;

            NIGHT_get_d3d_colour(*col, &pp->colour, &pp->specular);

            x += dx;
            z += dz;
            col += 1;
        }
        sy += block_height;
        height -= 4;
        hf += 1;
        foundation -= 1;
    }

    FacetRows[hf] = POLY_buffer_upto;
}

// uc_orig: SUPERFACET_create_points (fallen/DDEngine/Source/superfacet.cpp)
// Generates the geometry points for the given facet index into POLY_buffer[].
void SUPERFACET_create_points(SLONG facet)
{
    SLONG dx;
    SLONG dz;
    SLONG height;
    SLONG count;
    SLONG foundation;

    float sx;
    float sy;
    float sz;
    float block_height;

    DFacet* df;
    NIGHT_Colour* col;

    ASSERT(WITHIN(facet, 0, next_dfacet - 1));
    ASSERT(WITHIN(facet, 0, SUPERFACET_MAX_FACETS - 1));

    df = &dfacets[facet];

    set_facet_seed(df->x[0] * df->z[0] + df->Y[0]);

    ASSERT(WITHIN(df->Dfcache, 1, NIGHT_MAX_DFCACHES - 1));

    SUPERFACET_colour_base = col = NIGHT_dfcache[df->Dfcache].colour;

    dx = df->x[1] - df->x[0] << 8;
    dz = df->z[1] - df->z[0] << 8;

    sx = float(df->x[0] << 8);
    sy = float(df->Y[0]);
    sz = float(df->z[0] << 8);

    ASSERT(!(dx && dz));

    if (dx) {
        count = abs(dx) >> 8;
    } else {
        count = abs(dz) >> 8;
    }

    count++;

    if (df->FHeight) {
        foundation = 2;
    } else {
        foundation = 0;
    }

    block_height = float(df->BlockHeight << 4);
    height = df->Height;

    SUPERFACET_make_facet_points(
        sx, sy, sz,
        SIGN(dx) * 256.0F,
        SIGN(dz) * 256.0F,
        block_height,
        height,
        col,
        foundation,
        count,
        df->FacetFlags & FACET_FLAG_HUG_FLOOR);
}

// uc_orig: SUPERFACET_fill_facet_points (fallen/DDEngine/Source/superfacet.cpp)
// Fills one row of quads from POLY_buffer[] into the vertex/index arrays for a given call.
// Only quads that use sc->texture are appended.
void SUPERFACET_fill_facet_points(
    SLONG count,
    ULONG base_row,
    SLONG foundation,
    SLONG style_index,
    float block_height,
    SUPERFACET_Call* sc)
{
    SLONG i;
    SLONG j;
    SLONG page;
    float vheight = float(block_height) * (1.0f / 256.0f);

    POLY_Point* quad[4];
    LPDIRECT3DTEXTURE2 texture;
    D3DLVERTEX* lv;

    SLONG row1 = FacetRows[base_row];
    SLONG row2 = FacetRows[base_row + 1];

    ASSERT(row2 - row1 == count);

    for (i = 0; i < row2 - row1 - 1; i++) {
        quad[0] = &POLY_buffer[row2 + i + 1];
        quad[1] = &POLY_buffer[row2 + i];
        quad[2] = &POLY_buffer[row1 + i + 1];
        quad[3] = &POLY_buffer[row1 + i];

        page = texture_quad(quad, dstyles[style_index], i, count);

        // Scale UV V-coordinate by block height.
        // MASSIVE FUDGE: clamp wrapping UV to 0.
        if (quad[0]->v >= 0.9999f) {
            quad[0]->v = 0.0f;
        }
        if (quad[1]->v >= 0.9999f) {
            quad[1]->v = 0.0f;
        }

        quad[2]->v = vheight;
        quad[3]->v = vheight;

        if (foundation == 2) {
            quad[3]->v = FacetDiffY[i];
            quad[2]->v = FacetDiffY[i + 1];

            // Foundation texture V can exceed 1 due to terrain height variation — clamp.
            if (quad[3]->v > 1.0f) {
                quad[3]->v = 1.0f;
            }
            if (quad[2]->v > 1.0f) {
                quad[2]->v = 1.0f;
            }
        }

        texture = SUPERFACET_convert_texture(page, quad);

        if (texture == sc->texture) {
            // Append four vertices for this quad.
            for (j = 0; j < 4; j++) {
                ASSERT(WITHIN(SUPERFACET_lvert_upto, 0, SUPERFACET_MAX_LVERTS - 1));

                lv = &SUPERFACET_lvert[SUPERFACET_lvert_upto + j];

                lv->x = quad[j]->x;
                lv->y = quad[j]->y;
                lv->z = quad[j]->z;

                {
                    lv->color = quad[j]->colour;
                    lv->specular = quad[j]->specular;
                }
                lv->tu = quad[j]->u;
                lv->tv = quad[j]->v;
                // Store the lighting index (into NIGHT_Colour[]) in the high 16 bits.
                lv->dwReserved = quad[j]->user << 16;
            }

            // Append six indices for two triangles covering the quad.
            ASSERT(WITHIN(SUPERFACET_index_upto + 4, 0, SUPERFACET_MAX_INDICES - 1));

            SUPERFACET_index[SUPERFACET_index_upto + 0] = SUPERFACET_lvert_upto - sc->lvert + 0;
            SUPERFACET_index[SUPERFACET_index_upto + 1] = SUPERFACET_lvert_upto - sc->lvert + 2;
            SUPERFACET_index[SUPERFACET_index_upto + 2] = SUPERFACET_lvert_upto - sc->lvert + 1;
            SUPERFACET_index[SUPERFACET_index_upto + 3] = SUPERFACET_lvert_upto - sc->lvert + 1;
            SUPERFACET_index[SUPERFACET_index_upto + 4] = SUPERFACET_lvert_upto - sc->lvert + 2;
            SUPERFACET_index[SUPERFACET_index_upto + 5] = SUPERFACET_lvert_upto - sc->lvert + 3;

            sc->lvertcount += 4;
            sc->indexcount += 6;

            SUPERFACET_lvert_upto += 4;
            SUPERFACET_index_upto += 6;

            ASSERT(sc->lvert + sc->lvertcount == SUPERFACET_lvert_upto);
            ASSERT(sc->index + sc->indexcount == SUPERFACET_index_upto);
        }
    }

    // Advance the RNG state for any skipped quads (to maintain parity with facet.cpp).
    {
        facet_rand();
    }
}

// uc_orig: SUPERFACET_build_call (fallen/DDEngine/Source/superfacet.cpp)
// Populates vertex/index data for the 'call'th texture call of the given facet.
void SUPERFACET_build_call(SLONG facet, SLONG call)
{
    SLONG dx;
    SLONG dz;
    SLONG hf;
    SLONG height;
    SLONG count;
    SLONG foundation;
    SLONG style_index;
    SLONG style_index_step;

    float block_height;

    DFacet* df;
    SUPERFACET_Facet* sf;
    SUPERFACET_Call* sc;

    ASSERT(WITHIN(facet, 0, next_dfacet - 1));
    ASSERT(WITHIN(facet, 0, SUPERFACET_MAX_FACETS - 1));

    df = &dfacets[facet];
    sf = &SUPERFACET_facet[facet];

    ASSERT(WITHIN(call, 0, sf->num - 1));
    ASSERT(WITHIN(sf->call + call, 0, SUPERFACET_MAX_CALLS - 1));

    sc = &SUPERFACET_call[sf->call + call];

    sc->index = SUPERFACET_index_upto;
    sc->lvert = SUPERFACET_lvert_upto;
    sc->indexcount = 0;
    sc->lvertcount = 0;

    set_facet_seed(df->x[0] * df->z[0] + df->Y[0]);

    dx = (df->x[1] - df->x[0]) << 8;
    dz = (df->z[1] - df->z[0]) << 8;

    ASSERT(!(dx && dz));

    if (dx) {
        count = abs(dx) >> 8;
    } else {
        count = abs(dz) >> 8;
    }

    count++;

    style_index = df->StyleIndex;

    if (df->FacetFlags & FACET_FLAG_2TEXTURED) {
        style_index -= 1;
    }

    if (!(df->FacetFlags & FACET_FLAG_HUG_FLOOR) && (df->FacetFlags & (FACET_FLAG_2TEXTURED | FACET_FLAG_2SIDED))) {
        style_index_step = 2;
    } else {
        style_index_step = 1;
    }

    if (df->FHeight) {
        foundation = 2;
    }

    block_height = float(df->BlockHeight << 4);
    height = df->Height;

    hf = 0;

    while (height >= 0) {
        if (hf) {
            SUPERFACET_fill_facet_points(
                count,
                hf - 1,
                foundation + 1,
                style_index - 1,
                block_height,
                sc);
        }

        height -= 4;
        hf += 1;
        foundation -= 1;
        style_index += style_index_step;
    }

    ASSERT(sc->lvertcount == sc->quads * 4);

    SUPERFACET_free_range_start = sc->lvert + sc->lvertcount;

    return;
}

// uc_orig: SUPERFACET_create_calls (fallen/DDEngine/Source/superfacet.cpp)
// Scans all quads of a facet and creates one SUPERFACET_Call per unique D3D texture.
// Counts quads per call so vertex memory can be pre-allocated.
void SUPERFACET_create_calls(SLONG facet, SLONG direction)
{
    SLONG i;
    SLONG j;
    SLONG dx;
    SLONG dz;
    SLONG hf;
    SLONG page;
    SLONG count;
    SLONG height;
    SLONG style_index;
    SLONG style_index_step;

    LPDIRECT3DTEXTURE2 texture;
    DFacet* df;
    SUPERFACET_Facet* sf;
    SUPERFACET_Call* sc;
    POLY_Point* quad[4];

    ASSERT(WITHIN(facet, 0, next_dfacet - 1));
    ASSERT(WITHIN(facet, 0, SUPERFACET_MAX_FACETS - 1));

    df = &dfacets[facet];
    sf = &SUPERFACET_facet[facet];

    set_facet_seed(df->x[0] * df->z[0] + df->Y[0]);

    dx = (df->x[1] - df->x[0]) << 8;
    dz = (df->z[1] - df->z[0]) << 8;

    ASSERT(!(dx && dz));

    if (dx) {
        count = abs(dx) >> 8;
    } else {
        count = abs(dz) >> 8;
    }

    count++;

    style_index = df->StyleIndex;

    if (df->FacetFlags & FACET_FLAG_2TEXTURED) {
        style_index -= 1;
    }

    if (!(df->FacetFlags & FACET_FLAG_HUG_FLOOR) && (df->FacetFlags & (FACET_FLAG_2TEXTURED | FACET_FLAG_2SIDED))) {
        style_index_step = 2;
    } else {
        style_index_step = 1;
    }

    // Use a dummy POLY_Point array for texture lookup (coordinates don't matter here).
    POLY_Point pp[4];

    memset(pp, 0, sizeof(pp));

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    hf = 0;
    height = df->Height;

    while (height >= 0) {
        if (hf) {
            for (i = 0; i < count - 1; i++) {
                page = texture_quad(quad, dstyles[style_index - 1], i, count);

                texture = SUPERFACET_convert_texture(page, quad);

                // Check if we already have a call for this texture.
                for (j = 0; j < sf->num; j++) {
                    ASSERT(WITHIN(sf->call + j, 0, SUPERFACET_MAX_CALLS - 1));

                    sc = &SUPERFACET_call[sf->call + j];

                    if (sc->texture == texture) {
                        sc->quads += 1;
                        goto already_got_a_call;
                    }
                }

                // No existing call — create a new one.
                ASSERT(WITHIN(SUPERFACET_call_upto, 0, SUPERFACET_MAX_CALLS - 1));
                ASSERT(SUPERFACET_call_upto == sf->call + sf->num);

                sc = &SUPERFACET_call[SUPERFACET_call_upto++];

                ASSERT(!(sc->flag & SUPERFACET_CALL_FLAG_USED));

                sc->flag = SUPERFACET_CALL_FLAG_USED;
                sc->texture = texture;
                sc->quads = 1;
                sc->dir = direction;

                if (POLY_page_flag[page] & POLY_PAGE_FLAG_2PASS) {
                    sc->flag |= SUPERFACET_CALL_FLAG_2PASS;
                    sc->texture_2pass = SUPERFACET_convert_texture(page + 1, quad);
                }

                sf->num += 1;

            already_got_a_call:;
            }

            facet_rand();
        }

        height -= 4;
        hf += 1;
        style_index += style_index_step;
    }

    return;
}

// uc_orig: SUPERFACET_convert_facet (fallen/DDEngine/Source/superfacet.cpp)
// Caches the rendering data for a facet: creates calls, allocates vertex memory,
// builds geometry, and adds the facet to the circular eviction queue.
void SUPERFACET_convert_facet(SLONG facet)
{
    SLONG i;
    SLONG room;
    SLONG memory;
    SLONG direction;

    DFacet* df;
    SUPERFACET_Facet* sf;
    SUPERFACET_Call* sc;

    ASSERT(WITHIN(facet, 1, next_dfacet - 1));

    df = &dfacets[facet];

    if (df->Dfcache == NULL) {
        df->Dfcache = NIGHT_dfcache_create(facet);
        ASSERT(df->Dfcache);
    }

    // Determine facing direction (0=+X, 1=+Z, 2=-X, 3=-Z).
    if (df->z[0] == df->z[1]) {
        if (df->x[0] < df->x[1]) {
            direction = 0;
        } else {
            direction = 2;
        }
    } else {
        if (df->z[0] > df->z[1]) {
            direction = 3;
        } else {
            direction = 1;
        }
    }

    // Make sure there are enough call slots (leave 32 as safety margin).
    if (SUPERFACET_call_upto > SUPERFACET_MAX_CALLS - 32) {
        SUPERFACET_call_upto = 0;
    }

    ASSERT(WITHIN(facet, 0, SUPERFACET_MAX_FACETS - 1));

    sf = &SUPERFACET_facet[facet];

    sf->call = SUPERFACET_call_upto;
    sf->num = 0;

    SUPERFACET_create_calls(facet, direction);

    // Calculate how many lvert slots we need for this facet.
    memory = 0;

    for (i = 0; i < sf->num; i++) {
        ASSERT(WITHIN(sf->call + i, 0, SUPERFACET_MAX_CALLS - 1));

        sc = &SUPERFACET_call[sf->call + i];

        memory += sc->quads * 4;
    }

    // Evict old facets until there is enough free vertex memory.
    while (1) {
        if (SUPERFACET_free_range_start <= SUPERFACET_free_range_end) {
            room = SUPERFACET_free_range_end - SUPERFACET_free_range_start;
        } else {
            room = SUPERFACET_MAX_LVERTS - SUPERFACET_free_range_start;
        }

        if (room >= memory) {
            break;
        }

        if (SUPERFACET_free_range_start > SUPERFACET_free_range_end) {
            // Buffer has wrapped: reset write pointer to the beginning.
            SUPERFACET_free_range_start = 0;
            SUPERFACET_lvert_upto = 0;
            SUPERFACET_index_upto = 0;
        }

        SUPERFACET_free_end_of_queue();
    }

    SUPERFACET_create_points(facet);

    for (i = 0; i < sf->num; i++) {
        SUPERFACET_build_call(facet, i);
    }

    NIGHT_dfcache_destroy(df->Dfcache);

    df->Dfcache = NULL;

    // Push this facet onto the circular eviction queue.
    ASSERT(SUPERFACET_queue_start < SUPERFACET_queue_end + SUPERFACET_QUEUE_SIZE);

    SUPERFACET_queue[SUPERFACET_queue_start & (SUPERFACET_QUEUE_SIZE - 1)] = facet;

    SUPERFACET_queue_start++;
}

// uc_orig: SUPERFACET_init (fallen/DDEngine/Headers/superfacet.h)
// Allocates and initialises the supefacet batch buffers. Call after all assets are loaded.
void SUPERFACET_init()
{
    SUPERFACET_max_facets = next_dfacet;
    SUPERFACET_max_lverts = 8192;
    SUPERFACET_max_indices = SUPERFACET_max_lverts * 6 / 4;

    SUPERFACET_lvert_buffer = (UBYTE*)MemAlloc(sizeof(D3DLVERTEX) * SUPERFACET_MAX_LVERTS + 32);
    ASSERT(SUPERFACET_lvert_buffer != NULL);
    SUPERFACET_index = (UWORD*)MemAlloc(sizeof(UWORD) * SUPERFACET_MAX_INDICES);
    ASSERT(SUPERFACET_index != NULL);
    SUPERFACET_facet = (SUPERFACET_Facet*)MemAlloc(sizeof(SUPERFACET_Facet) * SUPERFACET_MAX_FACETS);
    ASSERT(SUPERFACET_facet != NULL);

    memset(SUPERFACET_lvert_buffer, 0, sizeof(D3DLVERTEX) * SUPERFACET_MAX_LVERTS + 32);
    memset(SUPERFACET_index, 0, sizeof(UWORD) * SUPERFACET_MAX_INDICES);
    memset(SUPERFACET_call, 0, sizeof(SUPERFACET_call));
    memset(SUPERFACET_facet, 0, sizeof(SUPERFACET_Facet) * SUPERFACET_MAX_FACETS);
    memset(SUPERFACET_queue, 0, sizeof(SUPERFACET_queue));

    SUPERFACET_queue_start = 0;
    SUPERFACET_queue_end = 0;

    SUPERFACET_free_range_start = 0;
    SUPERFACET_free_range_end = SUPERFACET_MAX_LVERTS;

    // Align the lvert pointer to a 32-byte boundary for SIMD-friendly access.
    SUPERFACET_lvert = (D3DLVERTEX*)((SLONG(SUPERFACET_lvert_buffer) + 31) & ~0x1f);

    SUPERFACET_lvert_upto = 0;
    SUPERFACET_index_upto = 0;
    SUPERFACET_call_upto = 0;
    SUPERFACET_queue_start = 0;
    SUPERFACET_queue_end = 0;

    SUPERFACET_matrix = (D3DMATRIX*)((SLONG(SUPERFACET_matrix_buffer) + 31) & ~0x1f);

    // Build the four cardinal direction matrices (0°, 90°, 180°, 270°).
    SLONG i;

    for (i = 0; i < 4; i++) {
        MATRIX_calc(
            SUPERFACET_direction_matrix[i],
            float(i) * (PI / 2),
            0.0F,
            0.0F);
    }
}

// uc_orig: SUPERFACET_redo_lighting (fallen/DDEngine/Source/superfacet.cpp)
// Updates the vertex colours of a cached facet after dynamic lighting changes.
// Currently always ASSERTs (not used in practice).
void SUPERFACET_redo_lighting(SLONG facet)
{
    SLONG i;
    SLONG j;

    DFacet* df;
    SUPERFACET_Call* sc;
    SUPERFACET_Facet* sf;
    D3DLVERTEX* lv;

    df = &dfacets[facet];
    sf = &SUPERFACET_facet[facet];

    ASSERT(0);
    if (df->Dfcache == NULL) {
        df->Dfcache = NIGHT_dfcache_create(facet);
        ASSERT(df->Dfcache);
    }

    NIGHT_Colour* col = NIGHT_dfcache[df->Dfcache].colour;

    for (i = 0; i < sf->num; i++) {
        ASSERT(WITHIN(sf->call + i, 0, SUPERFACET_MAX_CALLS - 1));

        sc = &SUPERFACET_call[sf->call + i];

        ASSERT(sc->flag & SUPERFACET_CALL_FLAG_USED);

        for (j = 0; j < sc->lvertcount; j++) {
            lv = &SUPERFACET_lvert[sc->lvert + j];

            ASSERT(WITHIN(lv->dwReserved >> 16, 0, 2048));

            NIGHT_get_d3d_colour(col[lv->dwReserved >> 16], &lv->color, &lv->specular);
        }
    }
}

// uc_orig: SUPERFACET_start_frame (fallen/DDEngine/Headers/superfacet.h)
// Clears the frame's batch buffers. Call once per frame before drawing.
void SUPERFACET_start_frame()
{
    POLY_set_local_rotation_none();
    POLY_flush_local_rot();
}

// uc_orig: SUPERFACET_fini (fallen/DDEngine/Headers/superfacet.h)
// Frees supefacet batch memory. Call at program shutdown.
void SUPERFACET_fini()
{
    MemFree(SUPERFACET_lvert_buffer);
    MemFree(SUPERFACET_index);
    MemFree(SUPERFACET_facet);
}
