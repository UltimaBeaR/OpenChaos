#include "engine/graphics/geometry/bind_palette.h"

#include "engine/animation/anim_types.h"                      // GameKeyFrameChunk, GameKeyFrameElement
#include "engine/core/fmatrix.h"                              // Matrix33/31, matrix_mult33, matrix_transformZMY, rotate_obj
#include "engine/core/macros.h"                               // SWAP
#include "engine/graphics/geometry/pose_composer.h"           // POSE_PERSON_BONE_COUNT, POSE_MAX_BONES, body_part_parent
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // GEMatrix

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// 15-bone person rig — animals, bats, Bane, Balrog use the flat-skeleton
// branch (chunk->ElementCount != PERSON_BONES).
static constexpr int PERSON_BONES = POSE_PERSON_BONE_COUNT; // 15

// UC engine angle convention: 0..2047 == one full turn.
// 30 degrees = round(2048 * 30 / 360) = 171. A-pose at 30 deg (not full 90 T)
// keeps the rigid shoulder-pad chunks separated from the upper-arm chunks at
// the seam — confirmed by the render_interp.cpp T-pose override visual test
// (see skeletal_skinning_phase2_plan.md section 3.2).
static constexpr SWORD A_POSE_SHOULDER_ANGLE = 171;

// Body-part indices that get a non-identity A-pose local rotation. The two
// upper-arm bones: LEFT_HUMORUS (5) gets -ANGLE about Z, RIGHT_HUMORUS (8)
// gets +ANGLE about Z. Opposite signs because the two arms are mirrored —
// same sign would send both arms to the same side through the body.
static constexpr int A_POSE_LEFT_HUMORUS  = 5;
static constexpr int A_POSE_RIGHT_HUMORUS = 8;

// ----------------------------------------------------------------------------
// Local helpers
// ----------------------------------------------------------------------------

// Decompress one CMatrix33 into a Matrix33 (15-bit fractional, scale 32768).
// Inverse of compress_matrix in render_interp.cpp:298; same bit layout as
// uncompress_matrix in hierarchy.cpp (kept file-local there). Reproduced
// here to keep bind_palette self-contained.
//
// CMatrix33 packs 9 signed 10-bit values across 3 SLONGs (mask layout in
// fmatrix.h). Each element is sign-extended via an arithmetic-shift chain,
// then scaled <<6 back to 15-bit fractional (10 bits stored + 6 bits scale).
static void uncompress_cmatrix(const CMatrix33& cm, Matrix33& m)
{
    SLONG v;
    v = ((cm.M[0] & CMAT0_MASK) <<  2) >> 22; m.M[0][0] = v << 6;
    v = ((cm.M[0] & CMAT1_MASK) << 12) >> 22; m.M[0][1] = v << 6;
    v = ((cm.M[0] & CMAT2_MASK) << 22) >> 22; m.M[0][2] = v << 6;
    v = ((cm.M[1] & CMAT0_MASK) <<  2) >> 22; m.M[1][0] = v << 6;
    v = ((cm.M[1] & CMAT1_MASK) << 12) >> 22; m.M[1][1] = v << 6;
    v = ((cm.M[1] & CMAT2_MASK) << 22) >> 22; m.M[1][2] = v << 6;
    v = ((cm.M[2] & CMAT0_MASK) <<  2) >> 22; m.M[2][0] = v << 6;
    v = ((cm.M[2] & CMAT1_MASK) << 12) >> 22; m.M[2][1] = v << 6;
    v = ((cm.M[2] & CMAT2_MASK) << 22) >> 22; m.M[2][2] = v << 6;
}

// Write a rigid-body (rotation R + translation t) transform into a GEMatrix.
//
// Convention here is the "M * v" (column-vector) form used by the engine's
// matrix_transformZMY, matrix_mult33, and the world/shadow skin shaders:
//   v_out = R * v_in + t            with R in the top-left 3x3 directly,
//                                   t  in COLUMN 4 (._14, ._24, ._34).
// Bottom row (._41, ._42, ._43, ._44) is the homogeneous (0,0,0,1) row,
// unused at runtime but kept tidy.
//
// This is intentionally NOT the row-vector layout used by g_matWorld /
// g_matProjection (where translation lives in row 4 and the rotation is
// stored transposed) — those matrices feed the screen projection bake,
// which is a separate uniform in the world-skin shader. bind_world is the
// per-bone skin operand, which the shader consumes in M*v form so it can
// share the shadow_sil layout.
//
//   M = | R  t |   ->  | R00 R01 R02 tx |
//       | 0  1 |       | R10 R11 R12 ty |
//                      | R20 R21 R22 tz |
//                      |  0   0   0   1 |
//
// R is at engine fixed-point scale 32768 (Matrix33 convention); t is in
// float "inches" — same units BoneInterpTransform.pos_x/_y/_z uses.
static void rigid_to_ge(const Matrix33& R, float tx, float ty, float tz, GEMatrix& out)
{
    constexpr float S = 1.0f / 32768.0f;
    out._11 = R.M[0][0] * S; out._12 = R.M[0][1] * S; out._13 = R.M[0][2] * S; out._14 = tx;
    out._21 = R.M[1][0] * S; out._22 = R.M[1][1] * S; out._23 = R.M[1][2] * S; out._24 = ty;
    out._31 = R.M[2][0] * S; out._32 = R.M[2][1] * S; out._33 = R.M[2][2] * S; out._34 = tz;
    out._41 = 0.0f;          out._42 = 0.0f;          out._43 = 0.0f;          out._44 = 1.0f;
}

// Inverse of a rigid-body transform, in the same "M * v" convention.
//
// Forward: v_out = R * v_in + t.
// Inverse: v_in = R^-1 * (v_out - t) = R^T * v_out - R^T * t.
//
// So  M^-1 = | R^T  -R^T * t |
//            |  0       1    |
//
// where R^T[i][j] = R[j][i]. The "-R^T * t" column is computed below.
static void rigid_inv_to_ge(const Matrix33& R, float tx, float ty, float tz, GEMatrix& out)
{
    constexpr float S = 1.0f / 32768.0f;
    // R^T: store R[j][i] in position [i][j].
    const float rT00 = R.M[0][0] * S, rT01 = R.M[1][0] * S, rT02 = R.M[2][0] * S;
    const float rT10 = R.M[0][1] * S, rT11 = R.M[1][1] * S, rT12 = R.M[2][1] * S;
    const float rT20 = R.M[0][2] * S, rT21 = R.M[1][2] * S, rT22 = R.M[2][2] * S;

    // -R^T * t  (column vector). Each component i: -dot(R^T row i, t)
    //                                            = -dot(R col i, t).
    const float itx = -(rT00 * tx + rT01 * ty + rT02 * tz);
    const float ity = -(rT10 * tx + rT11 * ty + rT12 * tz);
    const float itz = -(rT20 * tx + rT21 * ty + rT22 * tz);

    out._11 = rT00; out._12 = rT01; out._13 = rT02; out._14 = itx;
    out._21 = rT10; out._22 = rT11; out._23 = rT12; out._24 = ity;
    out._31 = rT20; out._32 = rT21; out._33 = rT22; out._34 = itz;
    out._41 = 0.0f; out._42 = 0.0f; out._43 = 0.0f; out._44 = 1.0f;
}

// ----------------------------------------------------------------------------
// Per-chunk cache
// ----------------------------------------------------------------------------

// Variable-length palette (up to POSE_MAX_BONES). bone_count tells how many
// of the slots are populated; the rest are uninitialised garbage and must
// not be indexed.
struct PaletteCache {
    const GameKeyFrameChunk* chunk;    // ptr-key; NULL = empty slot
    int                      bone_count;
    GEMatrix                 world   [POSE_MAX_BONES];
    GEMatrix                 inv_bind[POSE_MAX_BONES];
};

// Linear-scan table. The game has ~5 anim chunks total (4 person types +
// each non-person creature has its own chunk), so a small fixed array is
// fine — way cheaper than a hash map for this size.
static constexpr int MAX_PALETTE_CACHE = 16;
static PaletteCache  s_cache[MAX_PALETTE_CACHE] = {};

// Per-group soft rig parameters — see bind_palette.h. Only the HEAD
// parent slot is non-zero (band=20, w_max=1.0) — slightly softens the
// neck seam on most character meshes. HANDS and FEET stay at zero
// because mesh layouts vary too much across characters for a single
// universal tune (one model puts the neck in TORSO, another in HEAD;
// same story for wrists/ankles). Tracked as a Post-1.0 issue in
// known_issues_and_bugs.md (the rig is missing a dedicated upper-
// body / neck bone which would let this be solved properly).
SkinTuneGroup g_skin_tune_groups[SKIN_TUNE_GROUP_COUNT] = {
    /* HEAD  */ { /*parent_band*/ 20.0f, /*parent_wmax*/ 1.0f, /*child_band*/ 0.0f, /*child_wmax*/ 0.0f },
    /* HANDS */ { 0.0f, 0.0f, 0.0f, 0.0f },
    /* FEET  */ { 0.0f, 0.0f, 0.0f, 0.0f },
};

// Skeleton debug overlay — see bind_palette.h. Default off (no
// per-frame cost when disabled).
bool g_skin_debug_draw_skeleton = false;

// First keyframe in the chunk that actually carries element data.
// AnimKeyFrames[0] is a reserved sentinel (FirstElement=NULL) in the
// chunk's disk format — same "slot 0 invalid" pattern as next_prim_point
// and friends. Scan forward to the first keyframe with element data;
// gives us a reference rest-pose skeleton. The exact keyframe doesn't
// matter for the bind palette math — the same bind pose is used at mesh
// build time and at per-frame skin palette build (skin = current *
// inv_bind), so any consistent reference cancels out.
//
// Returns NULL when no keyframe in the chunk has element data.
static GameKeyFrameElement* first_valid_keyframe_elements(const GameKeyFrameChunk& chunk)
{
    if (!chunk.AnimKeyFrames) return NULL;
    for (SLONG kf = 0; kf < chunk.MaxKeyFrames; ++kf) {
        if (chunk.AnimKeyFrames[kf].FirstElement)
            return chunk.AnimKeyFrames[kf].FirstElement;
    }
    return NULL;
}

// Build the palette for a 15-bone person rig with the A-pose. PELVIS at
// origin, identity locals everywhere except the two upper-arms (±30° Z),
// then forward-kinematic the chain — same parent-local derivation that
// capture_pose_hierarchical (render_interp.cpp) uses.
static void build_palette_person(const GameKeyFrameChunk& chunk,
                                 GameKeyFrameElement* ae0,
                                 PaletteCache& cache)
{
    // ----- 1. Read keyframe-0 rotations and offsets ------------------------
    //
    // ae0[i].Offset* are signed shorts in skeleton-model units; the renderer
    // shifts them <<8 to align with the body_local_pos / BoneSnap convention
    // (scale x256). See pose_composer.cpp:132 for the root case and
    // render_interp.cpp:516-535 for the parent-local derivation we mirror.
    Matrix33 kf0_rot[PERSON_BONES];
    Matrix31 kf0_off[PERSON_BONES];
    for (int i = 0; i < PERSON_BONES; ++i) {
        uncompress_cmatrix(ae0[i].CMatrix, kf0_rot[i]);
        kf0_off[i].M[0] = SLONG(ae0[i].OffsetX) << 8;
        kf0_off[i].M[1] = SLONG(ae0[i].OffsetY) << 8;
        kf0_off[i].M[2] = SLONG(ae0[i].OffsetZ) << 8;
    }

    // ----- 2. Parent-local rest offsets ------------------------------------
    //
    // Each non-root bone's "where does this bone sit relative to its parent
    // in the parent's rest-rotation frame" — same derivation as
    // capture_pose_hierarchical (render_interp.cpp:516-535):
    //
    //   parent_local_offset[i] = parent_rot[p]^T * (kf0_off[i] - kf0_off[p])
    //
    // Output scale is x256 (same as input). matrix_transformZMY divides by
    // 32768 internally, so with parent_rot at scale 32768 and diff at x256
    // the result lands back at x256.
    Matrix31 parent_local_offset[PERSON_BONES] = {};
    for (int i = 1; i < PERSON_BONES; ++i) {
        const int p = body_part_parent[i];

        // Inverse of an orthogonal rotation matrix is its transpose
        // (same in-place swap as HIERARCHY_Get_Body_Part_Offset).
        Matrix33 parent_rot_inv = kf0_rot[p];
        SWAP(parent_rot_inv.M[0][1], parent_rot_inv.M[1][0]);
        SWAP(parent_rot_inv.M[0][2], parent_rot_inv.M[2][0]);
        SWAP(parent_rot_inv.M[1][2], parent_rot_inv.M[2][1]);

        Matrix31 diff;
        diff.M[0] = kf0_off[i].M[0] - kf0_off[p].M[0];
        diff.M[1] = kf0_off[i].M[1] - kf0_off[p].M[1];
        diff.M[2] = kf0_off[i].M[2] - kf0_off[p].M[2];
        matrix_transformZMY(&parent_local_offset[i], &parent_rot_inv, &diff);
    }

    // ----- 3. Forward kinematics with A-pose locals ------------------------
    //
    // PELVIS at origin with identity rotation; every other bone gets identity
    // parent-local rotation (the natural arms-down rest pose of the rig),
    // except the two upper-arm bones which get +/-30 deg about Z to splay
    // the arms outward into the A-pose. Mirror of the existing T-pose
    // override math in render_interp.cpp:1252-1273.
    Matrix33 ident = {};
    ident.M[0][0] = 32768;
    ident.M[1][1] = 32768;
    ident.M[2][2] = 32768;

    Matrix33 rot_left_shoulder, rot_right_shoulder;
    rotate_obj(0, 0, -A_POSE_SHOULDER_ANGLE, &rot_left_shoulder);
    rotate_obj(0, 0,  A_POSE_SHOULDER_ANGLE, &rot_right_shoulder);

    Matrix33 bind_rot[PERSON_BONES];
    float    bind_pos[PERSON_BONES][3];

    bind_rot[0]    = ident;
    bind_pos[0][0] = 0.0f;
    bind_pos[0][1] = 0.0f;
    bind_pos[0][2] = 0.0f;

    // body_part_parent[i] < i holds for all i (the table is laid out in
    // pre-order DFS — pose_composer.cpp:17). So a plain ascending walk is
    // safe: each bone's parent is already resolved when we get to it.
    for (int i = 1; i < PERSON_BONES; ++i) {
        const int p = body_part_parent[i];

        // matrix_mult33 takes non-const pointers; the operands aren't
        // modified, just an API quirk inherited from the original engine.
        Matrix33* local_rot;
        if      (i == A_POSE_LEFT_HUMORUS)  local_rot = &rot_left_shoulder;
        else if (i == A_POSE_RIGHT_HUMORUS) local_rot = &rot_right_shoulder;
        else                                local_rot = &ident;
        matrix_mult33(&bind_rot[i], &bind_rot[p], local_rot);

        // world_pos[i] = world_pos[parent] + world_rot[parent] * parent_local_offset[i] / 256.
        // Same conversion as the T-pose override in render_interp.cpp:1262-1264.
        Matrix31 rotated;
        matrix_transformZMY(&rotated, &bind_rot[p], &parent_local_offset[i]);
        bind_pos[i][0] = bind_pos[p][0] + float(rotated.M[0]) / 256.0f;
        bind_pos[i][1] = bind_pos[p][1] + float(rotated.M[1]) / 256.0f;
        bind_pos[i][2] = bind_pos[p][2] + float(rotated.M[2]) / 256.0f;
    }

    // ----- 4. Emit GEMatrix world transform + its inverse ------------------
    //
    // Rigid body (rotation + translation, no scale) — inverse is the
    // transpose-and-back-translate form (see rigid_inv_to_ge comment).
    for (int i = 0; i < PERSON_BONES; ++i) {
        rigid_to_ge    (bind_rot[i], bind_pos[i][0], bind_pos[i][1], bind_pos[i][2], cache.world   [i]);
        rigid_inv_to_ge(bind_rot[i], bind_pos[i][0], bind_pos[i][1], bind_pos[i][2], cache.inv_bind[i]);
    }
    cache.bone_count = PERSON_BONES;
    (void)chunk; // unused (kept for symmetry with build_palette_flat)
}

// Build the palette for a flat-skeleton rig (no parent/child hierarchy —
// Bane, Balrog, bats, Gargoyle). Each bone gets identity rotation, and
// its bind position is the raw keyframe-0 offset in float inches. This
// matches the per-frame world transform produced by render_interp's
// compose_flat_skeletal_pose:
//
//   pos[i] = WorldPos + R(body_angles) * (kf_offset * scale)
//
// `current[i] * inv_bind[i]` collapses to a transform that takes a
// bone-local authored vertex straight to world — equivalent to the old
// per-bone POLY_set_local_rotation path, just expressed inside the
// shared soft-skinning shader.
static void build_palette_flat(const GameKeyFrameChunk& chunk,
                               GameKeyFrameElement* ae0,
                               PaletteCache& cache)
{
    Matrix33 ident = {};
    ident.M[0][0] = 32768;
    ident.M[1][1] = 32768;
    ident.M[2][2] = 32768;

    const int n = chunk.ElementCount;
    for (int i = 0; i < n; ++i) {
        // Keyframe offsets are SWORD scale-x1 inches here (the per-frame
        // pose path multiplies by character_scale; the bind reference is
        // the unscaled rest-pose value — scale lives only in `current`).
        const float tx = float(ae0[i].OffsetX);
        const float ty = float(ae0[i].OffsetY);
        const float tz = float(ae0[i].OffsetZ);
        rigid_to_ge    (ident, tx, ty, tz, cache.world   [i]);
        rigid_inv_to_ge(ident, tx, ty, tz, cache.inv_bind[i]);
    }
    cache.bone_count = n;
}

// Build the palette for a chunk. Returns false if the chunk has no usable
// keyframe data or its element count is outside the supported range.
static bool build_palette(const GameKeyFrameChunk& chunk, PaletteCache& cache)
{
    if (chunk.ElementCount <= 0 || chunk.ElementCount > POSE_MAX_BONES)
        return false;

    GameKeyFrameElement* ae0 = first_valid_keyframe_elements(chunk);
    if (!ae0) return false;

    if (chunk.ElementCount == PERSON_BONES)
        build_palette_person(chunk, ae0, cache);
    else
        build_palette_flat  (chunk, ae0, cache);
    return true;
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

bool bind_palette_get(const GameKeyFrameChunk* chunk,
                      const GEMatrix** out_world,
                      const GEMatrix** out_inv_bind,
                      int*             out_bone_count)
{
    if (!chunk) return false;

    // 1) Existing slot for this chunk?
    int free_slot = -1;
    for (int i = 0; i < MAX_PALETTE_CACHE; ++i) {
        if (s_cache[i].chunk == chunk) {
            if (out_world)      *out_world      = s_cache[i].world;
            if (out_inv_bind)   *out_inv_bind   = s_cache[i].inv_bind;
            if (out_bone_count) *out_bone_count = s_cache[i].bone_count;
            return true;
        }
        if (free_slot < 0 && s_cache[i].chunk == NULL)
            free_slot = i;
    }

    // 2) No slot? Cache is full — caller bug or hard misconfiguration. The
    // game has ~5 anim chunks; running out of slots means MAX_PALETTE_CACHE
    // is too small for the current data. Fail loudly in release as `false`.
    if (free_slot < 0) return false;

    // 3) Build into the free slot.
    PaletteCache& slot = s_cache[free_slot];
    if (!build_palette(*chunk, slot)) return false;
    slot.chunk = chunk;
    if (out_world)      *out_world      = slot.world;
    if (out_inv_bind)   *out_inv_bind   = slot.inv_bind;
    if (out_bone_count) *out_bone_count = slot.bone_count;
    return true;
}

void bind_palette_invalidate_all(void)
{
    for (int i = 0; i < MAX_PALETTE_CACHE; ++i) {
        s_cache[i].chunk      = NULL;
        s_cache[i].bone_count = 0;
    }
}
