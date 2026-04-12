# Z-Order / Draw Order Investigation

**Дата:** 2026-04-11 — 2026-04-12
**Статус:** завершено (основные проблемы решены)

## Проблема

Множество визуальных багов с порядком отрисовки полупрозрачных объектов: кусты, лежаки, вывески, провода, пар, огонь, деревья — рисовались в неправильном порядке на разных миссиях.

---

## Архитектура рендер-pipeline (справка)

### Порядок отрисовки в AENG_draw_city()

```
1.  Небо (POLY_PAGE_SKY)
2.  Земля (terrain)
3.  Тени (POLY_PAGE_SHADOW, DepthBias=8)
4.  Отражения в лужах / Drips → flush
5.  Лужи → flush
6.  Далёкие здания (far facets)
7.  Полы
8.  Примы (мебель, лавочки, провода, вывески, bloom) → PolyPages
9.  Стены зданий (facets) → PolyPages
10. Персонажи → immediate draw (ge_draw_multi_matrix, в обход PolyPage)
11. Дождь, пиротехника, torch, messages
12. Dirt (листья, мусор, снежинки)
13. *** POLY_frame_draw *** — ГЛАВНЫЙ FLUSH
```

### POLY_frame_draw: два прохода

**Opaque pass:** все страницы с `DepthWrite=true` (`NeedsSorting()=false`) — рисуются в порядке PageOrder. Z-buffer разрешает перекрытие.

**Sorted pass:** все страницы с `DepthWrite=false` (`NeedsSorting()=true`) — полупрозрачные объекты. Рисуются back-to-front по sort_z. Depth test ReadOnly (тестируются против opaque depth, но не пишут).

### Какие объекты куда попадают

| Флаг текстуры | DepthWrite | Pass | Описание |
|---------------|-----------|------|----------|
| нет флага | true | opaque | Обычная геометрия |
| T (TRANSPARENT) | true | opaque | Alpha test cutout (заборы, решётки) |
| M (ALPHA) | false | sorted | Alpha blend (кроны, кусты, стёкла) |
| A (ADD_ALPHA) | false | sorted | Additive blend (bloom, огонь) |

Флаги загружаются из `textype.txt` файлов при старте уровня.

---

## Найденные проблемы и фиксы

### Проблема 1: sort_z не записывался в 3 из 4 code paths (КОРНЕВОЙ БАГ)

**Симптом:** полупрозрачные объекты рисовались в произвольном порядке — дальние поверх ближних, мерцание.

**Причина:** `PolyBufAlloc()` вызывается в 4 местах, но `sort_z` записывался только в `AddFan` (polypage.cpp). Три других пути оставляли sort_z неинициализированным:
1. `POLY_add_nearclipped_triangle` (near-clip fan triangulation)
2. `POLY_add_triangle_fast`
3. `POLY_add_quad_fast` (два PolyPoly на quad)

sort_z содержал мусор `0xBEBEBEBE` (-0.37254900 как float). С bucket sort мусорный sort_z попадал в bucket 0 (clamp отрицательных), LIFO давал стабильный (хоть и произвольный) порядок. Проблема была скрыта.

**Как нашли:** debug depth shade (шейдер затемнял по v_view_z) показал что глубина в вершинах правильная, а sort_z — нет. Лог sort_z в CollectForSort показал -0.37 у всех 720 полигонов. Лог в AddFan (проверка отрицательных) НЕ сработал → sort_z правильный в AddFan. Значит полигоны попадают в pipeline НЕ через AddFan. Grep по PolyBufAlloc нашёл 3 дополнительных вызова без записи sort_z.

**Фикс:** добавлен `sort_z = average(Z)` во все три code paths.

### Проблема 2: bucket sort — грубая сортировка

**Симптом:** объекты на похожей глубине рисовались в произвольном порядке.

**Причина:** оригинальный bucket sort (2048 бакетов) давал приблизительную сортировку. Внутри одного бакета порядок — LIFO (зависит от gamut traversal, не от глубины). Объекты на близкой глубине попадали в один бакет → произвольный порядок.

**Фикс:** bucket sort заменён на `std::stable_sort` по sort_z. Точная сортировка каждого полигона. stable_sort сохраняет порядок добавления при равных sort_z → нет мерцания.

### Проблема 3: sort_z = max(Z) — z-fighting на mesh'ах с общей вершиной

**Симптом:** грани ёлок (конусообразный mesh, все треугольники сходятся в вершине) мерцали при высоком угле камеры.

**Причина:** `sort_z = max(Z)` брал ближайшую к камере вершину. Когда камера сверху — общая вершина ёлки ближе всех к камере для ВСЕХ треугольников → одинаковый sort_z → нестабильный порядок.

**Что пробовали:** `(average + max) / 2` — хуже (возвращает часть проблемы max(Z)).

**Фикс:** `sort_z = average(Z)` — среднее по всем вершинам. У разных треугольников разные базовые вершины → разное среднее → стабильная сортировка.

### Проблема 4: двухфазный split (non-additive → additive)

**Симптом:** огонь/bloom всегда рисовался поверх полупрозрачных объектов (лавочек, кустов), даже когда огонь позади.

**Причина:** sorted pass был разделён на две фазы: сначала все non-additive (кроны, заборы), потом все additive (огонь, bloom). Additive всегда поверх non-additive, независимо от глубины. Это был костыль из предыдущего фикса (leaf_overdraw_investigation.md) для компенсации отсутствия нормальной сортировки в bucket sort.

**Фикс:** двухфазный split убран. Все sorted полигоны (additive и non-additive) в одной очереди, сортируются вместе по Z.

---

## Итоговые результаты проверки

**Исправлено:**
- ✅ Estate of Emergency — лежаки не перекрывают друг друга
- ✅ Psycho Park — лавочка и огонь ок, кусты между собой ок, зелёная зона ок
- ✅ The Fallen — Hot Dawg ок
- ✅ Assassin — деревья, провода, пар ок
- ✅ Rat Catch — труба и пар ок
- ✅ Target UC — ёлки между собой ок, гейзеры ок
- ✅ Cop Killers — мины через знак ок

**Остаётся (отдельные баги, записаны в known_issues):**
- Тень лежака поверх лежака (Estate of Emergency) — баг оригинала, есть в релизе
- Свет мин сквозь стенки зданий (Target UC) — отдельный баг, не z-order сортировки
- Z-fighting листьев и газеток на земле — одинаковая глубина, нужен bias

---

## Итоговые изменения кода

**poly.cpp:**
- `#include <algorithm>` для std::stable_sort
- `POLY_add_nearclipped_triangle` — добавлен `sort_z = average(Z)`
- `POLY_add_triangle_fast` — добавлен `sort_z = average(Z)`
- `POLY_add_quad_fast` — добавлен `sort_z = average(Z)` для обоих треугольников
- `POLY_frame_draw` — bucket sort → std::stable_sort, двухфазный split убран, одна очередь

**polypage.cpp:**
- `AddFan` — sort_z = average(Z) вместо max(Z)
- `AddToBuckets` → `CollectForSort` — собирает в плоский массив вместо бакетов

**polypage.h:**
- `AddToBuckets` → `CollectForSort`

---

## Ключевые файлы

| Файл | Что содержит |
|------|-------------|
| `engine/graphics/pipeline/poly.cpp` | `POLY_frame_draw` (sorted pass), `POLY_add_*_fast` (sort_z fix) |
| `engine/graphics/pipeline/polypage.cpp` | `AddFan` (sort_z = average), `CollectForSort` |
| `engine/graphics/pipeline/poly_render.cpp` | Инициализация RenderState для всех PolyPage типов |
| `engine/graphics/pipeline/aeng.cpp` | Главный render loop `AENG_draw_city` |
| `engine/graphics/graphics_engine/game_graphics_engine.h` | `NeedsSorting()`, `IsAdditiveBlend()` |

## Связанные расследования

- [stage5_bug_zbuffer_mismatch.md](stage5_bug_zbuffer_mismatch.md) — z-buffer mismatch персонажей (ранее пофикшен)
- [leaf_overdraw_investigation.md](leaf_overdraw_investigation.md) — листья поверх additive эффектов (ранее пофикшен, двухфазный split оттуда убран в этом расследовании)
