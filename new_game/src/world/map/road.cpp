#include <string.h>

#include "world/map/road.h"
#include "world/map/road_globals.h"
#include "world/map/map.h"
#include "world/map/pap.h"
// Temporary: world/ should not depend on actors/ — pre-existing coupling from original (game.h -> Thing.h).
#include "actors/core/thing.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/resources/console.h"
#include "assets/texture.h"

// Temporary: elev.h not yet migrated — needed for ELEV_fname_map.
#include "fallen/Headers/elev.h"
// Temporary: mav.h not yet migrated — needed for MAVHEIGHT macro.
#include "ai/mav.h"

// TEXTURE_set is defined in texture.cpp (not yet migrated to new/).
extern SLONG TEXTURE_set;

// Road width in map squares — used for camber calculation.
// uc_orig: ROAD_WIDTH (fallen/Source/road.cpp)
#define ROAD_WIDTH 5

// ============================================================
// Private helpers — only used within this file.
// ============================================================

// uc_orig: ROAD_find_node (fallen/Source/road.cpp)
// Returns index of the node at (x, z), creating one if needed.
static SLONG ROAD_find_node(SLONG x, SLONG z)
{
    SLONG i;

    for (i = 1; i < ROAD_node_upto; i++) {
        if (ROAD_node[i].x == x && ROAD_node[i].z == z) {
            return i;
        }
    }

    ASSERT(WITHIN(ROAD_node_upto, 1, ROAD_MAX_NODES - 1));

    ROAD_node[ROAD_node_upto].x = x;
    ROAD_node[ROAD_node_upto].z = z;

    return ROAD_node_upto++;
}

// uc_orig: ROAD_connect (fallen/Source/road.cpp)
// Creates a bidirectional connection between two road nodes.
static void ROAD_connect(SLONG n1, SLONG n2)
{
    SLONG i;

    ROAD_Node* rn1;
    ROAD_Node* rn2;

    ASSERT(WITHIN(n1, 1, ROAD_MAX_NODES - 1));
    ASSERT(WITHIN(n2, 1, ROAD_MAX_NODES - 1));

    rn1 = &ROAD_node[n1];
    rn2 = &ROAD_node[n2];

    for (i = 0; i < 4; i++) {
        if (rn1->c[i] == n2) {
            goto connected_2_to_1;
        }

        if (rn1->c[i] == 0) {
            rn1->c[i] = n2;
            goto connected_2_to_1;
        }
    }

    ASSERT(0);

connected_2_to_1:;

    for (i = 0; i < 4; i++) {
        if (rn2->c[i] == n1) {
            return;
        }

        if (rn2->c[i] == 0) {
            rn2->c[i] = n1;
            return;
        }
    }

    ASSERT(0);
}

// uc_orig: ROAD_disconnect (fallen/Source/road.cpp)
// Removes the bidirectional connection between two road nodes.
static void ROAD_disconnect(SLONG n1, SLONG n2)
{
    SLONG i;
    SLONG j;

    ASSERT(WITHIN(n1, 1, ROAD_node_upto - 1));
    ASSERT(WITHIN(n2, 1, ROAD_node_upto - 1));

    ROAD_Node* rn1 = &ROAD_node[n1];
    ROAD_Node* rn2 = &ROAD_node[n2];

    for (i = 0; i < 4; i++) {
        if (rn1->c[i] == n2) {
            for (j = i; j < 3; j++) {
                rn1->c[j] = rn1->c[j + 1];
            }

            rn1->c[3] = 0;

            goto found_2_from_1;
        }
    }

    ASSERT(0);

found_2_from_1:;

    for (i = 0; i < 4; i++) {
        if (rn2->c[i] == n1) {
            for (j = i; j < 3; j++) {
                rn2->c[j] = rn2->c[j + 1];
            }

            rn2->c[3] = 0;

            goto found_1_from_2;
        }
    }

    ASSERT(0);

found_1_from_2:;

    return;
}

// uc_orig: ROAD_intersect (fallen/Source/road.cpp)
// Finds a road segment that crosses the segment from (x1,z1) to (x2,z2).
// Returns UC_TRUE and fills *in1, *in2, *ix, *iz if an intersection was found.
// Roads must be axis-aligned (either x1==x2 or z1==z2).
static SLONG ROAD_intersect(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG* in1,
    SLONG* in2,
    SLONG* ix,
    SLONG* iz)
{
    SLONG i;
    SLONG j;

    ROAD_Node* rn;
    ROAD_Node* rm;

    SLONG minx;
    SLONG maxx;

    SLONG minz;
    SLONG maxz;

    for (i = 1; i < ROAD_node_upto; i++) {
        rn = &ROAD_node[i];

        // Two roads don't count as intersecting if they share an end point.
        if ((x1 == rn->x && z1 == rn->z) || (x2 == rn->x && z2 == rn->z)) {
            continue;
        }

        for (j = 0; j < 4; j++) {
            if (rn->c[j] == 0) {
                break;
            }

            if (rn->c[j] < i) {
                rm = &ROAD_node[rn->c[j]];

                if ((x1 == rm->x && z1 == rm->z) || (x2 == rm->x && z2 == rm->z)) {
                    continue;
                }

                if (x1 == x2) {
                    if (rn->x == rm->x) {
                        // Parallel roads can't intersect.
                    } else {
                        minx = rn->x;
                        maxx = rm->x;

                        if (minx > maxx) {
                            SWAP(minx, maxx);
                        }

                        if (WITHIN(x1, minx, maxx)) {
                            minz = z1;
                            maxz = z2;

                            if (minz > maxz) {
                                SWAP(minz, maxz);
                            }

                            if (WITHIN(rn->z, minz, maxz)) {
                                *in1 = i;
                                *in2 = rn->c[j];
                                *ix = x1;
                                *iz = rn->z;

                                return UC_TRUE;
                            }
                        }
                    }
                } else {
                    ASSERT(z1 == z2);

                    if (rn->z == rm->z) {
                        // Parallel roads can't intersect.
                    } else {
                        minz = rn->z;
                        maxz = rm->z;

                        if (minz > maxz) {
                            SWAP(minz, maxz);
                        }

                        if (WITHIN(z1, minz, maxz)) {
                            minx = x1;
                            maxx = x2;

                            if (minx > maxx) {
                                SWAP(minx, maxx);
                            }

                            if (WITHIN(rn->x, minx, maxx)) {
                                *in1 = i;
                                *in2 = rn->c[j];
                                *ix = rn->x;
                                *iz = z1;

                                return UC_TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    return UC_FALSE;
}

// uc_orig: ROAD_split (fallen/Source/road.cpp)
// Inserts a new node at (splitx, splitz) on the segment n1–n2, replacing it with two segments.
static void ROAD_split(SLONG n1, SLONG n2, SLONG splitx, SLONG splitz)
{
    SLONG sn = ROAD_find_node(splitx, splitz);

    if (sn == n1 || sn == n2) {
        // Splitting at an endpoint does nothing.
        return;
    }

    ASSERT(WITHIN(n1, 1, ROAD_node_upto - 1));
    ASSERT(WITHIN(n2, 1, ROAD_node_upto - 1));

    ROAD_disconnect(n1, n2);

    ROAD_connect(n1, sn);
    ROAD_connect(n2, sn);
}

// uc_orig: ROAD_add (fallen/Source/road.cpp)
// Adds a road segment from (x1,z1) to (x2,z2), splitting at any existing intersections.
static void ROAD_add(SLONG x1, SLONG z1, SLONG x2, SLONG z2)
{
    SLONG n1;
    SLONG n2;

    SLONG in1;
    SLONG in2;

    SLONG ix;
    SLONG iz;

    if (ROAD_node_upto >= ROAD_MAX_NODES - 4) {
        // Not enough nodes left for this to be safe.
        return;
    }

    if (x1 == x2 && z1 == z2) {
        return;
    }

    if ((x1 == 121 && z1 == 33) || (x2 == 121 && z2 == 33)) {
        // Don't add this road on "Cop Killers" / "Arms Breaker" mission.
        if (strstr(ELEV_fname_map, "gpost3.iam")) {
            return;
        }
    }

    n1 = ROAD_find_node(x1, z1);
    n2 = ROAD_find_node(x2, z2);

    if (!ROAD_intersect(x1, z1, x2, z2, &in1, &in2, &ix, &iz)) {
        ROAD_connect(n1, n2);
    } else {
        ROAD_split(in1, in2, ix, iz);

        ROAD_add(x1, z1, ix, iz);
        ROAD_add(x2, z2, ix, iz);
    }
}

// uc_orig: ROAD_is_middle (fallen/Source/road.cpp)
// Returns UC_TRUE if the given square is surrounded by road squares in a 5x5 area
// (i.e. it lies along the centre-line of a road, not at the edge).
static SLONG ROAD_is_middle(SLONG map_x, SLONG map_z)
{
    SLONG dx;
    SLONG dz;

    SLONG mx;
    SLONG mz;

    if (!WITHIN(map_x, 2, PAP_SIZE_HI - 3) || !WITHIN(map_z, 2, PAP_SIZE_HI - 3)) {
        return UC_FALSE;
    }

    if (!ROAD_is_road(map_x, map_z)) {
        return UC_FALSE;
    }

    if (!ROAD_is_road(map_x + 2, map_z + 2)) {
        return UC_FALSE;
    }

    for (dx = -2; dx <= +2; dx++)
        for (dz = -2; dz <= +2; dz++) {
            mx = map_x + dx;
            mz = map_z + dz;

            if (!ROAD_is_road(mx, mz)) {
                return UC_FALSE;
            }
        }

    return UC_TRUE;
}

// ============================================================
// Public functions
// ============================================================

// uc_orig: ROAD_node_pos (fallen/Source/road.cpp)
void ROAD_node_pos(
    SLONG node,
    SLONG* node_x,
    SLONG* node_z)
{
    ROAD_Node* rn;

    ASSERT(WITHIN(node, 1, ROAD_node_upto - 1));

    rn = &ROAD_node[node];

    *node_x = (rn->x << 8) + 0x80;
    *node_z = (rn->z << 8) + 0x80;
}

// uc_orig: ROAD_bend (fallen/Source/road.cpp)
SLONG ROAD_bend(SLONG n1, SLONG n2, SLONG n3)
{
    ASSERT(WITHIN(n1, 1, ROAD_node_upto - 1));
    ASSERT(WITHIN(n2, 1, ROAD_node_upto - 1));
    ASSERT(WITHIN(n3, 1, ROAD_node_upto - 1));

    ROAD_Node* rn1 = &ROAD_node[n1];
    ROAD_Node* rn2 = &ROAD_node[n2];
    ROAD_Node* rn3 = &ROAD_node[n3];

    SLONG x12 = rn2->x - rn1->x;
    SLONG z12 = rn2->z - rn1->z;

    SLONG x23 = rn3->x - rn2->x;
    SLONG z23 = rn3->z - rn2->z;

    return x12 * z23 - z12 * x23;
}

// uc_orig: ROAD_node_degree (fallen/Source/road.cpp)
SLONG ROAD_node_degree(SLONG node)
{
    ASSERT(WITHIN(node, 1, ROAD_node_upto - 1));

    ROAD_Node* rn = &ROAD_node[node];

    SLONG degree = 0;

    if (rn->c[0])
        degree++;
    if (rn->c[1])
        degree++;
    if (rn->c[2])
        degree++;
    if (rn->c[3])
        degree++;

    return degree;
}

// uc_orig: ROAD_nearest_node (fallen/Source/road.cpp)
SLONG ROAD_nearest_node(SLONG rn1, SLONG rn2, SLONG wx, SLONG wz, SLONG* nnd)
{
    ASSERT(WITHIN(rn1, 1, ROAD_node_upto - 1));
    ASSERT(WITHIN(rn2, 1, ROAD_node_upto - 1));

    ROAD_Node* prn1 = &ROAD_node[rn1];
    ROAD_Node* prn2 = &ROAD_node[rn2];

    SLONG x1 = (prn1->x << 8) + 0x80;
    SLONG z1 = (prn1->z << 8) + 0x80;
    SLONG x2 = (prn2->x << 8) + 0x80;
    SLONG z2 = (prn2->z << 8) + 0x80;

    SLONG d1 = (wx - x1) * (wx - x1) + (wz - z1) * (wz - z1);
    SLONG d2 = (wx - x2) * (wx - x2) + (wz - z2) * (wz - z2);

    if (d1 < d2) {
        *nnd = d1;
        return rn1;
    }

    *nnd = d2;
    return rn2;
}

// uc_orig: ROAD_sink (fallen/Source/road.cpp)
void ROAD_sink()
{
    SLONG dx;
    SLONG dz;

    SLONG mx;
    SLONG mz;

    SLONG dist;
    SLONG best_dist;

    SLONG page;

    PAP_Hi* ph;
    MapElement* me;

    for (mx = 0; mx < MAP_WIDTH - 1; mx++)
        for (mz = 0; mz < MAP_HEIGHT - 1; mz++) {
            if (ROAD_is_road(mx, mz)) {
                PAP_2HI(mx + 0, mz + 0).Flags |= PAP_FLAG_SINK_SQUARE;

                PAP_2HI(mx + 0, mz + 0).Flags |= PAP_FLAG_SINK_POINT;
                PAP_2HI(mx + 1, mz + 0).Flags |= PAP_FLAG_SINK_POINT;
                PAP_2HI(mx + 0, mz + 1).Flags |= PAP_FLAG_SINK_POINT;
                PAP_2HI(mx + 1, mz + 1).Flags |= PAP_FLAG_SINK_POINT;
            }
        }

    // Mark which PAP hi-points don't need the upper level.
    for (mx = 0; mx < MAP_WIDTH; mx++)
        for (mz = 0; mz < MAP_HEIGHT; mz++) {
            PAP_2HI(mx, mz).Flags |= PAP_FLAG_NOUPPER;
        }

    for (mx = 0; mx < MAP_WIDTH - 1; mx++)
        for (mz = 0; mz < MAP_HEIGHT - 1; mz++) {
            if (PAP_2HI(mx, mz).Flags & PAP_FLAG_SINK_SQUARE) {
            } else {
                PAP_2HI(mx + 0, mz + 0).Flags &= ~PAP_FLAG_NOUPPER;
                PAP_2HI(mx + 1, mz + 0).Flags &= ~PAP_FLAG_NOUPPER;
                PAP_2HI(mx + 0, mz + 1).Flags &= ~PAP_FLAG_NOUPPER;
                PAP_2HI(mx + 1, mz + 1).Flags &= ~PAP_FLAG_NOUPPER;
            }
        }

    // Apply camber: points close to a road edge get a small height boost.
    for (mx = 0; mx < MAP_WIDTH - 1; mx++)
        for (mz = 0; mz < MAP_HEIGHT - 1; mz++) {
            ph = &PAP_2HI(mx, mz);

            if (PAP_2HI(mx, mz).Flags & PAP_FLAG_SINK_POINT) {
                best_dist = 2;

                for (dx = -1; dx <= +1; dx++)
                    for (dz = -1; dz <= +1; dz++) {
                        if (WITHIN(mx + dx, 0, MAP_WIDTH - 1) && WITHIN(mz + dz, 0, MAP_HEIGHT - 1))

                            if (PAP_on_map_hi(mx + dx, mz + dz)) {
                                if (PAP_2HI(mx + dx, mz + dz).Flags & PAP_FLAG_NOUPPER) {
                                    // Not the edge of a road.
                                } else {
                                    dist = MAX(abs(dx), abs(dz));

                                    if (dist < best_dist) {
                                        best_dist = dist;
                                    }
                                }
                            }
                    }

                if (best_dist == 2) {
                    ph->Alt += 3;
                }
                if (best_dist == 1) {
                    ph->Alt += 2;
                }
            }
        }
}

// uc_orig: ROAD_is_road (fallen/Source/road.cpp)
SLONG ROAD_is_road(SLONG map_x, SLONG map_z)
{
    PAP_Hi* ph;
    SLONG num;

    if (!WITHIN(map_x, 0, PAP_SIZE_HI - 1) || !WITHIN(map_z, 0, PAP_SIZE_HI - 1)) {
        return UC_FALSE;
    }

    ph = &PAP_2HI(map_x, map_z);

    if (ph->Flags & PAP_FLAG_HIDDEN) {
        return UC_FALSE;
    }

    num = ph->Texture & 0x3ff;

    if (WITHIN(num, 323, 356) || ((TEXTURE_set == 7 || TEXTURE_set == 8) && (num == 35 || num == 36 || num == 39))) {
        return UC_TRUE;
    }

    return UC_FALSE;
}

// uc_orig: ROAD_is_zebra (fallen/Source/road.cpp)
SLONG ROAD_is_zebra(SLONG map_x, SLONG map_z)
{
    PAP_Hi* ph;
    SLONG num;

    if (!WITHIN(map_x, 0, PAP_SIZE_HI - 1) || !WITHIN(map_z, 0, PAP_SIZE_HI - 1)) {
        return UC_FALSE;
    }

    ph = &PAP_2HI(map_x, map_z);

    num = ph->Texture & 0x3ff;

    if (num == 333 || num == 334) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: ROAD_is_end_of_the_line (fallen/Source/road.cpp)
SLONG ROAD_is_end_of_the_line(SLONG n)
{
    ROAD_Node* rn;

    ASSERT(WITHIN(n, 1, ROAD_node_upto - 1));

    rn = &ROAD_node[n];

    if (rn->c[0] && rn->c[1] == NULL) {
        if (rn->x == 0 || rn->z == 0 || rn->x == PAP_SIZE_HI - 1 || rn->z == PAP_SIZE_HI - 1) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// uc_orig: ROAD_wander_calc (fallen/Source/road.cpp)
void ROAD_wander_calc()
{
    SLONG x;
    SLONG z;

    SLONG i;
    SLONG onedge;

    SLONG p1;
    SLONG p2;
    SLONG p1valid;
    SLONG p2valid;

    ROAD_Node* rn;

    ROAD_node_upto = 1;

    memset(ROAD_node, 0, sizeof(ROAD_Node) * ROAD_MAX_NODES);

    ROAD_edge_upto = 0;

    memset(ROAD_edge, 0, sizeof(UBYTE) * ROAD_MAX_EDGES);

    // Find all roads parallel to the z-axis.
    for (x = 2; x < PAP_SIZE_HI - 2; x++) {
        p1valid = UC_FALSE;
        p2valid = UC_FALSE;

        for (z = 1; z < PAP_SIZE_HI - 1; z++) {
            if (ROAD_is_middle(x, z)) {
                if (!p1valid) {
                    p1valid = UC_TRUE;
                    p1 = z;
                } else {
                    p2valid = UC_TRUE;
                    p2 = z;
                }
            } else {
                if (p1valid && p2valid) {
                    if (abs(p2 - p1) > 2) {
                        ROAD_add(x, p1, x, p2);
                    }
                }

                p1valid = UC_FALSE;
                p2valid = UC_FALSE;
            }
        }
    }

    // Find all roads parallel to the x-axis.
    for (z = 2; z < PAP_SIZE_HI - 2; z++) {
        p1valid = UC_FALSE;
        p2valid = UC_FALSE;

        for (x = 1; x < PAP_SIZE_HI - 1; x++) {
            if (ROAD_is_middle(x, z)) {
                if (!p1valid) {
                    p1valid = UC_TRUE;
                    p1 = x;
                } else {
                    p2valid = UC_TRUE;
                    p2 = x;
                }
            } else {
                if (p1valid && p2valid) {
                    if (abs(p2 - p1) > 2) {
                        ROAD_add(p1, z, p2, z);
                    }
                }

                p1valid = UC_FALSE;
                p2valid = UC_FALSE;
            }
        }
    }

    // Find edge nodes (single-connection nodes near the map edge) and record them.
    for (i = 1; i < ROAD_node_upto; i++) {
        rn = &ROAD_node[i];

        if (rn->c[0] && rn->c[1] == NULL) {
            onedge = UC_FALSE;

            if (rn->x == 2) {
                rn->x = 0;
                onedge = UC_TRUE;
            }
            if (rn->z == 2) {
                rn->z = 0;
                onedge = UC_TRUE;
            }

            if (rn->x == PAP_SIZE_HI - 3) {
                rn->x = PAP_SIZE_HI - 1;
                onedge = UC_TRUE;
            }
            if (rn->z == PAP_SIZE_HI - 3) {
                rn->z = PAP_SIZE_HI - 1;
                onedge = UC_TRUE;
            }

            if (onedge) {
                if (ROAD_edge_upto < ROAD_MAX_EDGES) {
                    ROAD_edge[ROAD_edge_upto++] = i;
                }
            }
        }
    }
}

// uc_orig: ROAD_find_me_somewhere_to_appear (fallen/Source/road.cpp)
void ROAD_find_me_somewhere_to_appear(
    SLONG* world_x,
    SLONG* world_z,
    SLONG* nrn1,
    SLONG* nrn2,
    SLONG* nyaw)
{
    UBYTE i;
    UBYTE e;
    SLONG dx;
    SLONG dz;

    ROAD_Node* rn;

    *nrn1 = 0;
    *nrn2 = 0;

    for (i = 0; i < ROAD_edge_upto; i++) {
        ASSERT(WITHIN(ROAD_edge[i], 1, ROAD_node_upto - 1));
        ASSERT(WITHIN(ROAD_node[ROAD_edge[i]].c[0], 1, ROAD_node_upto - 1));

        ROAD_Node* rn1 = &ROAD_node[ROAD_edge[i]];
        ROAD_Node* rn2 = &ROAD_node[rn1->c[0]];

        ASSERT(rn1->x == rn2->x || rn1->z == rn2->z);
    }

    if (ROAD_edge_upto) {
        for (i = 0; i < 32; i++) {
            e = Random() & (ROAD_MAX_EDGES - 1);

            if (e >= ROAD_edge_upto) {
                continue;
            }

            ASSERT(WITHIN(ROAD_edge[e], 1, ROAD_node_upto - 1));

            rn = &ROAD_node[ROAD_edge[e]];

            dx = abs((rn->x << 8) - *world_x);
            dz = abs((rn->z << 8) - *world_z);

            if (dx + dz > 0x1000) {
                if (THING_find_nearest(
                        rn->x << 8,
                        MAVHEIGHT(rn->x, rn->z) << 6,
                        rn->z << 8,
                        0x400,
                        1 << CLASS_VEHICLE)) {
                    // Another vehicle is too close to this node — skip it.
                } else {

                    *nrn1 = ROAD_edge[e];
                    *nrn2 = rn->c[0];

                    break;
                }
            }
        }

        if (*nrn1 == 0) {
            CONSOLE_text("Road node alert!");

            ASSERT(WITHIN(ROAD_edge[0], 1, ROAD_node_upto - 1));

            rn = &ROAD_node[ROAD_edge[0]];

            *nrn1 = ROAD_edge[0];
            *nrn2 = rn->c[0];
        }

        // Position the vehicle at the road entry point.
        *world_x = rn->x << 8;
        *world_z = rn->z << 8;

        *world_x += 0x80;
        *world_z += 0x80;

        dx = ROAD_node[*nrn2].x - ROAD_node[*nrn1].x;
        dz = ROAD_node[*nrn2].z - ROAD_node[*nrn1].z;

        *nyaw = -Arctan(dx, dz);
        *nyaw &= 2047;

        *world_x -= SIGN(dz) << 8;
        *world_z += SIGN(dx) << 8;

        ASSERT(
            ROAD_node[*nrn1].x == ROAD_node[*nrn2].x || ROAD_node[*nrn1].z == ROAD_node[*nrn2].z);

    } else {
        ASSERT(0);
    }
}

// uc_orig: ROAD_debug (fallen/Source/road.cpp)
void ROAD_debug()
{
    SLONG i;
    SLONG j;

    SLONG nx;
    SLONG ny;
    SLONG nz;

    SLONG mx;
    SLONG my;
    SLONG mz;

    ROAD_Node* rn;

    for (i = 1; i < ROAD_node_upto; i++) {
        rn = &ROAD_node[i];

        ROAD_node_pos(i, &nx, &nz);

        ny = PAP_calc_map_height_at(nx, nz);

        AENG_world_line(
            nx, ny, nz,
            32,
            0xffffff,
            nx, ny + 0x60, nz,
            0,
            0x000000,
            UC_FALSE);

        for (j = 0; j < 4; j++) {
            if (rn->c[j]) {
                ROAD_node_pos(rn->c[j], &mx, &mz);

                my = PAP_calc_map_height_at(mx, mz);

                AENG_world_line_infinite(
                    nx, ny + 0x10, nz,
                    32,
                    0x8888ff,
                    mx, my + 0x10, mz,
                    0,
                    0x008800,
                    UC_FALSE);
            }
        }
    }
}

// uc_orig: ROAD_signed_dist (fallen/Source/road.cpp)
SLONG ROAD_signed_dist(
    SLONG n1,
    SLONG n2,
    SLONG world_x,
    SLONG world_z)
{
    SLONG dist = 0x10000; // Very far — but not unreasonable as a default.

    ASSERT(WITHIN(n1, 1, ROAD_node_upto - 1));
    ASSERT(WITHIN(n2, 1, ROAD_node_upto - 1));

    ROAD_Node* rn1 = &ROAD_node[n1];
    ROAD_Node* rn2 = &ROAD_node[n2];

    if (rn1->x == rn2->x) {
        SLONG minz = rn1->z;
        SLONG maxz = rn2->z;

        if (minz > maxz) {
            SWAP(minz, maxz);
        }

        if (WITHIN(world_z >> 8, minz - 2, maxz + 2)) {
            dist = world_x - ((rn1->x << 8) + 0x80);

            if (rn1->z < rn2->z) {
                dist = -dist;
            }
        }
    } else {
        ASSERT(rn1->z == rn2->z);

        SLONG minx = rn1->x;
        SLONG maxx = rn2->x;

        if (minx > maxx) {
            SWAP(minx, maxx);
        }

        if (WITHIN(world_x >> 8, minx - 2, maxx + 2)) {
            dist = world_z - ((rn1->z << 8) + 0x80);

            if (rn1->x > rn2->x) {
                dist = -dist;
            }
        }
    }

    return dist;
}

// uc_orig: ROAD_find (fallen/Source/road.cpp)
void ROAD_find(
    SLONG world_x,
    SLONG world_z,
    SLONG* n1,
    SLONG* n2)
{
    SLONG i;
    SLONG j;

    ROAD_Node* rn;

    SLONG dist;

    SLONG best_dist = UC_INFINITY;
    SLONG best_n1 = NULL;
    SLONG best_n2 = NULL;

    for (i = 1; i < ROAD_node_upto; i++) {
        rn = &ROAD_node[i];

        for (j = 0; j < 4; j++) {
            if (rn->c[j] == 0) {
                break;
            }

            if (rn->c[j] < i) {
                dist = abs(0x100 - ROAD_signed_dist(i, rn->c[j], world_x, world_z));

                if (dist < best_dist) {
                    best_dist = dist;
                    best_n1 = i;
                    best_n2 = rn->c[j];
                }
            }
        }
    }

    *n1 = best_n1;
    *n2 = best_n2;
}

// uc_orig: ROAD_whereto_now (fallen/Source/road.cpp)
void ROAD_whereto_now(
    SLONG n1,
    SLONG n2,
    SLONG* wtn1,
    SLONG* wtn2)
{
    SLONG i;
    SLONG score;

    SLONG best_node = NULL;
    SLONG best_score = 0;

    ASSERT(WITHIN(n1, 1, ROAD_node_upto - 1));
    ASSERT(WITHIN(n2, 1, ROAD_node_upto - 1));

    ROAD_Node* rn = &ROAD_node[n2];

    for (i = 0; i < 4; i++) {
        if (rn->c[i]) {
            if (rn->c[i] == n1) {
                score = 10;
            } else {
                score = Random() & 0xff;
                score += 20;
            }

            if (score > best_score) {
                best_score = score;
                best_node = rn->c[i];
            }
        }
    }

    *wtn1 = n2;
    *wtn2 = best_node;
}

// uc_orig: ROAD_get_dest (fallen/Source/road.cpp)
void ROAD_get_dest(
    SLONG n1,
    SLONG n2,
    SLONG* world_x,
    SLONG* world_z)
{
    SLONG dx;
    SLONG dz;

    ASSERT(WITHIN(n1, 1, ROAD_node_upto - 1));
    ASSERT(WITHIN(n2, 1, ROAD_node_upto - 1));

    ROAD_Node* rn1 = &ROAD_node[n1];
    ROAD_Node* rn2 = &ROAD_node[n2];

    dx = SIGN(rn2->x - rn1->x) * ROAD_LANE;
    dz = SIGN(rn2->z - rn1->z) * ROAD_LANE;

    *world_x = ((rn2->x << 8) + 0x80) - dz;
    *world_z = ((rn2->z << 8) + 0x80) + dx;

    if (rn2->x == 0) {
        *world_x -= 0x400;
    }
    if (rn2->z == 0) {
        *world_z -= 0x400;
    }

    if (rn2->x == PAP_SIZE_HI - 1) {
        *world_x += 0x400;
    }
    if (rn2->z == PAP_SIZE_HI - 1) {
        *world_z += 0x400;
    }
}

// uc_orig: ROAD_calc_mapsquare_type (fallen/Source/road.cpp)
void ROAD_calc_mapsquare_type()
{
    SLONG mx;
    SLONG mz;
    SLONG page;
    SLONG offset;
    SLONG index;
    SLONG look;

    PAP_Hi* ph;

    UBYTE done[512];

    memset(done, 0, sizeof(done));

    for (mx = 0; mx < PAP_SIZE_HI; mx++)
        for (mz = 0; mz < PAP_SIZE_HI; mz++) {
            ph = &PAP_2HI(mx, mz);

            page = ph->Texture & 0x3ff;

            if (!WITHIN(page, 0, 511)) {
                continue;
            }

            if (!done[page]) {
                done[page] = UC_TRUE;

                if (ROAD_is_road(mx, mz)) {
                    look = TEXTURE_LOOK_ROAD;
                } else {
                    look = TEXTURE_looks_like(page);
                }

                switch (look) {
                case TEXTURE_LOOK_ROAD:
                    look = ROAD_TYPE_TARMAC;
                    break;

                case TEXTURE_LOOK_GRASS:
                    look = ROAD_TYPE_GRASS;
                    break;

                case TEXTURE_LOOK_DIRT:
                    look = ROAD_TYPE_DIRT;
                    break;

                case TEXTURE_LOOK_SLIPPERY:
                    look = ROAD_TYPE_SLIPPERY;
                    break;

                default:
                    ASSERT(0);
                    look = ROAD_TYPE_TARMAC;
                    break;
                }

                offset = (page & 0x3) << 1;
                index = page >> 2;

                ROAD_mapsquare_type[index] &= ~(0x3 << offset);
                ROAD_mapsquare_type[index] |= (look << offset);
            }
        }
}

// uc_orig: ROAD_get_mapsquare_type (fallen/Source/road.cpp)
SLONG ROAD_get_mapsquare_type(SLONG mx, SLONG mz)
{
    SLONG page;
    SLONG look;
    SLONG offset;
    SLONG index;

    PAP_Hi* ph;

    ph = &PAP_2HI(mx, mz);

    page = ph->Texture & 0x3ff;

    if (WITHIN(page, 0, 511)) {
        offset = (page & 0x3) << 1;
        index = page >> 2;

        look = ROAD_mapsquare_type[index] >> offset;
        look &= 0x3;
    } else {
        look = ROAD_TYPE_TARMAC;
    }

    return look;
}
