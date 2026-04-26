#include "engine/core/quaternion.h"
#include "engine/core/fmatrix.h"
#include "engine/core/math.h"

// 3x3 floating-point matrix used internally during quaternion interpolation.
// uc_orig: FloatMatrix (fallen/DDEngine/Headers/Quaternion.h)
class FloatMatrix {
public:
    float M[3][3];
};

// Threshold for switching from SLERP to linear interpolation (quaternions nearly identical).
// uc_orig: DELTA (fallen/DDEngine/Source/Quaternion.cpp)
#define DELTA 0.05

// Spherical linear interpolation between two quaternions using the shortest arc.
// When quaternions are very close (dot product near 1), falls back to lerp.
// uc_orig: QuatSlerp (fallen/DDEngine/Source/Quaternion.cpp)
static void QuatSlerp(CQuaternion* from, CQuaternion* to, float t, CQuaternion* res)
{
    float to1[4];
    double omega, cosom, sinom, scale0, scale1;

    // Compute dot product to find cosine of angle between quaternions.
    cosom = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;

    // Negate one quaternion if the dot is negative so we take the short arc.
    if (cosom < 0.0) {
        cosom = -cosom;

        to1[0] = -to->x;
        to1[1] = -to->y;
        to1[2] = -to->z;
        to1[3] = -to->w;
    } else {
        to1[0] = to->x;
        to1[1] = to->y;
        to1[2] = to->z;
        to1[3] = to->w;
    }

    if ((1.0 - cosom) > DELTA) {
        // Standard SLERP.
        omega = acos(cosom);
        sinom = sin(omega);
        scale0 = sin((1.0 - t) * omega) / sinom;
        scale1 = sin(t * omega) / sinom;

    } else {
        // Quaternions very close — use linear interpolation.
        scale0 = 1.0 - t;
        scale1 = t;
    }

    res->x = scale0 * from->x + scale1 * to1[0];
    res->y = scale0 * from->y + scale1 * to1[1];
    res->z = scale0 * from->z + scale1 * to1[2];
    res->w = scale0 * from->w + scale1 * to1[3];
}

// Convert quaternion to floating-point 3x3 rotation matrix.
// uc_orig: QuatToMatrix (fallen/DDEngine/Source/Quaternion.cpp)
static void QuatToMatrix(CQuaternion* quat, FloatMatrix* fm)
{
    float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

    x2 = quat->x + quat->x;
    y2 = quat->y + quat->y;
    z2 = quat->z + quat->z;
    xx = quat->x * x2;
    xy = quat->x * y2;
    xz = quat->x * z2;
    yy = quat->y * y2;
    yz = quat->y * z2;
    zz = quat->z * z2;
    wx = quat->w * x2;
    wy = quat->w * y2;
    wz = quat->w * z2;

    fm->M[0][0] = 1.0 - (yy + zz);
    fm->M[1][0] = xy - wz;
    fm->M[2][0] = xz + wy;

    fm->M[0][1] = xy + wz;
    fm->M[1][1] = 1.0 - (xx + zz);
    fm->M[2][1] = yz - wx;

    fm->M[0][2] = xz - wy;
    fm->M[1][2] = yz + wx;
    fm->M[2][2] = 1.0 - (xx + yy);
}

// Convert floating-point 3x3 matrix to quaternion using the trace method.
// Handles the degenerate case (negative diagonal) by finding the largest diagonal element.
// uc_orig: MatrixToQuat (fallen/DDEngine/Source/Quaternion.cpp)
static void MatrixToQuat(FloatMatrix* fm, CQuaternion* quat)
{
    float tr, s, q[4];
    int i, j, k;

    int nxt[3] = { 1, 2, 0 };

    tr = fm->M[0][0] + fm->M[1][1] + fm->M[2][2];

    if (tr > 0.0) {
        s = sqrt(tr + 1.0);
        quat->w = s / 2.0;
        s = 0.5 / s;
        quat->x = (fm->M[1][2] - fm->M[2][1]) * s;
        quat->y = (fm->M[2][0] - fm->M[0][2]) * s;
        quat->z = (fm->M[0][1] - fm->M[1][0]) * s;
    } else {
        // Diagonal is negative — find largest diagonal element.
        i = 0;
        if (fm->M[1][1] > fm->M[0][0])
            i = 1;
        if (fm->M[2][2] > fm->M[i][i])
            i = 2;
        j = nxt[i];
        k = nxt[j];

        s = sqrt((fm->M[i][i] - (fm->M[j][j] + fm->M[k][k])) + 1.0);

        q[i] = s * 0.5;

        if (s != 0.0)
            s = 0.5 / s;

        q[3] = (fm->M[j][k] - fm->M[k][j]) * s;
        q[j] = (fm->M[i][j] + fm->M[j][i]) * s;
        q[k] = (fm->M[i][k] + fm->M[k][i]) * s;

        quat->x = q[0];
        quat->y = q[1];
        quat->z = q[2];
        quat->w = q[3];
    }
}

// Convert a compressed integer matrix (CMatrix33) to a floating-point matrix.
// Each CMatrix33 row packs three 10-bit signed values in a single SLONG.
// Values are in range [-1, 1] encoded as 512 = 1.0; values >= 1.0 are sign-extended by subtracting 2.
// uc_orig: cmat_to_fmat (fallen/DDEngine/Source/Quaternion.cpp)
static void cmat_to_fmat(CMatrix33* cm, FloatMatrix* fm)
{
    fm->M[0][0] = float(((cm->M[0] & CMAT0_MASK) >> 20)) / 512.f;
    fm->M[0][1] = float(((cm->M[0] & CMAT1_MASK) >> 10)) / 512.f;
    fm->M[0][2] = float(((cm->M[0] & CMAT2_MASK) >> 00)) / 512.f;

    fm->M[1][0] = float(((cm->M[1] & CMAT0_MASK) >> 20)) / 512.f;
    fm->M[1][1] = float(((cm->M[1] & CMAT1_MASK) >> 10)) / 512.f;
    fm->M[1][2] = float(((cm->M[1] & CMAT2_MASK) >> 00)) / 512.f;

    fm->M[2][0] = float(((cm->M[2] & CMAT0_MASK) >> 20)) / 512.f;
    fm->M[2][1] = float(((cm->M[2] & CMAT1_MASK) >> 10)) / 512.f;
    fm->M[2][2] = float(((cm->M[2] & CMAT2_MASK) >> 00)) / 512.f;

    // Values >= 1.0 are actually negative (10-bit unsigned used as signed).
    if (fm->M[0][0] >= 1.f)
        fm->M[0][0] -= 2.f;
    if (fm->M[0][1] >= 1.f)
        fm->M[0][1] -= 2.f;
    if (fm->M[0][2] >= 1.f)
        fm->M[0][2] -= 2.f;
    if (fm->M[1][0] >= 1.f)
        fm->M[1][0] -= 2.f;
    if (fm->M[1][1] >= 1.f)
        fm->M[1][1] -= 2.f;
    if (fm->M[1][2] >= 1.f)
        fm->M[1][2] -= 2.f;
    if (fm->M[2][0] >= 1.f)
        fm->M[2][0] -= 2.f;
    if (fm->M[2][1] >= 1.f)
        fm->M[2][1] -= 2.f;
    if (fm->M[2][2] >= 1.f)
        fm->M[2][2] -= 2.f;
}

// Convert floating-point matrix to integer Matrix33 (fixed-point 1.0 = 32768).
// uc_orig: fmat_to_mat (fallen/DDEngine/Source/Quaternion.cpp)
static void fmat_to_mat(FloatMatrix* fm, Matrix33* m)
{
    m->M[0][0] = SLONG(fm->M[0][0] * 32768.f);
    m->M[0][1] = SLONG(fm->M[0][1] * 32768.f);
    m->M[0][2] = SLONG(fm->M[0][2] * 32768.f);

    m->M[1][0] = SLONG(fm->M[1][0] * 32768.f);
    m->M[1][1] = SLONG(fm->M[1][1] * 32768.f);
    m->M[1][2] = SLONG(fm->M[1][2] * 32768.f);

    m->M[2][0] = SLONG(fm->M[2][0] * 32768.f);
    m->M[2][1] = SLONG(fm->M[2][1] * 32768.f);
    m->M[2][2] = SLONG(fm->M[2][2] * 32768.f);
}

// Check whether the matrix rows form a right-handed orthonormal basis.
// Returns UC_TRUE if the cross product of rows 0 and 1 matches row 2 within tolerance.
// A zero first row is treated as a void (identity) matrix and returns UC_TRUE.
// uc_orig: check_isonormal (fallen/DDEngine/Source/Quaternion.cpp)
static BOOL check_isonormal(FloatMatrix& m)
{
    if ((m.M[0][0] == 0) && (m.M[0][1] == 0) && (m.M[0][2] == 0))
        return UC_TRUE;

    float x = m.M[0][1] * m.M[1][2] - m.M[0][2] * m.M[1][1];
    float y = m.M[0][2] * m.M[1][0] - m.M[0][0] * m.M[1][2];
    float z = m.M[0][0] * m.M[1][1] - m.M[0][1] * m.M[1][0];

    if ((fabs(x - m.M[2][0]) > 0.03) || (fabs(y - m.M[2][1]) > 0.03) || (fabs(z - m.M[2][2]) > 0.03))
        return UC_FALSE;

    return UC_TRUE;
}

// Quaternion SLERP via floating-point path.
// Converts compressed matrices to float, checks orthonormality, performs SLERP,
// then converts back to integer matrix. Falls back to linear tween if handedness differs.
// uc_orig: CQuaternion::BuildTween (fallen/DDEngine/Source/Quaternion.cpp)
void CQuaternion::BuildTween(struct Matrix33* dest, struct CMatrix33* cm1, struct CMatrix33* cm2, SLONG tween)
{
    FloatMatrix f1, f2, f3;

    cmat_to_fmat(cm1, &f1);
    cmat_to_fmat(cm2, &f2);

    BOOL a, b;
    a = check_isonormal(f1);
    b = check_isonormal(f2);

    // If both matrices have the same handedness, use quaternion SLERP.
    if (a == b) {
        float t = float(tween) / 256.f;

        CQuaternion q1, q2, q3;

        MatrixToQuat(&f1, &q1);
        MatrixToQuat(&f2, &q2);

        QuatSlerp(&q1, &q2, t, &q3);
        QuatToMatrix(&q3, &f3);

        fmat_to_mat(&f3, dest);

        return;
    }

    // Fallback for mismatched handedness — use linear matrix tween then renormalize.
    build_tween_matrix(dest, cm1, cm2, tween);
    normalise_matrix_rows(dest);
}
