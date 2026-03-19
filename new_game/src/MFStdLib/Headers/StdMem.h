// StdMem.h
// Guy Simmons, 18th December 1997

#ifndef MFSTDLIB_HEADERS_STDMEM_H
#define MFSTDLIB_HEADERS_STDMEM_H

//---------------------------------------------------------------

BOOL SetupMemory(void);
void ResetMemory(void);
void* MemAlloc(ULONG size);
void* MemReAlloc(void* ptr, ULONG size);
void MemFree(void* mem_ptr);

// Some templated new and delete stand-ins.
template <class T>
T* MFnew(void)
{
    T* ptr = new T;
    return ptr;
}

template <class T>
void MFdelete(T* thing)
{
    delete thing;
}

//---------------------------------------------------------------

#endif // MFSTDLIB_HEADERS_STDMEM_H
