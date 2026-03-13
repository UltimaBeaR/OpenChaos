# Рендеринг — Освещение и Crinkle

Вынесено из `rendering.md` (разделено при превышении 400 строк).

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

**Для новой игры:** Не реализовывать (отключены даже в оригинале). Если нужен аналог — parallax mapping или normal mapping в fragment shader.
