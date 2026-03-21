#include <MFStdLib.h>
#include "core/fixed_math.h"

// Temporary: STAIR_FLAG_UP/DOWN are defined in fallen/Headers/building.h.
// Copied here to avoid pulling in building.h which requires Structs.h.
// Will be cleaned up when the building system is migrated.
#ifndef STAIR_FLAG_UP
#define STAIR_FLAG_UP   (1 << 0)
#define STAIR_FLAG_DOWN (1 << 1)
#endif

#include "world/environment/stair.h"
#include "world/environment/stair_globals.h"

// Access STAIR_edge[] relative to bounding box (adjusts for STAIR_z1 offset).
// uc_orig: STAIR_EDGE (fallen/Source/Stair.cpp)
#define STAIR_EDGE(z) (STAIR_edge[(z) - STAIR_z1])

// --- Internal LCG random number generator ---
// Seeded from a UWORD before each STAIR_calculate() call for reproducible stair placement.

// uc_orig: STAIR_srand (fallen/Source/Stair.cpp)
static inline void STAIR_srand(ULONG seed)
{
    STAIR_rand_seed = seed;
}

// uc_orig: STAIR_grand (fallen/Source/Stair.cpp)
static inline ULONG STAIR_grand(void)
{
    return STAIR_rand_seed;
}

// LCG step: seed = seed * 328573 + 123456789; returns upper bits.
// uc_orig: STAIR_rand (fallen/Source/Stair.cpp)
static inline UWORD STAIR_rand(void)
{
    UWORD ans;

    STAIR_rand_seed *= 328573;
    STAIR_rand_seed += 123456789;

    ans = STAIR_rand_seed >> 7;

    return ans;
}

// Set the inside-tile bit for (x, z) in the given storey's packed bitfield.
// uc_orig: STAIR_set_bit (fallen/Source/Stair.cpp)
static void STAIR_set_bit(SLONG storey, SLONG x, SLONG z)
{
    SLONG off_x;
    SLONG off_z;

    SLONG byte;
    SLONG bit;

    ASSERT(WITHIN(storey, 0, STAIR_storey_upto - 1));
    ASSERT(WITHIN(x, STAIR_x1, STAIR_x2 - 1));
    ASSERT(WITHIN(z, STAIR_z1, STAIR_z2 - 1));

    off_x = x - STAIR_x1;
    off_z = z - STAIR_z1;

    ASSERT(WITHIN(off_x, 0, STAIR_MAX_SIZE - 1));
    ASSERT(WITHIN(off_z, 0, STAIR_MAX_SIZE - 1));

    byte = off_x >> 3;
    bit = off_x & 7;

    STAIR_storey[storey].square[off_z][byte] |= (1 << bit);
}

// Query the inside-tile bit for (x, z) from the given storey's packed bitfield.
// uc_orig: STAIR_get_bit (fallen/Source/Stair.cpp)
static UBYTE STAIR_get_bit(SLONG storey, SLONG x, SLONG z)
{
    SLONG off_x;
    SLONG off_z;

    SLONG byte;
    SLONG bit;

    ASSERT(WITHIN(storey, 0, STAIR_storey_upto - 1));
    ASSERT(WITHIN(x, STAIR_x1, STAIR_x2 - 1));
    ASSERT(WITHIN(z, STAIR_z1, STAIR_z2 - 1));

    off_x = x - STAIR_x1;
    off_z = z - STAIR_z1;

    ASSERT(WITHIN(off_x, 0, STAIR_MAX_SIZE - 1));
    ASSERT(WITHIN(off_z, 0, STAIR_MAX_SIZE - 1));

    byte = off_x >> 3;
    bit = off_x & 7;

    return STAIR_storey[storey].square[off_z][byte] & (1 << bit);
}

// Query the inside-tile bit for (x, z) from an arbitrary packed bitfield (not tied to a storey index).
// Used during STAIR_calculate to query the AND-ed combined square of two storeys.
// uc_orig: STAIR_get_bit_from_square (fallen/Source/Stair.cpp)
static UBYTE STAIR_get_bit_from_square(UBYTE square[STAIR_MAX_SIZE][STAIR_MAX_SIZE / 8], SLONG x, SLONG z)
{
    SLONG off_x;
    SLONG off_z;

    SLONG byte;
    SLONG bit;

    ASSERT(WITHIN(x, STAIR_x1, STAIR_x2 - 1));
    ASSERT(WITHIN(z, STAIR_z1, STAIR_z2 - 1));

    off_x = x - STAIR_x1;
    off_z = z - STAIR_z1;

    ASSERT(WITHIN(off_x, 0, STAIR_MAX_SIZE - 1));
    ASSERT(WITHIN(off_z, 0, STAIR_MAX_SIZE - 1));

    byte = off_x >> 3;
    bit = off_x & 7;

    return square[off_z][byte] & (1 << bit);
}

// Debug helper: asserts that z is within the bounding box.
// uc_orig: STAIR_check_edge (fallen/Source/Stair.cpp)
static void STAIR_check_edge(SLONG z)
{
    SLONG sz;

    ASSERT(WITHIN(z, STAIR_z1, STAIR_z2 - 1));

    sz = z - STAIR_z1;

    ASSERT(WITHIN(sz, 0, STAIR_MAX_SIZE - 1));
}

// Adds the stair index to the given storey's stair slot array.
// uc_orig: STAIR_add_to_storey (fallen/Source/Stair.cpp)
static void STAIR_add_to_storey(SLONG storey, SLONG stair)
{
    SLONG i;

    ASSERT(WITHIN(storey, 0, STAIR_storey_upto - 1));
    ASSERT(WITHIN(stair, 1, STAIR_stair_upto - 1));

    for (i = 0; i < STAIR_MAX_PER_STOREY; i++) {
        if (STAIR_storey[storey].stair[i] == NULL) {
            STAIR_storey[storey].stair[i] = stair;

            return;
        }
    }

    // No more room!
    ASSERT(0);
}

// Stub: returns whether there is an outside door at tile (x, z) for the given storey.
// Always returns 0 in the pre-release build, so the door-blocking penalty never fires.
// uc_orig: STAIR_is_door (fallen/Source/Stair.cpp)
static SLONG STAIR_is_door(SLONG storey, SLONG x, SLONG z)
{
    return 0;
}

// Reset all stair state. Call before processing a new building.
// Bounding box is initialized inverted (x1=255, x2=0) so the first real point sets it.
// stair_upto starts at 1 because index 0 is the NULL sentinel.
// uc_orig: STAIR_init (fallen/Source/Stair.cpp)
void STAIR_init()
{
    STAIR_x1 = 255;
    STAIR_z1 = 255;
    STAIR_x2 = 0;
    STAIR_z2 = 0;

    STAIR_stair_upto = 1;  // 0 is the NULL stair
    STAIR_storey_upto = 0;

    STAIR_roof_height = 0;
}

// uc_orig: STAIR_set_bounding_box (fallen/Source/Stair.cpp)
void STAIR_set_bounding_box(UBYTE x1, UBYTE z1, UBYTE x2, UBYTE z2)
{
    ASSERT(WITHIN(x1, 0, 255));
    ASSERT(WITHIN(z1, 0, 255));
    ASSERT(WITHIN(x2, 0, 255));
    ASSERT(WITHIN(z2, 0, 255));

    STAIR_x1 = x1;
    STAIR_z1 = z1;
    STAIR_x2 = x2;
    STAIR_z2 = z2;

    ASSERT(WITHIN(STAIR_x2 - STAIR_x1, 0, STAIR_MAX_SIZE));
    ASSERT(WITHIN(STAIR_z2 - STAIR_z1, 0, STAIR_MAX_SIZE));
}

// Begin a new storey. Must call STAIR_storey_wall + STAIR_storey_finish after.
// handle: opaque ID from the building system, retrieved later by STAIR_get().
// height: floor index (0=ground, 1=first floor, ...).
// uc_orig: STAIR_storey_new (fallen/Source/Stair.cpp)
void STAIR_storey_new(SLONG handle, UBYTE height)
{
    SLONG i;

    STAIR_Storey* st;

    ASSERT(WITHIN(STAIR_storey_upto, 0, STAIR_MAX_STOREYS - 1));

    st = &STAIR_storey[STAIR_storey_upto];

    if (STAIR_roof_height < height + 1) {
        STAIR_roof_height = height + 1;
    }

    memset((UBYTE*)st, 0, sizeof(STAIR_Storey));

    st->handle = handle;
    st->height = height;
    st->opp_x1 = 0;
    st->opp_z1 = 0;
    st->opp_x2 = 0;
    st->opp_z2 = 0;

    // Reset scanline edge lists for this storey.
    for (i = 0; i < STAIR_MAX_SIZE; i++) {
        STAIR_edge[i] = 0;
    }

    // Index 0 is the NULL link sentinel.
    STAIR_link_upto = 1;

    STAIR_storey_upto += 1;
}

// Feed one wall segment into the scanline edge lists for the current storey.
// If opposite is TRUE, records this wall as the "preferred opposite" wall for stair scoring.
// Horizontal walls (z1==z2) are skipped — they don't cross any z-scanlines.
// Uses a sorted linked list per z-row to record x-intercepts (16.16 fixed-point arithmetic).
// uc_orig: STAIR_storey_wall (fallen/Source/Stair.cpp)
void STAIR_storey_wall(UBYTE x1, UBYTE z1, UBYTE x2, UBYTE z2, SLONG opposite)
{
    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG dxdz;
    SLONG pos;
    SLONG square;

    UBYTE link_t;
    UBYTE next;
    UBYTE* prev;

    ASSERT(WITHIN(x1, STAIR_x1, STAIR_x2));
    ASSERT(WITHIN(z1, STAIR_z1, STAIR_z2));

    if (opposite) {
        STAIR_Storey* st;

        ASSERT(WITHIN(STAIR_storey_upto - 1, 0, STAIR_MAX_STOREYS - 1));

        st = &STAIR_storey[STAIR_storey_upto - 1];

        st->opp_x1 = x1;
        st->opp_z1 = z1;
        st->opp_x2 = x2;
        st->opp_z2 = z2;

        if (x1 > x2) {
            SWAP(st->opp_x1, st->opp_x2);
        }
        if (z1 > z2) {
            SWAP(st->opp_z1, st->opp_z2);
        }
    }

    if (z1 == z2) {
        // Horizontal wall — does not cross any z-scanlines.
        return;
    }

    if (z1 > z2) {
        link_t = STAIR_LINK_T_ENTER;

        // Always process top-to-bottom.
        SWAP(x1, x2);
        SWAP(z1, z2);
    } else {
        link_t = STAIR_LINK_T_LEAVE;
    }

    dx = x2 - x1 << 16;
    dz = z2 - z1 << 16;

    dxdz = DIV64(dx, dz);

    x = x1 << 16;
    z = z1;

    while (z < z2) {
        switch (link_t) {
        case STAIR_LINK_T_ENTER:
            pos = x >> 8;
            square = MAX(x, x + dxdz);
            square += 0xffff;
            square >>= 16;
            break;
        case STAIR_LINK_T_LEAVE:
            pos = x >> 8;
            square = MIN(x, x + dxdz);
            square >>= 16;
            break;

        default:
            ASSERT(0);
            break;
        }

        ASSERT(WITHIN(STAIR_link_upto, 1, STAIR_MAX_LINKS - 1));

        STAIR_link[STAIR_link_upto].square = square;
        STAIR_link[STAIR_link_upto].pos = pos;
        STAIR_link[STAIR_link_upto].next = 0;

        // Insert in sorted order by x-position into the z-row linked list.
        prev = &STAIR_EDGE(z);
        next = STAIR_EDGE(z);

        while (1) {
            if (next == NULL) {
                break;
            }

            ASSERT(WITHIN(next, 1, STAIR_link_upto - 1));

            if (STAIR_link[next].pos >= STAIR_link[STAIR_link_upto].pos) {
                if (STAIR_link[next].pos == STAIR_LINK_T_ENTER) {
                    // Keep ENTER links first.
                } else {
                    break;
                }
            }

            prev = &STAIR_link[next].next;
            next = STAIR_link[next].next;
        }

        ASSERT(*prev == next);

        STAIR_link[STAIR_link_upto].next = next;
        *prev = STAIR_link_upto;
        STAIR_link_upto += 1;

        x += dxdz;
        z += 1;
    }
}

// Completes the scanline fill for the current storey.
// Reads sorted ENTER/LEAVE link pairs per z-row and marks all interior tiles via STAIR_set_bit.
// Returns TRUE if the storey polygon is valid (all rows have exactly one ENTER/LEAVE pair).
// uc_orig: STAIR_storey_finish (fallen/Source/Stair.cpp)
SLONG STAIR_storey_finish()
{
    SLONG i;

    SLONG x;
    SLONG z;

    SLONG x1;
    SLONG x2;

    UBYTE next;
    UBYTE next1;
    UBYTE next2;

    STAIR_Storey* st;

    ASSERT(WITHIN(STAIR_storey_upto - 1, 0, STAIR_MAX_STOREYS - 1));

    st = &STAIR_storey[STAIR_storey_upto - 1];

    for (z = STAIR_z1; z < STAIR_z2; z++) {
        next = STAIR_EDGE(z);

        if (next == NULL) {
            continue;
        }

        // Links come in pairs: one ENTER, one LEAVE.
        next1 = next;
        next2 = STAIR_link[next].next;

        if (next1 == NULL || next2 == NULL) {
            return FALSE;
        }

        ASSERT(WITHIN(next1, 1, STAIR_link_upto - 1));
        ASSERT(WITHIN(next2, 1, STAIR_link_upto - 1));

        x1 = STAIR_link[next1].square;
        x2 = STAIR_link[next2].square;

        for (x = x1; x < x2; x++) {
            STAIR_set_bit(STAIR_storey_upto - 1, x, z);
        }
    }

    return TRUE;
}

// Main stair placement algorithm. Call after all storeys have been registered.
// seed: UWORD for the internal PRNG — same seed gives identical placement.
// Scoring: outside walls (+), interior (-), corners (+), opposite-wall hint (+0xbabe), door-blocker (-INF).
// Uses goto to break out of nested loops (idiomatic C of the era).
// uc_orig: STAIR_calculate (fallen/Source/Stair.cpp)
void STAIR_calculate(UWORD seed)
{
    SLONG i;
    SLONG j;
    SLONG k;
    SLONG l;

    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG x1;
    SLONG x2;
    SLONG z1;
    SLONG z2;
    SLONG score;

    SLONG ox;
    SLONG oz;
    SLONG outside;

    SLONG best_x1;
    SLONG best_z1;
    SLONG best_x2;
    SLONG best_z2;
    SLONG best_score;

    STAIR_Storey* st;
    STAIR_Storey* st_n;

    STAIR_Stair* ss;
    STAIR_Stair* ss_new;

    SLONG dox;
    SLONG doz;

    SLONG height;
    SLONG height_n;

    UBYTE square[STAIR_MAX_SIZE][STAIR_MAX_SIZE / 8];
    UBYTE go_up;
    UBYTE go_down;

    // Randomise the order we process storeys.
    UBYTE storey_order1[STAIR_MAX_STOREYS];

    STAIR_srand(seed);

    for (i = 0; i < STAIR_MAX_STOREYS; i++) {
        storey_order1[i] = i;
    }

    for (i = 0; i < STAIR_MAX_STOREYS; i++) {
        j = STAIR_rand() % STAIR_MAX_STOREYS;
        k = STAIR_rand() % STAIR_MAX_STOREYS;

        SWAP(storey_order1[j], storey_order1[k]);
    }

    for (i = 0; i < STAIR_storey_upto; i++) {
        st = &STAIR_storey[i];

        height = st->height;

        for (j = 0; j < STAIR_storey_upto; j++) {
            st_n = &STAIR_storey[j];

            height_n = st_n->height;

            if (abs(height - height_n) == 1) {
                // Storey j is adjacent to storey i. AND their inside bitmaps.
                memcpy(square, st->square, sizeof(square));

                for (z = 0; z < STAIR_MAX_SIZE; z++)
                    for (x = 0; x < STAIR_MAX_SIZE / 8; x++) {
                        square[z][x] &= st_n->square[z][x];
                    }

                go_up = 0;
                go_down = 0;

                if (height_n == height + 1) {
                    go_up = 1;
                } else if (height_n == height - 1) {
                    go_down = 1;
                } else {
                    ASSERT(0);
                }

                // Check if storey i already has a stair we can reuse.
                for (k = 0; k < STAIR_MAX_PER_STOREY; k++) {
                    if (st->stair[k] != NULL) {
                        ASSERT(WITHIN(st->stair[k], 1, STAIR_stair_upto - 1));

                        ss = &STAIR_stair[st->stair[k]];

                        if (STAIR_get_bit_from_square(square, ss->x1, ss->z1) && STAIR_get_bit_from_square(square, ss->x2, ss->z2)) {
                            if (go_up) {
                                if (ss->flag & STAIR_FLAG_UP) {
                                    goto placed_stairs;
                                } else {
                                    ss->flag |= STAIR_FLAG_UP;
                                    ss->up = j;

                                    ss_new = &STAIR_stair[STAIR_stair_upto++];

                                    ss_new->flag = STAIR_FLAG_DOWN;
                                    ss_new->down = i;
                                    ss_new->x1 = ss->x1;
                                    ss_new->z1 = ss->z1;
                                    ss_new->x2 = ss->x2;
                                    ss_new->z2 = ss->z2;

                                    STAIR_add_to_storey(j, STAIR_stair_upto - 1);

                                    goto placed_stairs;
                                }
                            }

                            if (go_down) {
                                if (ss->flag & STAIR_FLAG_DOWN) {
                                    goto placed_stairs;
                                } else {
                                    ss->flag |= STAIR_FLAG_DOWN;
                                    ss->down = j;

                                    ss_new = &STAIR_stair[STAIR_stair_upto++];

                                    ss_new->flag = STAIR_FLAG_UP;
                                    ss_new->up = i;
                                    ss_new->x1 = ss->x1;
                                    ss_new->z1 = ss->z1;
                                    ss_new->x2 = ss->x2;
                                    ss_new->z2 = ss->z2;

                                    STAIR_add_to_storey(j, STAIR_stair_upto - 1);

                                    goto placed_stairs;
                                }
                            }
                        }
                    }
                }

                // Check if the neighbouring storey j already has a suitable stair.
                for (k = 0; k < STAIR_MAX_PER_STOREY; k++) {
                    if (st_n->stair[k] != NULL) {
                        ASSERT(WITHIN(st_n->stair[k], 1, STAIR_stair_upto - 1));

                        ss = &STAIR_stair[st_n->stair[k]];

                        if (STAIR_get_bit_from_square(square, ss->x1, ss->z1) && STAIR_get_bit_from_square(square, ss->x2, ss->z2)) {
                            if (go_up) {
                                if (ss->flag & STAIR_FLAG_DOWN) {
                                    ASSERT(0);
                                }

                                ss->flag |= STAIR_FLAG_DOWN;
                                ss->down = i;

                                ss_new = &STAIR_stair[STAIR_stair_upto++];

                                ss_new->flag = STAIR_FLAG_UP;
                                ss_new->up = j;
                                ss_new->x1 = ss->x1;
                                ss_new->z1 = ss->z1;
                                ss_new->x2 = ss->x2;
                                ss_new->z2 = ss->z2;

                                STAIR_add_to_storey(i, STAIR_stair_upto - 1);

                                goto placed_stairs;
                            }

                            if (go_down) {
                                if (ss->flag & STAIR_FLAG_UP) {
                                    ASSERT(0);
                                }

                                ss->flag |= STAIR_FLAG_UP;
                                ss->up = i;

                                ss_new = &STAIR_stair[STAIR_stair_upto++];

                                ss_new->flag = STAIR_FLAG_DOWN;
                                ss_new->down = j;
                                ss_new->x1 = ss->x1;
                                ss_new->z1 = ss->z1;
                                ss_new->x2 = ss->x2;
                                ss_new->z2 = ss->z2;

                                STAIR_add_to_storey(i, STAIR_stair_upto - 1);

                                goto placed_stairs;
                            }
                        }
                    }
                }

                // No existing stair to reuse — find the best new placement.
                // Score all adjacent tile pairs in 4 orientations.
                best_score = -INFINITY;

                for (x = STAIR_x1; x < STAIR_x2; x++)
                    for (z = STAIR_z1; z < STAIR_z2; z++) {
                        x1 = x;
                        z1 = z;

                        for (k = 0; k < 4; k++) {
                            x2 = x1;
                            z2 = z1;

                            switch (k) {
                            case 0:
                                x2 += 1;
                                break;
                            case 1:
                                x2 -= 1;
                                break;
                            case 2:
                                z2 += 1;
                                break;
                            case 3:
                                z2 -= 1;
                                break;
                            default:
                                ASSERT(0);
                                break;
                            }

                            if (!WITHIN(x2, STAIR_x1, STAIR_x2 - 1) || !WITHIN(z2, STAIR_z1, STAIR_z2 - 1)) {
                                continue;
                            }

                            if (STAIR_get_bit_from_square(square, x1, z1) && STAIR_get_bit_from_square(square, x2, z2)) {
                                score = STAIR_rand() & 0xffff;
                                outside = 0;

                                dx = x2 - x1;
                                dz = z2 - z1;

                                // Check if each stair cell is against an outside wall.
                                for (l = 0; l < 2; l++) {
                                    switch (l) {
                                    case 0:
                                        ox = x1 + (+dz);
                                        oz = z1 + (-dx);
                                        break;
                                    case 1:
                                        ox = x2 + (+dz);
                                        oz = z2 + (-dx);
                                        break;
                                    default:
                                        ASSERT(0);
                                        break;
                                    }

                                    if (!WITHIN(ox, STAIR_x1, STAIR_x2 - 1) || !WITHIN(oz, STAIR_z1, STAIR_z2 - 1) || !STAIR_get_bit(i, ox, oz)) {
                                        outside += 1;

                                        if (STAIR_is_door(i, ox, oz)) {
                                            score = -INFINITY;
                                        }
                                    }
                                }

                                // Both cells on outside wall: strong bonus. One cell: penalty. Interior: neutral.
                                switch (outside) {
                                case 0:
                                    score = 0x0;
                                    break;
                                case 1:
                                    score -= 0x10000;
                                    break;
                                case 2:
                                    score += 0x10000;
                                    break;
                                default:
                                    ASSERT(0);
                                    break;
                                }

                                // Corner bonus: +0x5000 per bounding-box edge matched.
                                if (x1 == STAIR_x1) { score += 0x5000; }
                                if (x1 == STAIR_x2) { score += 0x5000; }
                                if (x2 == STAIR_x1) { score += 0x5000; }
                                if (x2 == STAIR_x2) { score += 0x5000; }

                                if (z1 == STAIR_z1) { score += 0x5000; }
                                if (z1 == STAIR_z2) { score += 0x5000; }
                                if (z2 == STAIR_z1) { score += 0x5000; }
                                if (z2 == STAIR_z2) { score += 0x5000; }

                                // Prefer stairs opposite the "opposite wall" hint (+0xbabe per cell).
                                if (z1 == z2 && st->opp_z1 == st->opp_z2) {
                                    doz = st->opp_z1 - z1;

                                    if (abs(doz) > 2) {
                                        if (WITHIN(x1, st->opp_x1, st->opp_x2)) { score += 0xbabe; }
                                        if (WITHIN(x2, st->opp_x1, st->opp_x2)) { score += 0xbabe; }
                                    }
                                }

                                if (x1 == x2 && st->opp_x1 == st->opp_x2) {
                                    dox = st->opp_x1 - x1;

                                    if (abs(dox) > 2) {
                                        if (WITHIN(z1, st->opp_z1, st->opp_z2)) { score += 0xbabe; }
                                        if (WITHIN(z2, st->opp_z1, st->opp_z2)) { score += 0xbabe; }
                                    }
                                }

                                if (score > best_score) {
                                    best_x1 = x1;
                                    best_z1 = z1;
                                    best_x2 = x2;
                                    best_z2 = z2;
                                    best_score = score;
                                }
                            }
                        }
                    }

                if (best_score >= 0) {
                    ASSERT(WITHIN(STAIR_stair_upto, 1, STAIR_MAX_STAIRS - 2));

                    // Place stair on storey i.
                    ss = &STAIR_stair[STAIR_stair_upto++];

                    ss->flag = 0;
                    ss->x1 = best_x1;
                    ss->z1 = best_z1;
                    ss->x2 = best_x2;
                    ss->z2 = best_z2;

                    if (go_up) {
                        ss->flag |= STAIR_FLAG_UP;
                        ss->up = j;
                    }
                    if (go_down) {
                        ss->flag |= STAIR_FLAG_DOWN;
                        ss->down = j;
                    }

                    STAIR_add_to_storey(i, STAIR_stair_upto - 1);

                    // Mirror stair on storey j with swapped flags.
                    ss = &STAIR_stair[STAIR_stair_upto++];

                    ss->flag = 0;
                    ss->x1 = best_x1;
                    ss->z1 = best_z1;
                    ss->x2 = best_x2;
                    ss->z2 = best_z2;

                    if (go_up) {
                        ss->flag |= STAIR_FLAG_DOWN;
                        ss->down = i;
                    }
                    if (go_down) {
                        ss->flag |= STAIR_FLAG_UP;
                        ss->up = j;
                    }

                    STAIR_add_to_storey(j, STAIR_stair_upto - 1);
                } else {
                    // Need stairs but no valid placement found.
                }

            placed_stairs:;
            } else {
                // Storeys i and j are more than one floor apart — no direct stair.
            }
        }
    }
}

// Returns the stairs for the given floor handle.
// On success: fills *stair with pointer to STAIR_id_stair[], sets *num_stairs, returns TRUE.
// uc_orig: STAIR_get (fallen/Source/Stair.cpp)
SLONG STAIR_get(SLONG handle, ID_Stair** stair, SLONG* num_stairs)
{
    SLONG i;
    SLONG storey;

    STAIR_Storey* st;
    STAIR_Stair* ss;
    ID_Stair* is;

    for (i = 0; i < STAIR_storey_upto; i++) {
        st = &STAIR_storey[i];

        if (st->handle == handle) {
            goto found_storey_handle;
        }
    }

    return FALSE;

found_storey_handle:

    *num_stairs = 0;

    for (i = 0; i < STAIR_MAX_PER_STOREY; i++) {
        if (st->stair[i] != NULL) {
            ASSERT(WITHIN(*num_stairs, 0, STAIR_MAX_PER_STOREY - 1));
            ASSERT(WITHIN(st->stair[i], 1, STAIR_stair_upto - 1));

            ss = &STAIR_stair[st->stair[i]];
            is = &STAIR_id_stair[*num_stairs];

            is->x1 = ss->x1;
            is->z1 = ss->z1;
            is->x2 = ss->x2;
            is->z2 = ss->z2;

            if (ss->flag == STAIR_FLAG_UP) {
                ASSERT(WITHIN(ss->up, 0, STAIR_storey_upto - 1));

                is->type = ID_STAIR_TYPE_BOTTOM;
                is->handle_up = STAIR_storey[ss->up].handle;
            } else if (ss->flag == STAIR_FLAG_DOWN) {
                ASSERT(WITHIN(ss->down, 0, STAIR_storey_upto - 1));

                is->type = ID_STAIR_TYPE_TOP;
                is->handle_down = STAIR_storey[ss->down].handle;
            } else {
                ASSERT(ss->flag == (STAIR_FLAG_UP | STAIR_FLAG_DOWN));

                ASSERT(WITHIN(ss->up, 0, STAIR_storey_upto - 1));
                ASSERT(WITHIN(ss->down, 0, STAIR_storey_upto - 1));

                is->type = ID_STAIR_TYPE_MIDDLE;

                is->handle_up = STAIR_storey[ss->up].handle;
                is->handle_down = STAIR_storey[ss->down].handle;
            }

            is->id = 0;

            *num_stairs += 1;
        }
    }

    *stair = STAIR_id_stair;

    return TRUE;
}
