//
// Marks navigation system.
//
// claude-ai: MAV = основная навигационная система NPC (A* на карте 128×128 клеток).
// claude-ai: MAV_nav[128][128] — карта навигации (UWORD на каждую клетку, 16 бит):
// claude-ai:   биты 0-9  (10 бит): MAV_NAV  — индекс в MAV_opt[] (опции движения пешком)
// claude-ai:   биты 10-13 (4 бита): MAV_CAR  — навигация машин (битовое поле: по 1 биту на каждое из 4 направлений)
// claude-ai:   биты 14-15 (2 бита): MAV_SPARE — вода и другие флаги
// claude-ai: MAV_do() — главная функция: из текущей позиции к цели, возвращает следующее действие (MAV_Action).
// claude-ai: MAV_LOOKAHEAD=32 — горизонт поиска пути (не вся карта, только вперёд на 32 клетки).

#ifndef _MAV_
#define _MAV_


#include "pap.h"
#include "structs.h"


//
// This array is indexed by the MAV_nav[] array
//

typedef struct
{
	UBYTE opt[4];	// The options for moving in each direction.

} MAV_Opt;

#define MAV_MAX_OPTS 1024	// don't change this!!!

extern MAV_Opt *MAV_opt;
extern SLONG    MAV_opt_upto;

//
// How you can move out of each square.  This is a 2D array
// whose pitch is given by MAV_nav_pitch;
//

// claude-ai: Карта навигации — 2D массив UWORD. Каждая клетка хранит 3 поля в одном UWORD:
// claude-ai:   MAV_NAV(x,z)   — биты 0-9:  индекс в MAV_opt[] (пешеходные маршруты)
// claude-ai:   MAV_CAR(x,z)   — биты 10-13: проходимость для машин (4 направления)
// claude-ai:   MAV_SPARE(x,z) — биты 14-15: вода и другие флаги среды
extern UWORD *MAV_nav;
extern SLONG  MAV_nav_pitch;

#define MAV_SPARE_FLAG_WATER  (1 << 0)	// The first spare bit
#define MAV_SPARE_FLAG_UNUSED (1 << 1)	// The second spare bit

#define MAV_NAV(x,z)			(MAV_nav[((x) * MAV_nav_pitch) + (z)] & 1023)			// 10 bits
#define MAV_CAR(x,z)			((MAV_nav[((x) * MAV_nav_pitch) + (z)] >> 10) & 15)		// 4 bits
#define MAV_SPARE(x,z)			(MAV_nav[((x) * MAV_nav_pitch) + (z)] >> 14)			// 2 bits
#define SET_MAV_NAV(x,z,v)		MAV_nav[((x) * MAV_nav_pitch) + (z)] = (MAV_nav[((x) * MAV_nav_pitch) + (z)] & 0xFC00) | ((v) & 1023)
#define SET_MAV_CAR(x,z,v)		MAV_nav[((x) * MAV_nav_pitch) + (z)] = (MAV_nav[((x) * MAV_nav_pitch) + (z)] & 0xC3FF) | (((v) & 15) << 10)
#define SET_MAV_SPARE(x,z,v)	MAV_nav[((x) * MAV_nav_pitch) + (z)] = (MAV_nav[((x) * MAV_nav_pitch) + (z)] & 0x3FFF) | ((v) << 14)

#define MAV_CAR_GOTO(x,z,d)		(!!(MAV_CAR(x,z) & (1 << d)))

#define	MAVHEIGHT(x,z)			(PAP_hi[x][z].Height)

//
// A UBYTE of height for each mapsquare.
// The height is in quarter blocks.
//

//typedef	SBYTE MAV_height_workaround[PAP_SIZE_HI];
//extern        MAV_height_workaround *MAV_height;


//
// Call this function first.
//

void MAV_init(void);



// ========================================================
//
// TO CALCULATE THE MAV ARRAYS FOR INSIDE WAREHOUSES
//
// ========================================================

//
// First, work out the MAV_height array without the warehouses.
//

void MAV_calc_height_array(SLONG ignore_warehouses);

//
// You must have set the bounding box and the
// mav_pitch and mav fields of the warehouse properly.
//

void MAV_precalculate_warehouse_nav(UBYTE ware);	// Index into the WARE_ware array.



//
// Works out everything for the current map. The MAV_height array and the
// MAV_nav array.
//

// claude-ai: Предрасчёт карты навигации при загрузке уровня. Вычисляет MAV_height и MAV_nav массивы.
// claude-ai: Вызывается один раз при инициализации уровня. Может занимать заметное время.
void MAV_precalculate(void);



//
// Returns what someone should do next in order to get somewhere.
//

// claude-ai: Типы навигационных действий (результат MAV_do()). Значения 0-7:
// claude-ai:   0 GOTO       — просто идти в направлении dest
// claude-ai:   1 JUMP       — прыгнуть через блок
// claude-ai:   2 JUMPPULL   — прыжок + подтягивание на 1 блок вверх
// claude-ai:   3 JUMPPULL2  — прыжок + подтягивание на 2 блока вверх
// claude-ai:   4 PULLUP     — подтянуться (без прыжка)
// claude-ai:   5 CLIMB_OVER — перелезть через препятствие
// claude-ai:   6 FALL_OFF   — прыгнуть вниз (намеренное падение)
// claude-ai:   7 LADDER_UP  — лезть вверх по лестнице
#define MAV_ACTION_GOTO			0
#define MAV_ACTION_JUMP			1		// Jump one block.
#define MAV_ACTION_JUMPPULL		2		// Jump one block by pulling yourself up
#define MAV_ACTION_JUMPPULL2	3		// Jump two blocks by pulling yourself up.
#define MAV_ACTION_PULLUP		4
#define MAV_ACTION_CLIMB_OVER	5
#define MAV_ACTION_FALL_OFF		6
#define MAV_ACTION_LADDER_UP	7

#define MAV_DIR_XS	0
#define MAV_DIR_XL	1
#define MAV_DIR_ZS	2
#define MAV_DIR_ZL	3

// claude-ai: Битовые флаги возможностей персонажа при навигации.
// claude-ai: Передаются в MAV_do() как параметр caps. OR-ить нужные биты.
// claude-ai: MAV_CAPS_DARCI = 0xff — Darci умеет всё (все 8 действий доступны).
// claude-ai: Обычные NPC (копы, thugs) имеют ограниченный набор — не могут подтягиваться или лезть по тросам.
#define MAV_CAPS_GOTO		(1 << MAV_ACTION_GOTO)
#define MAV_CAPS_JUMP		(1 << MAV_ACTION_JUMP)
#define MAV_CAPS_JUMPPULL	(1 << MAV_ACTION_JUMPPULL)
#define MAV_CAPS_JUMPPULL2	(1 << MAV_ACTION_JUMPPULL2)
#define MAV_CAPS_PULLUP		(1 << MAV_ACTION_PULLUP)
#define MAV_CAPS_CLIMB_OVER	(1 << MAV_ACTION_CLIMB_OVER)
#define MAV_CAPS_FALL_OFF	(1 << MAV_ACTION_FALL_OFF)
#define MAV_CAPS_LADDER_UP	(1 << MAV_ACTION_LADDER_UP)

#define MAV_CAPS_DARCI (0xff)	// She can do everything.

// claude-ai: Главная функция навигации. Возвращает следующее действие для перемещения из me_x/me_z к dest_x/dest_z.
// claude-ai: Координаты в единицах карты (mapsquares, не world coords). caps = MAV_CAPS_* OR-маска.
// claude-ai: Горизонт поиска: MAV_LOOKAHEAD=32 клетки. Результат: MAV_Action {action, dir, dest_x, dest_z}.
// claude-ai: После вызова MAV_do_found_dest показывает нашёл ли алгоритм полный путь до цели.
MAV_Action MAV_do(
			SLONG me_x,		// 0-bit fixed point- these are mapsquares.
			SLONG me_z,
			SLONG dest_x,	// 0-bit fixed point- these are mapsquares.
			SLONG dest_z,
			UBYTE caps);	// OR together all the thing you can do

//
// After calling MAV_do() if this value is TRUE, then the call found
// a route all the way to the destination. If this value is FALSE there
// still might be a way to get there, but if it is TRUE there definitely is.
//

extern UBYTE MAV_do_found_dest;

//
// Using the MAV_height array, this function returns TRUE if the given point
// is underground or within a building.
// 

SLONG MAV_inside(
		SLONG x,
		SLONG y,
		SLONG z);

//
// Returns the caps for going from the square in the given direction.
//

UBYTE MAV_get_caps(
		UBYTE x,
		UBYTE z,
		UBYTE dir);


#ifndef TARGET_DC
//
// Draw the MAV_info in the given squares (these are mapsquares... fixed point 0).
//

void MAV_draw(
		SLONG x1, SLONG z1,
		SLONG x2, SLONG z2);
#endif //#ifndef TARGET_DC


//
// A crude los function that only uses the MAV_height array. Coordinates are
// world coordinates given with 8-bits per mapsquare.
//

extern SLONG MAV_height_los_fail_x;
extern SLONG MAV_height_los_fail_y;
extern SLONG MAV_height_los_fail_z;

SLONG MAV_height_los_fast(
		SLONG x1, SLONG y1, SLONG z1,
		SLONG x2, SLONG y2, SLONG z2);

SLONG MAV_height_los_slow(
		SLONG ware,
		SLONG x1, SLONG y1, SLONG z1,
		SLONG x2, SLONG y2, SLONG z2);


//
// For changing NAV info on the fly... rather dangerous.
// 

// claude-ai: Динамическое изменение MAV-карты (например, для открытия/закрытия дверей).
// claude-ai: Включает/выключает пешеходный маршрут из клетки (mx,mz) в направлении dir.
void MAV_turn_movement_on (UBYTE mx, UBYTE mz, UBYTE dir);
void MAV_turn_movement_off(UBYTE mx, UBYTE mz, UBYTE dir);

//
// For changing car MAV info on the fly... not dangerous at all.
//

void MAV_turn_car_movement_on (UBYTE mx, UBYTE mz, UBYTE dir);
void MAV_turn_car_movement_off(UBYTE mx, UBYTE mz, UBYTE dir);


#endif
