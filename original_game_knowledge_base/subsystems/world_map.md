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

**Флаги PAP_Hi:**
```c
PAP_FLAG_SHADOW_1/2/3    // тени
PAP_FLAG_REFLECTIVE      // отражающая поверхность
PAP_FLAG_HIDDEN          // под крышей или внутри здания
PAP_FLAG_NOGO            // запретная зона (AI не ходит)
PAP_FLAG_ANIM_TMAP       // анимированная текстура
PAP_FLAG_ROOF_EXISTS     // есть крыша
PAP_FLAG_ZONE1/2/3/4     // 4 зоны для AI
PAP_FLAG_WANDER          // зона патруля
PAP_FLAG_FLAT_ROOF       // плоская крыша
PAP_FLAG_WATER           // есть вода
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

## 9. Что переносить в новую версию

| Аспект | Подход |
|--------|--------|
| PAP_Hi (128×128) | Перенести 1:1 — критично для физики и навигации |
| PAP_Lo (32×32) / MapWho | Перенести 1:1 — критично для производительности |
| Высоты и билинейная интерполяция | Перенести 1:1 |
| Здания (Building/Storey/Wall) | Перенести 1:1 — сложная система |
| Формат уровней (.lev) | Написать загрузчик совместимый с оригиналом |
| Освещение мира | Можно улучшить (динамические тени), базовая логика 1:1 |
| MapElement (старая система) | Только LIGHT_Colour — остальное выбросить |
