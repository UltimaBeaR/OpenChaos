# Z-Order / Draw Order Investigation

**Дата начала:** 2026-04-11
**Статус:** исследование

## Проблема

Множество визуальных багов связаны с неправильным порядком отрисовки полупрозрачных и непрозрачных объектов. Баги проявляются на разных миссиях и с разными типами объектов, но корневая причина общая — архитектура bucket sort в poly pipeline.

### Список конкретных багов (из known_issues_and_bugs.md)

1. **Лежаки у бассейна (Estate of Emergency)** — листья на заднем плане рисуются сквозь лежаки, два рядом стоящих лежака — один всегда поверх другого.
2. **Кусты и лавочки (Psycho Park)** — кусты перекрывают друг друга с артефактами, лавочка поверх куста, трава вокруг дерева поверх лавочки.
3. **Огонь за лавочкой (Psycho Park)** — огонь рисуется поверх лавочки, хотя находится позади.
4. **Вывеска Hot Dawg (The Fallen)** — зелёный текст иероглифами позади вывески просвечивает поверх неё.
5. **Провода поверх листвы (Assassin)** — провода рисуются поверх листвы деревьев.
6. **Пар/гейзеры vs листва (Target UC)** — задние гейзеры поверх передних, листва ёлок поверх гейзеров.
7. **Грани ёлок (Target UC)** — 4 полупрозрачные грани листвы ёлок рисуются в неправильном порядке при вращении камеры.
8. **Свет от мин сквозь объекты** — additive спрайт рисуется поверх непрозрачной геометрии (может быть намеренно — сверить с оригиналом).

---

## Архитектура рендер-pipeline

### Главная функция: AENG_draw_city() — aeng.cpp:3713

Порядок добавления геометрии в deferred pipeline:

```
1.  Небо (POLY_PAGE_SKY)
2.  Земля (terrain — PAP_Hi pages, стандартные текстуры)
3.  Тени (POLY_PAGE_SHADOW/SHADOW_OVAL/SHADOW_SQUARE, DepthBias=8)
4.  Отражения в лужах / Drips
5.  POLY_frame_draw (shadows=FALSE, text=FALSE) — flush отражений/drips
6.  Лужи — POLY_frame_draw_puddles()
7.  Второй flush (POLY_frame_draw_odd + puddles)
8.  Далёкие здания (far facets)
9.  Полы (дополнительные проходы)
10. Примы (мебель, лавочки, провода, гирлянды, вывески, bloom спрайты)
    → добавляются в стандартные PolyPages (0-1407) через MESH_draw_poly
11. Стены зданий (facets) → стандартные PolyPages
12. Персонажи → ge_draw_multi_matrix (IMMEDIATE DRAW, в обход PolyPage!)
13. Дождь
14. Пиротехника: AENG_draw_bangs(), AENG_draw_fire(), AENG_draw_sparks()
    → deferred pages: POLY_PAGE_BANG, FLAMES, EXPLODE, SMOKE и т.д.
15. Torch эффекты
16. People messages (chat bubbles)
17. Dirt: AENG_draw_dirt() — листья, мусор, снежинки
18. *** POLY_frame_draw *** — ГЛАВНЫЙ FLUSH всех deferred полигонов
```

### Как работает POLY_frame_draw (poly.cpp:1879)

При `AlphaSortEnabled=true` (всегда в геймплее):

#### Фаза A — Sky
`POLY_PAGE_SKY` рисуется первым (всегда на заднем плане).

#### Фаза B — Opaque pass (строки 1899-1947)
Итерация всех страниц в `PageOrder`. Для каждой страницы: если `!pa->RS.NeedsSorting()` (т.е. `DepthWrite=true`) — рисуем сразу. `POLY_PAGE_COLOUR` принудительно первым.

Сюда попадает вся непрозрачная геометрия: земля, стены, стандартные текстуры примов, листья (POLY_PAGE_LEAF — DepthWrite=true после фикса), мусор (POLY_PAGE_RUBBISH).

#### Фаза C — Sorted pass (строки 1949-2016)
Все страницы с `NeedsSorting()=true` (т.е. `DepthWrite=false`) объединяются в глобальный массив из **2048 бакетов**.

Ключ сортировки: `sort_z = max(pts->Z)` где Z — reciprocal depth (`POLY_ZCLIP_PLANE / view_z`). Индекс бакета = `int(sort_z * 2048)`, clamp [0, 2047]. Бакет 0 = далеко, бакет 2047 = близко.

Sorted pass разделён на **два прохода**:
- **Sub-phase 1 (non-additive):** полигоны с `!IsAdditiveBlend()` — alpha-blended геометрия
- **Sub-phase 2 (additive):** полигоны с `IsAdditiveBlend()` — аддитивные эффекты

Оба прохода итерируют бакеты 0..2047 (далеко → близко) = back-to-front.

### Критерии сортировки

```
NeedsSorting() = !DepthWrite          (game_graphics_engine.h:641)
IsAdditiveBlend() = AlphaBlendEnabled && DstBlend == One   (line 645)
```

### Таблица PolyPage типов и их render path

| Page | DepthWrite | Blend | NeedsSorting? | Фаза |
|------|-----------|-------|---------|-------|
| Стандартные текстуры (default) | true | нет | НЕТ → opaque | B |
| POLY_PAGE_FLAG_TRANSPARENT | true (default) | alpha test only | НЕТ → opaque | B |
| POLY_PAGE_FLAG_ADD_ALPHA | **false** | One+One | ДА | C sub-phase 2 (additive) |
| POLY_PAGE_FLAG_ALPHA | **false** | SrcAlpha+InvSrcAlpha | ДА | C sub-phase 1 |
| POLY_PAGE_LEAF | **true** (после фикса) | alpha test | НЕТ → opaque | B |
| POLY_PAGE_RUBBISH | **true** (после фикса) | alpha test | НЕТ → opaque | B |
| POLY_PAGE_BLOOM1/2 | false | SrcAlpha+One | ДА | C sub-phase 2 |
| POLY_PAGE_FLAMES/2/3 | false | SrcAlpha+One или InvSrcAlpha+One | ДА | C sub-phase 2 |
| POLY_PAGE_SMOKE | false | Zero+InvSrcColor | ДА | C sub-phase 1 (!) |
| POLY_PAGE_BANG | false | One+One | ДА | C sub-phase 2 |
| POLY_PAGE_SHADOW | false | varies | ДА | C sub-phase 1 |
| POLY_PAGE_BARBWIRE | false | SrcAlpha+InvSrcAlpha | ДА | C sub-phase 1 |
| POLY_PAGE_EXPLODE1/2 | false | SrcAlpha+InvSrcAlpha | ДА | C sub-phase 1 |
| POLY_PAGE_EXPLODE1/2_ADDITIVE | false | SrcAlpha+One | ДА | C sub-phase 2 |
| POLY_PAGE_SMOKECLOUD | false | Zero+InvSrcAlpha | ДА | C sub-phase 1 |
| POLY_PAGE_BLOODSPLAT | false | SrcAlpha+InvSrcAlpha | ДА | C sub-phase 1 |

### Depth Bias

`SetDepthBias(N)` → `glPolygonOffset(-1.0f, -N*512)`. Используется:
- `POLY_PAGE_SHADOW` (bias=8)
- `POLY_PAGE_SHADOW_OVAL` (bias=8)
- `POLY_PAGE_SHADOW_SQUARE` (bias=8)
- `POLY_PAGE_TYRESKID` (bias=4)

Это ground-hugging декали — нужен offset чтобы не z-файтились с землёй.

---

## Корневые причины багов

### 1. Грубая bucket-сортировка (2048 бакетов)

Весь диапазон глубины (reciprocal depth 0..1) разбит на 2048 бакетов. Объекты на похожей глубине попадают в один бакет. Внутри бакета порядок — **LIFO** (linked list, последний добавленный рисуется первым). Порядок добавления определяется порядком обхода gamut в `AENG_draw_city()`, а не реальной глубиной.

**Результат:** два куста/лавочки на близкой глубине — произвольный порядок, зависящий от пространственного расположения в gamut.

### 2. sort_z = max(Z) вершин полигона

polypage.cpp:215 — `sort_z` берёт максимальное Z (ближайшую вершину). Для больших полигонов, растянутых в глубину, ближайшая вершина может быть значительно ближе к камере чем центр. Большой фоновый полигон может отсортироваться ближе маленького переднего.

### 3. Двухфазная сортировка не решает порядок внутри одной фазы

Sub-phase 1 содержит: canopy деревьев, заборы, barbwire, дым (SMOKE!), взрывы, кровь. Все конкурируют в тех же 2048 бакетах. Проволока (BARBWIRE) и canopy дерева на одной глубине — произвольный порядок.

**Важно:** POLY_PAGE_SMOKE использует `SrcBlend=Zero, DstBlend=InvSrcColor`. `IsAdditiveBlend()` проверяет `DstBlend == One`, поэтому дым — **non-additive** и рендерится в sub-phase 1, рядом с листвой и заборами.

### 4. Примы с ALPHA/ADD_ALPHA флагом текстуры

Стандартные текстуры примов (pages 0-1407) получают флаги из .tma файлов. Если текстура помечена `A` (ADD_ALPHA) или `M` (ALPHA) — её DepthWrite=false, и грань прима попадает в sorted pass. Там bucket precision недостаточна для корректного порядка между близко стоящими объектами.

### 5. Персонажи рисуются immediate (в обход PolyPage)

`ge_draw_multi_matrix` в figure.cpp:2131 — персонажи рисуются напрямую, минуя PolyPage pipeline и bucket sort. Z-формула была исправлена ранее (stage5_bug_zbuffer_mismatch.md) — теперь z-значения совместимы с POLY path, z-buffer корректно разрешает перекрытие персонажей со стенами/машинами. Но персонажи не участвуют в alpha sort — их порядок относительно полупрозрачной геометрии определяется только z-buffer. Мёртвые персонажи с alpha-страницами просто не рисуются (fast-reject).

---

## Предыдущие фиксы и расследования (для контекста)

### 1. Z-buffer mismatch персонажей (stage5_bug_zbuffer_mismatch.md)

Персонажи рисовались поверх стен/машин. Причина: два пути растеризации писали в z-buffer несовместимые значения:
- **Software POLY path** (стены, машины): `z_buf = 1 - POLY_ZCLIP_PLANE / view_z` (обратное z, корректно)
- **Hardware DrawIndPrimMM** (персонажи): `z_buf = view_z × POLY_ZCLIP_PLANE` (линейное z, НЕПРАВИЛЬНО)

Диапазоны не пересекались → z-тест всегда давал неправильный результат.

**Фикс:** исправлена одна строка в `DrawIndPrimMM` (polypage.cpp:615): `dvSZ = 1.0f - POLY_ZCLIP_PLANE / dvSZ`

**Важно для текущего расследования:** персонажи по-прежнему рисуются **immediate** (в обход PolyPage pipeline). Их z-order теперь корректен через z-buffer, но они не участвуют в alpha sort. Мёртвые персонажи с alpha-страницами (fast-reject) просто не рисуются.

### 2. Листья/debris поверх additive эффектов (leaf_overdraw_investigation.md)

Листья, ground debris, кроны деревьев рисовались поверх bloom/огня/дыма. Баг пре-релизных исходников (не наш).

**Корневая причина:** листья рисовались через immediate draw (ge_draw_indexed_primitive_lit) ПОСЛЕ `POLY_frame_draw()` — не участвовали в alpha sort.

**Фикс (3 изменения):**
1. Листья/снег переведены с immediate draw на deferred `POLY_add_triangle` + `AENG_draw_dirt()` перемещён ДО `POLY_frame_draw()`
2. `POLY_PAGE_LEAF` и `POLY_PAGE_RUBBISH`: `DepthWrite=true` → opaque pass (бинарный cutout, depth write корректен)
3. Sorted pass разделён на non-additive → additive sub-phases (крона деревьев alpha-blended в sub-phase 1, bloom/fire в sub-phase 2)

**Что НЕ пофикшено этим фиксом:**
- Порядок между объектами ВНУТРИ одной sub-phase (кусты vs кусты, проволока vs крона)
- Порядок объектов в opaque pass (мебель vs мебель — зависит от PageOrder)
- Трупы/ограждения — частично (требует доп. тестирования)

### 3. Тени — VB pool corruption (shadow_corruption_investigation.md)

`AENG_world_line` из game tick (до `POLY_frame_init`) вызывал corruption VB pool. Фикс: `s_in_render_pass` guard. Не связано напрямую с z-order, но вызывало визуальные артефакты теней.

---

## Проверка кода sorted pass (2026-04-11)

Детальная проверка реализации bucket sort в poly.cpp и polypage.cpp.

### Bucket sort — точная реализация

**Определение бакетов** (poly.cpp:1956):
```cpp
PolyPoly* buckets[2048];  // stack-allocated, NULL-initialized
```

**Добавление в бакеты** (polypage.cpp:335-356, `PolyPage::AddToBuckets`):
```cpp
int bucket = int(poly->sort_z * count);  // count = 2048
poly->next = buckets[bucket];            // LIFO — prepend к linked list
buckets[bucket] = poly;
```
**LIFO** — последний добавленный в бакет рисуется первым. Никакой сортировки внутри бакета нет.

**sort_z** (polypage.cpp:170-215, `AddFan`):
```cpp
float zmax = pts[0]->Z;
for (ii = 0; ii < num_vertices; ii++) {
    if (pts[ii]->Z > zmax) zmax = pts[ii]->Z;
}
pp->sort_z = zmax;  // max Z = ближайшая вершина (Z — reciprocal depth, больше = ближе)
```

**Итерация** (poly.cpp:1982-2010):
```cpp
for (i = 0; i < 2048; i++) { ... }  // 0→2047 = back-to-front (бакет 0 = далеко, 2047 = близко)
```

**Двойная итерация** (poly.cpp:1976-2016):
```cpp
for (SLONG phase = 0; phase < 2; phase++) {
    for (i = 0; i < 2048; i++) {
        // phase 0: только !IsAdditiveBlend() (non-additive: крона, заборы, дым)
        // phase 1: только IsAdditiveBlend() (additive: bloom, огонь)
    }
}
```

### Depth test в sorted pass

**Depth test ON (ReadOnly)**, depth write OFF. Это значит:
- Sorted полигоны **тестируются** против z-buffer из opaque pass
- Sorted полигоны **не пишут** в z-buffer
- Если объект A в opaque pass записал depth, объект B в sorted pass ЗА ним будет отсечён depth test'ом — **корректно**

Код (game_graphics_engine.cpp:219-225): для страниц с `DepthWrite=false`, `DepthEnabled=true` → `GEDepthMode::ReadOnly`.

### Ключевой вывод

Проблема z-order между двумя объектами в sorted pass возникает **только** когда оба имеют `DepthWrite=false`. Тогда ни один не пишет depth, и порядок определяется исключительно bucket sort:
- Если объекты на разной глубине → разные бакеты → **корректный** back-to-front порядок
- Если объекты на похожей глубине → **один бакет** → LIFO порядок (зависит от gamut traversal, не от глубины) → **некорректный** порядок

Если один объект в opaque pass (DepthWrite=true), а другой в sorted pass — depth test корректно разрешает перекрытие. Проблемы z-order между opaque и sorted не должно быть.

**Следующий шаг:** проверить page flags конкретных проблемных текстур (лежаки, кусты, лавочки, провода). Если у них ALPHA флаг → они в sorted pass → подтверждение гипотезы.

## Проверка конкретных render paths (2026-04-11)

### Пар (steam) — POLY_PAGE_STEAM

Функция `draw_steam` в figure.cpp:475-542 рисует столб из billboard-спрайтов через `SPRITE_draw_tex()` на `POLY_PAGE_STEAM`.

Blend settings (poly_render.cpp:674-681):
```
POLY_PAGE_STEAM: DepthWrite=false, Blend=One+One (additive)
```
`IsAdditiveBlend() = true` → **sub-phase 2 (additive)**.

Значит пар рисуется ПОСЛЕ non-additive полупрозрачных объектов. Если труба/решётка — opaque (DepthWrite=true), depth test отсечёт пар ЗА трубой. Если пар ПЕРЕД трубой — должен быть виден. Если труба/решётка alpha → sub-phase 1, пар sub-phase 2 → пар поверх. **Баг с трубой поверх пара не должен происходить** при текущей архитектуре — нужно проверить вживую, возможно пар генерируется внутри трубы (origin позиция) и часть спрайтов реально за трубой.

**Важно:** POLY_PAGE_STEAM ≠ POLY_PAGE_SMOKE!
| Page | Src | Dst | IsAdditive? | Sub-phase |
|------|-----|-----|-------------|-----------|
| POLY_PAGE_STEAM | One | One | ДА | 2 (additive) |
| POLY_PAGE_SMOKE | Zero | InvSrcColor | НЕТ | 1 (non-additive) |
| POLY_PAGE_SMOKECLOUD | Zero | InvSrcAlpha | НЕТ | 1 |
| POLY_PAGE_SMOKECLOUD2 | SrcAlpha | InvSrcAlpha | НЕТ | 1 |
| POLY_PAGE_SMOKER | SrcAlpha | One | ДА | 2 |

### Текстурные флаги — откуда берутся

`POLY_page_flag[]` (массив UWORD, 1519 элементов) загружается из **трёх файлов** `textype.txt`:
1. `<world_dir>/textype.txt` — текстуры мира (offset 0)
2. `<shared_dir>/textype.txt` — общие текстуры (offset 0)
3. `server/textures/shared/prims/textype.txt` — текстуры примов (offset 704 = 11*64)

Формат: `Page N: <flags>`, где флаги — буквы (T=transparent/alpha-test, M=alpha-blend, A=add_alpha, и т.д.)

### Пример: прим-меш → page mapping

В `MESH_draw_guts` (mesh.cpp:305-308):
```cpp
page = (p_f4->UV[0][0] & 0xc0) << 2 | p_f4->TexturePage;
page += FACE_PAGE_OFFSET;  // +512
```
10-бит page index (0-1023) + offset 512 = финальный диапазон 512-1535.

Флаги для этих страниц берутся из `prims/textype.txt` (загружается с offset 704 = 11*64).
Если у текстуры куста/дерева флаг `M` (ALPHA) → `DepthWrite=false` → sorted pass sub-phase 1.
Если флаг `T` (TRANSPARENT) → alpha test only, `DepthWrite=true` → **opaque pass**.

## Возможные направления фикса

### A. Увеличить число бакетов (4096, 8192, 16384)
Уменьшит коллизии. Простое изменение — одна константа. Но не решает проблему полностью при очень близких объектах.

### B. Сортировка внутри бакета по средней Z
Вместо LIFO внутри бакета — дополнительная сортировка. Повышает точность за счёт CPU. Можно сортировать только бакеты с >1 полигоном.

### C. Использовать average Z вместо max Z для sort key
`sort_z = average(pts->Z)` вместо `max(pts->Z)`. Более честная оценка глубины для больших полигонов. Простое изменение в polypage.cpp.

### D. Комбинация A+C
Увеличить бакеты + average Z — два простых изменения, покрывают большинство случаев.

### E. Order-independent transparency (OIT)
Тяжёлая артиллерия (weighted blended OIT или linked-list OIT). Решает всё, но требует значительной переработки шейдеров и pipeline. Оставить как Plan B.

---

## Ключевые файлы

| Файл | Что содержит |
|------|-------------|
| `new_game/src/engine/graphics/pipeline/aeng.cpp` | Главный render loop (`AENG_draw_city`, line 3713) |
| `new_game/src/engine/graphics/pipeline/poly.cpp` | `POLY_frame_draw` (line 1879), bucket sort, add_quad/add_triangle |
| `new_game/src/engine/graphics/pipeline/polypage.cpp` | `PolyPage::Render`, sort_z вычисление (line 215) |
| `new_game/src/engine/graphics/pipeline/poly_render.cpp` | Инициализация RenderState для всех PolyPage типов |
| `new_game/src/engine/graphics/core/game_graphics_engine.h` | `NeedsSorting()`, `IsAdditiveBlend()` (lines 641, 645) |
| `new_game/src/engine/graphics/core/core.cpp` | `ge_draw_indexed_primitive_vb`, GL draw calls |
| `new_game/src/engine/graphics/pipeline/figure.cpp` | Immediate draw персонажей (line 2131) |

---

## Применённый фикс: замена bucket sort на полную сортировку (2026-04-11)

### Решение

Bucket sort (2048 бакетов, LIFO внутри) заменён на полную сортировку `std::sort` всех sorted полигонов по `sort_z`.

### Изменённые файлы

**polypage.h** — `AddToBuckets` заменён на `CollectForSort(PolyPoly** sort_array, int& count, int max_count)`.

**polypage.cpp** — `CollectForSort` собирает полигоны страницы в плоский массив (вместо распределения по бакетам). Сохраняет `MassageVertices()` и `ge_vb_prepare()`.

**poly.cpp** — sorted pass в `POLY_frame_draw`:
- Было: `PolyPoly* buckets[2048]` + `AddToBuckets` + итерация linked list по бакетам
- Стало: `static PolyPoly* sort_array[16384]` + `CollectForSort` + `std::sort` по `sort_z` ascending (back-to-front)
- Двухфазный проход (non-additive → additive) **сохранён**
- Batched rendering (IxBuffer) **сохранён**

### Почему это лучше

1. **Точная сортировка** — каждый полигон на своём месте, нет коллизий бакетов
2. **Нет LIFO артефактов** — порядок не зависит от gamut traversal
3. **Производительность** — `std::sort` на 200-500 указателях ≈ микросекунды. Static массив 16384 — потолок с запасом
4. **Проще код** — убраны бакеты, linked list, NULL-инициализация 2048 элементов

### Что НЕ меняется

- sort_z по-прежнему = max(Z) вершин (можно сменить на average позже если нужно)
- Двухфазный проход (non-additive → additive) сохранён
- Opaque pass не затронут
- Depth test ReadOnly для sorted pass — без изменений

### Результаты визуальной проверки (2026-04-11, после замены bucket sort)

**Target UC — ёлки (4 грани крест-накрест):**
- ✅ Проблема пропала — грани сортируются корректно при вращении камеры.

**Target UC — гейзеры vs ёлки:**
- 🔄 Гейзеры теперь рисуются ПОВЕРХ ёлок (раньше наоборот). Но спрайты самого гейзера z-файтятся друг с другом — постоянно пересортировываются туда-сюда, мерцание.
- Вероятная причина z-fighting: спрайты гейзера на почти одинаковой глубине, `std::sort` нестабилен — при незначительных изменениях sort_z между кадрами порядок прыгает. С bucket sort это маскировалось — все были в одном бакете с фиксированным LIFO порядком.
- Возможные решения: (a) stable_sort, (b) вторичный ключ сортировки (page index или poly index) для стабильного порядка при равных sort_z.

**Target UC — кусты:**
- ❌ Проблема не ушла — тоже что-то нездоровое с порядком.

**Psycho Park — зелёная зона:**
- ✅ Разные кусты друг относительно друга — стали ок.
- ✅ Лавочка и зелёная штука вокруг дерева — ок.
- ❌ Кусты конфликтуют сами с собой — грани внутри одного куста перекрывают друг друга неправильно. Тот же класс проблем что ёлки раньше, но у ёлок пофиксилось (4 грани на разной глубине), а у кустов грани видимо слишком близко.
- ❌ Куст рисуется поверх лавочки, хотя куст дальше. Лавочка тоже полупрозрачная (видны полупрозрачные текстурки). Вероятно sort_z = max(Z) лавочки меньше чем sort_z куста — куст сортируется ближе и рисуется последним, перекрывая лавочку. Возможно max(Z) vs average(Z) проблема.
- ❌ Лавочка конфликтует сама с собой — разные грани рисуются в неправильном порядке (тот же класс что кушетки).
- ❌ Огонь рисуется поверх лавочки, хотя лавочка перед огнём. При этом непрозрачный объект (ящик) рядом корректно перекрывает огонь. **Причина:** лавочка в sorted pass sub-phase 1 (non-additive, ALPHA), огонь в sub-phase 2 (additive). Sub-phase 2 ВСЕГДА рисуется ПОСЛЕ sub-phase 1 — огонь всегда поверх ВСЕЙ non-additive полупрозрачной геометрии, **независимо от глубины**. Ящик — opaque → пишет depth → depth test отсекает огонь за ним. Лавочка — alpha → НЕ пишет depth → depth test для огня проходит.
  - **Это фундаментальная проблема двухфазного подхода**: sub-phase split предполагает что additive эффекты всегда поверх non-additive (для glow-through-foliage). Но это неправильно когда огонь ПОЗАДИ полупрозрачного объекта.

**Estate of Emergency — лежаки (кушетки):**
- ❌ Неясно ушла ли исходная проблема, но появилась новая: внутренние грани/полигоны самой кушетки конфликтуют друг с другом, рисуются в неправильном порядке. Скорее всего вся кушетка помечена как полупрозрачная (один page с ALPHA флагом), потому что у неё есть полупрозрачные куски (видимо одним квадом с остальной геометрией). Из-за этого ВСЯ геометрия кушетки попадает в sorted pass без depth write — и грани внутри одного объекта конкурируют в сортировке.

**The Fallen — вывеска Hot Dawg:**
- ✅ Проблема пропала.

**Rat Catch / Assassin (одна карта):**
- ✅ Труба вдалеке больше не перекрывает пар.
- ❌ Кровь мёртвого мента перекрывает пар, а сам труп — нет. Кровь (BLOODSPLAT) в sub-phase 1, пар (STEAM) в sub-phase 2. Пар должен быть поверх крови по логике фаз — но визуально кровь поверх. Требует отдельного разбора.

**Assassin — деревья и провода:**
- ❌ Провода по-прежнему перекрывают деревья. Возможно большой спрайт или другая система рендеринга.
- ❌ Дальнее дерево (тёмный силуэт в тумане) перекрывает ближнее. Листья деревьев конфликтуют внутри одного дерева.

**Cop Killers — свет мины:**
- ❌ Свет мины до сих пор перекрывает знак (большая дистанция между ними). Если знак opaque → depth test должен отсечь. Возможно знак alpha, или спрайт мины позиционирован ближе чем кажется.

**Target UC — ёлки (дополнительно):**
- ❌ Дальние деревья перекрывают ближние (скриншот).

**Target UC — мины:**
- ❌ Свет мин сквозь стенки здания — осталось. Тот же класс что Cop Killers.

---

## Итоговые выводы после первого раунда тестирования (2026-04-11)

### Что исправила замена bucket sort → std::sort

| Было | Стало | Причина |
|------|-------|---------|
| Грани ёлок (4 крест-накрест) в неправильном порядке | ✅ | Грани на разной глубине, точная сортировка разрешила |
| Разные кусты друг поверх друга | ✅ | Были в одном бакете, теперь сортируются по Z |
| Лавочка и зелень вокруг дерева | ✅ | Аналогично |
| Вывеска Hot Dawg — текст сзади просвечивал | ✅ | Аналогично |
| Труба поверх пара | ✅ | Аналогично |

**Вывод:** полная сортировка решила все случаи где разные объекты на разной глубине были в одном бакете.

### Оставшиеся проблемы — 4 класса

#### Класс 1: Грани внутри одного объекта конфликтуют (НОВАЯ/ОБОСТРЁННАЯ проблема)

**Затронуты:** кусты (Psycho Park, Target UC), лавочки (Psycho Park), кушетки (Estate of Emergency), деревья-листва (Assassin), гейзеры (Target UC — мерцание спрайтов).

**Причина:** все грани объекта с ALPHA-текстурой попадают в sorted pass с `DepthWrite=false`. Грани на почти одинаковой глубине → порядок сортировки нестабилен. С bucket sort это маскировалось — все грани в одном бакете, LIFO давал фиксированный (хоть и неправильный) порядок. Теперь `std::sort` нестабилен → порядок прыгает между кадрами.

**Ключевое наблюдение:** проблема в том что ВСЯ геометрия объекта (включая непрозрачные грани) попадает в sorted pass без depth write, потому что page помечен как ALPHA. Один page = одна текстура = один набор render states. Если текстура содержит и полупрозрачные и непрозрачные участки — всё рисуется без depth write.

**Возможные решения:**
- (a) `std::stable_sort` — стабилизирует порядок при равных Z, убирает мерцание. Не решает неправильный порядок, но убирает визуальный шум.
- (b) Per-object face sorting — сортировать грани mesh'а по глубине от камеры перед добавлением в pipeline.
- (c) Alpha test вместо alpha blend где текстура бинарный cutout — `DepthWrite=true`, opaque pass.

#### Класс 2: Двухфазный split (non-additive → additive) ломает глубину

**Затронуты:** огонь поверх лавочки (Psycho Park).

**Причина:** sub-phase 2 (additive: огонь) ВСЕГДА рисуется ПОСЛЕ sub-phase 1 (non-additive: лавочка), **независимо от глубины**. Лавочка alpha → не пишет depth → depth test для огня проходит → огонь поверх. Ящик рядом — opaque → пишет depth → огонь за ним отсечён.

**Возможные решения:**
- (a) Убрать двухфазный split — все sorted полигоны в одном проходе по Z. Потеряется "glow through foliage". Нужно проверить был ли он в оригинале.
- (b) Depth-only pass после sub-phase 1 — записать depth от non-additive полигонов, чтобы additive позади них отсекались.

#### Класс 3: Дальние деревья перекрывают ближние

**Затронуты:** Assassin (дерево-силуэт в тумане), Target UC (ёлки).

**Возможная причина:** `sort_z = max(Z)` — ближайшая вершина большого полигона дальнего дерева может дать sort_z ближе чем у ближнего. Или проблема alpha/opaque классификации.

**Возможные решения:**
- (a) `sort_z = average(Z)` вместо `max(Z)`.
- (b) Разобраться: это сортировка или классификация?

#### Класс 4: Additive эффекты сквозь OPAQUE объекты

**Затронуты:** свет мин сквозь стенки здания (Target UC), свет мины сквозь знак (Cop Killers).

**Это НЕ проблема сортировки.** Если стена/знак opaque → depth test должен отсечь additive спрайт. Если не отсекает — либо объект НЕ opaque, либо спрайт позиционирован ближе к камере, либо баг в depth value. **Требует отдельного расследования.**

### Попытка фикса average(Z) — откачена

Заменил `sort_z = max(Z)` на `sort_z = average(Z)` в `AddFan`. Откатил — теория была ложной: деревья на километр друг от друга, разница в Z огромная, sort key тут ни при чём. Нужно отдельное расследование (см. ниже).

---

## ТЕКУЩЕЕ РАССЛЕДОВАНИЕ: дальние деревья поверх ближних (Класс 3)

**Симптомы:** дальнее дерево (тёмный силуэт в тумане, ~километр) рисуется поверх ближнего дерева (прямо перед камерой). Видно на Assassin и Target UC.

**Что уже исключено:**
- sort_z precision — разница в Z между деревьями огромная, сортировка однозначна
- max(Z) vs average(Z) — не объясняет при такой разнице

**Гипотезы для проверки:**
1. Деревья в разных render pass'ах (одно opaque, другое sorted) — но depth test должен работать
2. Одно из деревьев рисуется через другую систему (не PolyPage pipeline)
3. Баг в Z-значении для далёких объектов
4. Далёкое дерево — не прим, а facet (стена здания?) с текстурой дерева

**Нужно:** понять через какой именно render path рисуются оба дерева, какие у них page flags, и почему depth test не отсекает дальнее.

---

## Отложенные задачи (после расследования Класса 3)

### Класс 1: Грани внутри одного объекта конфликтуют
- **Проблема:** `std::sort` нестабилен → грани на одной глубине мерцают (было LIFO в бакете — стабильно но неправильно, теперь нестабильно)
- **Затронуты:** кусты, лавочки, кушетки, листва деревьев, гейзеры
- **TODO 1a:** попробовать `std::stable_sort` — убрать мерцание (stable_sort сохраняет порядок элементов с равными ключами → если грани добавлялись в определённом порядке, он сохранится)
- **TODO 1b:** проверить текстуры кустов/лавочек — если бинарная alpha (0/255) → можно сменить флаг с M (ALPHA, DepthWrite=false) на T (TRANSPARENT, alpha test, DepthWrite=true) → opaque pass, depth write решает порядок граней
- **TODO 1c:** per-object face sorting — сортировать грани mesh'а по глубине перед добавлением в pipeline (если 1a/1b недостаточно)

### Класс 2: Двухфазный split (non-additive → additive) ломает глубину
- **Проблема:** огонь (additive, sub-phase 2) ВСЕГДА поверх лавочки (non-additive alpha, sub-phase 1), даже если огонь позади
- **TODO 2a:** проверить в оригинальной PC-версии — есть ли glow-through-foliage эффект (bloom сквозь крону)
- **TODO 2b:** если glow-through-foliage не нужен → убрать двухфазный split, все sorted в одном проходе по Z
- **TODO 2c:** если нужен → depth-only pass после sub-phase 1 (записать depth от non-additive, чтобы additive позади отсекался)

### Класс 4: Additive эффекты сквозь OPAQUE объекты
- **Проблема:** свет мин сквозь стенки здания (Target UC), сквозь знак (Cop Killers)
- **TODO 4a:** проверить page flags стены/знака — opaque или alpha?
- **TODO 4b:** если opaque → depth test должен отсекать → баг в depth value или позиционировании спрайта
- **TODO 4c:** если alpha → та же проблема что Класс 2

### Нерасследованное
- Кровь мента перекрывает пар (Rat Catch) — кровь sub-phase 1, пар sub-phase 2, пар должен быть поверх. Визуально наоборот. Разобраться.
- Провода перекрывают деревья (Assassin) — возможно другая система рендеринга или большой спрайт. Разобраться.
