#include "engine/platform/uc_common.h"
#include "game/game_types.h"

#include "map/map.h"
#include "map/map_globals.h"

// uc_orig: init_map (fallen/Source/Map.cpp)
void init_map(void)
{
    memset((UBYTE*)MAP, 0, sizeof(MAP));
}

// uc_orig: MAP_light_get_height (fallen/Source/Map.cpp)
SLONG MAP_light_get_height(SLONG x, SLONG z)
{
    return 0;
}

// uc_orig: MAP_light_get_light (fallen/Source/Map.cpp)
LIGHT_Colour MAP_light_get_light(SLONG x, SLONG z)
{
    MapElement* me;
    me = &MAP[MAP_INDEX(x, z)];
    return me->Colour;
}

// uc_orig: MAP_light_set_light (fallen/Source/Map.cpp)
void MAP_light_set_light(SLONG x, SLONG z, LIGHT_Colour colour)
{
    MapElement* me;
    me = &MAP[MAP_INDEX(x, z)];
    me->Colour = colour;
}

// uc_orig: MAP_light_map (fallen/Source/Map.cpp)
LIGHT_Map MAP_light_map = {
    MAP_WIDTH,
    MAP_HEIGHT,
    MAP_light_get_height,
    MAP_light_get_light,
    MAP_light_set_light
};
