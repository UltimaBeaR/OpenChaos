# Black Holes Near-Clipping Investigation

**Баг:** Целые полигоны (пол, стены, машины, персонажи) пропадают — видны чёрные дырки. Проявляется на периферии зрения, когда вершины объектов уходят за фрустум камеры. Затрагивает **все типы геометрии** — отсекаются **по полигонам**.

**Оригинальный баг движка** — присутствовал в D3D6, воспроизводится и в OpenGL.

---

## Что точно установлено

### 1. Проблема зависит от `POLY_ZCLIP_PLANE` (CPU near-clip plane)

- Увеличение `POLY_ZCLIP_PLANE` с 1/64 до 1/16 → дырок **значительно больше**
- Уменьшение до 1/512 → дырок меньше
- Зависимость прямая и однозначная

### 2. Проблема в CPU-коде, не в GPU/OpenGL

Разделили `POLY_ZCLIP_PLANE` на две независимых константы:
- `POLY_CPU_ZCLIP_PLANE` = 1/16 (CPU near-clip threshold)
- `POLY_ZCLIP_PLANE` = 1/64 (GPU depth remap, projection matrix, viewport)

При увеличении **только CPU** порога дырок стало больше → **проблема в CPU-side коде** движка, не в OpenGL рендеринге.

### 3. Одно из мест которое **точно влияет**: `POLY_perspective()`

**Файл:** `new_game/src/engine/graphics/pipeline/poly.cpp`, функция `POLY_perspective()`:
```cpp
if (pt->z < POLY_Z_NEARPLANE) {
    pt->clip = POLY_CLIP_NEAR;
} else if (pt->z > 1.0F) {
    pt->clip = POLY_CLIP_FAR;
} else {
    pt->Z = POLY_ZCLIP_PLANE / pt->z;
    pt->X = POLY_screen_mid_x - POLY_screen_mul_x * pt->x * pt->Z;
    pt->Y = POLY_screen_mid_y - POLY_screen_mul_y * pt->y * pt->Z;
    POLY_setclip(pt);
}
```

Замена `POLY_Z_NEARPLANE` на `POLY_ZCLIP_PLANE` (1/64) в этом конкретном месте **уменьшила** дырки. Подтверждено тестированием.

Эта функция вызывается из `POLY_transform()` — основной путь трансформации для mesh objects, terrain points, эффектов.

### 4. Влияет **не только** `POLY_perspective()`

При изменении только `POLY_perspective()` дырки уменьшились, но **не исчезли полностью**. Есть минимум ещё одно место. Кандидаты (не проверены):
- `POLY_transform_from_view_space()` (poly.cpp) — аналогичная проверка
- `POLY_point_in_view()` (poly.cpp) — `if (vz < POLY_Z_NEARPLANE)`
- `POLY_sphere_visible()` (poly.cpp) — `if (view_z + radius <= POLY_Z_NEARPLANE)`
- `NewTweenVertex3D()` (poly.cpp) — `np->z = POLY_Z_NEARPLANE` (clamp to near plane)
- `POLY_add_nearclipped_triangle()` (poly.cpp) — `s_DistBuffer = z - POLY_Z_NEARPLANE`
- `figure.cpp` — bounding sphere checks against `POLY_ZCLIP_PLANE`

### 5. `POLY_transform_c_saturate_z()` — **НЕ влияет**

Замена порога в этой функции не изменила количество дырок. Эта функция используется для стен (FACET_draw) — значит стены дырятся не из-за `c_saturate_z`.

---

## Что точно **НЕ является** причиной

Все перечисленные системы были полностью отключены (через debug toggle) — дырки остались:

1. **Gamut** (NGAMUT) — расширен до всей карты 128×128
2. **FACET_FLAG_INVISIBLE** — рисуются невидимые facets
3. **2D backface cull** стен (FACET_draw / FACET_draw_rare)
4. **AABB frustum rejection** стен (FACET_draw / FACET_draw_rare)
5. **PAP_FLAG_HIDDEN / PAP_FLAG_ROOF_EXISTS** — skip пола
6. **POLY_valid_quad / POLY_valid_triangle** — всегда TRUE
7. **POLY_perspective FAR** — всегда проецирует
8. **POLY_add_tri/quad OFFSCREEN reject**
9. **POLY_add_tri/quad 3D backface cull**
10. **POLY_nearclip offscreen reject**
11. **POLY_sphere_visible** — всегда TRUE
12. **draw_quick_floor zclip** (dprod < 8.0)
13. **FASTPRIM distance rejection**
14. **STOREY_TYPE_JUST_COLLISION skip**
15. **BUILDING_TYPE_CRATE_IN / FACET_FLAG_INSIDE skip**
16. **FC_can_see_person + POLY_sphere_visible** — все люди видимы
17. **GF_NO_FLOOR flag**
18. **figure.cpp near-Z bounding sphere cull** (per body part)
19. **Scissor test** (OpenGL) — отключён, не помогло
20. **GL_DEPTH_CLAMP** — включён, не помогло
21. **Guardband clamp** screen coordinates (CPU и shader) — не помогло

---

## Характеристики бага

- Затрагивает **все типы геометрии**: пол, стены, mesh-объекты (машины, ящики), персонажи
- Отсекаются **целые полигоны**, не по depth-buffer
- Дырки всегда на **периферии зрения** (сбоку/сзади камеры), не перед ней
- При NOCULL (все CPU-reject отключены) дырки **остаются** → полигоны не генерируются или дропаются до PolyPage
- NOCULL влияет на персонажей (мент виден полностью) через figure.cpp bounding sphere, но **не** на стены/пол

---

## Ключевой вывод: вся система near-clip влияет как единое целое

Попытка разделить `POLY_Z_NEARPLANE` на CPU и GPU константы показала:
- **Все 7 мест** в poly.cpp, использующих `POLY_Z_NEARPLANE`, **взаимосвязаны** — это единая система near-clipping
- Нельзя менять порог в одном месте не меняя в остальных — рассогласование ломает проекцию (перекошенные полигоны)
- Каждое место по отдельности **влияет** на дырки (подтверждено итеративным тестированием)
- Все 7 мест: `POLY_perspective`, `POLY_transform_c_saturate_z`, `POLY_transform_from_view_space`, `POLY_point_in_view`, `POLY_sphere_visible`, `NewTweenVertex3D`, `POLY_add_nearclipped_triangle`

## Рабочая гипотеза

Near-clipping система как целое **теряет полигоны**. Возможные механизмы:

1. **Split quad → 2 triangles перед near-clip**: один треугольник может получить все 3 вершины NEAR → `POLY_clip_against_nearplane` возвращает 0 → треугольник дропается → дырка, хотя квад в целом виден.

2. **Offscreen reject после near-clip** (`clip_and & POLY_CLIP_OFFSCREEN`): вершины на near plane имеют огромные screen X/Y → все LEFT/RIGHT → дроп. (Попытка отключить не помогла — но возможно нужен более чистый тест.)

3. **Неизвестный механизм** в near-clip пути, пока не идентифицирован.

Увеличение near plane → больше вершин NEAR → больше квадов в near-clip → больше потерь.

---

## TODO

- [ ] Найти конкретный механизм потери полигонов в near-clip path
- [ ] Попробовать full-polygon near-clip для квадов вместо split-then-clip (чистый тест без рассогласования констант)
- [ ] Исследовать offscreen reject после near-clip с логированием конкретных дропнутых полигонов
- [ ] Проверить `draw_quick_floor` path (DrawIndPrimMM) — использует другую near-clip систему (dprod < 8.0)
