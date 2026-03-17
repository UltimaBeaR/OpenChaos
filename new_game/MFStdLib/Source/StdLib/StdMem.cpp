// StdMem.cpp
// Guy Simmons, 18th December 1997

#include <MFStdLib.h>

#define INITIAL_HEAP_SIZE (8 * 1024 * 1024)
#define MAXIMUM_HEAP_SIZE 0

HANDLE MFHeap = NULL;

//---------------------------------------------------------------

BOOL SetupMemory(void)
{
    if (MFHeap == NULL) {
        MFHeap = HeapCreate(0, INITIAL_HEAP_SIZE, MAXIMUM_HEAP_SIZE);
    }
    if (MFHeap)
        return TRUE;
    else
        return FALSE;
}

//---------------------------------------------------------------

void ResetMemory(void)
{
    if (MFHeap) {
        HeapDestroy(MFHeap);
        MFHeap = NULL;
    }
}

//---------------------------------------------------------------

void* MemAlloc(ULONG size)
{
    size = (size + 3) & 0xfffffffc;
    void* ptr = (void*)HeapAlloc(MFHeap, HEAP_ZERO_MEMORY, size);
    ASSERT(ptr != NULL);

    return ptr;
}

void* MemReAlloc(void* ptr, ULONG size)
{
    size = (size + 3) & 0xfffffffc;
    ptr = (void*)HeapReAlloc(MFHeap, HEAP_ZERO_MEMORY, ptr, size);
    ASSERT(ptr != NULL);

    return ptr;
}

//---------------------------------------------------------------

void MemFree(void* mem_ptr)
{
    HeapFree(MFHeap, 0, mem_ptr);
}

//---------------------------------------------------------------

/*
void	MemClear(void *mem_ptr,ULONG size)
{
}
*/

//---------------------------------------------------------------

