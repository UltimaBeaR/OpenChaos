# Ext-objects fix: рендер высоких объектов когда anchor cell вне гамута

## Проблема

В оригинальной системе рендера каждый статический объект (`OB_Ob` в `OB_mapwho`) приписан к одной lo-cell — anchor cell. Per-frame отрисовка идёт через основной PRIM-loop в `AENG_draw_city`: render-loop итерирует **только** клетки попавшие в `NGAMUT_lo_gamut` (2D-проекция camera frustum'а на плоскость земли) и для каждой клетки рисует объекты из её `OB_mapwho`.

**Симптом:** объекты висящие в воздухе (провода с гирляндами PRIM 215, баннеры, тросы между крышами зданий) пропадают при определённых ракурсах камеры. Хотя визуально объект полностью на экране, его anchor cell может быть **за пределами** гамута — потому что гамут это **2D ground projection**, а объект **высоко в воздухе**. Render-loop эту cell не посещает → объект не рисуется → исчезает.

Воспроизводилось:
- Любой городской уровень с проводами/гирляндами между зданиями. Стоишь рядом с проводом, поворачиваешь камеру так чтобы здание-якорь скрылось из гамута — гирлянда мигает/пропадает.
- Особенно заметно при беге по крышам зданий между которыми натянуты тросы — смотришь на трос сбоку, он рандомно исчезает.

## Решение: ext-system

При загрузке уровня для каждого статического объекта кэшируется параллельная запись в `g_ob_extended[]` — позиция, mesh AABB, центр после поворота, sphere radius, anchor cell. Каждый кадр перед PRIM-loop'ом запускается `OB_extended_pre_pass`:

- Если **anchor cell в гамуте** → `responsible_valid = 0`. Нормальный PRIM-loop посетит anchor cell и нарисует объект штатно. Ext не вмешивается.
- Если **anchor cell вне гамута** → сканируется весь гамут, выбирается **ближайшая** к anchor in-gamut cell (Chebyshev distance). Эта cell становится responsible. Объект пушится в её **per-frame guest list**.

Render-loop при заходе в cell дополнительно итерирует guest list этой cell. Для каждого guest — sphere-vs-frustum cull (6 плоскостей). Прошёл cull → ad-hoc per-vertex освещение (`NIGHT_light_prim` from scratch — кэш anchor cell может быть устаревшим если cell вне гамута) → `MESH_draw_poly`.

## Структуры данных

- **`OB_Extended`** ([ob.h](../new_game/src/map/ob.h)): cached per-object запись. Содержит prim, ob_idx, full world position, yaw, geometric center (после yaw rotation), `half_xz` (yaw-conservative), `min_wy/max_wy`, anchor lo coords, plus per-frame fields `responsible_lo_x/z` и `responsible_valid`.
- **`g_ob_extended[OB_MAX_EXTENDED]`** — параллельный массив на все статические OB. Размер == `OB_MAX_OBS` (2048) чтобы вместить всё.
- **`g_ob_ext_cell_first[lo_x][lo_z]`** — головы связных списков guest'ов per cell. Строится **каждый кадр заново** в pre-pass.
- **`g_ob_ext_refs[]`** — пул узлов связного списка.

## Алгоритм по шагам

**Loading time** (`OB_extended_build`, один раз на уровень):
1. Пройти `OB_mapwho`, для каждого OB вычислить mesh local AABB через `OB_extended_compute_local_bbox`.
2. Кэшировать в `g_ob_extended[ext_idx]`: world pos, mesh half_xz / min_wy / max_wy, yaw-rotated center, anchor lo cell.

**Per frame**:

1. **Гамут** считается рендером в начале кадра (`AENG_calc_gamut`) — для каждого z от zmin до zmax диапазон x [xmin..xmax] клеток на земле.

2. **Pre-pass** (`OB_extended_pre_pass`):
   - Сброс per-frame guest list (memset `g_ob_ext_cell_first`, refs_count = 0).
   - Для каждого `g_ob_extended[i]`: проверить anchor in gamut.
     - Да → `responsible_valid = 0`, не пушим никуда.
     - Нет → найти ближайшую in-gamut cell (Chebyshev), записать как responsible, push в её guest list.

3. **Render-loop** проходит **только клетки гамута**:
   ```
   for each (x, z) in gamut:
       (a) Normal PRIM-loop:
           OB_find(x, z) → объекты с anchor в этой cell
           Каждый: MESH_draw_poly с кэшированным освещением (NIGHT_cache)
       
       (b) Ext guest path:
           Идти по g_ob_ext_cell_first[x][z]
           Для каждого guest:
               sphere-vs-frustum cull (6 плоскостей)
               passed: NIGHT_light_prim ad-hoc + MESH_draw_poly
   ```

## Sphere-vs-frustum cull

Boundng sphere центрирована в AABB центре, radius = `sqrt(2·half_xz² + half_y²)` (диагональ коробки/2).

Координаты центра и радиуса переводятся в view-space через `POLY_cam_matrix`. Радиус домножается на max row magnitude матрицы (матрица содержит non-unit scale ~0.0002-0.0003 для приведения world coords к view-space диапазону [0..1]; без этого scale `r` остаётся в мировых единицах а `vz/vx/vy` в view, и все 6 проверок ломаются — давало 500+ ghost-draw'ов на кадр).

Тест по 6 плоскостям:
- **NEAR**: `vz + r < ZCLIP` (sphere целиком за near plane)
- **FAR**: `vz - r > 1.0` (sphere целиком за far plane)
- **LEFT/RIGHT**: `vx ± vz` vs `±r·√2` (диагональные плоскости 90°-FOV)
- **TOP/BOTTOM**: `vy ± vz` vs `±r·√2`

Если sphere полностью за хоть одной плоскостью — cull. Иначе — рисуем.

## Свойства

- **Не более 1 draw на объект за кадр.** Если anchor в гамуте → нормальный path, ext не трогает (responsible_valid=0). Если anchor вне → ext выбирает **одну** responsible cell, push один раз. Render-loop рисует один раз.
- **Никогда не убираем то что и так рисовалось.** Ext только добавляет — нормальный path работает как раньше.
- **Ext рисует с правильной геометрией** (свой full sphere cull через 6 плоскостей), не теряет visible объекты на straddle случаях которые ломали более простые версии cull'а.

## Почему лужи не через эту систему

Аналогичный баг для луж был решён [иначе](../new_game_planning/known_issues_and_bugs_resolved.md): цикл луж в `aeng.cpp` итерирует ВСЕ 128 z-строк карты с полным x-диапазоном (без гамута), а не-видимые лужи отсекаются на per-vertex `POLY_transform` внутри `PUDDLE_draw`.

Для **луж** этот brute-force подход дёшев потому что `PUDDLE_mapwho` маленький (~128 байт) и сам `PUDDLE_draw` дёшевый (пара polygon'ов, без per-vertex освещения). Per-frame cost ~1-2 нс.

Для **OB** это было бы дороже: один `MESH_draw_poly` = ~24 вершины × per-vertex transform + per-vertex `NIGHT_light_prim` (несколько тысяч ops). Brute-force обход 600+ ext-eligible объектов добавил бы ~0.5-1ms vs текущий ext system с cheap sphere cull (~1-3ms total). Поэтому для OB выгоднее cull до того как до тяжёлой части дошло, а не "иди по всему пусть per-face cull разбирается".

## Почему не через `POLY_sphere_visible`

В движке есть готовая `POLY_sphere_visible` ([poly.cpp:651](../new_game/src/engine/graphics/pipeline/poly.cpp#L651)). Не используем потому что:
1. **Нет проверки FAR plane** — пропускает все объекты за дальней плоскостью.
2. LEFT/RIGHT math упрощённый (без sqrt(2) factor), TOP/BOTTOM использует aspect compensation 1.4 — рассчитан под другой use case (where FAR is filtered by gamut/draw-distance elsewhere). В нашем случае ext path обходит гамут by design, так что FAR check критичен.

В render block лежит свой полный 6-plane тест с правильной математикой.

## Производительность

На Steam Deck-class hardware текущая система съедает **~1-3ms на кадр** в типичной городской сцене (~600-2000 OB на уровне):
- Pre-pass: ~0.1-0.5ms (итерация всего g_ob_extended[] + scan гамута для каждого anchor-out-of-gamut объекта).
- Sphere cull: ~0.05-0.1ms (cheap reject большинства).
- Реальные draws + false-positive draws (sphere консервативная, sphere ⊃ AABB): ~0.5-2ms.

Доминирует draw-cost, не cull. Будущие оптимизации см. блок-комментарий в [ob.h](../new_game/src/map/ob.h) над `OB_MAX_EXTENDED` (`r_world` cache, matrix scale once-per-frame, distance early-out).

## Ограничения / known issues

- **OB_remove не инвалидирует g_ob_extended[].** Если объект удаляется в runtime через `OB_remove()` (например `OB_convert_dustbins_to_barrels` при загрузке, либо ad-hoc), кэш остаётся — ext path продолжит рисовать объект пока не пересоберётся `OB_extended_build`. На практике `OB_convert_dustbins_to_barrels` срабатывает на load (до первого frame), runtime-удаление редкое. Если станет видимо — пересобирать ext-state после `OB_remove` или проверять `OB_ob[ob_idx].prim != 0` в pre-pass.
- **Sphere консервативная**: sphere radius > AABB diagonal/2 → некоторые видимые-в-AABB-но-не-в-mesh объекты проходят cull и платят полный draw cost (но не дают пикселей — внутренний per-face cull в `MESH_draw_poly` отбраковывает). Тратит CPU без визуального вреда.

## Файлы

- [ob.h](../new_game/src/map/ob.h) — типы `OB_Extended` / `OB_ExtRef`, пулы, futures-комментарий с оптимизациями.
- [ob.cpp](../new_game/src/map/ob.cpp) — `OB_extended_build` (build-time кэш), `OB_extended_pre_pass` (per-frame responsible cell + guest list rebuild).
- [ob_globals.cpp](../new_game/src/map/ob_globals.cpp) — определения глобалов (`g_ob_extended[]`, `g_ob_ext_cell_first[][]`, `g_ob_ext_refs[]`, etc.).
- [aeng.cpp](../new_game/src/engine/graphics/pipeline/aeng.cpp) — интеграция в `AENG_draw_city`: вызов pre-pass перед PRIM-loop, ext-guest render block внутри per-cell итерации с sphere cull.
- [elev.cpp](../new_game/src/assets/formats/elev.cpp) — вызов `OB_extended_build()` после `calc_slide_edges()` в level loader.
