# Физика и коллизии — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/collide.cpp` — главный файл физики (~11K строк)
- `fallen/Headers/collide.h` — структуры коллизий
- `fallen/Headers/pap.h` — heightmap структуры
- `fallen/Source/maths.cpp` — математика
- `fallen/Source/Vehicle.cpp` — физика транспорта

---

## 1. Система координат

**Базовый тип — GameCoord:**
```c
struct GameCoord {
    SLONG X;  // 32-bit signed
    SLONG Y;  // Высота
    SLONG Z;  // Глубина
};
```

**Важнейшие сдвиги (shifts):**

| Операция | Назначение |
|----------|-----------|
| `>> 8` | Блок → юнит (делит на 256) |
| `>> 16` | Юнит → индекс mapsquare |
| `<< 8` | Юнит → блок (умножает на 256) |
| `<< 3` (PAP_ALT_SHIFT) | Высота → хранение (×8) |
| `>> 3` | Хранение → высота (÷8) |
| `>> TICK_SHIFT` | Скорость → позиция (интеграция по времени) |

**Размеры карты:**
- PAP_SIZE_HI: 128×128 hi-res mapsquares
- PAP_SIZE_LO: 32×32 lo-res блоков
- PAP_SHIFT_HI: 8 бит (256 блоков на hi-res квадрат)
- ELE_SIZE: 256 юнитов = 1 mapsquare

**Важно:** В физике НЕ используются float — всё на 32-bit integer (SLONG). Это обеспечивает детерминизм между платформами.

---

## 2. Структуры данных коллизий

**CollisionVect** (collide.h, line 35-44):
```c
struct CollisionVect {
    SLONG X[2];      // X-координаты начала и конца отрезка
    SWORD Y[2];      // Высота (16-bit)
    SLONG Z[2];      // Z-координаты начала и конца
    UBYTE PrimType;  // Тип примитива (стена, забор и т.д.)
    UBYTE PrimExtra; // Доп. информация о типе
    SWORD Face;      // К какому face относится
};
```
- Массив: `col_vects[MAX_COL_VECT]` = 10 000 записей на PC (1 000 на PSX)
- Каждый — это линейный отрезок-барьер в 3D пространстве
- Память: ~300 KB

**CollisionVectLink** (collide.h, line 29-33):
```c
struct CollisionVectLink {
    UWORD Next;       // Linked list → следующий линк
    UWORD VectIndex;  // Индекс в col_vects[]
};
```
- Массив: `col_vects_links[MAX_COL_VECT_LINK]` = 10 000 (PC) / 4 000 (PSX)
- Привязывает CollisionVect к mapsquare (spatial hash)
- Память: ~40 KB

**WalkLink** (collide.h, line 47-51):
```c
struct WalkLink {
    UWORD Next;  // Linked list → следующая ссылка
    SWORD Face;  // Индекс walkable-поверхности (позитивный = обычная, негативный = крыша)
};
```
- Массив: `walk_links[MAX_WALK_POOL]` = 30 000 (PC) / 10 000 (PSX)
- Карта по mapsquare → список walkable-поверхностей
- Память: ~120 KB

---

## 3. Heightmap — структуры карты высот

**PAP_Hi** (pap.h, line 86-93) — **6 байт на ячейку** (UWORD+UWORD+SBYTE+SBYTE):
```c
typedef struct {
    UWORD Texture;  // ID текстуры
    UWORD Flags;    // Shadow, reflective, roof флаги
    SBYTE Alt;      // Высота (хранится: actual_height >> 3)
    SBYTE Height;   // Не используется
} PAP_Hi;
// PAP_Hi[128][128] = 128*128*6 = 98 304 байт (~96 KB)
```
- Реальная высота: `Alt << 3` (range: -1024 .. +1016 юнитов)

**PAP_Lo** (pap.h, line 69-77) — lo-res данные:
```c
typedef struct {
    UWORD MapWho;       // Здание/terrain в этом блоке
    SWORD Walkable;     // >0 = обычная face, <0 = крыша
    UWORD ColVectHead;  // Голова linked list коллизионных векторов
    SBYTE water;        // Высота воды (-127 = нет воды)
    UBYTE Flag;         // Склад (warehouse) и т.д.
} PAP_Lo;
// PAP_Lo[32][32]
```

**MapElement** (Map.h) — 14 байт:
```c
struct MapElement {
    LIGHT_Colour Colour;
    SBYTE        AltNotUsedAnyMore;  // устарело
    SWORD        Walkable;
    THING_INDEX  MapWho;             // Голова linked list объектов в ячейке
};
```

---

## 4. Функции получения высоты

**5 вариантов функций — для разных контекстов:**

1. **`PAP_calc_height_at_point(map_x, map_z)`** — высота в угловой точке mapsquare. Прямой lookup без интерполяции.

2. **`PAP_calc_height_at(x, z)`** — интерполированная высота. Билинейная интерполяция между 4 угловыми точками. Input: block-level координаты.

3. **`PAP_calc_height_at_thing(thing*, x, z)`** — с учётом контекста Thing. Проверяет находится ли Thing внутри здания, перенаправляет в `WARE_calc_height_at()` или `NS_calc_height_at()`. Для улицы — вызывает `PAP_calc_height_at()`.

4. **`PAP_calc_map_height_at(x, z)`** — максимум из terrain и зданий.

5. **`PAP_calc_height_noroads(x, z)`** — высота без учёта бордюров. Ищет максимальную высоту в окрестности. Используется для игнорирования curbs.

**`find_face_for_this_pos()` — в `walkable.cpp` (не в collide.cpp!)**
```c
SLONG find_face_for_this_pos(x, y, z, *ret_y, ignore_building, ignore_height_flag)
// Возврат: face index, GRAB_FLOOR (=MAX_PRIM_FACES4+1), или NULL (0=падение)
// FIND_ANYFACE=1: вернуть первый найденный face
// FIND_FACE_NEAR_BELOW=3: вернуть face если dy < 30 единиц выше
// 0 (default): вернуть ближайший (|dy| минимальный), порог кандидата = 0xa0 = 160 единиц
```

Алгоритм:
1. Быстрый путь: если PAP_FLAG_ROOF_EXISTS → проверить roof face сначала
2. Поиск по lo-res ячейкам в диапазоне ±0x200 (±2 ячейки)
3. Для каждого face: `is_thing_on_this_quad()` → `calc_height_on_face()`
4. Если dy <= 0xa0 (160 ед.) → кандидат. Выбирается с минимальным `|dy|`
5. Fallback: `PAP_calc_height_at()`, GRAB_FLOOR если `|dy| < 0x50` (80 ед.)
6. NULL если PAP_FLAG_HIDDEN или слишком далеко от terrain

Отрицательный face index = roof face (`roof_faces4[]`)

**Логика plant_feet() — детальная:**
- `fx=0; fz=0;` ПЕРЕД сложением с WorldPos — X/Z смещение анимированной ноги игнорируется!
- Это для согласованности с `predict_collision_with_face()` (одинаковый X/Z)
- Тестируемая точка: `(WorldPos.X>>8, foot_anim_Y + WorldPos.Y>>8, WorldPos.Z>>8)`
- Вызывает `find_face_for_this_pos()` с flag=0 (стандартный, порог 160 единиц)

Возвращаемые значения plant_feet():
- GRAB_FLOOR → snap к terrain, `OnFace=0`, return **-1**
- Face найден, `new_y < fy-60` → **падение** (set_person_drop_down), return **0**
- Face найден → snap к face, `OnFace=face`, return **1**
- NULL → PAP terrain, `new_y < fy-30` → **падение**, return **0**
- NULL → PAP terrain, близко → snap к terrain, return **-1**

---

## 5. Движение и коллизии

### move_thing() — основное движение
```c
ULONG move_thing(SLONG dx, SLONG dy, SLONG dz, Thing *p_thing)
```
Только для CLASS_PERSON. ASSERT: |dx/dz| < 2 mapsquares.

**Реальный порядок операций:**
1. Граница карты: выход за 128×128 → немедленный return 0
2. `collide_against_things()` — Things в сфере (radius + 0x1a0)
   - На PRIM/METAL поверхности: только CLASS_PERSON (без машин/мебели)
   - Мёртвые пропускаются, кроме машин (dead car всё ещё блокирует)
   - SUB_STATE_STEP_FORWARD: полная остановка (x2=x1, z2=z1)
   - Если thing умер в процессе → return 0
3. `collide_against_objects()` — OB (уличные объекты: фонари, урны, скамейки)
   - Поиск по PAP_Lo lo-res ячейкам в радиусе ±0x180 (lo-res единиц)
   - Коллизия: `PRIM_COLLIDE_BOX` / `PRIM_COLLIDE_SMALLBOX` → `slide_along_prim()`
   - Игнорирует prim, на котором стоит персонаж (OnFace с FACE_FLAG_PRIM)
   - **Специальная логика:** игрок идёт назад (SUB_STATE_WALKING_BACKWARDS) к скамейке (prim IDs 89,95,101,102,105,110,126) под углом < 220 → `set_person_sit_down()`
4. `slide_along()` — DFacet зданий
   - `SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT = -0x50` (-80 ед.)
   - STATE_HUG_WALL: radius >>= 2 (для плотного прилегания к стене)
5. `slide_along_edges()` / `slide_along_redges()` — кромки DFacet
   - Только: STATE_CIRCLING, SUB_STATE_STEP_FORWARD, walking player, FIRE_ESCAPE
6. `find_face_for_this_pos()` — новый face после движения
   - Не plant_feet()! plant_feet() вызывается отдельно из person-тика
   - Если y_diff > 0x50 (80 ед.) → FALL_OFF_FLAG_TRUE
7. **Fall-through bug workaround:** `x2 += dx>>2; y2 += dy>>2; z2 += dz>>2`
   (при падении добавляется 1/4 вектора движения)
8. Побочные эффекты: `DIRT_gust()`, `MIST_gust()`, `BARREL_hit_with_sphere()`
9. `move_thing_on_map()` — обновление MapWho

### move_thing_quick() — быстрое движение
```c
ULONG move_thing_quick(SLONG dx, SLONG dy, SLONG dz, Thing *p_thing)
{
    new_position = p_thing->WorldPos + delta;
    move_thing_on_map(p_thing, &new_position);
    return 0;
}
```
- **Без коллизий**, без скольжения
- Телепортация с обновлением карты
- Используется для ragdoll, knockback, катсцен

### slide_along() — алгоритм скольжения
```c
SLONG slide_along(
    SLONG x1, SLONG y1, SLONG z1,     // Начальная позиция
    SLONG *x2, SLONG *y2, SLONG *z2,  // Целевая позиция (изменяется in-place!)
    SLONG extra_wall_height,           // Корректировка высоты стены
    SLONG radius,                      // Радиус коллизии
    ULONG flags)                       // CRAWL, CARRYING, JUMPING
```

**Глобальные переменные (sic!):**
```c
SLONG actual_sliding = FALSE;   // Было ли столкновение?
SLONG last_slide_colvect;       // Последняя задетая стена
SLONG fence_colvect = 0;        // Коллизия с забором
```

**Детальный алгоритм slide_along():**
1. **NOGO squares:** `NOGO_COLLIDE_WIDTH = 0x5800` (~22 ед.) — буфер от NOGO-ячеек
   - SLIDE_ALONG_FLAG_CARRYING: также блокирует движение на ячейки с другой высотой
2. SUPERMAP_counter — каждый facet обрабатывается только раз за вызов
3. Итерация по lo-res ячейкам в bounding box движения
4. **Фильтрация facets по Y:**
   - Обычные стены: `y_bot = df->Y[0] - 64; y_top = df->Y[0] + (height*blockH<<2)`
   - `y_top += extra_wall_height` → с -0x50: стена "ниже" на 80 ед. (перешагивание!)
   - Заборы: `get_fence_top/bottom()` с отдельной логикой
   - STOREY_TYPE_CABLE: игнорируется (провода не блокируют)
   - STOREY_TYPE_OUTSIDE_DOOR + FACET_FLAG_OPEN: игнорируется (открытая дверь)
5. **Геометрия коллизии** (ВСЕ DFacets оси-aligned — либо X, либо Z):
   - Вдоль X (fz1==fz2): пуш по Z; endpoints — circle push
   - Вдоль Z (fx1==fx2): пуш по X; endpoints — circle push
   - Circle formula: `*pos = endpoint + dp * (radius>>8) / (dist>>8 + 1)`
6. Двери: проход через (return FALSE), устанавливает `slide_door` или `slide_into_warehouse`
7. Лестницы: устанавливает `slide_ladder`

**Глобальные выходы slide_along():**
```c
actual_sliding     = TRUE   // было столкновение
last_slide_colvect = facet  // последний facet (или fence_colvect для заборов)
slide_door         = storey  // индекс двери если прошли через неё
slide_ladder       = facet   // если лестница
slide_into_warehouse = bld   // если вошли в склад
```

**ВАЖНО:** Нет отскока (restitution=0). Нет трения. Все DFacets оси-aligned (нет диагональных стен).

**Другие функции скольжения:**
- `slide_around_box()` — box vs линейный отрезок (транспорт)
- `slide_around_sausage()` — capsule vs отрезок
- `slide_around_circle()` — circle vs circle (Person-Person)

---

## 5b. Лестницы (mount_ladder)

**Ключевые функции** (`collide.cpp`):
```c
SLONG correct_pos_for_ladder(DFacet*, px, pz, angle, scale)
// Вычисляет центр лестницы: midpoint двух точек facet.
// angle = Arctan(-dx,dz)+1024+512 (персонаж стоит лицом к лестнице)
// scale: +256 = у низа, -64 = у верха (Roper: +64 чтобы спускаться правильно)

SLONG ok_to_mount_ladder(Thing*, DFacet*)
// Проверяет QDIST2(dx,dz) < 75 до центра лестницы.
// 75 ≈ 8–9 юнитов (блок-уровень), персонаж должен быть рядом.

SLONG mount_ladder(Thing*, facet_index)
// ok_to_mount_ladder → set_person_climb_ladder()
```

**set_person_climb_ladder()** (`Person.cpp`):
- STATE → STATE_CLIMB_LADDER
- Анимация: ROPER/COP/THUG → `COP_ROPER_ANIM_LADDER_START`; остальные → `ANIM_MOUNT_LADDER`
- Velocity=0, Action=ACTION_CLIMBING, SubState=SUB_STATE_MOUNT_LADDER
- OnFacet = storey (DFacet index); sets NON_INT_M|NON_INT_C флаги; plays jump sound
- Вызывает `set_person_position_for_ladder()` — snap к центру лестницы

**Важно для пре-релиза:** вызов `mount_ladder()` из interfac.cpp (игрок нажимает ACTION) закомментирован! Игрок может:
- ✅ **Слезть на лестницу сверху** — `set_person_climb_down_onto_ladder()` (активно, строка ~1507)
- ❌ **Запрыгнуть снизу** — закомментировано (`/* mount_ladder(p_thing, ladder_col) */`)
AI (pcom.cpp:12549) — может запрыгивать снизу через прямой `set_person_climb_ladder()`.
Финальный релиз, вероятно, восстанавливает закомментированный путь.

---

## 5c. Вода (PAP_FLAG_WATER)

Вода в Urban Chaos — **просто невидимые коллизионные стены** по периметру.

**Флаги и поля:**
- `PAP_FLAG_WATER` — в `PAP_Hi.Flags` для водяных hi-res ячеек
- `PAP_FLAG_SINK_POINT` — устанавливается на водяной ячейке и трёх соседях (+1,0 / 0,+1 / +1,+1); используется системой луж/дождя для рендеринга поверхности воды и az.cpp
- `MAV_SPARE_FLAG_WATER` — в MAV_nav SPARE bits [15:14] для навигации AI
- `PAP_Lo.water` (SBYTE) — высота воды в Lo-res единицах; `PAP_LO_NO_WATER = -127` = нет воды

**Инициализация при загрузке уровня** (elev.cpp ~2044-2083):
1. Все `PAP_Lo[x][z].water = PAP_LO_NO_WATER (-127)`
2. Для каждой `PAP_Hi` с `PAP_FLAG_WATER`: `PAP_2LO(x>>2, z>>2).water = water_level >> 3`
   - `water_level = -0x80 = -128` — **хардкодированная** локальная переменная в `EL_load_map()`
   - Итоговое значение: `-128 >> 3 = -16` (в Lo-res ед.)
3. Объекты (OB) в водяных ячейках переставляются на `water_level`: `oi->y = water_level - prim->miny = -128 - miny`

**Коллизия — активный код** (`stop_movement_between()`, collide.cpp ~10027):
- Если одна ячейка `PAP_FLAG_WATER`, а соседняя — нет → `return TRUE` → создаётся invisible DFacet (стена) вдоль ребра
- Две водяные ячейки рядом → `normal = FALSE` (нет facet между водой и водой)
- Персонажи/транспорт не могут войти в воду — блокируются невидимой стеной

**`PAP_Lo.water` в рантайме — МЁРТВЫЙ КОД:**
- `OB_height_fiddle_de_dee()` в ob.cpp читал `PAP_2LO.water` для подъёма объектов до уровня воды
- Вся функция обёрнута в `/* */` — никогда не выполняется
- Вывод: высота воды хранится, но в реальной игре НЕ используется для физики

**Нет:**
- Замедления в воде, плавания, утопания
- Динамического изменения уровня воды

**Есть:**
- **Огонь гасится в воде** (Person.cpp:4414): если `PYRO_IMMOLATE` и персонаж в `PAP_FLAG_WATER` tile → dummy/radius сбрасываются

---

## 6. Гравитация

### Для персонажей — гравитация НЕ физическая, она animation-driven

**Не существует** явного вектора гравитации применяемого к персонажам!

**Как это работает:**
1. `plant_feet()` определяет высоту под персонажем
2. При разнице > 60 юнитов → переход в `STATE_FALLING`
3. В состоянии падения: анимационный handler уменьшает DY каждый фрейм (~-20/фрейм)
4. **DY** хранится в `Thing` как `SWORD` (16-bit signed)
5. При приземлении: детектируется через `height_above_anything()`

**Важно:** нельзя просто добавить гравитацию как вектор — нужно воспроизвести state-machine логику падения точно.

### Для транспорта — явная гравитация

```c
// vehicle.cpp, line 104
#define GRAVITY (-(UNITS_PER_METER * 10 * 256) / (TICKS_PER_SECOND * TICKS_PER_SECOND))
// = -(128 * 10 * 256) / (80 * 80) = -5120 юнит/тик²
// TICKS_PER_SECOND = 80 (20 * TICK_LOOP)
```

При нахождении в воздухе: `VelY += GRAVITY` каждый тик.
При контакте с землёй: VelY демпируется через систему подвески.

---

## 6b. Тайминг и физические константы

```c
// Game.h
#define NORMAL_TICK_TOCK  (1000/15)  // 66.67 мс — базовый тик (15 FPS логики)
#define TICK_SHIFT        8
// TICK_RATIO — динамический! Вычисляется каждый кадр (Thing.cpp — process_things_tick):
// TICK_RATIO = (реальное_мс_кадра << 8) / 66.67
// При норме ≈ 256 (1.0), при медленном кадре ≤ 512 (2.0x), при быстром ≥ 128 (0.5x)
// Сглаживание: 4-кадровая скользящая средняя (SmoothTicks)

// Slow-motion режим: TICK_RATIO = 32 (~12.5% скорости)
```

```c
// Vehicle.cpp
#define TICK_LOOP        4            // под-итераций подвески за кадр
#define TICKS_PER_SECOND (20*TICK_LOOP)  // = 80 — только для формул констант физики
// НЕ является частотой кадров. Игра работает на 30 FPS рендеринга, 15 FPS логики.

// Подвеска: count = TICK_LOOP; while (--count) {...}
// → цикл выполняется 3 раза за кадр

// Константы откалиброваны под TICKS_PER_SECOND = 80:
#define GRAVITY (-(128 * 10 * 256) / (80*80))   // = -5120 юнит/тик²
```

**Frame rate cap** (Game.cpp, `lock_frame_rate()`):
```c
// Spin-loop busy-wait:
// while(GetTickCount() - tick1 < 1000/fps) {}
// fps из config.ini: max_frame_rate=30 (default)
```

**Важно для портирования:** движение и физика умножаются на `TICK_RATIO >> TICK_SHIFT` —
это даёт frame-rate independence. При переписывании нужно воспроизвести эту схему точно,
включая 4-кадровое сглаживание и clamp 0.5x–2.0x.

---

## 7. Физика транспорта

Подробности: **`vehicles.md`** — подвеска (4 пружины), двигатели, ограничения скорости.
Гравитация транспорта: `GRAVITY = -5120 юн/тик²` — см. раздел 6 выше.

---

## 8. Лимиты массивов (PC vs PSX)

| Массив | PC | PSX |
|--------|----|-----|
| MAX_COL_VECT_LINK | 10 000 | 4 000 |
| MAX_COL_VECT | 10 000 | 1 000 |
| MAX_WALK_POOL | 30 000 | 10 000 |
| WMOVE_MAX_FACES (=RWMOVE_MAX_FACES) | 192 | 192 |

---

## 9. HyperMatter — spring-lattice физика мебели

**Файл:** `fallen/Source/hm.cpp` (~2000 строк, детально аннотирован)

**Ключевые факты:**
- Собственная разработка MuckyFoot (НЕ Criterion middleware)
- Только для мебели (бочки, ящики, стулья) — не для персонажей и транспорта
- Spring-lattice: 3D решётка масс-точек + пружины, Euler integration, 13 пружин/точку
- `HM_process()`: spring forces → bump forces → position update → cleanup
- **`gy = 0` hardcoded!** Объекты отскакивают от Y=0, НЕ от реального terrain
- Баги: `already_bumped[i]` вместо `[j]` (некоторые HM-HM столкновения пропускаются)
- `HM_stationary(threshold=0.25)` → "усыпляет" объект когда почти неподвижен
- `FURN_hypermatterise()` очищает `FLAG_FURN_DRIVING` при активации HM
- Porting: реализовать заново через **Jolt Physics** или **Bullet Physics**

---

## 9b. WMOVE — движущиеся ходимые поверхности

**Файл:** `fallen/Source/wmove.cpp`, `fallen/Headers/wmove.h`

Система позволяет персонажам стоять на крышах движущихся машин и платформ.

**Принцип:** для каждого транспорта/платформы создаётся виртуальная walkable face (PrimFace4 с FACE_FLAG_WMOVE), которая движется вместе с объектом.

**Лимит:** `WMOVE_MAX_FACES = 192` (PSX: то же)

**Количество faces per object** (`WMOVE_get_num_faces()`):
- CLASS_PERSON, CLASS_PLAT: 1
- VAN/AMBULANCE/WILDCATVAN: 1
- JEEP/MEATWAGON: 4
- CAR/TAXI/POLICE/SEDAN: 5

**PSX ограничение:** на PSX только фургоны (VAN/WILDCATVAN/AMBULANCE) создают WMOVE faces — остальные машины пропускаются (слишком медленно для PSX).

**Жизненный цикл:**
```
WMOVE_create(thing)   // при создании vehicle/platform (eway.cpp, Controls.cpp, plat.cpp)
WMOVE_process()       // в Thing.cpp каждый кадр: update positions + re-attach to map
WMOVE_remove(class)   // при очистке уровня
```

**WMOVE_process()** каждый кадр:
1. Для каждой WMOVE face: запомнить старые coords, снять с карты
2. Пересчитать позицию по текущему положению owner-thing (через матрицу поворота)
3. Обновить prim_points[], заново приаттачить к карте
4. Если машина STATE_DEAD и не на карте → очистить face

**WMOVE_relative_pos()** — как персонажи следуют за surface:
- Вызывается из `general_process_person()` когда person стоит на WMOVE face
- Вычисляет позицию (last_x, last_z) в базисе **прошлого** положения face (два вектора da, db)
- Переводит те же координаты в **новый** базис → new_x, new_y, new_z
- Также вычисляет `dangle` = поворот face за кадр → добавляется к углу персонажа
- Threshold: `|dy| < 0x200` → dy=0 (не скользить вверх/вниз по surface)

**Портирование:** WMOVE нужно реализовать — это важная геймплейная фича (стоять на крыше машины). Можно реализовать как динамические rigid bodies с walkable surfaces (Jolt Physics).

---

## 9c. Известные баги и хаки в коде

1. **HM_collide_all bug** (hm.cpp): `already_bumped[i]` вместо `[j]` во внутреннем цикле — некоторые HM-HM столкновения пропускаются. Из-за редкости ситуации и быстрого "засыпания" объектов практически не заметно.

2. **Координатный хак** (collide.cpp, line ~2753):
   Комментарий: *"shift is here to mimic code in predict_collision_with_face()"*
   — ручная подгонка координат для совпадения двух функций.

2. **Fall-through-roof** (move_thing, line ~7061):
   Активный workaround: *"Get rid of that nasty fall-through-the-roof bug"*
   — персонаж мог падать сквозь крышу.

3. **Несколько методов интерполяции высоты** — разные функции для одной задачи,
   указывает на edge-cases в terrain system.

4. **DeltaVelocity** в Thing.h (line 112-115) — поле существует, использование неясно.
   Возможно незавершённая физическая система.

5. **SWORD для скорости** — 16-bit ограничение (-32K..+32K). При высоких скоростях возможно clipping.

---

## 10. Углы и вращение

- Угловые значения: 0–2047 (11-bit, 2048 = 360°)
- Нет float-углов в физике

