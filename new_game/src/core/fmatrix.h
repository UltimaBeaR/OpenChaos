#ifndef CORE_FMATRIX_H
#define CORE_FMATRIX_H

#include "core/types.h"
#include "core/macros.h"
#include "core/fixed_math.h"

// Integer 3x3 matrix operations using PSX angle system (0-2047 = full rotation).
// Fixed-point 16.16: 65536 = 1.0. Uses SIN/COS lookup tables and MUL64.

// Bit field masks for the compressed CMatrix33 format.
// Each SLONG in CMatrix33.M[] packs three 10-bit signed values.
// uc_orig: CMAT0_MASK (fallen/Headers/prim.h)
#define CMAT0_MASK (0x3ff00000)
// uc_orig: CMAT1_MASK (fallen/Headers/prim.h)
#define CMAT1_MASK (0x000ffc00)
// uc_orig: CMAT2_MASK (fallen/Headers/prim.h)
#define CMAT2_MASK (0x000003ff)

// Compressed 3x3 integer matrix: each SLONG packs three 10-bit values via CMAT masks.
// uc_orig: CMatrix33 (fallen/Headers/prim.h)
struct CMatrix33 {
    SLONG M[3];
};

// Full 3x3 integer matrix, 15-bit fraction (elements in range [-32768, 32767]).
// uc_orig: Matrix33 (fallen/Headers/prim.h)
struct Matrix33 {
    SLONG M[3][3];
};

// 3x1 integer column vector (32-bit components).
// uc_orig: Matrix31 (fallen/Headers/prim.h)
struct Matrix31 {
    SLONG M[3];
};

// 3x1 short column vector (16-bit components).
// uc_orig: SMatrix31 (fallen/Headers/prim.h)
struct SMatrix31 {
    SWORD M[3];
};

// Build rotation matrix from PSX Euler angles.
// uc_orig: FMATRIX_calc (fallen/Headers/FMatrix.h)
void FMATRIX_calc(SLONG matrix[9], SLONG yaw, SLONG pitch, SLONG roll);

// Build direction vector from PSX yaw and pitch.
// uc_orig: FMATRIX_vector (fallen/Headers/FMatrix.h)
void FMATRIX_vector(SLONG vector[3], SLONG yaw, SLONG pitch);

// Extract PSX Euler angles from matrix.
// uc_orig: FMATRIX_find_angles (fallen/Headers/FMatrix.h)
void FMATRIX_find_angles(SLONG matrix[9], SLONG* yaw, SLONG* pitch, SLONG* roll);

// uc_orig: build_tween_matrix (fallen/Headers/FMatrix.h)
void build_tween_matrix(struct Matrix33* mat, struct CMatrix33* cmat1, struct CMatrix33* cmat2, SLONG tween);
// uc_orig: init_matrix33 (fallen/Headers/FMatrix.h)
void init_matrix33(struct Matrix33* mat);
// uc_orig: matrix_transform (fallen/Headers/FMatrix.h)
void matrix_transform(struct Matrix31* result, struct Matrix33* trans, struct Matrix31* mat2);
// uc_orig: matrix_transform_small (fallen/Headers/FMatrix.h)
void matrix_transform_small(struct Matrix31* result, struct Matrix33* trans, struct SMatrix31* mat2);
// uc_orig: matrix_transformZMY (fallen/Headers/FMatrix.h)
void matrix_transformZMY(Matrix31* result, Matrix33* trans, Matrix31* mat2);
// uc_orig: normalise_matrix (fallen/Headers/FMatrix.h)
void normalise_matrix(struct Matrix33* mat);
// uc_orig: normalise_matrix_rows (fallen/Headers/FMatrix.h)
void normalise_matrix_rows(struct Matrix33* mat);

// Multiply two 3x3 integer matrices (15-bit fraction, result >>15).
// uc_orig: matrix_mult33 (fallen/Source/maths.cpp)
void matrix_mult33(Matrix33* result, Matrix33* mat1, Matrix33* mat2);

// Build 3x3 rotation matrix from three Euler angles (0-2047 = full rotation, SIN/COS tables).
// uc_orig: rotate_obj (fallen/Source/maths.cpp)
void rotate_obj(SWORD xangle, SWORD yangle, SWORD zangle, Matrix33* r3);

// Transform point (x,y,z) by integer matrix m using MUL64 (16.16 fixed-point multiply).
// uc_orig: FMATRIX_MUL (fallen/Headers/FMatrix.h)
#define FMATRIX_MUL(m, x, y, z)     \
    {                               \
        SLONG xnew, ynew, znew;     \
                                    \
        xnew = MUL64((x), (m)[0]);  \
        ynew = MUL64((x), (m)[3]);  \
        znew = MUL64((x), (m)[6]);  \
                                    \
        xnew += MUL64((y), (m)[1]); \
        ynew += MUL64((y), (m)[4]); \
        znew += MUL64((y), (m)[7]); \
                                    \
        xnew += MUL64((z), (m)[2]); \
        ynew += MUL64((z), (m)[5]); \
        znew += MUL64((z), (m)[8]); \
                                    \
        (x) = xnew;                 \
        (y) = ynew;                 \
        (z) = znew;                 \
    }

// Transform by transpose of integer matrix.
// uc_orig: FMATRIX_MUL_BY_TRANSPOSE (fallen/Headers/FMatrix.h)
#define FMATRIX_MUL_BY_TRANSPOSE(m, x, y, z) \
    {                                        \
        SLONG xnew, ynew, znew;              \
                                             \
        xnew = MUL64((x), (m)[0]);           \
        ynew = MUL64((x), (m)[1]);           \
        znew = MUL64((x), (m)[2]);           \
                                             \
        xnew += MUL64((y), (m)[3]);          \
        ynew += MUL64((y), (m)[4]);          \
        znew += MUL64((y), (m)[5]);          \
                                             \
        xnew += MUL64((z), (m)[6]);          \
        ynew += MUL64((z), (m)[7]);          \
        znew += MUL64((z), (m)[8]);          \
                                             \
        (x) = xnew;                          \
        (y) = ynew;                          \
        (z) = znew;                          \
    }

// Transpose integer matrix in-place.
// uc_orig: FMATRIX_TRANSPOSE (fallen/Headers/FMatrix.h)
#define FMATRIX_TRANSPOSE(m) \
    {                        \
        SWAP(m[1], m[3]);    \
        SWAP(m[2], m[6]);    \
        SWAP(m[5], m[7]);    \
    }

#endif // CORE_FMATRIX_H
