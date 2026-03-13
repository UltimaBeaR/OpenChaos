# Игровой мир и карта — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/Map.cpp`, `fallen/Headers/Map.h` — PAP система, MapWho
- `fallen/Source/Level.cpp`, `fallen/Headers/Level.h` — загрузка уровней
- `fallen/Headers/Building.h` — FBuilding, FStorey, FWall
- `fallen/Source/Light.cpp`, `fallen/Headers/Light.h` — освещение

---

## 1. Двухуровневая карта (PAP система)

Карта представлена **двумя сетками разного разрешения:**

### PAP_Hi — 128×128 (высокое разрешение)

Каждая ячейка = 1 mapsquare = **256 единиц мира** (`PAP_SHIFT_HI = 8`).

```c
struct PAP_Hi {           // 6 байт
    UWORD Texture;        // индекс текстуры (3 свободных бита для флагов)
    UWORD Flags;          // флаги поверхности (15 бит)
    SBYTE Alt;            // высота (реальная = Alt << 3)
    SBYTE Height;         // padding
};
```

**Флаги PAP_Hi (pap.h):**
```c
PAP_FLAG_SHADOW_1   (1<<0)   // тень 1
PAP_FLAG_SHADOW_2   (1<<1)   // тень 2
PAP_FLAG_SHADOW_3   (1<<2)   // тень 3
PAP_FLAG_REFLECTIVE (1<<3)   // отражающая поверхность
PAP_FLAG_HIDDEN     (1<<4)   // под крышей или внутри здания
PAP_FLAG_SINK_SQUARE (1<<5)  // вся ячейка тонет (вода/трясина)
PAP_FLAG_SINK_POINT  (1<<6)  // отдельная точка тонет
PAP_FLAG_NOUPPER     (1<<7)  // нет верхней части (рендер)
PAP_FLAG_NOGO        (1<<8)  // запретная зона (AI не ходит)
// ВНИМАНИЕ: биты 9 и 14 РАЗДЕЛЯЮТСЯ между флагами (контекстно-зависимые):
PAP_FLAG_ANIM_TMAP  (1<<9)  // анимированная текстура — ИЛИ —
PAP_FLAG_ROOF_EXISTS (1<<9) // есть крыша (в контексте крышной ячейки)
PAP_FLAG_ZONE1      (1<<10) // AI зона 1
PAP_FLAG_ZONE2      (1<<11) // AI зона 2
PAP_FLAG_ZONE3      (1<<12) // AI зона 3
PAP_FLAG_ZONE4      (1<<13) // AI зона 4
PAP_FLAG_WANDER     (1<<14) // зона патруля AI — ИЛИ —
PAP_FLAG_FLAT_ROOF  (1<<14) // плоская крыша (в контексте крышной ячейки)
PAP_FLAG_WATER      (1<<15) // вода
```

**Высота:** `Alt << PAP_ALT_SHIFT` (т.е. `Alt << 3`). Диапазон Alt: 0–127, реальная высота: 0–1016.

### PAP_Lo — 32×32 (низкое разрешение)

Каждая ячейка = 4×4 mapsquares = **1024 единицы мира** (`PAP_SHIFT_LO = 10`).

```c
struct PAP_Lo {           // 8 байт
    UWORD MapWho;         // индекс первого Thing на этой ячейке
    SWORD Walkable;       // индекс face для ходьбы (или крыши если <0)
    UWORD ColVectHead;    // голова вектора коллизии
    SBYTE water;          // высота воды
    UBYTE Flag;           // PAP_LO_FLAG_WAREHOUSE — внутри склада
};
```

### Доступ:
```c
#define PAP_SHIFT_LO  10
#define PAP_SHIFT_HI  8
#define PAP_SIZE_LO   32
#define PAP_SIZE_HI   128
#define PAP_ALT_SHIFT 3

PAP_2LO(x, z)  // PAP_Lo[x][z]
PAP_2HI(x, z)  // PAP_Hi[x][z]
ON_PAP_LO(x,z) // проверка границ для PAP_Lo
ON_PAP_HI(x,z) // проверка границ для PAP_Hi
```

### Устаревший MapElement (128×128, 14 байт):
Старая система — осталась в коде, частично используется для освещения:
```c
struct MapElement {
    LIGHT_Colour Colour;         // RGB освещение ячейки
    SBYTE AltNotUsedAnyMore;
    UWORD FlagsNotUsedAnyMore;
    UWORD ColVectHead;
    SWORD Walkable;
    THING_INDEX MapWho;
};
```

---

## 2. MapWho — пространственный хэш

**Назначение:** быстро найти все Thing на определённой ячейке карты.

**Реализация:** linked list в PAP_Lo:
- `PAP_Lo[x][z].MapWho` → индекс первого Thing
- `Thing->Child` → следующий Thing на той же ячейке
- `Thing->Parent` → предыдущий Thing на той же ячейке
- `Thing->Flags & FLAGS_ON_MAPWHO` — флаг присутствия на карте

**Вычисление ячейки для Thing:**
```c
mx = thing->WorldPos.X >> (8 + PAP_SHIFT_LO);  // >> 18
mz = thing->WorldPos.Z >> (8 + PAP_SHIFT_LO);  // >> 18
```

**API:**
```c
add_thing_to_map(Thing *t)           // вставить в начало списка
remove_thing_from_map(Thing *t)      // удалить из списка
move_thing_on_map(Thing *t, GameCoord *new_pos)  // перенести в другую ячейку
```

**Важно:** `move_thing_on_map()` вызывается только при смене ячейки PAP_Lo (каждые 1024 ед.), не каждый кадр.

---

## 3. Координатная система

```
Карта: 128×128 mapsquares
- X: запад ↔ восток
- Y: вниз ↔ вверх
- Z: север ↔ юг

Размер в мировых единицах: 128 × 256 = 32768 единиц по X и Z

1 mapsquare = 256 единиц мира (1 << 8)
1 PAP_Lo ячейка = 1024 единицы мира (1 << 10)
```

**Преобразования:**
```c
// Мировые координаты → mapsquare (PAP_Hi)
mx = world_x >> 8;
mz = world_z >> 8;

// Мировые координаты → PAP_Lo ячейка
mx_lo = world_x >> 18;   // >> (8 + 10)
mz_lo = world_z >> 18;
```

---

## 4. Интерполяция высоты

PAP_Hi хранит высоту в вершинах, между ними — **билинейная интерполяция:**

```c
SLONG PAP_calc_height_at(SLONG x, SLONG z) {
    SLONG mx = x >> 8, mz = z >> 8;
    SLONG xfrac = x & 0xff, zfrac = z & 0xff;

    h0 = PAP_2HI(mx,   mz  ).Alt;
    h1 = PAP_2HI(mx+1, mz  ).Alt;
    h2 = PAP_2HI(mx,   mz+1).Alt;
    h3 = PAP_2HI(mx+1, mz+1).Alt;

    // Диагональная интерполяция (треугольники)
    if (xfrac + zfrac < 0x100)
        answer = h0 + (h2-h0)*xfrac + (h1-h0)*zfrac;
    else
        answer = h3 + (h1-h3)*(256-xfrac) + (h2-h3)*(256-zfrac);

    return answer << PAP_ALT_SHIFT;  // << 3
}
```

---

## 5. Загрузка уровней

**Файлы уровней:** `Levels/Level%3.3d.lev` (Level001.lev, Level002.lev, ...)

**Формат (бинарный, версионированный):**

| Версия | Содержимое |
|--------|-----------|
| 0 | Things |
| 1 | Things + Waypoints |
| 2 | Things + Waypoints + ConditionLists |
| 3 | Things + Waypoints + ConditionLists + CommandLists |

**Структуры в файле:**
```c
struct ThingDef {
    UBYTE Version;
    SLONG Class;         // CLASS_*
    SLONG Genus;         // подтип
    SLONG X, Y, Z;       // позиция
    UWORD CommandRef;
    SLONG Data[10];
    UWORD EdThingRef;
};

struct WaypointDef {
    UBYTE Version;
    UWORD Next, Prev;    // связный список
    SLONG X, Y, Z;
    UWORD EdWaypointRef;
};

struct ConditionDef {
    UBYTE Version;
    UWORD Flags, ConditionType, GroupRef;
    SLONG Data1, Data2, Data3;
};

struct CommandDef {
    UBYTE Version;
    UWORD Flags, CommandType, GroupRef;
    SLONG Data1, Data2, Data3;
};
```

---

## 6. Здания

**Иерархия:** Building → Storey (этаж) → Wall (стена)

### FBuilding (44 байта):
```c
struct FBuilding {
    UWORD ThingIndex;
    UWORD LastDrawn;
    UBYTE Foundation;      // тип фундамента
    SWORD OffsetY;
    UWORD InsideSeed;      // seed для генерации интерьера
    UBYTE BuildingFlags;
    SWORD StoreyHead;      // голова списка этажей
    SWORD Angle;           // ориентация
    UWORD StoreyCount;
    CBYTE str[20];         // имя здания
    UBYTE StairSeed;
    UBYTE BuildingType;    // HOUSE, WAREHOUSE, OFFICE, APARTMENT...
    UWORD Walkable;
};
```

### FStorey (48 байт) — этаж:
```c
struct FStorey {
    SWORD DX, DY, DZ;
    UBYTE StoreyType;      // NORMAL, ROOF, WALL, FIRE_ESCAPE, INSIDE...
    UBYTE StoreyFlags;
    UWORD Height;
    SWORD WallHead;        // голова списка стен
    SWORD Next, Prev;
    SWORD Inside;
};
```

**Типы StoreyType:**
```c
STOREY_TYPE_NORMAL      = 1
STOREY_TYPE_ROOF        = 2
STOREY_TYPE_WALL        = 3
STOREY_TYPE_FIRE_ESCAPE = 6
STOREY_TYPE_INSIDE      = 17
```

### FWall (38 байт) — стена:
```c
struct FWall {
    SWORD DX, DZ;
    UWORD WallFlags;       // USED, AUTO_WINDOWS...
    UWORD TextureStyle;
    SWORD TextureStyle2;
    UWORD DY;              // высота стены
    SWORD Next;
    UBYTE *Textures;
    UWORD Tcount;
};
```

---

## 7. Освещение мира

**LIGHT_Colour:** RGB (3 байта), максимум = 64 на канал.

**Типы источников света:**
```c
LIGHT_TYPE_NORMAL  = 1   // постоянный
LIGHT_TYPE_BROKEN  = 2   // мигающий (param = частота)
LIGHT_TYPE_PULSE   = 3   // импульсный (param = скорость)
LIGHT_TYPE_FADE    = 4   // затухающий (param = длительность)
```

**LIGHT_Map** — интерфейс для получения/установки освещения:
```c
struct LIGHT_Map {
    SLONG width, height;
    SLONG (*get_height)(SLONG x, SLONG z);
    LIGHT_Colour (*get_light)(SLONG x, SLONG z);
    void (*set_light)(SLONG x, SLONG z, LIGHT_Colour);
};
```

Освещение хранится в `MapElement.Colour` (старая система) и применяется к вершинам при рендеринге.

**NIGHT система** (основная, детально описана в [rendering.md](rendering.md)):
- Статические источники (max 256), динамические (max 64)
- Кэш освещения: до 256 lo-res квадратов, до 15000 walkable вершин
- Данные загружаются из `.lgt` файлов (`data/Lighting/`)
- 6-bit каналы (0-63) → ×4 → 0-255 для D3D
- Статические тени запекаются в PAP_FLAG_SHADOW_1/2/3 при загрузке

---

## 8. Лимиты

| Ресурс | Лимит (PC) |
|--------|-----------|
| Карта | 128×128 mapsquares |
| PAP_Hi | 16384 ячеек |
| PAP_Lo | 1024 ячеек |
| Things (primary) | 400 |
| Things (secondary) | 300 |
| Things (всего) | 700 |
| Здания | 500 |
| Этажей | 2500 (5 на здание) |
| Стен | 15000 (6 на этаж) |
| Внутренних этажей | 1024 |
| Building facets | 4000 |
| Building objects | 400 |
| Дальность света | 1536 мировых ед. |
| NIGHT статических источников | 256 |
| NIGHT динамических источников | 64 |
| NIGHT кэш lo-res квадратов | 256 |
| NIGHT walkable вершин | 15000 |

---

## 9. Постпроцессинг уровня — build_quick_city / ROAD / WAND

Вызывается при загрузке уровня из `elev.cpp` (после расстановки объектов):
```c
build_quick_city();    // шаг 1: строит коллизию и ходимые поверхности зданий
ROAD_wander_calc();    // шаг 2: строит граф дорог для водительского AI
WAND_init();           // шаг 3: размечает PAP_FLAG_WANDER для пешеходного AI
```

### build_quick_city() (build2.cpp)

Регистрирует DFacet (грани зданий) и roof_faces4 в системе PAP_Lo для коллизии и навигации.

**Алгоритм:**
1. `clear_colvects()` — сбрасывает `PAP_Lo[x][z].ColVectHead` и `.Walkable` для всех 32×32 ячеек
2. `clear_facet_links()` — обнуляет весь массив `facet_links[]`
3. `mark_naughty_facets()` — помечает `FACET_FLAG_INVISIBLE` дубликаты и полностью перекрытые DFacet (грани зданий с одинаковыми вершинами или скрытые за другими solid гранями)
4. `process_building(c0)` для каждого здания `1..next_dbuilding`:
   - Итерирует DFacet[StartFacet..EndFacet]; пропускает FACET_FLAG_INVISIBLE (кроме BUILDING_TYPE_CRATE_IN)
   - Вызывает `process_facet(c0)` → добавляет DFacet в `PAP_Lo[x][z].ColVectHead` linked list
5. `garbage_collection()` — уплотняет facet_links[], убирая дыры
6. Для каждого dwalkable (индексы крышных граней `1..next_dwalkable`):
   - `attach_walkable_to_map(-face)` → регистрирует `roof_faces4[face]` в `PAP_Lo[RX>>2][RZ>>2].Walkable` linked list

**Важно:** Положительные face индексы (prim_faces4, регулярные ходимые) **закомментированы** — только отрицательные (roof_faces4) реально регистрируются.

### ROAD_wander_calc() (road.cpp) — граф дорог для машин

**Данные:**
```c
struct ROAD_Node {   // 8 байт
    UBYTE x, z;      // позиция в mapsquare (PAP_Hi координаты)
    UBYTE c[4];      // до 4 соединений (индексы других нодов)
};
#define ROAD_MAX_NODES 256   // макс нодов
#define ROAD_MAX_EDGES 8     // макс граничных нодов (въезды/выезды с карты)
#define ROAD_LANE 0x100      // смещение от центра дороги для езды по полосе
```

**Алгоритм:**
1. Очистка всех ROAD_node[] и ROAD_edge[]
2. Сканирование Z-параллельных дорог: для каждого x [2..125] ищет runs ROAD_is_middle(x,z), если длина > 2 → `ROAD_add(x, p1, x, p2)`
3. Сканирование X-параллельных дорог: то же самое по горизонтали
4. `ROAD_add(x1,z1, x2,z2)`:
   - Если пересекается с существующей дорогой → рекурсивный split на точке пересечения
   - Иначе: `ROAD_connect(n1,n2)` — двунаправленная связь нодов
5. Граничные ноды (degree=1, x или z == 2 или 125) → смещаются на край карты (x=0/127) и добавляются в ROAD_edge[]

**`ROAD_is_middle(x,z)`** = TRUE если все 5×5 квадратов вокруг (x,z) являются дорогой (надёжно внутри, не на краю).

**`ROAD_is_road(x,z)`** = проверяет PAP_Hi.Texture & 0x3ff:
- PC: range 323-356 (дорожные текстуры); TextureSet 7/8: также 35, 36, 39
- PSX: range 256..278

**Специальный хардкод:** нод (121,33) на карте `gpost3.iam` (миссии Cop Killers / Arms Breaker) — пропускается.

**ROAD_LANE = 0x100** — AI едет со смещением 256 единиц от центра (левая сторона).

### WAND_init() (wand.cpp) — зоны блуждания пешеходов

Размечает `PAP_FLAG_WANDER` в PAP_Hi для каждого mapsquare.

**Алгоритм:**
1. Сбрасывает PAP_FLAG_WANDER для всего PAP_Hi 128×128
2. Для каждой не-скрытой, не-дорожной ячейки: проверяет 5×5 окрестность — если хоть одна ячейка является дорогой → ставит PAP_FLAG_WANDER (тротуар рядом с дорогой)
3. Зебра-переходы (ROAD_is_zebra = текстура 333 или 334): также получают PAP_FLAG_WANDER
4. Для каждого OB_Info (collision prim) в PAP_Lo:
   - Если PRIM_COLLIDE_BOX / SMALLBOX / CYLINDER И y ≤ terrain+0x40 → снимает PAP_FLAG_WANDER с центральной ячейки (не давать NPC бродить внутри припаркованных машин, у скамеек)

**`WAND_get_next_place()`** — выбор следующей точки для PCOM WANDER AI:
- 4 направления × WAND_MAX_MOVE=3 клетки ± 1 рандомный сдвиг = 4 кандидата
- Оценка = dot product с текущим направлением (движение прямо предпочтительнее)
- Если ни одна не подходит → 16 случайных проб в радиусе ±15 клеток, выбрать ближайшую
- CLASS_BAT (Bane) = может ходить и по дорогам (WANDER ИЛИ ROAD)
- NPC с pcom_zone != 0: используют PCOM_get_zone_for_position вместо WANDER флага

