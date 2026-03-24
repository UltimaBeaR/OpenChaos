// Shadow map generator: projects a character's skeleton into a texture bitmap
// to use as a blob shadow. SMAP_person() builds the map; SMAP_project_onto_poly()
// maps it onto a ground quad in world space.

#include "engine/platform/uc_common.h"
#include <math.h>
#include "engine/lighting/smap.h"
#include "engine/lighting/smap_globals.h"
#include "engine/graphics/geometry/aa.h"
#include "engine/core/matrix.h"
#include "engine/core/fmatrix.h"
#include "world/level_pools.h"
#include "missions/game_types.h"
#include "engine/animation/anim_types.h"    // GameKeyFrame, GameKeyFrameElement, GameKeyFrameChunk, GetCMatrix
#include "things/characters/person_types.h" // Person struct, FLAG_PERSON_*, ANIM_TYPE_*
#include "things/characters/person.h"       // person_get_scale
#include "things/core/interact.h"           // calc_sub_objects_position_global

// SMAP_vector_normalise: normalise a 3D float vector in-place.
// uc_orig: SMAP_vector_normalise (fallen/DDEngine/Source/smap.cpp)
static void inline SMAP_vector_normalise(float* x, float* y, float* z)
{
    float len2 = *x * *x + *y * *y + *z * *z;
    float len = sqrt(len2);
    float lenr = 1.0F / len;

    *x *= lenr;
    *y *= lenr;
    *z *= lenr;
}

// uc_orig: SMAP_init (fallen/DDEngine/Source/smap.cpp)
// Initialises the shadow mapper for a new shadow: sets up the projection plane,
// clears the point list and the bitmap.
void SMAP_init(
    float light_dx,
    float light_dy,
    float light_dz,
    UBYTE* bitmap,
    UBYTE res_u,
    UBYTE res_v)
{
    SMAP_bitmap = bitmap;
    SMAP_res_u = res_u;
    SMAP_res_v = res_v;

    memset(bitmap, 0, res_u * res_v);

    SMAP_plane_nx = -light_dx;
    SMAP_plane_ny = -light_dy;
    SMAP_plane_nz = -light_dz;

    SMAP_vector_normalise(
        &SMAP_plane_nx,
        &SMAP_plane_ny,
        &SMAP_plane_nz);

    if (fabsf(light_dx) + fabsf(light_dz) < 0.001F) {
        SMAP_plane_ux = 1.0F;
        SMAP_plane_uy = 0.0F;
        SMAP_plane_uz = 0.0F;
    } else {
        SMAP_plane_ux = -light_dz;
        SMAP_plane_uy = 0;
        SMAP_plane_uz = light_dx;

        SMAP_vector_normalise(
            &SMAP_plane_ux,
            &SMAP_plane_uy,
            &SMAP_plane_uz);
    }

    // v-vector is the cross product of n and u.
    SMAP_plane_vx = SMAP_plane_ny * SMAP_plane_uz - SMAP_plane_nz * SMAP_plane_uy;
    SMAP_plane_vy = SMAP_plane_nz * SMAP_plane_ux - SMAP_plane_nx * SMAP_plane_uz;
    SMAP_plane_vz = SMAP_plane_nx * SMAP_plane_uy - SMAP_plane_ny * SMAP_plane_ux;

#define FINFINITY ((float)UC_INFINITY)

    SMAP_point_upto = 0;

    SMAP_u_min = +FINFINITY;
    SMAP_u_max = -FINFINITY;

    SMAP_v_min = +FINFINITY;
    SMAP_v_max = -FINFINITY;

    SMAP_n_min = +FINFINITY;
    SMAP_n_max = -FINFINITY;
}

// uc_orig: SMAP_point_add (fallen/DDEngine/Source/smap.cpp)
// Projects a world-space point onto the shadow plane and stores it. Returns its index.
SLONG SMAP_point_add(
    float world_x,
    float world_y,
    float world_z)
{
    SMAP_Point* sp;

    ASSERT(WITHIN(SMAP_point_upto, 0, SMAP_MAX_POINTS - 1));

    sp = &SMAP_point[SMAP_point_upto];

    sp->world_x = world_x;
    sp->world_y = world_y;
    sp->world_z = world_z;

    sp->along_n = world_x * SMAP_plane_nx + world_y * SMAP_plane_ny + world_z * SMAP_plane_nz;
    sp->along_u = world_x * SMAP_plane_ux + world_y * SMAP_plane_uy + world_z * SMAP_plane_uz;
    sp->along_v = world_x * SMAP_plane_vx + world_y * SMAP_plane_vy + world_z * SMAP_plane_vz;

    if (sp->along_n < SMAP_n_min) {
        SMAP_n_min = sp->along_n;
    }
    if (sp->along_n > SMAP_n_max) {
        SMAP_n_max = sp->along_n;
    }

    if (sp->along_u < SMAP_u_min) {
        SMAP_u_min = sp->along_u;
    }
    if (sp->along_u > SMAP_u_max) {
        SMAP_u_max = sp->along_u;
    }

    if (sp->along_v < SMAP_v_min) {
        SMAP_v_min = sp->along_v;
    }
    if (sp->along_v > SMAP_v_max) {
        SMAP_v_max = sp->along_v;
    }

    return SMAP_point_upto++;
}

// uc_orig: SMAP_point_finished (fallen/DDEngine/Source/smap.cpp)
// Call after all points have been added. Computes UV scale/offset for bitmap mapping
// and converts all stored points to fixed-point bitmap coordinates.
void SMAP_point_finished()
{
    SLONG i;

    SMAP_Point* sp;

    float along_bu;
    float along_bv;

    float res_u = float(SMAP_res_u);
    float res_v = float(SMAP_res_v);

    // Push 1 pixel in from the bitmap edge.
    SMAP_u_map_mul_float = (res_u - 2.0F) / (SMAP_u_max - SMAP_u_min);
    SMAP_v_map_mul_float = (res_v - 2.0F) / (SMAP_v_max - SMAP_v_min);

    SMAP_u_map_mul_slong = SMAP_u_map_mul_float * 65536.0F;
    SMAP_v_map_mul_slong = SMAP_v_map_mul_float * 65536.0F;

    SMAP_u_map_add_float = 1.0F / res_u;
    SMAP_v_map_add_float = 1.0F / res_v;

    SMAP_u_map_mul_float *= SMAP_u_map_add_float;
    SMAP_v_map_mul_float *= SMAP_v_map_add_float;

    for (i = 0; i < SMAP_point_upto; i++) {
        sp = &SMAP_point[i];

        along_bu = 65536.0F + (sp->along_u - SMAP_u_min) * SMAP_u_map_mul_slong;
        along_bv = 65536.0F + (sp->along_v - SMAP_v_min) * SMAP_v_map_mul_slong;

        sp->u = SLONG(along_bu);
        sp->v = SLONG(along_bv);

        ASSERT(WITHIN(sp->u, 0, SMAP_res_u << 16));
        ASSERT(WITHIN(sp->v, 0, SMAP_res_v << 16));
    }
}

// uc_orig: SMAP_tri_add (fallen/DDEngine/Source/smap.cpp)
// Draws one triangle into the shadow bitmap using anti-aliased rasterisation.
// Points are specified by index into the SMAP_point[] array.
static void SMAP_tri_add(
    SLONG p1,
    SLONG p2,
    SLONG p3)
{
    SLONG du1;
    SLONG dv1;

    SLONG du2;
    SLONG dv2;

    ASSERT(WITHIN(p1, 0, SMAP_point_upto - 1));
    ASSERT(WITHIN(p2, 0, SMAP_point_upto - 1));
    ASSERT(WITHIN(p3, 0, SMAP_point_upto - 1));

    du1 = SMAP_point[p2].u - SMAP_point[p1].u;
    dv1 = SMAP_point[p2].v - SMAP_point[p1].v;

    du2 = SMAP_point[p3].u - SMAP_point[p1].u;
    dv2 = SMAP_point[p3].v - SMAP_point[p1].v;

    if (MUL64(du1, dv2) - MUL64(dv1, du2) <= 0) {
        // Backface culled — skip.
    } else {
        AA_draw(
            SMAP_bitmap,
            SMAP_res_u,
            SMAP_res_v,
            SMAP_res_v,
            SMAP_point[p1].u,
            SMAP_point[p1].v,
            SMAP_point[p2].u,
            SMAP_point[p2].v,
            SMAP_point[p3].u,
            SMAP_point[p3].v);
    }
}

// uc_orig: SMAP_add_prim_triangles (fallen/DDEngine/Source/smap.cpp)
// Submits all faces of a prim object to the shadow rasteriser.
static void SMAP_add_prim_triangles(
    SLONG prim,
    SLONG index)
{
    PrimFace4* p_f4;
    PrimFace3* p_f3;
    PrimObject* p_obj;

    p_obj = &prim_objects[prim];

    SLONG i;

    index -= p_obj->StartPoint;

    for (i = p_obj->StartFace3; i < p_obj->EndFace3; i++) {
        p_f3 = &prim_faces3[i];

        SMAP_tri_add(
            p_f3->Points[0] + index,
            p_f3->Points[1] + index,
            p_f3->Points[2] + index);
    }

    for (i = p_obj->StartFace4; i < p_obj->EndFace4; i++) {
        p_f4 = &prim_faces4[i];

        SMAP_tri_add(
            p_f4->Points[0] + index,
            p_f4->Points[1] + index,
            p_f4->Points[2] + index);

        SMAP_tri_add(
            p_f4->Points[1] + index,
            p_f4->Points[3] + index,
            p_f4->Points[2] + index);
    }
}


// uc_orig: SMAP_add_tweened_points (fallen/DDEngine/Source/smap.cpp)
// Adds all shadow-map points for one body part at a tweened position.
// Returns the index of the last point added.
static UWORD SMAP_add_tweened_points(
    SLONG prim,
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG tween,
    struct GameKeyFrameElement* anim_info,
    struct GameKeyFrameElement* anim_info_next,
    struct Matrix33* rot_mat,
    SLONG off_dx,
    SLONG off_dy,
    SLONG off_dz,
    Thing* p_thing)
{
    SLONG i;

    SLONG sp;
    SLONG ep;

    Matrix31 offset;
    Matrix33 mat2;
    Matrix33 mat_final;

    UWORD ans;

    SVector temp;

    PrimObject* p_obj;

    void matrix_transform(Matrix31 * result, Matrix33 * trans, Matrix31 * mat2);
    void matrix_transformZMY(Matrix31 * result, Matrix33 * trans, Matrix31 * mat2);
    void matrix_mult33(Matrix33 * result, Matrix33 * mat1, Matrix33 * mat2);

    offset.M[0] = anim_info->OffsetX + ((anim_info_next->OffsetX + off_dx - anim_info->OffsetX) * tween >> 8);
    offset.M[1] = anim_info->OffsetY + ((anim_info_next->OffsetY + off_dy - anim_info->OffsetY) * tween >> 8);
    offset.M[2] = anim_info->OffsetZ + ((anim_info_next->OffsetZ + off_dz - anim_info->OffsetZ) * tween >> 8);

    matrix_transformZMY((struct Matrix31*)&temp, rot_mat, &offset);

    SLONG character_scale = person_get_scale(p_thing);

    temp.X = (temp.X * character_scale) / 256;
    temp.Y = (temp.Y * character_scale) / 256;
    temp.Z = (temp.Z * character_scale) / 256;

    x += temp.X;
    y += temp.Y;
    z += temp.Z;

    CMatrix33 m1, m2;
    GetCMatrix(anim_info, &m1);
    GetCMatrix(anim_info_next, &m2);

    build_tween_matrix(&mat2, &m1, &m2, tween);

    normalise_matrix(&mat2);

    mat2.M[0][0] = (mat2.M[0][0] * character_scale) / 256;
    mat2.M[0][1] = (mat2.M[0][1] * character_scale) / 256;
    mat2.M[0][2] = (mat2.M[0][2] * character_scale) / 256;
    mat2.M[1][0] = (mat2.M[1][0] * character_scale) / 256;
    mat2.M[1][1] = (mat2.M[1][1] * character_scale) / 256;
    mat2.M[1][2] = (mat2.M[1][2] * character_scale) / 256;
    mat2.M[2][0] = (mat2.M[2][0] * character_scale) / 256;
    mat2.M[2][1] = (mat2.M[2][1] * character_scale) / 256;
    mat2.M[2][2] = (mat2.M[2][2] * character_scale) / 256;

    matrix_mult33(&mat_final, rot_mat, &mat2);

    p_obj = &prim_objects[prim];

    sp = p_obj->StartPoint;
    ep = p_obj->EndPoint;

    for (i = sp; i < ep; i++) {
        matrix_transform_small((struct Matrix31*)&temp, &mat_final, (struct SMatrix31*)&prim_points[i]);

        temp.X += x;
        temp.Y += y;
        temp.Z += z;

        ans = SMAP_point_add(
            float(temp.X),
            float(temp.Y),
            float(temp.Z));
    }

    return ans;
}

// uc_orig: SMAP_person (fallen/DDEngine/Source/smap.cpp)
void SMAP_person(
    Thing* p_thing,
    UBYTE* bitmap,
    UBYTE u_res,
    UBYTE v_res,
    SLONG light_dx,
    SLONG light_dy,
    SLONG light_dz)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    Matrix33 r_matrix;

    GameKeyFrameElement* ae1;
    GameKeyFrameElement* ae2;

    DrawTween* dt = p_thing->Draw.Tweened;

    if (dt->CurrentFrame == 0 || dt->NextFrame == 0) {
        MSG_add("!!!!!!!!!!!!!!!!!!!!!!!!ERROR SMAP_person");
        return;
    }

    if (dt->Locked) {
        SLONG x1, y1, z1;
        SLONG x2, y2, z2;

        calc_sub_objects_position_global(dt->CurrentFrame, dt->NextFrame, 0, dt->Locked, &x1, &y1, &z1);
        calc_sub_objects_position_global(dt->CurrentFrame, dt->NextFrame, 256, dt->Locked, &x2, &y2, &z2);

        dx = x1 - x2;
        dy = y1 - y2;
        dz = z1 - z2;
    } else {
        dx = 0;
        dy = 0;
        dz = 0;
    }

    ae1 = dt->CurrentFrame->FirstElement;
    ae2 = dt->NextFrame->FirstElement;

    if (!ae1 || !ae2) {
        MSG_add("!!!!!!!!!!!!!!!!!!!ERROR SMAP_person has no animation elements");

        return;
    }

    void FIGURE_rotate_obj(
        SLONG xangle,
        SLONG yangle,
        SLONG zangle,
        Matrix33 * r3);

    FIGURE_rotate_obj(
        dt->Tilt,
        dt->Angle,
        dt->Roll,
        &r_matrix);

    SMAP_init(
        float(light_dx),
        float(light_dy),
        float(light_dz),
        bitmap,
        u_res,
        v_res);

    SLONG i;
    SLONG ele_count;
    SLONG start_object;
    SLONG object_offset;

    ele_count = dt->TheChunk->ElementCount;
    start_object = prim_multi_objects[dt->TheChunk->MultiObject[0]].StartObject;

// uc_orig: SMAP_MAX_PARTS (fallen/DDEngine/Source/smap.cpp)
#define SMAP_MAX_PARTS 20

    SLONG indices[SMAP_MAX_PARTS];

    for (i = 0; i < ele_count; i++) {
        ASSERT(WITHIN(i, 0, SMAP_MAX_PARTS - 1));

        object_offset = dt->TheChunk->PeopleTypes[dt->PersonID & 0x1f].BodyPart[i];

        indices[i] = SMAP_add_tweened_points(
            start_object + object_offset,
            p_thing->WorldPos.X >> 8,
            p_thing->WorldPos.Y >> 8,
            p_thing->WorldPos.Z >> 8,
            dt->AnimTween,
            &ae1[i],
            &ae2[i],
            &r_matrix,
            dx, dy, dz, p_thing);
    }

    SMAP_point_finished();

    SLONG index = 0;

    for (i = 0; i < ele_count; i++) {
        ASSERT(WITHIN(i, 0, SMAP_MAX_PARTS - 1));

        object_offset = dt->TheChunk->PeopleTypes[dt->PersonID & 0x1f].BodyPart[i];

        SMAP_add_prim_triangles(
            start_object + object_offset,
            index);

        index = indices[i] + 1;
    }
}

// Clip-flag bits used by SMAP_project_onto_poly.
// uc_orig: SMAP_CLIP_U_LESS (fallen/DDEngine/Source/smap.cpp)
#define SMAP_CLIP_U_LESS (1 << 0)
// uc_orig: SMAP_CLIP_U_MORE (fallen/DDEngine/Source/smap.cpp)
#define SMAP_CLIP_U_MORE (1 << 1)
// uc_orig: SMAP_CLIP_V_LESS (fallen/DDEngine/Source/smap.cpp)
#define SMAP_CLIP_V_LESS (1 << 2)
// uc_orig: SMAP_CLIP_V_MORE (fallen/DDEngine/Source/smap.cpp)
#define SMAP_CLIP_V_MORE (1 << 3)

// uc_orig: SMAP_wrong_side (fallen/DDEngine/Source/smap.cpp)
// Returns UC_TRUE if the polygon is on the "bright" side of the shadow map (nearer to the light).
static SLONG SMAP_wrong_side(SMAP_Link* sl)
{
    float order = 0.0F;
    float overorder;

    float av_wx = 0.0F;
    float av_wy = 0.0F;
    float av_wz = 0.0F;

    float av_n;

    while (sl) {
        av_wx += sl->wx;
        av_wy += sl->wy;
        av_wz += sl->wz;

        order += 1.0F;

        sl = sl->next;
    }

    overorder = 1.0F / order;

    av_wx *= overorder;
    av_wy *= overorder;
    av_wz *= overorder;

    av_n = av_wx * SMAP_plane_nx + av_wy * SMAP_plane_ny + av_wz * SMAP_plane_nz;

    if (av_n >= SMAP_n_min + (SMAP_n_max - SMAP_n_min) * 0.75F) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: SMAP_convert_uvs (fallen/DDEngine/Source/smap.cpp)
// Converts raw u,v values in the linked list from world units to texture [0,1] UVs.
static void SMAP_convert_uvs(SMAP_Link* sl)
{
    while (sl) {
        sl->u -= SMAP_u_min;
        sl->v -= SMAP_v_min;

        sl->u *= SMAP_u_map_mul_float;
        sl->v *= SMAP_v_map_mul_float;

        sl->u += SMAP_u_map_add_float;
        sl->v += SMAP_v_map_add_float;

        sl = sl->next;
    }
}

// uc_orig: SMAP_project_onto_poly (fallen/DDEngine/Source/smap.cpp)
SMAP_Link* SMAP_project_onto_poly(SVector_F poly[], SLONG num_points)
{
    SLONG i;
    ULONG clip_and;
    ULONG clip_or;

    float along;

    SMAP_Link* poly_links;
    SMAP_Link* sl;
    SMAP_Link* sl1;
    SMAP_Link* sl2;
    SMAP_Link* sc;

    SMAP_Link** prev;
    SMAP_Link* next;

    SMAP_link_upto = 0;

    clip_or = 0;
    clip_and = 0xffffffff;

    for (i = 0; i < num_points; i++) {
        sl = &SMAP_link[SMAP_link_upto++];

        sl->wx = poly[i].X;
        sl->wy = poly[i].Y;
        sl->wz = poly[i].Z;

        sl->u = sl->wx * SMAP_plane_ux + sl->wy * SMAP_plane_uy + sl->wz * SMAP_plane_uz;
        sl->v = sl->wx * SMAP_plane_vx + sl->wy * SMAP_plane_vy + sl->wz * SMAP_plane_vz;

        sl->next = &SMAP_link[SMAP_link_upto];

        if (SMAP_link_upto == num_points) {
            sl->next = NULL;
        }

        sl->clip = 0;

        if (sl->u < SMAP_u_min) {
            sl->clip |= SMAP_CLIP_U_LESS;
        }
        if (sl->u > SMAP_u_max) {
            sl->clip |= SMAP_CLIP_U_MORE;
        }
        if (sl->v < SMAP_v_min) {
            sl->clip |= SMAP_CLIP_V_LESS;
        }
        if (sl->v > SMAP_v_max) {
            sl->clip |= SMAP_CLIP_V_MORE;
        }

        clip_or |= sl->clip;
        clip_and &= sl->clip;
    }

    if (clip_and) {
        // No part of the shadow map falls on this polygon.
        return NULL;
    }

    // Backface cull.
    {
        float vec1u;
        float vec1v;
        float vec2u;
        float vec2v;

        float cross;

        SMAP_Link* p1 = &SMAP_link[0];
        SMAP_Link* p2 = &SMAP_link[1];
        SMAP_Link* p3 = &SMAP_link[2];

        vec1u = p2->u - p1->u;
        vec1v = p2->v - p1->v;
        vec2u = p3->u - p1->u;
        vec2v = p3->v - p1->v;

        cross = vec1u * vec2v - vec1v * vec2u;

        if (cross >= 0) {
            return NULL;
        }
    }

    poly_links = &SMAP_link[0];

    if (clip_or == 0) {
        if (SMAP_wrong_side(poly_links)) {
            return NULL;
        } else {
            SMAP_convert_uvs(poly_links);

            return poly_links;
        }
    }

    // =========  CLIP TO SMAP_u_min  =========

    for (sl = poly_links; sl; sl = sl->next) {
        sl1 = sl;
        sl2 = sl->next;

        if (sl2 == NULL) {
            sl2 = poly_links;
        }

        if ((sl1->clip ^ sl2->clip) & SMAP_CLIP_U_LESS) {
            along = (SMAP_u_min - sl1->u) / (sl2->u - sl1->u);

            ASSERT(WITHIN(SMAP_link_upto, 0, SMAP_MAX_LINKS - 1));

            sc = &SMAP_link[SMAP_link_upto++];

            sc->wx = sl1->wx + along * (sl2->wx - sl1->wx);
            sc->wy = sl1->wy + along * (sl2->wy - sl1->wy);
            sc->wz = sl1->wz + along * (sl2->wz - sl1->wz);

            sc->u = SMAP_u_min;
            sc->v = sl1->v + along * (sl2->v - sl1->v);

            sc->clip = 0;

            if (sc->u < SMAP_u_min) {
                sc->clip |= SMAP_CLIP_U_LESS;
            }
            if (sc->u > SMAP_u_max) {
                sc->clip |= SMAP_CLIP_U_MORE;
            }
            if (sc->v < SMAP_v_min) {
                sc->clip |= SMAP_CLIP_V_LESS;
            }
            if (sc->v > SMAP_v_max) {
                sc->clip |= SMAP_CLIP_V_MORE;
            }

            sc->next = sl->next;
            sl->next = sc;

            sl = sl->next;
        }
    }

    // Remove all points outside the SMAP_u_min boundary.

    prev = &poly_links;
    next = poly_links;

    clip_and = 0xffffffff;
    clip_or = 0;

    while (1) {
        if (next == NULL) {
            break;
        }

        if (next->clip & SMAP_CLIP_U_LESS) {
            *prev = next->next;
            next = next->next;
        } else {
            clip_and &= next->clip;
            clip_or |= next->clip;

            prev = &next->next;
            next = next->next;
        }
    }

    if (clip_and) {
        return NULL;
    }
    if (!clip_or) {
        if (SMAP_wrong_side(poly_links)) {
            return NULL;
        } else {
            SMAP_convert_uvs(poly_links);
            return poly_links;
        }
    }

    // =========  CLIP TO SMAP_u_max  =========

    for (sl = poly_links; sl; sl = sl->next) {
        sl1 = sl;
        sl2 = sl->next;

        if (sl2 == NULL) {
            sl2 = poly_links;
        }

        if ((sl1->clip ^ sl2->clip) & SMAP_CLIP_U_MORE) {
            along = (SMAP_u_max - sl1->u) / (sl2->u - sl1->u);

            ASSERT(WITHIN(SMAP_link_upto, 0, SMAP_MAX_LINKS - 1));

            sc = &SMAP_link[SMAP_link_upto++];

            sc->wx = sl1->wx + along * (sl2->wx - sl1->wx);
            sc->wy = sl1->wy + along * (sl2->wy - sl1->wy);
            sc->wz = sl1->wz + along * (sl2->wz - sl1->wz);

            sc->u = SMAP_u_max;
            sc->v = sl1->v + along * (sl2->v - sl1->v);

            sc->clip = 0;

            if (sc->u < SMAP_u_min) {
                sc->clip |= SMAP_CLIP_U_LESS;
            }
            if (sc->u > SMAP_u_max) {
                sc->clip |= SMAP_CLIP_U_MORE;
            }
            if (sc->v < SMAP_v_min) {
                sc->clip |= SMAP_CLIP_V_LESS;
            }
            if (sc->v > SMAP_v_max) {
                sc->clip |= SMAP_CLIP_V_MORE;
            }

            sc->next = sl->next;
            sl->next = sc;

            sl = sl->next;
        }
    }

    // Remove all points outside the SMAP_u_max boundary.

    prev = &poly_links;
    next = poly_links;

    clip_and = 0xffffffff;
    clip_or = 0;

    while (1) {
        if (next == NULL) {
            break;
        }

        if (next->clip & SMAP_CLIP_U_MORE) {
            *prev = next->next;
            next = next->next;
        } else {
            clip_and &= next->clip;
            clip_or |= next->clip;

            prev = &next->next;
            next = next->next;
        }
    }

    if (clip_and) {
        return NULL;
    }
    if (!clip_or) {
        if (SMAP_wrong_side(poly_links)) {
            return NULL;
        } else {
            SMAP_convert_uvs(poly_links);
            return poly_links;
        }
    }

    // =========  CLIP TO SMAP_v_min  =========

    for (sl = poly_links; sl; sl = sl->next) {
        sl1 = sl;
        sl2 = sl->next;

        if (sl2 == NULL) {
            sl2 = poly_links;
        }

        if ((sl1->clip ^ sl2->clip) & SMAP_CLIP_V_LESS) {
            along = (SMAP_v_min - sl1->v) / (sl2->v - sl1->v);

            ASSERT(WITHIN(SMAP_link_upto, 0, SMAP_MAX_LINKS - 1));

            sc = &SMAP_link[SMAP_link_upto++];

            sc->wx = sl1->wx + along * (sl2->wx - sl1->wx);
            sc->wy = sl1->wy + along * (sl2->wy - sl1->wy);
            sc->wz = sl1->wz + along * (sl2->wz - sl1->wz);

            sc->u = sl1->u + along * (sl2->u - sl1->u);
            sc->v = SMAP_v_min;

            sc->clip = 0;

            if (sc->u < SMAP_u_min) {
                sc->clip |= SMAP_CLIP_U_LESS;
            }
            if (sc->u > SMAP_u_max) {
                sc->clip |= SMAP_CLIP_U_MORE;
            }
            if (sc->v < SMAP_v_min) {
                sc->clip |= SMAP_CLIP_V_LESS;
            }
            if (sc->v > SMAP_v_max) {
                sc->clip |= SMAP_CLIP_V_MORE;
            }

            sc->next = sl->next;
            sl->next = sc;

            sl = sl->next;
        }
    }

    // Remove all points outside the SMAP_v_min boundary.

    prev = &poly_links;
    next = poly_links;

    clip_and = 0xffffffff;
    clip_or = 0;

    while (1) {
        if (next == NULL) {
            break;
        }

        if (next->clip & SMAP_CLIP_V_LESS) {
            *prev = next->next;
            next = next->next;
        } else {
            clip_and &= next->clip;
            clip_or |= next->clip;

            prev = &next->next;
            next = next->next;
        }
    }

    if (clip_and) {
        return NULL;
    }
    if (!clip_or) {
        if (SMAP_wrong_side(poly_links)) {
            return NULL;
        } else {
            SMAP_convert_uvs(poly_links);
            return poly_links;
        }
    }

    // =========  CLIP TO SMAP_v_max  =========

    for (sl = poly_links; sl; sl = sl->next) {
        sl1 = sl;
        sl2 = sl->next;

        if (sl2 == NULL) {
            sl2 = poly_links;
        }

        if ((sl1->clip ^ sl2->clip) & SMAP_CLIP_V_MORE) {
            along = (SMAP_v_max - sl1->v) / (sl2->v - sl1->v);

            ASSERT(WITHIN(SMAP_link_upto, 0, SMAP_MAX_LINKS - 1));

            sc = &SMAP_link[SMAP_link_upto++];

            sc->wx = sl1->wx + along * (sl2->wx - sl1->wx);
            sc->wy = sl1->wy + along * (sl2->wy - sl1->wy);
            sc->wz = sl1->wz + along * (sl2->wz - sl1->wz);

            sc->u = sl1->u + along * (sl2->u - sl1->u);
            sc->v = SMAP_v_max;

            sc->clip = 0;

            if (sc->u < SMAP_u_min) {
                sc->clip |= SMAP_CLIP_U_LESS;
            }
            if (sc->u > SMAP_u_max) {
                sc->clip |= SMAP_CLIP_U_MORE;
            }
            if (sc->v < SMAP_v_min) {
                sc->clip |= SMAP_CLIP_V_LESS;
            }
            if (sc->v > SMAP_v_max) {
                sc->clip |= SMAP_CLIP_V_MORE;
            }

            sc->next = sl->next;
            sl->next = sc;

            sl = sl->next;
        }
    }

    // Remove all points outside the SMAP_v_max boundary.

    prev = &poly_links;
    next = poly_links;

    clip_and = 0xffffffff;
    clip_or = 0;

    while (1) {
        if (next == NULL) {
            break;
        }

        if (next->clip & SMAP_CLIP_V_MORE) {
            *prev = next->next;
            next = next->next;
        } else {
            clip_and &= next->clip;
            clip_or |= next->clip;

            prev = &next->next;
            next = next->next;
        }
    }

    if (clip_and) {
        return NULL;
    }
    if (!clip_or) {
        if (SMAP_wrong_side(poly_links)) {
            return NULL;
        } else {
            SMAP_convert_uvs(poly_links);
            return poly_links;
        }
    }

    // Should never reach here — all 4 planes clipped and still not done.
    ASSERT(0);

    return NULL;
}

// SMAP_bike is declared in smap.h for ABI compatibility but has no implementation.
// uc_orig: SMAP_bike (fallen/DDEngine/Headers/smap.h)
void SMAP_bike(
    Thing* person,
    UBYTE* bitmap,
    UBYTE u_res,
    UBYTE v_res,
    SLONG light_dx,
    SLONG light_dy,
    SLONG light_dz)
{
    // Not implemented in the original (pre-release) codebase.
}
