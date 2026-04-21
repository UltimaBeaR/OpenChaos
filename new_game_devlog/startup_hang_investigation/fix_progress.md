# Fix progress — переписывание readback-эффектов на GPU

Трекинг работы по устранению паттерна `ge_lock_screen` / `ge_unlock_screen`
из gameplay-кода. Общий план — см. [workarounds.md](workarounds.md) вариант 1
(шейдерная замена).

## План этапов

| # | Этап | Статус |
|---|------|--------|
| 1 | `FONT_buffer_draw` → textured glyph atlas | ✅ сделан |
| 2 | `SKY_draw_stars` → batched 1-pixel quads | ✅ сделан |
| 3 | `WIBBLE_simple` → GPU post-process (copy+shader) | ⏳ следующий |
| 4 | Cleanup: удалить `ge_lock_screen` / `ge_unlock_screen` / dead API; `AENG_screen_shot` на одноразовый `glReadPixels` без writeback | ⏳ |

## Архитектурное правило

Пост-процесс инфраструктура (`GERenderTarget`, `GEShaderProgram`, dedicated
shader'ы) строится **только** на Этапе 3 — когда появится первый реальный
клиент (wibble). Stage 1 и 2 работают на существующем TL-пути, без новых
шейдеров и без FBO, потому что это просто 2D textured quads поверх сцены.

MSAA на этапах 1-4 остаётся на default framebuffer — на offscreen scene-FBO
перейдём пост-1.0 (вместе с FXAA/TAA и fullscreen post-process цепью). Scratch
таргеты для wibble (Этап 3) — single-sample, MSAA резолвится через
`glCopyTexSubImage2D` (server-side, без CPU-traffic).

## Stage 1 — детали сделанного

Суть: `FONT_draw_coloured_char` больше не плотит 5×9 битмап попиксельно в
locked backbuffer через `ge_plot_pixel` — вместо этого эмитит один textured
quad из GPU-атласа через существующий TL-путь. `FONT_buffer_draw` больше не
оборачивает свой loop в `ge_lock_screen` / `ge_unlock_screen`.

### Новые файлы

- [font_atlas.h](../../new_game/src/engine/graphics/text/font_atlas.h) /
  [font_atlas.cpp](../../new_game/src/engine/graphics/text/font_atlas.cpp) —
  атлас пакует `FONT_upper[]` / `FONT_lower[]` / `FONT_number[]` / `FONT_punct[]`
  (87 глифов) в R8-текстуру 128×96 один раз лениво при первом draw. Каждая
  буква — индексом находит свою ячейку, эмитит quad с UV через
  `ge_draw_indexed_primitive`.

### Новое API в GE

- `ge_create_user_texture_r8_rrrr(w, h, pixels)` /
  `ge_destroy_user_texture(handle)` — лёгкий путь для инженерных текстур
  (атласы, lookup-таблицы, post-process scratch) в обход page-системы.
  Текстура семплится как `(R, R, R, R)` через GL swizzle — 1-битный
  glyph-mask модулирует vertex color через существующий `Modulate` путь,
  color-key discard'ит нули. Формат выбран под Stage 1 но переиспользуется
  Stage 3.
- `ge_push_render_state()` / `ge_pop_render_state()` — save/restore того
  подмножества GE-state (`s_alpha_test_enabled`, `s_color_key_enabled`,
  `s_texture_blend`, bound texture, GL blend/depth enables, depth mask), что
  трогают ad-hoc 2D-overlay draws. Нужно потому что `GERenderState` в
  `game_graphics_engine.cpp` кэширует `s_State` и при `MAYBE_FLUSH` пропускает
  flush при равенстве — мутация мимо GERenderState течёт в следующий игровой
  draw. Стек reentrant: только внешняя пара делает snapshot (важно для
  `FONT_draw_speech_bubble_text` — там двойной проход measure+draw, оба
  дёргают begin/end).

### Изменённые файлы

- [font.cpp](../../new_game/src/engine/graphics/text/font.cpp) —
  `FONT_draw_coloured_char` зовёт `FONT_atlas_draw_glyph`, все публичные
  draw-функции (`FONT_draw`, `FONT_draw_coloured_text`, `FONT_buffer_draw`,
  `FONT_draw_speech_bubble_text`) обёрнуты в `FONT_atlas_begin_batch` /
  `FONT_atlas_end_batch` — один push/pop на батч текста вместо per-glyph.
  `FONT_buffer_draw` без lock/unlock.
- [aeng.cpp](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  `AENG_draw_messages` — снял `ge_lock_screen` / `ge_unlock_screen`. Внутри
  теперь GL draws от `FONT_draw` / `MSG_draw`; lock/unlock после их эмита
  затёр бы фреймбуфер обратным аплоадом pre-lock контента.
- [core.cpp](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp) —
  имплементация нового GE API (push/pop + user texture).

### Гочи которые вылезли

**Half-pixel misalignment на quad'ах.** `tl_vert.glsl` вычитает `-0.5` из
каждой вершины (D3D6 integer → OpenGL half-integer pixel-center mapping).
Вершины `(sx, sy)` попадают на GL-координаты `(sx-0.5, sy-0.5)` — края квада
идут через **центры** пикселей, rasterizer даёт частичное покрытие +
UV-интерполяция в полутекселях → мыло. Правится `+0.5` сдвигом вершин; то
же делает fullscreen blit в `ge_unlock_screen`.

**`GERenderState::s_State` кэш.** См. выше — из-за `MAYBE_FLUSH` нельзя
просто вызвать `ge_set_color_key_enabled(true)` и забить: следующий игровой
draw думает что color key уже выключен (стал кэш), не переоткидывает, и
наш `true` течёт. Именно поэтому push/pop.

**Nested begin/end.** `FONT_draw_speech_bubble_text` делает первый проход
замера через `FONT_draw_coloured_text(-100, -100, ...)` с off-screen
координатами — inner функция тоже дёргает begin/end. Single-slot stack с
assertion ломал бы этот путь; поэтому reentrant (depth counter, snapshot
только на `0→1`, restore только на `1→0`).

### Проверка

F9 → `bangunsnotgames` → Enter → легенда debug-клавиш в углу на 5 сек.
Также F11 (input debug panel) — тот же `FONT_buffer_add` путь. Визуально
должно быть 1-в-1 с предыдущим билдом (нет мыла после `+0.5` фикса).

## Stage 2 — детали сделанного

Суть: `SKY_draw_stars` сохранил весь CPU-код (POLY_transform, clip, twinkle,
spread-логика) — менялся только выход: `ge_plot_pixel` / `ge_plot_formatted_pixel`
заменены на batched эмит 1×1 quad'ов без текстуры. Один (или несколько, если
звёзд > 16383 pixels) `ge_draw_indexed_primitive` в конце функции. Callsite
в `aeng.cpp` больше не оборачивает SKY_draw_stars в `ge_lock_screen`.

### Изменённые файлы

- [sky.cpp](../../new_game/src/engine/graphics/geometry/sky.cpp) — новый
  namespace-local batcher (`star_emit_pixel`, `star_flush_batch`) с
  `std::vector<GEVertexTL>` + `std::vector<uint16_t>` буферами. Capacity
  ретейнится между кадрами (vector::clear на POD — O(1)), первый ночной
  кадр прогревает ~несколько килобайт, дальше ноль аллокаций per-frame.
- [aeng.cpp](../../new_game/src/engine/graphics/pipeline/aeng.cpp) — снял
  `ge_lock_screen` / `ge_unlock_screen` wrapper вокруг callsite
  `SKY_draw_stars` (в `AENG_draw_city`). Сохранил `BreakTime("Drawn stars")`
  перф-маркер, убрал `BreakTime("Locked for stars")` — нет лока больше.

### Гочи / моменты

**16-bit indices.** `ge_draw_indexed_primitive` принимает `uint16_t*`, макс
65535. Худший случай (4096 звёзд × 5 пикселей × 4 верта = 81920) не
помещается. Flush-порог 16383 quads = 65532 верта (последний безопасный), за
ним принудительный flush. В типичном кадре видимых+не-мерцающих ~несколько
сотен пикселей — один flush в конце функции. Порог срабатывает разве что
если в будущем кто-то поднимет `SKY_MAX_STARS`.

**Color упаковка.** `0xFF000000 | (c<<16) | (c<<8) | c` — D3D6 0xAARRGGBB.
Байты в памяти little-endian (B, G, R, A), шейдер нормализует и свизлует
`.zyxw` в (R, G, B, A). Для c=0xCC → (0.8, 0.8, 0.8, 1.0) — серый. 1:1
с `ge_plot_pixel(c, c, c)`.

**Пустой кадр.** Если ни одна звезда не попала в экран (twinkle + clip) —
early return без `ge_push_render_state`, ни одной GL-операции. Бесплатно на
кадрах-без-звёзд.

### Проверка

Любая ночная миссия с `AENG_detail_stars` (дефолтно включено на medium+).
Смотреть в небо — яркие звёзды с spread рисуются крестом (центр + 4 соседа),
тусклые точкой. Мерцание работает — звёзды пропадают на случайный кадр. Цвет
и позиции должны быть идентичны предыдущему билду.

## Что осталось в gameplay на старом паттерне

После Stage 2 `ge_lock_screen` / `ge_unlock_screen` всё ещё вызывается из
одного per-frame gameplay callsite:
- `WIBBLE` — [aeng.cpp:4914](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L4914)
  (кадры где есть лужи с отражениями, hang триггер).

Плюс non-per-frame debug callsites (`AENG_screen_shot`, бенчмарк-stub, dead
API `AENG_lock` / `AENG_unlock` / `AENG_flush`) — их Stage 4 доест.
