# Физика и коллизии — Urban Chaos

**Связанный файл:** [physics_details.md](physics_details.md) — вода, гравитация, тайминг, HyperMatter, WMOVE, баги

**Ключевые файлы кода:**
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

**Секции 5c-10** (вода, гравитация, тайминг, HyperMatter, WMOVE, баги, углы) → [physics_details.md](physics_details.md)
