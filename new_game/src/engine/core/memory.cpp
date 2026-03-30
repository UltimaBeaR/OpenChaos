#include "engine/core/memory.h"
#include <stdlib.h>

// uc_orig: SetupMemory (MFStdLib/Source/StdLib/StdMem.cpp)
// Originally created a Win32 private heap. Now a no-op — standard allocator is used.
BOOL SetupMemory(void)
{
    return UC_TRUE;
}

// uc_orig: ResetMemory (MFStdLib/Source/StdLib/StdMem.cpp)
void ResetMemory(void)
{
}

// Allocations are 4-byte aligned and zero-initialized.
// uc_orig: MemAlloc (MFStdLib/Source/StdLib/StdMem.cpp)
void* MemAlloc(ULONG size)
{
    size = (size + 3) & 0xfffffffc;
    return calloc(1, size);
}

// uc_orig: MemReAlloc (MFStdLib/Source/StdLib/StdMem.cpp)
// Note: Win32 HeapReAlloc with HEAP_ZERO_MEMORY zeroed new bytes; realloc does not.
// Currently unused in the codebase — kept for API compatibility.
void* MemReAlloc(void* ptr, ULONG size)
{
    size = (size + 3) & 0xfffffffc;
    return realloc(ptr, size);
}

// uc_orig: MemFree (MFStdLib/Source/StdLib/StdMem.cpp)
void MemFree(void* mem_ptr)
{
    free(mem_ptr);
}
