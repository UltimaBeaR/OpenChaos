#ifndef WORLD_MAP_ROAD_GLOBALS_H
#define WORLD_MAP_ROAD_GLOBALS_H

#include "world/map/road.h"

// uc_orig: ROAD_node (fallen/Source/road.cpp)
extern ROAD_Node* ROAD_node;

// uc_orig: ROAD_node_upto (fallen/Source/road.cpp)
extern SLONG ROAD_node_upto;

// uc_orig: ROAD_edge (fallen/Source/road.cpp)
extern UBYTE* ROAD_edge;

// uc_orig: ROAD_edge_upto (fallen/Source/road.cpp)
extern UWORD ROAD_edge_upto;

// uc_orig: ROAD_mapsquare_type (fallen/Source/road.cpp)
extern UBYTE ROAD_mapsquare_type[512 / 4];

#endif // WORLD_MAP_ROAD_GLOBALS_H
