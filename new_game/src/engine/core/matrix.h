#ifndef ENGINE_CORE_MATRIX_H
#define ENGINE_CORE_MATRIX_H

#include "engine/core/types.h"
#include "engine/core/macros.h"

// 3x3 float matrix operations. All angles in radians.
// Row-major float[9] layout.

// Build rotation matrix from Euler angles (yaw, pitch, roll).
// uc_orig: MATRIX_calc (fallen/DDEngine/Headers/Matrix.h)
void MATRIX_calc(float matrix[9], float yaw, float pitch, float roll);

// Build direction vector from yaw and pitch.
// uc_orig: MATRIX_vector (fallen/DDEngine/Headers/Matrix.h)
void MATRIX_vector(float vector[3], float yaw, float pitch);

// Apply aspect ratio skew, zoom, and global scale to a view matrix.
// uc_orig: MATRIX_skew (fallen/DDEngine/Headers/Matrix.h)
void MATRIX_skew(float matrix[9], float skew, float zoom, float scale);

// Transform point (x,y,z) by matrix m in-place.
// uc_orig: MATRIX_MUL (fallen/DDEngine/Headers/Matrix.h)
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

// Integer version: result = (x*m) >> 16.
// uc_orig: MATRIX_MUL_INT (fallen/DDEngine/Headers/Matrix.h)
#define MATRIX_MUL_INT(m, x, y, z) \
    {                              \
        SLONG xnew, ynew, znew;    \
                                   \
        xnew = (x) * (m)[0];       \
        ynew = (x) * (m)[3];       \
        znew = (x) * (m)[6];       \
                                   \
        xnew += (y) * (m)[1];      \
        ynew += (y) * (m)[4];      \
        znew += (y) * (m)[7];      \
                                   \
        xnew += (z) * (m)[2];      \
        ynew += (z) * (m)[5];      \
        znew += (z) * (m)[8];      \
                                   \
        (x) = xnew >> 16;          \
        (y) = ynew >> 16;          \
        (z) = znew >> 16;          \
    }

// Transform point by transpose of matrix m.
// uc_orig: MATRIX_MUL_BY_TRANSPOSE (fallen/DDEngine/Headers/Matrix.h)
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

// Multiply two 3x3 matrices: a = m * n.
// uc_orig: MATRIX_3x3mul (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_3x3mul(float a[9], float m[9], float n[9]);

// Transpose matrix in-place.
// uc_orig: MATRIX_TRANSPOSE (fallen/DDEngine/Headers/Matrix.h)
#define MATRIX_TRANSPOSE(m)  \
    {                        \
        SWAP_FL(m[1], m[3]); \
        SWAP_FL(m[2], m[6]); \
        SWAP_FL(m[5], m[7]); \
    }

// Yaw/pitch/roll angles extracted from a rotation matrix.
// uc_orig: Direction (fallen/DDEngine/Headers/Matrix.h)
typedef struct
{
    float yaw;
    float pitch;
    float roll;
} Direction;

// Extract Euler angles from rotation matrix.
// uc_orig: MATRIX_find_angles (fallen/DDEngine/Source/Matrix.cpp)
Direction MATRIX_find_angles(float matrix[9]);

#endif // ENGINE_CORE_MATRIX_H
