#include "engine/graphics/geometry/pose_composer.h"

#include "things/core/thing.h"
#include "things/core/drawtype.h"
#include "things/core/hierarchy.h"
#include "things/characters/person.h"     // person_get_scale
#include "engine/animation/anim_types.h"  // GetCMatrix, GameKeyFrame*, DrawTween, ANIM_FLAG_LAST_FRAME
#include "engine/core/quaternion.h"       // CQuaternion::BuildTween
#include "engine/graphics/geometry/figure.h" // FIGURE_rotate_obj

// matrix_mult33 prototype is in fmatrix.h (transitively included).

// Inverse of body_part_children[][]. PELVIS (0) is root with parent = -1.
// Verified by walking body_part_children entries in hierarchy_globals.cpp:
//   0 → [1, 4, 12]; 1 → [2]; 2 → [3]; 4 → [5, 8, 11]; 5 → [6]; 6 → [7];
//   8 → [9]; 9 → [10]; 12 → [13]; 13 → [14]. Leaves: 3, 7, 10, 11, 14.
const int body_part_parent[POSE_MAX_BONES] = {
    -1,  // 0  PELVIS (root)
     0,  // 1  LEFT_FEMUR
     1,  // 2  LEFT_TIBIA
     2,  // 3  LEFT_FOOT
     0,  // 4  TORSO
     4,  // 5  LEFT_HUMORUS
     5,  // 6  LEFT_RADIUS
     6,  // 7  LEFT_HAND
     4,  // 8  RIGHT_HUMORUS
     8,  // 9  RIGHT_RADIUS
     9,  // 10 RIGHT_HAND
     4,  // 11 HEAD
     0,  // 12 RIGHT_FEMUR
    12,  // 13 RIGHT_TIBIA
    13,  // 14 RIGHT_FOOT
};

// Iterative pre-order DFS frame, mirroring FIGURE_draw_hierarchical_prim_recurse
// (figure.cpp:2266) but pure — no globals mutated.
namespace {

struct WalkFrame {
    int part_number;
    int current_child_number;
};

// Body rest-pose offset of a bone. Equals ae1[part].Offset{X,Y,Z} (the first
// keyframe's per-bone position), which the original recurse uses as the
// "base" position for HIERARCHY_Get_Body_Part_Offset.
inline Matrix31 rest_offset_from_ae1(GameKeyFrameElement* ae1, int part)
{
    Matrix31 r;
    r.M[0] = ae1[part].OffsetX;
    r.M[1] = ae1[part].OffsetY;
    r.M[2] = ae1[part].OffsetZ;
    return r;
}

}  // namespace

bool compose_full_skeletal_pose(Thing* p_thing, ComposedSkeletalPose* out)
{
    if (!out) return false;
    out->valid = false;
    out->bone_count = 0;

    if (!p_thing) return false;
    if (p_thing->Class != CLASS_PERSON) return false;
    if (p_thing->DrawType != DT_ROT_MULTI) return false;

    DrawTween* dt = p_thing->Draw.Tweened;
    if (!dt) return false;
    if (!dt->CurrentFrame || !dt->NextFrame) return false;

    GameKeyFrameElement* ae1 = dt->CurrentFrame->FirstElement;
    GameKeyFrameElement* ae2 = dt->NextFrame->FirstElement;
    if (!ae1 || !ae2) return false;

    if (!dt->TheChunk) return false;
    // Phase 1: hierarchical 15-bone skeleton only. Other DT_TWEEN family
    // (DT_ANIM_PRIM animals/bats, generic DT_TWEEN) use a flat path —
    // handled in a later phase.
    if (dt->TheChunk->ElementCount != 15) return false;

    SLONG character_scale = person_get_scale(p_thing);
    float character_scalef = float(character_scale) / 256.f;
    SLONG tween = dt->AnimTween;

    // Body world rotation R_body. Mirrors FIGURE_draw line 2736: arg order
    // (Tilt, Angle, (Roll+2048)&2047) and FIGURE_rotate_obj scaling — must
    // match exactly because mat_final = R_body × bone_local_rot is what the
    // existing renderer uses. (Note: calc_sub_objects_position uses
    // FMATRIX_calc with different arg order and scale — output of that path
    // diverges slightly from the figure.cpp render path. Composer matches
    // figure.cpp.)
    Matrix33 body_rot;
    FIGURE_rotate_obj(dt->Tilt, dt->Angle, (dt->Roll + 2048) & 2047, &body_rot);

    SLONG world_x = p_thing->WorldPos.X >> 8;
    SLONG world_y = p_thing->WorldPos.Y >> 8;
    SLONG world_z = p_thing->WorldPos.Z >> 8;

    // Per-bone body-local data — equivalent to FIGURE_dhpr_rdata2[].end_pos /
    // .end_mat in the original recurse. Used as parent_curr_pos / parent_curr_mat
    // when composing child bones via HIERARCHY_Get_Body_Part_Offset.
    Matrix31 end_pos[POSE_MAX_BONES];
    Matrix33 end_mat[POSE_MAX_BONES];
    int      parent_of[POSE_MAX_BONES];
    parent_of[0] = -1;

    // DFS stack. Hierarchy is a tree with depth ≤ 4 (PELVIS → TORSO →
    // ARM_HUMORUS → ARM_RADIUS → HAND), so stack depth bounded by 5.
    constexpr int MAX_STACK = 16;
    WalkFrame stack[MAX_STACK];
    Matrix31  parent_base_pos_at_level[MAX_STACK];   // ae1[parent].Offset, captured on descent

    int sp = 0;
    stack[0].part_number = 0;            // PELVIS — root
    stack[0].current_child_number = 0;

    while (sp >= 0) {
        WalkFrame* fr = &stack[sp];
        int part = fr->part_number;

        // First visit to this bone: compute its body-local position/rotation
        // and the corresponding world-space transform output.
        if (fr->current_child_number == 0) {
            Matrix31 bone_offset;

            if (sp == 0) {
                // Root (PELVIS): lerp(CF[0], NF[0], AT) — matches
                // figure_morph_root_offset() formula at figure.cpp:1678.
                // off_dx/dy/dz arguments are 0 here (no muzzle-flash hand
                // offset); composer is invoked outside the gun-flash path.
                bone_offset.M[0] = (SLONG(ae1[part].OffsetX) << 8)
                                 + (SLONG(ae2[part].OffsetX - ae1[part].OffsetX) * tween);
                bone_offset.M[1] = (SLONG(ae1[part].OffsetY) << 8)
                                 + (SLONG(ae2[part].OffsetY - ae1[part].OffsetY) * tween);
                bone_offset.M[2] = (SLONG(ae1[part].OffsetZ) << 8)
                                 + (SLONG(ae2[part].OffsetZ - ae1[part].OffsetZ) * tween);
            } else {
                // Child: HIERARCHY_Get_Body_Part_Offset, mirroring
                // figure.cpp:1851. Inputs:
                //   - rest_offset = ae1[part].Offset (this bone's rest position)
                //   - parent_base_mat = ae1[parent].CMatrix (parent's rest rotation)
                //   - parent_base_pos = ae1[parent].Offset (parent's rest position,
                //     captured during descent in parent_base_pos_at_level[])
                //   - parent_curr_mat = end_mat[parent] (parent's slerp result)
                //   - parent_curr_pos = end_pos[parent] (parent's animated position)
                int parent = parent_of[part];
                Matrix31 rest_off = rest_offset_from_ae1(ae1, part);
                CMatrix33 parent_base_cmat;
                GetCMatrix(&ae1[parent], &parent_base_cmat);
                HIERARCHY_Get_Body_Part_Offset(
                    &bone_offset,
                    &rest_off,
                    &parent_base_cmat,
                    &parent_base_pos_at_level[sp - 1],
                    &end_mat[parent],
                    &end_pos[parent]);
            }
            end_pos[part] = bone_offset;

            // Bone rotation slerp. Mirrors figure_morph_matrix() (figure.cpp:1747).
            // Cross-anim blend branch in figure_morph_matrix is intentionally
            // NOT mirrored — that's a hack the world-pose snapshot replaces
            // (per world_pose_snapshot_plan.md "Что вырезаем"). The composer
            // produces the canonical post-physics pose; render-side smoothing
            // happens by lerping prev/curr composed poses in Phase 3.
            CMatrix33 cm1, cm2;
            GetCMatrix(&ae1[part], &cm1);
            GetCMatrix(&ae2[part], &cm2);
            CQuaternion::BuildTween(&end_mat[part], &cm1, &cm2, tween);

            // World position: WorldPos + R_body × bone_offset, scaled by
            // character_scale. Mirrors figure.cpp:1868-1881 exactly.
            float ox = float(bone_offset.M[0]) / 256.f;
            float oy = float(bone_offset.M[1]) / 256.f;
            float oz = float(bone_offset.M[2]) / 256.f;
            float r00 = float(body_rot.M[0][0]) / 32768.f;
            float r01 = float(body_rot.M[0][1]) / 32768.f;
            float r02 = float(body_rot.M[0][2]) / 32768.f;
            float r10 = float(body_rot.M[1][0]) / 32768.f;
            float r11 = float(body_rot.M[1][1]) / 32768.f;
            float r12 = float(body_rot.M[1][2]) / 32768.f;
            float r20 = float(body_rot.M[2][0]) / 32768.f;
            float r21 = float(body_rot.M[2][1]) / 32768.f;
            float r22 = float(body_rot.M[2][2]) / 32768.f;

            float off_x = ox * r00 + oy * r01 + oz * r02;
            float off_y = ox * r10 + oy * r11 + oz * r12;
            float off_z = ox * r20 + oy * r21 + oz * r22;

            off_x *= character_scalef;
            off_y *= character_scalef;
            off_z *= character_scalef;

            off_x += float(world_x);
            off_y += float(world_y);
            off_z += float(world_z);

            out->bones[part].pos_x = off_x;
            out->bones[part].pos_y = off_y;
            out->bones[part].pos_z = off_z;

            // World rotation: mat_final = R_body × end_mat[part], scaled by
            // character_scale. Mirrors figure.cpp:1894-1905.
            Matrix33 mat_final;
            matrix_mult33(&mat_final, &body_rot, &end_mat[part]);
            mat_final.M[0][0] = (mat_final.M[0][0] * character_scale) / 256;
            mat_final.M[0][1] = (mat_final.M[0][1] * character_scale) / 256;
            mat_final.M[0][2] = (mat_final.M[0][2] * character_scale) / 256;
            mat_final.M[1][0] = (mat_final.M[1][0] * character_scale) / 256;
            mat_final.M[1][1] = (mat_final.M[1][1] * character_scale) / 256;
            mat_final.M[1][2] = (mat_final.M[1][2] * character_scale) / 256;
            mat_final.M[2][0] = (mat_final.M[2][0] * character_scale) / 256;
            mat_final.M[2][1] = (mat_final.M[2][1] * character_scale) / 256;
            mat_final.M[2][2] = (mat_final.M[2][2] * character_scale) / 256;
            out->bones[part].rot = mat_final;

            // Body-local intermediates (no character_scale, no R_body) — used
            // by snapshot capture to derive parent-local representation.
            out->bones[part].body_local_pos = end_pos[part];
            out->bones[part].body_local_rot = end_mat[part];
        }

        // Descend to next child (or backtrack if no more). Mirrors recurse
        // bookkeeping at figure.cpp:2449-2473.
        int child_idx = body_part_children[part][fr->current_child_number];
        if (child_idx != -1) {
            // Capture this bone's rest pose offset for use as parent_base_pos
            // when its child is processed.
            parent_base_pos_at_level[sp] = rest_offset_from_ae1(ae1, part);
            parent_of[child_idx] = part;

            fr->current_child_number++;
            ++sp;
            stack[sp].part_number = child_idx;
            stack[sp].current_child_number = 0;
        } else {
            --sp;
        }
    }

    out->bone_count = 15;
    out->valid = true;
    return true;
}
