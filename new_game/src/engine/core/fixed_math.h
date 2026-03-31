#ifndef ENGINE_CORE_FIXED_MATH_H
#define ENGINE_CORE_FIXED_MATH_H

#include <cstdint>
#include "engine/core/types.h"

// 16.16 fixed-point arithmetic.
// MUL64: (a * b) >> 16
// DIV64: (a << 16) / b
// uc_orig: MUL64, DIV64 (fallen/Headers/inline.h) — were inline x86 asm, replaced with C.

inline SLONG MUL64(SLONG a, SLONG b)
{
    return (SLONG)(((int64_t)a * b) >> 16);
}

inline SLONG DIV64(SLONG a, SLONG b)
{
    return (SLONG)(((int64_t)a << 16) / b);
}

#endif // ENGINE_CORE_FIXED_MATH_H
