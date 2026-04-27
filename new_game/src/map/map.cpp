#include "game/game_types.h"

#include "map/map.h"

// uc_orig: init_map (fallen/Source/Map.cpp)
void init_map(void)
{
    memset((UBYTE*)MAP, 0, sizeof(MAP));
}
