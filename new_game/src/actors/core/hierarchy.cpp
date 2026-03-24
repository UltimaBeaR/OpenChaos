#include "actors/core/hierarchy.h"
#include "actors/core/hierarchy_globals.h"
#include "engine/core/matrix.h"

// uc_orig: uncompress_matrix (fallen/Source/Hierarchy.cpp)
// Decompresses a 10-bit-per-component CMatrix33 into a full Matrix33.
// Each component is sign-extended from 10 bits, then shifted left 6 for fixed-point scale.
static inline void uncompress_matrix(CMatrix33* cm, Matrix33* m)
{
    SLONG v;

    v = ((cm->M[0] & CMAT0_MASK) << 2) >> 22;
    m->M[0][0] = (v << 6);

    v = ((cm->M[0] & CMAT1_MASK) << 12) >> 22;
    m->M[0][1] = (v << 6);

    v = ((cm->M[0] & CMAT2_MASK) << 22) >> 22;
    m->M[0][2] = (v << 6);

    v = ((cm->M[1] & CMAT0_MASK) << 2) >> 22;
    m->M[1][0] = (v << 6);

    v = ((cm->M[1] & CMAT1_MASK) << 12) >> 22;
    m->M[1][1] = (v << 6);

    v = ((cm->M[1] & CMAT2_MASK) << 22) >> 22;
    m->M[1][2] = (v << 6);

    v = ((cm->M[2] & CMAT0_MASK) << 2) >> 22;
    m->M[2][0] = (v << 6);

    v = ((cm->M[2] & CMAT1_MASK) << 12) >> 22;
    m->M[2][1] = (v << 6);

    v = ((cm->M[2] & CMAT2_MASK) << 22) >> 22;
    m->M[2][2] = (v << 6);
}

// uc_orig: HIERARCHY_Get_Body_Part_Offset (fallen/Source/Hierarchy.cpp)
// Computes the animated world position of a child body part given:
//   - base_position: rest-pose world position of this bone
//   - parent_base_matrix/position: rest-pose transform of the parent
//   - parent_curr_matrix/position: current animated transform of the parent
// Result in dest_position is in the same coordinate space as parent_curr_position.
void HIERARCHY_Get_Body_Part_Offset(
    Matrix31* dest_position,
    Matrix31* base_position,
    CMatrix33* parent_base_matrix,
    Matrix31* parent_base_position,
    Matrix33* parent_curr_matrix,
    Matrix31* parent_curr_position)
{
    struct Matrix33 pmatq, pmati;

    uncompress_matrix(parent_base_matrix, &pmati);
    pmatq = *parent_curr_matrix;

    // Invert the base parent matrix by transposing (rotation matrices are orthogonal).
    SWAP(pmati.M[0][1], pmati.M[1][0]);
    SWAP(pmati.M[0][2], pmati.M[2][0]);
    SWAP(pmati.M[2][1], pmati.M[1][2]);

    struct Matrix31 i, o;
    o.M[0] = (base_position->M[0] - parent_base_position->M[0]) * 256;
    o.M[1] = (base_position->M[1] - parent_base_position->M[1]) * 256;
    o.M[2] = (base_position->M[2] - parent_base_position->M[2]) * 256;

    // Transform offset into local space using the inverse rest-pose parent matrix.
    matrix_transformZMY(&i, &pmati, &o);

    // Then transform back into world space using the current animated parent matrix.
    matrix_transformZMY(dest_position, &pmatq, &i);

    dest_position->M[0] += parent_curr_position->M[0];
    dest_position->M[1] += parent_curr_position->M[1];
    dest_position->M[2] += parent_curr_position->M[2];
}
