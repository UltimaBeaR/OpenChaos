// Character shadow generator (Milestone 1E, GPU). SMAP_person_gpu()
// renders the skinned silhouette into the shared shadow texture on the
// GPU; SMAP_project_onto_poly() maps it onto ground quads in world space.
// The legacy CPU software rasteriser (SMAP_person + AA_draw) was removed.

#include "engine/platform/uc_common.h"
#include <math.h>
#include "engine/graphics/lighting/smap.h"
#include "engine/graphics/lighting/smap_globals.h"
#include "engine/core/matrix.h"
#include "engine/core/fmatrix.h"
#include "map/level_pools.h"
#include "game/game_types.h"
#include "engine/animation/anim_types.h" // GameKeyFrameChunk (used in SMAP_person_gpu)
#include "engine/graphics/render_interp.h" // render_interp_get_cached_pose, BoneInterpTransform
#include "engine/graphics/geometry/pose_composer.h" // POSE_MAX_BONES
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // GESkinMesh, ge_shadow_silhouette_*
#include "engine/graphics/geometry/figure.h" // P2-H: FIGURE_get_skin_mesh_for_thing

// SMAP_vector_normalise: normalise a 3D float vector in-place.
// uc_orig: SMAP_vector_normalise (fallen/DDEngine/Source/smap.cpp)
static void SMAP_vector_normalise(float* x, float* y, float* z)
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

// ----------------------------------------------------------------------
// GPU shadow-silhouette path — P2-H.
//
// SMAP_person_gpu renders the SAME consolidated bind-space character
// mesh the body draw uses (FIGURE_get_skin_mesh_for_thing) with the
// SAME per-frame skin palette, but projected to an orthographic
// shadow-clip instead of the camera. One draw per character (used to be
// N per body part). Soft-skin weights for the body apply to the shadow
// automatically because they share the VBO.
//
// The projection box is a tight shadow-plane fit derived from per-bone
// bind-space AABBs (stored on the consolidated TomsPrimObject by
// figure_build_consolidated_skin_world): each bone's 8 AABB corners are
// transformed by skin[bone] (= current × inv_bind) and projected onto
// the sun plane. This bounds the body almost exactly so the silhouette
// fills the texture (crisp) and stays near the feet, which makes the
// original 64/256 distance fade behave like the original (soft far edge,
// no global "breathing").
// ----------------------------------------------------------------------

// Scratch the (unused) SMAP_init bitmap memset writes into (res*res bytes;
// res = AENG_AA_BUF_SIZE). Content is never read. MUST be >= the largest
// res ever passed — undersizing here overruns it and corrupts adjacent
// statics (was the level-load crash when res went 64 -> 128). Guarded by
// an ASSERT(res <= SMAP_SCRATCH_DIM) at every SMAP_init call site.
#define SMAP_SCRATCH_DIM 256
static UBYTE SMAP_setup_scratch[SMAP_SCRATCH_DIM * SMAP_SCRATCH_DIM];

// P2-H GPU character shadow: one draw per character using the SAME
// consolidated bind-space mesh + per-frame skin palette the body will
// draw later this frame. Soft-skin weights deform the shadow naturally
// because we share the body's VBO. Returns false (doing nothing) if the
// model isn't ready for this frame (no shadow that frame, no fallback).
bool SMAP_person_gpu(
    Thing* p_thing,
    SLONG tex_page,
    SLONG off_x,
    SLONG off_y,
    UBYTE res,
    SLONG light_dx,
    SLONG light_dy,
    SLONG light_dz)
{
    // Resolve consolidated mesh + per-bone bind-space AABB + bind palette.
    // FIGURE_get_skin_mesh_for_thing builds whatever is missing — so when
    // SMAP runs BEFORE the body draw, this triggers the TPO walk and the
    // bind-space VBO build, and the later body draw just reuses them.
    GESkinMesh* mesh = NULL;
    const float* bone_aabb = NULL;
    int bone_count = 0;
    const GameKeyFrameChunk* chunk = NULL;
    const GEMatrix* bind_inv = NULL;
    if (!FIGURE_get_skin_mesh_for_thing(p_thing,
            &mesh, &bone_aabb, &bone_count,
            &chunk, &bind_inv))
        return false;
    if (!mesh || !bone_aabb || bone_count <= 0)
        return false;
    if (bone_count > POSE_MAX_BONES)
        bone_count = POSE_MAX_BONES;

    // Pose snapshot — per-bone interpolated world transforms.
    const BoneInterpTransform* current = render_interp_get_cached_pose(p_thing);
    if (!current)
        return false;

    // Build the skin palette (3 vec4 / bone in M*v form). Layout EXACTLY
    // matches skin_world_vert.glsl's u_skin and skin_shadow_vert.glsl's
    // u_skin — the same per-frame value the body draw will upload, so
    // shadow and body share their world-space pose.
    //
    //   skin[i] = current[i] × inv_bind[i]
    //
    // Inlined here (instead of calling figure_build_skin_world_palette)
    // because that helper is static in figure.cpp; the math is short and
    // bone_count is variable enough that exposing the helper would just
    // add a forwarding seam.
    float skin_palette[POSE_MAX_BONES * 12];
    {
        constexpr float S = 1.0F / 32768.0F;
        for (int i = 0; i < bone_count; ++i) {
            const Matrix33& cR = current[i].rot;
            const GEMatrix& bi = bind_inv[i];
            const float pos[3] = { current[i].pos_x, current[i].pos_y, current[i].pos_z };
            for (int r = 0; r < 3; ++r) {
                float* o = &skin_palette[(i * 3 + r) * 4];
                const float cR0 = float(cR.M[r][0]);
                const float cR1 = float(cR.M[r][1]);
                const float cR2 = float(cR.M[r][2]);
                o[0] = S * (cR0 * bi.m[0][0] + cR1 * bi.m[1][0] + cR2 * bi.m[2][0]);
                o[1] = S * (cR0 * bi.m[0][1] + cR1 * bi.m[1][1] + cR2 * bi.m[2][1]);
                o[2] = S * (cR0 * bi.m[0][2] + cR1 * bi.m[1][2] + cR2 * bi.m[2][2]);
                o[3] = S * (cR0 * bi.m[0][3] + cR1 * bi.m[1][3] + cR2 * bi.m[2][3]) + pos[r];
            }
        }
    }

    // Shadow-plane projection box. For each bone with a non-empty
    // bind-space AABB, transform its 8 corners by skin[bone] (= world
    // matrix for that bone) and project onto the sun plane. This is the
    // same shape as the legacy per-prim AABB approach, just expressed in
    // bind space — gives a tight world bound that keeps the silhouette
    // crisp in the texture and "stays near the feet" so the original
    // 64/256 distance fade behaves like the original.
    ASSERT(res <= SMAP_SCRATCH_DIM);
    SMAP_init(float(light_dx), float(light_dy), float(light_dz),
        SMAP_setup_scratch, res, res);

    int parts_used = 0;
    for (int b = 0; b < bone_count; ++b) {
        const float* mn = &bone_aabb[b * 6 + 0];
        const float* mx = &bone_aabb[b * 6 + 3];
        if (mn[0] > mx[0])
            continue; // bone unreferenced by any vertex

        const float* sp = &skin_palette[b * 12];
        for (int c = 0; c < 8; ++c) {
            float lx = (c & 1) ? mx[0] : mn[0];
            float ly = (c & 2) ? mx[1] : mn[1];
            float lz = (c & 4) ? mx[2] : mn[2];
            // world = skin[bone] × bind_corner (M*v form, .w = translation)
            float wxf = sp[0] * lx + sp[1] * ly + sp[2] * lz + sp[3];
            float wyf = sp[4] * lx + sp[5] * ly + sp[6] * lz + sp[7];
            float wzf = sp[8] * lx + sp[9] * ly + sp[10] * lz + sp[11];
            float au = wxf * SMAP_plane_ux + wyf * SMAP_plane_uy + wzf * SMAP_plane_uz;
            float av = wxf * SMAP_plane_vx + wyf * SMAP_plane_vy + wzf * SMAP_plane_vz;
            float an = wxf * SMAP_plane_nx + wyf * SMAP_plane_ny + wzf * SMAP_plane_nz;
            if (au < SMAP_u_min)
                SMAP_u_min = au;
            if (au > SMAP_u_max)
                SMAP_u_max = au;
            if (av < SMAP_v_min)
                SMAP_v_min = av;
            if (av > SMAP_v_max)
                SMAP_v_max = av;
            if (an < SMAP_n_min)
                SMAP_n_min = an;
            if (an > SMAP_n_max)
                SMAP_n_max = an;
        }
        parts_used++;
    }
    if (parts_used == 0)
        return false; // no bone with valid AABB (model variant uses none of the rig)
    SMAP_point_finished();

    // Orthographic world -> shadow-clip (w=1, z=0). Places a world point
    // at the SAME bitmap texel SMAP_point_finished/SMAP_project_onto_poly
    // use: pixel = 1 + (along - min)*(res-2)/(max-min); ndc = 2*pixel/res-1.
    float proj16[16];
    {
        float resf = float(res);
        float k = 2.0F / resf;
        float du = SMAP_u_max - SMAP_u_min;
        float dv = SMAP_v_max - SMAP_v_min;
        if (fabsf(du) < 1e-6F)
            du = 1e-6F;
        if (fabsf(dv) < 1e-6F)
            dv = 1e-6F;
        float su = (resf - 2.0F) / du;
        float sv = (resf - 2.0F) / dv;

        float Aux = k * su * SMAP_plane_ux;
        float Auy = k * su * SMAP_plane_uy;
        float Auz = k * su * SMAP_plane_uz;
        float bu = k * (1.0F - SMAP_u_min * su) - 1.0F;

        float Avx = k * sv * SMAP_plane_vx;
        float Avy = k * sv * SMAP_plane_vy;
        float Avz = k * sv * SMAP_plane_vz;
        float bv = k * (1.0F - SMAP_v_min * sv) - 1.0F;

        proj16[0] = Aux;
        proj16[4] = Auy;
        proj16[8] = Auz;
        proj16[12] = bu;
        proj16[1] = Avx;
        proj16[5] = Avy;
        proj16[9] = Avz;
        proj16[13] = bv;
        proj16[2] = 0;
        proj16[6] = 0;
        proj16[10] = 0;
        proj16[14] = 0;
        proj16[3] = 0;
        proj16[7] = 0;
        proj16[11] = 0;
        proj16[15] = 1;
    }

    // One draw per character — whole consolidated mesh through the
    // skin-shadow shader, sharing the body's per-frame skin palette.
    ge_shadow_silhouette_begin(tex_page, off_x, off_y, res, res);
    ge_shadow_silhouette_draw(mesh, skin_palette, (uint32_t)bone_count, proj16);
    ge_shadow_silhouette_end();
    return true;
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
