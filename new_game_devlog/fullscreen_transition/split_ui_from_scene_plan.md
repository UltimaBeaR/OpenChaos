# План: вынести UI из scene FBO в post-composition pass

**Статус (2026-04-25):** Этапы 1-5 + 7 сделаны (Этап 7 закрылся
автоматически через инфраструктуру Этапа 1 — UI рисуется в native
window pixels через dst rect-based scaling; отдельного шага не
понадобилось). Кошачий fade / F1 / F11 / консоль / pause меню /
FONT_buffer_draw / **HUD: PANEL_draw_buffered + PANEL_last +
PANEL_inventory + cheat==2 FPS overlay** сейчас рисуются в
post-composition, не блюрятся FXAA/bilinear. В scene FBO остались
только depth-зависимые оверлеи: gun sights, deferred view lines,
enemy health (со своей парой PANEL_start/PANEL_finish для
собственного POLY-батча).
Остался Этап 6 — финальная матрица визуальных проверок (4:3 /
16:9 / 21:9 / portrait × scale 1.0 / 0.5). По ходу всплыли и
зафикшены 4 латентных бага (подробно в разделе "Сделано"):
PANEL_fadeout overlay не сбрасывался после abandon миссии; UI scissor
не учитывал dst_x offset на окнах шире cap-aspect (clip справа);
UI слой пропадал во время интерактивного drag-resize; PANEL_fadeout
("кошка") размер брал из FBO, а должен из dst rect — при
`OC_RENDER_SCALE<1.0` quad покрывал только 1/4 экрана в углу.
Добавлен debug-флаг `OC_DEBUG_HIGHLIGHT_NON_UI` чтобы визуально
видеть границу между UI и scene (scene тинтится магентовым +
блюрится).

**⚠️ TODO — найти оставшиеся UI элементы которые ещё в scene FBO.**
Включить `OC_DEBUG_HIGHLIGHT_NON_UI = true` и пройтись по игре
систематически (главное меню, briefing, pause, won/lost, миссии,
видео, attract mode, заставка, finale, ...). Любой UI который виден
магентовым/блюрным — кандидат на перенос в `ui_render_post_composition`.
Пример выявленных позже элементов фиксировать в этой секции и в
issues.md, чтобы не забыть.

Задача: разделить отрисовку 3D мира и UI так чтобы UI не проходил через
композиционный шейдер (FXAA + bilinear upscale). Сегодня все UI рисует
в scene FBO, поэтому при `OC_AA_ENABLE = true` текст HUD'а и меню
блюрится FXAA-фильтром, а при `OC_RENDER_SCALE < 1` — ещё и
билинейным апскейлом.

После рефактора:
```
Scene FBO  ← AENG_draw + depth-зависимые оверлеи (gun sights, enemy health)
           → composition (FXAA + upscale + outer bars)
           → default FB
UI         ──────────────────────────────────────→ default FB напрямую
                                                   (native pixels, без AA)
```

Этот файл — **handoff plan для свежего агента** (контекст разговора, где
он родился, недоступен). Написан 2026-04-25, читать целиком, инструкции
самодостаточные.

Связанные доки:
- [`concepts.md`](concepts.md) — глоссарий (ScreenWidth, g_frame_scale,
  scene FBO, dst rect и т.п.).
- [`issues.md`](issues.md) "Render scale follow-ups" → пункт **"Split UI
  from main render"** — короткое описание задачи.
- [`fbo_as_virtual_screen_plan.md`](fbo_as_virtual_screen_plan.md) —
  предыдущий рефактор, после которого "FBO = виртуальный экран". Этот
  план продолжает ту же линию.
- [`ui_coords_plan.md`](ui_coords_plan.md) — координатная система UI
  (PolyPage::UIModeScope, ui_coords::*, anchors).

---

## Текущий пайплайн (что сегодня делает игра)

Главный игровой цикл (`game.cpp:1211-1239`):

```
draw_screen()              // 3D сцена через AENG_draw + frame stuff
OVERLAY_handle()           // HUD: PANEL_draw_buffered + gun sights +
                           // enemy health + PANEL_last + PANEL_inventory
input_debug_render()       // F11 input debug panel
debug_help_render()        // F1 hotkey legend
CONSOLE_draw()             // консоль
GAMEMENU_draw()            // pause/won/lost меню
PANEL_fadeout_draw()       // кошачий fade
screen_flip()
  ↳ AENG_screen_shot()
  ↳ AENG_draw_messages()    // только если ControlFlag (debug)
  ↳ FONT_buffer_draw()      // флаш буферизованного 2D текста
  ↳ AENG_blit() → ge_blit_back_buffer()
                  ↳ composition_bind_default()
                  ↳ composition_blit(win_w, win_h)
                                    // FXAA + bilinear upscale + outer bars
                  ↳ gl_context_swap()
                  ↳ composition_bind_scene()    // ребинд для след. кадра
```

Все вызовы между `ge_begin_scene()` (где-то в начале кадра) и
`composition_blit()` рисуют **в scene FBO**. Composition обрабатывает
весь FBO целиком — UI и сцену вместе.

**Важно:** в обычном пути (primary driver) flip идёт через
`ge_blit_back_buffer()` (не `ge_flip()`). Это значит существующий
`s_pre_flip_callback` в `ge_flip()` НЕ срабатывает в основном цикле.
Hook надо добавить в `ge_blit_back_buffer()` тоже (либо в обоих).

---

## Что куда переносится

### Остаётся в scene FBO (зависит от 3D / depth buffer)

| Вызов | Где | Почему остаётся |
|-------|-----|-----------------|
| `AENG_draw()` | `draw_screen()` | 3D мир, очевидно. |
| `OVERLAY_draw_gun_sights()` | `OVERLAY_handle` | Использует `POLY_get_sphere_circle()` для проекции world-space цели на экран, рендерится в координатах сцены. |
| `OVERLAY_draw_deferred_view_lines()` | `OVERLAY_handle` | Debug 3D линии (line of sight). |
| `OVERLAY_draw_enemy_health()` | `OVERLAY_handle` | Health bars над персонажами в world-space, нужен scene depth для z-sort. |
| `PANEL_draw_local_health()` | вызывается из game/AI | In-world health bar над thing'ом, world→screen проекция. |
| `s_pre_flip_callback` | `ge_flip` / `ge_blit_back_buffer` | Screensaver / depth bodge reset — должен видеть scene depth. |
| `AENG_screen_shot()` | `screen_flip` | Снимает scene FBO для скриншота-кнопки. |

### Переносится в post-composition (чистый 2D, без depth)

| Вызов | Где сейчас | Заметки |
|-------|------------|---------|
| `PANEL_draw_buffered()` | `OVERLAY_handle` | Радар, health, weapon, ammo, beacons, mission timer. **LEFT_BOTTOM scope.** |
| `PANEL_last()` | `OVERLAY_handle` | Главный HUD: msg stack, search bar, road sign flash, beacons, fadeout cat target, version overlay. |
| `PANEL_inventory()` | `OVERLAY_handle` | Weapon-switch popup. |
| `PANEL_finish()` | `OVERLAY_handle` | Финализирует POLY queue. |
| `input_debug_render()` | `game.cpp:1221` | F11 input debug. |
| `debug_help_render()` | `game.cpp:1226` | F1 legend. |
| `CONSOLE_draw()` | `game.cpp:1231` | Консоль. |
| `GAMEMENU_draw()` | `game.cpp:1233` | Pause/won/lost меню. |
| `PANEL_fadeout_draw()` | `game.cpp:1235` | Cat-iris fade. |
| `AENG_draw_messages()` | `screen_flip` | Debug сообщения. |
| `FONT_buffer_draw()` | `screen_flip` | Флаш 2D текста. |

### На границе — нужно проверить

- **`PANEL_start()`** (вызывается в `OVERLAY_handle:298`): инициализирует
  POLY queue для UI batch'а. Должен идти ПЕРЕД UI draws — значит вместе с
  ними переезжает.
- **`POLY_flush_local_rot()`** (`OVERLAY_handle:293`): очистка локальной
  ротации после 3D. Часть scene path или часть UI path? Посмотреть в
  процессе.
- **Кадр сообщения "GAME PAUSED" / "GAME OVER"**: GAMEMENU рисует поверх
  затемнённой сцены — затемнение должно остаться (в scene FBO как
  fullscreen UI quad), а текст GAME PAUSED — в post-composition.
  **Это требует разделить `GAMEMENU_draw()` на две части** — затемнение
  и текст. Возможно отложить на потом, начать с того что вся `GAMEMENU_draw`
  идёт post-composition (затемнение тоже), и проверить визуально.

---

## Архитектура решения

### Новый API в `composition.h`

```cpp
// Возвращает прямоугольник на default framebuffer'е, куда composition_blit
// нарисовал scene FBO (aspect-fitted, центрированный, с outer bars вокруг).
// Используется UI-пассом чтобы выставить viewport совпадающий со сценой.
// До первого composition_blit() возвращает (0, 0, 0, 0).
void composition_get_dst_rect(int32_t* x, int32_t* y, int32_t* w, int32_t* h);
```

Реализация — просто отдать `s_last_dst_x/y/w/h` (уже хранятся).

### Новый callback в `core.h`

```cpp
typedef void (*GEPostCompositionCallback)(void);

// Регистрирует callback который вызывается ПОСЛЕ composition_blit и ДО
// gl_context_swap. На этот момент: default FB забиндена, viewport = окно,
// scene FBO уже нарисован в dst rect. Колбэк может рисовать UI поверх.
void ge_set_post_composition_callback(GEPostCompositionCallback fn);
```

### Изменение `ge_flip()` и `ge_blit_back_buffer()`

```cpp
void ge_blit_back_buffer()
{
    int win_w = 0, win_h = 0;
    sdl3_window_get_drawable_size(&win_w, &win_h);
    composition_bind_default();
    composition_blit(win_w, win_h);

    // ─── NEW ───
    // Default FB всё ещё забиндена. UI-пасс рисует поверх сцены.
    if (s_post_composition_callback) s_post_composition_callback();
    // ───────────

    gl_context_swap();
    composition_bind_scene();
}
```

Аналогично в `ge_flip()`.

### UI-пасс — новая функция (предлагаемое расположение: `game/ui_render.cpp`)

```cpp
// Регистрируется как post-composition callback в инициализации игры.
// Все UI-draws вынесенные из draw_screen()/OVERLAY_handle/screen_flip
// собраны здесь.
void ui_render_post_composition()
{
    // 1. Сохранить GL state (viewport, scissor, blend, etc.)
    // 2. Установить viewport = dst rect от composition'а:
    //      composition_get_dst_rect(&x, &y, &w, &h);
    //      ge_set_viewport(x, y, w, h);
    //    Это критично: на ultra-wide / portrait окне без этого UI
    //    будет рисоваться через outer bars.
    // 3. Сбросить PolyPage state (POLY_frame_init или эквивалент) —
    //    composition_blit мог поломать кэш состояния.
    // 4. Вызвать UI-функции в нужном порядке:
    //      PANEL_start();
    //      PANEL_draw_buffered();   // если разрешено игровой логикой
    //      PANEL_last();
    //      PANEL_inventory(...);
    //      PANEL_finish();
    //      input_debug_render();
    //      debug_help_render();
    //      CONSOLE_draw();
    //      GAMEMENU_draw();
    //      PANEL_fadeout_draw();
    //      AENG_draw_messages();    // если ControlFlag
    //      FONT_buffer_draw();
    // 5. Восстановить GL state.
}
```

Условия "если разрешено игровой логикой" (типа `!EWAY_stop_player_moving`,
`!(GAME_STATE & GS_LEVEL_WON)`, `!(GAME_FLAGS & GF_PAUSED)`) переезжают
вместе с вызовами как есть.

**Вопрос:** где вызывается `OVERLAY_draw_gun_sights` и аналоги?
В сегодняшнем `OVERLAY_handle` они идут вперемешку с PANEL_draw_buffered.
Решение: оставить в `OVERLAY_handle` только depth-зависимые оверлеи; всё
остальное переезжает в `ui_render_post_composition`. После рефактора
`OVERLAY_handle` становится тонким wrapper'ом для in-scene оверлеев.

### Координатная система — что (не) меняется

**Главный инсайт:** при `OC_RENDER_SCALE = 1.0` И аспекте окна в пределах
`[OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT]` — scene FBO размер **равен**
window drawable size. Значит `s_XScale = W/640, s_YScale = H/480` дают
один и тот же результат, неважно куда рисуем — в FBO или в default FB.
Координаты не меняются, UI попадает на тот же экранный пиксель.

**Edge case 1: окно за пределами clamp'а (ultra-wide / portrait).** FBO
у́же/короче window'а, есть outer bars. Если viewport в UI-пассе не
поправить, UI рисуется на полный window и съезжает с центрированной
сцены. **Фикс:** установить viewport = composition dst rect в начале
UI-пасса.

**Edge case 2: `OC_RENDER_SCALE < 1.0`.** FBO мельче window'а. Если
оставить `s_XScale/s_YScale` без изменений — UI отрендерится в
FBO-разрешении и потом растянется до native (тот же блюр что был, минус
FXAA). Чтобы получить sharp UI в этом режиме, надо в UI-пассе временно
пересчитать `s_XScale/s_YScale` и `ui_coords::*` под dst rect размер.
**Откладываем — отдельный шаг после основной работы.** Default
`OC_RENDER_SCALE = 1.0`, edge case не критичен для 1.0.

---

## Этапы реализации

Каждый этап заканчивается рабочим билдом и визуальной проверкой. После
каждого этапа — review + просьба пользователю проверить.

### ✅ Сделано (Этапы 1 + 2 + 3 + часть 5)

**Инфраструктура:**
- `composition_get_dst_rect(x,y,w,h)` — экспорт публикованного
  composition'ом dst rect'а для UI-пасса.
- `GEPostCompositionCallback` + `ge_set_post_composition_callback()` —
  хук который стреляет между `composition_blit` и `gl_context_swap`
  (и в `ge_flip`, и в `ge_blit_back_buffer`).
- `ge_set_ui_mode(bool)` — флаг который редиректит `ge_begin_scene()`
  с scene FBO на default FB. Нужен потому что `POLY_frame_init` внутри
  UI-вызовов всегда вызывает `ge_begin_scene`, и без этого флага UI
  молча улетал бы обратно в scene FBO (мы попались на этом с fade).
- `ui_render.cpp/h` — хост для post-composition UI-пасса. Сохраняет
  viewport/scissor/depth, ставит viewport в dst rect, отключает
  depth test + writes, поднимает ui_mode, вызывает UI-функции,
  восстанавливает state.

**Перенесено в post-composition:**
- `PANEL_fadeout_draw()` (кошачий fade)
- `input_debug_render()` (F11)
- `debug_help_render()` (F1)
- `CONSOLE_draw()` (под тем же `!(GAME_FLAGS & GF_PAUSED)` condition
  что и раньше)
- `GAMEMENU_draw()` (pause / won / lost меню)
- `AENG_draw_messages()` (под `ControlFlag && allow_debug_keys`)
- `FONT_buffer_draw()` — финальный flush буферизованного
  pixel-path текста (раньше жил в `screen_flip()`).

**Бонусный фикс латентного бага №1:** после abandon миссии
`PANEL_fadeout_time` не сбрасывалось, и чёрный накапливающийся overlay
продолжал рисоваться forever. В старом пайплайне это не было видно
потому что main menu / loading overдрэйвал scene FBO; после разделения
оверлей оставался на default FB. Добавлен `PANEL_fadeout_init()` в
конце `if (PANEL_fadeout_finished())` в `game.cpp`.

**Бонусный фикс латентного бага №2 (UI scissor offset):** на окнах
шире `OC_FOV_CAP_ASPECT` composition центрирует FBO с pillar-bar'ами
(`dst_x ≠ 0`), а `ge_set_scissor` / `ge_disable_scissor` не учитывали
этот фактический offset viewport'а — `glScissor` ждёт абсолютные FB-
координаты, а `push_ui_mode` отдавал scissor относительно `g_screen_w_px =
dst_w` (как у `u_viewport` в шейдере, который имеет origin (0,0)).
Из-за этого scissor попадал в окно `[0..dst_w]`, а viewport рисовал
в `[dst_x..dst_x+dst_w]`, и видимая область сужалась до их пересечения.
Симптом: меню паузы / катсцен-диалог / любой UI с `UIModeScope`
обрезался полосой справа.

В Stage 3 баг существовал, но не проявлялся: пользователь тестировал
только на 16:9 (где FBO == окно, dst_x=0). Stage 4 принёс HUD и
катсцен-диалоги в UI pass — больше UIModeScope-вызовов, при первом же
расширении окна за cap-aspect баг стрельнул.

Фикс: завёл `s_ui_vp_x/y` в `core.cpp`, `ge_enter_ui_viewport`
сохраняет туда фактический dst_x/dst_y, `ge_set_scissor` /
`ge_disable_scissor` в UI mode добавляют этот offset в `glScissor`.

**Бонусный фикс латентного бага №3 (UI исчезал во время drag-resize):**
во время интерактивного ресайза окна Windows захватывает message pump,
основной flip останавливается, SDL шлёт `SDL_EVENT_WINDOW_RESIZED` →
host через `on_window_resize_live` зовёт `ge_present_for_drag`, который
делает свой flip (`composition_present_stretched` + swap). Но
post-composition callback там не вызывался → UI слой полностью
пропадал на время перетаскивания. Дополнительно
`composition_present_stretched` не обновлял `s_last_dst_*`, так что
даже если бы callback стрельнул, UI pass работал бы со stale dst rect.

Фикс: `composition_present_stretched` теперь публикует `s_last_dst_*`
(input mapping during drag не страдает — мышь на title bar, не в
client area), `ge_present_for_drag` дёргает `s_post_composition_callback`
сразу после `composition_present_stretched` и до `gl_context_swap`.

**Бонусный фикс латентного бага №4 (PANEL_fadeout «кошка» при
OC_RENDER_SCALE < 1.0):** quad с кошачьей радужкой при abandon миссии
рисовался размером 1/4 экрана в верхне-левом углу вместо полноэкранного.
Регрессия с Этапа 2 (когда PANEL_fadeout впервые переехал в UI pass).
Корень: [panel.cpp:1163-1182](../../new_game/src/ui/hud/panel.cpp#L1163-L1182)
вычислял свою affine на основе `ScreenWidth`/`ScreenHeight` (= scene
FBO size). Но в UI pass рендер-таргет — default FB на dst rect (=
window pixels). При `OC_RENDER_SCALE < 1.0` FBO в N раз меньше окна
→ quad занимает 1/N² экрана в углу.

При `RENDER_SCALE = 1.0` баг не виден потому что FBO == dst rect ==
окно. Не проявлялся раньше, пока не начали тестировать на 0.25.

Фикс: `PANEL_fadeout_draw` теперь читает `ui_coords::g_screen_w_px /
g_screen_h_px` вместо `ScreenWidth/Height`. Эти globals в UI pass
указывают на dst rect, в scene pass — на FBO. Универсально.

**Debug визуализация:** `OC_DEBUG_HIGHLIGHT_NON_UI` в `config.h` +
uniform в `composite_frag.glsl`. При `true` сцена рисуется магентовой
+ 3×3 box blur, UI-пасс остаётся чётким → сразу видно границу между
"в UI" и "всё ещё в scene".

### Этап 1: Инфраструктура хука (DONE — см. выше)

Добавить API без изменений в game loop. После этого этапа всё работает
ровно как раньше — просто появилась новая возможность.

1. `composition.h`: добавить `composition_get_dst_rect()`.
2. `composition.cpp`: реализовать (просто отдать `s_last_dst_*`).
3. Где-нибудь в `engine/graphics/graphics_engine/...` (в `core.h` или
   соседний файл) — добавить тип `GEPostCompositionCallback`,
   статик `s_post_composition_callback`, сеттер
   `ge_set_post_composition_callback`.
4. В `ge_flip()` и `ge_blit_back_buffer()` вставить вызов callback
   ровно между composition и swap.
5. Билд + игра должна работать как раньше (callback не зарегистрирован).

### Этап 2: Перенос fade-out (DONE)

Начинаем с самого изолированного вызова чтобы проверить инфру:

1. Создать `game/ui_render.cpp` с пустой функцией `ui_render_post_composition()`.
2. Зарегистрировать в init игры через `ge_set_post_composition_callback`.
3. Перенести в неё `PANEL_fadeout_draw()` (последний из draw_screen
   списка, проще всего изолировать).
4. Удалить вызов из `game.cpp:1235`.
5. В `ui_render_post_composition` обернуть вызов в save/restore GL state
   + установка viewport в dst rect.
6. Билд + проверить fade-out при выходе из миссии (Esc → Abandon).

Если fade выглядит нормально на 4:3 и широком — инфра рабочая.

### Этап 3: Перенос debug overlays + console + меню + FONT_buffer flush (DONE)

**Примечание:** изначально Этап 5 (FONT_buffer_draw / AENG_draw_messages)
был отдельным, но мы наткнулись на то что debug_help_render и прочий
pixel-path текст **только queue'ит** в FONT buffer, а сам flush —
это `FONT_buffer_draw` — жил в `screen_flip()` до composition'а. Без
переноса flush в post-composition pass debug-текст всё равно блюрился.
Поэтому Этап 5 сдвинут вперёд и сделан вместе с Этапом 3.

1. Перенести `input_debug_render()`, `debug_help_render()`, `CONSOLE_draw()`,
   `GAMEMENU_draw()` в `ui_render_post_composition` (в том же порядке
   что в `game.cpp:1221-1233`).
2. Удалить вызовы из game loop'а.
3. Билд. Проверить:
   - F11 (input debug) — рисуется поверх сцены, не блюрится.
   - F1 (legend) — рисуется поверх.
   - F9 → консоль — открывается, текст чёткий.
   - Esc → меню паузы — выглядит правильно (затемнение + текст).
   - Won/Lost экраны — выглядят правильно.

⚠️ **GAMEMENU_draw затемнение**: если затемнение выглядит неправильно
(не покрывает outer bars / не покрывает всю сцену), переразделить на
"darken остаётся в scene FBO" + "menu text в post-composition".
Затемнение использует `FullscreenUIModeScope` — он растягивает виртуальный
640×480 на ВЕСЬ FBO. Если FBO теперь не совпадает с default FB, это
поломается. **Запасной план:** разделить `GAMEMENU_draw()` на
`GAMEMENU_draw_backdrop()` (остаётся в scene FBO) и `GAMEMENU_draw_text()`
(переезжает в post-composition).

### Этап 4: Перенос HUD (PANEL_*) — DONE

Сделано:
- `OVERLAY_handle` сократился до depth-зависимых оверлеев:
  `OVERLAY_draw_gun_sights`, `OVERLAY_draw_deferred_view_lines`,
  `OVERLAY_draw_enemy_health` плюс setup (`POLY_flush_local_rot`,
  `ge_set_viewport`). Они обёрнуты в собственную пару
  `PANEL_start`/`PANEL_finish` — раньше пиггибэкали на той же паре что и
  HUD, теперь у каждой стороны свой POLY-батч.
- `PANEL_start`, `PANEL_draw_buffered`, `PANEL_last`, `PANEL_inventory`,
  `cheat==2` FPS overlay (со всей EMA-логикой), `PANEL_finish` переехали
  в `ui_render_post_composition`. Условия (`!EWAY_stop_player_moving`,
  `!(GAME_STATE & GS_LEVEL_WON)`, `cheat == 2`) переехали вместе.
- Весь HUD-блок гейтится по `NET_PERSON(0) && NET_PLAYER(0)` — точное
  условие "есть живой игрок". Изначально пробовал гейтить по
  `GAME_STATE & (GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON)` (как
  в game loop'е [game.cpp:914](../../new_game/src/game/game.cpp#L914)),
  но получили краш в `PANEL_inventory` из `ATTRACT_loadscreen_init →
  ge_flip → ui_render_post_composition` — при загрузочном экране
  миссии state-биты уже стоят, а `NET_PERSON(0)` ещё null. Оригинальный
  `OVERLAY_handle` крутился ТОЛЬКО внутри game loop где darci был
  гарантирован, поэтому null-чек ему был не нужен; нам — нужен,
  потому что `ui_render_post_composition` стреляет на КАЖДЫЙ flip,
  включая loadscreen / video / attract / frontend.
- Cleanup в `overlay.cpp`: убраны unused includes
  (`panel_globals.h`, `interact_globals.h`, `fc.h`, `fc_globals.h`,
  `font2d.h`) и dead externs (`draw_map_screen`, `cheat`,
  `tick_tock_unclipped`).

Что проверить пользователю:
- Радар, health, weapon, ammo на месте.
- Mission timer внутри радара.
- Speech / radio / message stack.
- Search progress bar (если получится воспроизвести).
- Road sign flash (если получится воспроизвести).
- Beacons.
- Fade-out cat (миссия → Abandon).
- Главный визуальный win: `OC_AA_ENABLE = true` — HUD-текст должен
  стать чётким (раньше блюрился FXAA).

### Этап 5: Перенос финальной 2D-флаш фазы

1. `AENG_draw_messages()` (debug-only, под ControlFlag) и
   `FONT_buffer_draw()` — переехать из `screen_flip()` в
   `ui_render_post_composition`.
2. `screen_flip()` становится: `AENG_screen_shot() + AENG_blit()`.
3. `ge_blit_back_buffer` сам вызовет post-comp callback — там пройдёт
   весь UI включая flush.
4. Билд. Проверить что debug overlays (под Ctrl) рисуются.

### Этап 6: Тестирование на edge cases

1. Включить `OC_AA_ENABLE = true` — текст должен быть чётким (раньше
   блюрился). Это главная цель рефактора.
2. Проверить на 4:3 (640×480), 16:9 (1920×1080), 21:9 (2560×1080,
   ultra-wide → outer bars), портрет (480×1920 → letterbox).
3. На каждом разрешении: главное меню, миссия в игре, пауза, fade-out
   при выходе из миссии, briefing экран, save/load экран.
4. (Optional, отдельный шаг.) `OC_RENDER_SCALE = 0.5` — UI всё ещё
   будет блюрный (bilinear upscale во время рендера), но FXAA не
   трогает. Полный фикс — Этап 7.

### Этап 7: Sharp UI на render scale < 1.0 — DONE (как побочный эффект Этапа 1)

Изначально планировался отдельным шагом после релиза. По факту
закрыт инфраструктурой Этапа 1: `ui_render_post_composition` с самого
начала использует **dst rect-based** scaling, а не FBO-based:

```cpp
// ui_render.cpp:80-84
const float uniform_scale = float(dst_h) / 480.0f;
PolyPage::SetScaling(uniform_scale, uniform_scale);
ui_coords::recompute(dst_w, dst_h);
```

`dst_w/dst_h` приходят из `composition_get_dst_rect()` и равны
window-pixel размеру (после aspect-fit FBO в окно). При
`OC_RENDER_SCALE < 1.0` FBO мельче окна, но dst rect остаётся в
window pixels — UI рисуется в native разрешении.

В конце пасса восстанавливаются prev-значения (scene FBO scaling) для
следующего кадра scene-рендеринга.

Отдельных функций `ui_coords::recompute_native()` /
`PolyPage::set_scaling_native()` (предлагалось в плане) делать не
пришлось — текущие функции вызываются дважды в одном пассе с разными
аргументами.

Что осталось: проверить визуально (входит в матрицу Этапа 6).

---

## Риски и подводные камни

### Order dependencies — что должно быть до чего

- `PANEL_start()` ДО любого `PANEL_*` draw.
- `PANEL_finish()` ПОСЛЕ всех `PANEL_*` draw.
- `GAMEMENU_draw` использует `POLY_frame_init` который очищает POLY queue
  → всё что добавилось ПОСЛЕ него в существующем коде дропалось. См.
  комментарий в `game.cpp:1218-1220` про `input_debug_render`. Сохранить
  тот же порядок в callback'е.

### POLY queue / FONT_buffer состояние

Вся 2D-отрисовка проходит через POLY queue (для текстурированных квадов)
и FONT_buffer (для пиксельного шрифта). Они ОТЕДЛЬНЫЕ от scene 3D
queue. Когда composition_blit рендерится, POLY/FONT queue могут быть
не пусты (UI ещё не флашился). Но в существующем коде `FONT_buffer_draw`
вызывается ПЕРЕД `AENG_blit` → значит оно успевает в scene FBO.

После рефактора: UI вообще не идёт в scene FBO. Значит флаш POLY/FONT
queue должен происходить **внутри** post-composition callback'а. Не
до, не после composition_blit.

### Mouse picking

Сегодня `composition_window_to_fbo()` мапит клик в окне → FBO координаты.
UI хит-тесты используют это. После рефактора UI рисуется в native
window pixel space, не в FBO space. Однако визуально UI попадает на ту
же область экрана (dst rect). То есть для существующих хит-тестов
**ничего менять не нужно**: window pixel → composition_window_to_fbo →
FBO coord → UI coord (через `ui_coords::screen_to_frame`) — этот путь
рабочий и совпадает с тем как UI рендерится визуально (потому что
в UI-пассе мы поставили viewport = dst rect, что даёт ту же мапу
NDC↔пиксели что и FBO).

### glClear в UI-пути

UI код может где-то делать `glClear(GL_COLOR_BUFFER_BIT)` — это сейчас
очищает scene FBO. После переноса оно очистит default FB и **сотрёт
композированную сцену**. Перед началом этапа 4 — `Grep` по UI-вызовам
на `ge_clear`, `glClear`, `AENG_clear_*`. Если найдены — оставить в
scene FBO либо адаптировать.

### Pre-flip callback

Существующий `s_pre_flip_callback` (screensaver / depth bodge reset)
рисует в scene FBO ДО composition. Он остаётся как есть. НЕ переносить.

### Видео и фронтенд

Видео-плеер (`ge_video_draw_and_swap`) и frontend transitions имеют
свой path флипа. Проверить что они не сломаются от появления
post-composition callback'а — если callback зарегистрирован, он
сработает и при видео-флипе, что нежелательно (видео в полный экран,
UI не нужен поверх).

**Решение:** callback срабатывает только когда зарегистрирован. В
игровом цикле регистрируем при старте миссии (или при инициализации),
снимаем регистрацию (или передаём nullptr) на время видео и frontend
transitions. Либо игнорируем — frontend transitions сейчас идут через
тот же `ge_blit_back_buffer`, и если в callback'е ui_render_post_composition
реагирует только на game state (типа `if !in_video) ...`), всё ок.

### `OC_DEBUG_COMPOSITION_BARS_RED`

Этот debug-флаг красит outer bars в красный. После переноса UI в
post-composition бары всё ещё рисуются `composition_blit` → красный
сохраняется. Но если UI-пасс выставит viewport на полный window
(ошибочно), он перезапишет бары. Тестировать с этим флагом.

---

## Тестирование

### Чеклист после каждого этапа

1. Билд (`make build-release`) проходит без ошибок.
2. Игра запускается, главное меню рисуется.
3. Старт миссии (Urban Shakedown), HUD на месте.
4. Esc → меню паузы открывается.
5. Esc → Abandon → fade-out → возврат в main menu.

### Финальный чеклист (после Этапа 6)

| Тест | Ожидание |
|------|----------|
| `OC_AA_ENABLE = true`, 1920×1080 | HUD текст чёткий (раньше блюрился) |
| 640×480 (4:3) | Pixel-identical к до-рефакторному билду |
| 1920×1080 (16:9) | HUD анкоры правильные, текст чёткий |
| 2560×1080 (21:9 ultra-wide) | Outer bars слева/справа, HUD на сцене не съехал |
| 480×1920 (портрет) | Letterbox bars сверху/снизу, HUD на сцене не съехал |
| Briefing screen | Картинка + текст в framed 4:3 |
| Save/load экран | Меню в framed 4:3 |
| Pause menu в игре | Сцена за затемнением, меню текст чёткий |
| Won / Lost экран | То же что pause |
| Outro (после прохождения игры) | Wireframe + credits в framed 4:3 |
| Видео (intro Eidos / logo24) | Видео фит'ится, UI не торчит поверх |
| F11 (input debug) | Панель чёткая, рисуется поверх |
| F9 → консоль | Текст чёткий |
| Ctrl debug overlay | AENG_draw_messages чёткий |
| `OC_RENDER_SCALE = 0.5` + `OC_AA_ENABLE = true` | HUD чёткий (UI рисуется в native window pixels мимо scaled FBO) |

### ⚠️ Debug-оверлеи — отдельная матрица тестирования

На 2026-04-25 эти дебаг-шрифты **глючат на разных разрешениях и рендер
скейлах** (известные баги, пока не зафиксены — проявились после
UI/scene split'а):

- **F11 → DualSense sub-page** (input_debug_render_dualsense_page) —
  текст/разметка страницы могут плыть / обрезаться на нестандартных
  разрешениях и при scale ≠ 1.0.
- **Ctrl debug overlay** (AENG_draw_messages) — текст в 3D-пространстве,
  привязан к world-space координатам. При разных разрешениях/скейлах
  позиционирование / размер могут быть некорректными.

**При проверке / починке этих оверлеев обязательно тестировать матрицу:**
- `OC_RENDER_SCALE` = 1.0, 0.5, 0.25
- Разрешения: 640×480 (4:3), 1920×1080 (16:9), 2560×1080 (21:9),
  портрет (e.g. 480×1920)
- Комбинации: разные разрешения × разные scale (9+ комбинаций).

Проблемы скорее всего того же класса что мы уже ловили (FONT-glyph
bounds check, scissor Y-flip, s_vp_* uniform) — искать по тем же
местам. Если нашёл новый класс — записать сюда.

---

## Открытые вопросы (решать в процессе)

1. **GAMEMENU darken** — оставить целиком в post-composition или
   разделить на backdrop (scene FBO) + текст (post-comp)?
   *Гипотеза:* целиком в post-comp ОК, потому что fullscreen UI mode
   растягивается на default FB полностью. Проверить визуально.
2. **`OVERLAY_draw_gun_sights` через FXAA** — gun sights остаются в
   scene FBO значит проходят через FXAA. Это **тонкие линии и круги**
   которые FXAA как раз blur'ит. Если визуально станет хуже — рассмотреть
   перенос gun sights тоже (потребует пересчёта world→native screen
   проекции).
3. **`OVERLAY_draw_enemy_health`** — то же что выше, но это полоска
   а не линия. Меньше страдает от FXAA. Скорее оставить в сцене.
4. **Scene FBO clear** — после рефактора scene FBO больше НЕ содержит
   UI. Между кадрами UI был "грязным" в FBO; теперь не будет. Если
   что-то полагается на это — сломается. Маловероятно, но проверить.

---

## Файлы которые поменяются (примерный список)

- `new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.h` (+API)
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp` (impl)
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp` (callback hook)
- `new_game/src/engine/graphics/graphics_engine/ge.h` (или соседний — экспорт сеттера)
- `new_game/src/game/game.cpp` (удаление вызовов из draw_screen path, регистрация callback'а)
- `new_game/src/game/ui_render.cpp` (новый — UI-пасс)
- `new_game/src/game/ui_render.h` (новый)
- `new_game/src/ui/hud/overlay.cpp` (split OVERLAY_handle)
- `new_game/CMakeLists.txt` (добавить новый файл)

Возможно также:
- `new_game/src/ui/menus/gamemenu.cpp` — если придётся разделять darken/text
