# AI персонажей — Urban Chaos

**Ключевые файлы:**
- `fallen/Headers/pcom.h` — типы AI, состояния, флаги (~250 строк)
- `fallen/Source/Person.cpp` — AI логика (~22K строк)
- `fallen/Headers/Person.h` — структура Person (~249 строк)
- `fallen/Headers/Nav.h` — waypoint навигация
- `fallen/Headers/mav.h` — пространственное планирование (MAV)
- `fallen/Source/combat.cpp` / `fallen/Headers/combat.h` — боевая система

---

## 1. Структура Person

Расширение Thing для персонажей (NPC и игрок).

```c
typedef struct {
    // Базовые
    UBYTE Action;           // Текущее действие
    UBYTE SubAction;        // Подействие
    SWORD Health;           // Здоровье (0 = мёртв)

    // Цель и навигация
    THING_INDEX Target;     // Враг в поле зрения
    THING_INDEX InWay;      // Препятствие на пути
    THING_INDEX InCar;      // В какой машине
    THING_INDEX InBike;     // На каком байке
    UWORD NavIndex;         // Индекс текущей навигационной точки
    SWORD NavX, NavZ;       // Целевая позиция навигации
    SWORD TargetX, TargetZ; // Позиция цели

    // Инвентарь
    THING_INDEX SpecialList; // Список спецпредметов
    THING_INDEX SpecialUse;  // Используемый предмет
    THING_INDEX SpecialDraw; // Отображаемый предмет
    SWORD Hold;              // Удерживаемый объект
    UBYTE Ammo;              // Боеприпасы в пистолете

    // Характеристики
    UBYTE Mode;             // Режим движения (бег/ходьба/крадение)
    UBYTE Power;            // Сила удара (0–255)
    UBYTE Stamina;          // Выносливость
    UBYTE GotoSpeed;        // Скорость перемещения
    UBYTE FightRating;      // Навык боя + группировка
    SWORD Agression;        // Агрессивность

    // Позиция и ориентация
    UWORD HomeX, HomeZ;     // Домашняя позиция
    UWORD GotoX, GotoZ;     // Целевая позиция
    UBYTE HomeYaw;          // Домашнее направление
    UBYTE AttackAngle;      // Угол атаки (0–255)

    // AI система (высокоуровневая)
    UBYTE pcom_ai;          // Тип AI (PCOM_AI_*)
    UBYTE pcom_ai_state;    // Состояние AI (PCOM_AI_STATE_*)
    UBYTE pcom_ai_substate; // Подсостояние
    UWORD pcom_ai_counter;  // Таймер для AI
    UWORD pcom_ai_arg;      // Аргумент для AI
    UWORD pcom_ai_other;    // Вспомогательная переменная

    // AI система (движение)
    UBYTE pcom_move;          // Тип движения (PCOM_MOVE_*)
    UBYTE pcom_move_state;    // Состояние движения
    UBYTE pcom_move_substate; // Подсостояние
    UWORD pcom_move_counter;  // Таймер
    UWORD pcom_move_arg;      // Аргумент
    UWORD pcom_move_follow;   // Точка следования

    // Прочее
    UBYTE BurnIndex;        // Индекс pyro если горит
    UWORD UnderAttack;      // 0 = нет, иначе таймер
    UBYTE Ware;             // Индекс хранилища (если внутри)
    UBYTE pcom_zone;        // Зона патрулирования
    SBYTE CombatNode;       // Позиция в дереве боевых комбо
    GameCoord GunMuzzle;    // Позиция дула пистолета (PC)
} PersonStruct;
```

---

## 2. Типы персонажей (PERSON_*)

```c
PERSON_DARCI         (0)   // Главная героиня (игрок)
PERSON_ROPER         (1)   // Второй персонаж (игрок)
PERSON_COP           (2)   // Полицейский
PERSON_THUG_RASTA    (4)   // Головорез (растаман)
PERSON_THUG_GREY     (5)   // Головорез (серый)
PERSON_THUG_RED      (6)   // Головорез (красный)
PERSON_SLAG_TART     (7)   // Девушка
PERSON_SLAG_FATUGLY  (8)   // Толстая женщина
PERSON_HOSTAGE       (9)   // Заложник
PERSON_MECHANIC      (10)  // Механик
PERSON_TRAMP         (11)  // Бродяга
PERSON_MIB1/2/3      (12-14) // Men In Black
```

---

## 3. Флаги Person (FLAG_PERSON_*)

```c
FLAG_PERSON_NON_INT_M         // Не интерактивен (мужчина)
FLAG_PERSON_NON_INT_C         // Не интерактивен (женщина)
FLAG_PERSON_LOCK_ANIM_CHANGE  // Анимация заблокирована
FLAG_PERSON_GUN_OUT           // Пистолет выведен
FLAG_PERSON_DRIVING           // Управляет машиной
FLAG_PERSON_SLIDING           // Скользит (tackling)
FLAG_PERSON_NO_RETURN_TO_NORMAL // Не возвращается в нормальное состояние
FLAG_PERSON_HIT_WALL          // Столкнулся со стеной
FLAG_PERSON_USEABLE           // Можно взаимодействовать (клавиша U)
FLAG_PERSON_REQUEST_KICK      // AI запросила удар ногой
FLAG_PERSON_REQUEST_JUMP      // AI запросила прыжок
FLAG_PERSON_NAV_TO_KILL       // Навигирует к цели для убийства
FLAG_PERSON_ON_CABLE          // На кабеле
FLAG_PERSON_GRAPPLING         // Использует grappling hook
FLAG_PERSON_HELPLESS          // Не может выполнять AI команды
FLAG_PERSON_CANNING           // Держит банку
FLAG_PERSON_REQUEST_PUNCH     // AI запросила удар кулаком
FLAG_PERSON_REQUEST_BLOCK     // AI запросила блокировку
FLAG_PERSON_FALL_BACKWARDS    // Падает назад
FLAG_PERSON_BIKING            // Катается на байке
FLAG_PERSON_PASSENGER         // Пассажир в машине
FLAG_PERSON_HIT               // Был поражён
FLAG_PERSON_SPRINTING         // Спринт
FLAG_PERSON_FELON             // Разыскивается полицией
FLAG_PERSON_PEEING            // Мочится
FLAG_PERSON_SEARCHED          // Обыскан полицией
FLAG_PERSON_ARRESTED          // Арестован
FLAG_PERSON_BARRELING         // Держит бочку
FLAG_PERSON_MOVE_ANGLETO      // Движется под углом к цели
FLAG_PERSON_KO                // Выбит из сознания
FLAG_PERSON_WAREHOUSE         // Внутри склада
FLAG_PERSON_KILL_WITH_A_PURPOSE // Убивает с конкретной целью

// Flags2:
FLAG2_PERSON_INVULNERABLE     // Неуязвим
FLAG2_PERSON_IS_MURDERER      // Убийца
FLAG2_PERSON_CARRYING         // Несёт объект
FLAG2_PERSON_GUILTY           // Виновен в преступлении
```

---

## 4. PCOM — система AI (Person Commands)

### Типы AI (PCOM_AI_*)

```c
PCOM_AI_NONE          (0)   // Бездействует
PCOM_AI_CIV           (1)   // Гражданский (блуждает/патрулирует)
PCOM_AI_GUARD         (2)   // Охранник (защищает территорию)
PCOM_AI_ASSASIN       (3)   // Ассасин (убивает конкретного врага)
PCOM_AI_BOSS          (4)   // Босс (атакует после гибели группы)
PCOM_AI_COP           (5)   // Полицейский (арестовывает преступников)
PCOM_AI_GANG          (6)   // Банда (атакует при виде)
PCOM_AI_DOORMAN       (7)   // Охранник двери
PCOM_AI_BODYGUARD     (8)   // Телохранитель конкретного NPC
PCOM_AI_DRIVER        (9)   // Водитель (ищет машину, ездит)
PCOM_AI_BDISPOSER     (10)  // Сапёр (разряжает бомбы)
PCOM_AI_BIKER         (11)  // Байкер (ищет мотоцикл)
PCOM_AI_FIGHT_TEST    (12)  // Боевой манекен (тестирование)
PCOM_AI_BULLY         (13)  // Задира (убивает всех не из банды)
PCOM_AI_COP_DRIVER    (14)  // Полицейский водитель
PCOM_AI_SUICIDE       (15)  // Самоубийца
PCOM_AI_FLEE_PLAYER   (16)  // Убегает от игрока
PCOM_AI_KILL_COLOUR   (17)  // Убивает всех одного цвета/группировки
PCOM_AI_MIB           (18)  // Men in Black (спецагент)
PCOM_AI_BANE          (19)  // Главный антагонист
PCOM_AI_HYPOCHONDRIA  (20)  // Ипохондрик (ищет здоровье)
PCOM_AI_SHOOT_DEAD    (21)  // Ассасин-снайпер
```

### Состояния AI (PCOM_AI_STATE_*)

```c
PCOM_AI_STATE_PLAYER        (0)   // Это игрок (не AI)
PCOM_AI_STATE_NORMAL        (1)   // Нормальное поведение
PCOM_AI_STATE_INVESTIGATING (2)   // Исследует подозрительный звук
PCOM_AI_STATE_SEARCHING     (3)   // Активный поиск нарушителя
PCOM_AI_STATE_KILLING       (4)   // Пытается убить цель
PCOM_AI_STATE_SLEEPING      (5)   // Спит
PCOM_AI_STATE_FLEE_PLACE    (6)   // Убегает из места
PCOM_AI_STATE_FLEE_PERSON   (7)   // Убегает от персонажа
PCOM_AI_STATE_FOLLOWING     (8)   // Следует за целью
PCOM_AI_STATE_NAVTOKILL     (9)   // Навигирует к цели для убийства
PCOM_AI_STATE_HOMESICK      (10)  // Возвращается домой
PCOM_AI_STATE_LOOKAROUND    (11)  // Осматривается (поиск врагов)
PCOM_AI_STATE_FINDCAR       (12)  // Ищет машину
PCOM_AI_STATE_BDEACTIVATE   (13)  // Разряжает бомбу
PCOM_AI_STATE_LEAVECAR      (14)  // Выходит из машины
PCOM_AI_STATE_SNIPE         (15)  // Снайперская позиция
PCOM_AI_STATE_WARM_HANDS    (16)  // Греет руки у огня
PCOM_AI_STATE_FINDBIKE      (17)  // Ищет байк
PCOM_AI_STATE_KNOCKEDOUT    (18)  // Выбит из сознания
PCOM_AI_STATE_TAUNT         (19)  // Дразнит врага перед боем
PCOM_AI_STATE_ARREST        (20)  // Арестовывает нарушителя
PCOM_AI_STATE_TALK          (21)  // Разговаривает
PCOM_AI_STATE_GRAPPLED      (22)  // Удерживается в захвате
PCOM_AI_STATE_HITCH         (23)  // Садится как пассажир
PCOM_AI_STATE_AIMLESS       (24)  // Ищет машину, не находит
PCOM_AI_STATE_HANDS_UP      (25)  // Руки вверх (сдача)
PCOM_AI_STATE_SUMMON        (26)  // Вызывает подкрепление
PCOM_AI_STATE_GETITEM       (27)  // Идёт за предметом
```

### Типы движения (PCOM_MOVE_*)

```c
PCOM_MOVE_STILL         (1)  // Стоит
PCOM_MOVE_PATROL        (2)  // Патрулирует по waypoints
PCOM_MOVE_PATROL_RAND   (3)  // Случайный патруль
PCOM_MOVE_WANDER        (4)  // Блуждает по улицам
PCOM_MOVE_FOLLOW        (5)  // Следует за NPC
PCOM_MOVE_WARM_HANDS    (6)  // Греет руки у огня
PCOM_MOVE_FOLLOW_ON_SEE (7)  // Следует если видит цель
PCOM_MOVE_DANCE         (8)  // Танцует
PCOM_MOVE_HANDS_UP      (9)  // Руки вверх (статичный)
PCOM_MOVE_TIED_UP       (10) // Связан
```

### Черты персонажа (PCOM_BENT_*)

```c
PCOM_BENT_LAZY             (0)  // Движется медленнее
PCOM_BENT_DILIGENT         (1)  // Движется быстрее
PCOM_BENT_GANG             (2)  // Член банды (защищает товарищей)
PCOM_BENT_FIGHT_BACK       (3)  // Может драться в ответ
PCOM_BENT_ONLY_KILL_PLAYER (4)  // Убивает только игрока
PCOM_BENT_ROBOT            (5)  // Игнорирует всё вокруг
PCOM_BENT_RESTRICTED       (6)  // Не прыгает и не лазит
PCOM_BENT_PLAYERKILL       (7)  // Убить может только игрок
```

---

## 5. Восприятие — видимость и слышимость

```c
SLONG can_i_see_player(Thing *p_person);
SLONG can_a_see_b(Thing *a, Thing *b, SLONG range=0, SLONG no_los=0);
SLONG can_i_see_place(Thing *p_person, SLONG x, SLONG y, SLONG z);
```

**Факторы:**
1. Дистанция до цели (~20 mapsquare = видимость)
2. Угол зрения (~160° впереди)
3. Line of Sight (нет препятствий между A и B)
4. Состояние цели

**no_los=1** — игнорировать проверку прямой видимости (например слышимость).

---

## 6. Система навигации

### NAV — waypoint-based

```c
typedef struct {
    UWORD x;       // X позиция точки
    UWORD z;       // Z позиция точки
    UBYTE flag;    // NAV_WAYPOINT_FLAG_LAST, FLAG_PULLUP и т.д.
    UBYTE length;  // Количество точек после этой
    UWORD next;    // Индекс следующей точки
} NAV_Waypoint;
// Максимум 512 waypoints
```

Использование:
```c
UWORD first = NAV_do(x1, z1, x2, z2, flags);
// Идти по: NAV_WAYPOINT(first) → NAV_WAYPOINT(first)->next → ...
// Конец пути: flag & NAV_WAYPOINT_FLAG_LAST
```

### MAV — пространственное планирование (More Advanced Nav)

Работает на уровне mapsquares (не waypoints). Определяет что нужно сделать чтобы перейти из одного квадрата в соседний.

```c
// Данные MAV карты
#define MAV_NAV(x,z)    (MAV_nav[(x)*MAV_nav_pitch + (z)] & 1023)      // 10 бит
#define MAV_CAR(x,z)    ((MAV_nav[(x)*MAV_nav_pitch + (z)] >> 10) & 15) // 4 бита
#define MAV_SPARE(x,z)  (MAV_nav[(x)*MAV_nav_pitch + (z)] >> 14)        // 2 бита

// Действия при переходе в соседний квадрат
#define MAV_ACTION_GOTO       (0)  // Просто идти
#define MAV_ACTION_JUMP       (1)  // Прыжок на 1 блок
#define MAV_ACTION_JUMPPULL   (2)  // Прыжок + подтянуться на 1 блок
#define MAV_ACTION_JUMPPULL2  (3)  // Прыжок + подтянуться на 2 блока
#define MAV_ACTION_PULLUP     (4)  // Подтянуться
#define MAV_ACTION_CLIMB_OVER (5)  // Залезть через препятствие
#define MAV_ACTION_FALL_OFF   (6)  // Спрыгнуть вниз
#define MAV_ACTION_LADDER_UP  (7)  // Лестница вверх

// Направления
MAV_DIR_XS (0), MAV_DIR_XL (1), MAV_DIR_ZS (2), MAV_DIR_ZL (3)
```

Использование:
```c
MAV_Action result = MAV_do(me_x, me_z, dest_x, dest_z, MAV_CAPS_DARCI);
// result.action — что делать
// result.dir    — куда
// MAV_do_found_dest — найден ли полный путь
```

**Двухуровневая навигация:**
1. **MAV** — высокоуровневое планирование через mapsquares (глобальный путь)
2. **NAV** — детальное планирование через waypoints (локальные точки)

---

## 7. Боевая система

### Типы ударов (HIT_TYPE_*)

```c
HIT_TYPE_GUN_SHOT_H      (1)   // Выстрел в голову
HIT_TYPE_GUN_SHOT_M      (2)   // Выстрел в туловище
HIT_TYPE_GUN_SHOT_L      (3)   // Выстрел в ноги
HIT_TYPE_PUNCH_H/M/L     (4-6) // Удар кулаком (голова/туловище/ноги)
HIT_TYPE_KICK_H/M/L      (7-9) // Удар ногой
HIT_TYPE_GUN_SHOT_PISTOL (10)
HIT_TYPE_GUN_SHOT_SHOTGUN(11)
HIT_TYPE_GUN_SHOT_AK47   (12)
```

### Комбо система

```c
struct ComboHistory {
    UWORD Owner;
    SBYTE Power[MAX_MOVES];    // Сила каждого удара
    SBYTE Moves[MAX_MOVES];    // Тип удара (punch/kick/knife)
    UWORD Times[MAX_MOVES];    // Время удара
    UBYTE Result[MAX_MOVES];   // Попал/заблокировал
    UBYTE Index, Count;
};

struct BlockingHistory {
    UWORD Owner;
    UBYTE Attack[MAX_MOVES];   // Тип атаки
    UBYTE Flags[MAX_MOVES];    // Флаги (попала/заблокирована)
    UWORD Times[MAX_MOVES];
    UWORD Perp[MAX_MOVES];     // Кто атакует
    UBYTE Index, Count;
};

struct GangAttack {
    UWORD Owner;               // На кого нападают
    UWORD Perp[8];             // 8 направлений атаки (компас)
    UBYTE Index, Count;
};
```

### Ключевые боевые функции

```c
// Найти цель для атаки
SLONG find_attack_stance(
    Thing *p_person,
    SLONG attack_direction,   // FIND_DIR_FRONT / BACK / LEFT / RIGHT
    SLONG attack_distance,
    Thing **stance_target,
    GameCoord *stance_position,
    SLONG *stance_angle);

// Применить урон
SLONG apply_hit_to_person(
    Thing *p_thing,
    SLONG angle,
    SLONG type,               // HIT_TYPE_*
    SLONG damage,
    Thing *p_aggressor,
    struct GameFightCol *fight);

// Повернуться к цели и атаковать
SLONG turn_to_target_and_punch(Thing *p_person);
SLONG turn_to_target_and_kick(Thing *p_person);
```

---

## 8. Режимы и скорости движения

```c
// Режимы
PERSON_MODE_RUN    (0)
PERSON_MODE_WALK   (1)
PERSON_MODE_SNEAK  (2)
PERSON_MODE_FIGHT  (3)  // Боевой (сниженная скорость)
PERSON_MODE_SPRINT (4)  // Спринт (требует выносливости)

// Скорости
PERSON_SPEED_WALK   (1)
PERSON_SPEED_RUN    (2)
PERSON_SPEED_SNEAK  (3)
PERSON_SPEED_SPRINT (4)
PERSON_SPEED_YOMP   (5)  // Быстрая ходьба (марш)
PERSON_SPEED_CRAWL  (6)
```

---

## 9. Поток AI за кадр

```
PCOM_process_person(Person):
    if (pcom_ai == NONE) → return

    switch(pcom_ai_state):
        NORMAL:
            → Движение по pcom_move (патруль/блуждание)
            → can_a_see_b() для каждого врага поблизости
            → Если видит врага → переход в SEARCHING/KILLING

        INVESTIGATING:
            → Идти к подозрительному месту
            → MAV_do() → NAV_do() → move_thing()

        KILLING:
            → find_attack_stance() — позиционирование
            → Если враг рядом → REQUEST_PUNCH/KICK флаг
            → set_state_function(STATE_FIGHTING)
            → apply_hit_to_person() при попадании

        FLEE_PERSON:
            → Найти путь от врага
            → Переход в STATE_NAVIGATING

        ARREST:
            → Полицейский подходит к преступнику
            → Если рядом → анимация ареста
```

---

## 10. Что переносить в новую версию

**Переносить 1:1 (критично для геймплея):**
- Все типы AI (PCOM_AI_*) и их поведение
- Все состояния AI (PCOM_AI_STATE_*) и переходы
- Систему восприятия (can_a_see_b с её параметрами)
- Комбо систему и BlockingHistory
- Двухуровневую навигацию MAV + NAV
- Все боевые константы (HIT_TYPE, урон, дистанции)

**Можно модернизировать:**
- Связные списки навигации → более эффективные структуры
- Таблицы состояний → C++ виртуальные функции или std::function
- PCOM_BENT флаги → более гибкая система характеристик NPC
