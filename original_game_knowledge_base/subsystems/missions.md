# Система миссий — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/Mission.cpp` — основная логика миссий
- `fallen/Headers/Mission.h` — структуры и константы
- `fallen/Source/EventPoint.cpp` — EventPoint граф
- `fallen/Headers/EventPoint.h` — структуры EP
- `fallen/Source/Level.cpp` — загрузка уровней и миссий

---

## 1. Структура Mission

```c
struct Mission {
    char Name[32];           // Название миссии
    char MapName[32];        // Имя карты (файл .pam/.pi)
    UBYTE MapIndex;          // Индекс карты (0..19)
    UBYTE Flags;             // Флаги миссии
    UWORD EventPointHead;    // Голова linked list EventPoints
    UWORD NumEventPoints;    // Количество EventPoints
    // Setup данные для объектов уровня:
    PlayerSetup  Player;
    EnemySetup   Enemies[MAX_ENEMIES];
    ItemSetup    Items[MAX_ITEMS];
    VehicleSetup Vehicles[MAX_VEHICLES];
};
```

**Лимиты:**
- `MAX_MISSIONS = 100`
- `MAX_MAPS = 20`
- `MAX_EVENT_POINTS = 512` (на миссию)

---

## 2. Структура EventPoint

Граф триггеров и waypoints, описывающий сценарий миссии.

```c
struct EventPoint {
    SLONG X, Y, Z;          // Позиция в мировых координатах
    UWORD Radius;            // Радиус активации (в world units)
    UBYTE Type;              // Тип (TT_* или WPT_*)
    UBYTE Flags;             // EP_FLAG_*
    UWORD Next;              // Следующий EP в linked list
    UWORD Data[10];          // Параметры (зависят от типа)
};
```

EventPoints образуют граф: одни активируют другие через `Data[]` поля.

---

## 3. Типы триггеров (TT_*)

45 типов триггерных событий:

| Группа | Триггеры |
|--------|----------|
| Активация зоны | `TT_ENTER_ZONE`, `TT_EXIT_ZONE`, `TT_IN_ZONE` |
| Персонажи | `TT_PERSON_DEAD`, `TT_PERSON_ALIVE`, `TT_PERSON_NEAR`, `TT_PERSON_RESCUED` |
| Предметы | `TT_ITEM_COLLECTED`, `TT_ITEM_DESTROYED`, `TT_ITEM_NEAR` |
| Транспорт | `TT_VEHICLE_NEAR`, `TT_VEHICLE_DESTROYED`, `TT_VEHICLE_IN_ZONE` |
| Игрок | `TT_PLAYER_DEAD`, `TT_PLAYER_ARRESTED`, `TT_PLAYER_HAS_ITEM` |
| Время | `TT_TIMER_EXPIRED`, `TT_TIMER_START` |
| Скрипт | `TT_SCRIPT_EVENT` — вызывается из MuckyBasic |
| Миссия | `TT_MISSION_SUCCESS`, `TT_MISSION_FAILED` |
| Разное | `TT_ALWAYS`, `TT_NEVER`, `TT_RANDOM` |

---

## 4. Типы waypoints (WPT_*)

57 типов команд для NPC и игровых событий:

| Группа | Waypoints |
|--------|-----------|
| Движение | `WPT_GOTO`, `WPT_RUN_TO`, `WPT_PATROL`, `WPT_FOLLOW`, `WPT_FLEE` |
| Действия | `WPT_WAIT`, `WPT_WAIT_FOR_PLAYER`, `WPT_ANIMATE` |
| Персонажи | `WPT_SPAWN_PERSON`, `WPT_KILL_PERSON`, `WPT_SET_AI` |
| Диалог | `WPT_PLAY_DIALOG`, `WPT_SHOW_TEXT` |
| Транспорт | `WPT_ENTER_VEHICLE`, `WPT_EXIT_VEHICLE`, `WPT_DRIVE_TO` |
| Камера | `WPT_CAMERA_LOOK`, `WPT_CAMERA_CUT` |
| Игровой мир | `WPT_OPEN_DOOR`, `WPT_LOCK_DOOR`, `WPT_TRIGGER_ALARM` |
| Миссия | `WPT_MISSION_SUCCESS`, `WPT_MISSION_FAILED`, `WPT_CHECKPOINT` |
| Звук | `WPT_PLAY_SOUND`, `WPT_STOP_SOUND` |

---

## 5. Режимы операций (OT_*)

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

## 9. Взаимодействие со скриптами

MuckyBasic скрипт (.ucm) может:
- Вызывать `TT_SCRIPT_EVENT` для активации EventPoints
- Устанавливать состояния NPC через API функции
- Читать/писать переменные миссии

EventPoints могут запускать выполнение скриптовых функций через `WPT_PLAY_DIALOG` и аналогичные.

---

## 10. Что переносить в новую версию

**Переносить 1:1:**
- Структуру EventPoint с полем Data[10]
- Все TT_* и WPT_* типы (должны быть совместимы с оригинальными .ucm скриптами)
- Загрузку/сохранение бинарного формата миссий
- Граф активации EventPoints (linked list с Radius-based trigger)

**Можно улучшить:**
- Вместо raw Data[10] использовать union или typed parameters
- Добавить отладочную визуализацию EventPoint графа
- Сохранить лимиты (512 EP) или увеличить если нужно

**Критично:** Порядок активации EventPoints и логика триггеров должны воспроизводиться точно — это основа всего сценария миссий.
