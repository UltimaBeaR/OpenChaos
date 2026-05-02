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

#include "engine/core/types.h"
#include "engine/core/fmatrix.h"

struct Thing;

// Per-bone world-space transform — exactly the (off_x/y/z, mat_final) pair
// FIGURE_draw_prim_tween computes and passes to POLY_set_local_rotation.
struct ComposedBone {
    float pos_x, pos_y, pos_z;   // world position (post character_scale, post WorldPos add)
    Matrix33 rot;                 // world rotation (post character_scale, fixed-point ×32768)
};

// Bounded by the person rig in anim_ids.h (PELVIS..RIGHT_FOOT = 15 bones).
// Other DT_TWEEN family (animals, bats, generic) use a flat skeleton with
// different bone counts — handled via a separate path in later phases.
constexpr int POSE_MAX_BONES = 15;

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
