#include "engine/graphics/geometry/mesh.h"
#include "engine/graphics/geometry/mesh_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "engine/animation/morph.h"
#include "assets/texture.h"
#include "engine/core/matrix.h"
#include "engine/core/types.h"
#include "world/environment/prim_types.h" // PrimObject, PrimFace3/4, PRIM_OBJ_*, PRIM_FLAG_*, FACE_FLAG_*
#include "world/environment/prim.h"       // get_prim_info
#include "game/game_types.h"
#include "engine/graphics/geometry/shape_globals.h"
#include "world/level_pools.h"
#include "engine/core/memory.h"               // MemAlloc, MemFree (used by reflection mesh cache)
#include "assets/anim_tmap.h"

#include <math.h>
#include <stdlib.h>                    // realloc, fabs

// uc_orig: frand (fallen/DDEngine/Source/mesh.cpp)
// Returns a random float in [0.0, 1.0].
static inline float frand(void)
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
// then submits all quad and triangle faces. Called by MESH_draw_poly and MESH_draw_morph.
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
        NIGHT_get_d3d_colour(
            NIGHT_get_light_at(at_x, at_y, at_z),
            &default_colour,
            &default_specular);
    }

    if (prim == PRIM_OBJ_SPIKE) {
        // Stretch the spike prim sinusoidally based on world position and time.
        float angle;
        float stretch;

        angle = float(GetTickCount()) * 0.001F;
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
    POLY_shadow_upto = 0;

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
                    NIGHT_get_d3d_colour(
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
                    NIGHT_get_d3d_colour(
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
                ASSERT(0);

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

                    tri[0]->colour = PolyPoint2D::ModulateD3DColours(qc0, MESH_colour_and);
                    tri[1]->colour = PolyPoint2D::ModulateD3DColours(qc1, MESH_colour_and);
                    tri[2]->colour = PolyPoint2D::ModulateD3DColours(qc2, MESH_colour_and);
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

// uc_orig: MESH_draw_morph (fallen/DDEngine/Source/mesh.cpp)
// Draws a prim with keyframe vertex morphing. Vertices are linearly interpolated
// between morph frames morph1 and morph2 by blend factor tween (0..256 = 0.0..1.0).
// Face topology (UVs, texture pages) comes from the base prim geometry unchanged.
void MESH_draw_morph(
    SLONG prim,
    UBYTE morph1,
    UBYTE morph2,
    UWORD tween,
    MAPCO16 at_x,
    MAPCO16 at_y,
    MAPCO16 at_z,
    SLONG i_yaw,
    SLONG i_pitch,
    SLONG i_roll,
    NIGHT_Colour* lpc)
{
    SLONG i;
    SLONG sp;

    SLONG p0;
    SLONG p1;
    SLONG p2;
    SLONG p3;

    float px;
    float py;
    float pz;

    float px1, px2;
    float py1, py2;
    float pz1, pz2;

    float ptween;

    SLONG page;

    PrimFace4* p_f4;
    PrimFace3* p_f3;
    PrimObject* p_obj;

    MORPH_Point* mp1 = MORPH_get_points(morph1);
    MORPH_Point* mp2 = MORPH_get_points(morph2);

    SLONG num_points;

    POLY_Point* pp;

    POLY_Point* tri[3];
    POLY_Point* quad[4];

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

    MATRIX_TRANSPOSE(matrix);

    POLY_set_local_rotation(
        float(at_x),
        float(at_y),
        float(at_z),
        matrix);

    p_obj = &prim_objects[prim];

    sp = p_obj->StartPoint;

    POLY_buffer_upto = 0;
    POLY_shadow_upto = 0;

    ASSERT(
        MORPH_get_num_points(morph1) == MORPH_get_num_points(morph2));

    num_points = MORPH_get_num_points(morph1);
    ptween = float(tween) * (1.0F / 256.0F);

    // Transform all vertices into POLY_buffer, blending between morph frames.
    for (i = 0; i < num_points; i++) {
        ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

        pp = &POLY_buffer[POLY_buffer_upto++];

        px1 = float(mp1->x);
        py1 = float(mp1->y);
        pz1 = float(mp1->z);

        px2 = float(mp2->x);
        py2 = float(mp2->y);
        pz2 = float(mp2->z);

        // Linear interpolation between the two morph frames.
        px = px1 + (px2 - px1) * ptween;
        py = py1 + (py2 - py1) * ptween;
        pz = pz1 + (pz2 - pz1) * ptween;

        POLY_transform_using_local_rotation(px, py, pz, pp);

        if (lpc) {
            if (pp->MaybeValid()) {
                NIGHT_get_d3d_colour(
                    *lpc,
                    &pp->colour,
                    &pp->specular);
            }

            lpc += 1;
        } else {
            pp->colour = NIGHT_amb_d3d_colour;
            pp->specular = NIGHT_amb_d3d_specular;

            pp->colour &= ~POLY_colour_restrict;
            pp->specular &= ~POLY_colour_restrict;
        }

        mp1 += 1;
        mp2 += 1;
    }

    // Submit quads.
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
                if (p_f4->FaceFlags & FACE_FLAG_ANIMATE) {
                    struct AnimTmap* p_a;
                    SLONG cur;
                    p_a = &anim_tmaps[p_f4->TexturePage];
                    cur = p_a->Current;

                    quad[0]->u = float(p_a->UV[cur][0][0] & 0x3f) * (1.0F / 32.0F);
                    quad[0]->v = float(p_a->UV[cur][0][1]) * (1.0F / 32.0F);

                    quad[1]->u = float(p_a->UV[cur][1][0]) * (1.0F / 32.0F);
                    quad[1]->v = float(p_a->UV[cur][1][1]) * (1.0F / 32.0F);

                    quad[2]->u = float(p_a->UV[cur][2][0]) * (1.0F / 32.0F);
                    quad[2]->v = float(p_a->UV[cur][2][1]) * (1.0F / 32.0F);

                    quad[3]->u = float(p_a->UV[cur][3][0]) * (1.0F / 32.0F);
                    quad[3]->v = float(p_a->UV[cur][3][1]) * (1.0F / 32.0F);

                    page = p_a->UV[cur][0][0] & 0xc0;
                    page <<= 2;
                    page |= p_a->Page[cur];

                } else {

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
                }

                POLY_add_quad(quad, page, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
            } else {
                POLY_add_quad(quad, POLY_PAGE_COLOUR, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
            }
        }
    }

    // Submit triangles.
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

                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;
                page += FACE_PAGE_OFFSET;

                POLY_add_triangle(tri, page, !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED));
            } else {
                POLY_add_triangle(tri, POLY_PAGE_COLOUR, !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED));
            }
        }
    }
}

// uc_orig: MESH_MAX_DY (fallen/DDEngine/Source/mesh.cpp)
// Maximum vertical fade distance for reflections: points more than this many units
// above the water surface are fully transparent.
#define MESH_MAX_DY (128.0F)
// uc_orig: MESH_255_DIVIDED_BY_MAX_DY (fallen/DDEngine/Source/mesh.cpp)
#define MESH_255_DIVIDED_BY_MAX_DY (255.0F / MESH_MAX_DY)

// uc_orig: MESH_init_reflections (fallen/DDEngine/Source/mesh.cpp)
void MESH_init_reflections(void)
{
    SLONG i;

    MESH_Reflection* mr;

    for (i = 0; i < MESH_MAX_REFLECTIONS; i++) {
        mr = &MESH_reflection[i];

        if (mr->calculated) {
            mr->calculated = UC_FALSE;

            MemFree(mr->mp);
            MemFree(mr->mf);

            mr->num_points = 0;
            mr->num_faces = 0;

            mr->max_points = 0;
            mr->max_faces = 0;
        }
    }
}

// uc_orig: MESH_grow_points (fallen/DDEngine/Source/mesh.cpp)
// Doubles the capacity of the reflection point array.
static void MESH_grow_points(MESH_Reflection* mr)
{
    mr->max_points *= 2;
    mr->mp = (MESH_Point*)realloc(mr->mp, sizeof(MESH_Point) * mr->max_points);
}

// uc_orig: MESH_grow_faces (fallen/DDEngine/Source/mesh.cpp)
// Doubles the capacity of the reflection face array.
static void MESH_grow_faces(MESH_Reflection* mr)
{
    mr->max_faces *= 2;
    mr->mf = (MESH_Face*)realloc(mr->mf, sizeof(MESH_Face) * mr->max_faces);
}

// uc_orig: MESH_add_point (fallen/DDEngine/Source/mesh.cpp)
// Finds or creates a point at (x,y,z) with given fade depth in the reflection mesh.
// Returns the index of the point. Merges nearby points (dist < 0.1).
static SLONG MESH_add_point(
    MESH_Reflection* mr,
    float x,
    float y,
    float z,
    float fade)
{
    SLONG i;

    float dx;
    float dy;
    float dz;

    float dist;

    SLONG ans;

    for (i = mr->num_points - 1; i >= 0; i--) {
        dx = fabs(mr->mp[i].x - x);
        dy = fabs(mr->mp[i].y - y);
        dz = fabs(mr->mp[i].z - z);

        dist = dx + dy + dz;

        if (dist < 0.1F) {
            return i;
        }
    }

    if (mr->num_points >= mr->max_points) {
        MESH_grow_points(mr);
    }

    ans = mr->num_points;

    mr->mp[ans].x = x;
    mr->mp[ans].y = y;
    mr->mp[ans].z = z;
    mr->mp[ans].fade = MIN(255, (SLONG)fade);

    mr->num_points += 1;

    return ans;
}

// uc_orig: MESH_add_face (fallen/DDEngine/Source/mesh.cpp)
// Appends a triangle face to the reflection mesh.
static void MESH_add_face(
    MESH_Reflection* mr,
    SLONG p1,
    float u1,
    float v1,
    SLONG p2,
    float u2,
    float v2,
    SLONG p3,
    float u3,
    float v3,
    SLONG page)
{
    if (mr->num_faces >= mr->max_faces) {
        MESH_grow_faces(mr);
    }

    mr->mf[mr->num_faces].p[0] = p1;
    mr->mf[mr->num_faces].p[1] = p2;
    mr->mf[mr->num_faces].p[2] = p3;

    mr->mf[mr->num_faces].u[0] = u1;
    mr->mf[mr->num_faces].u[1] = u2;
    mr->mf[mr->num_faces].u[2] = u3;

    mr->mf[mr->num_faces].v[0] = v1;
    mr->mf[mr->num_faces].v[1] = v2;
    mr->mf[mr->num_faces].v[2] = v3;

    mr->mf[mr->num_faces].page = page;

    mr->num_faces += 1;
}

// uc_orig: MESH_NEXT_CLOCK (fallen/DDEngine/Source/mesh.cpp)
// uc_orig: MESH_NEXT_ANTI (fallen/DDEngine/Source/mesh.cpp)
// Advance clockwise or counter-clockwise through the MESH_add[] ring buffer.
#define MESH_NEXT_CLOCK(p) (((p) + 1 >= MESH_add_upto) ? 0 : (p) + 1)
#define MESH_NEXT_ANTI(p) (((p) - 1 >= 0) ? (p) - 1 : MESH_add_upto - 1)

// uc_orig: MESH_add_poly (fallen/DDEngine/Source/mesh.cpp)
// Tessellates a polygon into triangles and adds them to the reflection mesh.
// Subdivides polygon edges when dy > 8 units to avoid visible interpolation artifacts.
// Polygons entirely below the fade threshold (fade >= 255) are skipped.
static void MESH_add_poly(MESH_Reflection* mr, MESH_Add poly[], SLONG num_points, SLONG page)
{
    SLONG i;
    SLONG bot;
    SLONG top;
    SLONG p1;
    SLONG p2;
    SLONG last;
    SLONG count;

    float f;
    float tween;
    float top_height;
    float bot_height;
    float dx;
    float dy;
    float dz;
    float du;
    float dv;
    float dfade;
    float x;
    float y;
    float z;
    float u;
    float v;
    float fade;
    float mul;

    MESH_Add* ma;
    MESH_Add* mp1;
    MESH_Add* mp2;
    MESH_Add* mpl;

    ASSERT(num_points > 2);

    // Subdivide each edge into segments no taller than 8 units.
    MESH_add_upto = 0;

    for (i = 0; i < num_points; i++) {
        p1 = i + 0;
        p2 = i + 1;

        if (p2 >= num_points) {
            p2 = 0;
        }

        mp1 = &poly[p1];
        mp2 = &poly[p2];

        dx = mp2->x - mp1->x;
        dy = mp2->y - mp1->y;
        dz = mp2->z - mp1->z;
        du = mp2->u - mp1->u;
        dv = mp2->v - mp1->v;
        dfade = mp2->fade - mp1->fade;

        tween = fabs(dy * (1.0F / 8.0F));
        tween = floor(tween);

        if (tween < 1.0F) {
            tween = 1.0F;
        }

        for (f = 0.0F; f < tween; f += 1.0F) {
            mul = f * (1.0F / tween);

            x = mp1->x + mul * dx;
            y = mp1->y + mul * dy;
            z = mp1->z + mul * dz;
            u = mp1->u + mul * du;
            v = mp1->v + mul * dv;
            fade = mp1->fade + mul * dfade;

            ASSERT(WITHIN(MESH_add_upto, 0, MESH_MAX_ADD - 1));

            ma = &MESH_add[MESH_add_upto++];

            ma->x = x;
            ma->y = y;
            ma->z = z;
            ma->u = u;
            ma->v = v;
            ma->fade = fade;
            ma->index = UC_INFINITY;
        }
    }

    // Find the highest and lowest subdivided points.
    top = 0;
    bot = 0;

    top_height = MESH_add[0].y;
    bot_height = MESH_add[0].y;

    for (i = 1; i < MESH_add_upto; i++) {
        if (MESH_add[i].y > top_height) {
            top = i;
            top_height = MESH_add[i].y;
        }

        if (MESH_add[i].y < bot_height) {
            bot = i;
            bot_height = MESH_add[i].y;
        }
    }

    // Triangulate the polygon from top to bottom using a dual-pointer sweep.
    p1 = top;
    p2 = MESH_NEXT_ANTI(top);

    count = 0;

    while (1) {
        ASSERT(count++ < 2048);

        ASSERT(WITHIN(p1, 0, MESH_add_upto - 1));
        ASSERT(WITHIN(p2, 0, MESH_add_upto - 1));

        mp1 = &MESH_add[p1];
        mp2 = &MESH_add[p2];

        // Advance the highest of the two active pointers.
        if (p1 != bot && mp1->y >= mp2->y) {
            last = p1;
            p1 = MESH_NEXT_CLOCK(p1);
        } else {
            last = p2;
            p2 = MESH_NEXT_ANTI(p2);
        }

        ASSERT(WITHIN(p1, 0, MESH_add_upto - 1));
        ASSERT(WITHIN(p2, 0, MESH_add_upto - 1));
        ASSERT(WITHIN(last, 0, MESH_add_upto - 1));

        mp1 = &MESH_add[p1];
        mp2 = &MESH_add[p2];
        mpl = &MESH_add[last];

        // All three vertices fully faded — stop, no deeper face will be visible.
        if (mp1->fade >= 255.0F && mp2->fade >= 255.0F && mpl->fade >= 255.0F) {
            break;
        }

        // Assign indices to any new vertices.
        if (mp1->index == UC_INFINITY) {
            mp1->index = MESH_add_point(mr, mp1->x, mp1->y, mp1->z, mp1->fade);
        }
        if (mp2->index == UC_INFINITY) {
            mp2->index = MESH_add_point(mr, mp2->x, mp2->y, mp2->z, mp2->fade);
        }
        if (mpl->index == UC_INFINITY) {
            mpl->index = MESH_add_point(mr, mpl->x, mpl->y, mpl->z, mpl->fade);
        }

        MESH_add_face(
            mr,
            mpl->index,
            mpl->u,
            mpl->v,
            mp1->index,
            mp1->u,
            mp1->v,
            mp2->index,
            mp2->u,
            mp2->v,
            page);

        if (p1 == bot && p2 == bot) {
            break;
        }
    }
}

// uc_orig: MESH_create_reflection (fallen/DDEngine/Source/mesh.cpp)
// Builds the cached reflection mesh for the given prim. Called lazily by MESH_draw_reflection.
// The reflection is computed by mirroring each face about the minimum-y boundary (water surface)
// and fading vertices proportionally to their depth below that surface.
static void MESH_create_reflection(SLONG prim)
{
    SLONG i;
    SLONG j;
    SLONG pt;
    SLONG page;

    UBYTE order3[3] = { 0, 2, 1 };
    UBYTE order4[4] = { 0, 2, 3, 1 };

    float dy;
    float fade;
    float reflect_height;

    PrimInfo* pi;
    PrimObject* po;
    PrimFace3* f3;
    PrimFace4* f4;
    MESH_Reflection* mr;

    MESH_Add ma[4];

    ASSERT(WITHIN(prim, 0, MESH_MAX_REFLECTIONS - 1));

    mr = &MESH_reflection[prim];

    ASSERT(!mr->calculated);

    po = &prim_objects[prim];
    pi = get_prim_info(prim);

    mr->max_points = 32;
    mr->max_faces = 32;

    mr->num_points = 0;
    mr->num_faces = 0;

    mr->mp = (MESH_Point*)MemAlloc(mr->max_points * sizeof(MESH_Point));
    mr->mf = (MESH_Face*)MemAlloc(mr->max_faces * sizeof(MESH_Face));

    // Water surface = minimum Y of the prim bounding box.
    reflect_height = float(pi->miny);

    // Triangles.
    for (i = po->StartFace3; i < po->EndFace3; i++) {
        f3 = &prim_faces3[i];

        for (j = 0; j < 3; j++) {
            pt = order3[j];

            ma[j].x = AENG_dx_prim_points[f3->Points[pt]].X;
            ma[j].y = AENG_dx_prim_points[f3->Points[pt]].Y;
            ma[j].z = AENG_dx_prim_points[f3->Points[pt]].Z;
            ma[j].u = float(f3->UV[pt][0] & 0x3f) * (1.0F / 32.0F);
            ma[j].v = float(f3->UV[pt][1]) * (1.0F / 32.0F);
            dy = ma[j].y - reflect_height;
            fade = dy * MESH_255_DIVIDED_BY_MAX_DY;
            ma[j].fade = fade;
            ma[j].y = reflect_height - dy; // Mirror about water surface.

            ASSERT(ma[j].fade >= 0.0F);
        }

        if (ma[0].fade >= 255.0F && ma[1].fade >= 255.0F && ma[2].fade >= 255.0F) {
            // Whole poly faded out.
        } else {
            page = f3->UV[0][0] & 0xc0;
            page <<= 2;
            page |= f3->TexturePage;
            page += FACE_PAGE_OFFSET;

            MESH_add_poly(mr, ma, 3, page);
        }
    }

    // Quads.
    for (i = po->StartFace4; i < po->EndFace4; i++) {
        f4 = &prim_faces4[i];

        for (j = 0; j < 4; j++) {
            pt = order4[j];

            ma[j].x = AENG_dx_prim_points[f4->Points[pt]].X;
            ma[j].y = AENG_dx_prim_points[f4->Points[pt]].Y;
            ma[j].z = AENG_dx_prim_points[f4->Points[pt]].Z;
            ma[j].u = float(f4->UV[pt][0] & 0x3f) * (1.0F / 32.0F);
            ma[j].v = float(f4->UV[pt][1]) * (1.0F / 32.0F);
            dy = ma[j].y - reflect_height;
            fade = dy * MESH_255_DIVIDED_BY_MAX_DY;
            ma[j].fade = fade;
            ma[j].y = reflect_height - dy;

            ASSERT(ma[j].fade >= 0.0F);
        }

        if (ma[0].fade >= 255.0F && ma[1].fade >= 255.0F && ma[2].fade >= 255.0F && ma[3].fade >= 255.0F) {
            // Whole poly faded out.
        } else {
            page = f4->UV[0][0] & 0xc0;
            page <<= 2;
            page |= f4->TexturePage;
            page += FACE_PAGE_OFFSET;

            MESH_add_poly(mr, ma, 4, page);
        }
    }

    mr->calculated = UC_TRUE;
}

// uc_orig: MESH_draw_reflection (fallen/DDEngine/Source/mesh.cpp)
// Draws the water reflection of the given prim at position (at_x,at_y,at_z) with yaw rotation.
// Reflection mesh is built lazily on first call and cached until MESH_init_reflections() is called.
// Specular (fog) alpha of each reflected vertex is additionally faded by water depth.
void MESH_draw_reflection(
    SLONG prim,
    SLONG at_x,
    SLONG at_y,
    SLONG at_z,
    SLONG yaw,
    NIGHT_Colour* lpc)
{
    SLONG i;
    SLONG fog;

    float px;
    float py;
    float pz;

    float matrix[9];

    POLY_Point* pp;
    POLY_Point* tri[3];
    MESH_Face* mf;
    MESH_Reflection* mr;

    // Prim 83 is hardcoded to skip reflection — it caused issues in the original.
    if (prim == 83) {
        return;
    }

    ASSERT(WITHIN(prim, 0, MESH_MAX_REFLECTIONS - 1));

    mr = &MESH_reflection[prim];

    if (!mr->calculated) {
        MESH_create_reflection(prim);
    }

    // Rotation matrix — yaw only for reflections (no pitch/roll).
    MATRIX_calc(
        matrix,
        float(yaw) * (2.0F * PI / 2048.0F),
        0,
        0);

    MATRIX_TRANSPOSE(matrix);

    POLY_set_local_rotation(
        float(at_x),
        float(at_y),
        float(at_z),
        matrix);

    // Transform all reflection points into POLY_buffer.
    for (i = 0; i < mr->num_points; i++) {
        pp = &POLY_buffer[i];

        px = mr->mp[i].x;
        py = mr->mp[i].y;
        pz = mr->mp[i].z;

        POLY_transform_using_local_rotation_and_wibble(
            px,
            py,
            pz,
            pp,
            mr->mp[i].fade);

        if (lpc) {
            if (pp->MaybeValid()) {
                NIGHT_get_d3d_colour(
                    *lpc,
                    &pp->colour,
                    &pp->specular);
            }

            lpc += 1;
        } else {
            pp->colour = NIGHT_amb_d3d_colour;
            pp->specular = NIGHT_amb_d3d_specular;
        }

        if (pp->MaybeValid()) {
            // Fade out specular alpha by water depth (deeper = more transparent).
            fog = pp->specular >> 24;
            fog *= 255 - mr->mp[i].fade;
            fog >>= 8;

            pp->specular &= 0x00ffffff;
            pp->specular |= fog << 24;
        }
    }

    // Submit all reflection triangles.
    for (i = 0; i < mr->num_faces; i++) {
        mf = &mr->mf[i];

        ASSERT(WITHIN(mf->p[0], 0, mr->num_points - 1));
        ASSERT(WITHIN(mf->p[1], 0, mr->num_points - 1));
        ASSERT(WITHIN(mf->p[2], 0, mr->num_points - 1));

        tri[0] = &POLY_buffer[mf->p[0]];
        tri[1] = &POLY_buffer[mf->p[1]];
        tri[2] = &POLY_buffer[mf->p[2]];

        if (POLY_valid_triangle(tri)) {
            tri[0]->u = mf->u[0];
            tri[0]->v = mf->v[0];

            tri[1]->u = mf->u[1];
            tri[1]->v = mf->v[1];

            tri[2]->u = mf->u[2];
            tri[2]->v = mf->v[2];

            POLY_add_triangle(tri, mf->page, UC_TRUE);
        }
    }
}
