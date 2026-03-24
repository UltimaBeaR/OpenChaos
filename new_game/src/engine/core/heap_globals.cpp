#include "engine/core/heap_globals.h"

// uc_orig: HEAP_pad (fallen/Source/heap.cpp)
UBYTE HEAP_pad[HEAP_PAD_SIZE];

// uc_orig: HEAP_heap (fallen/Source/heap.cpp)
UBYTE HEAP_heap[HEAP_SIZE];

// uc_orig: HEAP_free (fallen/Source/heap.cpp)
struct HEAP_Free* HEAP_free;
