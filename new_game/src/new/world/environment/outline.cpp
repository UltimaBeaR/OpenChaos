#include "world/environment/outline.h"
#include "core/macros.h"
#include <MFStdLib.h>
#include <cstdlib>
#include <cstring>

// Link direction tag within the sorted per-row lists.
// uc_orig: OUTLINE_LINK_TYPE_START (fallen/Editor/Source/outline.cpp)
#define OUTLINE_LINK_TYPE_START 0
// uc_orig: OUTLINE_LINK_TYPE_END (fallen/Editor/Source/outline.cpp)
#define OUTLINE_LINK_TYPE_END 1

// One sorted edge event within a single row (z-scanline).
// Links are kept sorted by x within each row.
// uc_orig: outline_link (fallen/Editor/Source/outline.cpp)
typedef struct outline_link {
    SLONG x;
    SLONG type;    // OUTLINE_LINK_TYPE_START or OUTLINE_LINK_TYPE_END
    struct outline_link* next;
} OUTLINE_Link;

// The outline object: an array of per-row sorted link lists.
// Flexible array member — allocated with room for max_z OUTLINE_Link* slots.
// uc_orig: outline_outline (fallen/Editor/Source/outline.cpp)
typedef struct outline_outline {
    SLONG max_z;
    OUTLINE_Link* link[];
} OUTLINE_Outline;

// uc_orig: OUTLINE_create (fallen/Editor/Source/outline.cpp)
OUTLINE_Outline* OUTLINE_create(SLONG num_z_squares)
{
    SLONG num_bytes = sizeof(OUTLINE_Outline) + sizeof(OUTLINE_Link*) * num_z_squares;
    OUTLINE_Outline* oo = (OUTLINE_Outline*)malloc(num_bytes);
    memset(oo, 0, num_bytes);
    oo->max_z = num_z_squares;
    return oo;
}

// Inserts ol into the sorted link list for row link_z.
// Links are ordered ascending by x; at the same x, END links come before START links.
// uc_orig: OUTLINE_insert_link (fallen/Editor/Source/outline.cpp)
static void OUTLINE_insert_link(OUTLINE_Outline* oo, OUTLINE_Link* ol, SLONG link_z)
{
    OUTLINE_Link* next;
    OUTLINE_Link** prev;

    ASSERT(WITHIN(link_z, 0, oo->max_z - 1));

    prev = &oo->link[link_z];
    next = oo->link[link_z];

    while (1) {
        SLONG here = UC_FALSE;

        if (next == NULL) {
            here = UC_TRUE;
        } else if (next->x > ol->x) {
            here = UC_TRUE;
        } else if (next->x == ol->x) {
            if (ol->x == OUTLINE_LINK_TYPE_END) {
                here = UC_TRUE;
            }
        }

        if (here) {
            ol->next = next;
            *prev = ol;
            return;
        }

        prev = &next->next;
        next = next->next;
    }
}

// uc_orig: OUTLINE_add_line (fallen/Editor/Source/outline.cpp)
void OUTLINE_add_line(
    OUTLINE_Outline* oo,
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG z;
    SLONG type;

    ASSERT(x1 == x2 || z1 == z2);
    ASSERT(WITHIN(z1, 0, oo->max_z - 1));
    ASSERT(WITHIN(z2, 0, oo->max_z - 1));

    if (z1 == z2) {
        // Horizontal line contributes no z-crossing events.
        return;
    }

    if (z1 > z2) {
        SWAP(z1, z2);
        type = OUTLINE_LINK_TYPE_START;
    } else {
        type = OUTLINE_LINK_TYPE_END;
    }

    for (z = z1; z < z2; z++) {
        OUTLINE_Link* ol = (OUTLINE_Link*)malloc(sizeof(OUTLINE_Link));
        ol->type = type;
        ol->x = x1;
        ol->next = NULL;
        OUTLINE_insert_link(oo, ol, z);
    }
}

// uc_orig: OUTLINE_free (fallen/Editor/Source/outline.cpp)
void OUTLINE_free(OUTLINE_Outline* oo)
{
    SLONG z;
    OUTLINE_Link* ol;
    OUTLINE_Link* next;

    for (z = 0; z < oo->max_z; z++) {
        for (ol = oo->link[z]; ol; ol = next) {
            next = ol->next;
            free(ol);
        }
    }
    free(oo);
}

// Tests whether two sorted scanline link lists overlap using a sweep algorithm.
// uc_orig: OUTLINE_overlap (fallen/Editor/Source/outline.cpp) [inner overload]
static SLONG OUTLINE_overlap(OUTLINE_Link* ol1, OUTLINE_Link* ol2)
{
    SLONG on1 = UC_FALSE;
    SLONG on2 = UC_FALSE;

    while (1) {
        if (ol1 == NULL || ol2 == NULL) {
            if (ol1 == NULL) {
                ASSERT(!on1);
            }
            if (ol2 == NULL) {
                ASSERT(!on2);
            }
            return UC_FALSE;
        }

        if ((ol1->x < ol2->x) || (ol1->x == ol2->x && ol1->type == OUTLINE_LINK_TYPE_END)) {
            if ((!on1 && ol1->type == OUTLINE_LINK_TYPE_START) || (on1 && ol1->type == OUTLINE_LINK_TYPE_END)) {
            } else
                return (0);
            ASSERT(
                (!on1 && ol1->type == OUTLINE_LINK_TYPE_START) || (on1 && ol1->type == OUTLINE_LINK_TYPE_END));
            ol1 = ol1->next;
            on1 ^= UC_TRUE;
        } else if (ol2->x < ol1->x || (ol1->x == ol2->x && ol2->type == OUTLINE_LINK_TYPE_END)) {
            if ((!on2 && ol2->type == OUTLINE_LINK_TYPE_START) || (on2 && ol2->type == OUTLINE_LINK_TYPE_END)) {
            } else {
                return (0);
            }
            ASSERT(
                (!on2 && ol2->type == OUTLINE_LINK_TYPE_START) || (on2 && ol2->type == OUTLINE_LINK_TYPE_END));
            ol2 = ol2->next;
            on2 ^= UC_TRUE;
        } else if (ol1->x == ol2->x) {
            ASSERT(ol1->type == OUTLINE_LINK_TYPE_START);
            ASSERT(ol2->type == OUTLINE_LINK_TYPE_START);
            // Both outlines are starting a span at the same x — they overlap.
            return UC_TRUE;
        }

        if (on1 && on2) {
            return UC_TRUE;
        }
    }
}

// uc_orig: OUTLINE_overlap (fallen/Editor/Source/outline.cpp) [outer overload]
SLONG OUTLINE_overlap(OUTLINE_Outline* oo1, OUTLINE_Outline* oo2)
{
    SLONG z;
    SLONG minz = MIN(oo1->max_z, oo2->max_z);

    for (z = 0; z < minz; z++) {
        if (OUTLINE_overlap(oo1->link[z], oo2->link[z])) {
            return UC_TRUE;
        }
    }
    return UC_FALSE;
}

// Returns UC_TRUE if the point (x, z) is inside the outline.
// uc_orig: OUTLINE_inside (fallen/Editor/Source/outline.cpp)
static SLONG OUTLINE_inside(OUTLINE_Outline* oo, SLONG x, SLONG z)
{
    OUTLINE_Link* ol;

    if (!WITHIN(z, 0, oo->max_z - 1)) {
        return UC_FALSE;
    }

    ol = oo->link[z];

    while (1) {
        if (ol == NULL || ol->next == NULL) {
            return UC_FALSE;
        }
        if (ol->type == OUTLINE_LINK_TYPE_START && ol->next->type == OUTLINE_LINK_TYPE_END) {
            if (WITHIN(x, ol->x, ol->next->x - 1)) {
                return UC_TRUE;
            }
        }
        ol = ol->next;
    }
}

// uc_orig: OUTLINE_intersects (fallen/Editor/Source/outline.cpp)
SLONG OUTLINE_intersects(
    OUTLINE_Outline* oo,
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG i;
    SLONG x;
    SLONG z;
    SLONG dx;
    SLONG dz;
    SLONG mx1;
    SLONG mz1;
    SLONG mx2;
    SLONG len;

    ASSERT(x1 == x2 || z1 == z2);

    dx = x2 - x1;
    dz = z2 - z1;
    len = MAX(abs(dx), abs(dz));

    x = x1 << 8;
    z = z1 << 8;
    dx = SIGN(dx);
    dz = SIGN(dz);
    x += dx << 7;
    z += dz << 7;

    for (i = 0; i < len; i++) {
        mx1 = x - (dz << 7) >> 8;
        mz1 = z + (dx << 7) >> 8;
        mx2 = x + (dz << 7) >> 8;

        if (OUTLINE_inside(oo, mx1, mz1) || OUTLINE_inside(oo, mx2, mz1)) {
            return UC_TRUE;
        }

        x += dx << 8;
        z += dz << 8;
    }

    return UC_FALSE;
}
