#ifndef OUTRO_CORE_OUTRO_MATRIX_H
#define OUTRO_CORE_OUTRO_MATRIX_H

// 3x3 rotation matrix utilities for the outro/credits sequence.
// The outro uses a flat float[9] layout, row-major. All angles are in radians.

#include "outro/core/outro_always.h"

// Fills matrix with a rotation for a camera looking along the Z axis,
// parameterised by yaw, pitch, and roll.
// uc_orig: MATRIX_calc (fallen/outro/outroMatrix.cpp)
void MATRIX_calc(float matrix[9], float yaw, float pitch, float roll);

// Constructs a rotation matrix for angle radians about the given unit vector (ux, uy, uz).
// uc_orig: MATRIX_calc_arb (fallen/outro/outroMatrix.cpp)
void MATRIX_calc_arb(
    float matrix[9],
    float ux, float uy, float uz,
    float angle);

// Computes the direction vector (yaw, pitch) from a 3-element float array.
// uc_orig: MATRIX_vector (fallen/outro/outroMatrix.cpp)
void MATRIX_vector(float vector[3], float yaw, float pitch);

// Adjusts matrix so the x/y vectors are scaled by zoom and the whole
// matrix is multiplied by scale. Skew is 1/(horiz_res/vert_res).
// uc_orig: MATRIX_skew (fallen/outro/outroMatrix.cpp)
void MATRIX_skew(float matrix[9], float skew, float zoom, float scale);

// Multiplies the point (x, y, z) by 3x3 matrix m in-place.
// uc_orig: MATRIX_MUL (fallen/outro/Matrix.h)
#define MATRIX_MUL(m, x, y, z)  \
    {                           \
        float xnew, ynew, znew; \
                                \
        xnew = (x) * (m)[0];    \
        ynew = (x) * (m)[3];    \
        znew = (x) * (m)[6];    \
                                \
        xnew += (y) * (m)[1];   \
        ynew += (y) * (m)[4];   \
        znew += (y) * (m)[7];   \
                                \
        xnew += (z) * (m)[2];   \
        ynew += (z) * (m)[5];   \
        znew += (z) * (m)[8];   \
                                \
        (x) = xnew;             \
        (y) = ynew;             \
        (z) = znew;             \
    }

// Multiplies the point (x, y, z) by the transpose of 3x3 matrix m in-place.
// uc_orig: MATRIX_MUL_BY_TRANSPOSE (fallen/outro/Matrix.h)
#define MATRIX_MUL_BY_TRANSPOSE(m, x, y, z) \
    {                                       \
        float xnew, ynew, znew;             \
                                            \
        xnew = (x) * (m)[0];                \
        ynew = (x) * (m)[1];                \
        znew = (x) * (m)[2];                \
                                            \
        xnew += (y) * (m)[3];               \
        ynew += (y) * (m)[4];               \
        znew += (y) * (m)[5];               \
                                            \
        xnew += (z) * (m)[6];               \
        ynew += (z) * (m)[7];               \
        znew += (z) * (m)[8];               \
                                            \
        (x) = xnew;                         \
        (y) = ynew;                         \
        (z) = znew;                         \
    }

// Computes A = M * N (3x3 matrix multiply).
// uc_orig: MATRIX_3x3mul (fallen/outro/outroMatrix.cpp)
void MATRIX_3x3mul(float a[9], float m[9], float n[9]);

// Transposes a 3x3 matrix in-place.
// uc_orig: MATRIX_TRANSPOSE (fallen/outro/Matrix.h)
#define MATRIX_TRANSPOSE(m)  \
    {                        \
        SWAP_FL(m[1], m[3]); \
        SWAP_FL(m[2], m[6]); \
        SWAP_FL(m[5], m[7]); \
    }

// Scales all elements of the matrix by mulby.
// uc_orig: MATRIX_scale (fallen/outro/outroMatrix.cpp)
void MATRIX_scale(float matrix[9], float mulby);

// Builds an orthonormal matrix from a direction vector at [6][7][8].
// uc_orig: MATRIX_construct (fallen/outro/outroMatrix.cpp)
void MATRIX_construct(float matrix[9], float dx, float dy, float dz);

#endif // OUTRO_CORE_OUTRO_MATRIX_H
