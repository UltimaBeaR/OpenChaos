#ifndef ENGINE_CORE_PQ_H
#define ENGINE_CORE_PQ_H

// Generic binary min-heap (priority queue) — C-style "textual template".
//
// NOTE: In the original codebase this was split into two files:
//   - fallen/Headers/pq.h  — forward declarations of the 5 static functions
//   - fallen/Source/pq.cpp  — macros, heap storage, and function definitions
// The consumer would #include both files (pq.h then pq.cpp) into its own .cpp
// after defining PQ_Type, PQ_HEAP_MAX_SIZE, and PQ_better(). This gave each
// consumer a private static copy of the heap — a C-era substitute for templates.
// We merged them into a single header since there's only one consumer and the
// two-file split added nothing. To restore the original layout: move everything
// below the forward declarations into a separate pq_impl.h or pq.cpp.
//
// Usage: before #including this header, the consumer must define:
//   - PQ_Type            — element typedef (e.g. a struct)
//   - PQ_HEAP_MAX_SIZE   — max heap capacity (integer literal)
//   - PQ_better(a, b)    — comparison function: returns true if *a is better than *b
//
// Currently only used by ai/mav.cpp (A* open set for NPC pathfinding).

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------

// uc_orig: PQ_init (fallen/Headers/pq.h)
static void PQ_init(void);

// uc_orig: PQ_add (fallen/Headers/pq.h)
static void PQ_add(PQ_Type);

// uc_orig: PQ_remove (fallen/Headers/pq.h)
static void PQ_remove(void);

// uc_orig: PQ_best (fallen/Headers/pq.h)
static PQ_Type PQ_best(void);

// uc_orig: PQ_empty (fallen/Headers/pq.h)
static SLONG PQ_empty(void);

// -----------------------------------------------------------------------
// Implementation (originally in fallen/Source/pq.cpp)
// -----------------------------------------------------------------------

// uc_orig: PQ_BETTER (fallen/Source/pq.cpp)
#define PQ_BETTER(x, y) (PQ_better(&PQ_heap[x], &PQ_heap[y]))

// uc_orig: PQ_BEST (fallen/Source/pq.cpp)
#define PQ_BEST(x, y, z)                  \
    (                                     \
        (PQ_BETTER(x, y))                 \
            ? ((PQ_BETTER(x, z)) ? x : z) \
            : ((PQ_BETTER(y, z)) ? y : z))

// uc_orig: PQ_SISTER (fallen/Source/pq.cpp)
#define PQ_SISTER(x) ((x) ^ 1)

// uc_orig: PQ_MOTHER (fallen/Source/pq.cpp)
#define PQ_MOTHER(x) ((x) >> 1)

// uc_orig: PQ_DAUGHTER (fallen/Source/pq.cpp)
#define PQ_DAUGHTER(x) ((x) << 1)

// Heap storage. Index 0 is unused; elements occupy indices 1..PQ_heap_end.
// uc_orig: PQ_heap (fallen/Source/pq.cpp)
static PQ_Type PQ_heap[PQ_HEAP_MAX_SIZE + 1];

// uc_orig: PQ_heap_end (fallen/Source/pq.cpp)
static SLONG PQ_heap_end;

// uc_orig: PQ_init (fallen/Source/pq.cpp)
static void PQ_init(void)
{
    PQ_heap_end = 0;
}

// uc_orig: PQ_best (fallen/Source/pq.cpp)
static PQ_Type PQ_best(void)
{
    return PQ_heap[1];
}

// uc_orig: PQ_empty (fallen/Source/pq.cpp)
static SLONG PQ_empty(void)
{
    return PQ_heap_end == 0;
}

// Add element and percolate up to maintain heap property.
// uc_orig: PQ_add (fallen/Source/pq.cpp)
static void PQ_add(PQ_Type newh)
{
    SLONG i, best, mum;
    PQ_Type spare;

    if (PQ_heap_end < PQ_HEAP_MAX_SIZE) {
        PQ_heap_end++;
    } else {
        // Overflow: silently drops the new element, which can cause missed routes.
        // Original had an ERROR() call commented out here.
    }

    PQ_heap[PQ_heap_end] = newh;

    // Percolate the new element up the heap.

    i = PQ_heap_end;

    if (i == 1)
        return;

    if (!(i & 1)) {
        // Even index — no sister yet. Compare directly with mother.

        if (PQ_BETTER(PQ_MOTHER(i), i))
            return;
        spare = PQ_heap[PQ_MOTHER(i)];
        PQ_heap[PQ_MOTHER(i)] = PQ_heap[i];
        PQ_heap[i] = spare;
        i = PQ_MOTHER(i);
    }

    while (i != 1) {
        mum = PQ_MOTHER(i);
        best = PQ_BEST(i, PQ_SISTER(i), mum);
        if (best == mum)
            return;
        i = mum;
        spare = PQ_heap[i];
        PQ_heap[i] = PQ_heap[best];
        PQ_heap[best] = spare;
    }

    return;
}

// Remove the best (root) element and rebalance by percolating down.
// uc_orig: PQ_remove (fallen/Source/pq.cpp)
static void PQ_remove(void)
{
    SLONG i = 1, best;
    PQ_Type spare;

    PQ_heap[1] = PQ_heap[PQ_heap_end];

    PQ_heap_end--;
    if (PQ_heap_end == 0)
        return;

    while (PQ_SISTER(PQ_DAUGHTER(i)) <= PQ_heap_end) {
        // Full family of three — pick the best among parent and both children.

        best = PQ_BEST(i, PQ_DAUGHTER(i), PQ_SISTER(PQ_DAUGHTER(i)));
        if (best == i)
            return;

        spare = PQ_heap[i];
        PQ_heap[i] = PQ_heap[best];
        PQ_heap[best] = spare;

        i = best;
    }

    if (i == PQ_heap_end)
        return;

    // Only one child (daughter == PQ_heap_end).

    if (PQ_BETTER(i, PQ_heap_end))
        return;

    spare = PQ_heap[i];
    PQ_heap[i] = PQ_heap[PQ_heap_end];
    PQ_heap[PQ_heap_end] = spare;

    return;
}

#endif // ENGINE_CORE_PQ_H
