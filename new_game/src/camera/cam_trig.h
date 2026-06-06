#ifndef CAMERA_CAM_TRIG_H
#define CAMERA_CAM_TRIG_H

#include "engine/core/types.h"
#include <cmath>

// High-precision atan2 for camera-only use.
//
// `FC_look_at_focus` derives the camera's want_yaw / want_pitch from the
// vector between camera and focus. The engine-wide `Arctan(X, Y)` returns
// an integer PSX angle (0-2047), which the caller then shifts `<<8` into
// FC_Cam's 11.8 fixed-point yaw/pitch storage — losing the bottom 8 bits
// to the round-to-integer step.
//
// At a typical orbit radius (~10^5 fc-units) per-tick rotation produces
// only ~10-60 PSX units of want_yaw change; the integer round picks one
// of two adjacent values inconsistently between ticks → ±1 PSX unit
// (= ±256 in the 11.8 scale) jitter on steady camera rotation. The
// snap-mode angle smoothing (MANUAL mode, or AUTO at rubberness 0)
// copies that jittery want_yaw into fc.yaw and the render interpolator
// amplifies the per-tick rate variation into visible micro-jerks.
//
// `cam_arctan_psx_fp8` uses `std::atan2` and rounds directly to the
// 11.8 PSX format — no `<<8` shift on the caller side, no bottom-bit
// loss. Effective precision ~1/256 of a PSX unit. Eliminates the
// observed micro-jerks.
//
// SIN / COS table lookups (engine-wide `SIN(angle)` / `COS(angle)`)
// are left untouched: they're already 16-bit fixed-point, which is
// indistinguishable from `std::sin(...) * 65536` after the rounding
// step, AND their callers downstream pass through position smoothing
// (>>2) which dwarfs sub-unit precision differences anyway.
//
// Performance: called once per camera per tick (20 Hz) — ~50 ns
// overhead, negligible.
//
// Determinism note: `std::atan2` is not guaranteed bit-identical
// across compilers / libc. OpenChaos has no replay or networked play,
// so this is acceptable.

// High-precision atan2 returning the angle in 11.8 fixed-point PSX
// format (range [0, 2048*256), where 256 == 1 PSX game-angle unit).
//
// Convention (same as the engine's `Arctan`): at integer PSX scale,
// 0 = −Z axis (X=0, Y<0), 512 = +X (X>0, Y=0), 1024 = +Z (X=0, Y>0),
// 1536 = −X (X<0, Y=0). The FP8 output multiplies these by 256.
static inline SLONG cam_arctan_psx_fp8(SLONG X, SLONG Y)
{
    if (X == 0 && Y == 0)
        return 0;

    // std::atan2(X, Y) here treats X as the "y-coord" and Y as the
    // "x-coord" — matches the engine's `Arctan(X, Y)` argument order.
    // Result is in (-π, +π], with 0 at the +Y axis.
    static constexpr double CAM_PI = 3.14159265358979323846;
    const double r = std::atan2((double)X, (double)Y);

    // Map (-π, +π] → 11.8 PSX [0, 2048*256):
    //   PSX_fp8 = 1024*256 - r * (1024*256 / π).
    static constexpr long YAW_FP8_RANGE = 2048L * 256L;
    static constexpr double FP8_PER_HALF_TURN = 1024.0 * 256.0;
    long psx_fp8 = (long)(1024L * 256L)
        - (long)std::lround(r * (FP8_PER_HALF_TURN / CAM_PI));

    // Wrap to [0, YAW_FP8_RANGE). C `%` can produce a negative result
    // for negative dividends, hence the `+ range) % range` idiom.
    psx_fp8 = ((psx_fp8 % YAW_FP8_RANGE) + YAW_FP8_RANGE) % YAW_FP8_RANGE;
    return (SLONG)psx_fp8;
}

#endif // CAMERA_CAM_TRIG_H
