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

## Следующие шаги

1. Сверить с релизной PC-версией: какие из этих багов присутствуют в оригинале, а какие внесены нами
2. Попробовать вариант D (увеличить бакеты + average Z) — минимальные изменения, максимальный эффект
3. Если недостаточно — сортировка внутри бакета (вариант B)
4. Проверить конкретные текстуры примов (лавочки, кусты) — какие флаги у их pages
