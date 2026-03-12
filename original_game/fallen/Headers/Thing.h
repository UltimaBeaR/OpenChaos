// Thing.h
// Guy Simmons, 15th October 1997.

#ifndef	THING_H
#define	THING_H
#include	"../Headers/Game.h"
#include	"../Headers/drawtype.h"
#include	"../Headers/vehicle.h"


//---------------------------------------------------------------
/*
#define	MAX_PRIMARY_THINGS		MAX_PLAYERS		+		\
								MAX_PROJECTILES	+		\
								MAX_PEOPLE		+		\
								MAX_FURNITURE	+		\
								MAX_SPECIALS	+		\
								BAT_MAX_BATS	+		\
								100
#define	MAX_SECONDARY_THINGS	MAX_SWITCHES	+		\
								TRACK_BUFFER_LENGTH +   \
								1
*/

// claude-ai: Лимиты объектов — 400 primary (игровые персонажи, машины, мебель и т.д.) + 300 secondary (PC) / 50 (PSX) (switches, tracks) = 700 всего
#define	MAX_PRIMARY_THINGS		(400)
#ifndef BUILD_PSX
#define	MAX_SECONDARY_THINGS	(300)
#else
#define	MAX_SECONDARY_THINGS	(50)
#endif

#define	MAX_THINGS				(MAX_PRIMARY_THINGS+MAX_SECONDARY_THINGS+1)

// claude-ai: Флаги Thing.Flags (ULONG, 32 бит) — битовое поле состояния объекта
#define	FLAGS_ON_MAPWHO				(1 <<  0)
#define	FLAGS_IN_VIEW				(1 <<  1)
#define	FLAGS_TWEEN_TO_QUEUED_FRAME	(1 <<  2)
#define	FLAGS_PROJECTILE_MOVEMENT	(1 <<  3)
#define	FLAGS_LOCKED_ANIM			(1 <<  4)
#define FLAGS_IN_BUILDING			(1 <<  5)
#define FLAGS_INDOORS_GENERATED		(1 <<  6)
#define FLAGS_LOCKED				(1 <<  7)	// A locked door...
#define FLAGS_CABLE_BUILDING		(1 <<  8)	// A building thing that is a cable.
#define FLAGS_COLLIDED				(1 <<  9)	// For a vehicle that collided with a wall last frame.
#define FLAGS_IN_SEWERS             (1 << 10)
#define FLAGS_SWITCHED_ON			(1 << 11)
#define FLAGS_PLAYED_FOOTSTEP		(1 << 12)
#define FLAGS_HAS_GUN				(1 << 13)
#define FLAGS_BURNING				(1 << 14)   // flag burning: illegal in some countries
#define FLAGS_HAS_ATTACHED_SOUND	(1 << 15)   // a sound fx is "attached" to this thing

#define	FLAGS_PERSON_BEEN_SHOT		(1 << 16)

#define FLAGS_PERSON_AIM_AND_RUN	(1 <<  6)

#define	FLAG_EDIT_PRIM_ON_FLOOR		(1<<2)
#define	FLAG_EDIT_PRIM_JUST_FLOOR	(1<<3)
#define	FLAG_EDIT_PRIM_INSIDE		(1<<4)

#define	FLAG_SPECIAL_UNTOUCHED		(1<<2)
#define	FLAG_SPECIAL_HIDDEN			(1<<3)

//---------------------------------------------------------------


// claude-ai: Базовый игровой объект — 125 байт. Полиморфизм через union Genus (тип задаётся Class) и function pointer StateFn (вызывается каждый кадр).
// claude-ai: Классы: PLAYER(1), CAMERA(2), PROJECTILE(3), BUILDING(4), PERSON(5), ANIMAL(6), FURNITURE(7), SWITCH(8), VEHICLE(9), SPECIAL(10), ANIM_PRIM(11), CHOPPER(12), PYRO(13), TRACK(14), PLAT(15), BARREL(16), BIKE(17 — не перенесён), BAT(18)
// claude-ai: Координаты GameCoord: SLONG X, Y, Z, 256 единиц = 1 mapsquare. NextLink — итерация объектов одного класса через thing_class_head[CLASS].
struct Thing
{
	UBYTE			Class,
					State,
					OldState,
					SubState;
	ULONG			Flags;
	THING_INDEX		Child,
					Parent,
					LinkChild,
					LinkParent;

	GameCoord		WorldPos;

	// claude-ai: StateFn — function pointer на текущий обработчик состояния. Вызывается каждый кадр из process_things(). Меняется при смене State (например, set_person_running() пишет сюда новый handler).
	void			(*StateFn)(Thing*);	// Things state function.

	// claude-ai: Draw union — способ отрисовки: Tweened (анимированная меш с интерполяцией кадров) или Mesh (статичный меш без анимации). DrawType определяет какой вариант активен.
	union
	{
		DrawTween			*Tweened;
		DrawMesh			*Mesh;
	}Draw;

	// claude-ai: Genus union — данные конкретного типа объекта. Активный член определяется полем Class. Например, если Class==CLASS_PERSON — используется Genus.Person (PersonPtr -> struct Person из Person.h).
	union
	{
		VehiclePtr		 Vehicle;
		FurniturePtr     Furniture;
		PersonPtr		 Person;
		AnimalPtr		 Animal;
		ChopperPtr		 Chopper;
		PyroPtr			 Pyro;
		ProjectilePtr	 Projectile;
		PlayerPtr		 Player;
		SpecialPtr		 Special;
		SwitchPtr		 Switch;
		TrackPtr		 Track;
		PlatPtr          Plat;
		BarrelPtr        Barrel;
		#ifdef BIKE
		BikePtr          Bike;
		#endif
		BatPtr			 Bat;

	}Genus;
	UBYTE			DrawType;
	UBYTE			Lead; //42

	// TEMP STUFF.
	SWORD			Velocity;					// 
	SWORD			DeltaVelocity;

	SWORD			RequiredVelocity;
	SWORD			DY;

	UWORD			Index;
	SWORD			OnFace;

	UWORD			NextLink;//BuildingList;	// For a building thing...
	UWORD			DogPoo1; //Timer1;

	THING_INDEX		DogPoo2; //SwitchThing;	//	Temporary for building unlock switches.
};

typedef struct Thing Thing;

// claude-ai: thing_class_head[CLASS] — голова однонаправленного связного списка объектов данного класса. Итерация: начать с thing_class_head[CLASS], следующий — Thing.NextLink. Конец списка = 0.
extern	UWORD	*thing_class_head;

//---------------------------------------------------------------

void			init_things(void);
THING_INDEX		alloc_primary_thing(UWORD thing_class);
void			free_primary_thing(THING_INDEX thing);
THING_INDEX		alloc_secondary_thing(UWORD secondary_thing);
void			free_secondary_thing(THING_INDEX thing);
void			add_thing_to_map(Thing *t_thing);
void			remove_thing_from_map(Thing *t_thing);
void			move_thing_on_map(Thing *t_thing,GameCoord *new_position);
void			process_things(SLONG f_r_i);

void			log_primary_used_list(void);
void			log_primary_unused_list(void);

Thing			*alloc_thing(SBYTE classification);
void			free_thing(Thing *t_thing);

Thing			*nearest_class(Thing *the_thing,ULONG class_mask,ULONG *closest);

inline void		set_thing_pos(Thing *t,SLONG x,SLONG y,SLONG z)	\
												{
													t->WorldPos.X=x;t->WorldPos.Y=y;t->WorldPos.Z=z;
												}

//
// Returns the distance between the two things.
// This is in 8-bits per mapsquares and approximate.
//

SLONG THING_dist_between(Thing *p_thing_a, Thing *p_thing_b);


//
// Removes a thing from the map and the game. It clears up all the memory
// it is supposed to aswell.
//

void THING_kill(Thing *t);

//
// A handy array for finding things with...
// 

#define THING_ARRAY_SIZE 32

extern THING_INDEX THING_array[THING_ARRAY_SIZE];


//
// Fills an array with all the things in the given bounding sphere.
// It returns the number of things found.  'Classes' gives which classes
// of thing to include and which to ignore.  EG if you want to only find
// projectiles, pass (1 << CLASS_PROJECTILE), there is a bit for each
// class type.
//
// Returns the number of things found and put into the array.
//

#define THING_FIND_EVERYTHING (0xffffffff)
#define THING_FIND_PEOPLE     (1 << CLASS_PERSON)
#define THING_FIND_LIVING	  ((1 << CLASS_PERSON) | (1 << CLASS_ANIMAL))
#define THING_FIND_MOVING	  ((1 << CLASS_PERSON) | (1 << CLASS_ANIMAL) | (1 << CLASS_PROJECTILE))

// claude-ai: THING_find_sphere — ищет все объекты указанных классов (bitmask) в радиусе от точки. Записывает THING_INDEX в array, возвращает кол-во найденных. Используй THING_FIND_* маски или (1 << CLASS_*).
SLONG THING_find_sphere(
		SLONG        centre_x,	// 50 << 8
		SLONG        centre_y,	//  0
		SLONG        centre_z,	// 50 << 8
		SLONG        radius,
		THING_INDEX *array,
		SLONG        array_size,
		ULONG        classes);

// claude-ai: THING_find_box — как THING_find_sphere, но ищет в прямоугольной 2D-области (X/Z плоскость). Y-координата не учитывается. Полезно для проверок на карте.
SLONG THING_find_box(
		SLONG x1, SLONG z1,
		SLONG x2, SLONG z2,
		THING_INDEX *array,
		SLONG        array_size,
		ULONG        classes);

//
// Finds the nearest thing of the given class. Returns NULL on failure.
//

// claude-ai: THING_find_nearest — возвращает THING_INDEX ближайшего объекта нужного класса в радиусе. NULL если не найдено. Быстрее чем find_sphere + ручной поиск минимума.
SLONG THING_find_nearest(
		SLONG centre_x,
		SLONG centre_y,
		SLONG centre_z,
		SLONG radius,
		ULONG classes);


//---------------------------------------------------------------

#endif



