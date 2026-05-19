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
#include "engine/animation/anim_types.h" // GameKeyFrame, GameKeyFrameElement, GameKeyFrameChunk, GetCMatrix
#include "things/characters/person_types.h" // Person struct, FLAG_PERSON_*, ANIM_TYPE_*
#include "things/characters/person.h" // person_get_scale
#include "things/core/interact.h" // calc_sub_objects_position_global
#include "engine/graphics/render_interp.h" // Phase 4: per-bone snapshot pose for shadow
#include "engine/graphics/geometry/pose_composer.h" // POSE_MAX_BONES
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // Milestone 1E: GESkinMesh, ge_skin_mesh_*, ge_shadow_silhouette_*
#include "buildings/prim_types.h" // Milestone 1E: PrimObject/PrimFace3/4, MAX_PRIM_OBJECTS
#include <vector> // Milestone 1E: per-prim shadow mesh build scratch

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
// GPU shadow-silhouette path (skeletal_skinning_plan.md, Milestone 1E).
//
// Self-contained: SMAP_person_gpu does its own per-bone walk (so it works
// in every situation — cutscenes, partial visibility — independent of how
// the body is drawn) and renders each body part's static prim mesh on the
// GPU into the shadow texture sub-rect. Replaces the deleted CPU
// per-vertex transform + software AA rasteriser.
//
// The projection box is a TIGHT shadow-plane fit: each part's model-space
// AABB (8 corners) is transformed by the bone matrix and projected onto
// the sun plane. This bounds the body almost exactly (≈ the original's
// per-vertex bounds) so the silhouette fills the texture (crisp) and
// stays near the feet, which makes the original 64/256 distance fade
// behave like the original (soft far edge, no global "breathing").
// ----------------------------------------------------------------------

// Scratch the (unused) SMAP_init bitmap memset writes into (res*res bytes;
// res = AENG_AA_BUF_SIZE). Content is never read. MUST be >= the largest
// res ever passed — undersizing here overruns it and corrupts adjacent
// statics (was the level-load crash when res went 64 -> 128). Guarded by
// an ASSERT(res <= SMAP_SCRATCH_DIM) at every SMAP_init call site.
#define SMAP_SCRATCH_DIM 256
static UBYTE SMAP_setup_scratch[SMAP_SCRATCH_DIM * SMAP_SCRATCH_DIM];

// --- Per-prim persistent shadow mesh cache ---------------------------
// One static GPU mesh per body-part prim (model-space prim_points +
// triangulated faces, single bone). Built lazily, reused every frame.
// MUST be invalidated by clear_prims() on level load — the prim pools
// are reloaded then, so a kept mesh would dangle (a real corruption bug
// earlier in this milestone).
static GESkinMesh* s_prim_shadow_mesh[MAX_PRIM_OBJECTS] = { nullptr };
// Model-space AABB of each prim (min xyz, max xyz), computed once at mesh
// build. Per frame its 8 corners are transformed by the bone matrix and
// projected → the tight shadow-plane fit (see header above).
static float s_prim_shadow_min[MAX_PRIM_OBJECTS][3];
static float s_prim_shadow_max[MAX_PRIM_OBJECTS][3];

void SMAP_shadow_prim_cache_reset(void)
{
    for (int i = 0; i < MAX_PRIM_OBJECTS; i++) {
        if (s_prim_shadow_mesh[i]) {
            ge_skin_mesh_destroy(s_prim_shadow_mesh[i]);
            s_prim_shadow_mesh[i] = nullptr;
        }
    }
}

static GESkinMesh* smap_get_prim_shadow_mesh(SLONG prim)
{
    if (prim < 0 || prim >= MAX_PRIM_OBJECTS)
        return nullptr;
    if (s_prim_shadow_mesh[prim])
        return s_prim_shadow_mesh[prim];

    PrimObject* po = &prim_objects[prim];
    SLONG sp = po->StartPoint;
    SLONG ep = po->EndPoint;
    SLONG vcount = ep - sp;
    if (vcount <= 0)
        return nullptr;

    std::vector<GESkinVertex> verts((size_t)vcount);
    float mn[3] = { +1e30F, +1e30F, +1e30F };
    float mx[3] = { -1e30F, -1e30F, -1e30F };
    for (SLONG i = 0; i < vcount; i++) {
        GESkinVertex& v = verts[(size_t)i];
        memset(&v, 0, sizeof(v));
        float px = float(prim_points[sp + i].X);
        float py = float(prim_points[sp + i].Y);
        float pz = float(prim_points[sp + i].Z);
        v.x = px;
        v.y = py;
        v.z = pz;
        v.bone = 0; // single-bone (this part's world matrix)
        if (px < mn[0]) mn[0] = px;
        if (px > mx[0]) mx[0] = px;
        if (py < mn[1]) mn[1] = py;
        if (py > mx[1]) mx[1] = py;
        if (pz < mn[2]) mn[2] = pz;
        if (pz > mx[2]) mx[2] = pz;
    }
    s_prim_shadow_min[prim][0] = mn[0];
    s_prim_shadow_min[prim][1] = mn[1];
    s_prim_shadow_min[prim][2] = mn[2];
    s_prim_shadow_max[prim][0] = mx[0];
    s_prim_shadow_max[prim][1] = mx[1];
    s_prim_shadow_max[prim][2] = mx[2];

    std::vector<uint16_t> idx;
    if (po->EndFace3 > po->StartFace3) {
        for (SLONG f = po->StartFace3; f < po->EndFace3; f++) {
            PrimFace3* p = &prim_faces3[f];
            idx.push_back((uint16_t)(p->Points[0] - sp));
            idx.push_back((uint16_t)(p->Points[1] - sp));
            idx.push_back((uint16_t)(p->Points[2] - sp));
        }
    }
    if (po->EndFace4 > po->StartFace4) {
        for (SLONG f = po->StartFace4; f < po->EndFace4; f++) {
            PrimFace4* p = &prim_faces4[f];
            // Quad -> 2 tris: (0,1,2) and (1,3,2) (original prim winding).
            idx.push_back((uint16_t)(p->Points[0] - sp));
            idx.push_back((uint16_t)(p->Points[1] - sp));
            idx.push_back((uint16_t)(p->Points[2] - sp));
            idx.push_back((uint16_t)(p->Points[1] - sp));
            idx.push_back((uint16_t)(p->Points[3] - sp));
            idx.push_back((uint16_t)(p->Points[2] - sp));
        }
    }
    if (idx.empty())
        return nullptr;

    s_prim_shadow_mesh[prim] = ge_skin_mesh_create(
        verts.data(), (uint32_t)verts.size(),
        idx.data(), (uint32_t)idx.size(), 1);
    return s_prim_shadow_mesh[prim];
}

// --- Per-bone WORLD transform ----------------------------------------
// Per-body-part WORLD transform read straight from the shared interpolation
// pose snapshot — the SINGLE always-available source of pose. No legacy
// keyframe recompute (the snapshot already bakes the same math the body
// draw uses, so the shadow silhouette matches the rendered body). Returns
// false if this part has no pose entry; the caller then skips it.
static bool smap_bone_world(
    struct GameKeyFrameElement* anim_info,
    Thing* p_thing,
    Matrix33* out_mat, SLONG* out_x, SLONG* out_y, SLONG* out_z)
{
    const BoneInterpTransform* pose = render_interp_get_cached_pose(p_thing);
    if (!pose)
        return false;

    DrawTween* dt = p_thing->Draw.Tweened;
    if (!dt || !dt->CurrentFrame || !dt->CurrentFrame->FirstElement)
        return false;

    SLONG part = SLONG(anim_info - dt->CurrentFrame->FirstElement);
    if (part < 0 || part >= POSE_MAX_BONES)
        return false;

    const BoneInterpTransform& xf = pose[part];
    *out_x = SLONG(xf.pos_x);
    *out_y = SLONG(xf.pos_y);
    *out_z = SLONG(xf.pos_z);
    *out_mat = xf.rot;
    return true;
}

// SMAP_* globals + world->shadow-clip matrix from a TIGHT shadow-plane
// fit: each part's model-space AABB (8 corners) is transformed by the
// bone matrix and projected onto the plane. This bounds the actual body
// almost exactly (≈ the original SMAP's per-vertex bounds) so the
// silhouette fills the texture → crisp, and stays near the feet → the
// original 64/256 distance fade then behaves like the original (soft far
// edge, no global breathing). uc_orig: none — Milestone 1E.
static void SMAP_setup_box(
    SLONG light_dx, SLONG light_dy, SLONG light_dz, UBYTE res,
    const Matrix33* m_arr, const SLONG* wx, const SLONG* wy, const SLONG* wz,
    const SLONG* prim_arr, int n_parts, float out_proj[16])
{
    ASSERT(res <= SMAP_SCRATCH_DIM); // SMAP_init memsets res*res of scratch
    SMAP_init(float(light_dx), float(light_dy), float(light_dz),
        SMAP_setup_scratch, res, res);

    const float inv = 1.0F / 32768.0F; // matrix is fixed-point ×32768
    for (int p = 0; p < n_parts; p++) {
        const Matrix33& m = m_arr[p];
        const float* bmn = s_prim_shadow_min[prim_arr[p]];
        const float* bmx = s_prim_shadow_max[prim_arr[p]];
        for (int c = 0; c < 8; c++) {
            float lx = (c & 1) ? bmx[0] : bmn[0];
            float ly = (c & 2) ? bmx[1] : bmn[1];
            float lz = (c & 4) ? bmx[2] : bmn[2];
            float wxf = (m.M[0][0] * lx + m.M[0][1] * ly + m.M[0][2] * lz) * inv + float(wx[p]);
            float wyf = (m.M[1][0] * lx + m.M[1][1] * ly + m.M[1][2] * lz) * inv + float(wy[p]);
            float wzf = (m.M[2][0] * lx + m.M[2][1] * ly + m.M[2][2] * lz) * inv + float(wz[p]);
            float au = wxf * SMAP_plane_ux + wyf * SMAP_plane_uy + wzf * SMAP_plane_uz;
            float av = wxf * SMAP_plane_vx + wyf * SMAP_plane_vy + wzf * SMAP_plane_vz;
            float an = wxf * SMAP_plane_nx + wyf * SMAP_plane_ny + wzf * SMAP_plane_nz;
            if (au < SMAP_u_min) SMAP_u_min = au;
            if (au > SMAP_u_max) SMAP_u_max = au;
            if (av < SMAP_v_min) SMAP_v_min = av;
            if (av > SMAP_v_max) SMAP_v_max = av;
            if (an < SMAP_n_min) SMAP_n_min = an;
            if (an > SMAP_n_max) SMAP_n_max = an;
        }
    }
    SMAP_point_finished();

    // Orthographic world -> shadow-clip (w=1, z=0). Places a world point
    // at the SAME bitmap texel SMAP_point_finished/SMAP_project_onto_poly
    // use: pixel = 1 + (along - min)*(res-2)/(max-min); ndc = 2*pixel/res-1.
    float resf = float(res);
    float k = 2.0F / resf;
    float du = SMAP_u_max - SMAP_u_min;
    float dv = SMAP_v_max - SMAP_v_min;
    if (fabsf(du) < 1e-6F) du = 1e-6F;
    if (fabsf(dv) < 1e-6F) dv = 1e-6F;
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

    out_proj[0] = Aux; out_proj[4] = Auy; out_proj[8] = Auz;  out_proj[12] = bu;
    out_proj[1] = Avx; out_proj[5] = Avy; out_proj[9] = Avz;  out_proj[13] = bv;
    out_proj[2] = 0;   out_proj[6] = 0;   out_proj[10] = 0;   out_proj[14] = 0;
    out_proj[3] = 0;   out_proj[7] = 0;   out_proj[11] = 0;   out_proj[15] = 1;
}

// uc_orig: none — Milestone 1E. GPU replacement of SMAP_person.
// Sets the SMAP_* globals (so the unchanged ground projection works) and
// renders the silhouette into texture page `tex_page` sub-rect
// (off_x,off_y,res,res). Returns false (doing nothing) if the model is
// not ready that frame (no shadow that frame; no CPU fallback).
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
    DrawTween* dt = p_thing->Draw.Tweened;
    if (!dt || dt->CurrentFrame == 0 || dt->NextFrame == 0)
        return false;

    GameKeyFrameElement* ae1 = dt->CurrentFrame->FirstElement;
    if (!ae1)
        return false;

    SLONG ele_count = dt->TheChunk->ElementCount;
    SLONG start_object = prim_multi_objects[dt->TheChunk->MultiObject[0]].StartObject;
#define SMAP_GPU_MAX_PARTS 20
    if (ele_count > SMAP_GPU_MAX_PARTS)
        ele_count = SMAP_GPU_MAX_PARTS;

    // Pass 1: per-part bone world transform (15 transforms — cheap, no
    // per-vertex pass). The tight shadow-plane bounds are computed from
    // each part's model AABB in SMAP_setup_box.
    Matrix33 m_arr[SMAP_GPU_MAX_PARTS];
    SLONG wx_arr[SMAP_GPU_MAX_PARTS], wy_arr[SMAP_GPU_MAX_PARTS], wz_arr[SMAP_GPU_MAX_PARTS];
    SLONG prim_arr[SMAP_GPU_MAX_PARTS];
    GESkinMesh* mesh_arr[SMAP_GPU_MAX_PARTS];
    int n_parts = 0;

    for (SLONG i = 0; i < ele_count; i++) {
        SLONG object_offset = dt->TheChunk->PeopleTypes[dt->PersonID & 0x1f].BodyPart[i];
        SLONG prim = start_object + object_offset;

        GESkinMesh* mesh = smap_get_prim_shadow_mesh(prim);
        if (!mesh)
            continue;

        Matrix33 m;
        SLONG wx, wy, wz;
        if (!smap_bone_world(&ae1[i], p_thing, &m, &wx, &wy, &wz))
            continue;

        m_arr[n_parts] = m;
        wx_arr[n_parts] = wx;
        wy_arr[n_parts] = wy;
        wz_arr[n_parts] = wz;
        prim_arr[n_parts] = prim;
        mesh_arr[n_parts] = mesh;
        n_parts++;
    }

    if (n_parts == 0)
        return false; // nothing to draw — caller skips (no stale globals)

    float proj16[16];
    SMAP_setup_box(light_dx, light_dy, light_dz, res,
        m_arr, wx_arr, wy_arr, wz_arr, prim_arr, n_parts, proj16);

    // Pass 2: render the silhouette.
    ge_shadow_silhouette_begin(tex_page, off_x, off_y, res, res);

    const float inv = 1.0F / 32768.0F;
    for (int p = 0; p < n_parts; p++) {
        const Matrix33& m = m_arr[p];
        // World affine palette (1 bone, 3 vec4 rows). matrix_transform_small
        // does (sum)>>15, so divide the fixed-point ×32768 matrix by 32768.
        float palette[12] = {
            float(m.M[0][0]) * inv, float(m.M[0][1]) * inv, float(m.M[0][2]) * inv, float(wx_arr[p]),
            float(m.M[1][0]) * inv, float(m.M[1][1]) * inv, float(m.M[1][2]) * inv, float(wy_arr[p]),
            float(m.M[2][0]) * inv, float(m.M[2][1]) * inv, float(m.M[2][2]) * inv, float(wz_arr[p])
        };
        ge_shadow_silhouette_draw(mesh_arr[p], palette, 1, proj16);
    }

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
