#include "fallen/Headers/Game.h"
#include "world/map/supermap.h"
#include "world/map/pap_globals.h"
#include "world/map/ob.h"
#include "world/map/ob_globals.h"
#include "missions/memory_globals.h" // Temporary: world → missions DAG violation (prim_faces4, prim_points externs)
#include "world/environment/build2.h"

// Forward declarations of MAV function (mav.cpp not yet migrated).
extern void MAV_remove_facet_car(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// ---- Private helpers ----

// uc_orig: calc_ladder_ends (fallen/Source/build2.cpp)
// Shrinks a ladder facet's endpoints inward by 1/3 and offsets them perpendicularly
// by 1/8 of their length, so collision only triggers inside the ladder opening.
void calc_ladder_ends(SLONG* x1, SLONG* z1, SLONG* x2, SLONG* z2)
{
    SLONG dx, dz;

    dx = *x2 - *x1;
    dz = *z2 - *z1;

    *x1 += dx / 3;
    *z1 += dz / 3;

    *x2 -= dx / 3;
    *z2 -= dz / 3;

    *x1 += dz >> 3;
    *x2 += dz >> 3;

    *z1 -= dx >> 3;
    *z2 -= dx >> 3;
}

// uc_orig: find_empty_facet_block (fallen/Source/build2.cpp)
// Finds a run of 'count' consecutive zeroed slots in facet_links[].
// First tries the end of the used range; then scans for gaps.
// Returns the slot index, or 0 on failure.
static SLONG find_empty_facet_block(SLONG count)
{
    SLONG c0, c1;

    if ((MAX_FACET_LINK) - next_facet_link > count) {
        next_facet_link += count;
        return next_facet_link - count;
    } else {
        for (c0 = 1; c0 < next_facet_link; c0++) {
            if (facet_links[c0] == 0) {
                SLONG empty = 1;
                for (c1 = c0 + 1; (c1 < next_facet_link) && (empty < count); c1++) {
                    if (facet_links[c1])
                        break;
                    empty++;
                }
                if (empty == count)
                    return c0;
            }
        }
    }
    return 0;
}

// uc_orig: garbage_collection (fallen/Source/build2.cpp)
// Compacts facet_links[] by rebuilding it from the PAP ColVectHead lists,
// eliminating gaps left by earlier deletions. Updates all ColVectHead pointers.
static SLONG garbage_collection(void)
{
    SLONG index, next = 1;
    SWORD* mem;
    SLONG x, z;
    UWORD per_map[150], c0;

    memset(per_map, 0, 300);

    mem = (SWORD*)MemAlloc(MAX_FACET_LINK * sizeof(SWORD));
    ASSERT(mem);

    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            SLONG count;
            if ((index = PAP_2LO(x, z).ColVectHead)) {
                PAP_2LO(x, z).ColVectHead = next;
                count = 0;
                while (1) {
                    count++;
                    mem[next++] = facet_links[index++];
                    if (facet_links[index - 1] < 0) {
                        ASSERT(count < 150);
                        per_map[count]++;
                        break;
                    }
                }
            }
        }

    for (c0 = 0; c0 < 150; c0++) {
    }
    memcpy((UBYTE*)&facet_links[0], (UBYTE*)mem, next * sizeof(UWORD));
    next_facet_link = next;

    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            if (PAP_2LO(x, z).ColVectHead)
                ASSERT(facet_links[PAP_2LO(x, z).ColVectHead] != 0);
        }

    MemFree((UBYTE*)mem);
    return 0;
}

// uc_orig: create_extra_facet (fallen/Source/build2.cpp)
// Extends an existing facet_links[] run at 'findex' by one slot to accommodate
// a new facet. Copies the run to a new larger slot, erasing the old location.
// Runs garbage collection if no free run of the needed size exists.
static SLONG create_extra_facet(SLONG facet, SLONG findex)
{
    SLONG pos, count = 1;
    SLONG garbage = 0;

    pos = findex;
    while (facet_links[pos++] > 0) {
        ASSERT(facet_links[pos - 1] != facet);
        count++;
    }
    ASSERT(abs(facet_links[pos - 1]) != abs(facet));

try_again:;
    pos = find_empty_facet_block(count + 1);
    if (pos) {
        SLONG pos_copy;
        pos_copy = pos + 1;
        while (count) {
            facet_links[pos_copy++] = facet_links[findex++];
            facet_links[findex - 1] = 0;
            count--;
        }
    } else {
        ASSERT(garbage == 0);
        garbage++;
        garbage_collection();
        goto try_again;
    }

    return pos;
}

// uc_orig: link_facet_to_mapwho (fallen/Source/build2.cpp)
// Inserts dfacet 'facet' into the ColVectHead linked list of lo-res cell (mx, mz).
// The last entry in each run is stored as negative (sentinel), all others positive.
static void link_facet_to_mapwho(SLONG mx, SLONG mz, SLONG facet)
{
    SLONG index;
    SLONG pos;

    if (mx < 0 || mx > PAP_SIZE_LO || mz < 0 || mz > PAP_SIZE_LO)
        return;

    facet_link_count++;

    if (facet_link_count >= MAX_FACET_LINK)
        return;

    if ((index = PAP_2LO(mx, mz).ColVectHead)) {
        pos = create_extra_facet(facet, index);
        if (pos) {
            PAP_2LO(mx, mz).ColVectHead = pos;
            facet_links[pos] = facet;
        } else {
            ASSERT(0);
        }
    } else {
        pos = find_empty_facet_block(1);
        if (pos) {
            PAP_2LO(mx, mz).ColVectHead = pos;
            facet_links[pos] = -facet; // negative = only/last entry
        } else {
            ASSERT(0);
        }
    }
}

// uc_orig: add_facet_to_map (fallen/Source/build2.cpp)
void add_facet_to_map(SLONG facet)
{
    SLONG x1, z1, x2, z2, dx, dz;
    struct DFacet* p_f;

    p_f = &dfacets[facet];

    x1 = p_f->x[0] << 8;
    x2 = p_f->x[1] << 8;
    z1 = p_f->z[0] << 8;
    z2 = p_f->z[1] << 8;

    dx = x2 - x1;
    dz = z2 - z1;

    if (dx == 0 && dz == 0)
        return;

    switch (p_f->FacetType) {
    case STOREY_TYPE_LADDER: {
        SLONG y, extra_height;
        calc_ladder_ends(&x1, &z1, &x2, &z2);
    } break;
    }

    {
        SLONG x;
        SLONG z;

        SLONG mx;
        SLONG mz;

        SLONG end_mx;
        SLONG end_mz;

        SLONG frac;

        SLONG xfrac;
        SLONG zfrac;

        dx = x2 - x1;
        dz = z2 - z1;

        SLONG adx = abs(dx);
        SLONG adz = abs(dz);

        mx = x1 >> PAP_SHIFT_LO;
        mz = z1 >> PAP_SHIFT_LO;
        end_mx = x2 >> PAP_SHIFT_LO;
        end_mz = z2 >> PAP_SHIFT_LO;

        xfrac = x1 & ((1 << PAP_SHIFT_LO) - 1);
        zfrac = z1 & ((1 << PAP_SHIFT_LO) - 1);

        if (adx > adz) {
            frac = (dz << PAP_SHIFT_LO) / dx;

            if (dx > 0) {
                z = z1;
                z -= frac * xfrac >> PAP_SHIFT_LO;
            } else {
                z = z1;
                z += frac * ((1 << PAP_SHIFT_LO) - xfrac) >> PAP_SHIFT_LO;
            }

            while (1) {
                if (WITHIN(mx, 0, PAP_SIZE_LO - 1) && WITHIN(mz, 0, PAP_SIZE_LO - 1))
                    link_facet_to_mapwho(mx, mz, facet);

                if (mx == end_mx && mz == end_mz)
                    return;

                if (dx > 0)
                    z += frac;
                else
                    z -= frac;

                if ((z >> PAP_SHIFT_LO) != mz) {
                    mz = z >> PAP_SHIFT_LO;
                    if (WITHIN(mx, 0, PAP_SIZE_LO - 1) && WITHIN(mz, 0, PAP_SIZE_LO - 1))
                        link_facet_to_mapwho(mx, mz, facet);
                }

                if (dx > 0) {
                    mx += 1;
                    if (mx > end_mx)
                        return;
                } else {
                    mx -= 1;
                    if (mx < end_mx)
                        return;
                }
            }
        } else {
            frac = (dx << PAP_SHIFT_LO) / dz;

            if (dz > 0) {
                x = x1;
                x -= frac * zfrac >> PAP_SHIFT_LO;
            } else {
                x = x1;
                x += frac * ((1 << PAP_SHIFT_LO) - zfrac) >> PAP_SHIFT_LO;
            }

            while (1) {
                if (WITHIN(mx, 0, PAP_SIZE_LO - 1) && WITHIN(mz, 0, PAP_SIZE_LO - 1))
                    link_facet_to_mapwho(mx, mz, facet);

                if (mx == end_mx && mz == end_mz)
                    return;

                if (dz > 0)
                    x += frac;
                else
                    x -= frac;

                if ((x >> PAP_SHIFT_LO) != mx) {
                    mx = x >> PAP_SHIFT_LO;
                    if (WITHIN(mx, 0, PAP_SIZE_LO - 1) && WITHIN(mz, 0, PAP_SIZE_LO - 1))
                        link_facet_to_mapwho(mx, mz, facet);
                }

                if (dz > 0) {
                    mz += 1;
                    if (mz > end_mz)
                        return;
                } else {
                    mz -= 1;
                    if (mz < end_mz)
                        return;
                }
            }
        }
    }
}

// uc_orig: process_facet (fallen/Source/build2.cpp)
// Registers a facet in the PAP map unless it is an interior facet type.
// Interior facets (INSIDE, OINSIDE, INSIDE_DOOR) are invisible from outside and
// should not participate in external collision queries.
static void process_facet(SLONG facet)
{
    if (dfacets[facet].FacetType != STOREY_TYPE_INSIDE)
        if (dfacets[facet].FacetType != STOREY_TYPE_OINSIDE)
            if (dfacets[facet].FacetType != STOREY_TYPE_INSIDE_DOOR)
                add_facet_to_map(facet);
}

// uc_orig: process_building (fallen/Source/build2.cpp)
// Registers all non-invisible facets of a DBuilding in the PAP map.
// Special case: CRATE_IN type buildings skip the invisible check so that
// their floor facets still participate in collision even when marked invisible.
static void process_building(SLONG build)
{
    SLONG c0;
    struct DBuilding* p_build = &dbuildings[build];

    for (c0 = p_build->StartFacet; c0 < p_build->EndFacet; c0++) {
        if (dfacets[c0].FacetFlags & FACET_FLAG_INVISIBLE) {
            if (dbuildings[dfacets[c0].Building].Type == BUILDING_TYPE_CRATE_IN) {
                // Must add to map even when invisible — removing it causes Poshetas' floor to vanish.
            } else {
                continue;
            }
        }
        process_facet(c0);
    }
}

// uc_orig: clear_colvects (fallen/Source/build2.cpp)
static void clear_colvects(void)
{
    SLONG x, z;
    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            PAP_2LO(x, z).ColVectHead = 0;
            PAP_2LO(x, z).Walkable = 0;
        }
}

// uc_orig: clear_facet_links (fallen/Source/build2.cpp)
static void clear_facet_links(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_FACET_LINK; c0++)
        facet_links[c0] = 0;
}

// uc_orig: attach_walkable_to_map (fallen/Source/build2.cpp)
void attach_walkable_to_map(SLONG face)
{
    SLONG x = 0, z = 0;
    SLONG c0;

    if (face > 0) {
        for (c0 = 0; c0 < 4; c0++) {
            x += prim_points[prim_faces4[face].Points[c0]].X;
            z += prim_points[prim_faces4[face].Points[c0]].Z;
        }

        x >>= 2;
        z >>= 2;

        x >>= PAP_SHIFT_LO;
        z >>= PAP_SHIFT_LO;

        SATURATE(x, 0, PAP_SIZE_LO - 1);
        SATURATE(z, 0, PAP_SIZE_LO - 1);

        prim_faces4[face].WALKABLE = PAP_2LO(x, z).Walkable;
        PAP_2LO(x, z).Walkable = face;
    } else {
        roof_faces4[-face].Next = PAP_2LO((roof_faces4[-face].RX & 127) >> 2, (roof_faces4[-face].RZ & 127) >> 2).Walkable;
        PAP_2LO((roof_faces4[-face].RX & 127) >> 2, (roof_faces4[-face].RZ & 127) >> 2).Walkable = face;
    }
}

// uc_orig: remove_walkable_from_map (fallen/Source/build2.cpp)
void remove_walkable_from_map(SLONG face)
{
    SLONG x = 0, z = 0;
    SLONG c0;

    SWORD* prev;
    SWORD next;
    SWORD count = 50;

    for (c0 = 0; c0 < 4; c0++) {
        x += prim_points[prim_faces4[face].Points[c0]].X;
        z += prim_points[prim_faces4[face].Points[c0]].Z;
    }

    x >>= 2;
    z >>= 2;

    x >>= PAP_SHIFT_LO;
    z >>= PAP_SHIFT_LO;

    SATURATE(x, 0, PAP_SIZE_LO - 1);
    SATURATE(z, 0, PAP_SIZE_LO - 1);

    prev = &PAP_2LO(x, z).Walkable;
    next = PAP_2LO(x, z).Walkable;

    while (count--) {
        if (next == face) {
            if (face < 0)
                *prev = roof_faces4[-next].Next;
            else
                *prev = prim_faces4[next].WALKABLE;
            return;
        }
        if (next < 0) {
            prev = &roof_faces4[-next].Next;
            next = roof_faces4[-next].Next;
        } else {
            prev = &prim_faces4[next].WALKABLE;
            next = prim_faces4[next].WALKABLE;
        }
    }
    ASSERT(0);
}

// ---- facet_is_solid / compare_facets helpers for mark_naughty_facets ----

// uc_orig: facet_is_solid (fallen/Source/build2.cpp)
// Returns true if the facet is a solid, one-sided wall that can occlude another facet.
// Two-sided, transparent, or non-wall facet types are not considered solid.
static bool facet_is_solid(const DFacet* pf)
{
    if (pf->FacetFlags & FACET_FLAG_2SIDED)
        return false;
    if (pf->FacetFlags & FACET_FLAG_2TEXTURED)
        return false;

    if ((pf->FacetType == STOREY_TYPE_JUST_COLLISION) ||
        (pf->FacetType == STOREY_TYPE_FENCE) ||
        (pf->FacetType == STOREY_TYPE_FENCE_FLAT) ||
        (pf->FacetType == STOREY_TYPE_FENCE_BRICK) ||
        (pf->FacetType == STOREY_TYPE_INSIDE) ||
        (pf->FacetType == STOREY_TYPE_INSIDE_DOOR) ||
        (pf->FacetType == STOREY_TYPE_OUTSIDE_DOOR) ||
        (pf->FacetType == STOREY_TYPE_LADDER) ||
        (pf->FacetType == STOREY_TYPE_CABLE)) {
        return false;
    }

    return true;
}

// uc_orig: compare_facets (fallen/Source/build2.cpp)
// Determines if two parallel, collinear, solid facets occlude each other.
// Returns: 1 = remove pf1, 2 = remove pf2, 3 = remove both, 0 = no occlusion.
static int compare_facets(const DFacet* pf1, const DFacet* pf2)
{
    SLONG dx1 = pf1->x[1] - pf1->x[0];
    SLONG dz1 = pf1->z[1] - pf1->z[0];
    SLONG dx2 = pf2->x[1] - pf2->x[0];
    SLONG dz2 = pf2->z[1] - pf2->z[0];

    // Must be parallel.
    if (dx1 * dz2 != dx2 * dz1)
        return 0;

    // Must be collinear.
    if (!dz1) {
        if (pf1->z[0] != pf2->z[0])
            return 0;
    } else if (!dx1) {
        if (pf1->x[0] != pf2->x[0])
            return 0;
    } else {
        SLONG lhs = (pf1->z[0] * dx1 - pf1->x[0] * dz1) * dx2;
        SLONG rhs = (pf2->z[0] * dx2 - pf2->x[0] * dz2) * dx1;
        if (lhs != rhs)
            return 0;
    }

    // Project onto the longer axis for containment tests.
    SLONG s1, e1, s2, e2;
    if (abs(dx1) > abs(dz1)) {
        s1 = pf1->x[0]; e1 = pf1->x[1];
        s2 = pf2->x[0]; e2 = pf2->x[1];
    } else {
        s1 = pf1->z[0]; e1 = pf1->z[1];
        s2 = pf2->z[0]; e2 = pf2->z[1];
    }

    int rc = 0;
    if (s1 == e1) rc += 1;
    if (s2 == e2) rc += 2;
    if (rc) return rc;

    SLONG sgn1 = s1 - e1;
    SLONG sgn2 = s2 - e2;
    bool sameway = ((sgn1 ^ sgn2) >= 0);

    SLONG ys1 = pf1->Y[0], ye1 = pf1->Y[1];
    SLONG ys2 = pf2->Y[0], ye2 = pf2->Y[1];
    SLONG h1 = (pf1->Height / 4) * pf1->BlockHeight * 16;
    SLONG h2 = (pf2->Height / 4) * pf2->BlockHeight * 16;

    if (s1 > e1) { SWAP(s1, e1); SWAP(ys1, ye1); }
    if (s2 > e2) { SWAP(s2, e2); SWAP(ys2, ye2); }

    if ((s1 == s2) && (e1 == e2) && (ys1 == ys2) && (ye1 == ye2) && (h1 == h2)) {
        if (sameway)
            return (rand() & 128) ? 1 : 2;
        else
            return 3;
    }

    if ((s1 == s2) && (e1 == e2)) {
        if ((ys1 <= ys2) && (ys1 + h1 >= ys2 + h2) && (ye1 <= ye2) && (ye1 + h1 >= ye2 + h2))
            return 2;
        if ((ys2 <= ys1) && (ys2 + h2 >= ys1 + h1) && (ye2 <= ye1) && (ye2 + h2 >= ye1 + h1))
            return 1;
        return 0;
    }

    if ((s1 <= s2) && (e1 >= e2)) {
        ys1 += (s2 - s1) * (ye1 - ys1) / (e1 - s1);
        s1 = s2;
        ye1 += (e2 - e1) * (ye1 - ys1) / (e1 - s1);
        e1 = e2;
        if ((ys1 <= ys2) && (ys1 + h1 >= ys2 + h2) && (ye1 <= ye2) && (ye1 + h1 >= ye2 + h2))
            return 2;
        return 0;
    }

    if ((s2 <= s1) && (e2 >= e1)) {
        ys2 += (s1 - s2) * (ye2 - ys2) / (e2 - s2);
        s2 = s1;
        ye2 += (e1 - e2) * (ye2 - ys2) / (e2 - s2);
        e2 = e1;
        if ((ys2 <= ys1) && (ys2 + h2 >= ys1 + h1) && (ye2 <= ye1) && (ye2 + h2 >= ye1 + h1))
            return 1;
        return 0;
    }

    return 0;
}

// uc_orig: mark_naughty_facets (fallen/Source/build2.cpp)
// Marks facets as FACET_FLAG_INVISIBLE when they are completely hidden by another
// solid facet. Zero-length facets are always marked invisible.
static void mark_naughty_facets(void)
{
    SLONG ii, jj;

    // Start all facets as live.
    for (ii = 0; ii < next_dfacet; ii++)
        dfacets[ii].FacetFlags &= ~FACET_FLAG_INVISIBLE;

    for (ii = 0; ii < next_dfacet; ii++) {
        DFacet* pf1 = &dfacets[ii];

        if (pf1->x[0] == pf1->x[1] && pf1->z[0] == pf1->z[1]) {
            pf1->FacetFlags |= FACET_FLAG_INVISIBLE;
            continue;
        }
        if (pf1->FacetFlags & FACET_FLAG_INVISIBLE)
            continue;
        if (!facet_is_solid(pf1))
            continue;

        for (jj = ii + 1; jj < next_dfacet; jj++) {
            DFacet* pf2 = &dfacets[jj];

            if (pf2->x[0] == pf2->x[1] && pf2->z[0] == pf2->z[1]) {
                pf2->FacetFlags |= FACET_FLAG_INVISIBLE;
                continue;
            }
            if (pf2->FacetFlags & FACET_FLAG_INVISIBLE)
                continue;
            if (!facet_is_solid(pf2))
                continue;

            switch (compare_facets(pf1, pf2)) {
            case 1: pf1->FacetFlags |= FACET_FLAG_INVISIBLE; break;
            case 2: pf2->FacetFlags |= FACET_FLAG_INVISIBLE; break;
            case 3:
                pf1->FacetFlags |= FACET_FLAG_INVISIBLE;
                pf2->FacetFlags |= FACET_FLAG_INVISIBLE;
                break;
            }
        }
    }
}

// uc_orig: build_quick_city (fallen/Source/build2.cpp)
void build_quick_city(void)
{
    SLONG c0;

    clear_colvects();
    clear_facet_links();

    facet_link_count = 0;
    next_facet_link = 1;

    mark_naughty_facets();

    for (c0 = 1; c0 < next_dbuilding; c0++)
        process_building(c0);

    garbage_collection();

    for (c0 = 1; c0 < next_dwalkable; c0++) {
        SLONG face;
        ASSERT(dwalkables[c0].StartFace4 <= next_roof_face4);
        ASSERT(dwalkables[c0].EndFace4 <= next_roof_face4);

        for (face = dwalkables[c0].StartFace4; face < dwalkables[c0].EndFace4; face++)
            attach_walkable_to_map(-face);
    }
}

// uc_orig: BUILD_car_facets (fallen/Source/build2.cpp)
void BUILD_car_facets(void)
{
    for (int ii = 0; ii < next_dfacet; ii++) {
        DFacet* pf = &dfacets[ii];

        if (pf->FacetFlags & FACET_FLAG_INVISIBLE)
            continue;
        if (pf->FacetType == STOREY_TYPE_CABLE)
            continue;

        MAV_remove_facet_car(pf->x[0], pf->z[0], pf->x[1], pf->z[1]);
    }
}
