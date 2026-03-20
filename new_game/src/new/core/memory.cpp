#include <windows.h>
#include "core/memory.h"
#include "core/memory_globals.h"

// uc_orig: INITIAL_HEAP_SIZE (MFStdLib/Source/StdLib/StdMem.cpp)
#define INITIAL_HEAP_SIZE (8 * 1024 * 1024)

// uc_orig: MAXIMUM_HEAP_SIZE (MFStdLib/Source/StdLib/StdMem.cpp)
#define MAXIMUM_HEAP_SIZE 0

// uc_orig: SetupMemory (MFStdLib/Source/StdLib/StdMem.cpp)
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

// uc_orig: ResetMemory (MFStdLib/Source/StdLib/StdMem.cpp)
void ResetMemory(void)
{
    if (MFHeap) {
        HeapDestroy(MFHeap);
        MFHeap = NULL;
    }
}

// Allocations are 4-byte aligned and zero-initialized.
// uc_orig: MemAlloc (MFStdLib/Source/StdLib/StdMem.cpp)
void* MemAlloc(ULONG size)
{
    size = (size + 3) & 0xfffffffc;
    void* ptr = (void*)HeapAlloc(MFHeap, HEAP_ZERO_MEMORY, size);
    return ptr;
}

// uc_orig: MemReAlloc (MFStdLib/Source/StdLib/StdMem.cpp)
void* MemReAlloc(void* ptr, ULONG size)
{
    size = (size + 3) & 0xfffffffc;
    ptr = (void*)HeapReAlloc(MFHeap, HEAP_ZERO_MEMORY, ptr, size);
    return ptr;
}

// uc_orig: MemFree (MFStdLib/Source/StdLib/StdMem.cpp)
void MemFree(void* mem_ptr)
{
    HeapFree(MFHeap, 0, mem_ptr);
}
