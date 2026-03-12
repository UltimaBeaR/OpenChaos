# Транспорт — Urban Chaos

**Ключевые файлы:**
- `fallen/Headers/Vehicle.h` — VehicleStruct, флаги, лимиты
- `fallen/Source/Vehicle.cpp` — физика, вождение, коллизии, повреждения
- `fallen/Source/Bike.cpp`, `fallen/Headers/Bike.h` — мотоциклы (BIKE_*)
- `fallen/Source/Person.cpp` — посадка/высадка

---

## 1. Типы транспорта (VEH_TYPE_*)

```c
VEH_TYPE_VAN       = 0   // Фургон
VEH_TYPE_CAR       = 1   // Легковой автомобиль
VEH_TYPE_TAXI      = 2   // Такси
VEH_TYPE_POLICE    = 3   // Полицейский автомобиль
VEH_TYPE_AMBULANCE = 4   // Скорая помощь
VEH_TYPE_JEEP      = 5   // Джип
VEH_TYPE_MEATWAGON = 6   // Катафалк (грузовик)
VEH_TYPE_SEDAN     = 7   // Седан
VEH_TYPE_WILDCATVAN= 8   // Специальный фургон
```

**Лимит:** `MAX_VEHICLES = 40`

---

## 2. VehicleStruct — все поля

```c
struct VehicleStruct {
    DrawTween Draw;          // угол, tilt, roll для рендеринга

    // Подвеска
    Suspension Spring[4];    // 4 пружины (колёса)
    SLONG DY[4];             // вертикальное смещение колёс

    // Ориентация
    SLONG Angle;             // угол поворота (0-2047)
    SLONG Tilt;              // наклон (неровности)
    SLONG Roll;              // крен (повороты)

    // Управление
    SBYTE Steering;          // угол руля (-64 до +64)
    UBYTE IsAnalog;          // аналоговый ввод
    UBYTE DControl;          // цифровые команды (VEH_ACCEL, VEH_DECEL, VEH_FASTER, VEH_SIREN)
    UBYTE GrabAction;        // захват кнопки действия

    SWORD Wheel;             // позиция руля
    UWORD Allocated;         // 0 если не выделен

    // Флаги состояния
    UWORD Flags;
    // FLAG_VEH_DRIVING       — кто-то управляет
    // FLAG_VEH_WHEEL1..4_GRIP — сцепление каждого колеса
    // FLAG_VEH_ANIMATING     — анимация (дверь открыта)
    // FLAG_VEH_SHOT_AT       — был обстрелян
    // FLAG_VEH_STALLED       — не может ехать
    // FLAG_VEH_IN_AIR        — в воздухе
    // FLAG_FURN_DRIVING      — внутриигровой объект управляет

    UWORD Driver;            // THING_INDEX водителя (0 = нет)
    UWORD Passenger;         // связный список пассажиров

    UWORD Type;              // VEH_TYPE_*

    UBYTE still;             // кадров стоит на месте
    UBYTE dlight;            // динамический источник света
    UBYTE key;               // ключ для разблокировки (SPECIAL_NONE = открыт)
    UBYTE Brakelight;        // тормозной свет (0=выкл, >0=таймер)

    UBYTE damage[6];         // повреждения 6 зон (0-4 уровня)
    SWORD Spin;              // крутящий момент

    SLONG VelX, VelY, VelZ; // скорости
    SLONG VelR;              // ротационная скорость

    SWORD WheelAngle;        // угол поворота колёс
    UBYTE Siren;             // сирена включена
    UBYTE LastSoundState;

    SBYTE Dir;               // направление:
                             // +2 = вперёд, +1 = вперёд с торм.,
                             // -1 = назад с торм., -2 = назад, 0 = стой

    UBYTE Skid;              // занос (>=SKID_START=3 — заносит)
    UBYTE Stable;            // нужно STABLE_COUNT=16 кадров для стабильности
    UBYTE Smokin;            // дымит

    UBYTE Scrapin;           // скребет о стену (звуковой эффект)
    UBYTE OnRoadFlags;       // каждое колесо — на дороге ли

    SWORD Health;            // здоровье (начало: 300, 0 = взрыв)

    SLONG oldX[4], oldZ[4]; // старые координаты колёс (для физики)
};
```

---

## 3. Физика транспорта

### Параметры двигателей

```c
// ENGINE_* — разгон вперёд, разгон назад, мягкое торм., жёсткое торм.
ENGINE_LGV: 17, 10, 4, 8   // фургон
ENGINE_CAR: 21, 10, 4, 8   // легковой
ENGINE_PIG: 25, 15, 5, 10  // полицейский (быстрее, лучше тормоза)
ENGINE_AMB: 25, 10, 5, 8   // скорая помощь
```

### Скоростные лимиты

```c
VEH_SPEED_LIMIT   = 750    // ~35 mph, макс. вперёд
VEH_REVERSE_SPEED = 300    // макс. назад
```

### Подвеска

```c
struct Suspension {
    UWORD Compression;  // сжатие (8.8 fixed-point)
    UWORD Length;       // полная длина
};

MIN_COMPRESSION = 13 << 8  // (0x0D00)
MAX_COMPRESSION = 115 << 8 // (0x7300)
// При сжатии >50% → Smokin = TRUE
```

### Рулевое управление

```c
WHEELTIME  = 35    // кадров до полного поворота руля
WHEELRATIO = 45    // определяет максимальный угол поворота
// Скорость влияет на эффективность поворота

SKID_START   = 3     // кадров сильного торм. перед заносом
SKID_FORCE   = 8500  // боковая сила для заноса
NEAR_SKID_FORCE = 5000  // звук без заноса
```

### Гравитация и единицы

```c
UNITS_PER_METER   = 128
TICKS_PER_SECOND  = 80       // 20 * 4
GRAVITY = -(128 * 10 * 256) / (80^2)
```

---

## 4. Посадка и высадка

### Посадка (`set_person_enter_vehicle`)
```c
void set_person_enter_vehicle(Thing *p_person, Thing *p_vehicle, SLONG door);
// door: 0 = водитель, 1 = пассажир
```

Проверки:
1. Не занята ли водительская позиция
2. Не разбита ли машина (State != STATE_DEAD)
3. Если `Vehicle->key != SPECIAL_NONE` — нужен ключ в инвентаре

Процесс:
1. Скорость персонажа = 0
2. State = STATE_MOVING, SubState = SUB_STATE_ENTERING_VEHICLE
3. Флаги: FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C
4. Анимация: ANIM_ENTER_CAR или ANIM_ENTER_TAXI
5. InCar = THING_NUMBER(p_vehicle)
6. Vehicle->Flags |= FLAG_VEH_ANIMATING

### Высадка (`set_person_exit_vehicle`)

1. Найти свободную сторону (проверка препятствий)
2. Если обе стороны заблокированы — выход внутри машины
3. Удалить из списка Driver / Passenger
4. Сбросить флаги, остановить звуки двигателя

---

## 5. Повреждения

### 6 зон повреждения (0–4 уровня каждая):
```c
damage[0] — левая передняя
damage[1] — правая передняя
damage[2] — левая средняя
damage[3] — правая средняя
damage[4] — левая задняя
damage[5] — правая задняя
```

`void VEH_add_damage(Vehicle *vp, UBYTE area, UBYTE hp)`:
- Повреждения выравниваются между соседними зонами
- При Health ≤ 0 → **взрыв:**
  - PYRO_FIREBOMB + S_EXPLODE_MEDIUM
  - Водитель/пассажиры выброшены
  - State = STATE_DEAD

---

## 6. Коллизии

```c
struct VEH_Col {
    // тип: VEH_COL_TYPE_BBOX или VEH_COL_TYPE_CYLINDER
};
#define VEH_MAX_COL 8   // одновременных коллизий
#define MAX_RUNOVER 8   // персонажей за раз

// Переезд персонажей:
// - THING_find_sphere() радиус 0x200
// - урон через GetRunoverHP() по скорости
// - звук S_THUMP_SQUISH
```

---

## 7. Мотоциклы (BIKE)

**Реализованы полностью**, но с одним ограничением:

```c
#define BIKE_MAX_BIKES 2   // только 2 мотоцикла одновременно!
```

### Режимы мотоцикла
```c
BIKE_MODE_PARKED     = 0
BIKE_MODE_MOUNTING   = 1   // садится
BIKE_MODE_DRIVING    = 2   // едет
BIKE_MODE_DISMOUNTING= 3   // слезает
```

### BIKE_Bike структура
```c
struct BIKE_Bike {
    UWORD yaw, pitch;        // ориентация
    UBYTE flag;              // BIKE_FLAG_USED, BIKE_FLAG_ONGROUND_FRONT/BACK
    UBYTE mode;              // BIKE_MODE_*

    SBYTE accel;             // ускорение
    SBYTE steer;             // поворот

    // Заднее колесо
    SLONG back_x, back_y, back_z;
    SLONG back_dx, back_dy, back_dz;

    // Переднее колесо
    SLONG front_x, front_y, front_z;
    SLONG front_dy;

    // Подвеска колёс
    SLONG wheel_y_back, wheel_y_front;
    SLONG wheel_dy_back, wheel_dy_front;

    UWORD wheel_rot_front, wheel_rot_back;
    UWORD tyrelast;          // последний след шины
    UWORD ribbon, ribbon2;   // следы

    UWORD driver;            // THING_INDEX водителя
    SWORD SlideTimer;        // таймер заноса
    UBYTE dirt;              // поднимается пыль
};
```

### Параметры физики мотоцикла
```c
BIKE_WHEEL_RADIUS = 0x18   // радиус колеса
BIKE_WHEEL_APART  = 0xa0   // расстояние между колёсами
BIKE_WHEEL_COL    = 0x50   // радиус коллизии
BIKE_WHEEL_SUS    = 0x10   // ход подвески
```

### Физика мотоцикла
- Два независимых колеса (переднее и заднее) с позициями и скоростями
- Заднее колесо — приводное
- Подвеска через spring-damper для каждого колеса
- Roll вычисляется из разности высот колёс

### API
```c
void BIKE_init();
void BIKE_create(SLONG x, SLONG z, UWORD yaw);
BOOL BIKE_person_can_mount(Thing *p_person, BIKE_Bike *bike);
void BIKE_set_mounting(BIKE_Bike *bike, Thing *p_person);
void BIKE_set_parked(BIKE_Bike *bike);
void BIKE_set_dismounting(BIKE_Bike *bike);
SLONG BIKE_get_roll(BIKE_Bike *bike);
SLONG BIKE_get_speed(BIKE_Bike *bike);
void BIKE_process_suspension(BIKE_Bike *bike);
void BIKE_process_normal(BIKE_Bike *bike);
```

### Флаги в Person для мотоцикла
```c
FLAG_PERSON_BIKING    = (1<<19)  // управляет мотоциклом
void set_person_mount_bike(Thing *p_person, Thing *p_bike);
void set_person_dismount_bike(Thing *p_person);
ANIM_BIKE_MOUNT = 236
ANIM_BIKE_RIDE  = 237
```

---

## 8. AI в транспорте

```c
void VEH_find_runover_things(Thing *p_vehicle);
// Поиск через THING_find_sphere(радиус 0x200)
// Проверка: не в воздухе, не водитель в этой же машине
// Трансформация через матрицу угла машины
// Проверка коллизии с PRIM_INFO
```

Скрежет о стену: `Scrapin` флаг → звуковой эффект пропорционально величине.
Сирена: `Siren` флаг, `LastSoundState` для оптимизации.

---

## 9. Лимиты

| Ресурс | Лимит |
|--------|-------|
| Транспортных средств | 40 |
| Мотоциклов | **2** (жёсткое ограничение) |
| Одновременных коллизий | 8 |
| Переезжаемых персонажей за раз | 8 |

---

## 10. Что переносить в новую версию

| Аспект | Подход |
|--------|--------|
| 9 типов транспорта | Перенести 1:1 |
| VehicleStruct поля | Перенести 1:1 |
| Физика подвески (4 пружины) | Перенести 1:1 (критично для геймплея) |
| Рулевое управление (WHEELTIME/RATIO) | Перенести 1:1 |
| Параметры двигателей | Перенести 1:1 |
| 6 зон повреждения | Перенести 1:1 |
| Посадка/высадка | Перенести 1:1 |
| BIKE система | Перенести 1:1, включая лимит 2 мотоцикла |
| Переезд персонажей | Перенести 1:1 |
| DirectDraw/PSX специфика в Draw | Заменить на современный рендер |
