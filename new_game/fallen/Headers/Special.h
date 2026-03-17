//	Special.h
//	Guy Simmons, 28th March 1998.
// claude-ai: Определяет все типы предметов (оружие, боеприпасы, ключи, расходники) и структуру Special.
// claude-ai: MAX_SPECIALS=260 (RMAX_SPECIALS). Таймер 16*20 тиков/сек (20 fps * 16 subticks).

#ifndef FALLEN_HEADERS_SPECIAL_H
#define FALLEN_HEADERS_SPECIAL_H

#define SPECIAL_AMMO_IN_A_PISTOL 15
#define SPECIAL_AMMO_IN_A_SHOTGUN 8
#define SPECIAL_AMMO_IN_A_GRENADE 3
#define SPECIAL_AMMO_IN_A_AK47 30

//---------------------------------------------------------------

// claude-ai: Максимум предметов одновременно на уровне = 260.
#define RMAX_SPECIALS 260
#define MAX_SPECIALS (save_table[SAVE_TABLE_SPECIAL].Maximum)

// claude-ai: Типы предметов (0-29). Всего 30 типов (SPECIAL_NUM_TYPES=30).
// claude-ai:   0  NONE         — нет предмета
// claude-ai:   1  KEY          — ключ (миссийный предмет)
// claude-ai:   2  GUN          — пистолет (одноручное оружие)
// claude-ai:   3  HEALTH       — аптечка
// claude-ai:   4  BOMB         — бомба
// claude-ai:   5  SHOTGUN      — дробовик (двуручное)
// claude-ai:   6  KNIFE        — нож (одноручное)
// claude-ai:   7  EXPLOSIVES   — взрывчатка (таймер 5 сек, шоквейв force=500, radius=0x500)
// claude-ai:   8  GRENADE      — граната (запал 6 сек = 16*20*6 тиков, отскок: velocity >>= 1)
// claude-ai:   9  AK47         — автомат (двуручное)
// claude-ai:  10  MINE         — мина (в пре-релизе НЕ подбирается; SPECIAL_throw_mine() закомментирована)
// claude-ai:  11  THERMODROID  — термодроид (не до конца задокументирован в этой версии)
// claude-ai:  12  BASEBALLBAT  — бита (одноручное)
// claude-ai:  13  AMMO_PISTOL  — патроны для пистолета (15 шт. за подбор)
// claude-ai:  14  AMMO_SHOTGUN — патроны для дробовика (8 шт. за подбор)
// claude-ai:  15  AMMO_AK47    — патроны для AK47 (30 шт. за подбор)
// claude-ai:  16  KEYCARD      — ключ-карта (миссийный)
// claude-ai:  17  FILE         — папка с делом (миссийный)
// claude-ai:  18  FLOPPY_DISK  — дискета (миссийный)
// claude-ai:  19  CROWBAR      — монтировка (одноручное)
// claude-ai:  20  VIDEO        — видеокассета (миссийный)
// claude-ai:  21  GLOVES       — перчатки (расходник)
// claude-ai:  22  WEEDAWAY     — гербицид (расходник)
// claude-ai:  23  TREASURE     — ценность
// claude-ai:  24-28 CARKEY_*  — ключи от машин (RED/BLUE/GREEN/BLACK/WHITE)
// claude-ai:  29  WIRE_CUTTER  — кусачки для проволоки (расходник)
#define SPECIAL_NONE 0
#define SPECIAL_KEY 1
#define SPECIAL_GUN 2
#define SPECIAL_HEALTH 3
#define SPECIAL_BOMB 4
#define SPECIAL_SHOTGUN 5
#define SPECIAL_KNIFE 6
#define SPECIAL_EXPLOSIVES 7
#define SPECIAL_GRENADE 8
#define SPECIAL_AK47 9
#define SPECIAL_MINE 10
#define SPECIAL_THERMODROID 11
#define SPECIAL_BASEBALLBAT 12
#define SPECIAL_AMMO_PISTOL 13
#define SPECIAL_AMMO_SHOTGUN 14
#define SPECIAL_AMMO_AK47 15
#define SPECIAL_KEYCARD 16
#define SPECIAL_FILE 17
#define SPECIAL_FLOPPY_DISK 18
#define SPECIAL_CROWBAR 19
#define SPECIAL_VIDEO 20
#define SPECIAL_GLOVES 21
#define SPECIAL_WEEDAWAY 22
#define SPECIAL_TREASURE 23
#define SPECIAL_CARKEY_RED 24
#define SPECIAL_CARKEY_BLUE 25
#define SPECIAL_CARKEY_GREEN 26
#define SPECIAL_CARKEY_BLACK 27
#define SPECIAL_CARKEY_WHITE 28
#define SPECIAL_WIRE_CUTTER 29
#define SPECIAL_NUM_TYPES 30

//
// Info about every special
//

// claude-ai: Группы предметов — используются для классификации поведения.
// claude-ai:   1 USEFUL          — полезные предметы (ключи, аптечки)
// claude-ai:   2 ONEHANDED_WEAPON — одноручное оружие (пистолет, нож, бита, монтировка)
// claude-ai:   3 TWOHANDED_WEAPON — двуручное оружие (дробовик, AK47)
// claude-ai:   4 STRANGE          — странные предметы (бомба, мина, взрывчатка)
// claude-ai:   5 AMMO             — боеприпасы
// claude-ai:   6 COOKIE           — миссийные предметы (файлы, дискеты, видео и т.д.)
#define SPECIAL_GROUP_USEFUL 1
#define SPECIAL_GROUP_ONEHANDED_WEAPON 2
#define SPECIAL_GROUP_TWOHANDED_WEAPON 3
#define SPECIAL_GROUP_STRANGE 4
#define SPECIAL_GROUP_AMMO 5
#define SPECIAL_GROUP_COOKIE 6

typedef struct
{
    CBYTE* name; // Why not eh?
    UBYTE prim;
    UBYTE group;

} SPECIAL_Info;

extern SPECIAL_Info SPECIAL_info[SPECIAL_NUM_TYPES];

// claude-ai: Подсостояния предметов:
// claude-ai:   0 NONE       — лежит на земле / в инвентаре
// claude-ai:   1 ACTIVATED  — активирован (бомба тикает, мина взведена, граната с запалом)
// claude-ai:   2 IS_DIRT    — активированная мина закопана в dirt (waypoint = индекс dirt)
// claude-ai:   3 PROJECTILE — летит как снаряд (граната в полёте)
#define SPECIAL_SUBSTATE_NONE 0
#define SPECIAL_SUBSTATE_ACTIVATED 1 // For a bomb or a mine or a grenade
#define SPECIAL_SUBSTATE_IS_DIRT 2 // For an activated mine. 'waypoint'
#define SPECIAL_SUBSTATE_PROJECTILE 3

//---------------------------------------------------------------

// claude-ai: Структура предмета (Special). Поля:
// claude-ai:   NextSpecial — linked list инвентаря персонажа (цепочка предметов)
// claude-ai:   OwnerThing  — владелец предмета (Thing*)
// claude-ai:   ammo        — кол-во патронов / обратный отсчёт для активированной мины
// claude-ai:   waypoint    — индекс waypoint, создавшего предмет (или индекс dirt для мины IS_DIRT)
// claude-ai:   counter     — вспомогательный счётчик (термодроиды и т.д.)
// claude-ai:   timer       — таймер гранаты (16*20 тиков/сек)
typedef struct
{
    COMMON(SpecialType)

    THING_INDEX NextSpecial,
        OwnerThing;

    UWORD ammo; // The amount of ammo this thing has or the countdown to going off for an activated mine.

    UWORD waypoint; // The index of the waypoint that created this special- or NULL
                    // if it wasn't created by a waypoint.

    // For an activate MINE in SPECIAL_SUBSTATE_IS_DIRT, this is the index of the DIRT_dirt
    // that is processing the movement of the mine.

    //
    // These are for the thermodroids.
    //

    //	UBYTE home_x;
    //	UBYTE home_z;
    //	UBYTE goto_x;
    //	UBYTE goto_z;

    UWORD counter;
    UWORD timer; // The countdown timer for grenades. 16*20 ticks per second.

} Special;

typedef Special* SpecialPtr;

//---------------------------------------------------------------

void init_specials(void);

//
// Creates an item.  'waypoint' is the index of the waypoint that created
// the item or NULL if this item was not created by a waypoint.  When the item
// is collected, the waypoint that created the item is notified.
//

Thing* alloc_special(
    UBYTE type,
    UBYTE substate,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    UWORD waypoint);

//
// Removes the special item from a person.
//

void special_drop(Thing* p_special, Thing* p_person);

//
// Returns the special if the person own a special of the given type or
// NULL if the person isn't carrying a special of that type.
//

Thing* person_has_special(Thing* p_person, SLONG special_type);

//
// Giving a person specials.  person_get_item() removes the special from the map and does everything...
//

SLONG should_person_get_item(Thing* p_person, Thing* p_special); // Ignores distance to the special- consider only if that person already has a special like that and whether she can carry it.
void person_get_item(Thing* p_person, Thing* p_special);

//
// Primes the given grenade. You have 6 seconds until it blows up.
//

void SPECIAL_prime_grenade(Thing* p_special);

//
// Throws a grenade. The grenade must be being USED and OWNED by a person.
//

void SPECIAL_throw_grenade(Thing* p_special);

/*

// You can't throw mines now. They explode the moment you go near them.

//
// Throws an activated mine. The mine must be in the SpecialList
// of the person.  It creates a bit of MINE dirt, takes the special out
// of the person's SpecialList and puts the special in substate IS_DIRT.
//

void SPECIAL_throw_mine(Thing *p_special);

*/

//
// If this person has some explosives. It primes the explosives and
// places them on the map.
//

void SPECIAL_set_explosives(Thing* p_person);

//---------------------------------------------------------------

#endif // FALLEN_HEADERS_SPECIAL_H
