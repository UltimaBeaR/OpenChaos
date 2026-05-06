# Debug: runtime physics/render rate tuning

Дата: 2026-04-27

## Зачем

Нужен инструмент для изучения влияния частоты физики и рендера на геймплей и производительность.
Конкретная цель — понять при каком соотношении physics/render Hz игра ощущается нормально,
и как это влияет на A/V sync во FMV.

## Что сделано

### Глобалы (`game_globals.h` / `game_globals.cpp`)

```cpp
SLONG g_physics_hz     = UC_PHYSICS_DESIGN_HZ;  // = 20, физика: тиков в секунду
SLONG g_render_fps_cap = 0;                     // рендер: 0 = unlimited (без cap'а)
```

`g_physics_hz` — runtime-меняемый параметр шага физики, default равен
дизайновой частоте `UC_PHYSICS_DESIGN_HZ`. Hotkeys (1/9/0) дёргают его
для отладки decoupling'а — production-default не должен меняться.

`g_render_fps_cap` — debug frame cap (default 0 = unlimited). Использует
`lock_frame_rate(g_render_fps_cap)` во всех render-циклах.

### Клавиши управления

Функция `check_debug_timing_keys()` в `game.cpp` — вызывается из двух мест:
- `special_keys()` — работает в игре
- `game_attract_mode()` — работает в главном меню

Клавиши (без `bangunsnotgames`, доступны всегда — в игре, главном меню, attract,
cutscenes, outro и FMV):
- **1** — toggle physics 20 ↔ 5 (по умолчанию `UC_PHYSICS_DESIGN_HZ` = 20)
- **2** — toggle render cap unlimited ↔ 30 (по умолчанию unlimited, `0` в `g_render_fps_cap`). Также дублируется нажатием на тачпад DualSense.
- **3** — toggle render interpolation on/off (по умолчанию on, `g_render_interp_enabled`)
- **9** — physics −1 (мин. 1)
- **0** — physics +1 (макс. `UC_PHYSICS_DESIGN_HZ`)

Все клавиши работают через `check_debug_timing_keys()` в [`game.cpp`](../../new_game/src/game/game.cpp),
которая вызывается из 4 мест: `special_keys` (in-game), `game_attract_mode`,
`playcuts` (cutscenes), `outro_main`. В [`video_player.cpp`](../../new_game/src/engine/video/video_player.cpp)
сделан зеркальный switch для FMV playback — значения констант должны совпадать.

`lock_frame_rate(fps)` при `fps <= 0` делает early return — frame cap отключён,
рендер бежит на максимуме.

### Overlay в левом верхнем углу

`debug_timing_overlay_render_font2d()` показывает две строки.

**Строка 1 — timing:**
```
phys: <N>  lock: <M> fps  IP: <on/off>  fps: <X.X>
phys: <N>  lock: unlim  IP: <on/off>  fps: <X.X>     // когда render cap = 0
```
- `phys` — текущий physics rate (тиков в секунду, `g_physics_hz`)
- `lock` — render cap в FPS или `unlim` если cap отключён
- `IP` — interpolation enabled (`on`/`off`, `g_render_interp_enabled`)
- `fps` — измеренный wall-clock FPS

**Строка 2 — анимация Дарси (диагностика render-interp):**
```
darci anim=<id> mesh=<id> frame=<N>/<M> tw=<0..255>
```
- `anim` — `Draw.Tweened->CurrentAnim` (ID текущей анимации)
- `mesh` — `Draw.Tweened->MeshID`
- `frame=N/M` — текущий keyframe (1-based) из общего числа в анимации
  (M считается walking по `NextFrame` от `CurrentFrame` до `ANIM_FLAG_LAST_FRAME`,
  плюс `FrameIndex` уже сыгранных)
- `tw` — текущий `AnimTween` (0..255), фракция вертексного морфа

Читается через `NET_PERSON(0)` (Дарси в SP). Overlay вызывается из
`ge_flip` callback'а — после `draw_screen()` и dtor'а `RenderInterpFrame`,
поэтому видим **live физические** значения, не интерполированные.

### Физика: fixed-step accumulator (`game.cpp`)

Заменён старый `process_things(tick_diff)` на accumulator loop:

```cpp
double phys_step_ms = 1000.0 / double(g_physics_hz);
while (physics_acc_ms >= phys_step_ms) {
    process_things(1, phys_tick_diff);
    PARTICLE_Run(); OB_process(); TRIP_process(); ...
    physics_acc_ms -= phys_step_ms;
}
```

`phys_tick_diff = 1000 / g_physics_hz` — целочисленный ms шаг, передаётся в
`process_things`. При default 20 Hz = 50 мс — нормирующая база TICK_RATIO
(`THING_TICK_BASE_MS = 1000 / UC_PHYSICS_DESIGN_HZ`, см. `thing.cpp`).

Канонические rate-константы определены в `game_types.h`:
```cpp
#define UC_PHYSICS_DESIGN_HZ      20    // physics design rate
#define UC_VISUAL_CADENCE_HZ      30    // visual cadence (drip/puddle/rain/VISUAL_TURN)
#define UC_VISUAL_CADENCE_TICK_MS (1000.0f / float(UC_VISUAL_CADENCE_HZ))
#define UC_PARTICLE_SCALING_HZ    15    // psystem scaling artefact
```

### Render lock

`lock_frame_rate(g_render_fps_cap)` вызывается:
- в основном game loop (уже было через `lock_frame_rate`)
- в `video_player.cpp` после `ge_video_draw_and_swap` — новое, чтобы замедлять видео тоже

**Замечание по видео:** у video player есть PTS-based A/V sync с `SDL_Delay`. `lock_frame_rate`
добавляет сверху дополнительное ожидание. Если lock_fps < fps видео — A/V sync дрейфует.
Это ожидаемо для debug режима.

### Overlay: отображение текущих значений (`debug_timing_overlay.cpp`)

`debug_timing_overlay_render_font2d()` рисует строку:

```
phys: 20  lock: unlim  IP: on  fps: 144.3
phys: 20  lock: 25 fps  IP: on  fps: 24.9    // когда render cap = 25
```

Через `FONT2D_DrawString` с `DEBUG_FONT_SCALE = 384` (1.5× от нормального размера FONT2D).
Координаты: (4, 4) — левый верхний угол. Цвет: жёлтый (0xffff00).

Регистрируется как diagnostic-overlay callback через
`ge_set_diagnostic_overlay_callback()` — рисуется из любого present path
(main flip, blit, video player), не зависит от game state.

```cpp
POLY_frame_init(UC_FALSE, UC_FALSE);
FONT2D_DrawString((CBYTE*)tbuf, 4, 4, 0xffff00, DEBUG_FONT_SCALE);
POLY_frame_draw(UC_FALSE, UC_FALSE);
```

## Нерешённая проблема: overlay не рендерится в initial attract mode

**Симптом:** overlay не виден на экране при первом запуске игры (главное меню сразу после
старта). После того как зайти в игру и выйти обратно в меню — overlay рендерится нормально.

**Что проверялось:**
- Текстура `multifontPC.tga` (TEXTURE_page_font2d) загружается в `TEXTURE_load_needed` —
  вызывается при старте (`game.cpp:289`) ДО перехода в `game_attract_mode`.
- `FONT2D_init(TEXTURE_page_font2d)` тоже вызывается там же.
- `POLY_init_render_states()` (через `POLY_frame_init` в конце `TEXTURE_load_needed`) должен
  корректно прописать хендл в render state страницы `POLY_PAGE_FONT2D`.
- `POLY_reset_render_states()` перед `POLY_frame_init` в overlay — не помогло.
- Координаты и viewport идентичны в обоих состояниях (пользователь подтвердил).

**Временное решение:** текст в левом верхнем углу (4, 4) — там гарантированно помещается.
Right-justification убрана именно из-за этой неопределённости с рендером.

**Куда копать если захочется разобраться:**
- Что происходит с GL-хендлом для `TEXTURE_page_font2d` между `TEXTURE_load_needed` и
  первым кадром `game_attract_mode`. Возможно `FRONTEND_init` → `FRONTEND_scr_new_theme`
  делает что-то с текстурами.
- Почему второй заход в `game_attract_mode` (после game exit) работает — что именно
  происходит во время `TEXTURE_load_needed` для миссии, что не происходит для frontend.ucm.
- `POLY_frame_draw` с alpha-sort: убедиться что `POLY_PAGE_FONT2D` (alpha-blended)
  правильно попадает в sorted pass и рендерится с правильным `p->page`.

## Затронутые файлы

| Файл | Изменение |
|------|-----------|
| `game/game_globals.h` + `.cpp` | `g_physics_hz`, `g_render_fps_cap`, `VISUAL_TURN`, `visual_turn_tick()` |
| `game/game_types.h` | `UC_PHYSICS_DESIGN_HZ`, `UC_VISUAL_CADENCE_HZ`, `UC_VISUAL_CADENCE_TICK_MS`, `UC_PARTICLE_SCALING_HZ`, `extern VISUAL_TURN` |
| `game/game.cpp` | fixed-step accumulator, `check_debug_timing_keys`, render lock, `visual_turn_tick` |
| `game/debug_timing_overlay.cpp` + `.h` | overlay через `ge_set_diagnostic_overlay_callback` |
| `engine/video/video_player.cpp` | `lock_frame_rate` после каждого видеокадра, mirror клавиш 1/2/3/9/0 |
| `ui/frontend/attract.cpp` | вызов `check_debug_timing_keys()` в loop |
| `outro/outro_main.cpp` | вызов `check_debug_timing_keys()` в loop |
| `missions/playcuts.cpp` | вызов `check_debug_timing_keys()` в loop, `visual_turn_tick` |
