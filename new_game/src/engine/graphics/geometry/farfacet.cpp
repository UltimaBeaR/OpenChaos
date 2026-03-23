// Temporary includes: game.h, supermap.h, memory.h (fallen) not yet migrated
#include "fallen/Headers/Game.h"
#include "world/map/pap.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/graphics_api/gd_display.h"
#include "world/map/supermap.h"
#include "fallen/Headers/memory.h"
#include "core/matrix.h"
#include <math.h>
#include "engine/graphics/geometry/farfacet.h"
#include "engine/graphics/geometry/farfacet_globals.h"
#include "engine/graphics/pipeline/render_state.h"
#include "engine/input/keyboard.h"

// uc_orig: FARFACET_find_vertex (fallen/DDEngine/Source/farfacet.cpp)
// Looks up a vertex at (map_x, map_y, map_z) in the given square's vertex buffer.
// If not found, creates a new vertex. Returns the local vertex index within the square.
static UWORD FARFACET_find_vertex(FARFACET_Square* fs, UBYTE map_x, SBYTE map_y, UBYTE map_z)
{
    SLONG i;

    float x = float(map_x << 8);
    float y = float(map_y << 6);
    float z = float(map_z << 8);

    D3DLVERTEX* lv;

    for (i = 0; i < fs->lvertcount; i++) {
        ASSERT(WITHIN(fs->lvert + i, 0, FARFACET_lvert_upto - 1));

        lv = &FARFACET_lvert[fs->lvert + i];

        if (lv->x == x && lv->y == y && lv->z == z) {
            return i;
        }
    }

    // Vertex not found — create a new one, growing the buffer if needed.
    if (FARFACET_lvert_upto >= FARFACET_lvert_max) {
        SLONG old_offset;
        SLONG new_offset;

        old_offset = ((UBYTE*)FARFACET_lvert) - ((UBYTE*)FARFACET_lvert_buffer);

        FARFACET_lvert_max *= 2;
        FARFACET_lvert_buffer = (D3DLVERTEX*)realloc(FARFACET_lvert_buffer, sizeof(D3DLVERTEX) * FARFACET_lvert_max + 31);
        ASSERT(FARFACET_lvert_buffer != NULL);
        FARFACET_lvert = (D3DLVERTEX*)((SLONG(FARFACET_lvert_buffer) + 31) & ~0x1f);

        ASSERT(FARFACET_lvert_upto < FARFACET_lvert_max);

        new_offset = ((UBYTE*)FARFACET_lvert) - ((UBYTE*)FARFACET_lvert_buffer);

        if (new_offset != old_offset) {
            memmove(((UBYTE*)FARFACET_lvert_buffer) + new_offset, ((UBYTE*)FARFACET_lvert_buffer) + old_offset, sizeof(D3DLVERTEX) * FARFACET_lvert_upto);
        }
    }

    ASSERT(fs->lvert + fs->lvertcount == FARFACET_lvert_upto);

    lv = &FARFACET_lvert[FARFACET_lvert_upto++];

    lv->x = x;
    lv->y = y;
    lv->z = z;
    lv->tu = 0.0F;
    lv->tv = 0.0F;
    lv->color = 0x00000000;
    lv->specular = 0x00000000;
    lv->dwReserved = 0;

    return fs->lvertcount++;
}

// uc_orig: FARFACET_add_index (fallen/DDEngine/Source/farfacet.cpp)
// Appends an index value to the index buffer, growing it if needed.
static void FARFACET_add_index(FARFACET_Square* fs, UWORD index)
{
    if (FARFACET_index_upto >= FARFACET_index_max) {
        FARFACET_index_max *= 2;
        FARFACET_index = (UWORD*)realloc(FARFACET_index, sizeof(UWORD) * FARFACET_index_max);
    }

    ASSERT(FARFACET_index_upto == fs->index + fs->indexcount);

    FARFACET_index[FARFACET_index_upto] = index;

    FARFACET_index_upto += 1;
    fs->indexcount += 1;
}

// uc_orig: FARFACET_create_square (fallen/DDEngine/Source/farfacet.cpp)
// Builds DrawIndexedPrimitive data for a single farfacet square.
// Gathers all facets in the covered lo-res squares, merges aligned rectangles into strips,
// then converts strips into indexed triangle lists.
static void FARFACET_create_square(SLONG square_x, SLONG square_z)
{
    SLONG i;
    SLONG j;
    SLONG f_list;
    SLONG facet;
    SLONG build;
    SLONG exit;
    SLONG lvert_memory;
    SLONG old_outline_upto;

    SLONG v1;
    SLONG v2;

    SLONG dx1;
    SLONG dz1;

    SLONG dx2;
    SLONG dz2;

    FARFACET_Outline* fo;
    FARFACET_Outline* fo1;
    FARFACET_Outline* fo2;
    DFacet* df;
    FARFACET_Square* fs;

    ASSERT(WITHIN(square_x, 0, FARFACET_SIZE - 1));
    ASSERT(WITHIN(square_z, 0, FARFACET_SIZE - 1));

    fs = &FARFACET_square[square_x][square_z];

    memset(FARFACET_outline, 0, sizeof(FARFACET_outline));

    FARFACET_outline_upto = 0;

    // Mark all facets as not yet processed.
    for (i = 1; i < next_dfacet; i++) {
        df = &dfacets[i];
        df->Counter[0] = UC_FALSE;
    }

    SLONG lo_min_x;
    SLONG lo_min_z;

    SLONG lo_max_x;
    SLONG lo_max_z;

    SLONG lo_x;
    SLONG lo_z;

    lo_min_x = (square_x + 0) * FARFACET_RATIO;
    lo_min_z = (square_z + 0) * FARFACET_RATIO;

    lo_max_x = (square_x + 1) * FARFACET_RATIO;
    lo_max_z = (square_z + 1) * FARFACET_RATIO;

    SLONG hi_min_x = lo_min_x * 4;
    SLONG hi_min_z = lo_min_z * 4;

    SLONG hi_max_x = lo_max_x * 4;
    SLONG hi_max_z = lo_max_z * 4;

    // Collect outlines of all eligible facets in this farfacet square.
    for (lo_x = lo_min_x; lo_x < lo_max_x; lo_x++)
        for (lo_z = lo_min_z; lo_z < lo_max_z; lo_z++) {
            f_list = PAP_2LO(lo_x, lo_z).ColVectHead;

            if (f_list) {
                exit = UC_FALSE;

                while (!exit) {
                    facet = facet_links[f_list];

                    if (facet < 0) {
                        facet = -facet;
                        exit = UC_TRUE;
                    }

                    ASSERT(WITHIN(facet, 1, next_dfacet - 1));

                    df = &dfacets[facet];

                    if (df->Counter[0]) {
                        goto abort_facet;
                    }

                    df->Counter[0] = UC_TRUE;

                    // Only draw normal walls and doors; skip foundations and inside-only walls.
                    if (df->FacetType != STOREY_TYPE_NORMAL && df->FacetType != STOREY_TYPE_DOOR) {
                        goto abort_facet;
                    }

                    if (dbuildings[df->Building].Type == BUILDING_TYPE_CRATE_IN) {
                        goto abort_facet;
                    }

                    if (df->FacetFlags & FACET_FLAG_INSIDE) {
                        goto abort_facet;
                    }

                    ASSERT(WITHIN(FARFACET_outline_upto, 0, FARFACET_MAX_OUTLINES - 1));

                    fo = &FARFACET_outline[FARFACET_outline_upto++];

                    fo->x1 = df->x[0];
                    fo->y1 = df->Y[0] >> 6;
                    fo->z1 = df->z[0];

                    fo->x2 = df->x[1];
                    fo->y2 = df->Y[0] + df->Height * df->BlockHeight * 4 >> 6;
                    fo->z2 = df->z[1];

                    SATURATE(fo->x1, hi_min_x, hi_max_x);
                    SATURATE(fo->z1, hi_min_z, hi_max_z);

                    SATURATE(fo->x2, hi_min_x, hi_max_x);
                    SATURATE(fo->z2, hi_min_z, hi_max_z);

                abort_facet:;

                    f_list++;
                }
            }
        }

    // Merge aligned adjacent outline rectangles (vertically stacked or horizontally adjacent).
    while (1) {
        old_outline_upto = FARFACET_outline_upto;

        for (i = 0; i < FARFACET_outline_upto; i++) {
            fo1 = &FARFACET_outline[i];

            dx1 = SIGN(fo1->x2 - fo1->x1);
            dz1 = SIGN(fo1->z2 - fo1->z1);

            for (j = i + 1; j < FARFACET_outline_upto; j++) {
                fo2 = &FARFACET_outline[j];

                dx2 = SIGN(fo2->x2 - fo2->x1);
                dz2 = SIGN(fo2->z2 - fo2->z1);

                if (dx1 == dx2 && dz1 == dz2) {
                    if (fo1->x1 == fo2->x1 && fo1->z1 == fo2->z1 && fo1->x2 == fo2->x2 && fo1->z2 == fo2->z2) {
                        // Same footprint: try to merge vertically.
                        if (fo1->y2 == fo2->y1) {
                            fo1->y2 = fo2->y2;
                            FARFACET_outline[j] = FARFACET_outline[--FARFACET_outline_upto];
                            j--;
                        } else if (fo1->y1 == fo2->y2) {
                            fo1->y1 = fo2->y1;
                            FARFACET_outline[j] = FARFACET_outline[--FARFACET_outline_upto];
                            j--;
                        }
                    } else if (fo1->y1 == fo2->y1 && fo1->y2 == fo2->y2) {
                        // Same height range: try to merge horizontally.
                        if (fo1->x1 == fo2->x2 && fo1->z1 == fo2->z2) {
                            fo1->x1 = fo2->x1;
                            fo1->z1 = fo2->z1;
                            FARFACET_outline[j] = FARFACET_outline[--FARFACET_outline_upto];
                            j--;
                        } else if (fo1->x2 == fo2->x1 && fo1->z2 == fo2->z1) {
                            fo1->x2 = fo2->x2;
                            fo1->z2 = fo2->z2;
                            FARFACET_outline[j] = FARFACET_outline[--FARFACET_outline_upto];
                            j--;
                        }
                    }
                }
            }
        }

        if (FARFACET_outline_upto == old_outline_upto) {
            break;
        }
    }

    // Convert outlines into strips, then into indexed triangles.
    fs->lvert = FARFACET_lvert_upto;
    fs->lvertcount = 0;
    fs->index = FARFACET_index_upto;
    fs->indexcount = 0;

#define FARFACET_MAX_STRIP_LENGTH 256

    UWORD strip[FARFACET_MAX_STRIP_LENGTH];
    SLONG strip_upto;

    while (1) {
        if (FARFACET_outline_upto == 0) {
            break;
        }

        // Start a strip from the last outline entry.
        strip[0] = FARFACET_outline_upto - 1;
        strip_upto = 1;
        FARFACET_outline_upto -= 1;

        // Grow the strip by prepending outlines that connect to the current head.
        fo1 = &FARFACET_outline[strip[0]];

    add_onto_beginning:;

        for (i = 0; i < FARFACET_outline_upto; i++) {
            fo2 = &FARFACET_outline[i];

            if (fo2->x2 == fo1->x1 && fo2->y2 == fo1->y1 && fo2->z2 == fo1->z1) {
                FARFACET_outline_upto -= 1;

                SWAP(fo2->x1, FARFACET_outline[FARFACET_outline_upto].x1);
                SWAP(fo2->y1, FARFACET_outline[FARFACET_outline_upto].y1);
                SWAP(fo2->z1, FARFACET_outline[FARFACET_outline_upto].z1);

                SWAP(fo2->x2, FARFACET_outline[FARFACET_outline_upto].x2);
                SWAP(fo2->y2, FARFACET_outline[FARFACET_outline_upto].y2);
                SWAP(fo2->z2, FARFACET_outline[FARFACET_outline_upto].z2);

                ASSERT(WITHIN(strip_upto, 1, FARFACET_MAX_STRIP_LENGTH - 1));

                memmove(strip + 1, strip + 0, sizeof(UWORD) * strip_upto);
                strip_upto += 1;

                strip[0] = FARFACET_outline_upto;

                goto add_onto_beginning;
            }
        }

        // Grow the strip by appending outlines that connect to the current tail.
        ASSERT(WITHIN(strip_upto, 1, FARFACET_MAX_STRIP_LENGTH - 1));

        fo1 = &FARFACET_outline[strip[strip_upto - 1]];

    add_onto_end:;

        for (i = 0; i < FARFACET_outline_upto; i++) {
            fo2 = &FARFACET_outline[i];

            if (fo2->x1 == fo1->x2 && fo2->y1 == fo1->y2 && fo2->z1 == fo1->z2) {
                FARFACET_outline_upto -= 1;

                SWAP(fo2->x1, FARFACET_outline[FARFACET_outline_upto].x1);
                SWAP(fo2->y1, FARFACET_outline[FARFACET_outline_upto].y1);
                SWAP(fo2->z1, FARFACET_outline[FARFACET_outline_upto].z1);

                SWAP(fo2->x2, FARFACET_outline[FARFACET_outline_upto].x2);
                SWAP(fo2->y2, FARFACET_outline[FARFACET_outline_upto].y2);
                SWAP(fo2->z2, FARFACET_outline[FARFACET_outline_upto].z2);

                ASSERT(WITHIN(strip_upto, 1, FARFACET_MAX_STRIP_LENGTH - 1));

                strip[strip_upto++] = FARFACET_outline_upto;

                goto add_onto_end;
            }
        }

        // Emit the strip as a triangle list into the square's index buffer.
        fo = &FARFACET_outline[strip[0]];

        v1 = FARFACET_find_vertex(fs, fo->x1, fo->y1, fo->z1);
        v2 = FARFACET_find_vertex(fs, fo->x1, fo->y2, fo->z1);

        SLONG v1prev = v1;
        SLONG v2prev = v2;

        for (i = 0; i < strip_upto; i++) {
            fo = &FARFACET_outline[strip[i]];

            v1 = FARFACET_find_vertex(fs, fo->x2, fo->y1, fo->z2);
            v2 = FARFACET_find_vertex(fs, fo->x2, fo->y2, fo->z2);

            FARFACET_add_index(fs, v1prev);
            FARFACET_add_index(fs, v2prev);
            FARFACET_add_index(fs, v1);
            FARFACET_add_index(fs, v1);
            FARFACET_add_index(fs, v2prev);
            FARFACET_add_index(fs, v2);
            v1prev = v1;
            v2prev = v2;
        }
    }
}

// uc_orig: FARFACET_init (fallen/DDEngine/Source/farfacet.cpp)
void FARFACET_init()
{
    FARFACET_lvert_max = 1024;
    FARFACET_lvert_upto = 0;
    FARFACET_lvert_buffer = (D3DLVERTEX*)malloc(sizeof(D3DLVERTEX) * FARFACET_lvert_max + 31);
    FARFACET_lvert = (D3DLVERTEX*)((SLONG(FARFACET_lvert_buffer) + 31) & ~0x1f);

    FARFACET_index_max = FARFACET_lvert_max * 5 / 4;
    FARFACET_index_upto = 0;
    FARFACET_index = (UWORD*)malloc(sizeof(UWORD) * FARFACET_index_max);

    FARFACET_outline = (FARFACET_Outline*)malloc(sizeof(FARFACET_Outline) * FARFACET_MAX_OUTLINES);
    FARFACET_outline_upto = 0;

    memset(FARFACET_square, 0, sizeof(FARFACET_square));
    memset(FARFACET_lvert, 0, sizeof(D3DLVERTEX) * FARFACET_lvert_max);
    memset(FARFACET_index, 0, sizeof(UWORD) * FARFACET_index_max);
    memset(FARFACET_outline, 0, sizeof(FARFACET_Outline) * FARFACET_MAX_OUTLINES);

    FARFACET_matrix = (D3DMATRIX*)((SLONG(FARFACET_matrix_buffer) + 31) & ~0x1f);

    SLONG x;
    SLONG z;

    for (x = 0; x < FARFACET_SIZE; x++)
        for (z = 0; z < FARFACET_SIZE; z++) {
            FARFACET_create_square(x, z);
        }

    free(FARFACET_outline);

    FARFACET_renderstate.SetRenderState(D3DRENDERSTATE_FOGENABLE, UC_TRUE);
    FARFACET_renderstate.SetTexture(NULL);
}

// uc_orig: FARFACET_draw (fallen/DDEngine/Source/farfacet.cpp)
void FARFACET_draw(
    float x,
    float y,
    float z,
    float yaw,
    float pitch,
    float roll,
    float draw_dist,
    float lens)
{
    if (!Keys[KB_R]) {
        return;
    }

    float width;
    float height;
    float depth;
    float aspect;
    float matrix[9];

    struct
    {
        float x;
        float z;

    } cone[5];

    MATRIX_calc(
        matrix,
        yaw,
        pitch,
        roll);

    FARFACET_num_squares_drawn = 0;

    depth = draw_dist;
    width = draw_dist;
    height = draw_dist;

    width *= POLY_screen_width;
    width /= POLY_screen_height;

    width /= lens;
    height /= lens;

    // Project the four corners of the view frustum onto the XZ plane.
    cone[3].x = cone[4].x = x / 256.0f;
    cone[3].z = cone[4].z = z / 256.0f;

    cone[3].x += depth * matrix[6];
    cone[3].z += depth * matrix[8];

    cone[0].x = cone[3].x + height * matrix[3];
    cone[0].z = cone[3].z + height * matrix[5];

    cone[0].x = cone[0].x - width * matrix[0];
    cone[0].z = cone[0].z - width * matrix[2];

    cone[1].x = cone[3].x + height * matrix[3];
    cone[1].z = cone[3].z + height * matrix[5];

    cone[1].x = cone[1].x + width * matrix[0];
    cone[1].z = cone[1].z + width * matrix[2];

    cone[2].x = cone[3].x - height * matrix[3];
    cone[2].z = cone[3].z - height * matrix[5];

    cone[2].x = cone[2].x + width * matrix[0];
    cone[2].z = cone[2].z + width * matrix[2];

    cone[3].x = cone[3].x - height * matrix[3];
    cone[3].z = cone[3].z - height * matrix[5];

    cone[3].x = cone[3].x - width * matrix[0];
    cone[3].z = cone[3].z - width * matrix[2];

    SLONG square_min_x = +UC_INFINITY;
    SLONG square_min_z = +UC_INFINITY;

    SLONG square_max_x = -UC_INFINITY;
    SLONG square_max_z = -UC_INFINITY;

    SLONG i;

    for (i = 0; i < 5; i++) {
        if (ftol(cone[i].x) < square_min_x) {
            square_min_x = ftol(cone[i].x);
        }
        if (ftol(cone[i].z) < square_min_z) {
            square_min_z = ftol(cone[i].z);
        }

        if (ftol(cone[i].x) > square_max_x) {
            square_max_x = ftol(cone[i].x);
        }
        if (ftol(cone[i].z) > square_max_z) {
            square_max_z = ftol(cone[i].z);
        }
    }

    // Convert world coordinates to farfacet square indices.
    square_min_x /= 4 * FARFACET_RATIO;
    square_min_z /= 4 * FARFACET_RATIO;

    square_max_x /= 4 * FARFACET_RATIO;
    square_max_z /= 4 * FARFACET_RATIO;

    SATURATE(square_min_x, 0, FARFACET_SIZE - 1);
    SATURATE(square_min_z, 0, FARFACET_SIZE - 1);

    SATURATE(square_max_x, 0, FARFACET_SIZE - 1);
    SATURATE(square_max_z, 0, FARFACET_SIZE - 1);

    FARFACET_renderstate.SetChanged();

    // Slightly scale the projection matrix to push RHW values further back,
    // acting as a Z-bias to avoid Z-fighting with the normal scene polygons.
#define MY_PROJ_MATRIX_SCALE 1.01f
    D3DMATRIX matMyProj = g_matProjection;
    matMyProj._11 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._12 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._13 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._14 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._21 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._22 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._23 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._24 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._31 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._32 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._33 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._34 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._41 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._42 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._43 *= MY_PROJ_MATRIX_SCALE;
    matMyProj._44 *= MY_PROJ_MATRIX_SCALE;

    (the_display.lp_D3D_Device)->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matMyProj);

    SLONG square_x;
    SLONG square_z;

    float mid_x;
    float mid_y;
    float mid_z;

    float dx;
    float dy;
    float dz;

    float dist;
    float dprod;

    FARFACET_Square* fs;

    for (square_x = square_min_x; square_x <= square_max_x; square_x++)
        for (square_z = square_min_z; square_z <= square_max_z; square_z++) {
            fs = &FARFACET_square[square_x][square_z];

            if (fs->indexcount) {
                mid_y = 0.0F;

                mid_x = (float(square_x) + 0.5F) * (4.0F * FARFACET_RATIO * 256.0F);
                mid_z = (float(square_z) + 0.5F) * (4.0F * FARFACET_RATIO * 256.0F);

                dx = mid_x - x;
                dy = mid_y - y;
                dz = mid_z - z;

                dprod = dx * matrix[6] + dy * matrix[7] + dz * matrix[8];

                // Only draw squares beyond the fog/draw distance.
                extern SLONG CurDrawDistance;
                if (dprod * 2.5f < (float)CurDrawDistance) {
                    continue;
                }

                FARFACET_num_squares_drawn += 1;

                the_display.DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST,
                    D3DFVF_LVERTEX,
                    FARFACET_lvert + fs->lvert,
                    fs->lvertcount,
                    FARFACET_index + fs->index,
                    fs->indexcount,
                    D3DDP_DONOTUPDATEEXTENTS);
            }
        }

    FARFACET_default_renderstate.SetChanged();

    (the_display.lp_D3D_Device)->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_matProjection);
}

// uc_orig: FARFACET_fini (fallen/DDEngine/Source/farfacet.cpp)
void FARFACET_fini()
{
    free(FARFACET_lvert_buffer);
    free(FARFACET_index);
}
