#ifndef ACTORS_CORE_HIERARCHY_H
#define ACTORS_CORE_HIERARCHY_H

#include "engine/core/types.h"
#include "engine/core/fmatrix.h"

// Bone hierarchy data for skeletal animation.
// 15 body parts: pelvis, leg joints, torso, arm joints, skull.

// uc_orig: body_part_parent (fallen/Headers/Hierarchy.h)
// Name pairs: [bone_name, parent_bone_name]. Root bone has empty parent string.
extern CBYTE* body_part_parent[][2];

// uc_orig: body_part_parent_numbers (fallen/Headers/Hierarchy.h)
// Index of the parent bone for each bone. -1 = root (no parent).
extern SLONG body_part_parent_numbers[];

// uc_orig: body_part_children (fallen/Headers/Hierarchy.h)
// Up to 5 children per bone. Terminated by -1.
extern SLONG body_part_children[][5];

// uc_orig: HIERARCHY_Get_Body_Part_Offset (fallen/Headers/Hierarchy.h)
// Computes the world position of a body part given the parent's base and current transform.
// Accounts for the difference between rest pose and animated pose.
void HIERARCHY_Get_Body_Part_Offset(
    Matrix31* dest_position,
    Matrix31* base_position,
    CMatrix33* parent_base_matrix,
    Matrix31* parent_base_position,
    Matrix33* parent_curr_matrix,
    Matrix31* parent_curr_position);

#endif // ACTORS_CORE_HIERARCHY_H
