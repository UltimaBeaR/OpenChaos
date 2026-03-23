#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include "core/types.h"

// BOOL comes from Windows headers (windef.h). Temporary dependency until Étape 8.
#include <windef.h>

// Custom heap allocator. All allocations are 4-byte aligned and zero-initialized.

// uc_orig: SetupMemory (MFStdLib/Source/StdLib/StdMem.cpp)
BOOL SetupMemory(void);

// uc_orig: ResetMemory (MFStdLib/Source/StdLib/StdMem.cpp)
void ResetMemory(void);

// uc_orig: MemAlloc (MFStdLib/Source/StdLib/StdMem.cpp)
void* MemAlloc(ULONG size);

// uc_orig: MemReAlloc (MFStdLib/Source/StdLib/StdMem.cpp)
void* MemReAlloc(void* ptr, ULONG size);

// uc_orig: MemFree (MFStdLib/Source/StdLib/StdMem.cpp)
void MemFree(void* mem_ptr);

// Typed allocation wrapper.
// uc_orig: MFnew (MFStdLib/Headers/StdMem.h)
template <class T>
T* MFnew(void)
{
    T* ptr = new T;
    return ptr;
}

// Typed deallocation wrapper.
// uc_orig: MFdelete (MFStdLib/Headers/StdMem.h)
template <class T>
void MFdelete(T* thing)
{
    delete thing;
}

#endif // CORE_MEMORY_H
