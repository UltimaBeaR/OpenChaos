#ifndef WORLD_MAP_ROAD_H
#define WORLD_MAP_ROAD_H

#include "engine/core/types.h"

// uc_orig: ROAD_Node (fallen/Headers/road.h)
typedef struct
{
    UBYTE x;
    UBYTE z;
    UBYTE c[4]; // Other nodes connected to this one.
} ROAD_Node;

// uc_orig: ROAD_MAX_NODES (fallen/Headers/road.h)
#define ROAD_MAX_NODES 256

// uc_orig: ROAD_MAX_EDGES (fallen/Headers/road.h)
#define ROAD_MAX_EDGES 8

// uc_orig: ROAD_LANE (fallen/Headers/road.h)
// Offset from road center to drive along (in world units, fixed-point).
#define ROAD_LANE (0x100)

// uc_orig: JN_RADIUS_IN (fallen/Headers/road.h)
#define JN_RADIUS_IN 0x300

// uc_orig: JN_RADIUS_OUT (fallen/Headers/road.h)
#define JN_RADIUS_OUT 0x500

// uc_orig: AT_JUNCTION (fallen/Headers/road.h)
// Squared distance threshold: car is "at" the junction.
#define AT_JUNCTION (JN_RADIUS_IN * JN_RADIUS_IN)

// uc_orig: NEAR_JUNCTION (fallen/Headers/road.h)
// Squared distance threshold: car is "approaching" the junction.
#define NEAR_JUNCTION (JN_RADIUS_OUT * JN_RADIUS_OUT)

// Surface type codes returned by ROAD_get_mapsquare_type.
// uc_orig: ROAD_TYPE_TARMAC (fallen/Headers/road.h)
#define ROAD_TYPE_TARMAC 0
// uc_orig: ROAD_TYPE_GRASS (fallen/Headers/road.h)
#define ROAD_TYPE_GRASS 1
// uc_orig: ROAD_TYPE_DIRT (fallen/Headers/road.h)
#define ROAD_TYPE_DIRT 2
// uc_orig: ROAD_TYPE_SLIPPERY (fallen/Headers/road.h)
#define ROAD_TYPE_SLIPPERY 3

// uc_orig: ROAD_sink (fallen/Source/road.cpp)
// Applies camber and curb height adjustments to road squares in the PAP height map.
void ROAD_sink(void);

// uc_orig: ROAD_is_road (fallen/Source/road.cpp)
SLONG ROAD_is_road(SLONG map_x, SLONG map_z);

// uc_orig: ROAD_is_zebra (fallen/Source/road.cpp)
SLONG ROAD_is_zebra(SLONG map_x, SLONG map_z);

// uc_orig: ROAD_calc_mapsquare_type (fallen/Source/road.cpp)
// Precomputes the surface type (tarmac/grass/dirt/slippery) for every texture page.
// Slow — called once on level load.
void ROAD_calc_mapsquare_type(void);

// uc_orig: ROAD_get_mapsquare_type (fallen/Source/road.cpp)
SLONG ROAD_get_mapsquare_type(SLONG map_x, SLONG map_z);

// uc_orig: ROAD_wander_calc (fallen/Source/road.cpp)
// Builds the vehicle wander graph (road nodes + edge list) from map texture data.
void ROAD_wander_calc(void);

// uc_orig: ROAD_find (fallen/Source/road.cpp)
// Returns the nearest road segment (n1, n2) to the given world position.
void ROAD_find(SLONG world_x, SLONG world_z, SLONG* n1, SLONG* n2);

// uc_orig: ROAD_node_pos (fallen/Source/road.cpp)
void ROAD_node_pos(SLONG node, SLONG* world_x, SLONG* world_z);

// uc_orig: ROAD_node_degree (fallen/Source/road.cpp)
SLONG ROAD_node_degree(SLONG node);

// uc_orig: ROAD_nearest_node (fallen/Source/road.cpp)
SLONG ROAD_nearest_node(SLONG rn1, SLONG rn2, SLONG wx, SLONG wz, SLONG* nnd);

// uc_orig: ROAD_whereto_now (fallen/Source/road.cpp)
// Given that a vehicle just drove from n1 to n2, picks the next road segment to follow.
void ROAD_whereto_now(SLONG n1, SLONG n2, SLONG* wtn1, SLONG* wtn2);

// uc_orig: ROAD_get_dest (fallen/Source/road.cpp)
// Returns the world position a vehicle should aim for when driving segment n1 -> n2.
void ROAD_get_dest(SLONG n1, SLONG n2, SLONG* world_x, SLONG* world_z);

// uc_orig: ROAD_is_end_of_the_line (fallen/Source/road.cpp)
SLONG ROAD_is_end_of_the_line(SLONG n);

// uc_orig: ROAD_bend (fallen/Source/road.cpp)
// Returns cross-product sign: negative = left bend, positive = right bend, 0 = straight.
SLONG ROAD_bend(SLONG n1, SLONG n2, SLONG n3);

// uc_orig: ROAD_find_me_somewhere_to_appear (fallen/Source/road.cpp)
// Finds a road entry point far from the current position for vehicle re-spawn.
void ROAD_find_me_somewhere_to_appear(
    SLONG* world_x,
    SLONG* world_z,
    SLONG* nrn1,
    SLONG* nrn2,
    SLONG* ryaw);

// uc_orig: ROAD_signed_dist (fallen/Source/road.cpp)
// Returns signed lateral distance from road center (negative = wrong lane).
SLONG ROAD_signed_dist(SLONG n1, SLONG n2, SLONG world_x, SLONG world_z);

// uc_orig: ROAD_debug (fallen/Source/road.cpp)
void ROAD_debug(void);

#endif // WORLD_MAP_ROAD_H
