#ifndef CORE_QUATERNION_H
#define CORE_QUATERNION_H

#include "core/types.h"

struct Matrix33;
struct CMatrix33;

// 3x3 floating-point matrix used during quaternion interpolation.
// uc_orig: FloatMatrix (fallen/DDEngine/Headers/Quaternion.h)
class FloatMatrix {
public:
    float M[3][3];
};

// Unit quaternion (w, x, y, z) used for smooth rotation interpolation (SLERP).
// uc_orig: CQuaternion (fallen/DDEngine/Headers/Quaternion.h)
class CQuaternion {
public:
    float w;
    float x, y, z;

    // Interpolate between two compressed integer matrices using quaternion SLERP.
    // tween is 0-256 where 0 = cm1 and 256 = cm2.
    // uc_orig: CQuaternion::BuildTween (fallen/DDEngine/Headers/Quaternion.h)
    static void BuildTween(struct Matrix33* dest, struct CMatrix33* cm1, struct CMatrix33* cm2, SLONG tween);
};

// Spherical linear interpolation between two quaternions.
// t is [0.0, 1.0] where 0 = from and 1 = to.
// uc_orig: QuatSlerp (fallen/DDEngine/Source/Quaternion.cpp)
void QuatSlerp(CQuaternion* from, CQuaternion* to, float t, CQuaternion* res);

// Convert quaternion to 3x3 floating-point matrix.
// uc_orig: QuatToMatrix (fallen/DDEngine/Source/Quaternion.cpp)
void QuatToMatrix(CQuaternion* quat, FloatMatrix* fm);

// Integer SLERP path: interpolate two compressed matrices via integer quaternions.
// tween is 0-256 where 0 = cm1 and 256 = cm2.
// uc_orig: QUATERNION_BuildTweenInteger (fallen/DDEngine/Source/Quaternion.cpp)
void QUATERNION_BuildTweenInteger(struct Matrix33* dest, struct CMatrix33* cm1, struct CMatrix33* cm2, SLONG tween);

#endif // CORE_QUATERNION_H
