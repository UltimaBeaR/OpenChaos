# Fix progress — переписывание readback-эффектов на GPU

Трекинг работы по устранению паттерна `ge_lock_screen` / `ge_unlock_screen`
из gameplay-кода. Общий план — см. [workarounds.md](workarounds.md) вариант 1
(шейдерная замена).

## План этапов

| # | Этап | Статус |
|---|------|--------|
| 1 | `FONT_buffer_draw` → textured glyph atlas | ✅ сделан |
| 2 | `SKY_draw_stars` → batched 1-pixel quads | ✅ сделан |
| 3 | `WIBBLE_simple` → GPU post-process (copy+shader) | ✅ сделан |
| 4 | Cleanup: удалить `ge_lock_screen` / `ge_unlock_screen` / dead API; `AENG_screen_shot` на одноразовый `glReadPixels` без writeback | ✅ сделан |
| 5 | **Найти остаточный триггер** (см. ниже — hang всё ещё воспроизводится) | ⏳ |

## ⚠️ Hang НЕ закрыт после Stages 1-4 (2026-04-22)

После прохождения всех четырёх этапов и полной зачистки lock/writeback-путей
пользователь подтвердил: **hang всё ещё воспроизводится** с теми же
симптомами (system-wide freeze ~1 FPS, десктоп не отвечает). Репро:

- **Момент срабатывания:** финальный этап загрузки миссии, прогресс-бар в
  районе **~80%**. Воспроизводится многократным перезапуском игры +
  запуском миссии (не на каждом старте, но достаточно надёжно).
- **Характер симптомов идентичен исходному:** lag всей ОС, audio играет
  нормально, клавиатура/мышь почти не реагируют.

### Что это значит

Оригинальный root cause (per-frame readback+writeback в gameplay) был
верный — те три тригера действительно дёргали WDDM throttle. Но это был
**не единственный** источник паттерна. Что-то ещё на финальном этапе
pipeline загрузки уровня создаёт тот же sync-read/stall паттерн, о
котором мы ещё не знаем.

### Возможные кандидаты (не проверены)

- Пакетный аплоад текстур уровня — `TEXTURE_load_needed` лит пачкой
  большой объём через `glTexImage2D`. На affected hardware при попадании
  в sync-point с VSync'нутым SwapBuffers теоретически может триггерить
  тот же throttle.
- `glFlush` / `glFinish` где-то в пайпе загрузки (мало где мы их зовём,
  но стоит проверить `ge_texture_loading_done`, промежуточные `ge_flip`'ы
  loader'а).
- Какой-то оставшийся `glReadPixels` при создании mipmaps / conversion
  текстур — надо сгрепать.
- Запись в `s_dummy_screen` (даже если путь был мёртв, какой-то fallback
  code-path мог читать нули) — Stage 4 это удалил, но проверим что ничего
  не обращается к freed-памяти.

### Следующий шаг (Stage 5)

Профилировать/отлаживать что именно происходит в районе 80% загрузки на
affected hardware:

1. Инструментировать pipeline загрузки миссии (`TEXTURE_load_needed`,
   `TEXTURE_load_page`, и прочие точки где могут делаться GL-sync calls).
   Логировать время между ключевыми шагами — найти большой gap = там
   и триггер.
2. Сгрепать кодовую базу ещё раз на **любые GL sync-point'ы**:
   `glReadPixels`, `glFlush`, `glFinish`, `glGetTexImage`, sync objects,
   fences. Всё что блокирует CPU на GPU должно быть либо удалено, либо
   перенесено вне per-frame path.
3. Если ничего подозрительного не найдётся — рассмотреть:
   - Отключить VSync на время загрузки (swap interval = 0 → swap interval = 1 по завершении).
   - Выдавать CPU quantum через `Sleep(0)` / `sched_yield` в прогресс-лупе
     loader'а, чтобы ОС успевала обрабатывать desktop compositor.

**Это блокер 1.0.** Запись в [known_issues_and_bugs.md](../../new_game_planning/known_issues_and_bugs.md)
«Тормоза при загрузке миссии» обновлена.



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

## Stage 3 — детали сделанного

Суть: `WIBBLE_simple` больше не трогает CPU-буфер. Он
1) копирует регион из default FB в persistent scratch-текстуру
   (`glCopyTexSubImage2D`, server-side GPU→GPU);
2) рисует warped quad обратно с новым fragment shader'ом, который делает
   per-row horizontal shift из `sin/cos` (формула 1:1 с CPU, float
   вместо fixed-point). Scissor ограничивает зону эффекта прямоугольником
   лужи. Lock/unlock wrapper в `aeng.cpp` снят.

### Новые файлы

- [fullscreen_quad_vert.glsl](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/fullscreen_quad_vert.glsl) —
  минимальный NDC-passthrough vertex shader. Позиция задаётся прямо в
  `[-1, +1]` без viewport-матрицы — для post-process всё проще: квад
  полностраничный, а конкретный регион режет `glScissor`.
- [wibble_frag.glsl](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/wibble_frag.glsl) —
  per-row UV-shift шейдер. `gl_FragCoord.y` инвертится в top-down row
  index, `mod()` возвращает angle в [0, 2048), `sin/cos` дают offset
  в пикселях (`* s / 8` — алгебраическая замена `SIN(a)*s >> 19`).
- [wibble_effect.h](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.h) /
  [wibble_effect.cpp](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.cpp) —
  backend-side имплементация `ge_apply_wibble`. Lazy-init
  программы/scratch-текстуры/VAO при первой лужи-кадре, `gl_wibble_effect_shutdown`
  освобождает в `ge_shutdown`.

### Новое API в GE

- `ge_apply_wibble(x1, y1, x2, y2, GEWibbleParams)` — публичный вход для
  эффекта. Принимает `GEWibbleParams { wibble_y1/2, wibble_g1/2,
  wibble_s1/2, game_turn }` — прямой перенос оригинальных UBYTE-параметров
  `puddle_*` + `GAME_TURN`.

### Изменённые файлы

- [wibble.cpp](../../new_game/src/engine/graphics/postprocess/wibble.cpp) —
  CPU-реализация (lock + per-row memcpy + `SIN`/`COS` LUT) удалена.
  Осталась только скейл-коррекция
  `x,y * RealDisplay* / Display*`, упаковка в `GEWibbleParams` и вызов
  `ge_apply_wibble`. Размер файла сжался со 135 строк до ~42.
- [wibble.h](../../new_game/src/engine/graphics/postprocess/wibble.h) —
  комментарий «SCREEN MUST BE LOCKED» выкинут, описание обновлено под
  GPU-путь.
- [aeng.cpp:4909](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L4909) —
  `if (ge_lock_screen()) { WIBBLE_simple(...); ge_unlock_screen(); }`
  заменён на голый `WIBBLE_simple(...)` под внешним
  `if (ix1 < ix2 && iy1 < iy2)`.
- [core.cpp](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp) —
  `#include "postprocess/wibble_effect.h"` + `gl_wibble_effect_shutdown()`
  из `ge_shutdown`.
- [game_graphics_engine.h](../../new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h) —
  новая секция «Post-process effects» с `GEWibbleParams` и
  `ge_apply_wibble`.
- [CMakeLists.txt](../../new_game/CMakeLists.txt) — `embed_shaders`
  берёт ещё два `.glsl` файла; `BACKEND_SOURCES` получает
  `wibble_effect.cpp`.

### Что **не** делалось (сознательно, вопреки плану)

Handoff-план предлагал целый набор helper'ов: `GEShaderProgram` (compile
+ uniform-cache abstraction), `GERenderTarget` (FBO + color tex + опц.
depth), `fullscreen_quad.h` (общая draw-помощник). Ничего из этого не
добавлено — у нас **один** клиент (wibble), `gl_shader_create_program`
из `common/gl_shader.cpp` уже делает compile+link, uniform-locations
лежат напротив как `static GLint`, FBO вообще не нужен (мы никуда не
render-to-texture). CLAUDE.md прямо запрещает «не добавлять абстракции
сверх задачи»; как появится второй post-process клиент (bloom/tonemap),
дубликат вытащится в helper. Пока — namespace-local функции
`ensure_program` / `ensure_scratch` / `ensure_quad` в
`wibble_effect.cpp`.

### Гочи

**Формула: float заменяет fixed-point.** `SIN(a) * s >> 19` становится
`sin(rad) * s / 8`. Алгебра:
`SIN(a) = sin(a*2π/2048) * 65536`, `>> 19 = / 524288`, итого коэффициент
`65536 / 524288 = 1/8`. Визуально не отличается, bit-exact не требуется
(вода).

**Phase accumulation и float-precision.** `GAME_TURN` растёт без границ
(SLONG, ~25Hz tick). `GAME_TURN * wibble_g1` за ≈65k кадров выскакивает
за 23-битную мантиссу float'а — стал бы ступенчатым в шейдере. Решение:
на CPU сразу сворачиваем `(game_turn * g) & 2047`, передаём уже
`phase ∈ [0, 2048)`. Шейдер домешивает только `row_y * wibble_y ≤ 275k`
— в мантиссе помещается безопасно.

**MSAA resolve через `glCopyTexSubImage2D`.** Spec'у GL 4.1 по поводу
чтения MSAA default FB в single-sample texture: implementation-defined
(обычно — sample 0, без реального resolve). Приемлемо: wibble маскирует
aliasing под искажением. Если визуально будет заметно — switch на
`glBlitFramebuffer` + отдельный single-sample FBO. Не сделано сейчас
чтобы не добавлять FBO до первого реального клиента.

**Half-pixel alignment — не проблема.** Post-process квад идёт через
свой vertex shader (NDC passthrough, без `-0.5`-сдвига из `tl_vert.glsl`).
Сэмплим через `gl_FragCoord` — это уже центры пикселей. Шейдер сразу в
GL-конвенции, D3D6 pixel-center трюк не нужен.

**State save/restore.** Эффект трогает: viewport, scissor+enable,
current program, VAO, bound texture на TEXTURE0, cull-face enable,
blend/depth/depth-mask. Первые шесть — сохраняются/восстанавливаются
руками (`glGetIntegerv` + `glIsEnabled` → в конце `glUseProgram`/
`glBindVertexArray`/`glViewport`/…). Последние три — через
`ge_push_render_state` / `ge_pop_render_state` из Stage 1.

**GERenderState texture cache консистентность.** `set_frag_uniforms` в
`core.cpp` пропускает `glBindTexture` если snapshot совпадает с
текущим s_bound_texture — а мы через прямой `glBindTexture(scratch)`
разошли GL state с кэшем. Фикс: сохранить prev `GL_TEXTURE_BINDING_2D`
и восстановить перед возвратом, чтобы реальный GL-bind совпал с тем,
что думает кэш.

**Cull-face.** `ge_set_cull_mode` умеет ставить `glFrontFace(GL_CW)` —
под таким режимом мой CCW-quad culling'ом вырежется в ноль. Поэтому
локально `glDisable(GL_CULL_FACE)` на время draw + восстановление.

### Проверка

Миссия днём с лужами после дождя (или `rain_mode` / погода с водой).
Точная миссия для cross-check'а — у пользователя Steam PC версия есть
— ждём какую именно запустит.

После Stage 1+2+3 в game loop'е не остаётся ни одного per-frame
`ge_lock_screen`: font, stars и wibble все идут через GPU. Это финальный
тест — hang на affected hardware должен не триггериться вообще в
нормальной игре. Remaining lock-сайты (`AENG_screen_shot` — по нажатию
PrtScr, dead API `AENG_lock`/`_unlock`/`_flush`) доест Stage 4.

## Stage 3 — оригинальный handoff-план (для истории)

Написан перед имплементацией Stage 3. Оставлен как срез контекста на
момент старта — что было задумано vs что реально сделано (см. «Stage 3
— детали сделанного» выше, секция «Что **не** делалось» описывает
расхождения).

Wibble — screen-space рябь воды для отражений в лужах. Единственный
оставшийся per-frame gameplay trigger hang'а после Stage 2. Это первый
реальный клиент post-process инфраструктуры.

### Что делает `WIBBLE_simple` сейчас

[wibble.cpp](../../new_game/src/engine/graphics/postprocess/wibble.cpp) —
per-row horizontal shift уже отрендеренного backbuffer'а внутри прямоугольного
региона `(x1,y1)-(x2,y2)`:

```
offset = (sin(y*wy1 + turn*g1)*s1 + cos(y*wy2 + turn*g2)*s2) >> 19
dest[x] = src[x + offset]   // шифтит пиксели строки по горизонтали
```

Работает через `ge_get_screen_buffer()` + прямая запись CPU-буфера. Lock/unlock
в [aeng.cpp:4914](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L4914)
в цикле по отражениям-луж.

### Целевая архитектура

**Подход:** `glCopyTexSubImage2D` рендер-региона из default FB в scratch
текстуру (auto-resolve MSAA, server-side GPU→GPU — вне паттерна который
висит), потом рисуем warped quad на тот же регион со своим fragment shader'ом
что делает per-row UV-shift из sin/cos.

**Почему именно copy+quad а не чистый FBO render-to-texture:** investigation
[workarounds.md](workarounds.md) вариант 5 отметил FBO+readpixels как тоже
синхронный read — но это про CPU readback. `glCopyTexSubImage2D` (GPU→GPU)
**не** попадает в паттерн throttle'а. Проверено умозрительно — реально
проверить на affected hardware во время имплементации.

### Архитектурные решения (все согласованы с пользователем)

1. **Scene FBO НЕ делаем сейчас.** Scene остаётся на default MSAA backbuffer.
   Переход на scene-FBO + FXAA/TAA — пост-1.0 задача. `glCopyTexSubImage2D`
   из MSAA default FB в single-sample scratch texture автоматически резолвит
   MSAA per GL spec — нам этого хватает.

2. **Формат scratch color texture — `GL_RGBA8`.** Без HDR. Steam Deck
   ориентир — RGBA16F дорогой.

3. **`GEShaderProgram` только для нового кода.** Существующие `s_program_tl`
   / video program не трогаем — они рабочие. `GEShaderProgram` это helper
   для post-process шейдеров чтобы не дублировать `glCreateShader` +
   `glCompileShader` + uniform-location cache в каждом новом файле.

4. **`GERenderTarget` абстракция:** FBO + color texture + опц. depth
   renderbuffer + size, с create/resize/destroy. Для Stage 3 wibble нужен
   только color. Для будущего bloom'а/tonemap'а будет ping-pong из двух
   таких таргетов.

5. **Расположение инфры.** В обсуждении я предложил `backend_opengl/postprocess/`,
   пользователь ответил "делай как удобнее". Внутри `backend_opengl/` уже
   есть `common/`, `game/`, `outro/`, `shaders/` — `postprocess/` ложится
   естественно. В `new_game/src/engine/graphics/postprocess/` лежит код
   **эффектов** (wibble.cpp), а в `backend_opengl/postprocess/` — их
   GPU-реализация.

6. **Существующие helper'ы из Stage 1 переиспользовать:**
   - `ge_push_render_state()` / `ge_pop_render_state()` — уже в
     [core.cpp](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)
     с reentrant depth counter. Оборачиваем wibble draws.
   - `ge_create_user_texture_r8_rrrr` — паттерн для backend-only текстур.
     Для RGBA8 scratch нужен будет аналогичный `ge_create_user_texture_rgba8`
     или добавить в `GERenderTarget` create-логику напрямую через glad.

### Предлагаемая структура файлов

```
new_game/src/engine/graphics/graphics_engine/backend_opengl/
  postprocess/
    shader_program.h         — GEShaderProgram helper (load vert+frag, uniform cache)
    shader_program.cpp
    render_target.h          — GERenderTarget (FBO + color tex + optional depth)
    render_target.cpp
    fullscreen_quad.h        — draw_screen_rect(program, rect, uniform-setter)
    fullscreen_quad.cpp
    wibble_effect.h          — apply_wibble_effect(rect, wibble_params) — новый GE API
    wibble_effect.cpp
  shaders/
    wibble_frag.glsl         — per-row UV shift shader
    fullscreen_quad_vert.glsl — минимальный screen-space vertex shader (NDC квад с UV)
```

И публичный API в `game_graphics_engine.h`:
```cpp
struct GEWibbleParams {
    uint8_t wibble_y1, wibble_y2, wibble_g1, wibble_g2, wibble_s1, wibble_s2;
    int32_t game_turn;   // GAME_TURN — нужен шейдеру как фаза анимации
};
void ge_apply_wibble(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                     const GEWibbleParams& params);
```

`wibble.cpp` (в engine/graphics/postprocess/) становится тонкой обёрткой
над `ge_apply_wibble` — пропадает весь CPU-shift код. `aeng.cpp:4914`
снимает `ge_lock_screen` / `ge_unlock_screen` wrapper.

### Что нужно знать для шейдера

Формула оригинала (wibble.cpp:48-49):
```c
angle1 = y * wibble_y1 + GAME_TURN * wibble_g1;  // & 2047
angle2 = y * wibble_y2 + GAME_TURN * wibble_g2;  // & 2047
offset = SIN(angle1) * s1 >> 19;
offset += COS(angle2) * s2 >> 19;
```

`SIN`/`COS` в оригинале — fixed-point LUT 2048 элементов, range `-65536..65536`.
`>> 19` нормализует до pixel-space offset (`65536 * 128 = 8388608`, `>> 19 = 16`
— ~16 пикселей макс амплитуда при s=128).

В GLSL можно:
- вариант A: заменить `SIN`/`COS` на `sin`/`cos` (float) — проще, визуально 1:1
  на глаз но не bit-exact. Для визуального эффекта достаточно.
- вариант B: передать LUT как uniform array или texture — bit-exact но
  overkill для фонового эффекта.

**Рекомендую вариант A** — проще, достаточно. Пересчитать масштабы: вместо
`>> 19` делим на `524288.0` или эквивалентно умножаем `s` на `1.0/524288.0`.

### Гочи

**Half-pixel offset.** `tl_vert.glsl` вычитает 0.5 от D3D6 координат. Для
wibble quad'а — работаем в нём же или в новом `fullscreen_quad_vert.glsl`?
Если новый vertex shader, для него выравнивание задаём по-своему (NDC-space
напрямую, без D3D6 convention). Я бы сделал новый — post-process квадраты
чище задавать в NDC [-1, 1] с прямой UV mapping'ой.

**MSAA resolve.** `glCopyTexSubImage2D` из MSAA default FB в single-sample
texture auto-резолвит **по spec GL 3.0+**. Проверить драйвер-специфику на
NVIDIA/Intel/AMD — теоретически работает везде, на практике иногда баги.

**State management.** `apply_wibble_effect` должна сохранить/восстановить
GE state — используй существующий `ge_push_render_state` / `ge_pop_render_state`
(Stage 1 добавил reentrant stack, работает). Плюс — bind новой shader-программы
требует отдельного тракинга (s_cached_program в core.cpp, можно не лезть
туда, просто `glUseProgram(old_program)` в конце effect'а).

### Проверка

Нужна миссия с лужами **днём после дождя** (чтобы отражения wibble'ились
видимо). Предполагаю `testdrive` серия или любая downtown миссия с
конкретной погодой. Надо спросить пользователя какую миссию удобнее
запустить — у него Steam PC версия есть для cross-check визуала.

После фикса на миссии testdrive1a ночью (stars триггер, теперь убран) и
wibble-миссии hang должен перестать триггериться полностью. Это финальный
тест после Stage 3.

## Stage 4 — детали сделанного

Суть: вся инфраструктура `ge_lock_screen` / `ge_unlock_screen` + сопутствующие
CPU-pixel API удалены из бэкенда. Скриншот-путь переписан на одноразовый
`glReadPixels` без writeback'а. GPU-бенчмарк эпохи Voodoo1..GeForce 2
выкинут целиком — он был и бессмыслен на современном железе (всегда
максимальные детали), и сам дёргал пустой lock/unlock на старте.

### Удалено

- `ge_lock_screen`, `ge_unlock_screen` — per-frame readback+writeback.
- `ge_plot_pixel`, `ge_plot_formatted_pixel`, `ge_get_pixel` — прямой
  доступ к locked-buffer.
- `ge_get_screen_buffer`, `ge_get_screen_pitch`, `ge_get_formatted_pixel`
  — ненужные геттеры CPU-буфера.
- Внутренний state в [core.cpp](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp):
  `s_dummy_screen`, `s_screen_locked`, `s_screen_w/h/pitch`,
  `ensure_screen_buffer()`.
- `AENG_lock`, `AENG_unlock` в
  [aeng.cpp](../../new_game/src/engine/graphics/pipeline/aeng.cpp) — thin
  wrapper'ы над `ge_lock_screen`/`ge_unlock_screen`, никем не вызывались.
- `AENG_draw_some_polys`, `AENG_guess_detail_levels`, глобал
  `AENG_estimate_detail_levels`, макрос `GENVAR` — GPU бенчмарк
  1999-го образца. Любое железо последних 20+ лет всегда оказывалось
  в верхнем пороге → все детали всегда ON, что совпадает с дефолтами
  `.env`. Сам бенчмарк рисовал 10k полигонов и дёргал
  `ge_lock_screen()+ge_unlock_screen()` без модификации (осколок D3D6-эры,
  форсившей pipeline-flush) — тот же hang-паттерн ради измерения
  производительности.
- Ключ `estimate_detail_levels` из дефолтного `.env`-блока в
  [env.cpp](../../new_game/src/engine/io/env.cpp).
- Мёртвый закомментированный блок в `AENG_draw_FPS` ссылавшийся на
  `ge_lock_screen` для FPS-наложения.

### Добавлено

- `ge_read_framebuffer_rgba(out, w, h)` в
  [game_graphics_engine.h](../../new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h)
  / [core.cpp](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)
  — **только read**, одноразовый `glReadPixels` в caller-owned buffer с
  флипом строк. Нет persistent CPU-side buffer, нет writeback'а. Используется
  только из screenshot-пути.

### Изменённые файлы

- [game_tick.cpp:tga_dump](../../new_game/src/game/game_tick.cpp) —
  переписан на `ge_read_framebuffer_rgba`: один `glReadPixels`, линейный
  проход по RGBA → `TGA_Pixel`, `TGA_save`. Больше нет pixel-by-pixel
  `ge_get_pixel` внутри двойного цикла.
- [aeng.cpp:AENG_screen_shot](../../new_game/src/engine/graphics/pipeline/aeng.cpp) —
  снят `ge_lock_screen` wrapper, теперь просто зовёт `tga_dump()`.
- [texture.cpp](../../new_game/src/assets/texture.cpp) — удалён
  `AENG_guess_detail_levels()` callsite после загрузки текстур.

### Что осталось в gameplay на старом паттерне

**Ничего.** Ни в одном код-пути нет `ge_lock_screen` / `ge_unlock_screen`
или related plot-API. `grep` показывает только упоминания в комментариях
(исторический контекст — почему font/stars/wibble пошли через GPU).

Остался единственный `glReadPixels` — внутри `ge_read_framebuffer_rgba`,
вызывается только когда игрок жмёт PrtScr или включает `record_video`
(последнее — если юзер осознанно хочет записывать видео через дебажный
тумблер, тогда per-frame glReadPixels неизбежен; writeback'а по-прежнему
нет, так что даже этот путь не триггерит hang).
