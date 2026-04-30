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

    // Cross-anim blend: triple SLERP composing two pose-pair tweens with a
    // wall-clock blend factor. Used by render-interp's variant-A1 cross-fade
    // to mix old anim's pose (snapped at transition: old_cm1, old_cm2, old_tween)
    // with new anim's live pose (new_cm1, new_cm2, new_tween) at fraction blend_t.
    // blend_t in [0, 1]: 0 = pure old, 1 = pure new. tweens are 0..256.
    // No counterpart in original Urban Chaos — added for OpenChaos render-side blend.
    static void BuildBlendTween(
        struct Matrix33* dest,
        struct CMatrix33* old_cm1, struct CMatrix33* old_cm2, SLONG old_tween,
        struct CMatrix33* new_cm1, struct CMatrix33* new_cm2, SLONG new_tween,
        float blend_t);
};

#endif // ENGINE_CORE_QUATERNION_H
