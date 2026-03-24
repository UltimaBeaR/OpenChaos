#include "engine/core/heap.h"
#include "engine/core/heap_globals.h"
#include "engine/core/macros.h"
#include "engine/platform/platform.h"

// Free list node — stored inline at the start of each free block.
// uc_orig: HEAP_Free (fallen/Source/heap.cpp)
typedef struct HEAP_Free {
    UBYTE* start;
    UBYTE* end;
    SLONG size;
    struct HEAP_Free* next;
} HEAP_Free;

// Minimum allocation granularity — all sizes rounded up to this.
// uc_orig: HEAP_QUANTISE (fallen/Source/heap.cpp)
#define HEAP_QUANTISE 16

// Validates the free list — checks that all blocks are internally consistent.
// uc_orig: HEAP_check (fallen/Source/heap.cpp)
static void HEAP_check()
{
    HEAP_Free* hf;
    for (hf = HEAP_free; hf; hf = hf->next) {
        ASSERT(hf->size <= HEAP_SIZE);
        ASSERT(hf->start + hf->size == hf->end);
    }
}

// uc_orig: HEAP_init (fallen/Source/heap.cpp)
void HEAP_init()
{
    HEAP_free = (HEAP_Free*)HEAP_heap;
    HEAP_free->start = (UBYTE*)&HEAP_heap[0];
    HEAP_free->end = (UBYTE*)&HEAP_heap[HEAP_SIZE];
    HEAP_free->size = HEAP_SIZE;
    HEAP_free->next = NULL;
    HEAP_check();
}

// Inserts a free block back into the free list sorted by size (largest first).
// Merges with adjacent blocks when possible.
// uc_orig: HEAP_add_to_free (fallen/Source/heap.cpp)
static void HEAP_add_to_free(HEAP_Free* bit)
{
    HEAP_Free* next;
    HEAP_Free** prev;

start_again_with_a_bigger_bit:;

    ASSERT(bit->size <= HEAP_SIZE);
    ASSERT(bit->start + bit->size == bit->end);

    prev = &HEAP_free;
    next = HEAP_free;

    while (next) {
        // Can we merge bit and next (bit ends where next starts)?
        if (bit->end == next->start) {
            *prev = next->next;
            ASSERT(next->size <= HEAP_SIZE);
            bit->end = next->end;
            bit->size += next->size;
            goto start_again_with_a_bigger_bit;
        } else if (next->end == bit->start) {
            // Can we merge bit into next (next ends where bit starts)?
            *prev = next->next;
            ASSERT(bit->size <= HEAP_SIZE);
            next->end = bit->end;
            next->size += bit->size;
            ASSERT(next->size <= HEAP_SIZE);
            bit = next;
            goto start_again_with_a_bigger_bit;
        }

        prev = &next->next;
        next = next->next;
    }

    // Insert bit at the correct position ordered by size (descending).
    prev = &HEAP_free;
    next = HEAP_free;
    while (1) {
        if (next == NULL || next->size <= bit->size) {
            *prev = bit;
            bit->next = next;
            break;
        }
        prev = &next->next;
        next = next->next;
    }
}

// uc_orig: HEAP_get (fallen/Source/heap.cpp)
void* HEAP_get(SLONG size)
{
    void* ans;
    HEAP_Free bit;
    HEAP_Free* onheap;

    if (HEAP_free == NULL) {
        return NULL;
    }

    // Round up to nearest 16-byte boundary.
    size += (HEAP_QUANTISE - 1);
    size &= ~(HEAP_QUANTISE - 1);

    ASSERT(WITHIN((UBYTE*)HEAP_free, &HEAP_heap[0], &HEAP_heap[HEAP_SIZE - sizeof(HEAP_Free)]));

    if (HEAP_free->size < size) {
        return NULL;
    }

    // Take memory from the largest free block (first in list).
    bit.start = HEAP_free->start + size;
    bit.end = HEAP_free->end;
    bit.size = HEAP_free->size - size;
    bit.next = NULL;

    ASSERT(bit.start + bit.size == bit.end);

    ans = HEAP_free;
    HEAP_free = HEAP_free->next;

    if (bit.size == 0) {
        return ans;
    } else {
        ASSERT(bit.size >= sizeof(HEAP_Free));
        onheap = (HEAP_Free*)bit.start;
        *onheap = bit;
        HEAP_add_to_free(onheap);
        return ans;
    }
}

// uc_orig: HEAP_max_free (fallen/Source/heap.cpp)
SLONG HEAP_max_free(void)
{
    if (HEAP_free) {
        return (HEAP_free->size);
    } else {
        return (0);
    }
}

// uc_orig: HEAP_give (fallen/Source/heap.cpp)
void HEAP_give(void* mem, SLONG num_bytes)
{
    HEAP_Free* onheap = (HEAP_Free*)mem;

    num_bytes += (HEAP_QUANTISE - 1);
    num_bytes &= ~(HEAP_QUANTISE - 1);

    ASSERT(WITHIN((UBYTE*)onheap, &HEAP_heap[0], &HEAP_heap[HEAP_SIZE - sizeof(HEAP_Free)]));

    ASSERT(num_bytes <= HEAP_SIZE);

    onheap->start = (UBYTE*)mem;
    onheap->end = onheap->start + num_bytes;
    onheap->size = num_bytes;
    onheap->next = NULL;

    HEAP_add_to_free(onheap);
}
