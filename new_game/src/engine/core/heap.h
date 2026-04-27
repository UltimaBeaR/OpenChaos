#ifndef ENGINE_CORE_HEAP_H
#define ENGINE_CORE_HEAP_H

// A fixed-size memory heap with defragmentation.
// Designed for allocating and freeing large blocks (minimum 16 bytes, aligned to 16).
// All allocation comes from a statically sized internal pool (HEAP_SIZE = 128 KB).

#include "engine/core/types.h"

// uc_orig: HEAP_PAD_SIZE (fallen/Headers/heap.h)
#define HEAP_PAD_SIZE (1024 * 4)

// A general-purpose scratch buffer — the caller may use it freely between frames.
// uc_orig: HEAP_pad (fallen/Headers/heap.h)
extern UBYTE HEAP_pad[HEAP_PAD_SIZE];

// Initialises the heap free list. Must be called before any HEAP_get/HEAP_give calls.
// uc_orig: HEAP_init (fallen/Source/heap.cpp)
void HEAP_init(void);

// Allocates at least num_bytes from the heap (rounded up to 16-byte boundary).
// Returns NULL if no sufficiently large block is free.
// uc_orig: HEAP_get (fallen/Source/heap.cpp)
void* HEAP_get(SLONG num_bytes);

// Returns num_bytes of memory at mem back to the free list.
// Automatically merges adjacent free blocks (defragmentation).
// uc_orig: HEAP_give (fallen/Source/heap.cpp)
void HEAP_give(void* mem, SLONG num_bytes);

#endif // ENGINE_CORE_HEAP_H
