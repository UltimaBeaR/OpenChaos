#ifndef ENGINE_CORE_FIXED_MATH_H
#define ENGINE_CORE_FIXED_MATH_H

#include "engine/core/types.h"

// 16.16 fixed-point arithmetic via inline x86 assembly.
// MUL64: (a * b) >> 16
// DIV64: (a << 16) / b

// uc_orig: DIV64 (fallen/Headers/inline.h)
inline SLONG DIV64(SLONG a, SLONG b)
{
    SLONG ret_v;
    __asm
    {
		 mov	edx,a
		 mov	ebx,b
		 mov	eax,edx
		 shl	eax,16
		 sar	edx,16
		 idiv	ebx
		 mov	ret_v,eax
    }
    return (ret_v);
}

// uc_orig: MUL64 (fallen/Headers/inline.h)
inline SLONG MUL64(SLONG a, SLONG b)
{
    SLONG ret_v;
    __asm
    {
		 mov	eax,a
		 imul   b
		 mov	ax,dx
		 rol	eax,16
		 mov	ret_v,eax
    }
    return (ret_v);
}

#endif // ENGINE_CORE_FIXED_MATH_H
