# Персонажи и анимации — Urban Chaos

**Ключевые файлы:**
- `Person.h`, `Person.cpp` — главная структура персонажа, типы, состояния
- `Editor/Headers/Anim.h` — структуры анимации (GameKeyFrame, GameKeyFrameChunk)
- `drawtype.h` — DrawTween (управление анимацией в реальном времени)

---

## 1. Типы персонажей (PERSON_TYPE_*)

Определено **15 типов** (PERSON_NUM_TYPES = 15):

```c
PERSON_DARCI        = 0   // Главная героиня (protagonistka)
PERSON_ROPER        = 1   // Второй playable character
PERSON_COP          = 2   // Полицейский
PERSON_CIV          = 3   // Гражданский
PERSON_THUG_RASTA   = 4   // Бандит (дреды)
PERSON_THUG_GREY    = 5   // Бандит (серый)
PERSON_THUG_RED     = 6   // Бандит (красный)
PERSON_SLAG_TART    = 7   // Женщина-преступница
PERSON_SLAG_FATUGLY = 8   // Толстая женщина
PERSON_HOSTAGE      = 9   // Заложник
PERSON_MECHANIC     = 10  // Механик
PERSON_TRAMP        = 11  // Бродяга
PERSON_MIB1         = 12  // Men in Black #1
PERSON_MIB2         = 13  // Men in Black #2
PERSON_MIB3         = 14  // Men in Black #3
```

Каждый тип имеет свой **ANIM_TYPE** (набор анимаций):
```c
ANIM_TYPE_DARCI  = 0
ANIM_TYPE_ROPER  = 1
ANIM_TYPE_ROPER2 = 2
ANIM_TYPE_CIV    = 3
```

---

## 2. Анимационная система: Vertex Morphing + Tweening

**Тип:** НЕ скелетная анимация. Vertex morphing с tweening между кадрами.
- Каждый ключевой кадр хранит **полный набор позиций вершин** (через иерархию костей)
- Между кадрами — линейная интерполяция (tweening) для плавности

### GameKeyFrameElement — элемент "кости" в кадре

```c
struct GameKeyFrameElement {
    struct CMatrix33 CMatrix;    // Сжатая матрица ротации 3x3
    SWORD OffsetX, OffsetY, OffsetZ;  // Смещение от родителя
    UWORD Next;    // Следующий элемент в цепи
    UWORD Parent;  // Родительский элемент (иерархия)
};
```

### GameKeyFrame — один ключевой кадр

```c
struct GameKeyFrame {
    UBYTE XYZIndex;          // Индекс для быстрого поиска
    UBYTE TweenStep;         // Скорость tweening к следующему кадру
    SWORD Flags;
    GameKeyFrameElement *FirstElement;  // Цепь элементов-костей
    GameKeyFrame *PrevFrame, *NextFrame;
    GameFightCol *Fight;     // Коллизионные данные для боя (если кадр удара)
};
```

### GameKeyFrameChunk — весь набор анимаций персонажа

```c
struct GameKeyFrameChunk {
    UWORD MultiObject[10];               // ID объектов (части тела)
    SLONG ElementCount;
    struct BodyDef *PeopleTypes;         // Типы тела (15 типов)
    struct GameKeyFrame *AnimKeyFrames;  // Все кадры всех анимаций
    struct GameKeyFrame **AnimList;      // Индекс для быстрого доступа
    struct GameKeyFrameElement *TheElements;  // Все элементы
    struct GameFightCol *FightCols;      // Коллизии ударов
    SWORD MaxKeyFrames;
    SLONG MaxElements;
};
```

---

## 3. DrawTween — управление анимацией в реальном времени

**Главная рабочая структура** каждого персонажа (в `drawtype.h`):

```c
typedef struct {
    UBYTE TweakSpeed;          // Скорость воспроизведения (0-255)
    SBYTE Locked;              // Заблокированный кадр (или -1)
    UBYTE FrameIndex;          // Текущий кадр
    UBYTE QueuedFrameIndex;    // Следующий в очереди

    SWORD Angle, AngleTo;      // Текущий угол поворота и целевой
    SWORD Roll, DRoll;         // Крен
    SWORD Tilt, TiltTo;        // Наклон

    SLONG CurrentAnim;         // ID текущей анимации
    SLONG AnimTween;           // Стадия tweening
    SLONG TweenStage;          // Прогресс (0-256)

    struct GameKeyFrame *CurrentFrame;    // Текущий кадр
    struct GameKeyFrame *NextFrame;       // Следующий кадр
    struct GameKeyFrame *InterruptFrame;  // Кадр-прерывание (МЁРТВЫЙ КОД — всегда 0 в пре-релизе!)
    struct GameKeyFrame *QueuedFrame;     // В очереди

    struct GameKeyFrameChunk *TheChunk;  // Набор анимаций персонажа

    UBYTE Flags;
    UBYTE Drawn;   // Когда последний раз отрисован
    UBYTE MeshID;  // ID модели (вариант)
    UBYTE PersonID; // ID текстуры/лица (0-10 вариантов)
} DrawTween;
```

### Инициализация:
```c
person_thing->Draw.Tweened = alloc_draw_tween(DT_ROT_MULTI);
person_thing->Draw.Tweened->TheChunk = &game_chunk[anim_type[type]];
```

---

## 4. Управление анимацией — API

```c
void set_anim(Thing *p_person, SLONG anim);        // Сразу переключить
void tween_to_anim(Thing *p_person, SLONG anim);   // Плавный переход
void queue_anim(Thing *p_person, SLONG anim);      // Добавить в очередь
```

### Основные анимации (ANIM_*):
```c
// Движение
ANIM_IDLE          // Стояние
ANIM_WALK          // Ходьба
ANIM_RUN           // Бег

// Лазание
ANIM_MOUNT_LADDER        = 13
ANIM_ON_LADDER           = 14
ANIM_OFF_LADDER_TOP      = 55
ANIM_OFF_LADDER_BOT      = 56
ANIM_LAND_ON_FENCE       = 57
ANIM_CLIMB_OVER_FENCE    = 58
ANIM_CLIMB_UP_FENCE      = 45

// Транспорт
ANIM_ENTER_CAR  = 89
ANIM_EXIT_CAR   = 90
ANIM_BIKE_MOUNT = 236
ANIM_BIKE_RIDE  = 237

// Гарпун
ANIM_GRAPPLING_HOOK_WINDUP  = 137
ANIM_GRAPPLING_HOOK_RELEASE = 138
ANIM_GRAPPLING_HOOK_PICKUP  = 140
```

---

## 5. Режимы скорости (PERSON_MODE_*)

```c
PERSON_MODE_RUN    = 0
PERSON_MODE_WALK   = 1
PERSON_MODE_SNEAK  = 2
PERSON_MODE_FIGHT  = 3
PERSON_MODE_SPRINT = 4
```

---

## 6. Здоровье и смерть

```c
struct Person {
    SWORD Health;  // Текущее здоровье
    ...
};
extern SWORD health[];  // Начальное здоровье по типам персонажей
```

### Функции:
```c
void set_person_dead(
    Thing *p_thing,
    Thing *p_aggressor,
    SLONG death_type,    // PERSON_DEATH_TYPE_*
    SLONG behind,
    SLONG height);

void knock_person_down(
    Thing *p_person,
    SLONG hitpoints,
    SLONG origin_x,
    SLONG origin_z,
    Thing *p_aggressor);
```

### Типы смерти (PERSON_DEATH_TYPE_*):
```c
PERSON_DEATH_TYPE_COMBAT       = 1   // Боевой урон
PERSON_DEATH_TYPE_LAND         = 2   // Падение с высоты
PERSON_DEATH_TYPE_OTHER        = 3   // Прочее
PERSON_DEATH_TYPE_LEG_SWEEP    = 5   // Подножка (встаёт снова)
PERSON_DEATH_TYPE_SHOT_PISTOL  = 10
PERSON_DEATH_TYPE_SHOT_SHOTGUN = 11
PERSON_DEATH_TYPE_SHOT_AK47    = 12
```

**Ragdoll: ОТСУТСТВУЕТ.** Смерть = воспроизведение анимации + персонаж остаётся неподвижным.

---

## 7. Специальные состояния

### Вождение и мотоцикл:
```c
FLAG_PERSON_DRIVING   = (1<<4)   // Управляет машиной
FLAG_PERSON_BIKING    = (1<<19)  // Управляет мотоциклом
FLAG_PERSON_PASSENGER = (1<<20)  // Пассажир

void set_person_enter_vehicle(Thing *p_person, Thing *p_vehicle, SLONG door);
void set_person_exit_vehicle(Thing *p_person);
void set_person_mount_bike(Thing *p_person, Thing *p_bike);
void set_person_dismount_bike(Thing *p_person);
```

### Гарпун:
```c
FLAG_PERSON_GRAPPLING = (1<<13)  // Держит гарпун

void set_person_grappling_hook_pickup(Thing *p_person);
void set_person_grappling_hook_release(Thing *p_person);
```

### Лазание по заборам и лестницам:
```c
void set_person_land_on_fence(Thing *p_person, SLONG wall, SLONG set_pos);
void set_person_climb_ladder(Thing *p_person, UWORD storey);
```

### Плавание: **не реализовано** (флаг воды MAV_SPARE_FLAG_WATER есть, но анимации нет).

---

## 8. Ключевые флаги персонажа

```c
// Flags (32 бита)
FLAG_PERSON_GUN_OUT     = (1<<3)   // Оружие выведено
FLAG_PERSON_DRIVING     = (1<<4)
FLAG_PERSON_SLIDING     = (1<<5)   // Скользит
FLAG_PERSON_GRAPPLING   = (1<<13)
FLAG_PERSON_HELPLESS    = (1<<14)  // Не может действовать
FLAG_PERSON_CANNING     = (1<<15)  // Держит банку
FLAG_PERSON_KO          = (1<<29)  // Нокаут
FLAG_PERSON_WAREHOUSE   = (1<<30)  // Внутри склада
FLAG_PERSON_BIKING      = (1<<19)
FLAG_PERSON_PASSENGER   = (1<<20)
FLAG_PERSON_BARRELING   = (1<<27)  // Держит бочку

// Flags2 (8 бит)
FLAG2_PERSON_CARRYING      = (1<<7)  // Несёт что-то / кого-то
FLAG2_PERSON_INVULNERABLE  = (1<<3)  // Неуязвим
```

---

## 9. Внешний вид и вариации

В DrawTween:
- `MeshID` — выбор модели (мужчина/женщина, разные геометрии)
- `PersonID` — выбор текстуры/лица (до 10 вариантов на тип)

Для гражданских — 4 случайных варианта:
```c
person_thing->Draw.Tweened->PersonID = 6 + random_number % 4;
```

---

## 10. Специфика по типам персонажей (из анализа исходников)

### Таблица здоровья

| Тип | PERSON_TYPE | Health | AnimType | MeshID | Статус кода |
|-----|-------------|--------|----------|--------|------------|
| Darci | 0 | 200 | DARCI(0) | 0 | Полная реализация |
| Roper | 1 | **400** | ROPER(2) | 0 | fn_roper_normal() **пустая** |
| Cop | 2 | 200 | CIV(1) | 4 | fn_cop_normal() **#if 0** |
| CIV | 3 | 130 | CIV(1) | 7 | через PCOM |
| Thug (все) | 4-6 | 200 | CIV(1) | 0-2 | **ASSERT(0)** в init |
| Slag/Hostage/etc | 7-11 | 200 | DARCI(0) | 1-3 | через PCOM |
| MIB 1/2/3 | 12-14 | **700** | CIV(1) | 5 | через PCOM |

### Darci (PERSON_DARCI = 0) — единственный полностью реализованный

- **Падение:** `DY < -30000` → мгновенная смерть; `DY < -20000` → `damage = (-DY - 20000) / 100`
- Звуки падения: PCOM_SOUND_DROP (низкое), DROP_MED, DROP_BIG; ScreamFallSound при смерти
- Физика на крышах складов: скольжение по WARE_inside()
- Коллизия с заборами: обнаружение + лазание
- Гравитация: 4<<8 = 1024 ед/тик, терминальная скорость: -30000
- Анимация при инициализации: `ANIM_STAND_READY`

### Roper (PERSON_ROPER = 1) — почти пуст

- Health = **400** (в 2× больше Darci)
- `fn_roper_normal()` — **полностью пустая функция**, всё поведение через общий Person.cpp
- Начальная скорость: 10 единиц
- Roper: урон огнестрельным оружием ×2, ближний бой +20

### Cop (PERSON_COP = 2) — отключён в пре-релизе

- `fn_cop_normal()` — **обёрнута в `#if 0`** (весь код закомментирован)
- Внутри: патрулирование по вэйпойнтам, угловая интерполяция (сдвиг >>4), случайное idle (50%)
- Система звуковых оповещений: PCOM_SOUND_GUNSHOT(6) > FIGHT(5) > ALARM(4) > HEY(3) > UNUSUAL(2) > FOOTSTEP(1)
- AI-типы: PCOM_AI_COP(5), PCOM_AI_COP_DRIVER(14), PCOM_AI_ARREST(20)

### Thug (PERSON_THUG_* = 4-6) — сломан в пре-релизе

- `fn_thug_init()` содержит **ASSERT(0)** — инициализация упадёт
- Весь нормальный код также в `#if 0`
- **В финальной игре**: `people_functions[]` маппит PERSON_THUG_RASTA/GREY/RED → `cop_states`, поэтому запускается `fn_cop_init()` (не fn_thug_init). fn_thug_init никогда не вызывается в рабочем коде.

### Архитектурное подтверждение (people_functions[] таблица)

```cpp
// Person.cpp
GenusFunctions people_functions[] = {
    { PERSON_DARCI,          darci_states },
    { PERSON_ROPER,          roper_states },
    { PERSON_COP,            cop_states   },
    { PERSON_CIV,            cop_states   },
    { PERSON_THUG_RASTA,     cop_states   },  // НЕ thug_states!
    { PERSON_THUG_GREY,      cop_states   },
    { PERSON_THUG_RED,       cop_states   },
    { PERSON_SLAG_TART,      cop_states   },
    // ... все остальные → cop_states
    { PERSON_MIB1/2/3,       cop_states   },
};
```

**Итог:**
- Все NPC кроме Darci/Roper → `fn_cop_init()` (реально работает), потом STATE_NORMAL = `fn_cop_normal()` (#if 0, мёртвый)
- Весь AI через PCOM_process_person, state functions = заглушки после init
- `fn_roper_normal()` пуста — Roper поведение только через PCOM
- `fn_thug_init()` с ASSERT(0) — НИКОГДА не вызывается (thug_states не в people_functions!)

### Canid/Dog — сломан в пре-релизе

- Большинство функций содержат **ASSERT(0)**
- Архитектура: CANID_SUBSTATE_SLEEP(1), PROWL(2), CHASE(3), FLEE(4), BARK(5)
- Радиус обнаружения: 0xd0 (~208 ед.), угол зрения: 256 единиц дуги
- Скорость: PROWL=6, CHASE=4
- Анимации: idle=1, бег=2
- **Не переносить** (не было в финальной игре — MIB Diskett подтверждает)

### SubClass.cpp — НЕ система подклассов персонажей

Это инструмент **редактора**:
- `DeleteCivs()`, `DeleteCars()`, `DeleteMissions()` — утилиты очистки
- `save_prim_map()`, `update_prims_on_map()` — работа с объектами
- Настройки миссий: crime rate, civvy count, boredom rate
- **Никак не влияет на gameplay**

### AnimBank — загрузка анимаций

```cpp
// global_anim_array[4][450] — индекс AnimType × AnimID
load_anim_system(&game_chunk[0], "data\\darci1.all");
load_anim_system(&game_chunk[1], "data\\hero.all");      // Roper
load_anim_system(&game_chunk[3], "data\\bossprtg.all");  // CIV
append_anim_system(&game_chunk[1], "police1.all", 200, 0);  // доп. анимации копа
append_anim_system(&game_chunk[3], "newciv.all", CIV_M_START, 1);
```

Darci заимствует некоторые анимации CIV (например, вход в такси).

---

## 11. CMatrix33 — сжатый формат матриц анимации

### Структуры (Editor/Headers/Prim.h):

```c
struct CMatrix33 {
    SLONG M[3];  // 3 упакованных 32-bit слова, не 3×3 float!
};
// Каждый M[row] = (elem0 << 22) | (elem1 << 12) | (elem2 << 2)
// 10-bit элементы, range -512..511 (маски: CMAT0_MASK=0x3ff00000, CMAT1_MASK=0x000ffc00, CMAT2_MASK=0x000003ff)

struct Matrix33 {
    SLONG M[3][3];  // Рабочая матрица (распакованная)
};
```

**ВНИМАНИЕ:** В DDEngine/Headers/Quaternion.h есть своя `class FloatMatrix { float M[3][3]; }` — это ДРУГАЯ структура, только для рендерера.

### ULTRA_COMPRESSED_ANIMATIONS — хранилище (8 байт на элемент):

```c
struct GameKeyFrameElement {      // ULTRA_COMPRESSED_ANIMATIONS path (PSX+PC)
    SBYTE m00, m01;  // 2 из 3 значений строки 0 (range -127..127 = -1.0..1.0, scale=128)
    SBYTE m10, m11;  // 2 из 3 значений строки 1
    SBYTE OffsetX, OffsetY, OffsetZ;  // Смещение кости (тоже в scale=128 units)
    UBYTE Pad;       // Битовые поля для декомпрессии
};
```

### Декомпрессия GetCMatrix():

```c
// UCA_Lookup[a][b] = Root(16383 - a² - b²)  -- pre-computed table (128×128 entries)
// Scale: значения в диапазоне [-127,127] где 127 ≈ 1.0 (scale factor = 128)

void GetCMatrix(GameKeyFrameElement *e, CMatrix33 *cm) {
    // 1. Читаем m00/m01 из packed SLONG
    a = byte0; b = byte1;
    c = UCA_Lookup[a][b];         // c = sqrt(1 - a² - b²) в той же шкале
    if (Pad & 1) c = -c;          // знак c для строки 0
    // Pad bits 2-3 = позиция c в строке 0: 0→col0, 4→col2, 8→col1
    // → определяем m00, m01, m02

    // 2. Читаем m10/m11 из bytes 2/3
    // Pad bits 4-5 = позиция c в строке 1: 0→col0, 16→col2, 32→col1
    // → определяем m10, m11, m12

    // 3. Строка 2 = cross product строк 0 и 1 (с >>7 для деления на scale)
    m20 = (m01*m12 >> 7) - (m02*m11 >> 7)  // clamp [-127,127]

    // 4. Упаковываем в CMatrix33 формат:
    cm->M[0] = (m00 << 22) | (m01 << 12) | (m02 << 2)
    cm->M[1] = (m10 << 22) | (m11 << 12) | (m12 << 2)
    cm->M[2] = (m20 << 22) | (m21 << 12) | (m22 << 2)
}
```

**InterruptFrame = МЁРТВЫЙ КОД**: поле объявлено в DrawTween, но в этом пре-релизе везде `InterruptFrame = 0`. Нигде не присваивается ненулевое значение. Была идея "прерывающего кадра при перебитии анимации", но так и не реализована.

---

## 12. Физика Darci (Darci.cpp — 1431 строк)

Ранее не задокументирована. `projectile_move_thing()` — основной цикл airborne-физики (576 строк).

**Гравитация и падение:**
- GRAVITY = `(4<<8)` = 1024 units/tick
- Terminal velocity: DY = -30000 (hard clamp)
- **Fall damage формула:** `damage = (-DY - 20000) / 100` (линейная, НЕ экспоненциальная)
- DY ≤ -30000 → damage = 250 (instant kill)
- DY ≥ -20000 → damage = 0

**Death plunge (спецанимация падения):**
- Player порог: DY < -12000
- NPC порог: DY < -6000
- Lookahead: проверяет 3 шага вперёд (`dx*i>>5`); если нет пола в 0x600 units → ANIM_PLUNGE_START
- Velocity обнуляется при plunge

**Velocity scaling:** все velocity inputs: `(vel*3)>>2` (×0.75, конвертация FPS)
- `change_velocity_to()`: half-step per frame (asymptotic)
- `change_velocity_to_slow()`: 1 unit/frame
- `trickle_velocity_to()`: 1 unit/frame in either direction

**Wall collision при прыжке:** `slide_along(SLIDE_ALONG_FLAG_JUMPING)`
- Fence landing → `set_person_land_on_fence()` (если не unclimbable)
- Wall kick-off: **#if 0** (мёртвый код)
- Jump-grab: **#ifdef DOG_POO** (мёртвый код)

**Sewer special path:** `NS_slide_along()` (нет object collisions), `NS_calc_height_at()`

**Barrel collision:** `BARREL_hit_with_sphere()` radius 0x70 при каждом движении

**Return values projectile_move_thing:** 0=no collision, 1=landed, 2=fence/wall, 100=death

