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
- Каждый из 512 эвент-вэйпойнтов проверяет своё условие
- Если условие истинно → выполнить действие (WaypointType)
- Ссылки на Things: по индексу вэйпойнта-создателя (arg1), не по имени

### Режимы активации (OnTrigger)

```
OT_NONE / OT_ACTIVE      — всегда активен
OT_ACTIVE_WHILE          — активен пока условие истинно
OT_ACTIVE_TIME           — активен заданное время (в 1/100 сек)
OT_ACTIVE_DIE            — срабатывает однократно, затем удаляется
```

## 4. Условия триггеров (EWAY_COND_*) — 41 тип

| ID | Имя | Условие |
|----|-----|---------|
| 0 | EWAY_COND_FALSE | Никогда |
| 1 | EWAY_COND_TRUE | Немедленно |
| 2 | EWAY_COND_PROXIMITY | Игрок/Thing в радиусе |
| 3 | EWAY_COND_TRIPWIRE | Пересечение проволоки |
| 4 | EWAY_COND_PRESSURE | Нажата контактная площадка |
| 5 | EWAY_COND_CAMERA | Камера в позиции |
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

## 6. Zone флаги

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

## 10. Глобальный прогресс и сохранение

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

## 11. Что переносить в новую версию

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
