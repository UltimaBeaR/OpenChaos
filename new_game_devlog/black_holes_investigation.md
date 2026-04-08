# Black Holes Near-Clipping Investigation

**Баг:** Целые полигоны (пол, стены, машины, персонажи) пропадают — видны чёрные дырки. Проявляется на периферии зрения, когда вершины объектов уходят за фрустум камеры. Затрагивает **все типы геометрии** — отсекаются **по полигонам**.

**Оригинальный баг движка** — присутствовал в D3D6, воспроизводится и в OpenGL.

**Статус: ИСПРАВЛЕНО.**

---

## Как работает near-clip path

1. `POLY_perspective()` помечает вершину как `POLY_CLIP_NEAR` если `z < POLY_ZCLIP_PLANE`
2. `POLY_add_quad_fast()` видит что у квада есть вершины с NEAR → сплитит квад на 2 треугольника → каждый отправляет в `POLY_add_nearclipped_triangle()`
3. `POLY_add_nearclipped_triangle()` клипает треугольник по near plane через Sutherland-Hodgman → fan-triangulate → submit в PolyPage

---

## Причина найдена: отсутствовал PolyBufAlloc

**Файл:** `new_game/src/engine/graphics/pipeline/poly.cpp`, функция `POLY_add_nearclipped_triangle()`

### Суть бага

В fan-triangulation после Sutherland-Hodgman вершины записывались в PolyPage через `PointAlloc()`, но **полигоны не регистрировались** — `PolyBufAlloc()` не вызывался. GPU никогда не рисовал эти вершины.

Для сравнения, `POLY_add_triangle_fast()` и `POLY_add_quad_fast()` правильно вызывают `PolyBufAlloc()` и устанавливают `first_vertex` / `num_vertices`. В near-clip path этого не было.

### Фикс

Переписана fan-triangulation: для каждого треугольника fan'а вызывается `PolyBufAlloc()`, устанавливается `first_vertex` и `num_vertices = 3`. Полигоны теперь регистрируются и рисуются GPU.

### Дополнительно: персонажи (figure.cpp)

Персонажи рисуются через MultiMatrix path (`ge_draw_multi_matrix`), минуя POLY pipeline. Три проблемы:

1. **Distance check с пустым else** (строки 2114, 4344): когда часть тела ближе near plane — fast path пропускал, а fallback не был реализован (оригинальный `FIXME` от разработчиков). **Фикс:** distance check убран, MultiMatrix рисует всегда.

2. **Bounding sphere cull per body part** (строка 3804): отбрасывал целые конечности. **Фикс:** убран, frustum cull в `POLY_sphere_visible` достаточен.

3. **Деление на ноль в MultiMatrix** (polypage.cpp): `1.0f / z` при z близком к нулю давало мусор. **Фикс:** z guard `if (z < 0.001f) z = 0.001f`.

---

## Ход расследования

### Что точно установлено

1. **Проблема зависит от `POLY_ZCLIP_PLANE`** — увеличение порога → больше вершин NEAR → больше потерь. Зависимость прямая.

2. **Проблема в CPU-коде, не в GPU/OpenGL** — подтверждено разделением на две константы (CPU и GPU) и независимым тестированием.

3. **`POLY_CLIP_NEAR` в 3 функциях проекции — вход в near-clip path** — при отключении `POLY_CLIP_NEAR` во всех 3 функциях все дырки полностью пропадают (побочный эффект: артефакты от вершин за камерой).

4. **Sutherland-Hodgman работает корректно** — логирование показало что `POLY_clip_against_nearplane` возвращает 3-4 вершины (не 0) для всех входящих треугольников. Алгоритм не теряет полигоны.

5. **Offscreen reject после near-clip отбрасывал ВСЁ** — логирование показало `offscreen=1` для 100% near-clip треугольников. Вершины у near plane имеют экстремальные screen coords → все помечаются BOTTOM → `clip_and & POLY_CLIP_OFFSCREEN` отбрасывает. Но это следствие, не причина — вершины записывались в буфер но не регистрировались.

6. **Проверка `pt->z < POLY_ZCLIP_PLANE` работает правильно** — проверено визуально пользователем: зелёная покраска вершин точно совпадает с границей near plane в wireframe-режиме.

### Что точно НЕ является причиной

Все перечисленные системы были полностью отключены — дырки остались:

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

### Эксперименты

**ALWAYS_DISCARD** (return в начале `POLY_add_nearclipped_triangle`): картинка идентична обычному поведению → near-clip path и так ничего не пропускал (из-за отсутствия PolyBufAlloc).

**NEVER_DISCARD** (пропуск Sutherland-Hodgman): артефакты — NEAR-вершины не спроецированы (нет X,Y), проскакивают с мусором.

**Покраска вершин** по `z < POLY_ZCLIP_PLANE`: работает корректно, точно совпадает с debug plane. Проверено пользователем в wireframe.

---

## Характеристики бага

- Затрагивает **все типы геометрии**: пол, стены, mesh-объекты (машины, ящики), персонажи
- Отсекаются **целые полигоны**, не по depth-buffer
- Дырки всегда на **периферии зрения** (сбоку/сзади камеры), не перед ней
- Увеличение near plane → больше вершин NEAR → больше потерь
