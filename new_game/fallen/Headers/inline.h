#ifndef INLINE_H
#define INLINE_H



#ifdef __WINDOWS_386__
SLONG MUL64(SLONG, SLONG);
#pragma aux MUL64 = "	imul	ebx				"  \
                    "	mov		ax,dx			" \
                    "	rol		eax,16			" parm[eax][ebx] modify[eax ebx edx] value[eax]

#pragma aux DIV64 = "   mov     eax,edx         " \
                    "   shl     eax,16          " \
                    "   sar     edx,16          " \
                    "   idiv    ebx             " parm[edx][ebx] modify[eax ebx edx] value[eax]

#else

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
#endif


#endif
