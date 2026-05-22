# Stage 13 — Оптимизации рендерера

Перф-улучшения которые **не блокируют 1.0** — игра уже работает на целевом
железе. Сюда собираются вещи которые требуют серьёзной переделки конвейера
и/или дают выигрыш в основном на слабом железе (Steam Deck, интегрированная
графика, старые ноуты). Каждый пункт — самодостаточное описание задачи без
зависимости от других документов.

---

## Draw call batching на уровне PolyPage

Сейчас каждый PolyPage slot делает отдельный draw call со своим VBO/EBO/VAO
(per-slot, как D3D6 оригинал). Батчинг нескольких draw calls в один большой
VBO с одним `glDrawElements` уменьшит количество draw calls с сотен до
единиц за кадр.

**Что надо:**
- Общий VBO с sub-allocation, offset-based indexing.
- Переписать `ge_draw_indexed_primitive_*` API под аккумуляцию вершин в
  общий буфер вместо per-slot flush'а.
- Учесть смены состояния (blend, cull, texture address) — батчить можно
  только последовательные draw calls с **одинаковым** state. Меняется
  state — flush, начать новый батч.

**Замер:** ожидаемый выигрыш — десятки fps на слабом GPU где driver overhead
на draw call велик. На сильном GPU разницы почти не будет.

---

## GPU-transform для 3D геометрии

Перевести всю трёхмерную отрисовку (мир, объекты, эффекты — **кроме**
персонажей и UI) на аппаратные трансформации вершин через шейдеры, вместо
CPU-transform.

Сейчас `ge_draw_indexed_primitive_lit` делает CPU-transform (`World ×
Projection`), результат рисуется как TL (pre-transformed) вершины. Это
исторический подход из D3D6 эпохи — вершинный шейдер `tl_vert.glsl`
просто пробрасывает вершину без матриц.

Персонажи уже на GPU-трансформе (после Phase 2 скиннинга — `skin_world_vert.glsl`).
Здесь речь про остальную 3D геометрию: статичные меши (`MESH_draw_poly`),
эффекты (sprite billboards, particles), и т.п.

**Что надо:**
- Реанимировать `lit_vert.glsl` (сейчас не работает из-за несовместимости
  D3D vs GL clip-space конвенций) — переработать матрицы, viewport transform,
  depth mapping.
- Перевести вызовы `ge_draw_indexed_primitive_lit` на новый путь.
- **НЕ трогать** UI/HUD — пусть остаются на TL path (там CPU-transform
  оправдан, объём вершин минимальный).
- **НЕ трогать** POLY pipeline целиком — он deferred, его trade-off
  отдельный.

**Замер:** разгрузка CPU на сцене со множеством статичной геометрии (RTA с
машинами, Urban Shakedown). Не критично на сильном CPU, ощутимо на Steam Deck.

**Сложность:** средняя/высокая — clip-space gotchas обязательно вылезут,
нужна тщательная сверка 1:1 со старым путём.

---

## Texture array / atlas для персонажей (один draw call на персонажа)

Сейчас рендер тела персонажа = **N draw calls** (N = количество материалов
тела, обычно 3–5). На сцене с десятком NPC это десятки лишних draw calls.

**Цель:** **один draw call на персонажа** через `GL_TEXTURE_2D_ARRAY` (или
атлас) — все материалы тела как слои/под-ректы одной GL-текстуры,
per-vertex layer/UV-offset выбирает нужный.

### Текущее состояние

- Геометрия персонажа уже консолидирована в **один VBO** per (chunk, MeshID)
  — после Фазы 2 скиннинга. Содержит все вершины всех 15 частей тела в
  bind-space, с multi-bone weights.
- Per-material slice индексов хранится в `skin_consolidated_ranges[]` —
  массив `(index_start, index_count)` пар на каждый материал.
- Draw-loop в `figure.cpp` (`FIGURE_draw_hierarchical_prim_recurse`)
  итерирует материалы, на каждом:
  - резолвит `wRealPage` из `pMat->wTexturePage` (см. ниже про JACKET/OFFSET);
  - выбирает `fadeTable` (Tint vs Normal по `TEXTURE_PAGE_FLAG_TINT`);
  - вызывает `ge_skin_world_draw_range(mesh, start, count, ...)` со срезом
    индексов и нужной текстурой.

То есть **геометрия общая, отдельные draw'ы только из-за смены текстуры**.

### Что мешает «просто слить»

1. **Размеры текстур разные.** Для `GL_TEXTURE_2D_ARRAY` все слои = один
   размер. Padding меньших до размера наибольшего → перерасход памяти.
   Альтернатива — атлас (упаковка прямоугольников), но это другой алгоритм
   и UV-маппинг сложнее.

2. **Runtime-резолв текстур.** Часть материалов помечены флагами
   `TEXTURE_PAGE_FLAG_JACKET` / `TEXTURE_PAGE_FLAG_OFFSET` — фактическая
   страница вычисляется в runtime по полю `skill` персонажа (для JACKET) или
   глобальному `tex_page_offset` (для OFFSET). Значит набор страниц
   персонажа зависит от конкретного экземпляра, не только от
   `(chunk, MeshID)`. Кэш массивов нельзя ключевать просто по
   `(chunk, MeshID)` — нужна композитная ключёвка `(chunk, MeshID,
   resolved_page_set)` с LRU-эвикшеном.

3. **Per-material `fadeTable`.** Сейчас Tint и Normal — две таблицы
   передаются как uniform на per-material основе. С 1-draw подходом надо
   либо передавать **обе** и per-vertex флагом выбирать, либо мерджить
   в одну с дополнительной координатой индексации.

4. **PolyPage state.** Сейчас на каждый материал выставляется state через
   PolyPage по `wRealPage` (`Cull`, `Blend`, `TextureBlend`). Для 1-draw
   нужен **один** PolyPage state на персонажа. Все материалы тела по факту
   используют одинаковый state (`Cull=CCW / Blend=false /
   TextureBlend=ModulateAlpha`) — можно зафиксировать на стороне draw API.

5. **Прозрачные части тела (`FIGURE_alpha`).** Если у персонажа есть
   alpha-blended материал, его нельзя смержить в один draw с непрозрачными
   — порядок сортировки сломается. Решение: отдельный alpha-pass (второй
   draw на персонажа). В первой версии можно оставить fallback на
   per-material loop для таких случаев.

### Расширение формата вершины

`GESkinVertex` дописать:
- `uint8_t layer_index` — индекс слоя в texture array.
- `uint8_t tint_flag` — 0 = normal fadeTable, 1 = Tint fadeTable.

При сборке консолидированного меша (`figure_build_consolidated_skin_world`)
заполнять эти поля per-vertex на основе исходного материала вершины.

### Кэш array'ев

Новый кэш `texture_array_cache[]` (LRU, ёмкость ~16-32 записей):
- **Ключ:** хеш от массива resolved page IDs (после применения JACKET/OFFSET).
- **Значение:** GL texture array handle + mapping `material_index → layer`.
- **Создание:** лениво при первом draw character'а с этим resolved page set.
- **Размер layers:** максимум W×H по всем входящим текстурам, остальные
  паддятся (заливка transparent / repeat по краю — на тюнинг).

### Шейдер

- `sampler2D u_texture` → `sampler2DArray u_texture_array`.
- В вершинном шейдере: пробросить `a_layer_index` во фрагмент как `flat int`.
- Во фрагментном: `texture(u_texture_array, vec3(v_uv, v_layer))`.
- Tint flag: добавить `uniform uint u_fade_table_normal[64]` и
  `u_fade_table_tint[64]`, во фрагменте выбирать по per-vertex флагу.

### Этапность

| Шаг | Что делать |
|-----|-----------|
| 1 | API `ge_texture_array_*` (создание / upload / destroy `GL_TEXTURE_2D_ARRAY`). Изолированно, без интеграции с figure. |
| 2 | Расширение `GESkinVertex` под `layer_index` + `tint_flag`. Заполнение в `figure_build_consolidated_skin_world`. Шейдер пока игнорирует. Гейт: визуально 1:1, не сломали ничего. |
| 3 | LRU-кэш `texture_array_by_resolved_pages[]`. Сборка array при первом draw с уникальным resolved page set. |
| 4 | Шейдер `skin_world_vert.glsl` → sampler2DArray + per-vertex layer. `ge_skin_world_draw_range` принимает array handle вместо PolyPage. |
| 5 | Per-vertex `tint_flag` → шейдер выбирает fadeTable из двух. |
| 6 | Replace per-material loop в figure.cpp + smap.cpp + figure.cpp reflection на **один** `ge_skin_world_draw` вызов на персонажа. Fallback на per-material loop для alpha-blended случаев. |
| 7 | Замер: число draw call'ов до/после, перф на RTA / Urban Shakedown. Подписание визуально 1:1 на ключевых сценах. |

### Что НЕ в скоупе первой итерации

- Атлас (packed atlas) вместо array — атлас даёт лучшую упаковку памяти для
  разных размеров, но сложнее (rectangle packing + UV per-vertex offset).
  Array проще и достаточен для людей UC (текстуры тела достаточно
  однородны по размерам).
- `FIGURE_alpha` — fallback на старый путь для alpha-blended персонажей.
- Не-люди (Балрог, Bane, etc.) — у них меньше материалов на персонажа,
  оптимизация менее ценная. Сделать вторым проходом если будет нужно.

### Гейт

- **Визуально 1:1** со старым путём на: RTA intro (много NPC + Дарси с
  оружием), Urban Shakedown (близкий план, скилл-варианты курток), ночь /
  туман (проверка fadeTable per-vertex), сцена с прозрачным персонажем
  (fallback path).
- **Перф:** число draw call'ов на персонажа упало с 3–5 до 1. На
  тяжёлой сцене (10+ NPC одновременно) общий выигрыш десятки draw calls
  → измеримый прирост FPS на слабом GPU.

---

## Увеличение дистанции отрисовки и тумана

Отодвинуть границу тумана дальше от игрока — современное железо легко
потянет, текущие значения — наследие 1999 года. Когда это будет сделано,
нужно подтянуть связанные дистанции, иначе артефакты вылезут на новой
увеличенной дальности:

- **Дистанция детальных теней** — константа `AENG_SHADOW_MAX_DIST` в
  `aeng.cpp`. Определена прямо перед циклами выбора теней (grep
  `AENG_SHADOW_MAX_DIST`). Сейчас `0xC0000` (удвоено относительно
  оригинального `0x60000`). После расширения тумана — поднять
  соответственно. Замечание: количество одновременных детальных теней
  ограничено `AENG_NUM_SHADOWS = 4` (ограничение по shadow texture 64×64 =
  2×2 слота) — см. коммент там же.
- **Bump mapping (crinkle) fade range** — константы `CRINKLE_FADE_NEAR` /
  `CRINKLE_FADE_FAR` в `facet.cpp` (вверху файла, grep `CRINKLE_FADE_NEAR`).
  Сейчас `[0.3, 0.8]` по post-transform Z. Используются через inline-хелпер
  `crinkle_extrude_for_z()` в трёх местах: `FillFacetPoints`,
  `FillFacetPointsCommon`, `FACET_project_crinkled_shadow`. После
  расширения тумана — подвинуть FAR ближе к 1.0 (или выше, если Z может
  превышать 1.0 для видимых полигонов — проверить).
- **Прочие дистанции, которые могут понадобиться:** `AENG_DRAW_DIST`,
  `AENG_DRAW_PEOPLE_DIST`, дистанции farfacet'ов — проверить при
  расширении тумана.
