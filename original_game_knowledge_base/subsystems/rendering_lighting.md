# Рендеринг — Освещение и Crinkle

Вынесено из `rendering.md` (разделено при превышении 400 строк).

---

## MapElement.Colour — где используется

`MAP[128][128]` — массив `MapElement`, каждый содержит поле `Colour` типа `LIGHT_Colour {r,g,b}`.

**В DDEngine (PC DirectX6):** `MapElement.Colour` = **МЁРТВЫЙ КОД**.
- `MAP_light_set_light()` / `MAP_light_get_light()` определены в Map.cpp, но **нигде не вызываются** кроме самого Map.cpp
- `MAP_light_map` (LIGHT_Map struct с указателями на эти функции) — тоже нигде не используется
- Vertex colours для terrain в DDEngine вычисляются **только через NIGHT систему** (см. ниже)

**В Glide Engine** (старый путь, не PC/DC): `me->Colour` применялся напрямую:
```c
// glaeng.cpp
pp->colour = LIGHT_get_glide_colour(me->Colour);  // terrain vertex colour
```

---

## Освещение — NIGHT система

**Vertex Lighting** — не per-pixel, освещение запекается в вершины (NIGHT система из `night.h`).

### Ёмкость NIGHT системы
```c
NIGHT_MAX_SLIGHTS   = 256    // статических источников света
NIGHT_MAX_DLIGHTS   = 64     // динамических источников света
NIGHT_MAX_SQUARES   = 256    // кэшированных lo-res квадратов освещения
NIGHT_MAX_WALKABLE  = 15000  // вершин walkable поверхностей с цветом
NIGHT_MAX_BRIGHT    = 64     // яркость канала (6-bit, 0-63 → ×4 → 0-255 для D3D)
NIGHT_MAX_FOUND     = 4      // максимум источников влияния на 1 точку
```

### Форматы данных
```c
struct NIGHT_Colour {
    UBYTE r, g, b;  // 6-bit каналы (0-63), конвертируются в D3D через ×4
};

// NIGHT_get_d3d_colour() — конвертирует NIGHT_Colour → D3D ARGB (0xFF, r×4, g×4, b×4)
```

### Полный pipeline — vertex colour для terrain (DDEngine)

**Шаг 1: Предвычисление при загрузке — `NIGHT_cache_create(lo_x, lo_z)` (night.cpp:2009)**

Lo-map cell (1/32 карты) → аллоцирует `NIGHT_Square`:
```c
struct NIGHT_Square {
    NIGHT_Colour colour[PAP_BLOCKS * PAP_BLOCKS];  // 4×4 = 16 вершин
    UBYTE lo_map_x, lo_map_z;
    UBYTE next, flag;
};
NIGHT_cache[32][32] → индекс в NIGHT_square[]  // per lo-cell кэш
```

Вызывает `NIGHT_light_mapsquare(lo_x, lo_z, colour[16], floor_y=0, inside=0)`:

**Шаг 1а: Ambient освещение (для каждой из 16 вершин в 4×4 блоке):**
```c
// Нормаль из высот соседей
nx = (h[mx-1,mz] - h[mx+1,mz]) * 4, ny = 256 - QDIST2(|nx|,|nz|), nz аналогично
clamp: if |nx|>120 → clamp(±100); аналогично nz
// Ambient dot product
dprod = -dot(NIGHT_amb_norm, normal)    // отрицательный = facing away negated
dprod = clamp(dprod, 0..65536) >> 9 + 128  // → диапазон 128..255
dprod *= NIGHT_LIGHT_MULTIPLIER
colour[x+z*4].r = NIGHT_amb_red   * dprod >> 8  // NIGHT_Colour range 0..63
colour[x+z*4].g = NIGHT_amb_green * dprod >> 8
colour[x+z*4].b = NIGHT_amb_blue  * dprod >> 8
```

**Шаг 1б: Статические источники (`NIGHT_Slight`) из 3×3 соседних lo-cells:**
```c
// Для каждого источника внутри 3×3 neighbourhood:
lradius = nsl->radius << 2  // субпиксельные координаты
dist = QDIST3(|lx-px|, |ly-py|, |lz-pz|) + 1
if dist < lradius:
    dprod = (dx*nx + dy*ny + dz*nx) / dist  // ⚠️ БАГ: dz*nx вместо dz*nz!
    bright = (256 - dist*256/lradius) * dprod/256 * NIGHT_LIGHT_MULTIPLIER
    colour[i].r = SATURATE(colour[i].r + nsl->red   * bright >> 8, 0..255)
    // аналогично g, b
```

**Шаг 1в: Лампосты (`NIGHT_llight[]`)** — аналогично Slight, radius = NIGHT_lampost_radius << 2.

**Шаг 2: Per-frame рендеринг (aeng.cpp:7530)**

Для каждой видимой Hi-map вершины (x, z) в gamut:
```c
square = NIGHT_cache[x>>2][z>>2]   // lo-cell index
nq = &NIGHT_square[square]
dx = x & 3; dz = z & 3            // позиция внутри 4×4
NIGHT_get_d3d_colour(nq->colour[dx + dz*PAP_BLOCKS], &pp->colour, &pp->specular)
// затем:
apply_cloud(world_x, world_y, world_z, &pp->colour)  // облачные тени
POLY_fadeout_point(pp)                               // distance fade
```

**Indoor путь:** фиксированный `colour = 0x80808080` (закомментированный код с NIGHT_get_d3d_colour_dim существует, но не активен — `/* */`).

**`NIGHT_get_d3d_colour(NIGHT_Colour col, ULONG *colour, ULONG *specular)` (night.h:266):**
```c
r = col.red * 4; g = col.green * 4; b = col.blue * 4  // 0..63 → 0..252
if NIGHT_specular_enable:
    if r > 255: wred = (r-255)>>1; r = 255           // overflow → pseudo-specular
    *specular = 0xFF000000 | (wr<<16)|(wg<<8)|wb
else:
    clamp r,g,b to 0..255; *specular = 0xFF000000
*colour = 0xFF000000 | (r<<16) | (g<<8) | b
```

### Типы источников
- **`NIGHT_Slight`** — статические (загружаются из `.lgt` файлов через `NIGHT_load_ed_file()`)
- **`NIGHT_Dlight`** — динамические (создаются в рантайме, до 64 одновременно)

### Флаги освещения уровня
```c
NIGHT_FLAG_LIGHTS_UNDER_LAMPOSTS   (1<<0)  // фонари
NIGHT_FLAG_DARKEN_BUILDING_POINTS  (1<<1)  // затемнение стен зданий
NIGHT_FLAG_DAYTIME                 (1<<2)  // дневное освещение
```

### Specular эффект
```c
NIGHT_specular_enable  // на PC: oversaturation создаёт pseudo-specular highlights
                       // на DC: отключён
```

### Статические тени (shadow.cpp)
Запекаются **при загрузке уровня**, не в рантайме:
- Направление солнца: `SHADOW_DIR_X=+147, SHADOW_DIR_Y=-148, SHADOW_DIR_Z=-147`
- Метод: ray-cast через `there_is_a_los()` в сторону солнца
- Результат: биты `PAP_FLAG_SHADOW_1/2/3` в PAP_Hi.Flags для каждого хай-рез квадрата
- `shadow[8] = {0, 1, 7, 5, 3, 2, 4, 5}` — lookup таблица для 3 соседних квадратов

### Затухание по расстоянию (дистанс-фейд)
```c
POLY_FADEOUT_START = 0.60F  // z/view_dist — начало затухания
POLY_FADEOUT_END   = 0.95F  // полностью прозрачно
```

### NIGHT_generate_walkable_lighting()
Освещение вершин walkable поверхностей (полы зданий, крыши).
- Вызывает `NIGHT_generate_roof_walkable()` — освещение кровельных граней
- **⚠️ МЁРТВЫЙ КОД: `return;` сразу после вызова roof_walkable** — остальное тело не выполняется
- Скрытая логика (если бы работала): ambient + точечные источники из NIGHT_slight[]; hardcoded нормаль (0,1,0) для всех walkable вершин; поиск в bounding box ±0x400 по мап-квадратам; `bright = (256 - dist*256/radius) * dot(-dy/dist, -1)` → RGB накопление

### Динамические источники (light.cpp)
```c
LIGHT_create(x, y, z, range, type):
// Типы: NORMAL, BROKEN (мерцание), PULSE, FADE
// LIGHT_range = 0x600 максимум (~1536 units)
```

**ВАЖНО: Динамических теней НЕТ.**
Тени под персонажами — flat sprite (`POLY_PAGE_SHADOW`), позиционируется под ногами.
Статическое освещение уровня запекается при загрузке, обновляется только при смене дня/ночи.

---

## Crinkle система

Crinkle = **микро-геометрический bump mapping**. НЕ texture-based normal mapping.
Принцип: заранее заданный 3D mesh (crinkle) проецируется на поверхность квада в мировых координатах.

### Структуры (`Crinkle.cpp`)
- `CRINKLE_Point` {vec1, vec2, vec3, c[4]}: (vec1, vec2) = барицентрические координаты в [0,1] по U/V квада; vec3 = фактор выдавливания вдоль нормали; c[4] = билинейные веса цветов из 4 углов квада
- `CRINKLE_Face` = треугольник из 3 индексов в CRINKLE_Point[]
- `CRINKLE_Crinkle` = набор points + faces

Два типа: **ground crinkle** (XZ варьируется → на полу) и **wall crinkle** (XY варьируется → на стене).

### CRINKLE_do(handle, page, extrude, pp[4], flip)
1. Un-warp view space (убирает искажение FOV)
2. Берёт векторы квада: ax/ay/az = pp[1]-pp[0], bx/by/bz = pp[2]-pp[0]
3. Нормаль cx/cy/cz = cross(a,b), нормализуется до длины 0.05/len → вектор выдавливания
4. Для каждой crinkle-точки: `pos = pp[0] + vec1*a + vec2*b + vec3*c*extrude`
5. Цвет = билинейная интерполяция из 4 углов квада (веса c[0..3], scale=128)
6. Рендерит все треугольники crinkle mesh с перспективной проекцией

Дистанционный LOD в aeng.cpp: близко → полный extrude; промежуточная зона → меньший; далеко → не рендерится.

Хранение: `TEXTURE_crinkle[page]` — один crinkle handle на текстурную страницу (22×64 = 1408 слотов).
Загрузка: `CRINKLE_read_bin()` из FileClump (бинарный) или `CRINKLE_load()` из .asc файла.

**Лимиты**: 256 crinkles, 8192 points, 8192 faces, 700 points/crinkle.

### ⚠️ ПОЛНОСТЬЮ ОТКЛЮЧЕНЫ В ПРЕ-РЕЛИЗЕ
- `CRINKLE_load()` — `return NULL` в начале функции (всегда возвращает NULL!)
- `aeng.cpp:9605`: `if (/*crinkles enabled*/ 0 && ...)` → всегда FALSE
- На DC: `DISABLE_CRINKLES=1` → все массивы размером 1

**Статус:** Отключены в пре-релизном коде (`return NULL` + `if(0)`), но **пользователь подтвердил: bump mapping на деревянных ящиках РАБОТАЕТ в финальной PC версии** (не в PS1). Значит код был включён после заморозки этой кодовой базы.
**Для новой игры:** Переносить (PC фича). Реализация в оригинале = микро-геометрия. В новой игре лучше через normal/parallax mapping в fragment shader (визуально эквивалентно, но проще и эффективнее).
