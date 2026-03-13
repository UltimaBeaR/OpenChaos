// Map.cpp
// Guy Simmons, 22nd October 1997.

// claude-ai: Map.cpp — тонкая обёртка над системой карты, содержит только функции освещения.
// claude-ai: Основная структура карты:
// claude-ai:   MAP[128][128] — массив MapElement (только поле Colour используется — LIGHT_Colour RGB)
// claude-ai:   PAP_Hi[128][128] — хайрез-ячейки, 6 байт/ячейка, высота Alt << PAP_ALT_SHIFT(3)
// claude-ai:   PAP_Lo[32][32]  — лорез-ячейки, 8 байт/ячейка: MapWho, Walkable, ColVectHead, water
// claude-ai:
// claude-ai: Инициализация PAP_Hi/PAP_Lo и MapWho-списков: в pap.cpp (PAP_init, PAP_load).
// claude-ai: Управление объектами на карте: в Thing.cpp (add_thing_to_map, remove_thing_from_map, move_thing_on_map).
// claude-ai: Поиск объектов в сфере/прямоугольнике: в Thing.cpp (THING_find_sphere, THING_find_box).
// claude-ai:
// claude-ai: MAP_light_map — интерфейс системы освещения (callbacks get/set light по ячейкам).
// claude-ai: ⚠️ МЁРТВЫЙ КОД в DDEngine (PC): MAP_light_set_light/get_light нигде не вызываются!
// claude-ai:   MAP_light_map struct определён но нигде не используется в рантайме.
// claude-ai:   В DDEngine vertex colour terrain = NIGHT_light_mapsquare → NIGHT_cache → NIGHT_get_d3d_colour.
// claude-ai:   MapElement.Colour применялся только в Glide Engine (glaeng.cpp) — legacy путь.
#include	"Game.h"


//---------------------------------------------------------------
#if !defined(PSX) && !defined(TARGET_DC)
void	init_map(void)
{
	memset((UBYTE*)MAP,0,sizeof(MAP));
}
#endif
//---------------------------------------------------------------



SLONG MAP_light_get_height(SLONG x, SLONG z)
{
	return 0;
}

LIGHT_Colour MAP_light_get_light(SLONG x, SLONG z)
{
#ifdef TARGET_DC
	// Shouldn't be using this, apparently.
	ASSERT ( FALSE );
#endif

	MapElement *me;

	me = &MAP[MAP_INDEX(x,z)];

	return me->Colour;
}

void MAP_light_set_light(SLONG x, SLONG z, LIGHT_Colour colour)
{
#ifdef TARGET_DC
	// Shouldn't be using this, apparently.
	ASSERT ( FALSE );
#endif

	MapElement *me;

	me = &MAP[MAP_INDEX(x,z)];

	me->Colour = colour;
}


LIGHT_Map MAP_light_map = 
{
	MAP_WIDTH,
	MAP_HEIGHT,
	MAP_light_get_height,
	MAP_light_get_light,
	MAP_light_set_light
};




