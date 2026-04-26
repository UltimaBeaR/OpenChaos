#ifndef ENGINE_CORE_QUATERNION_H
#define ENGINE_CORE_QUATERNION_H

#include "engine/core/types.h"

struct Matrix33;
struct CMatrix33;

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

#endif // ENGINE_CORE_QUATERNION_H
