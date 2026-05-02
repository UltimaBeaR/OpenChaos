#ifndef ENGINE_GRAPHICS_GEOMETRY_POSE_COMPOSER_H
#define ENGINE_GRAPHICS_GEOMETRY_POSE_COMPOSER_H

// Pure pose composition for Tween-family Things (CLASS_PERSON).
//
// Reads the current (post-physics-tick) Draw.Tweened state and writes a full
// per-bone world-space transform array — equivalent to what figure.cpp
// computes per render frame and feeds to POLY_set_local_rotation.
//
// Used by:
//   - Phase 0/1 debug visualisation: draw a "PEL_NEW" label at bones[0].pos
//     and verify it matches the existing "PEL" label (= calc_sub_objects_position
//     for SUB_OBJECT_PELVIS) on the visible pelvis bone.
//   - Phase 2 capture (planned): write per-tick output into ThingSnap pose data
//     so render frame can lerp between prev/curr per-bone world transforms,
//     replacing the current AnimTween + body-angle substitution path.
//
// Pure: does not modify any global state (interact_globals offset/matrix,
// figure.cpp FIGURE_dhpr_data, etc). Safe to call multiple times per render
// frame or in physics tick.

#include "engine/core/fmatrix.h" // Matrix31, Matrix33, CMatrix33 (transitively brings types.h)

struct Thing;

// Per-bone transforms: world-space (for figure.cpp render path / Phase 1
// golden test) AND body-local intermediates (for Phase 2 snapshot capture
// to derive parent-local representation).
struct ComposedBone {
    // World transform — the (off_x/y/z, mat_final) pair FIGURE_draw_prim_tween
    // computes and passes to POLY_set_local_rotation.
    float pos_x, pos_y, pos_z;     // world position (post character_scale, post WorldPos add)
    Matrix33 rot;                   // world rotation (post character_scale, fixed-point ×32768)

    // Body-local intermediates (mirrors FIGURE_dhpr_rdata2[].end_pos / .end_mat
    // in the original recurse). Used by snapshot capture to derive
    // parent-local representation via:
    //   local_pos[i] = body_local_rot[parent]^T × (body_local_pos[i] - body_local_pos[parent])
    //   local_rot[i] = body_local_rot[parent]^T × body_local_rot[i]
    // These are independent of body angles and WorldPos — pure functions of
    // anim keyframes (CF/NF/AT) — which is why parent-local data lerps
    // smoothly across anim transitions while world data preserves cancel-out.
    Matrix31 body_local_pos;        // end_pos: per-bone body-frame position (×256 scale)
    Matrix33 body_local_rot;        // end_mat: per-bone body-frame rotation (slerp result, no character_scale)
};

// Bounded by the person rig in anim_ids.h (PELVIS..RIGHT_FOOT = 15 bones).
// Other DT_TWEEN family (animals, bats, generic) use a flat skeleton with
// different bone counts — handled via a separate path in later phases.
constexpr int POSE_MAX_BONES = 15;

// Parent index per bone, derived from body_part_children[][] (hierarchy_globals.cpp).
// PELVIS (0) is root with parent = -1.
extern const int body_part_parent[POSE_MAX_BONES];

struct ComposedSkeletalPose {
    int  bone_count;                       // 15 for persons; 0 if invalid
    ComposedBone bones[POSE_MAX_BONES];
    bool valid;                            // true iff composition succeeded
};

// Compose full per-bone pose for a person Thing using current Draw.Tweened state.
// Returns false (and sets out->valid = false) if Thing isn't a 15-bone person
// or has no valid keyframe data. Outputs identical math to FIGURE_draw_prim_tween's
// per-bone computation including HIERARCHY_Get_Body_Part_Offset for child bones.
bool compose_full_skeletal_pose(Thing* p_thing, ComposedSkeletalPose* out);

#endif // ENGINE_GRAPHICS_GEOMETRY_POSE_COMPOSER_H
