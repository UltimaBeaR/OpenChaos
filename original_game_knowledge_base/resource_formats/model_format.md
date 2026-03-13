# Форматы 3D моделей — Urban Chaos

**Расположение:** `original_game/fallen/Debug/Meshes/`
**Ключевые файлы загрузки:** `fallen/Source/io.cpp`, `fallen/DDEngine/Source/mesh.cpp`

---

## .SEX — исходный текстовый формат

ASCII текст, экспорт из 3DS MAX. Используется в редакторе, не в игре напрямую.

```
# комментарий
Version: 3.14

Triangle mesh: {MeshName}
    Pivot: (x, y, z)
    Matrix: (m00, m01, m02, m10, m11, m12, m20, m21, m22)   — матрица 3×3
    Material: DiffuseRGB (r,g,b), shininess 0.25, ...
    Vertex: (x, y, z)
    Texture Vertex: (u, v)
    Face: v1, v2, v3 [, v4]   — треугольник или квадрат
```

В игре не используется — только исходник для компилятора моделей.

---

## .IMP — скомпилированный бинарный формат

**Функция загрузки:** `io.cpp` (конкретная функция уточнить)

```
[4 байта]  SLONG version = 1
[4 байта]  SLONG mesh_count            — количество сеток

Для каждой сетки:
  [C-строка]  name                     — имя сетки (нулл-терминированная)

  Для каждого материала:
    [2 байта]  UWORD vertex_count
    [2 байта]  UWORD face_count
    [6 байт]   UWORD index_start, index_end, index3   — диапазоны индексов
    [float×6]  AABB bounding box (min/max по X,Y,Z)
    [float×3]  Diffuse RGB (значения 0.0–0.5)
    [float]    shininess
    [float]    shinstr                  — strength блика
    [1 байт]   UBYTE flags             — single-sided, filtered alpha и т.д.
    [64 байта] char diffuse_texture[]  — путь к текстуре diffuse
    [64 байта] char bump_map[]         — путь к bump map

  Вершины:
    struct Vertex { float x, y, z; float u, v; };

  Грани:
    UWORD indices[]                    — индексированные треугольники
```

---

## .MOJ — скомпилированные иерархические объекты (multi-prim)

**Функция загрузки:** `io.cpp::load_a_multi_prim()`

Основной рабочий формат движка. Содержит иерархические объекты — машины, персонажи, элементы окружения. Prim = отдельный примитив (меш с материалом).

### Ключевые структуры

```c
struct PrimPoint {         // вершина — 6 байт (SWORD, не SLONG!)
    SWORD X, Y, Z;         // 2 байта каждый — локальные координаты
};
// ВАЖНО: SWORD (int16), не SLONG (int32)!

struct PrimFace3 {         // треугольный полигон
    UWORD Points[3];       // индексы вершин
    UWORD material;        // индекс материала/текстуры
};

struct PrimFace4 {         // четырёхугольный полигон
    UWORD Points[4];       // индексы вершин
    UWORD texture;         // индекс текстуры
    UWORD flags;           // double-sided, transparent и т.д.
};
```

### Иерархия объектов

.MOJ файл содержит дерево примитивов с трансформациями (матрицы смещения/поворота).
Используется для:
- Автомобилей (кузов + колёса — отдельные примы, связанные иерархически)
- Персонажей (сегменты тела для vertex morphing анимации)
- Элементов окружения (составные объекты)

---

## .TXC — архивы текстур уровней

**Расположение:** `original_game/fallen/Debug/clumps/`

Бинарный архив текстур, специфичных для конкретного уровня (не глобальных).
TGA → внутренний формат при пакете.
Заголовок: начинается с 4-байтного значения (вероятно количество текстур или размер).

Полная структура требует дополнительного анализа.

---

## .PRM — статические примы мира

**Расположение:** `server/prims/nprim###.prm` (новый формат) или `server/prims/prim###.prm` (старый)
**Функция загрузки:** `io.cpp::load_prim_object(prim)`
**Количество типов:** 1–265 (индексы в prim_objects[])

Прим = статический 3D объект мира: фонарь, урна, знак, бочка, забор, дерево и т.д.

### .PRM бинарный формат (новый, nprim###.prm)

```
[2 байта]  UWORD save_type             — версия (PRIM_START_SAVE_TYPE=5793 + offset)
[32 байта] char  name[]                — имя объекта (null-padded)
[sizeof(PrimObject)] PrimObject        — заголовок: диапазоны индексов

[num_points * sizeof(PrimPoint)] PrimPoint[]    — вершины
[num_faces3 * sizeof(PrimFace3)] PrimFace3[]    — треугольные полигоны
[num_faces4 * sizeof(PrimFace4)] PrimFace4[]    — квадратные полигоны
```

### Структура PrimObject (16 байт)

```c
// fallen/Editor/Headers/Prim.h
struct PrimObject {
    UWORD StartPoint;   // первый индекс в prim_points[] (file-relative)
    UWORD EndPoint;     // последний+1 → num_points = EndPoint - StartPoint
    UWORD StartFace4;   // диапазон quads в prim_faces4[]
    UWORD EndFace4;
    SWORD StartFace3;   // диапазон tris в prim_faces3[]
    SWORD EndFace3;
    UBYTE coltype;      // тип коллизии
    UBYTE damage;       // как пострадает при ударе
    UBYTE shadowtype;   // тип тени
    UBYTE flag;         // доп. флаги
};
// sizeof = UWORD×4(8б) + SWORD×2(4б) + UBYTE×4(4б) = 16 байт
// Внимание: SWORD StartFace3/EndFace3 (знаковые — отрицательные = нет треугольников)
```

После загрузки: индексы Points[j] в face-структурах смещаются от file-relative
до global-array offset: `Points[j] += next_prim_point - po->StartPoint`.

### Структура PrimPoint (6 байт)

```c
struct PrimPoint {
    SWORD X, Y, Z;   // SWORD (int16)! НЕ SLONG (int32)!
};
// Старый формат (OldPrimPoint): SLONG X,Y,Z = 12 байт — определяется по save_type
```

**Критично:** при save_type == PRIM_START_SAVE_TYPE+1 → SWORD (6 байт),
иначе → OldPrimPoint (SLONG, 12 байт). Все Steam ресурсы используют новый формат.

### Структура PrimFace3 (PC, 28 байт)

```c
struct PrimFace3 {
    UBYTE TexturePage;  // индекс текстурной страницы
    UBYTE DrawFlags;    // флаги рендеринга
    UWORD Points[3];    // индексы вершин (global, после fixup)
    UBYTE UV[3][2];     // UV координаты (каждый 0-255)
    SWORD Bright[3];    // яркость вершины (используется для людей)
    SWORD ThingIndex;   // индекс Thing для walkable faces
    UWORD Col2;         // (WALKABLE alias) walkable face index
    UWORD FaceFlags;    // FACE_FLAG_* (WALKABLE, ROOF, SHADOW, ENVMAP...)
    UBYTE Type;         // тип поверхности
    SBYTE ID;
};
// sizeof = 1+1+6+6+6+2+2+2+1+1 = 28 байт

### Структура PrimFace4 (PC, 34 байта)

```c
struct PrimFace4 {
    UBYTE TexturePage;
    UBYTE DrawFlags;
    UWORD Points[4];    // 4 индекса вершин
    UBYTE UV[4][2];
    union {
        SWORD Bright[4];          // для людей
        struct { UBYTE red, green, blue; } col;  // для зданий
    };
    SWORD ThingIndex;
    SWORD Col2;
    UWORD FaceFlags;    // FACE_FLAG_WALKABLE (1<<6) = ходимая поверхность!
    UBYTE Type;
    SBYTE ID;
};
// sizeof = 1+1+8+8+8(union: max(SWORD×4=8, struct col=3)=8)+2+2+2+1+1 = 34 байта

**FACE_FLAG_WALKABLE (1<<6)** — ключевой флаг: именно эти PrimFace4 образуют поверхности для
навигации, коллизий и `find_face_for_this_pos()`.

### Известные PRIM_OBJ_* константы

```c
#define PRIM_OBJ_LAMPPOST        1
#define PRIM_OBJ_TRAFFIC_LIGHT   2
#define PRIM_OBJ_PETROL_PUMP     4
#define PRIM_OBJ_BILLBOARD       7
// ...всего ~265 типов
```

