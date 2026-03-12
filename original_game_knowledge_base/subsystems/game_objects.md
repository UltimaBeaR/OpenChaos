# Thing System — Игровые объекты Urban Chaos

**Ключевые файлы:**
- `fallen/Headers/Thing.h` — структура Thing (читать полностью!)
- `fallen/Source/Thing.cpp` — управление жизненным циклом
- `fallen/Headers/statedef.h` — все состояния (STATE_*)
- `fallen/Source/State.cpp` — машина состояний

---

## 1. Структура Thing (125 байт)

Базовый объект для ВСЕГО в игре. Полиморфизм через union + function pointer, без C++ наследования.

```c
struct Thing {
    UBYTE Class;              // Тип объекта (CLASS_PERSON, CLASS_VEHICLE и т.д.)
    UBYTE State;              // Текущее состояние
    UBYTE OldState;           // Предыдущее состояние
    UBYTE SubState;           // Подсостояние (детализация поведения)
    ULONG Flags;              // 32 флага состояния

    THING_INDEX Child;        // Иерархия объектов (Parent/Child)
    THING_INDEX Parent;
    THING_INDEX LinkChild;    // Управление памятью (связный список USED/UNUSED)
    THING_INDEX LinkParent;

    GameCoord WorldPos;       // Позиция в 3D (X, Y, Z) — SLONG координаты

    void (*StateFn)(Thing*);  // Указатель на функцию текущего состояния

    union {                   // Рисование
        DrawTween *Tweened;   // Анимированная модель (персонажи)
        DrawMesh  *Mesh;      // Статичный меш (здания, предметы)
    } Draw;

    union {                   // Данные специфичные для типа
        VehiclePtr   Vehicle;
        FurniturePtr Furniture;
        PersonPtr    Person;
        AnimalPtr    Animal;
        ChopperPtr   Chopper;
        PyroPtr      Pyro;
        ProjectilePtr Projectile;
        PlayerPtr    Player;
        SpecialPtr   Special;
        SwitchPtr    Switch;
        TrackPtr     Track;
        PlatPtr      Plat;
        BarrelPtr    Barrel;
        BikePtr      Bike;
        BatPtr       Bat;
    } Genus;

    UBYTE DrawType;           // Тип отрисовки
    UBYTE Lead;               // Неясное поле

    SWORD Velocity;           // Скорость движения
    SWORD DeltaVelocity;      // Изменение скорости за кадр
    SWORD RequiredVelocity;   // Требуемая скорость
    SWORD DY;                 // Вертикальная скорость (прыжки/падение)

    UWORD Index;              // Индекс в массиве Things
    SWORD OnFace;             // На какой face стоит (-1 = земля/вода)

    UWORD NextLink;           // Связный список объектов одного класса
    UWORD DogPoo1;            // Timer1
    THING_INDEX DogPoo2;      // SwitchThing (временная переменная)
};
```

---

## 2. Классы объектов (CLASS_*)

```c
CLASS_NONE       (0)   // Не существует
CLASS_PLAYER     (1)   // Управляемый персонаж
CLASS_CAMERA     (2)   // Камера
CLASS_PROJECTILE (3)   // Снаряд/пуля
CLASS_BUILDING   (4)   // Здание
CLASS_PERSON     (5)   // NPC
CLASS_ANIMAL     (6)   // Животное (собаки, голуби)
CLASS_FURNITURE  (7)   // Мебель/предметы
CLASS_SWITCH     (8)   // Переключатель (двери, рычаги)
CLASS_VEHICLE    (9)   // Машина/фургон
CLASS_SPECIAL    (10)  // Специальный объект (гранаты и т.д.)
CLASS_ANIM_PRIM  (11)  // Анимированный примитив
CLASS_CHOPPER    (12)  // Вертолёт
CLASS_PYRO       (13)  // Огонь/взрыв
CLASS_TRACK      (14)  // Трек записи движения (waypoints)
CLASS_PLAT       (15)  // Движущаяся платформа
CLASS_BARREL     (16)  // Бочка
CLASS_BIKE       (17)  // Мотоцикл
CLASS_BAT        (18)  // Летучая мышь (враг)
CLASS_END        (19)  // Конец списка
```

---

## 3. Флаги Thing (FLAGS_*)

| Флаг | Бит | Описание |
|------|-----|---------|
| FLAGS_ON_MAPWHO | 0 | Добавлен в пространственную карту MapWho |
| FLAGS_IN_VIEW | 1 | Видим на экране |
| FLAGS_TWEEN_TO_QUEUED_FRAME | 2 | Анимация переходит к следующему кадру |
| FLAGS_PROJECTILE_MOVEMENT | 3 | Движение по параболе (снаряд) |
| FLAGS_LOCKED_ANIM | 4 | Анимация заблокирована |
| FLAGS_IN_BUILDING | 5 | Объект внутри здания |
| FLAGS_INDOORS_GENERATED | 6 | Интерьер здания уже сгенерирован |
| FLAGS_LOCKED | 7 | Объект заблокирован (двери) |
| FLAGS_CABLE_BUILDING | 8 | Здание с кабелем (grappling hook) |
| FLAGS_COLLIDED | 9 | Транспорт столкнулся со стеной в этом кадре |
| FLAGS_IN_SEWERS | 10 | Объект в канализации |
| FLAGS_SWITCHED_ON | 11 | Переключатель активирован |
| FLAGS_PLAYED_FOOTSTEP | 12 | Звук шага воспроизведён в этом кадре |
| FLAGS_HAS_GUN | 13 | Объект имеет пистолет |
| FLAGS_BURNING | 14 | Объект горит |
| FLAGS_HAS_ATTACHED_SOUND | 15 | Звук привязан к объекту |
| FLAGS_PERSON_BEEN_SHOT | 16 | Персонаж был ранен |

---

## 4. State Machine (машина состояний)

**Состояния (STATE_*):**

```c
STATE_INIT              (0)
STATE_NORMAL            (1)
STATE_COLLISION         (2)
STATE_ABOUT_TO_REMOVE   (3)
STATE_REMOVE_ME         (4)
STATE_MOVEING           (5)   // Движение (ходьба/бег/боковое)
STATE_IDLE              (6)
STATE_LANDING           (7)
STATE_JUMPING           (8)
STATE_FIGHTING          (9)
STATE_FALLING           (10)
STATE_USE_SCENERY       (11)
STATE_DOWN              (12)  // Лежит
STATE_HIT               (13)
STATE_CHANGE_LOCATION   (14)  // Вход/выход из здания/машины
STATE_DYING             (16)
STATE_DEAD              (17)
STATE_DANGLING          (18)  // Висит на чём-то
STATE_CLIMB_LADDER      (19)
STATE_HIT_RECOIL        (20)
STATE_CLIMBING          (21)
STATE_GUN               (22)  // Вооружённое состояние
STATE_SHOOT             (23)
STATE_DRIVING           (24)
STATE_NAVIGATING        (25)
STATE_WAIT              (26)
STATE_FIGHT             (27)
STATE_STAND_UP          (28)
STATE_MAVIGATING        (29)  // MAV навигация
STATE_GRAPPLING         (30)
STATE_GOTOING           (31)  // Идёт в точку без навигации
STATE_CANNING           (32)  // Держит банку
STATE_CIRCLING          (33)  // Кружит вокруг врага
STATE_HUG_WALL          (34)
STATE_SEARCH            (35)
STATE_CARRY             (36)
STATE_FLOAT             (37)
```

**Как работает State Machine:**

```c
void set_state_function(Thing *t_thing, UBYTE state) {
    // Получить таблицу функций для этого класса и типа
    StateFunction *functions = people_functions[PersonType].StateFunctions;
    // или player_functions[PlayerType].StateFunctions

    // Установить функцию состояния
    t_thing->StateFn = functions[state].StateFn;
    t_thing->State = state;
}
```

`StateFn` вызывается КАЖДЫЙ кадр. Она отвечает за обновление анимации, позиции, проверку завершения состояния.

**SubState — детализация состояния:**

```c
// Движение:
SUB_STATE_WALKING           (1)
SUB_STATE_RUNNING           (2)
SUB_STATE_SIDELING          (3)   // Боковое движение
SUB_STATE_STOPPING          (4)
SUB_STATE_HOP_BACK          (5)
SUB_STATE_STEP_LEFT         (6)
SUB_STATE_STEP_RIGHT        (7)
SUB_STATE_FLIPING           (8)   // Оборот 180°
SUB_STATE_WALKING_BACKWARDS (16)
SUB_STATE_CRAWLING          (19)
SUB_STATE_SNEAKING          (20)

// Прыжки/падения:
SUB_STATE_STANDING_JUMP     (30)
SUB_STATE_RUNNING_JUMP      (32)
SUB_STATE_RUNNING_JUMP_FLY  (33)
SUB_STATE_DROP_LAND         (14)
SUB_STATE_RUNNING_LAND      (15)

// Бой:
SUB_STATE_PUNCH         (90)
SUB_STATE_KICK          (91)
SUB_STATE_BLOCK         (92)
SUB_STATE_GRAPPLE       (93)
SUB_STATE_GRAPPLEE      (94)
SUB_STATE_WALL_KICK     (95)
SUB_STATE_GRAPPLE_HOLD  (98)
SUB_STATE_HEADBUTT      (102)
SUB_STATE_STRANGLE      (103)
```

---

## 5. Управление памятью Thing

**Два связных списка:**
- `PRIMARY_USED` — активные объекты
- `PRIMARY_UNUSED` — свободные слоты

**Лимиты:**
- PRIMARY_THINGS: 400
- SECONDARY_THINGS: 300 (PC) / 50 (PSX)
- Итого максимум: 700 объектов

**Жизненный цикл:**
1. `init_things()` — создаёт оба списка
2. `alloc_primary_thing(class)` — берёт из UNUSED, добавляет в USED (O(1))
3. `free_primary_thing(index)` — перемещает из USED в UNUSED (O(1))
4. `thing_class_head[CLASS]` → `NextLink` — итерация объектов одного класса

**Иерархия:**
- `Parent/Child` — иерархия объектов (капот внутри машины). В основном не используется.
- `LinkParent/LinkChild` — для связных списков управления памятью.

---

## 6. MapWho — пространственный хэш

Каждый `MapElement` содержит `THING_INDEX MapWho` — голову связного списка объектов в этой ячейке.

При перемещении объекта обязательно вызывать `move_thing_on_map()` — иначе MapWho рассинхронизируется и AI перестанет видеть объекты.

**Поиск объектов:**
```c
THING_find_sphere(cx, cy, cz, radius, array, size, class_mask)
THING_find_box(x1, z1, x2, z2, array, size, class_mask)
THING_find_nearest(cx, cy, cz, radius, class_mask)

// Маски классов:
THING_FIND_PEOPLE   = (1 << CLASS_PERSON)
THING_FIND_LIVING   = (1 << CLASS_PERSON) | (1 << CLASS_ANIMAL)
THING_FIND_MOVING   = PERSON | ANIMAL | PROJECTILE
```

---

## 7. Система координат и вращение

**Координаты:** SLONG (32-bit)
- 256 мировых единиц = 1 mapsquare (map block)
- `>> 8` → mapsquare index
- `>> 10` → lo-res square index

**Вращение (yaw):** UBYTE (0–255)
- 255 = 360°
- 0 = восток (+X)
- 64 = юг (+Z)
- 128 = запад (–X)
- 192 = север (–Z)

---

## 8. Игровой цикл — обработка объектов

```c
process_things(frameindex):
    для каждого Thing в PRIMARY_USED:
        → Thing->StateFn(Thing)        // Обновление поведения
        → plant_feet()                  // Высота под персонажем
        → move_thing() / DY updates    // Физика
        → MapWho sync                   // Обновить позицию в карте

    для каждого CLASS_PERSON:
        → PCOM_process_person()         // Высокоуровневый AI
```

---

## 9. Что переносить в новую версию

**Переносить 1:1:**
- Структуру классов объектов (CLASS_* иерархия)
- State Machine паттерн (State + StateFn)
- SubState систему
- Флаги (FLAGS_*, FLAG_PERSON_*)
- MapWho пространственный хэш (можно заменить реализацию, но сохранить семантику)

**Можно модернизировать:**
- Связные списки → modern C++ containers (но сохранить O(1) alloc/free)
- Union → std::variant или polymorphism (аккуратно — влияет на производительность)
- Управление памятью → custom allocator или object pool

**Лимиты объектов (700) — пересмотреть:** в современной версии можно увеличить.
