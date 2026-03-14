//
// A lower memory map: 100k
//

#ifndef _PAP_
#define _PAP_
//#include	"game.h" //really requires thing.h but thing.h required game.h so lets cut it off at the pass#
struct	Thing;

//
// The size of the map and the number of blocks per map square.
//

// claude-ai: Размеры двухуровневой карты.
// claude-ai: PAP_SIZE_HI=128 — хайрез сетка 128×128 ячеек (каждая ~1 метр в мире).
// claude-ai: PAP_SIZE_LO=32  — лорез сетка 32×32 (по 4 хайрез-ячейки на сторону, 16 суммарно).
// claude-ai: PAP_BLOCKS=4    — количество хайрез-ячеек по одной оси на лорез-ячейку.
#define PAP_SIZE_HI	128
#define PAP_SIZE_LO	32
#define PAP_BLOCKS	(PAP_SIZE_HI / PAP_SIZE_LO)



//
// The flags in the hi-res map.
//

// claude-ai: Флаги хайрез-ячейки карты (PAP_Hi.Flags, UWORD = 16 бит).
// claude-ai: ВАЖНО: биты 9 и 14 разделяются по контексту:
// claude-ai:   bit9  = PAP_FLAG_ANIM_TMAP (анимированная текстура) ИЛИ PAP_FLAG_ROOF_EXISTS (есть крыша)
// claude-ai:   bit14 = PAP_FLAG_WANDER (AI может бродить здесь) ИЛИ PAP_FLAG_FLAT_ROOF (крыша плоская)
// claude-ai: ZONE1..ZONE4 (биты 10-13) — группировка ячеек для AI-навигации.
// claude-ai: WATER (бит 15) — в ячейке есть вода.
#define PAP_FLAG_SHADOW_1		(1<<0)
#define PAP_FLAG_SHADOW_2		(1<<1)
#define PAP_FLAG_SHADOW_3		(1<<2)
#define PAP_FLAG_REFLECTIVE		(1<<3)
#define	PAP_FLAG_HIDDEN			(1<<4)
#define PAP_FLAG_SINK_SQUARE	(1<<5)	// Lowers the floorsquare to create a curb.
#define PAP_FLAG_SINK_POINT		(1<<6)	// Transform the point on the lower level.
#define PAP_FLAG_NOUPPER		(1<<7)	// Don't transform the point on the upper level.
#define	PAP_FLAG_NOGO			(1<<8)  // A square nobody is allowed onto
#define	PAP_FLAG_ANIM_TMAP		(1<<9)
#define	PAP_FLAG_ROOF_EXISTS	(1<<9)
#define PAP_FLAG_ZONE1			(1<<10)	// These four bits identify groups of mapsquares
#define PAP_FLAG_ZONE2			(1<<11)	// used by the AI system to gives zones.
#define PAP_FLAG_ZONE3			(1<<12)
#define PAP_FLAG_ZONE4	    	(1<<13)
#define PAP_FLAG_WANDER			(1<<14)
#define	PAP_FLAG_FLAT_ROOF		(1<<14)

#define PAP_FLAG_WATER          (1<<15)


#define	ROOF_HIDDEN_INDEX		(-10000)
#define	IS_ROOF_HIDDEN_FACE(face)	(((face)<ROOF_HIDDEN_INDEX)?1:0)
#define	ROOF_HIDDEN_X(face)     (((-(face))+ROOF_HIDDEN_INDEX)&127)
#define	ROOF_HIDDEN_Z(face)     ((((-(face))+ROOF_HIDDEN_INDEX)>>7)&127)
#define	ROOF_HIDDEN_GET_FACE(x,z) (-((x)+(z)*128)+ROOF_HIDDEN_INDEX)
//
// The scale of Alt in the hi-res map.
// 

// claude-ai: Масштаб поля Alt в PAP_Hi. Реальная высота = Alt << PAP_ALT_SHIFT (т.е. Alt * 8).
// claude-ai: Alt — SBYTE (-128..127), реальная высота в юнитах (-1024..1016).
#define PAP_ALT_SHIFT 3

//
// If the water altitude == PAP_LO_NO_WATER then no hi-res square in this
// lo-res mapwho has any water.
//

#define PAP_LO_NO_WATER (-127)

//
// The lo-res map.
//

// claude-ai: Флаг лорез-ячейки: есть склад (warehouse) — влияет на вычисление высоты PAP_calc_height_at_thing.
#define PAP_LO_FLAG_WAREHOUSE (1 << 0)	// This lo-res mapsquare has square that are inside a warehouse

// claude-ai: Лорез-ячейка карты (32×32), 8 байт. Покрывает 4×4 хайрез-ячейки.
// claude-ai:   MapWho     — THING_INDEX первого Thing, стоящего в этой лорез-ячейке (связный список)
// claude-ai:   Walkable   — +ve: нормальный prim_face4; -ve: специальный roof quad
// claude-ai:   ColVectHead — голова связного списка col_vect_links для этой ячейки
// claude-ai:   water       — высота воды в юнитах; -127 (PAP_LO_NO_WATER) = нет воды
// claude-ai:   Flag        — PAP_LO_FLAG_WAREHOUSE
typedef struct
{
	UWORD MapWho;
	SWORD Walkable;     // +ve normal prim_face4, -ve is special roof quad
	UWORD ColVectHead;  //don't need this, it could be -ve mapwho
	SBYTE water;		// The height of any water in this mapsquare.
	UBYTE Flag;

} PAP_Lo;

//extern PAP_Lo PAP_lo[PAP_SIZE_LO][PAP_SIZE_LO];


//
// The hi-res map.
//

// claude-ai: Хайрез-ячейка карты (128×128), 6 байт.
// claude-ai:   Texture — индекс текстуры пола (3 бита запасных)
// claude-ai:   Flags   — PAP_FLAG_* (UWORD, см. выше; биты 9 и 14 — контекстно-зависимые!)
// claude-ai:   Alt     — высота угла ячейки в SBYTE; реальная высота = Alt << PAP_ALT_SHIFT
// claude-ai:   Height  — не используется (padding, 16K памяти "на будущее")
typedef struct
{
	UWORD Texture; //3 spare bits here
	UWORD Flags;   // full but some sewer stuff that could go perhaps
	SBYTE Alt;
	SBYTE Height;//padding; // better find something to do with this 16K

} PAP_Hi;



typedef	PAP_Lo  MEM_PAP_Lo[PAP_SIZE_LO];
typedef	PAP_Hi  MEM_PAP_Hi[PAP_SIZE_HI];

extern	MEM_PAP_Lo *PAP_lo; //[PAP_SIZE_LO][PAP_SIZE_LO];
extern	MEM_PAP_Hi *PAP_hi; //[PAP_SIZE_HI][PAP_SIZE_HI];


//extern PAP_Hi PAP_hi[PAP_SIZE_HI][PAP_SIZE_HI];


//
// The shifts needed to get a UWORD coordinate to a
// mapsquare coordinate for the hi and lo res maps.
//

// claude-ai: Битовые сдвиги для перевода мировых координат (UWORD) в индексы mapsquare.
// claude-ai:   coord >> PAP_SHIFT_HI (8) → индекс хайрез-ячейки (0..127)
// claude-ai:   coord >> PAP_SHIFT_LO (10) → индекс лорез-ячейки (0..31)
// claude-ai: Т.е. 1 хайрез-ячейка = 256 юнитов; 1 лорез-ячейка = 1024 юнита.
#define PAP_SHIFT_LO	10
#define PAP_SHIFT_HI	 8


//
// Returns true if the mapsquare coordinate is on each map.
//

SLONG PAP_on_map_lo(SLONG x, SLONG z);
SLONG PAP_on_map_hi(SLONG x, SLONG z);


//
// Returns the given square.
//

// claude-ai: Макросы доступа к ячейкам карты по индексам (не мировым координатам!).
// claude-ai:   PAP_2LO(x,z) — лорез-ячейка, x и z уже сдвинуты >> PAP_SHIFT_LO
// claude-ai:   PAP_2HI(x,z) — хайрез-ячейка, x и z уже сдвинуты >> PAP_SHIFT_HI
// claude-ai: В debug-сборке содержат assert; в release — прямой доступ к массиву.
// claude-ai: ON_PAP_LO(x,z) — проверка что (x,z) в пределах лорез-карты [0..31].
#ifdef PSX

#define PAP_2LO(x,z) (PAP_lo[(x)][(z)])
#define PAP_2HI(x,z) (PAP_hi[(x)][(z)])

#define PAP_on_map_hi(x,z) ((WITHIN((x), 0, PAP_SIZE_HI -1) && WITHIN((z), 0, PAP_SIZE_HI -1))?TRUE:FALSE)
#define PAP_on_map_lo(x,z) ((WITHIN((x), 0, PAP_SIZE_LO -1) && WITHIN((z), 0, PAP_SIZE_LO -1))?TRUE:FALSE)


#else

#ifdef NDEBUG

#define PAP_2LO(x,z) (PAP_lo[(x)][(z)])
#define PAP_2HI(x,z) (PAP_hi[(x)][(z)])

#else

//#define PAP_on_map_hi(x,z) ((WITHIN((x), 0, PAP_SIZE_HI -1) && WITHIN((z), 0, PAP_SIZE_HI -1))?TRUE:FALSE)
//#define PAP_on_map_lo(x,z) ((WITHIN((x), 0, PAP_SIZE_LO -1) && WITHIN((z), 0, PAP_SIZE_LO -1))?TRUE:FALSE)

void PAP_assert_if_off_map_lo(SLONG x, SLONG z);
void PAP_assert_if_off_map_hi(SLONG x, SLONG z);

#define PAP_2LO(x,z) (PAP_assert_if_off_map_lo((x),(z)), PAP_lo[(x)][(z)])
#define PAP_2HI(x,z) (PAP_assert_if_off_map_hi((x),(z)), PAP_hi[(x)][(z)])

#endif	// NDEBUG
#endif	// PSX

#define	ON_PAP_LO(x,z)	( (x)>=0 && (z)>=0 && (x)<PAP_SIZE_LO && (z)<PAP_SIZE_LO)

//
// Returns the height at (map_x,map_z) the height at the corner of a mapsquare.
// Returns the interpolated height at the given point.
// Returns the height of the map including buildings.
//

// claude-ai: Функции вычисления высоты рельефа. Аргументы x,z — мировые координаты (юниты).
// claude-ai:   PAP_calc_height_at_point — высота в углу хайрез-ячейки (без интерполяции)
// claude-ai:   PAP_calc_height_at       — билинейная интерполяция высоты в произвольной точке
// claude-ai:   PAP_calc_height_at_thing — то же, но учитывает склад (warehouse): если p_thing
// claude-ai:                              находится внутри склада, возвращает высоту его крыши
// claude-ai:   PAP_calc_map_height_at   — высота карты с учётом зданий (верх здания, если внутри)
SLONG PAP_calc_height_at_point(SLONG map_x, SLONG map_z);
SLONG PAP_calc_height_at      (SLONG x, SLONG z);
SLONG PAP_calc_height_at_thing(Thing	*p_thing,SLONG x, SLONG z);
SLONG PAP_calc_map_height_at  (SLONG x, SLONG z);


//
// Calculate the height of the floor ignoring curbs and roads.
// Looks around (x,z) for the highest point and returns that point.
//

// claude-ai: PAP_calc_height_noroads  — высота рельефа без учёта бордюров и дорог
// claude-ai: PAP_calc_map_height_near — высота карты в ближайшей точке (для размещения объектов)
SLONG PAP_calc_height_noroads (SLONG x, SLONG z);
SLONG PAP_calc_map_height_near(SLONG x, SLONG z);


void	PAP_clear(void);

//
// Returns TRUE if the given region of map is not on a hill.
//

SLONG PAP_is_flattish(
		SLONG x1, SLONG z1,
		SLONG x2, SLONG z2);

#endif

