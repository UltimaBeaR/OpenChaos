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

## 5. Восприятие — видимость

`can_a_see_b(Thing *observer, Thing *target, SLONG range=0, SLONG no_los=0)`

**Дальность видения (range=0 → default):**
- NPC по умолчанию: `8 << 8` = 2048 world-units
- Игрок: `256 << 8` = 65536 (видит всегда на max дистанцию)
- Отрицательный range: игнорировать освещение

**Освещение влияет на дальность NPC:**
```c
light_at_target = NIGHT_get_light_at(target_pos);
view_range = (R+G+B) + (R+G+B)<<3 + (R+G+B)>>2 + 256;
// Хорошее освещение → дальше видят
// Тёмные зоны → ближе
```

**Коррекции дальности:**
- Цель приседает (`crouching`): view_range >>= 1 (половина)
- Цель движется: view_range += 256 (проще заметить)
- Высотная разница: если `dy >= 0x80` (128 units), добавляет `dy << 2` к эффективной дистанции

**Угол зрения (FOV), из 2048 = 360°:**
- Близко (dist < 0xc0 = 192 units): FOV = 700 (≈123°)
- Далеко (dist ≥ 192): FOV = 420 (≈74°)
- "Краем глаза" (250/2048 ≈ 44°): видит на половинную дистанцию только если цель не движется

**Line of Sight:**
- Eye height standing: 0x60 (96 units)
- Eye height crouching: 0x20 (32 units)
- Проверяется через `there_is_a_los()` от головы наблюдателя к голове цели

---

## 5b. Восприятие — звуки

`PCOM_oscillate_tympanum(type, originator, x, y, z, store_it)`

Звуки распространяются с радиусом (в world-units):

```c
PCOM_SOUND_FOOTSTEP      = 0x280   // 640 units / 2.5 squares
PCOM_SOUND_VAN           = 0x180   // 384 units / 1.5 squares (самый тихий)
PCOM_SOUND_DROP          = 0x200   // 512 units / 2 squares
PCOM_SOUND_MINE          = 0x300   // 768 units / 3 squares
PCOM_SOUND_GRENADE_HIT   = 0x300
PCOM_SOUND_GRENADE_FLY   = 0x300
PCOM_SOUND_LOOKINGATME   = 0x400
PCOM_SOUND_WANKER        = 0x400
PCOM_SOUND_DROP_MED      = 0x400   // 1024 units / 4 squares
PCOM_SOUND_UNUSUAL       = 0x600   // 1536 units / 6 squares
PCOM_SOUND_HEY           = 0x600
PCOM_SOUND_DRAW_GUN      = 0x600   // визуальная проверка (нужен LOS!)
PCOM_SOUND_DROP_BIG      = 0x600
PCOM_SOUND_BANG          = 0x700   // 1792 units / 7 squares
PCOM_SOUND_ALARM         = 0x800   // 2048 units / 8 squares
PCOM_SOUND_FIGHT         = 0x900   // 2304 units / 9 squares
PCOM_SOUND_GUNSHOT       = 0xa00   // 2560 units / 10 squares (самый громкий)
```

**Фильтры распространения:**
- Zone check: NPC с `pcom_zone` слышат только звуки из своей зоны
- Warehouse boundary: звуки не проникают через стены склада
- PCOM_SOUND_VAN пугает только гражданских
- PCOM_SOUND_DRAW_GUN требует LOS (это визуальный триггер)
- Гранаты: нужно видеть гранату (LOS к гранате)

**Реакция:**
- Большинство звуков → `PCOM_AI_STATE_INVESTIGATING`
- Бой/выстрел → `PCOM_AI_STATE_SEARCHING` или сразу KILLING

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

## 8b. Собаки (canid) — статус в пре-релизе

**Файл:** `fallen/Source/canid.cpp` (~1475 строк)

**ВАЖНО: В этой сборке собаки полностью инертны!**

Главный dispatch switch в `CANID_fn_normal()` закомментирован — ни одно состояние собаки не обрабатывается. Все остальные функции (`CANID_6sense`, `CANID_can_see`, `CANID_LOS`, `CANID_Homing`) реализованы, но не вызываются.

**Состояния собак (в коде, но не активны):**
- Patrol, Chase, Attack, Flee
- `CANID_6sense()` — дальность обнаружения
- `CANID_init_prowl()`, `CANID_init_chase()` — переходы состояний

**Вывод:** это пре-релизная сборка. В финальной игре собаки могут присутствовать в отдельных миссиях (уточнить у автора).

---

## 9. Групповое поведение (банды и полиция)

### Оповещение банды

```c
PCOM_alert_my_gang_to_a_fight(attacker, target)
// → все члены банды (same pcom_colour) в состояниях
//   NORMAL/WARM_HANDS/FOLLOWING/SEARCHING/TAUNT/INVESTIGATING
//   переходят в KILLING с тем же target

PCOM_alert_my_gang_to_flee(threat, source)
// → не-GANG_flagged члены начинают флить вместе
```

- Группировка по `pcom_colour` (0–15), не по внешнему цвету
- `PCOM_BENT_GANG` (бит 2): NPC защищает членов своей банды

### Гражданские: воскрешение

Блуждающие гражданские (PCOM_MOVE_WANDER) воскресают если мертвы:
- Не в поле зрения игрока (флаг `FLAGS_IN_VIEW` = 0)
- Мертвы 200+ кадров → перемещаются на home позицию с полным здоровьем
- Только для WANDER-режима (не патруль)

### Полиция: арест

1. Нарушитель получает флаг `FLAG_PERSON_FELON` через `PCOM_call_cop_to_arrest_me()`
2. Каждые 4 кадра коп проверяет: сканирует сферу радиуса 0x800 (2048 units)
3. Если находит `FLAG_PERSON_FELON` + есть LOS → переходит в `PCOM_AI_STATE_ARREST`
4. Водитель-коп (COP_DRIVER): паркует машину, выходит, арестовывает
5. Другие копы слышат `PCOM_SOUND_HEY` (радиус 0x600) → сходятся на место

**Приоритет цели для ареста:**
- Расстояние + высота: ближе = выше приоритет
- Игрок: score <<= 1 (деприоритет)
- Wandering civvies: score <<= 2 (деприоритет)
- С флагом GUILTY: score >>= 2 (высокий приоритет)

---

## 9b. Числовые константы

```c
#define PCOM_TICKS_PER_TURN    16         // Тиков за кадр
#define PCOM_TICKS_PER_SEC     (16 * 20)  // = 320 тиков/сек

#define PCOM_ARRIVE_DIST       0x40       // 64 units — считается "прибыл"

#define RMAX_PEOPLE            180        // Максимум NPC в сцене
#define PCOM_MAX_GANGS         16         // Максимум банд (цвета 0–15)
#define PCOM_MAX_GANG_PEOPLE   64         // Суммарно членов банд
#define PCOM_MAX_FIND          16         // Максимум результатов sphere-поиска
```

`PCOM_get_duration(tenths)` = `tenths * 32` тиков (конвертация из десятых секунды)

---

## 10. Поток AI за кадр

```
PCOM_process_person(Thing *p_person):  // pcom.cpp:13003
    StateFn(p_person)                   // анимационный state machine
    if PlayerID:
        idle→pcom_move_counter++ → если >PCOM_get_duration(30) и рядом танцующий NPC:
            Darci копирует его танец (dance join логика)
        return  // игрок: больше ничего не делаем

    PCOM_process_movement(p_person)     // низкоуровневое движение

    if HELPLESS: return

    if DEAD/DYING:
        if AI==CIV && MOVE==WANDER && !IN_VIEW:
            pcom_ai_counter++ → если >200: воскресить (см. 9)
    else:
        PCOM_process_state_change(p_person)  // главный AI dispatcher
```

### PCOM_process_default() — диспетчер состояний

Вызывается большинством AI-типов. Простой switch по `pcom_ai_state`:

| Состояние | Обработчик |
|-----------|-----------|
| NORMAL | PCOM_process_normal() |
| INVESTIGATING | PCOM_process_investigating() |
| **SEARCHING** | **пусто (stub, break)** |
| KILLING | PCOM_process_killing() |
| **SLEEPING** | **пусто (stub, break)** |
| FLEE_PLACE / FLEE_PERSON | PCOM_process_fleeing() |
| FOLLOWING | PCOM_process_following() |
| NAVTOKILL | PCOM_process_navtokill() |
| HOMESICK | PCOM_process_homesick() |
| **LOOKAROUND** | **только счётчик (no code)** |
| FINDCAR | PCOM_process_findcar() |
| BDEACTIVATE | PCOM_process_bdeactivate() |
| LEAVECAR | PCOM_process_leavecar() |
| SNIPE | PCOM_process_snipe() |
| WARM_HANDS | PCOM_process_warm_hands() |
| KNOCKEDOUT | PCOM_process_knockedout() |
| TAUNT | PCOM_process_taunt() |
| ARREST | PCOM_process_arrest() |
| TALK | PCOM_process_talk() |
| HITCH | PCOM_process_hitch() |
| AIMLESS | PCOM_process_wander() |
| HANDS_UP | PCOM_process_hands_up() |
| GETITEM | PCOM_process_getitem() |
| default | ASSERT(0) |

Примечание: FINDBIKE обёрнут в `#ifdef BIKE` (отключён — байк не переносится).

### PCOM_process_normal() — диспетчер NORMAL state

Маршрутизирует по `pcom_move`:

- **STILL / DANCE / HANDS_UP / TIED_UP:**
  - Если уже в STATE_MOVEING с SIMPLE_ANIM → проверяет сидит/чешется, иначе scratch animation
  - Если далеко от дома (>256 units) → HOMESICK
  - GUARD на домашней позиции: рисует пистолет если не нарисован
  - LAZY-флаг: каждые 0x3f тиков ищет скамейку/диван в радиусе 0x200=512, садится если нашёл
- **PATROL / PATROL_RAND** → `PCOM_process_patrol()`
- **WANDER** → `PCOM_process_wander()`
- **FOLLOW**: если CANTFIND substate — каждые 16 кадров пробует `can_a_see_b` к цели, иначе wander
- **FOLLOW_ON_SEE**: если дистанция < 0x200 И `can_a_see_b` → переключается на FOLLOW
- **WARM_HANDS**: если далеко от дома (>0x200) → HOMESICK, иначе `PCOM_set_person_ai_warm_hands()`

---

## 10b. MIB AI детали

**PCOM_AI_MIB (18)** — самый агрессивный AI, всегда убивает игрока:

```
PCOM_process_state_change → case PCOM_AI_MIB:
    if pcom_ai_state == NORMAL:
        if can_a_see_b(me, Darci) && !stealth_debug:
            PCOM_alert_nearby_mib_to_attack(me)  // mass aggro
    PCOM_process_default(me)  // обычные состояния
```

**PCOM_alert_nearby_mib_to_attack()** — массовый aggro trigger (pcom.cpp:10825):
- `THING_find_sphere(radius=0x500=1280 units)` — все персонажи поблизости
- Для каждого NPC с pcom_ai ∈ {MIB, GUARD, GANG, FIGHT_TEST}:
  - Если NOT HELPLESS и в состоянии NORMAL/HOMESICK/INVESTIGATING:
  - → `PCOM_set_person_ai_navtokill(found, Darci)` (немедленно начинает преследование)

**Ключевые отличия MIB от других AI:**
- Не ждёт звука/события — проверяет can_a_see_b каждый кадр в NORMAL state
- Групповой aggro: один MIB увидел → все MIB/GUARD/GANG/FIGHT_TEST в радиусе 1280 атакуют
- Нет агрессивного порога — убивает всегда (не как COP/GANG с условиями)
- KO невозможен (Health=700, FLAG2_PERSON_INVULNERABLE)

---

## 10c. Bane AI детали

**PCOM_AI_BANE (19)** — всегда в состоянии SUMMON:

```
case PCOM_AI_BANE:
    if pcom_ai_state != SUMMON: PCOM_set_person_ai_summon(me)
    PCOM_process_summon(me)
```

**PCOM_process_summon()** — 2 подсостояния (pcom.cpp:5280):

**SUMMON_START:** ждёт анимацию SUB_STATE_FLOAT_BOB, затем:
- Поднимает до 4 тел (PCOM_summon[0..3]) в воздух (`set_person_float_up()`)
- Создаёт SPARK электродуги от конечностей Bane к телам (лимбы: L_HAND, R_HAND, L_FOOT, R_FOOT → таз жертвы)
- Переходит в SUMMON_FLOAT

**SUMMON_FLOAT:** каждые `PCOM_get_duration(50)=160 тиков` пересоздаёт SPARK дуги.

Близость Darci → электрошок:
- `dx+dz < 0x60000` (Darci рядом) → `pcom_move_counter` растёт
- Если Darci смещается → `pcom_move_counter >>= 1` (сброс при движении)
- `pcom_move_counter >= PCOM_get_duration(20)=640 тиков (~2 сек на месте)` → electrocute:
  - `set_person_recoil(Darci, ANIM_HIT_FRONT_MID, 0)` + `Health -= 25`
  - SPARK дуга Bane.PELVIS → Darci.PELVIS (intensity=50)
  - `pcom_move_counter = 0` (сброс)

**Вывод:** Bane не атакует напрямую — он статичен, поднимает тела, и если игрок стоит рядом 2 секунды — бьёт током.

---

## 10d. Driving AI детали

**PCOM_AI_DRIVER (9) / PCOM_AI_COP_DRIVER (14):**

COP_DRIVER → fallthrough в DRIVER логику.

```
NORMAL state:
    if !FLAG_PERSON_DRIVING:
        PCOM_set_person_ai_findcar(me, NULL)  // ищет любую машину
    else:
        dispatch по pcom_move:
            STILL  → PCOM_process_driving_still()
            PATROL/PATROL_RAND → PCOM_process_driving_patrol()
            WANDER → PCOM_process_driving_wander()
            FOLLOW → (пусто — не реализовано)
non-NORMAL states: PCOM_process_default()
```

**Важно:** логика COP_DRIVER (выйти из машины и арестовать) закомментирована (`/* ... */`).

**Driving movement states** (в PCOM_process_movement):
- `PCOM_MOVE_STATE_DRIVETO (8)` — едет к конкретной точке
- `PCOM_MOVE_STATE_DRIVE_DOWN (11)` — едет по дорожному графу (ROAD wander)
- Завершение DRIVE_DOWN → переход к следующему NAV-узлу дороги

---

## 10e. Воскрешение гражданских — детали реализации

(pcom.cpp:13104)

```c
if (pcom_ai == PCOM_AI_CIV && pcom_move == PCOM_MOVE_WANDER && !IN_VIEW):
    pcom_ai_counter++
    if pcom_ai_counter > 200:
        newpos = (HomeX<<16+0x8000, HomeZ<<16+0x8000, terrain_height)
        // ⚠️ БАГИ пре-релиза: newpos вычисляется но НЕ присваивается p_person->WorldPos!
        plant_feet(p_person)       // просто снижает к полу на ТЕКУЩЕЙ позиции
        Health = health[PersonType] // восстанавливает здоровье
        clear FLAGS_BURNING, BurnIndex = 0
        PCOM_set_person_ai_normal(p_person)
```

Вывод: в пре-релизной версии гражданские воскресают **на месте смерти**, а не на домашней позиции (teleport в newpos — мёртвый код). В финале может быть исправлено.

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
