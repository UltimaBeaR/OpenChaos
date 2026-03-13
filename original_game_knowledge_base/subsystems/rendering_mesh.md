# Рендеринг: Tom's Engine и система мешей — Urban Chaos (DDEngine)

**Ключевые файлы:**
- `fallen/DDEngine/figure.cpp` — Tom's Engine, рендеринг персонажей
- `fallen/DDEngine/mesh.cpp` — загрузка и рендеринг 3D мешей
- `fallen/DDEngine/aeng.cpp` — главный движок рендеринга (DrawTween/DrawMesh dispatch)

Общая документация рендеринга: **[rendering.md](rendering.md)**

---

## 1. Tom's Engine — рендеринг персонажей

**`USE_TOMS_ENGINE_PLEASE_BOB = 1`** (`DDEngine/Headers/aeng.h`) — всегда включён (PC и DC).

D3D-дружественный рендерер персонажей. Вся графика персонажей идёт через него.
В figure.cpp весь путь рендеринга под `#if USE_TOMS_ENGINE_PLEASE_BOB` — единственный активный путь.

```c
// figure.cpp
SLONG FIGURE_alpha = 255;  // альфа прозрачности персонажа (0=прозрачный, 255=непрозрачный)
UBYTE body_part_upper[];   // таблица upper body parts для 15 типов персонажей
```

**Освещение персонажей:**
- `BuildMMLightingTable()` — строит таблицу освещения из NIGHT системы
- Источники: `NIGHT_found[]` (статические) + `NIGHT_amb_r/g/b` (ambient)
- **Anti-lights:** существуют **отрицательные** источники света — вычитают яркость (для тёмных зон)

**`HIGH_REZ_PEOPLE_PLEASE_BOB`** — закомментирован ("Now enabled only on a cheat!").
Высокополигональные модели в финале **не активны** — только low-poly.

---

## 2. Система мешей (mesh.cpp)

**Структура загруженного меша:**
```c
struct PrimObject {
    UWORD StartPoint, EndPoint;  // Диапазон вершин в глобальном массиве
    PrimFace3 *faces3;           // Треугольные грани
    PrimFace4 *faces4;           // Четырёхугольные грани
    // Каждая грань: индексы вершин + page (текстура) + флаги
};
```

Детальный формат .prm файла: **[resource_formats/model_format.md](../resource_formats/model_format.md)**

**Флаги граней:**
- masked (маска/cutout), semi-transparent, self-illuminating и т.д.
- `FACE_FLAG_WALKABLE = (1<<6)` — только эти face4 образуют ходимые поверхности

**MESH_draw_poly() — процесс рендеринга:**
1. Трансформация вершин (локальное → мировое, матрица поворота 3×3)
2. Для каждой грани:
   - Back-face culling
   - Трансформация вершин в view space (`POLY_transform`)
   - Добавление в POLY buffer: `POLY_add_triangle()` / `POLY_add_quad()`
3. Освещение из предвычисленного `NIGHT_Colour` массива

---

## 3. Vertex Morphing (tweening) — анимация персонажей

```c
MESH_draw_morph(prim, morph1, morph2, tween, ...):
// tween ∈ [0, 256], где 256 = полностью morph2
// Вершины линейно интерполируются между двумя ключевыми кадрами
```

- **Нет скелетной анимации** — только vertex morphing между keyframes
- Топология (грани) одинакова для всех morphs
- `DrawTween` = морфинг между двумя кадрами (для персонажей)
- `DrawMesh` = один конкретный кадр (быстрее, для статичных объектов)

**InterruptFrame** — МЁРТВЫЙ КОД: всегда NULL, нигде не присваивается ненулевое значение в пре-релизе.

Детальный формат .all файла (анимации): **[resource_formats/animation_format.md](../resource_formats/animation_format.md)**

**Сжатая матрица CMatrix33** (characters.md):
- `CMatrix33 = {SLONG M[3]}` — 3 packed SLONG, scale=128
- `GameKeyFrameElement` = 8 байт: {m00,m01,m10,m11: SBYTE; OffX,Y,Z: SBYTE; Pad: UBYTE}
- `GetCMatrix()`: UCA_Lookup[a][b]=Root(16383-a²-b²); строка 2 = cross product >>7

---

## 4. Деформация транспорта (Crumple)

```c
MESH_set_crumple(...)         // установить параметры деформации меша
MESH_car_crumples[]           // таблица предвычисленных смещений
```

- **5 уровней урона × 8 вариантов × 6 точек** — predefined crumple vectors
- Смещения вершин применяются при трансформации меша
- Используется для визуального повреждения машин

---

## 5. Отражения в лужах

```c
MESH_draw_reflection(...)     // специальный путь рендеринга — объект как отражение
PUDDLE_in(...)                // определяет зону отражения
```

- Объекты, попадающие в зону лужи, рендерятся дополнительно как отражение
- Используется страница `POLY_PAGE_PUDDLE` (скипается из основного bucket sort)
- Рендерится через `POLY_frame_draw_puddles()` — отдельный проход

---

## 6. UV упаковка в PrimFace4

UV координаты граней упакованы в 16-bit формат (mesh.cpp):

```c
// Из поля UV[i][0] и UV[i][1]:
float u = float(UV[i][0] & 0x3f) * (1.0F / 32.0F);  // 6 бит = позиция U внутри тайла
float v = float(UV[i][1]       ) * (1.0F / 32.0F);  // 8 бит = позиция V

// Страница текстуры из UV[i][0] верхних 2 бит + TexturePage поля:
SLONG page = (UV[i][0] & 0xc0) << 2;   // grid X позиция (0-3)
page |= face->TexturePage;              // base page
page += FACE_PAGE_OFFSET;              // FACE_PAGE_OFFSET = 8*64
```

Текстурный атлас организован как сетка 8×8 тайлов, каждый тайл 64×64 пикселей.

---

## 7. Портирование на новый движок

**DrawTween → vertex morphing в vertex shader:**
```glsl
// lerp вершин между двумя frame буферами
vec3 pos = mix(frame0.pos, frame1.pos, tween);
```

**Crumple → offset в vertex shader или CPU-side:**
- Простые UBYTE смещения, легко маппируются

**Советы:**
- Vertex morphing хорошо ложится на double VBO (два frame, uniform tween)
- MESH_car_crumples таблица переносится as-is (предвычисленные данные)
- Отражения в лужах: в новом движке — render-to-texture + planar reflection (или просто screen-space)
