#ifndef CORE_HEAP_GLOBALS_H
#define CORE_HEAP_GLOBALS_H

// Global state for the heap module.

#include "core/types.h"
#include "core/heap.h"

// uc_orig: HEAP_pad (fallen/Source/heap.cpp)
extern UBYTE HEAP_pad[HEAP_PAD_SIZE];

// uc_orig: HEAP_heap (fallen/Source/heap.cpp)
#define HEAP_SIZE (1024 * 128)
extern UBYTE HEAP_heap[HEAP_SIZE];

// Internal free list node type — forward declared here for the _globals .cpp only.
struct HEAP_Free;

// uc_orig: HEAP_free (fallen/Source/heap.cpp)
extern struct HEAP_Free* HEAP_free;

#endif // CORE_HEAP_GLOBALS_H
