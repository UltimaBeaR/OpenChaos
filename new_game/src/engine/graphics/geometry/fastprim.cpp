// Fast prim renderer: caches D3D vertex and index buffers per-prim to avoid rebuilding geometry
// each frame. Uses DrawIndPrimMM for opaque/tinted prims, DrawIndexedPrimitive for alpha and
// environment-mapped ones.

#include <platform.h>
#include "engine/graphics/geometry/fastprim.h"
#include "engine/graphics/geometry/fastprim_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/graphics_api/gd_display.h"
#include "core/matrix.h"
#include "assets/texture.h"
#include "world/environment/prim_types.h"    // PrimFace3/4, PrimObject, FACE_FLAG_*, PRIM_FLAG_*
#include "world/environment/prim.h"          // get_prim_info
#include "world/environment/building_types.h" // TEXTURE_PIECE_*
#include "world/level_pools.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"

// uc_orig: FASTPRIM_find_texture_from_page (fallen/DDEngine/Source/fastprim.cpp)
LPDIRECT3DTEXTURE2 FASTPRIM_find_texture_from_page(SLONG page)
{
    PolyPage* pp = &POLY_Page[page];
    return pp->RS.GetTexture();
}

// uc_orig: FASTPRIM_init (fallen/DDEngine/Source/fastprim.cpp)
void FASTPRIM_init()
{
    FASTPRIM_lvert_max = 4096;

    FASTPRIM_lvert_buffer = (D3DLVERTEX*)MemAlloc(sizeof(D3DLVERTEX) * FASTPRIM_lvert_max + 31);
    FASTPRIM_lvert = (D3DLVERTEX*)((((SLONG)FASTPRIM_lvert_buffer) + 31) & ~0x1f);
    FASTPRIM_lvert_upto = 0;
    FASTPRIM_lvert_free_end = FASTPRIM_lvert_max;
    FASTPRIM_lvert_free_unused = FASTPRIM_lvert_max;

    FASTPRIM_index_max = FASTPRIM_lvert_max * 3;
    FASTPRIM_index = (UWORD*)MemAlloc(sizeof(UWORD) * FASTPRIM_index_max);
    FASTPRIM_index_upto = 0;
    FASTPRIM_index_free_end = FASTPRIM_index_max;
    FASTPRIM_index_free_unused = FASTPRIM_index_max;

    FASTPRIM_call_upto = 0;

    FASTPRIM_queue_start = 0;
    FASTPRIM_queue_end = 0;

    memset(FASTPRIM_lvert, 0, sizeof(D3DLVERTEX) * FASTPRIM_lvert_max);
    memset(FASTPRIM_index, 0, sizeof(UWORD) * FASTPRIM_index_max);
    memset(FASTPRIM_call, 0, sizeof(FASTPRIM_call));
    memset(FASTPRIM_prim, 0, sizeof(FASTPRIM_prim));
    memset(FASTPRIM_queue, 0, sizeof(FASTPRIM_queue));

    FASTPRIM_matrix = (D3DMATRIX*)((SLONG(FASTPRIM_matrix_buffer) + 31) & ~0x1f);

    // Car wheels have rotating texture coordinates and cannot be cached.
    FASTPRIM_prim[PRIM_OBJ_CAR_WHEEL].flag = FASTPRIM_PRIM_FLAG_INVALID;
}

// Releases the vertex/index space used by a cached prim, updating the free-range cursors.
// uc_orig: FASTPRIM_free_cached_prim (fallen/DDEngine/Source/fastprim.cpp)
void FASTPRIM_free_cached_prim(SLONG prim)
{
    SLONG i;

    FASTPRIM_Call* fc;
    FASTPRIM_Prim* fp;

    ASSERT(WITHIN(prim, 0, FASTPRIM_MAX_PRIMS - 1));

    fp = &FASTPRIM_prim[prim];

    for (i = 0; i < fp->call_count; i++) {
        ASSERT(WITHIN(fp->call_index, 0, FASTPRIM_MAX_CALLS - 1));

        fc = &FASTPRIM_call[fp->call_index + i];

        FASTPRIM_lvert_free_end = fc->lvert + fc->lvertcount;
        FASTPRIM_index_free_end = fc->index + fc->indexcount;

        if (FASTPRIM_lvert_free_end >= FASTPRIM_lvert_free_unused || FASTPRIM_index_free_end >= FASTPRIM_index_free_unused) {
            FASTPRIM_lvert_free_end = FASTPRIM_lvert_max;
            FASTPRIM_index_free_end = FASTPRIM_index_max;

            FASTPRIM_lvert_free_unused = FASTPRIM_lvert_max;
            FASTPRIM_index_free_unused = FASTPRIM_index_max;

            break;
        }
    }

    fp->flag &= ~FASTPRIM_PRIM_FLAG_CACHED;
    fp->call_count = 0;
    fp->call_index = 0;
}

// Evicts oldest cached prims from the LRU queue until there is room for fc's data.
// May relocate fc's vertex/index data to the beginning of the arrays if they wrapped.
// uc_orig: FASTPRIM_free_queue_for_call (fallen/DDEngine/Source/fastprim.cpp)
void FASTPRIM_free_queue_for_call(FASTPRIM_Call* fc)
{
    SLONG old_lvert_free_end;
    SLONG old_index_free_end;

    SLONG copy_to_beginning = UC_FALSE;

    while (1) {
        old_lvert_free_end = FASTPRIM_lvert_free_end;
        old_index_free_end = FASTPRIM_index_free_end;

        ASSERT(FASTPRIM_queue_end < FASTPRIM_queue_start);

        FASTPRIM_free_cached_prim(FASTPRIM_queue[FASTPRIM_queue_end++ & (FASTPRIM_MAX_QUEUE - 1)]);

        if (FASTPRIM_lvert_free_end < old_lvert_free_end || FASTPRIM_index_free_end < old_index_free_end) {
            // The buffer wrapped around; copy the current call's data to the beginning.
            FASTPRIM_lvert_upto = fc->lvertcount;
            FASTPRIM_index_upto = fc->indexcount;

            copy_to_beginning = UC_TRUE;
        }

        if (FASTPRIM_index_upto + 16 < FASTPRIM_index_free_end && FASTPRIM_lvert_upto + 16 < FASTPRIM_lvert_free_end) {
            if (copy_to_beginning) {
                memcpy(FASTPRIM_lvert, FASTPRIM_lvert + fc->lvert, sizeof(D3DLVERTEX) * fc->lvertcount);
                memcpy(FASTPRIM_index, FASTPRIM_index + fc->index, sizeof(UWORD) * fc->indexcount);

                FASTPRIM_lvert_free_unused = fc->lvert;
                FASTPRIM_index_free_unused = fc->index;

                fc->lvert = 0;
                fc->index = 0;
            }

            return;
        }
    }
}

// Ensures the call list contains a slot for the given texture/type pair.
// No-op if a matching slot already exists.
// uc_orig: FASTPRIM_create_call (fallen/DDEngine/Source/fastprim.cpp)
void FASTPRIM_create_call(FASTPRIM_Prim* fp, LPDIRECT3DTEXTURE2 texture, UWORD type)
{
    SLONG i;

    FASTPRIM_Call* fc;

    for (i = 0; i < fp->call_count; i++) {
        ASSERT(WITHIN(fp->call_index + i, 0, FASTPRIM_call_upto - 1));

        fc = &FASTPRIM_call[fp->call_index + i];

        if (fc->texture == texture && fc->type == type) {
            return;
        }
    }

    ASSERT(WITHIN(FASTPRIM_call_upto, 0, FASTPRIM_MAX_CALLS - 1));
    ASSERT(fp->call_index + fp->call_count == FASTPRIM_call_upto);

    fc = &FASTPRIM_call[fp->call_index + fp->call_count];

    fc->type = type;
    fc->lvert = 0;
    fc->lvertcount = 0;
    fc->index = 0;
    fc->indexcount = 0;
    fc->texture = texture;

    fp->call_count += 1;
    FASTPRIM_call_upto += 1;

    return;
}

// Adds a vertex to the call's vertex buffer, reusing an existing entry if the vertex matches.
// Returns the within-call vertex index.
// uc_orig: FASTPRIM_add_point_to_call (fallen/DDEngine/Source/fastprim.cpp)
UWORD FASTPRIM_add_point_to_call(
    FASTPRIM_Call* fc,
    float x,
    float y,
    float z,
    float u,
    float v,
    ULONG colour,
    ULONG specular)
{
    SLONG i;

    D3DLVERTEX* lv;

    for (i = fc->lvertcount - 1; i >= 0; i--) {
        ASSERT(WITHIN(fc->lvert + i, 0, FASTPRIM_lvert_max - 1));

        lv = &FASTPRIM_lvert[fc->lvert + i];

        if (lv->x == x && lv->y == y && lv->z == z && lv->tu == u && lv->tv == v && lv->color == colour && lv->specular == specular) {
            return i;
        }
    }

    if (FASTPRIM_lvert_upto >= FASTPRIM_lvert_free_end) {
        FASTPRIM_free_queue_for_call(fc);
    }

    ASSERT(WITHIN(FASTPRIM_lvert_upto, 0, FASTPRIM_lvert_max - 1));
    ASSERT(fc->lvert + fc->lvertcount == FASTPRIM_lvert_upto);

    lv = &FASTPRIM_lvert[fc->lvert + fc->lvertcount];

    lv->x = x;
    lv->y = y;
    lv->z = z;
    lv->tu = u;
    lv->tv = v;
    lv->color = colour;
    lv->specular = specular;

    FASTPRIM_lvert_upto += 1;

    return fc->lvertcount++;
}

// Ensures the index buffer has room for at least 8 more indices, evicting if necessary.
// uc_orig: FASTPRIM_ensure_room_for_indices (fallen/DDEngine/Source/fastprim.cpp)
void FASTPRIM_ensure_room_for_indices(FASTPRIM_Call* fc)
{
    if (FASTPRIM_index_upto + 8 >= FASTPRIM_index_free_end) {

        FASTPRIM_free_queue_for_call(fc);
    }
}

// uc_orig: FASTPRIM_draw (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_draw(
    SLONG prim,
    float x,
    float y,
    float z,
    float matrix[9],
    NIGHT_Colour* lpc)
{

    if (!Keys[KB_R]) {
        return UC_FALSE;
    }

    SLONG i;
    SLONG j;
    SLONG k;
    SLONG page;
    SLONG type;
    SLONG index[4];

    float px;
    float py;
    float pz;
    float pu;
    float pv;

    ULONG pcolour;
    ULONG pspecular;

    FASTPRIM_Prim* fp;
    PrimObject* po;
    PrimFace4* f4;
    PrimFace3* f3;
    FASTPRIM_Call* fc;
    PolyPage* pp;

    LPDIRECT3DTEXTURE2 texture;

    ASSERT(WITHIN(prim, 0, FASTPRIM_MAX_PRIMS - 1));

    // Skip prims that are too close to the camera (drawn the old way instead).
    {
        PrimInfo* pi = get_prim_info(prim);

        extern float AENG_cam_x;
        extern float AENG_cam_z;

        float dx = x - AENG_cam_x;
        float dz = z - AENG_cam_z;

        float dist = dx * dx + dz * dz;
        float radius = float(MAX(pi->maxx - pi->minx, pi->maxz - pi->minz)) + 16.0f;

        if (dist < radius * radius) {
            return UC_FALSE;
        }
    }

    fp = &FASTPRIM_prim[prim];

    if (!Keys[KB_R]) {
        return UC_FALSE;
    }

    if (!(fp->flag & FASTPRIM_PRIM_FLAG_CACHED)) {
        // First time seeing this prim: build and cache its vertex/index data.

        if (FASTPRIM_call_upto + 32 >= FASTPRIM_MAX_CALLS) {
            FASTPRIM_call_upto = 0;
        }

        fp->call_count = 0;
        fp->call_index = FASTPRIM_call_upto;

        // Pass 1: create call slots for each unique texture/type combination.
        po = &prim_objects[prim];

        for (i = po->StartFace3; i < po->EndFace3; i++) {
            ASSERT(WITHIN(i, 0, next_prim_face3 - 1));

            f3 = &prim_faces3[i];

            page = f3->UV[0][0] & 0xc0;
            page <<= 2;
            page |= f3->TexturePage;
            page += FACE_PAGE_OFFSET;

            texture = FASTPRIM_find_texture_from_page(page);

            if (POLY_Page[page].RS.IsAlphaBlendEnabled()) {
                type = FASTPRIM_CALL_TYPE_INDEXED;
            } else if (f3->FaceFlags & FACE_FLAG_TINT) {
                type = FASTPRIM_CALL_TYPE_COLOURAND;
            } else {
                type = FASTPRIM_CALL_TYPE_NORMAL;
            }

            FASTPRIM_create_call(fp, texture, type);
        }

        for (i = po->StartFace4; i < po->EndFace4; i++) {
            ASSERT(WITHIN(i, 0, next_prim_face4 - 1));

            f4 = &prim_faces4[i];

            page = f4->UV[0][0] & 0xc0;
            page <<= 2;
            page |= f4->TexturePage;
            page += FACE_PAGE_OFFSET;

            texture = FASTPRIM_find_texture_from_page(page);

            if (POLY_Page[page].RS.IsAlphaBlendEnabled()) {
                type = FASTPRIM_CALL_TYPE_INDEXED;
            } else if (f4->FaceFlags & FACE_FLAG_TINT) {
                type = FASTPRIM_CALL_TYPE_COLOURAND;
            } else {
                type = FASTPRIM_CALL_TYPE_NORMAL;
            }

            FASTPRIM_create_call(fp, texture, type);
        }

        // Pass 2: for each call slot, fill in vertex and index data.
        for (i = 0; i < fp->call_count; i++) {
            ASSERT(WITHIN(fp->call_index + i, 0, FASTPRIM_call_upto - 1));

            fc = &FASTPRIM_call[fp->call_index + i];

            fc->flag = 0;

            fc->lvert = FASTPRIM_lvert_upto;
            fc->index = FASTPRIM_index_upto;

            fc->lvertcount = 0;
            fc->indexcount = 0;

            for (j = po->StartFace3; j < po->EndFace3; j++) {
                ASSERT(WITHIN(j, 0, next_prim_face3 - 1));

                f3 = &prim_faces3[j];

                page = f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= f3->TexturePage;
                page += FACE_PAGE_OFFSET;

                texture = FASTPRIM_find_texture_from_page(page);

                if (POLY_Page[page].RS.IsAlphaBlendEnabled()) {
                    type = FASTPRIM_CALL_TYPE_INDEXED;
                } else if (f3->FaceFlags & FACE_FLAG_TINT) {
                    type = FASTPRIM_CALL_TYPE_COLOURAND;
                } else {
                    type = FASTPRIM_CALL_TYPE_NORMAL;
                }

                if (texture == fc->texture && type == fc->type) {
                    pp = &POLY_Page[page];

                    for (k = 0; k < 3; k++) {
                        ASSERT(WITHIN(f3->Points[k], 0, next_prim_point - 1));

                        px = AENG_dx_prim_points[f3->Points[k]].X;
                        py = AENG_dx_prim_points[f3->Points[k]].Y;
                        pz = AENG_dx_prim_points[f3->Points[k]].Z;

                        pu = (f3->UV[k][0] & 0x3f) * (1.0F / 32.0F);
                        pv = (f3->UV[k][1]) * (1.0F / 32.0F);

                        pu = pu * pp->m_UScale + pp->m_UOffset;
                        pv = pv * pp->m_VScale + pp->m_VOffset;

                        if (lpc) {
                            NIGHT_get_d3d_colour(
                                lpc[f3->Points[k] - po->StartPoint],
                                &pcolour,
                                &pspecular);
                        } else {
                            pcolour = NIGHT_amb_d3d_colour;
                            pspecular = NIGHT_amb_d3d_specular;
                        }

                        if (POLY_page_flag[page] & POLY_PAGE_FLAG_SELF_ILLUM) {
                            pcolour = 0xffffff;
                            pspecular = 0xff000000;

                            fc->flag |= FASTPRIM_CALL_FLAG_SELF_ILLUM;
                        }

                        index[k] = FASTPRIM_add_point_to_call(fc, px, py, pz, pu, pv, pcolour, pspecular);

                        if (type == FASTPRIM_CALL_TYPE_COLOURAND) {
                            // Store the normal Y component in the top byte of dwReserved for
                            // per-call relighting.
                            ASSERT(WITHIN(fc->lvert + index[k], 0, FASTPRIM_lvert_max - 1));

                            FASTPRIM_lvert[fc->lvert + index[k]].dwReserved = prim_normal[f3->Points[k]].Y << 24;
                        } else {
                            // Store the point index in the top UWORD of dwReserved for
                            // per-vertex relighting (NORMAL and ENVMAP cases).
                            ASSERT(WITHIN(fc->lvert + index[k], 0, FASTPRIM_lvert_max - 1));

                            FASTPRIM_lvert[fc->lvert + index[k]].dwReserved = (f3->Points[k] - po->StartPoint) << 16;
                        }
                    }

                    FASTPRIM_ensure_room_for_indices(fc);

                    ASSERT(WITHIN(FASTPRIM_index_upto + 3, 0, FASTPRIM_index_max));
                    ASSERT(fc->index + fc->indexcount == FASTPRIM_index_upto);

                    if (type != FASTPRIM_CALL_TYPE_INDEXED) {
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[0];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[1];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[2];
                        FASTPRIM_index[FASTPRIM_index_upto++] = -1;

                        fc->indexcount += 4;
                    } else {
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[0];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[1];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[2];

                        fc->indexcount += 3;
                    }
                }
            }

            for (j = po->StartFace4; j < po->EndFace4; j++) {
                ASSERT(WITHIN(j, 0, next_prim_face4 - 1));

                f4 = &prim_faces4[j];

                page = f4->UV[0][0] & 0xc0;
                page <<= 2;
                page |= f4->TexturePage;
                page += FACE_PAGE_OFFSET;

                texture = FASTPRIM_find_texture_from_page(page);

                if (POLY_Page[page].RS.IsAlphaBlendEnabled()) {
                    type = FASTPRIM_CALL_TYPE_INDEXED;
                } else if (f4->FaceFlags & FACE_FLAG_TINT) {
                    type = FASTPRIM_CALL_TYPE_COLOURAND;
                } else {
                    type = FASTPRIM_CALL_TYPE_NORMAL;
                }

                if (texture == fc->texture && type == fc->type) {
                    pp = &POLY_Page[page];

                    for (k = 0; k < 4; k++) {
                        ASSERT(WITHIN(f4->Points[k], 0, next_prim_point - 1));

                        px = AENG_dx_prim_points[f4->Points[k]].X;
                        py = AENG_dx_prim_points[f4->Points[k]].Y;
                        pz = AENG_dx_prim_points[f4->Points[k]].Z;

                        pu = (f4->UV[k][0] & 0x3f) * (1.0F / 32.0F);
                        pv = (f4->UV[k][1]) * (1.0F / 32.0F);

                        pu = pu * pp->m_UScale + pp->m_UOffset;
                        pv = pv * pp->m_VScale + pp->m_VOffset;

                        if (lpc) {
                            NIGHT_get_d3d_colour(
                                lpc[f4->Points[k] - po->StartPoint],
                                &pcolour,
                                &pspecular);
                        } else {
                            pcolour = NIGHT_amb_d3d_colour;
                            pspecular = NIGHT_amb_d3d_specular;
                        }

                        if (POLY_page_flag[page] & POLY_PAGE_FLAG_SELF_ILLUM) {
                            pcolour = 0xffffff;
                            pspecular = 0xff000000;
                        }

                        index[k] = FASTPRIM_add_point_to_call(fc, px, py, pz, pu, pv, pcolour, pspecular);

                        if (type == FASTPRIM_CALL_TYPE_COLOURAND) {
                            ASSERT(WITHIN(fc->lvert + index[k], 0, FASTPRIM_lvert_max - 1));

                            FASTPRIM_lvert[fc->lvert + index[k]].dwReserved = prim_normal[f4->Points[k]].Y << 24;
                        } else {
                            ASSERT(WITHIN(fc->lvert + index[k], 0, FASTPRIM_lvert_max - 1));

                            FASTPRIM_lvert[fc->lvert + index[k]].dwReserved = (f4->Points[k] - po->StartPoint) << 16;
                        }
                    }

                    FASTPRIM_ensure_room_for_indices(fc);

                    ASSERT(WITHIN(FASTPRIM_index_upto + 6, 0, FASTPRIM_index_max));
                    ASSERT(fc->index + fc->indexcount == FASTPRIM_index_upto);

                    if (type != FASTPRIM_CALL_TYPE_INDEXED) {
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[0];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[1];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[2];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[3];
                        FASTPRIM_index[FASTPRIM_index_upto++] = -1;

                        fc->indexcount += 5;
                    } else {
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[0];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[1];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[2];

                        FASTPRIM_index[FASTPRIM_index_upto++] = index[3];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[2];
                        FASTPRIM_index[FASTPRIM_index_upto++] = index[1];

                        fc->indexcount += 6;
                    }
                }
            }
        }

        if (po->flag & PRIM_FLAG_ENVMAPPED) {
            // Build the environment-mapped call: one pass for faces marked FACE_FLAG_ENVMAP.

            LPDIRECT3DTEXTURE2 envtexture = FASTPRIM_find_texture_from_page(POLY_PAGE_ENVMAP);

            FASTPRIM_create_call(fp, envtexture, FASTPRIM_CALL_TYPE_ENVMAP);

            ASSERT(WITHIN(fp->call_index + fp->call_count - 1, 0, FASTPRIM_call_upto - 1));

            fc = &FASTPRIM_call[fp->call_index + fp->call_count - 1];

            ASSERT(fc->texture == envtexture && fc->type == FASTPRIM_CALL_TYPE_ENVMAP);

            fc->flag = 0;

            fc->lvert = FASTPRIM_lvert_upto;
            fc->index = FASTPRIM_index_upto;

            fc->lvertcount = 0;
            fc->indexcount = 0;

            for (i = po->StartFace3; i < po->EndFace3; i++) {
                ASSERT(WITHIN(i, 0, next_prim_face3 - 1));

                f3 = &prim_faces3[i];

                if (f3->FaceFlags & FACE_FLAG_ENVMAP) {
                    for (j = 0; j < 3; j++) {
                        ASSERT(WITHIN(f3->Points[j], 0, next_prim_point - 1));

                        px = AENG_dx_prim_points[f3->Points[j]].X;
                        py = AENG_dx_prim_points[f3->Points[j]].Y;
                        pz = AENG_dx_prim_points[f3->Points[j]].Z;

                        pu = 0.0F;
                        pv = 0.0F;

                        pcolour = 0xff888888;
                        pspecular = 0x00000000;

                        index[j] = FASTPRIM_add_point_to_call(fc, px, py, pz, pu, pv, pcolour, pspecular);

                        // Store the absolute point index for normal lookup during env-map update.
                        ASSERT(WITHIN(fc->lvert + index[j], 0, FASTPRIM_lvert_max - 1));

                        FASTPRIM_lvert[fc->lvert + index[j]].dwReserved = f3->Points[j] << 16;
                    }

                    FASTPRIM_ensure_room_for_indices(fc);

                    ASSERT(WITHIN(FASTPRIM_index_upto + 4, 0, FASTPRIM_index_max));
                    ASSERT(fc->index + fc->indexcount == FASTPRIM_index_upto);

                    FASTPRIM_index[FASTPRIM_index_upto++] = index[0];
                    FASTPRIM_index[FASTPRIM_index_upto++] = index[1];
                    FASTPRIM_index[FASTPRIM_index_upto++] = index[2];

                    fc->indexcount += 3;
                }
            }

            for (i = po->StartFace4; i < po->EndFace4; i++) {
                ASSERT(WITHIN(i, 0, next_prim_face4 - 1));

                f4 = &prim_faces4[i];

                if (f4->FaceFlags & FACE_FLAG_ENVMAP) {
                    for (j = 0; j < 4; j++) {
                        ASSERT(WITHIN(f4->Points[j], 0, next_prim_point - 1));

                        px = AENG_dx_prim_points[f4->Points[j]].X;
                        py = AENG_dx_prim_points[f4->Points[j]].Y;
                        pz = AENG_dx_prim_points[f4->Points[j]].Z;

                        pu = 0.0F;
                        pv = 0.0F;

                        pcolour = 0xff888888;
                        pspecular = 0x00000000;

                        index[j] = FASTPRIM_add_point_to_call(fc, px, py, pz, pu, pv, pcolour, pspecular);

                        ASSERT(WITHIN(fc->lvert + index[j], 0, FASTPRIM_lvert_max - 1));

                        FASTPRIM_lvert[fc->lvert + index[j]].dwReserved = f4->Points[j] << 16;
                    }

                    FASTPRIM_ensure_room_for_indices(fc);

                    ASSERT(WITHIN(FASTPRIM_index_upto + 5, 0, FASTPRIM_index_max));
                    ASSERT(fc->index + fc->indexcount == FASTPRIM_index_upto);

                    FASTPRIM_index[FASTPRIM_index_upto++] = index[0];
                    FASTPRIM_index[FASTPRIM_index_upto++] = index[1];
                    FASTPRIM_index[FASTPRIM_index_upto++] = index[2];

                    FASTPRIM_index[FASTPRIM_index_upto++] = index[3];
                    FASTPRIM_index[FASTPRIM_index_upto++] = index[2];
                    FASTPRIM_index[FASTPRIM_index_upto++] = index[1];

                    fc->indexcount += 6;
                }
            }
        }

        fp->flag |= FASTPRIM_PRIM_FLAG_CACHED;

        FASTPRIM_queue[FASTPRIM_queue_start++ & (FASTPRIM_MAX_QUEUE - 1)] = prim;
    }

    // kludge_shrink: duplicated block in the original source — preserved 1:1.
    extern UBYTE kludge_shrink;

    if (kludge_shrink) {
        float fstretch;

        fstretch = (prim == PRIM_OBJ_ITEM_AMMO_SHOTGUN) ? 0.15F : 0.7F;

        matrix[0] *= fstretch;
        matrix[1] *= fstretch;
        matrix[2] *= fstretch;

        matrix[3] *= fstretch;
        matrix[4] *= fstretch;
        matrix[5] *= fstretch;

        matrix[6] *= fstretch;
        matrix[7] *= fstretch;
        matrix[8] *= fstretch;
    }

    // kludge_shrink: second identical block — duplicate from the original, preserved 1:1.
    extern UBYTE kludge_shrink;

    if (kludge_shrink) {
        float fstretch;

        fstretch = (prim == PRIM_OBJ_ITEM_AMMO_SHOTGUN) ? 0.15F : 0.7F;

        matrix[0] *= fstretch;
        matrix[1] *= fstretch;
        matrix[2] *= fstretch;

        matrix[3] *= fstretch;
        matrix[4] *= fstretch;
        matrix[5] *= fstretch;

        matrix[6] *= fstretch;
        matrix[7] *= fstretch;
        matrix[8] *= fstretch;
    }

    // Set the object's position and rotation, then render each call.
    POLY_set_local_rotation(
        x, y, z,
        matrix);

    GenerateMMMatrixFromStandardD3DOnes(
        FASTPRIM_matrix,
        &g_matProjection,
        &g_matWorld,
        &g_viewData);

    for (i = 0; i < fp->call_count; i++) {
        ASSERT(WITHIN(fp->call_index + i, 0, FASTPRIM_MAX_CALLS - 1));

        fc = &FASTPRIM_call[fp->call_index + i];

        if (fc->type == FASTPRIM_CALL_TYPE_ENVMAP) {
            // Recompute environment-map UV coordinates each frame based on camera orientation.
            float nx;
            float ny;
            float nz;

            float comb[9];
            float cam_matrix[9];

            extern float AENG_cam_yaw;
            extern float AENG_cam_pitch;
            extern float AENG_cam_roll;

            D3DLVERTEX* lv;

            MATRIX_calc(cam_matrix, AENG_cam_yaw, AENG_cam_pitch, AENG_cam_roll);
            MATRIX_3x3mul(comb, cam_matrix, matrix);

            for (j = 0; j < fc->lvertcount; j++) {
                ASSERT(WITHIN(fc->lvert + j, 0, FASTPRIM_lvert_max - 1));

                lv = &FASTPRIM_lvert[fc->lvert + j];

                ASSERT(WITHIN(lv->dwReserved >> 16, 0, (unsigned)(next_prim_point - 1)));

                nx = prim_normal[lv->dwReserved >> 16].X * (2.0F / 256.0F);
                ny = prim_normal[lv->dwReserved >> 16].Y * (2.0F / 256.0F);
                nz = prim_normal[lv->dwReserved >> 16].Z * (2.0F / 256.0F);

                MATRIX_MUL(
                    comb,
                    nx,
                    ny,
                    nz);

                lv->tu = (nx * 0.5F) + 0.5F;
                lv->tv = (ny * 0.5F) + 0.5F;
            }
        } else if (fc->type == FASTPRIM_CALL_TYPE_COLOURAND) {
            ULONG default_colour;
            ULONG default_specular;

            NIGHT_get_d3d_colour(
                NIGHT_get_light_at(x, y, z),
                &default_colour,
                &default_specular);

            extern ULONG MESH_colour_and;

            default_colour &= MESH_colour_and;

            D3DLVERTEX* lv;

            for (j = 0; j < fc->lvertcount; j++) {
                ASSERT(WITHIN(fc->lvert + j, 0, FASTPRIM_lvert_max - 1));

                lv = &FASTPRIM_lvert[fc->lvert + j];

                lv->color = default_colour;
                lv->specular = default_specular;
            }
        } else if (fc->type == FASTPRIM_CALL_TYPE_NORMAL) {
            if (lpc) {
                // Relight each vertex from the cached per-point light colours.
                D3DLVERTEX* lv;

                for (j = 0; j < fc->lvertcount; j++) {
                    ASSERT(WITHIN(fc->lvert + j, 0, FASTPRIM_lvert_max - 1));

                    lv = &FASTPRIM_lvert[fc->lvert + j];

                    NIGHT_get_d3d_colour(
                        lpc[lv->dwReserved >> 16],
                        &lv->color,
                        &lv->specular);
                }
            } else {
                // Relight all vertices to the local ambient.
                ULONG default_colour;
                ULONG default_specular;

                NIGHT_get_d3d_colour(
                    NIGHT_get_light_at(x, y, z),
                    &default_colour,
                    &default_specular);

                D3DLVERTEX* lv;

                for (j = 0; j < fc->lvertcount; j++) {
                    ASSERT(WITHIN(fc->lvert + j, 0, FASTPRIM_lvert_max - 1));

                    lv = &FASTPRIM_lvert[fc->lvert + j];

                    lv->color = default_colour;
                    lv->specular = default_specular;
                }
            }
        }

        if (fc->type == FASTPRIM_CALL_TYPE_INDEXED || fc->type == FASTPRIM_CALL_TYPE_ENVMAP) {
            // Standard DrawIndexedPrimitive path (alpha-blended or environment-mapped).

            if (fc->type == FASTPRIM_CALL_TYPE_INDEXED) {
                the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
                the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
                the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
            } else {
                // Additive blend for environment maps.
                the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
                the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
                the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
            }

            the_display.lp_D3D_Device->SetTexture(0, fc->texture);

            the_display.lp_D3D_Device->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST,
                D3DFVF_LVERTEX,
                FASTPRIM_lvert + fc->lvert,
                fc->lvertcount,
                FASTPRIM_index + fc->index,
                fc->indexcount,
                0);

            the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_FALSE);
        } else {
            // DrawIndPrimMM path for opaque/colour-and prims (Tom's custom batch call).
            D3DMULTIMATRIX d3dmm = {
                FASTPRIM_lvert + fc->lvert,
                FASTPRIM_matrix,
                NULL,
                NULL
            };

            the_display.lp_D3D_Device->SetTexture(0, fc->texture);

            if (fc->flag & FASTPRIM_CALL_FLAG_SELF_ILLUM) {
                the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_DECAL);
            }

            DrawIndPrimMM(
                the_display.lp_D3D_Device,
                D3DFVF_LVERTEX,
                &d3dmm,
                fc->lvertcount,
                FASTPRIM_index + fc->index,
                fc->indexcount);

            if (fc->flag & FASTPRIM_CALL_FLAG_SELF_ILLUM) {
                the_display.lp_D3D_Device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
            }
        }
    }

    return UC_TRUE;
}

// uc_orig: FASTPRIM_fini (fallen/DDEngine/Source/fastprim.cpp)
void FASTPRIM_fini()
{
    MemFree(FASTPRIM_lvert_buffer);
    MemFree(FASTPRIM_index);

    FASTPRIM_lvert_buffer = NULL;
    FASTPRIM_index = NULL;
}
