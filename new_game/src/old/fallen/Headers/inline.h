#ifndef FALLEN_HEADERS_INLINE_H
#define FALLEN_HEADERS_INLINE_H

inline SLONG DIV64(SLONG a, SLONG b)
{
    /* 64bit divide */
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

inline SLONG MUL64(SLONG a, SLONG b)
{
    /* 64bit multiply */
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

#endif // FALLEN_HEADERS_INLINE_H
