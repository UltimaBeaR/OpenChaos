# Система миссий — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/Mission.cpp` — основная логика миссий
- `fallen/Headers/Mission.h` — структуры и константы
- `fallen/Source/EventPoint.cpp` — EventPoint граф
- `fallen/Headers/EventPoint.h` — структуры EP
- `fallen/Source/Level.cpp` — загрузка уровней и миссий

---

## 1. Структура Mission

Из `Mission.h` строки 53–71:

```c
struct Mission {
    CBYTE Flags;                              // MISSION_FLAG_*
    CBYTE BriefName[_MAX_PATH];               // Текст брифинга миссии
    CBYTE LightMapName[_MAX_PATH];            // Файл карты освещения/теней
    CBYTE MapName[_MAX_PATH];                 // Файл геометрии уровня
    CBYTE MissionName[_MAX_PATH];             // Идентификатор миссии
    CBYTE CitSezMapName[_MAX_PATH];           // Карта канализации/интерьеров
    UWORD MapIndex;                           // Ссылка на игровую карту
    UWORD FreeEPoints;                        // Количество неиспользованных EP
    UWORD UsedEPoints;                        // Количество активных EP
    UBYTE CrimeRate;                          // Уровень преступности (0–100)
    UBYTE CivsRate;                           // Порог потерь среди гражданских
    EventPoint EventPoints[MAX_EVENTPOINTS];  // Массив event points (512 max)
    UBYTE SkillLevels[254];                   // AI скиллы по типам врагов
    UBYTE BoredomRate;                        // Скорость поведения на покое
    UBYTE CarsRate;                           // Плотность трафика
    UBYTE MusicWorld;                         // Выбор музыкальной дорожки
};
```

**Флаги миссии (Mission.h строки 49–51):**
```c
#define MISSION_FLAG_USED                 (1 << 0)
#define MISSION_FLAG_SHOW_CRIMERATE       (1 << 1)
#define MISSION_FLAG_CARS_WITH_ROAD_PRIMS (1 << 2)
```

**Лимиты:**
- `MAX_EVENTPOINTS = 512` (event points на миссию)

---

## 2. Структура EventPoint

Из `Mission.h` строки 18–41:

```c
struct EventPoint {
    UBYTE Colour;        // Группа (A-Z) — для группировки связанных EP
    UBYTE Group;         // Подгруппа
    UBYTE WaypointType;  // WPT_* — тип действия
    UBYTE Used;          // Флаг активности (0 = неактивен)
    UBYTE TriggeredBy;   // TT_* — тип триггера (условие срабатывания)
    UBYTE OnTrigger;     // OT_* — поведение после срабатывания
    UBYTE Direction;     // Угол (0–255, = 0–360°)
    UBYTE Flags;         // WPT_FLAGS_*
    UWORD EPRef;         // Индекс зависимого EP
    UWORD EPRefBool;     // Второй булев вход
    UWORD AfterTimer;    // Таймаут для OT_ACTIVE_TIME (мс)
    SLONG Data[10];      // Данные специфичные для типа
    SLONG Radius;        // Радиус триггера (для TT_RADIUS и др.)
    SLONG X, Y, Z;       // Позиция в мире
    UWORD Next, Prev;    // Linked list указатели
};
```

### Поведение после срабатывания (OT_*)

```c
#define OT_NONE          0   // Выполнить один раз, больше не активен
#define OT_ACTIVE        1   // Всегда активен (для непрерывных триггеров — radius)
#define OT_ACTIVE_WHILE  2   // Активен пока условие выполняется
#define OT_ACTIVE_TIME   3   // Активен указанное время (поле AfterTimer)
#define OT_ACTIVE_DIE    4   // Активен пока цель не умрёт
```

EventPoints образуют граф: одни активируют другие через `Data[]` поля.

---

## 3. EWAY — реальная система событий и скриптинга

> **ВАЖНО:** `.ucm` файлы — это **EWAY Mission Data** (EventPoint массивы), **НЕ MuckyBasic скрипты**.
> MuckyBasic — отдельная изолированная система, не интегрированная с игрой. См. [muckybasic.md](muckybasic.md).
>
> Реальная логика миссий — непрерывный опрос (polling) `EWAY_process()` каждый кадр.

Файлы: `fallen/Source/eway.cpp` (8229 строк), `fallen/Headers/eway.h`, `fallen/Headers/Mission.h`

### Модель выполнения

- Вызывается **каждый кадр** из main game loop
- Индекс 0 зарезервирован — никогда не используется; валидный диапазон: `1..EWAY_way_upto-1`
- PC: все вэйпойнты обрабатываются каждый кадр (`step=1, offset=0`)
- PSX: оптимизация чётных/нечётных кадров (`step=2, offset=GAME_TURN&1`), кроме кадра 0 (step=1)
- Стоп если `GS_LEVEL_LOST | GS_LEVEL_WON` — вэйпойнты перестают обрабатываться
- EWAY_time += 80 × TICK_RATIO >> TICK_SHIFT каждый кадр (≈1000 ед/сек при 50 fps)

**Per-waypoint per-frame логика:**
```
if EWAY_FLAG_DEAD → skip
if EWAY_FLAG_ACTIVE:
    if EMIT_STEAM/WATER_SPOUT/SHAKE_CAMERA/VISIBLE_COUNT_UP → run continuous effect
    if EWAY_FLAG_COUNTDOWN → decrement timer; if ≤0 → set_inactive
    else → check EWAY_Stay rule:
        EWAY_STAY_ALWAYS      → set EWAY_FLAG_DEAD immediately (fire-once, destroy waypoint)
        EWAY_STAY_WHILE       → re-evaluate condition; if FALSE → set_inactive
        EWAY_STAY_WHILE_TIME  → re-evaluate; if FALSE → start countdown timer → set_inactive after delay
        EWAY_STAY_TIME        → timer already set at activation, not re-checked here
else (not ACTIVE):
    → evaluate_condition; if TRUE → set_active
```

**После main loop:**
- `EWAY_process_conversation()` — продвигает активный диалог
- `EWAY_process_penalties()` — показывает штрафы конусов на HUD
- `EWAY_process_camera()` — двигает катсценовую камеру

### Режимы активации (EWAY_Stay)

```
EWAY_STAY_ALWAYS      — сработать один раз → сразу DEAD (= "одноразовый")
EWAY_STAY_WHILE       — активен пока условие истинно (re-poll каждый кадр)
EWAY_STAY_WHILE_TIME  — активен пока условие + N тиков после отпускания
EWAY_STAY_TIME        — активен фиксированное время (N тиков от активации)
EWAY_STAY_DIE         — как ALWAYS, но через set_inactive (редкий вариант)
```

## 4. Условия триггеров (EWAY_COND_*) — 41 тип

| ID | Имя | Условие |
|----|-----|---------|
| 0 | EWAY_COND_FALSE | Никогда |
| 1 | EWAY_COND_TRUE | Немедленно |
| 2 | EWAY_COND_PROXIMITY | Игрок/Thing в радиусе |
| 3 | EWAY_COND_TRIPWIRE | Пересечение проволоки |
| 4 | EWAY_COND_PRESSURE | **STUB — always FALSE** (не реализован в пре-релизе) |
| 5 | EWAY_COND_CAMERA | **STUB — always FALSE** (не реализован в пре-релизе) |
| 6 | EWAY_COND_SWITCH | Рычаг/выключатель активирован |
| 7 | EWAY_COND_TIME | Таймер (arg1 = время в 1/100 сек) |
| 8 | EWAY_COND_DEPENDENT | Другой вэйпойнт (arg1) активен |
| 9 | EWAY_COND_BOOL_AND | Оба условия истинны |
| 10 | EWAY_COND_BOOL_OR | Хотя бы одно условие истинно |
| 11 | EWAY_COND_COUNTDOWN | Таймер с зависимостью (arg1=dep, arg2=время) |
| 12 | EWAY_COND_COUNTDOWN_SEE | Видимый обратный отсчёт |
| 13 | EWAY_COND_PERSON_DEAD | Персонаж (arg1=ID вэйпойнта) погиб |
| 14 | EWAY_COND_PERSON_NEAR | Персонаж (arg1) в радиусе (arg2) |
| 15 | EWAY_COND_CAMERA_AT | Камера достигла вэйпойнта |
| 16 | EWAY_COND_PLAYER_CUBE | Игрок в прямоугольной зоне |
| 17 | EWAY_COND_A_SEE_B | Персонаж (arg1) видит персонажа (arg2) |
| 18 | EWAY_COND_GROUP_DEAD | Все из группы мертвы |
| 19 | EWAY_COND_HALF_DEAD | Персонаж (arg1) при 50% здоровья |
| 20 | EWAY_COND_PERSON_USED | Игрок взаимодействует с персонажем (arg1) |
| 21 | EWAY_COND_ITEM_HELD | Игрок держит предмет (arg1) |
| 22 | EWAY_COND_RADIUS_USED | Игрок нажал USE в радиусе |
| 23 | EWAY_COND_PRIM_DAMAGED | Объект получил урон |
| 24 | EWAY_COND_CONVERSE_END | Разговор (arg1) закончился |
| 25 | EWAY_COND_COUNTER_GTEQ | Счётчик (arg1) >= значения (arg2) |
| 26 | EWAY_COND_PERSON_ARRESTED | Персонаж (arg1) арестован |
| 27 | EWAY_COND_PLAYER_CUBOID | Игрок в 3D-параллелепипеде |
| 28 | EWAY_COND_KILLED_NOT_ARRESTED | Персонаж убит (не арестован) |
| 29 | EWAY_COND_CRIME_RATE_GTEQ | Crime rate >= значения (arg1) |
| 30 | EWAY_COND_CRIME_RATE_LTEQ | Crime rate <= значения (arg1) |
| 31 | EWAY_COND_IS_MURDERER | Персонаж (arg1) убил кого-то |
| 32 | EWAY_COND_PERSON_IN_VEHICLE | Персонаж (arg1) в машине |
| 33 | EWAY_COND_THING_RADIUS_DIR | Объект (arg1) в радиусе + направление |
| 34 | EWAY_COND_SPECIFIC_ITEM_HELD | Конкретный предмет (arg1) в инвентаре |
| 35 | EWAY_COND_RANDOM | Случайный шанс |
| 36 | EWAY_COND_PLAYER_FIRED_GUN | Игрок выстрелил |
| 37 | EWAY_COND_PRIM_ACTIVATED | Рычаг/вентиль в радиусе активирован |
| 38 | EWAY_COND_DARCI_GRABBED | Игрок в захвате |
| 39 | EWAY_COND_PUNCHED_AND_KICKED | Персонаж ударен (arg1=счётчик) |
| 40 | EWAY_COND_MOVE_RADIUS_DIR | Объект (arg1) в радиусе + нужная сторона |
| 41 | EWAY_COND_AFTER_FIRST_GAMETURN | Со 2-го тика |

## 5. Действия вэйпойнтов (WaypointType) — 57 типов

| ID | Имя | Действие |
|----|-----|---------|
| 0 | WPT_NONE | Нет действия |
| 2 | WPT_CREATE_PLAYER | Создать игрока |
| 3 | WPT_CREATE_ENEMIES | Создать NPC-врагов |
| 4 | WPT_CREATE_VEHICLE | Создать машину |
| 5 | WPT_CREATE_ITEM | Создать предмет |
| 6 | WPT_CREATE_CREATURE | Создать животное |
| 7 | WPT_CREATE_CAMERA | Создать камеру |
| 8 | WPT_CREATE_TARGET | Создать маркер цели |
| 9 | WPT_CREATE_MAP_EXIT | Создать выход с уровня |
| 10 | WPT_CAMERA_WAYPOINT | Переместить камеру |
| 11 | WPT_TARGET_WAYPOINT | Переместить маркер цели |
| 12 | WPT_MESSAGE | Показать диалог/сообщение |
| 13 | WPT_SOUND_EFFECT | Воспроизвести звук |
| 14 | WPT_VISUAL_EFFECT | Визуальный эффект |
| 15 | WPT_CUT_SCENE | Воспроизвести катсцену |
| 16 | WPT_TELEPORT | Телепортировать |
| 17 | WPT_TELEPORT_TARGET | Телепортировать цель |
| 18 | WPT_END_GAME_LOSE | Провал миссии |
| 19 | WPT_SHOUT | Персонаж кричит |
| 20 | WPT_ACTIVATE_PRIM | Запустить анимацию объекта |
| 21 | WPT_CREATE_TRAP | Создать ловушку |
| 22 | WPT_ADJUST_ENEMY | Изменить AI врага |
| 23 | WPT_LINK_PLATFORM | Связать физику платформы |
| 24 | WPT_CREATE_BOMB | Создать взрывчатку |
| 25 | WPT_BURN_PRIM | Поджечь объект |
| 26 | WPT_END_GAME_WIN | Победа в миссии |
| 27 | WPT_NAV_BEACON | Навигационный маяк |
| 28 | WPT_SPOT_EFFECT | Фонтан/точечный эффект |
| 29 | WPT_CREATE_BARREL | Создать бочку/урну |
| 30 | WPT_KILL_WAYPOINT | Отключить вэйпойнт (arg1=ID) |
| 31 | WPT_CREATE_TREASURE | Создать коллектабл |
| 32 | WPT_BONUS_POINTS | Дать бонусные очки |
| 33 | WPT_GROUP_LIFE | Активировать все WP одного цвета/группы |
| 34 | WPT_GROUP_DEATH | Деактивировать группу |
| 35 | WPT_CONVERSATION | Диалог между двумя персонажами |
| 36 | WPT_INTERESTING | Отметить как интересное |
| 37 | WPT_INCREMENT | Увеличить счётчик (arg1=ID) |
| 38 | WPT_DYNAMIC_LIGHT | Создать динамический свет |
| 39 | WPT_GOTHERE_DOTHIS | NPC: идти куда-то и сделать что-то |
| 40 | WPT_TRANSFER_PLAYER | Переключить управление на другого игрока |
| 41 | WPT_AUTOSAVE | Автосохранение |
| 42 | WPT_MAKE_SEARCHABLE | Сделать объект обыскиваемым |
| 43 | WPT_LOCK_VEHICLE | Заблокировать/разблокировать машину |
| 44 | WPT_GROUP_RESET | Сбросить состояние группы |
| 45 | WPT_COUNT_UP_TIMER | Запустить видимый таймер |
| 46 | WPT_RESET_COUNTER | Сбросить счётчик (arg1=ID) |
| 47 | WPT_CREATE_MIST | Создать туман |
| 48 | WPT_ENEMY_FLAGS | Установить флаги поведения врага |
| 49 | WPT_STALL_CAR | Остановить машину |
| 50 | WPT_EXTEND | Продлить обратный отсчёт |
| 51 | WPT_MOVE_THING | Переместить объект к вэйпойнту |
| 52 | WPT_MAKE_PERSON_PEE | Анимация мочеиспускания |
| 53 | WPT_CONE_PENALTIES | Включить коллизию конусов |
| 54 | WPT_SIGN | Показать знак/надпись |
| 55 | WPT_WAREFX | Настроить атмосферу склада |
| 56 | WPT_NO_FLOOR | Убрать коллизию пола |
| 57 | WPT_SHAKE_CAMERA | Тряска камеры |

## 6. Режимы операций (OT_*)

Управляют логикой активации EventPoints:
```c
OT_ONCE          // Сработать один раз
OT_REPEAT        // Повторять каждый раз
OT_TOGGLE        // Переключаться
OT_WHILE_TRUE    // Пока условие истинно
```

---

## 6. Детали реализации (из анализа eway.cpp)

### Spawn-детали

- **Координаты вэйпойнта** хранятся в mapsquare-единицах; `EWAY_create_enemy()` умножает на 256 (`<< 8`) перед передачей в `PCOM_create_person()`
- **Yaw** хранится как `yaw >> 3` в `EWAY_Way.yaw`; при создании персонажа передаётся как `yaw << 3` (восстановление). Диапазон 0-255 (8-bit) = 360°
- **zone byte** в `EWAY_create_enemy()`: биты 0-3 = spawn zone (low nibble), бит 4 = `FLAG2_PERSON_INVULNERABLE`, бит 5 = `FLAG2_PERSON_GUILTY`, бит 6 = `FLAG2_PERSON_FAKE_WANDER`
- **NO_PLAYERS** счётчик: инкрементируется в `EWAY_create_player()`. Поддержка мультиплеера до 2 игроков (ASSERT(NO_PLAYERS <= 1) перед созданием)
- **CREATE_PLAYER subtypes**: PLAYER_DARCI (0), PLAYER_ROPER (1), PLAYER_COP (2), PLAYER_THUG (3); default вызывает ASSERT(0)
- **Roper stats bug (пре-релиз)**: `EWAY_create_player(PLAYER_ROPER)` использует Darci-статы вместо Roper-статов (блок Roper закомментирован)

### Непрерывные эффекты while-active

Следующие типы вэйпойнтов исполняют эффект **каждый кадр пока активны** (не только при активации):
- `EWAY_DO_EMIT_STEAM` → `EWAY_process_emit_steam()` каждый кадр
- `EWAY_DO_WATER_SPOUT` → `DIRT_new_water()` 4× каждый кадр (крест: +X, +Z, -Z, -X)
- `EWAY_DO_SHAKE_CAMERA` → `FC_cam[0].shake += 3` (PC) или `+6` (PSX) каждый кадр до max 100
- `EWAY_DO_VISIBLE_COUNT_UP` → инкремент таймера + `PANEL_draw_timer()` каждый кадр; счётчик[3] = секунды

### Post-load fixup (EWAY_created_last_waypoint)

Вызывается один раз после загрузки всех вэйпойнтов:
1. `EWAY_fix_cond()` + `EWAY_fix_do()` + `EWAY_fix_edef()` → resolve ID → array index
2. Подсчёт очков объективов и числа "guilty" людей
3. Инициализация таймеров для COUNTDOWN вэйпойнтов
4. Регистрация строк WHY_LOST для экрана провала
5. CAMERA_TARGET_THING → attach к ближайшему CREATE_* вэйпойнту (radius 0x300)

---

## 6a. Zone флаги

```c
ZONE_FLAG_ACTIVE     // Зона активна
ZONE_FLAG_TRIGGERED  // Зона уже сработала
ZONE_FLAG_ONE_SHOT   // Только одно срабатывание
ZONE_FLAG_INVERSE    // Инвертированное условие (exit вместо enter)
```

---

## 7. Setup-структуры (начальный расстановка объектов)

```c
struct PlayerSetup {
    SLONG X, Y, Z;     // Начальная позиция
    UBYTE Angle;        // Направление
    UWORD WeaponFlags;  // Начальное снаряжение
};

struct EnemySetup {
    SLONG X, Y, Z;
    UBYTE PersonType;   // PCOM_AI_* тип
    UBYTE Angle;
    UWORD Flags;
};

struct ItemSetup {
    SLONG X, Y, Z;
    UBYTE ItemType;     // SPECIAL_* тип
};

struct VehicleSetup {
    SLONG X, Y, Z;
    UBYTE VehicleType;
    UBYTE Angle;
};
```

---

## 8. Бинарный формат сохранения

```
M_VERSION = 10   // Версия формата миссии
EP_VERSION = 1   // Версия формата EventPoint

Структура файла миссии:
[4 байта]  SLONG M_VERSION
[4 байта]  SLONG количество EventPoints
[Для каждого EP]:
  [4 байта]  SLONG X
  [4 байта]  SLONG Y
  [4 байта]  SLONG Z
  [2 байта]  UWORD Radius
  [1 байт]   UBYTE Type
  [1 байт]   UBYTE Flags
  [2 байта]  UWORD Next
  [20 байт]  UWORD Data[10]
[Setup данные персонажей, предметов, транспорта]
```

---

## 9. Важно: .ucm — НЕ MuckyBasic

**`.ucm` файлы = EWAY Mission Data**, не скомпилированные MuckyBasic скрипты.

Вся логика миссий описывается через EventPoint массив (до 512 вэйпойнтов) с условиями EWAY_COND_* и действиями WaypointType. Никаких вызовов MuckyBasic VM нет.

MuckyBasic — отдельная изолированная система (`MuckyBasic/` папка), не интегрированная с игрой. Подробнее: [muckybasic.md](muckybasic.md).

---

## 10. Трансляция WPT_* → EWAY_DO_* (elev.cpp)

> **Важно:** WPT_* значения (Mission.h) и EWAY_DO_* значения (eway.h) — **разные нумерации!**

- `.ucm` файл хранит **WPT_* значения** (редакторский формат)
- При загрузке уровня в `elev.cpp` (строки 748–1654) происходит трансляция в **EWAY_DO_*** (рантайм формат)
- `EWAY_create()` получает уже EWAY_DO_* значения

### Ключевые маппинги:
```
WPT_END_GAME_WIN    → EWAY_DO_MISSION_COMPLETE
WPT_END_GAME_LOSE   → EWAY_DO_MISSION_FAIL
WPT_GROUP_LIFE      → EWAY_DO_GROUP_LIFE
WPT_GROUP_DEATH     → EWAY_DO_GROUP_DEATH
WPT_INCREMENT       → EWAY_DO_INCREASE_COUNTER  (subtype = Data[1] = counter index)
WPT_RESET_COUNTER   → EWAY_DO_RESET_COUNTER     (subtype = Data[0], 0 = все счётчики)
WPT_CONVERSATION    → EWAY_DO_CONVERSATION или EWAY_DO_AMBIENT_CONV (если Data[3]!=0)
WPT_CUT_SCENE       → EWAY_DO_CUTSCENE
WPT_ACTIVATE_PRIM   → EWAY_DO_CONTROL_DOOR или EWAY_DO_ELECTRIFY_FENCE или ... (depends on prim type)
```

### НЕ реализованные типы (пре-релиз):
- **WPT_GOTHERE_DOTHIS (39)** — отсутствует в switch → hits `default: ASSERT(0)`. NPC scripting **не реализован**.
- **WPT_BONUS_POINTS (32)** → маппится в EWAY_DO_MESSAGE (текст) из-за `if(0)` блока:
  ```c
  if (0) {  // "Bonus points are just messages nowadays."
      ed.type = EWAY_DO_OBJECTIVE;  // ← МЁРТВЫЙ КОД, никогда не выполняется
  } else {
      ed.type = EWAY_DO_MESSAGE;    // ← всегда
  }
  ```
  Т.е. WPT_BONUS_POINTS = просто текстовое сообщение, очки не начисляются, CRIME_RATE не меняется.

### Трансляция OT_* → EWAY_STAY_*:
```
OT_NONE / OT_ACTIVE   → EWAY_STAY_ALWAYS
OT_ACTIVE_WHILE       → EWAY_STAY_WHILE
OT_ACTIVE_TIME        → EWAY_STAY_TIME  (es.arg = AfterTimer)
OT_ACTIVE_DIE         → EWAY_STAY_DIE
```

---

## 10a. CRIME_RATE — детали обновления

### Загрузка:
- Загружается из `.iam` файла: `CRIME_RATE = junk[0]`
- Если 0 → устанавливается 50 (дефолт)

### CRIME_RATE_SCORE_MUL (pre-game, в EWAY_created_last_waypoint):
```c
for_guilty = num_guilty_people * 4;              // 4% за каждого guilty NPC
SATURATE(for_guilty, 0, CRIME_RATE >> 1);        // cap at 50% of CRIME_RATE
CRIME_RATE_SCORE_MUL = (CRIME_RATE - for_guilty) << 8 / total_objective_points;
// Если total_points == 0 → CRIME_RATE_SCORE_MUL = 0
```
Fixed-point (Q8) множитель: сколько % CRIME_RATE даёт один "очковый балл".

### Динамическое обновление в игре (Person.cpp):
| Событие | Изменение |
|---------|-----------|
| Убийство guilty NPC | -2 |
| Убийство wandering civ (PCOM_AI_CIV + PCOM_MOVE_WANDER) | +5 |
| Арест guilty NPC | -4 |
| Объектив EWAY_DO_OBJECTIVE (мёртвый код, не вызывается) | -percent |

Все изменения: `SATURATE(CRIME_RATE, 0, 100)`.

### Спец. счётчик: EWAY_counter[7]
- Инкрементируется (до max 255) при каждом убийстве PERSON_COP
- Используется в EWAY_COND_COUNTER_GTEQ условиях

---

## 10b. Win/Lose flow (Game.cpp)

### GS_LEVEL_WON (EWAY_DO_MISSION_COMPLETE):
1. Darci получает FLAG2_PERSON_INVULNERABLE (нельзя умереть при победе)
2. `GAME_STATE = GS_LEVEL_WON`
3. `set_stats()` — записывает только `stat_game_time = GetTickCount() - stat_start_time` (тривиально!)

Inner loop продолжает работать при GS_LEVEL_WON, EWAY_process() тоже — но loop прерывается по другим условиям.

### После выхода из inner loop при GS_LEVEL_WON:
1. `park2.ucm` → RunCutscene(1) (MIB intro)
2. `Finale1.ucm` → RunCutscene(3) + OS_hack() (финальный экран игры)
3. Остальные → **DarciDeadCivWarnings check**:
   - Если `Player->RedMarks > 1` (убиты мирные)
   - Показывает экран `deadcivs.tga` с предупреждением
   - `the_game.DarciDeadCivWarnings 0` → warning 1; `1` → warning 2; `2` → warning 3; `>=3` → **GS_LEVEL_LOST!**
   - `the_game.DarciDeadCivWarnings += 1` — накапливается между миссиями!

### После DarciDeadCivWarnings:
- `GAME_STATE == GS_LEVEL_WON` → PC: `FRONTEND_level_won()`, PSX: `Wadmenu_gameover(1)`
- `GAME_STATE == GS_LEVEL_LOST` → PC: `FRONTEND_level_lost()`, PSX: `Wadmenu_gameover(0)`
- `GAME_STATE == GS_REPLAY` → `goto round_again` (перезапуск той же миссии)

### Статистика (Person.cpp, только для конечного экрана):
```c
stat_killed_thug       // инкр. при убийстве guilty NPC
stat_killed_innocent   // инкр. при убийстве non-guilty NPC
stat_arrested_thug     // инкр. при аресте guilty NPC
stat_arrested_innocent // инкр. при аресте non-guilty NPC
stat_count_bonus       // инкр. вручную (не из кода Person.cpp?)
stat_game_time         // мс от start до set_stats()
```
Статистика инициализируется в `init_stats()` в начале каждого уровня.

### GROUP_LIFE / GROUP_DEATH детали:
- **GROUP_LIFE**: clears EWAY_FLAG_DEAD на всех WP с тем же `colour` AND `group`
  - Иммунны: сами WP типов GROUP_LIFE и GROUP_DEATH
  - Эффект: "воскрешает" мёртвые скрипты
- **GROUP_DEATH**: sets EWAY_FLAG_DEAD на всех WP с тем же `colour` AND `group`
  - Иммунны: сами WP типов GROUP_LIFE и GROUP_DEATH
  - Эффект: отключает целую ветку скрипта

---

## 11. Глобальный прогресс и сохранение

**Файл:** `frontend.cpp` — `FRONTEND_save_savegame()` / `FRONTEND_load_savegame()`
**Путь:** `saves/slot{N}.wag` (N = **1, 2, 3** — 1-based! `save_slot = menu_state.selected + 1`)
**Версия формата:** `version = 3`

### Структура .wag файла (бинарный, sequential write):
```c
// frontend.cpp — FRONTEND_save_savegame():
char mission_name[32];     // текущая миссия
UBYTE complete_point;      // глобальный прогресс (0-24+)
UBYTE version = 3;

// Характеристики Darci:
UBYTE DarciStrength, DarciConstitution, DarciSkill, DarciStamina;
// Характеристики Roper:
UBYTE RoperStrength, RoperConstitution, RoperSkill, RoperStamina;

UBYTE DarciDeadCivWarnings;  // предупреждения за убийство мирных

UBYTE mission_hierarchy[60]; // статус 60 миссий
                             // bit 2 = миссия завершена

UBYTE best_found[50][4];     // лучшие результаты для 50 миссий (4 байта на миссию)
```

### complete_point — глобальный прогресс
```c
UBYTE complete_point = 0;   // Game.cpp
// Увеличивается при завершении ключевых миссий
// Диапазон: 0–24+ (точное макс. зависит от кол-ва миссий)
// Влияет на визуальный стиль уровней (texture_set) и открытые миссии
```

### mission_hierarchy[60]
```c
UBYTE mission_hierarchy[60];
// Каждый байт — статус одной миссии
// bit 2 (значение 4) = миссия завершена
// Используется для ветвящейся структуры миссий (branching)
```

### CRIME_RATE — динамический параметр миссии
```c
// elev.cpp: CRIME_RATE = junk[0]; — загружается из файла уровня
// Если 0 → устанавливается 50 (дефолт)
// Диапазон: 0–100
SLONG CRIME_RATE = 50;  // default

// EventPoint триггеры:
TT_CRIME_RATE_ABOVE   // сработает если CRIME_RATE превысит порог
TT_CRIME_RATE_BELOW   // сработает если CRIME_RATE упадёт ниже порога

// Влияет на: CRIME_RATE_SCORE_MUL — множитель очков
```

### Характеристики персонажей (RPG-прогресс)
Хранятся в `struct Game` (`Game.h`), сохраняются в .wag:
- `DarciStrength`, `DarciConstitution`, `DarciSkill`, `DarciStamina` (UBYTE каждый)
- `RoperStrength`, `RoperConstitution`, `RoperSkill`, `RoperStamina` (UBYTE каждый)
Прокачиваются по ходу прохождения, влияют на боевые возможности.

---

## 11. Точный бинарный формат .ucm файла

`.ucm` = EWAY Mission Data. Загружается функцией `ELEV_load_level()` в `elev.cpp`.

### Заголовок миссии

```
[4 байта]  SLONG version               — версия формата
[4 байта]  SLONG flag                  — MISSION_FLAG_* биты:
                                          bit0=USED, bit1=SHOW_CRIMERATE, bit2=CARS_WITH_ROAD_PRIMS
[_MAX_PATH] char BriefName[]           — путь к файлу брифинга (_MAX_PATH = 260 байт)
[_MAX_PATH] char LightMapName[]        — путь к .lgt файлу освещения
[_MAX_PATH] char MapName[]             — имя .iam файла карты (без пути)
[_MAX_PATH] char MissionName[]         — имя миссии (строка)
[_MAX_PATH] char CitSezMapName[]       — путь к файлу "cit-sez" (был SewerMapName)
[2 байта]  UWORD MapIndex              — игнорируется при загрузке
[2 байта]  UWORD UsedEPoints           — игнорируется при загрузке
[2 байта]  UWORD FreeEPoints           — игнорируется при загрузке
[1 байт]   UBYTE CrimeRate             — CRIME_RATE (0-100; если 0 → дефолт 50)
[1 байт]   UBYTE FakeCivs              — количество "фоновых" гражданских (PSX: >>1, max 3)
```

Итого заголовок (до EventPoints): **4+4+260×5+2+2+2+1+1 = 1316 байт**

### EventPoint записи

Читаются в цикле `for (i = 0; i < MAX_EVENTPOINTS; i++)`:
```
[14 байт] EventPoint header:
  UBYTE Colour, Group, WaypointType, Used    (4 байта)
  UBYTE TriggeredBy, OnTrigger, Direction, Flags (4 байта)
  UWORD EPRef, EPRefBool, AfterTimer         (6 байт)
[60 байт] EventPoint data:
  SLONG Data[10]                             (40 байт)
  SLONG Radius, X, Y, Z                     (16 байт)
  UWORD Next, Prev                           (4 байт)
```

**EventPoint = ровно 74 байта** (14 заголовок + 60 данных).
`MAX_EVENTPOINTS = 512` → всего 512 × 74 = **37 888 байт** EventPoint секция.

Только записи с `Used != 0` обрабатываются → создаются EWAY_Way вэйпойнты.
Трансляция: `TT_*` → `EWAY_COND_*`, `WPT_*` → `EWAY_DO_*`, `OT_*` → `EWAY_STAY_*` (в elev.cpp:748-1654).

### После EventPoints

Чтение версионное (поле `version` из заголовка):

```
if (version > 5):
  FileSeek(+254)        — пропуск SkillLevels[254] (AI skills per person type), не читается в runtime

FileRead(1 byte)        — BOREDOM_RATE (if version < 10: defaulted to 4; min 4 enforced)

if (version > 8):
  FileRead(1 byte)      — FAKE_CARS (CarsRate в Mission struct) — трафик NPC
  FileRead(1 byte)      — MUSIC_WORLD (1-based; если <1 → 1)
```

После этого идёт секция сообщений (строки брифингов/диалогов):
- `if version < 8`: только сообщения (mess_count строк)
- `if version >= 8`: сообщения + катсцены (mess_count + cutscene_count строк)
- Каждая строка: если `version > 4` — SLONG длина, потом N байт текста; иначе фикс. _MAX_PATH байт

Итого полный .ucm: **≈1316 + 37888 + ~260 ≈ 39 460 байт** (плюс строки сообщений).

**Актуальная версия** (Steam/финал): скорее всего `version >= 10`, то есть все поля присутствуют.

### Путь к .iam карте

```c
// elev.cpp — после загрузки .ucm берёт MapName и строит полный путь:
sprintf(fname_map, "%s%s", DATA_DIR, ucm.MapName);
// → открывает data/MAPNAME.iam через load_game_map()
```

### Путь к .lgt освещению

```c
// LightMapName из .ucm (или "data\\lighting\\jumper1.lgt" по умолчанию)
NIGHT_load_ed_file(fname_lighting);
```

---

## 12. Что переносить в новую версию

**Переносить 1:1:**
- Структуру EventPoint с полем Data[10]
- Все TT_* и WPT_* типы (должны быть совместимы с оригинальными .ucm скриптами)
- Загрузку/сохранение бинарного формата миссий
- Граф активации EventPoints (linked list с Radius-based trigger)
- Формат .wag сохранений (версия 3) — для совместимости с оригинальными save файлами
- mission_hierarchy[60], complete_point (0-24+), best_found[50][4]
- CRIME_RATE (0-100, default 50) с EventPoint триггерами TT_CRIME_RATE_ABOVE/BELOW
- RPG характеристики Darci и Roper (8 UBYTE значений)

**Можно улучшить:**
- Вместо raw Data[10] использовать union или typed parameters
- Добавить отладочную визуализацию EventPoint графа
- Сохранить лимиты (512 EP) или увеличить если нужно

**Критично:** Порядок активации EventPoints и логика триггеров должны воспроизводиться точно — это основа всего сценария миссий.
