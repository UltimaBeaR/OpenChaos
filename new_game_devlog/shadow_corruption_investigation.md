# Расследование: артефакты теней при отрисовке debug линий (2026-04-05)

## Симптомы

При прицеливании NPC из огнестрельного оружия или при включённом debug overlay (Ctrl):
1. На тенях персонажей появлялись белые/серые квадратики поверх нормальной тени
2. На дороге/зебре появлялись артефакты-силуэты — фрагменты теней разных персонажей в неправильных местах
3. Тень Дарси могла пропадать когда артефакты появлялись
4. Тень машины не затрагивалась
5. Без debug overlay и без целящегося NPC — тени рисовались нормально
6. Баг проявлялся только в OpenGL рендерере (в D3D6 предположительно не было — не удалось проверить, сборка D3D6 не собиралась на текущем окружении)

## Расследование

### Что проверяли и исключили

| Гипотеза | Результат |
|----------|-----------|
| Render state кеш (SetChanged) | Исключено — полное отключение кеша (принудительная установка всех GL состояний каждый draw call) не помогло |
| Uniform кеш (snapshot memcmp) | Исключено — полное отключение кеширования uniforms не помогло |
| glBindTexture при GE_TEXTURE_NONE | Исключено — добавление glBindTexture(0) не помогло |
| pTheRealPolyPage перенаправление | Исключено — лог показал что никаких перенаправлений между страницами нет |
| sort_to_front=true (z=0 в depth buffer) | Исключено — замена на sort_to_front=false не помогло |
| Смена poly page (COLOUR → COLOUR_ALPHA) | Исключено — баг воспроизводился на обеих страницах |
| DepthWrite=false на POLY_PAGE_COLOUR | Частично помогло для квадратиков на тенях, но сломало HP бары врагов (они тоже через POLY_PAGE_COLOUR) |
| DepthEnabled=false на POLY_PAGE_COLOUR | Не помогло для зебра-артефактов |
| memset VB данных при release | Не помогло |
| VBO orphaning (glBufferData перед каждым upload) | Не помогло |
| Не освобождать VB в Clear() | Не помогло |
| Загрузка только используемых вершин (max index scan) | Не помогло |

### Шейдерная диагностика

Добавили в fragment shader: если `u_has_texture == 0` → рисовать красным. Результат:
- Квадратики на тенях персонажей — **красные** (u_has_texture=0, тень рисуется без текстуры)
- Артефакты-силуэты на зебре — **не красные** (рисуются с текстурой, но в неправильных местах)

### Ключевые эксперименты

1. **Отключение `draw_view_line` (guard за allow_debug_keys)** → квадратики на тенях и пунктиры пропали. Тени нормальные. Но артефакты на зебре оставались в debug overlay.

2. **Полное отключение `AENG_world_line` (return в начале)** → ВСЕ артефакты теней пропали (и квадратики и зебра). Подтверждение что именно `AENG_world_line` — источник.

3. **`AENG_world_line` делает POLY_transform но НЕ POLY_add_line** → артефактов нет. Значит проблема именно в добавлении геометрии в poly pipeline.

4. **Deferred draw_view_line (из game tick в render pass)** → квадратики на тенях пофикшены в обычном режиме. В debug overlay зебра-артефакты оставались (другие AENG_world_line вызовы из game tick).

5. **Флаг s_in_render_pass блокирует AENG_world_line вне render pass** → ВСЕ артефакты пофикшены и в обычном режиме и в debug overlay.

### Наблюдение про ряд теней на зебре

Артефакты-силуэты на дороге — это фрагменты теней РАЗНЫХ персонажей, расположенные в ряд. Некоторые из них двигались при движении соответствующего персонажа. Выглядело как содержимое shadow page разбросанное по сцене.

## Root cause (предположительный)

Порядок выполнения в игровом цикле:
1. **Game tick** — `process_controls()`, PCOM AI, `WMOVE_draw()`, physics debug, etc.
2. **`POLY_frame_init()`** — очищает ВСЕ poly pages (вызывает `PolyPage::Clear()` → `ge_vb_release()`)
3. **Render pass** — `AENG_draw()` добавляет геометрию (стены, персонажи, тени, HUD) и рисует

`draw_view_line` (пунктиры NPC) и debug визуализация (`WMOVE_draw`, physics springs) вызывали `AENG_world_line` на шаге 1, добавляя геометрию в poly pipeline. На шаге 2 `POLY_frame_init` стирала эту геометрию и освобождала VB slot'ы. На шаге 3 другие страницы (тени) могли получить те же slot'ы.

Предположительно остатки данных от debug линий в переиспользованных буферах вызывали артефакты. Однако прямые попытки исправить переиспользование (memset, orphaning, no-release) не помогли, что ставит эту теорию под сомнение. Возможно реальная причина сложнее — например в том как OpenGL драйвер обрабатывает VAO/VBO при быстром цикле alloc→write→release→realloc.

### Почему только OpenGL (предположительно)

В D3D6 vertex buffer'ы управлялись драйвером DirectDraw. При `DrawIndexedPrimitiveVB` D3D копировал данные во внутренний буфер. При переиспользовании VB — D3D выделял новую GPU память. В нашем OpenGL backend VB pool переиспользует те же CPU+GPU буферы напрямую (один `calloc` буфер + один GL VBO на slot). Не удалось собрать D3D6 для прямой проверки.

## Фикс

Два уровня:

### 1. Универсальный: `s_in_render_pass` флаг (aeng.cpp, game.cpp)

`AENG_world_line` проверяет флаг `s_in_render_pass`:
- `true` → геометрия добавляется нормально
- `false` → ранний return, геометрия отбрасывается

Флаг устанавливается:
- `true` — в конце `AENG_draw()` сразу после `POLY_frame_init()`
- `false` — в `game.cpp` после `screen_flip()`

Это предотвращает добавление любой геометрии через `AENG_world_line` вне render pass.

### 2. Конкретный: deferred `draw_view_line` (pcom.cpp, overlay.cpp, overlay.h)

`draw_view_line` (пунктиры от NPC к цели) вызывалась из `PCOM_process_navtokill` во время game tick. Заменена на `OVERLAY_queue_view_line` — сохраняет пару shooter/target в массив `s_deferred_view_lines[8]`. Отрисовка происходит в `OVERLAY_draw_deferred_view_lines()` во время overlay pass (рядом с `OVERLAY_draw_gun_sights`).

Дополнительно: пунктиры спрятаны за `allow_debug_keys && ControlFlag` — видны только в debug overlay. В оригинальном коде разработчики пометили `draw_view_line` комментарием "(a) doesn't work and (b) doesn't look any good. So it's toast." и исключили из Dreamcast и PSX версий.

## Открытые вопросы

- Почему memset/orphaning/no-release не помогли — если root cause в VB reuse, эти фиксы должны были работать. Возможно root cause глубже (GL driver, VAO state, timing).
- FONT2D_DrawString и draw_text_at тоже ломают тени аналогично — не вызываются в обычном геймплее, но если понадобятся, столкнёмся с той же проблемой. `s_in_render_pass` флаг их не покрывает (они не через `AENG_world_line`).
- Не проверено на D3D6 — сборка x86 не работала на текущем окружении.

## Файлы изменённые в фиксе

| Файл | Изменение |
|------|-----------|
| `new_game/src/engine/graphics/pipeline/aeng.cpp` | `s_in_render_pass` флаг + `AENG_set_render_pass()` + early return в `AENG_world_line` + установка true после `POLY_frame_init` |
| `new_game/src/game/game.cpp` | `AENG_set_render_pass(false)` после `screen_flip()` |
| `new_game/src/ai/pcom.cpp` | `draw_view_line()` → `OVERLAY_queue_view_line()` |
| `new_game/src/ui/hud/overlay.cpp` | `OVERLAY_queue_view_line()`, `OVERLAY_draw_deferred_view_lines()`, `s_deferred_view_lines[8]` |
| `new_game/src/ui/hud/overlay.h` | Declaration `OVERLAY_queue_view_line` |
