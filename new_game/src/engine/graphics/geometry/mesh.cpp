#include "engine/graphics/geometry/mesh.h"
#include "engine/graphics/geometry/mesh_globals.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "engine/animation/morph.h"
#include "assets/texture.h"
#include "engine/core/matrix.h"
#include "engine/core/types.h"
#include "buildings/prim_types.h" // PrimObject, PrimFace3/4, PRIM_OBJ_*, PRIM_FLAG_*, FACE_FLAG_*
#include "buildings/prim.h"       // get_prim_info
#include "game/game_types.h"
#include "engine/graphics/geometry/shape_globals.h"
#include "map/level_pools.h"
#include "engine/core/memory.h"               // MemAlloc, MemFree (used by reflection mesh cache)
#include "assets/formats/anim_tmap.h"

#include <math.h>
#include <stdlib.h>                    // realloc, fabs

// uc_orig: frand (fallen/DDEngine/Source/mesh.cpp)
// Returns a random float in [0.0, 1.0].
static float frand(void)
{
    SLONG irand = rand();
    float ans = float(irand) * (1.0F / float(RAND_MAX));

    return ans;
}

// Per-call car crumple parameters set by MESH_set_crumple().
// Defined in mesh_globals.cpp.
extern UBYTE* car_crumples;
extern UBYTE* car_assign;

// kludge_shrink is defined in aeng.cpp (not yet migrated); it is set around
// DRAWXTRA_MIB_destruct calls to shrink prim 253 and any other prims to small scale.
extern UBYTE kludge_shrink;

// AENG_cam_yaw/pitch/roll are defined in aeng.cpp (not yet migrated).
extern float AENG_cam_yaw;
extern float AENG_cam_pitch;
extern float AENG_cam_roll;

// uc_orig: MESH_init (fallen/DDEngine/Source/mesh.cpp)
void MESH_init(void)
{
    SLONG i;
    SLONG j;

    float amp;

    for (i = 0; i < MESH_NUM_CRUMPLES; i++) {
        amp = float(i) * 2.5F;

        for (j = 0; j < MESH_NUM_CRUMPVALS; j++) {
            MESH_crumple[i][j].dx = ((frand() * 2.0F) - 1.0F) * amp;
            MESH_crumple[i][j].dy = ((frand() * 2.0F) - 1.0F) * amp;
            MESH_crumple[i][j].dz = ((frand() * 2.0F) - 1.0F) * amp;
        }
    }
}

// uc_orig: MESH_set_crumple (fallen/DDEngine/Source/mesh.cpp)
void MESH_set_crumple(UBYTE* assignments, UBYTE* crumples)
{
    car_assign = assignments;
    car_crumples = crumples;
}

// uc_orig: MESH_draw_guts (fallen/DDEngine/Source/mesh.cpp)
// Core rendering worker: transforms all vertices of the given prim into POLY_buffer[],
// then submits all quad and triangle faces. Called by MESH_draw_poly and MESH_draw_poly_inv_matrix.
// lpc: per-vertex lighting array or NULL for uniform ambient. fade: alpha packed in high byte.
// crumple >= 0: use generic crumple table. crumple == -1: use car zone crumple.
static NIGHT_Colour* MESH_draw_guts(
    SLONG prim,
    MAPCO16 at_x,
    MAPCO16 at_y,
    MAPCO16 at_z,
    float matrix[9],
    NIGHT_Colour* lpc,
    ULONG fade,
    SLONG crumple = 0)
{
    SLONG i;

    SLONG sp;
    SLONG ep;
    SLONG cv = 0;

    SLONG p0;
    SLONG p1;
    SLONG p2;
    SLONG p3;

    ULONG qc0;
    ULONG qc1;
    ULONG qc2;
    ULONG qc3;

    SLONG page;

    PrimFace4* p_f4;
    PrimFace3* p_f3;
    PrimObject* p_obj;

    POLY_Point* pp;

    POLY_Point* tri[3];
    POLY_Point* quad[4];

    ULONG default_colour;
    ULONG default_specular;

    ASSERT(WITHIN(crumple, -1, MESH_NUM_CRUMPLES - 1));

    if (lpc == NULL) {
        NIGHT_get_colour(
            NIGHT_get_light_at(at_x, at_y, at_z),
            &default_colour,
            &default_specular);
    }

    if (prim == PRIM_OBJ_SPIKE) {
        // Stretch the spike prim sinusoidally based on world position and time.
        float angle;
        float stretch;

        angle = float(sdl3_get_ticks()) * 0.001F;
        angle += float(at_x) * 0.67f;
        angle += float(at_z) * 0.44f;

        stretch = sin(angle);
        stretch += 2.00F;

        matrix[3] *= stretch;
        matrix[4] *= stretch;
        matrix[5] *= stretch;
    } else if (prim == 58 || prim == 59 || prim == 34 || prim == 35) {
        // Scale these prims by a deterministic pseudo-random factor derived from position.
        ULONG stretch;

        stretch = at_x ^ at_z;
        stretch >>= 3;
        stretch &= 0xf;
        stretch += 0xf;

        float fstretch = float(stretch) * 0.03F + 0.5F;

        matrix[0] *= fstretch;
        matrix[1] *= fstretch;
        matrix[2] *= fstretch;

        matrix[3] *= fstretch;
        matrix[4] *= fstretch;
        matrix[5] *= fstretch;

        matrix[6] *= fstretch;
        matrix[7] *= fstretch;
        matrix[8] *= fstretch;
    } else if (kludge_shrink) {
        // Shrink prim 253 to 15% and all others to 70% for the MIB destruct effect.
        float fstretch;
        if (prim == 253)
            fstretch = 0.15F;
        else
            fstretch = 0.7F;

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

    POLY_set_local_rotation(
        float(at_x),
        float(at_y),
        float(at_z),
        matrix);

    p_obj = &prim_objects[prim];

    sp = p_obj->StartPoint;
    ep = p_obj->EndPoint;

    POLY_buffer_upto = 0;

    if (crumple >= 0) {
        for (i = sp; i < ep; i++) {
            ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

            pp = &POLY_buffer[POLY_buffer_upto++];

            POLY_transform_using_local_rotation(
                AENG_dx_prim_points[i].X + MESH_crumple[crumple][cv].dx,
                AENG_dx_prim_points[i].Y + MESH_crumple[crumple][cv].dy,
                AENG_dx_prim_points[i].Z + MESH_crumple[crumple][cv].dz,
                pp);

            cv += 1;
            cv &= MESH_NUM_CRUMPVALS - 1;

            if (lpc) {
                if (pp->MaybeValid()) {
                    NIGHT_get_colour(
                        *lpc,
                        &pp->colour,
                        &pp->specular);
                }

                lpc += 1;
            } else {
                pp->colour = default_colour;
                pp->specular = default_specular;

                // Balloon colour hack: tint the vertex colour by the balloon's assigned colour.
                // See SHAPE_draw_balloon() for where SHAPE_balloon_colour is set.
                if (prim == PRIM_OBJ_BALLOON) {
                    pp->colour &= SHAPE_balloon_colour;
                }
            }
        }
    } else {
        // Car zone crumple: each vertex belongs to a body zone; crumple level is per-zone.
        UBYTE* assign = car_assign;

        for (i = sp; i < ep; i++) {
            ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

            pp = &POLY_buffer[POLY_buffer_upto++];

            POLY_transform_using_local_rotation(
                AENG_dx_prim_points[i].X + (MESH_car_crumples[car_crumples[*assign]][cv][*assign].dx / 2.0f),
                AENG_dx_prim_points[i].Y + (MESH_car_crumples[car_crumples[*assign]][cv][*assign].dy / 2.0f),
                AENG_dx_prim_points[i].Z + (MESH_car_crumples[car_crumples[*assign]][cv][*assign].dz / 2.0f),
                pp);

            cv += 1;
            cv &= MESH_NUM_CRUMPVALS - 1;

            if (lpc) {
                if (pp->MaybeValid()) {
                    NIGHT_get_colour(
                        *lpc,
                        &pp->colour,
                        &pp->specular);
                }

                lpc += 1;
            } else {
                pp->colour = default_colour;
                pp->specular = default_specular;
            }

            assign++;
        }
    }

    // Submit quad faces.
    for (i = p_obj->StartFace4; i < p_obj->EndFace4; i++) {
        p_f4 = &prim_faces4[i];

        p0 = p_f4->Points[0] - sp;
        p1 = p_f4->Points[1] - sp;
        p2 = p_f4->Points[2] - sp;
        p3 = p_f4->Points[3] - sp;

        ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p3, 0, POLY_buffer_upto - 1));

        quad[0] = &POLY_buffer[p0];
        quad[1] = &POLY_buffer[p1];
        quad[2] = &POLY_buffer[p2];
        quad[3] = &POLY_buffer[p3];

        if (POLY_valid_quad(quad)) {
            if (p_f4->DrawFlags & POLY_FLAG_TEXTURED) {
                {
                    quad[0]->u = float(p_f4->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                    quad[0]->v = float(p_f4->UV[0][1]) * (1.0F / 32.0F);

                    quad[1]->u = float(p_f4->UV[1][0]) * (1.0F / 32.0F);
                    quad[1]->v = float(p_f4->UV[1][1]) * (1.0F / 32.0F);

                    quad[2]->u = float(p_f4->UV[2][0]) * (1.0F / 32.0F);
                    quad[2]->v = float(p_f4->UV[2][1]) * (1.0F / 32.0F);

                    quad[3]->u = float(p_f4->UV[3][0]) * (1.0F / 32.0F);
                    quad[3]->v = float(p_f4->UV[3][1]) * (1.0F / 32.0F);

                    page = p_f4->UV[0][0] & 0xc0;
                    page <<= 2;
                    page |= p_f4->TexturePage;
                    page += FACE_PAGE_OFFSET;

                    if (p_f4->FaceFlags & FACE_FLAG_TINT) {
                        qc0 = quad[0]->colour;
                        qc1 = quad[1]->colour;
                        qc2 = quad[2]->colour;
                        qc3 = quad[3]->colour;

                        quad[0]->colour &= MESH_colour_and;
                        quad[1]->colour &= MESH_colour_and;
                        quad[2]->colour &= MESH_colour_and;
                        quad[3]->colour &= MESH_colour_and;
                    }

                    if (p_f4->FaceFlags & FACE_FLAG_WALKABLE) {
                        // Debug: highlight walkable faces with animated colour.
                        quad[0]->colour = (GAME_TURN * 55);
                        quad[1]->colour = (GAME_TURN * 35);
                        quad[2]->colour = (GAME_TURN * 25);
                        quad[3]->colour = (GAME_TURN * 15);
                        POLY_add_quad(quad, POLY_PAGE_COLOUR, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
                    } else {
                        POLY_add_quad(quad, page, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
                    }

                    if (p_f4->FaceFlags & FACE_FLAG_TINT) {
                        quad[0]->colour = qc0;
                        quad[1]->colour = qc1;
                        quad[2]->colour = qc2;
                        quad[3]->colour = qc3;
                    }
                }
            } else {
                POLY_add_quad(quad, POLY_PAGE_COLOUR, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
            }
        }
    }

    // Submit triangle faces.
    for (i = p_obj->StartFace3; i < p_obj->EndFace3; i++) {
        p_f3 = &prim_faces3[i];

        p0 = p_f3->Points[0] - sp;
        p1 = p_f3->Points[1] - sp;
        p2 = p_f3->Points[2] - sp;

        ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));

        tri[0] = &POLY_buffer[p0];
        tri[1] = &POLY_buffer[p1];
        tri[2] = &POLY_buffer[p2];

        if (POLY_valid_triangle(tri)) {
            if (p_f3->DrawFlags & POLY_FLAG_TEXTURED) {
                tri[0]->u = float(p_f3->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                tri[0]->v = float(p_f3->UV[0][1]) * (1.0F / 32.0F);

                tri[1]->u = float(p_f3->UV[1][0]) * (1.0F / 32.0F);
                tri[1]->v = float(p_f3->UV[1][1]) * (1.0F / 32.0F);

                tri[2]->u = float(p_f3->UV[2][0]) * (1.0F / 32.0F);
                tri[2]->v = float(p_f3->UV[2][1]) * (1.0F / 32.0F);

                if (p_f3->FaceFlags & FACE_FLAG_TINT) {
                    qc0 = tri[0]->colour;
                    qc1 = tri[1]->colour;
                    qc2 = tri[2]->colour;

                    tri[0]->colour = PolyPoint2D::ModulateColours(qc0, MESH_colour_and);
                    tri[1]->colour = PolyPoint2D::ModulateColours(qc1, MESH_colour_and);
                    tri[2]->colour = PolyPoint2D::ModulateColours(qc2, MESH_colour_and);
                }

                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;
                page += FACE_PAGE_OFFSET;

                POLY_add_triangle(tri, page, !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED));

                if (p_f3->FaceFlags & FACE_FLAG_TINT) {
                    tri[0]->colour = qc0;
                    tri[1]->colour = qc1;
                    tri[2]->colour = qc2;
                }
            } else {
                POLY_add_triangle(tri, POLY_PAGE_COLOUR, !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED));
            }
        }
    }

    // Re-submit faces with environment mapping UV (sphere mapping from vertex normals).
    if (p_obj->flag & PRIM_FLAG_ENVMAPPED) {
        float nx;
        float ny;
        float nz;

        float comb[9];
        float cam_matrix[9];

        SLONG num_points = ep - sp;

        MATRIX_calc(cam_matrix, AENG_cam_yaw, AENG_cam_pitch, AENG_cam_roll);
        MATRIX_3x3mul(comb, cam_matrix, matrix);

        if (crumple != -1) {
            for (i = 0; i < num_points; i++) {
                nx = prim_normal[sp + i].X * (2.0F / 256.0F);
                ny = prim_normal[sp + i].Y * (2.0F / 256.0F);
                nz = prim_normal[sp + i].Z * (2.0F / 256.0F);

                MATRIX_MUL(
                    comb,
                    nx,
                    ny,
                    nz);

                POLY_buffer[i].u = (nx * 0.5F) + 0.5F;
                POLY_buffer[i].v = (ny * 0.5F) + 0.5F;

                POLY_buffer[i].colour |= 0xff000000;
            }
        } else {
            UBYTE* assign = car_assign;

            for (i = 0; i < num_points; i++) {
                nx = prim_normal[sp + i].X * (2.0F / 256.0F);
                ny = prim_normal[sp + i].Y * (2.0F / 256.0F);
                nz = prim_normal[sp + i].Z * (2.0F / 256.0F);

                nx -= float(MESH_car_crumples[car_crumples[*assign]][cv][*assign].dx) / 32;
                ny -= float(MESH_car_crumples[car_crumples[*assign]][cv][*assign].dy) / 32;
                nz -= float(MESH_car_crumples[car_crumples[*assign]][cv][*assign].dz) / 32;

                MATRIX_MUL(
                    comb,
                    nx,
                    ny,
                    nz);

                POLY_buffer[i].u = (nx * 0.5F) + 0.5F;
                POLY_buffer[i].v = (ny * 0.5F) + 0.5F;

                POLY_buffer[i].colour |= 0xff000000;
            }
        }

        // Submit env-mapped quads.
        for (i = p_obj->StartFace4; i < p_obj->EndFace4; i++) {
            p_f4 = &prim_faces4[i];

            if (p_f4->FaceFlags & FACE_FLAG_ENVMAP) {
                p0 = p_f4->Points[0] - sp;
                p1 = p_f4->Points[1] - sp;
                p2 = p_f4->Points[2] - sp;
                p3 = p_f4->Points[3] - sp;

                ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
                ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
                ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));
                ASSERT(WITHIN(p3, 0, POLY_buffer_upto - 1));

                quad[0] = &POLY_buffer[p0];
                quad[1] = &POLY_buffer[p1];
                quad[2] = &POLY_buffer[p2];
                quad[3] = &POLY_buffer[p3];

                if (POLY_valid_quad(quad)) {
                    POLY_add_quad(quad, POLY_PAGE_ENVMAP, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
                }
            }
        }

        // Submit env-mapped triangles.
        for (i = p_obj->StartFace3; i < p_obj->EndFace3; i++) {
            p_f3 = &prim_faces3[i];

            if (p_f3->FaceFlags & FACE_FLAG_ENVMAP) {
                p0 = p_f3->Points[0] - sp;
                p1 = p_f3->Points[1] - sp;
                p2 = p_f3->Points[2] - sp;

                ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
                ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
                ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));

                tri[0] = &POLY_buffer[p0];
                tri[1] = &POLY_buffer[p1];
                tri[2] = &POLY_buffer[p2];

                if (POLY_valid_triangle(tri)) {
                    POLY_add_triangle(tri, POLY_PAGE_ENVMAP, !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED));
                }
            }
        }
    }

    return lpc;
}

// uc_orig: MESH_draw_poly (fallen/DDEngine/Source/mesh.cpp)
NIGHT_Colour* MESH_draw_poly(
    SLONG prim,
    MAPCO16 at_x,
    MAPCO16 at_y,
    MAPCO16 at_z,
    SLONG i_yaw,
    SLONG i_pitch,
    SLONG i_roll,
    NIGHT_Colour* lpc,
    UBYTE fade,
    SLONG crumple)
{
    float matrix[9];
    float yaw;
    float pitch;
    float roll;
    ULONG alpha_bits = fade << 24;

    yaw = float(i_yaw) * (2.0F * PI / 2048.0F);
    pitch = float(i_pitch) * (2.0F * PI / 2048.0F);
    roll = float(i_roll) * (2.0F * PI / 2048.0F);

    MATRIX_calc(
        matrix,
        yaw,
        pitch,
        roll);

    MATRIX_TRANSPOSE(matrix);

    POLY_set_local_rotation(
        float(at_x),
        float(at_y),
        float(at_z),
        matrix);

    return MESH_draw_guts(prim, at_x, at_y, at_z, matrix, lpc, alpha_bits, crumple);
}

// uc_orig: MESH_draw_poly_inv_matrix (fallen/DDEngine/Source/mesh.cpp)
NIGHT_Colour* MESH_draw_poly_inv_matrix(
    SLONG prim,
    MAPCO16 at_x,
    MAPCO16 at_y,
    MAPCO16 at_z,
    SLONG i_yaw,
    SLONG i_pitch,
    SLONG i_roll,
    NIGHT_Colour* lpc)
{
    float matrix[9];
    float yaw;
    float pitch;
    float roll;

    yaw = float(i_yaw) * (2.0F * PI / 2048.0F);
    pitch = float(i_pitch) * (2.0F * PI / 2048.0F);
    roll = float(i_roll) * (2.0F * PI / 2048.0F);

    MATRIX_calc(
        matrix,
        yaw,
        pitch,
        roll);

    // Note: no MATRIX_TRANSPOSE here — used for helicopters where the flipped matrix is needed.

    POLY_set_local_rotation(
        float(at_x),
        float(at_y),
        float(at_z),
        matrix);

    return MESH_draw_guts(prim, at_x, at_y, at_z, matrix, lpc, 0);
}

