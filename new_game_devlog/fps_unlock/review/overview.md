# FPS Unlock — Review Brief

> **Полное ревью всей работы по FPS unlock относительно `main`.**
>
> Это **не** инкрементальное ревью «что изменилось с прошлого раза» —
> ревьюится **вся** работа по FPS unlock от начала.
>
> Изменения уже **в staging area** относительно ветки `main`
> (репозиторий стоит на `main`, все правки fps_unlock'а лежат в
> `git status`/`git diff --staged`). Ревьюить именно то что в staging.

## Что ревьюить и в каком приоритете

**Приоритет №1 — код.** Корректность каждого изменения, отсутствие
регрессий, соответствие архитектурному принципу задачи (см.
`CORE_PRINCIPLE.md`).

**Приоритет №2 — документация в `new_game_devlog/fps_unlock/`.** На
консистентность, отсутствие устаревших утверждений, отсутствие
противоречий между документами и реальным кодом, отсутствие бреда
после нескольких итераций правок. Список .md файлов в папке
большой — каждый стоит прочитать целиком и проверить что не врёт.

## ⚠️ Установка для ревьюера

1. **Прочитай эти три документа ПЕРВЫМИ — обязательно, до анализа кода:**
   - [`../README.md`](../README.md) — главная точка входа задачи: статус,
     цель, ключевые концепции, файловая карта.
   - [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md) — основной принцип
     задачи. Описывает что считается багом по умолчанию. Любое
     нарушение принципа в коде = баг, не фича.
   - [`../original_tick_rates/summary.md`](../original_tick_rates/summary.md)
     — карта дизайн-частот оригинала (20 Hz physics / 30 Hz visual /
     15 Hz particle). **Любое утверждение про rate в коде или доках
     обязано соответствовать этому документу.** Полное расследование
     с code-citations → [`../original_tick_rates/findings.md`](../original_tick_rates/findings.md).

2. **Прочитай остальные связанные `.md` до конца:**
   - [`../fps_unlock.md`](../fps_unlock.md) — главный технический
     документ (что/почему/пути вперёд).
   - [`../debug_physics_render_rate.md`](../debug_physics_render_rate.md)
     — debug-инструмент (клавиши, overlay).
   - [`../render_interpolation/README.md`](../render_interpolation/README.md)
     + `architecture.md`, `coverage.md`, `known_issues.md`, `plans.md`
     — подзадача интерполяции.
   - [`../fps_unlock_issues.md`](../fps_unlock_issues.md) — открытые баги.
   - [`../fps_unlock_issues_resolved.md`](../fps_unlock_issues_resolved.md)
     — закрытые баги.
   - [`../fps_unlock_physics_visual.md`](../fps_unlock_physics_visual.md)
     — визуальные эффекты в physics-пасе (низкий приоритет).
   - [`../original_tick_rates/research.md`](../original_tick_rates/research.md)
     — исходный research brief.

3. **Не доверяй кратким описаниям этого файла.** Этот документ — карта,
   а не замена ревью. Каждое описание упрощает реальность; реальные
   нюансы только в коде.

4. **Читай каждый затронутый файл целиком.** Не «по диагонали» — каждая
   правка может тащить контекст из соседних строк, не показанных в
   диффе.

5. **Сделай `git diff --staged`** (или `git diff main`, состояние то же)
   и пройдись построчно. Этот файл может опускать мелкие правки.

6. **Прочти git log ветки fps_unlock на удалённой стороне** (если
   доступен) — последовательность коммитов отражает порядок мысли и
   эволюцию решений (`fps unlock pt1` … pt6+).

7. **Test plan — проверять руками.** Компиляция и type-check ловят
   только часть багов; визуальные регрессии видны только в игре.

8. **«Опасные места» — начальный список, не исчерпывающий.** Думай
   сам что могло сломаться. Особенно — пограничные случаи: pause,
   save/load, переходы между состояниями игры.

9. **Не считай что текущее «работает» = «корректно».** Часть багов
   проявляется только при определённых условиях (низкий FPS, high FPS,
   throttling, switch render cap, lag spike). Тестируй edge cases.

10. **Если код выглядит странно — ищи комментарий-обоснование.** В
    проекте принято объяснять _почему_ нетривиальное решение было
    принято. Если объяснения нет — это пробел и кандидат на правку.

## Что задача делает

**Цель:** позволить рендеру бежать на любой частоте (60+ Hz) без
ломки геймплея, который изначально был калиброван под смешанные
30 Hz / 20 Hz / 15 Hz разных подсистем оригинала.

**Решение:** physics декаплен от рендера через accumulator-loop с
фиксированным шагом физики (`UC_PHYSICS_DESIGN_HZ = 20 Hz`,
дизайновая частота оригинала). Render бежит без cap'а
(`g_render_fps_cap = 0` по умолчанию). Между physics-снапшотами
рендер интерполирует положение/углы движущихся Things+камеры
(`RenderInterpFrame` RAII в `draw_screen`) — даёт визуальную
плавность на любом render rate.

Wall-clock эффекты (rain, drip, puddle, ribbon, spark) работают
через 30 Hz accumulator'ы (`UC_VISUAL_CADENCE_TICK_MS = 33.33ms`).
Визуальные `GAME_TURN`-gated эффекты (siren flash, wheel rotation,
sky wibble, etc.) используют отдельный `VISUAL_TURN` (30/sec wall-clock
counter). `GAME_TURN` остаётся как чисто game-state counter
(20/sec physics-tick, паузится на паузе).

Debug-инструмент: клавиши **1/2/3/9/0** для runtime тюнинга
physics/render rate и интерполяции, overlay в левом верхнем углу.

## Канонические rate-константы оригинала

> **Read first:** [`../original_tick_rates/summary.md`](../original_tick_rates/summary.md).

В оригинале **три** разные частоты:

| Константа | Hz | Природа | Что под ней |
|---|---|---|---|
| `UC_PHYSICS_DESIGN_HZ` | **20** | Physics design rate | Thing/Vehicle/Bat/Pow/EWAY/network/PSX gamemenu |
| `UC_VISUAL_CADENCE_HZ` | **30** | Visual calibration rate | Rain/drip/puddle/mist/spark fence/GAME_TURN-gated визуалы |
| `UC_PARTICLE_SCALING_HZ` | **15** | Particle velocity scaling artefact | psystem.cpp local_ratio compensation |

**Любое утверждение про rate — проверять по
`original_tick_rates/findings.md` с code-citations оригинала.**
Не угадывать.

## Базовый принцип задачи

[`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md) — обязателен. Кратко:
визуал + звук → wall-clock; физика + gameplay таймеры → physics-tick;
рендер сам по себе ничему не служит, его частота — debug-параметр.

Любая логика которая зависит от render-frame counter или
`g_render_fps_cap` для не-рендер целей — **баг**, не фича.

## Затронутые файлы кода

Это карта по подсистемам. Каждая ячейка — кандидат на чтение целиком.
Build files (`CMakeLists.txt`, etc.) тоже стоит проверить.

### 1. Game loop / физика — ядро задачи

| Файл | Что |
|---|---|
| [`game/game.cpp`](../../../new_game/src/game/game.cpp) | Accumulator-based physics loop, `lock_frame_rate` на perf-counter (early return при fps≤0), `check_debug_timing_keys`, `RenderInterpFrame` в `draw_screen`, capture loop по 11 movement классам Things + camera, `render_interp_reset` перед main loop, `visual_turn_tick` |
| [`game/game_globals.cpp`](../../../new_game/src/game/game_globals.cpp) + [`.h`](../../../new_game/src/game/game_globals.h) | Глобалы `g_physics_hz` (= `UC_PHYSICS_DESIGN_HZ`), `g_render_fps_cap = 0`, `g_frame_dt_ms`, `VISUAL_TURN`, `visual_turn_tick(dt_ms)` helper |
| [`game/game_types.h`](../../../new_game/src/game/game_types.h) | Канонические rate-константы (см. выше), `extern VISUAL_TURN` рядом с `GAME_TURN`, обновлённые комментарии про TICK_RATIO/TickTock |
| [`things/core/thing.cpp`](../../../new_game/src/things/core/thing.cpp) | `THING_TICK_BASE_MS = 1000/UC_PHYSICS_DESIGN_HZ` (50ms) — заменил оригинальный `#undef NORMAL_TICK_TOCK` + redefine pattern. Hook `render_interp_invalidate(t)` в `free_thing()` (slot reuse fix) |
| [`engine/effects/psystem.cpp`](../../../new_game/src/engine/effects/psystem.cpp) | `PSYSTEM_TICK_BASE_MS = 1000/UC_PARTICLE_SCALING_HZ` (66.67ms) — заменил глобальный `NORMAL_TICK_TOCK` |

### 2. Render interpolation — новый модуль

| Файл | Что |
|---|---|
| [`engine/graphics/render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp) + [`.h`](../../../new_game/src/engine/graphics/render_interp.h) | **Новый модуль.** `RenderInterpFrame` RAII frame-scope, `g_thing_snaps[MAX_THINGS]`, `g_cam_snaps[FC_MAX_CAMS]`, `g_render_alpha`, `g_render_interp_enabled`, capture/invalidate/teleport API, anim-transition детектор (по MeshID/CurrentAnim), optional debug log (`RENDER_INTERP_LOG`) |
| [`CMakeLists.txt`](../../../new_game/CMakeLists.txt) | Подключение `render_interp.cpp`, `debug_timing_overlay.cpp` |

Подробности → [`../render_interpolation/`](../render_interpolation/).

### 3. Debug-инструмент

| Файл | Что |
|---|---|
| [`game/debug_timing_overlay.cpp`](../../../new_game/src/game/debug_timing_overlay.cpp) + [`.h`](../../../new_game/src/game/debug_timing_overlay.h) | **Новый модуль.** Overlay `phys: 20  lock: unlim  IP: on  fps: 144.3` через `FONT2D_DrawString`, регистрируется как diagnostic-overlay callback |
| [`engine/graphics/graphics_engine/backend_opengl/game/core.cpp`](../../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp) + [`engine/graphics/graphics_engine/game_graphics_engine.h`](../../../new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h) | `ge_set_diagnostic_overlay_callback` — overlay из любого present path (main flip, blit, video) |
| [`engine/video/video_player.cpp`](../../../new_game/src/engine/video/video_player.cpp) | Зеркальный switch debug-keys 1/2/3/9/0 для FMV; `lock_frame_rate(g_render_fps_cap)` после видеокадра |
| [`ui/frontend/attract.cpp`](../../../new_game/src/ui/frontend/attract.cpp), [`outro/outro_main.cpp`](../../../new_game/src/outro/outro_main.cpp), [`missions/playcuts.cpp`](../../../new_game/src/missions/playcuts.cpp) | Вызов `check_debug_timing_keys()` в loop'ах |
| [`game/ui_render.cpp`](../../../new_game/src/game/ui_render.cpp) | Включение overlay в render path |

### 4. Wall-clock эффекты (30 Hz visual cadence)

> Все эти эффекты вызываются из render-frame block в `game.cpp`
> с `g_frame_dt_ms`. Внутренний accumulator с шагом
> `UC_VISUAL_CADENCE_TICK_MS`. См. [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md)
> "Паттерн bucket-strobe".

| Файл | Что |
|---|---|
| [`effects/weather/drip.cpp`](../../../new_game/src/effects/weather/drip.cpp) + [`.h`](../../../new_game/src/effects/weather/drip.h) | `DRIP_process(dt_ms)` — 30 Hz accumulator, drip ripple fade/expand |
| [`world_objects/puddle.cpp`](../../../new_game/src/world_objects/puddle.cpp) + [`.h`](../../../new_game/src/world_objects/puddle.h) | `PUDDLE_process(dt_ms)` — 30 Hz accumulator, splash blob decay |
| [`effects/environment/ribbon.cpp`](../../../new_game/src/effects/environment/ribbon.cpp) + [`.h`](../../../new_game/src/effects/environment/ribbon.h) | `RIBBON_process(dt_ms)` — 30 Hz accumulator (Scroll/Y/Life умножаются на `ticks` для multi-tick frames) |
| [`effects/combat/spark.cpp`](../../../new_game/src/effects/combat/spark.cpp) + [`.h`](../../../new_game/src/effects/combat/spark.h) | `SPARK_show_electric_fences(dt_ms)` — wall-clock period accumulator (~1067 ms = 32 visual ticks). Ранее был `GAME_TURN & 0x1f` gate |
| [`engine/graphics/pipeline/aeng.cpp`](../../../new_game/src/engine/graphics/pipeline/aeng.cpp) | `AENG_draw_rain` через bucket-strobe на `UC_VISUAL_CADENCE_TICK_MS`; drip-spawn density по `g_frame_dt_ms`; FPS-каунтер на perf-counter; + сайты `VISUAL_TURN` (object yaw, camera oscillation, legacy FPS counter) |

### 5. Visual GAME_TURN-gated эффекты — миграция на VISUAL_TURN

> `VISUAL_TURN` тикает **30/sec wall-clock** через `visual_turn_tick(g_frame_dt_ms)`
> в main loop'е и playcuts loop'е. Используется для визуальных gating'ов
> которые в оригинале калибровались под 30 Hz `GAME_TURN` (PS1 hardware lock /
> PC config default). `GAME_TURN` теперь чисто game-state — паузится, идёт
> на 20/sec physics-tick.
>
> **Ревью point:** для каждого сайта проверить — это правда визуал, или
> game-state? Game-state на VISUAL_TURN — баг (анимация продолжится на паузе,
> или AI rate-зависим от render). Visual на GAME_TURN — баг (1.5× медленнее
> чем на PS1, паузится).

| Файл | Сайт |
|---|---|
| [`things/vehicles/vehicle.cpp`](../../../new_game/src/things/vehicles/vehicle.cpp) | Siren flash bloom (`VISUAL_TURN<<7`), tire skid track gate (`VISUAL_TURN & 1`) |
| [`engine/audio/sound.cpp`](../../../new_game/src/engine/audio/sound.cpp) | Rain ambient retrigger (`VISUAL_TURN & 0x3f`), wind gust cooldown (tick_tock = VISUAL_TURN + N) |
| [`engine/graphics/pipeline/aeng.cpp`](../../../new_game/src/engine/graphics/pipeline/aeng.cpp) | Object yaw rotations, camera oscillation, legacy FPS counter |
| [`engine/graphics/geometry/sky.cpp`](../../../new_game/src/engine/graphics/geometry/sky.cpp) | Moon/cloud wibble phase |
| [`engine/graphics/geometry/mesh.cpp`](../../../new_game/src/engine/graphics/geometry/mesh.cpp) | Debug walkable colour |
| [`engine/graphics/geometry/figure.cpp`](../../../new_game/src/engine/graphics/geometry/figure.cpp) | Flame animation phases (5 мест) |
| [`engine/graphics/geometry/shape.cpp`](../../../new_game/src/engine/graphics/geometry/shape.cpp) | UV index animation |
| [`engine/graphics/postprocess/wibble.cpp`](../../../new_game/src/engine/graphics/postprocess/wibble.cpp) | Wibble effect phase (passes to GPU shader) |
| [`effects/combat/pyro.cpp`](../../../new_game/src/effects/combat/pyro.cpp) | Bonfire sound gate, flame phase, sprite phase |
| [`combat/combat.cpp`](../../../new_game/src/combat/combat.cpp) | Hit-wave sound variation |
| [`game/game_tick.cpp`](../../../new_game/src/game/game_tick.cpp) | Darci dirt water phase (`VISUAL_TURN * 7`), flashlight colour alternation (`VISUAL_TURN & 0x20`) |
| [`things/characters/person.cpp`](../../../new_game/src/things/characters/person.cpp) | Punch sound variation (×2), spark spawn gate. **Также:** `set_person_fight_step_forward` восстановлен из оригинала (use-without-define из Stage4 миграции, попало в эту ветку случайно) |
| [`things/items/grenade.cpp`](../../../new_game/src/things/items/grenade.cpp) | Trail rotation, path count |
| [`things/items/special.cpp`](../../../new_game/src/things/items/special.cpp) | Bloom rotation phase |
| [`ui/hud/panel.cpp`](../../../new_game/src/ui/hud/panel.cpp) | Минимапа: мерцание dot'а врагов (`PANEL_LSPRITE_DOT` flash). NB: к issue #18 (мерцание истёкшего mission timer'а) **отношения не имеет** — это другое место в panel.cpp около `PANEL_draw_timer`. |

### 6. Прочие правки (gameplay / UI)

| Файл | Что |
|---|---|
| [`game/input_actions.cpp`](../../../new_game/src/game/input_actions.cpp) | Turn rate `max_angle` поднят 128→192 + TICK_RATIO scaling (issue #15) |
| [`ui/hud/overlay.cpp`](../../../new_game/src/ui/hud/overlay.cpp) + [`.h`](../../../new_game/src/ui/hud/overlay.h) | `OVERLAY_begin_physics_tick` — для tracking какие visuals рисуются per-tick |
| [`ui/menus/gamemenu.cpp`](../../../new_game/src/ui/menus/gamemenu.cpp) | Drop dead `extern void process_things_tick(SLONG)` (сигнатура изменилась) |
| [`ui/frontend/frontend.cpp`](../../../new_game/src/ui/frontend/frontend.cpp) | `FADE_SPEED_MAX = 4` clamp (instant fade на первом кадре после загрузки). **Не** фиксит issue #12 (high FPS truncation) — тот остаётся открытым |
| [`ui/frontend/startscr_globals.cpp`](../../../new_game/src/ui/frontend/startscr_globals.cpp) | Minor |
| [`missions/eway.cpp`](../../../new_game/src/missions/eway.cpp) + [`.h`](../../../new_game/src/missions/eway.h) + [`eway_globals.{cpp,h}`](../../../new_game/src/missions/eway_globals.cpp) | Mission timer formula adjustments |

### 7. Изменения в `original_game/`

> Единственная правка в `original_game/` за всю ветку — комментарий-аннотация.

| Файл | Что |
|---|---|
| [`original_game/fallen/Source/Game.cpp:587`](../../../original_game/fallen/Source/Game.cpp#L587) | claude-ai коммент про двойное определение `NORMAL_TICK_TOCK` (глобальный 66ms + локальный override 50ms в Thing.cpp) |

## Test plan (ручная проверка)

### Базовое поведение

- [ ] Игра запускается, overlay виден: `phys: 20  lock: unlim  IP: on  fps: <реальные FPS>`
- [ ] FPS на десктопе при render unlimited — 60+ Hz (зависит от GPU)
- [ ] Hotkeys работают **везде**: in-game, главное меню, attract, FMV (intro), cutscenes, outro
  - **1**: physics 20 ↔ 5 (тогл)
  - **2**: render cap unlimited ↔ 25 (тогл)
  - **3**: интерполяция on ↔ off (тогл)
  - **9**: physics −1 (мин 1)
  - **0**: physics +1 (макс 20)
- [ ] Visual smoothness: с интерполяцией ON движение плавное даже при `physics=5`

### Wall-clock эффекты (МАСТЕР-ТЕСТ — НЕ должны зависеть ни от physics, ни от render rate)

При смене **обеих** клавиш (1 ↔ 9/0 для physics, 2 для render) скорость каждого эффекта **должна оставаться константной** по wall-clock:

- [ ] **Дождь** — плотность одинаковая, не дёргается, выглядит strobe-pattern
- [ ] **Рябь от капель в лужах** — частота возникновения постоянная
- [ ] **Splash от попадания в лужу** — затухание одинаковое по реальному времени
- [ ] **Электрические провода (ribbon)** — амплитуда/частота волн постоянные, scroll скорость
- [ ] **Искры от электрозаборов (spark)** — burst раз в ~1 секунду, ровно, не зависит от Hz
- [ ] **Сирена машин (мигалка)** — частота вспышек константная
- [ ] **Sky / луна / облака** — wibble идёт ровно
- [ ] **Огонь / bonfire** — анимация с постоянной частотой
- [ ] **Touch fire (фонарик Дарси)** — мигание красное/синее ровное
- [ ] **Time-of-day (если влияет)** — переходы день/ночь не ускоряются/замедляются
- [ ] **Звук** — без артефактов синхронизации

> Если **что-то** из списка ускоряется/замедляется при смене physics
> или render rate — это баг. Сообщать с указанием конкретного эффекта
> и при каком значении меняется.

### Mission timer

- [ ] При physics=20 (default) таймер идёт **1:1 с реальным временем**
  (5-секундный тест: реально 5 сек = -5 у game timer)
- [ ] На паузе таймер замораживается, render продолжается
- [ ] При снятии с паузы таймер продолжает с того же значения (нет скачка)

### FMV / cutscenes / attract

- [ ] FMV (intro): A/V sync работает, debug-клавиши тоже работают
- [ ] Cutscene на dev/test картах (cutstest, smoketest, moo) — playback на **20 fps** (cutscene packet rate), не зависит от render cap
- [ ] Attract mode — FPS не аномальный, debug overlay виден
- [ ] Outro (после финальной миссии) — рендерится корректно, debug-клавиши работают

### Pause / save / load

- [ ] Пауза замораживает game-state (включая `GAME_TURN`), render продолжается
- [ ] Снятие с паузы — без визуального скачка/рывка
- [ ] Сохранение и загрузка — без рывка позиции Дарси/NPC, без длинного физического catch-up
- [ ] Сразу после загрузки — wall-clock эффекты не «проматываются» (например drip не должен мгновенно исчезнуть)

### Render interpolation specific

- [ ] Тогл клавишей **3** — видимая разница в плавности на `physics=5`
- [ ] Прыжок Дарси: анимация плавная, без «съёживания» поз (anim-transition детектор)
- [ ] Толпа NPC: не должно быть промельков при spawn'е новых NPC (slot reuse fix; см. известный bug #1 в [`../render_interpolation/known_issues.md`](../render_interpolation/known_issues.md))
- [ ] Третье/первое лицо: тело и камера двигаются согласованно (камера тоже интерполируется)
- [ ] При смерти NPC и появлении нового — нет «прилёта» из старой позы
- [ ] Высокий physics rate (20) + unlimited render — всё работает как раньше
- [ ] Низкий physics rate (5) + unlimited render — мир движется плавно, AI/таймеры адекватные

### Edge cases

- [ ] Переключение render cap unlimited → 25 → unlimited — без stutter'а или с минимальным (см. [`../fps_unlock_issues.md`](../fps_unlock_issues.md) issue #14)
- [ ] При искусственной задержке (например `SDL_Delay(300)` где-нибудь в render) — нет spiral-of-death, мир продолжает работать (clamp 200ms)

## Опасные места (специально проверять на регрессии)

### Audio listener (50ms лаг слышимо)

`MFX_set_listener` обновляется по wall-clock, **до или после** `draw_screen`
(вне `RenderInterpFrame` scope) — должен видеть actual `FC_cam[0]`,
не интерполированный. Проверить что не зашёл случайно внутрь scope —
иначе появится 50мс лаг в позиционном звуке.

### Save/load и render_interp_reset

После загрузки сейва первый physics-tick должен корректно
проинициализировать snapshots без рывков. `render_interp_reset()`
вызывается перед main loop, но повторно после `MEMORY_quick_init` —
нет. Может ли state перенестись с прошлого session'а если на той же
миссии остаёмся? Проверить визуально загрузку из сейва.

### Cutscene cuts (камера-телепорт)

Между кадрами катсцены (особенно при смене focus камеры в EWAY)
`mark_camera_teleport(&FC_cam[0])` нигде не вызывается. Камера
теоретически может «лететь» через сцену один кадр при cutscene cut.
Проверить визуально все катсцены.

### Видео-плеер vs lock_frame_rate

Video player делает PTS-based A/V sync через `SDL_Delay`. Поверх
вызывается `lock_frame_rate(g_render_fps_cap)`. При cap = 0 (unlimited,
default) — `lock_frame_rate` делает early return, всё ок. При cap < native
fps видео — A/V sync дрейфует (это ожидаемое debug-поведение, не регрессия).

### Spiral of death

При просадке производительности (термальный throttling, фоновая задача,
тяжёлый кадр) `physics_acc_ms` может накапливаться. Защита через clamp
`frame_dt_ms = 200ms` есть, но не идеальная. См.
[`../fps_unlock_issues.md` issue #17](../fps_unlock_issues.md). Проверить
стабильность при искусственной задержке.

### `GAME_TURN` vs `VISUAL_TURN` — корректность миграции

Каждый сайт `VISUAL_TURN` (см. таблицу 5 выше) проверить — это правда визуал?
Если влияет на gameplay state (AI, таймер, cooldown, флаг, save/load) —
должно быть на `GAME_TURN`, не `VISUAL_TURN`.

И наоборот: каждый оставшийся `GAME_TURN` сайт — правда game-state?
Если чистый визуал калиброванный под 30 Hz — должен быть на `VISUAL_TURN`.

**Поиск:** `git grep -n GAME_TURN new_game/src/`, `git grep -n VISUAL_TURN
new_game/src/`. Сравнить с таблицей в
[`../original_tick_rates/summary.md`](../original_tick_rates/summary.md).

### Тайминги старых пре-релизных фиксов под 30 Hz

Многие пре-релизные фиксы (см.
[`../../../new_game_planning/prerelease_fixes.md`](../../../new_game_planning/prerelease_fixes.md))
калибровались под 30 Hz physics. Сейчас 20 Hz — таймеры с per-tick
декрементами на 1 идут в 1.5× медленнее. **Частично закрыто** введением
`VISUAL_TURN` (визуальные сайты), но per-tick gameplay таймеры
(`Timer1`/`Timer2` в person.cpp и пр.) остаются на physics rate.
См. [`../fps_unlock_issues.md` issue #16](../fps_unlock_issues.md).

### `g_render_interp_enabled = false` режим

Когда интерполяция выключена клавишей **3** — всё должно работать как
до интерполяции (мир дёргается на physics rate, без артефактов).
Capture продолжает работать; повторное включение должно дать мгновенно
плавность без рывка/скачка.

### Particles / DT_VEHICLE угол

**Не покрыто интерполяцией** (известные ограничения):
- Партиклы (искры, дым, glitter) — отдельный pool, движутся на TICK_RATIO
- Угол поворота машин (`Genus.Vehicle->Draw.Angle`, не в DrawTween) —
  позиция плавная, поворот рывками

См. [`../render_interpolation/known_issues.md`](../render_interpolation/known_issues.md).

### Множество physics-тиков за кадр (lag spike)

Если render отстаёт (>50ms кадр), цикл `while (acc >= step)` сделает
несколько тиков подряд. Capture происходит в конце **каждого** тика —
snap.prev/curr к моменту рендера всегда покрывают **последний** тик.
Это правильно. Проверить искусственным delay в render — не должно быть pop'ов.

### Frontend.cpp — конфликт debug-клавиш

В [`frontend.cpp:2554`](../../../new_game/src/ui/frontend/frontend.cpp#L2554)
есть pre-existing использование `Keys[KB_1..KB_4]` для своей логики
(выбор пункта меню?). Сейчас debug-клавиши в attract используют 1/2/3
(плюс 9/0). Проверить что нет конфликта в frontend меню.

### `RENDER_INTERP_LOG = 1` — debug-вывод включён

В [`render_interp.cpp:11`](../../../new_game/src/engine/graphics/render_interp.cpp#L11)
`#define RENDER_INTERP_LOG 1` пишет в stderr на FIRST_CAPTURE/FREE
events. Включён намеренно — пока активный bug #1 (мельтешащие педестрианы)
не закрыт. **Перед мерджем в main / релизом** — выключить.

### Двойное определение `NORMAL_TICK_TOCK` в оригинале (legacy)

Оригинал имел `NORMAL_TICK_TOCK = 1000/15` глобально (Game.h) и
`#undef + #define 1000/20` локально в Thing.cpp. Это создавало две
reference base'ы одновременно. Мы убрали этот плумбинг — каждая
подсистема теперь имеет явную базу:
- `thing.cpp`: `THING_TICK_BASE_MS = 1000 / UC_PHYSICS_DESIGN_HZ` (50ms)
- `psystem.cpp`: `PSYSTEM_TICK_BASE_MS = 1000 / UC_PARTICLE_SCALING_HZ` (66.67ms)

Глобальный `NORMAL_TICK_TOCK` удалён из `game_types.h`. Проверить
что нигде в коде не осталось ссылок на него.

## Doc consistency check (приоритет №2)

После того как код проревьювен — пройтись по всем `.md` в
`new_game_devlog/fps_unlock/` и проверить:

### На что обращать внимание в документации

- **Упоминания имён переменных:** `g_physics_hz` (не `g_debug_physics_fps`),
  `g_render_fps_cap` (не `g_debug_render_fps`). Старые имена встречаются
  в **исключительно historical context** (например в archived findings
  с резолюциями). Если старое имя подаётся как актуальное — баг доки.
- **Упоминания констант:** `UC_PHYSICS_DESIGN_HZ` / `UC_VISUAL_CADENCE_HZ`
  / `UC_VISUAL_CADENCE_TICK_MS` / `UC_PARTICLE_SCALING_HZ`. Старые
  `UC_ORIG_TICK_HZ` / `UC_ORIG_TICK_MS` / `PHYSICS_STEP_MS` /
  `PHYSICS_TICK_DIFF_MS` удалены из кода — упоминания только в
  исторических документах.
- **Упоминания `NORMAL_TICK_TOCK`:** глобальный define убран,
  заменён на per-subsystem `THING_TICK_BASE_MS` / `PSYSTEM_TICK_BASE_MS`.
  Старое имя только в исторических ссылках на оригинальный код.
- **Упоминания GAME_TURN vs VISUAL_TURN:** документация должна правильно
  разделять — `GAME_TURN` для game-state (паузится, 20 Hz physics-tick),
  `VISUAL_TURN` для визуальных gating'ов (30 Hz wall-clock).
- **Утверждения "physics 30 Hz default" / "30 Hz target":** устарело,
  default — `UC_PHYSICS_DESIGN_HZ = 20`. Если такая фраза встретилась —
  баг доки.
- **Упоминания `PLAYCUTS_PACKET_HZ` / hardcoded "20" в playcuts.cpp:**
  использует `UC_PHYSICS_DESIGN_HZ`. Документация должна это отражать.
- **Test plan / overlay format:** актуальный формат
  `phys: 20  lock: unlim  IP: on  fps: <X.X>`. Старые
  `phys: 1/30 s  lock: 30 fps` — устарели.
- **Список debug-клавиш:** актуальный набор **1/2/3/9/0**. Упоминание
  «1/2/3/4» или отсутствие клавиши **3** — устаревшая дока.

### Что отдельно проверить

- **Cross-references между документами** — не битые ли ссылки.
- **Дублирование содержания** — два документа описывают одну тему
  по-разному (cм. CLAUDE.md «Дублирование — не допускать»).
- **Описания закрытых багов в `fps_unlock_issues.md`** — не помечены ли
  закрытые проблемы как «открытые»; описание соответствует ли реальности.
- **Заголовочные TL;DR в каждом документе** — соответствуют ли реальному
  содержимому документа.
- **Утверждения "уже сделано" / "ещё надо сделать"** — не противоречат ли
  фактическому состоянию кода.

### Какие документы относительно стабильные

Эти документы — research / historical, **в них правок мало или нет**, не
надо их активно править если содержание корректно как research output:

- `original_tick_rates/research.md` — исходный research brief (historical).
- `original_tick_rates/findings.md` — расследование с code-citations
  (research output, ссылается на оригинальный код).

### Какие документы — живые и должны точно соответствовать состоянию кода

- `README.md` — entrypoint, должен быть актуален.
- `CORE_PRINCIPLE.md` — принцип, должен соответствовать тому что в коде.
- `fps_unlock.md` — главный технический документ.
- `debug_physics_render_rate.md` — debug-инструмент.
- `render_interpolation/*.md` — описание интерполяции (архитектура,
  coverage, known issues, plans).
- `fps_unlock_issues.md` / `fps_unlock_issues_resolved.md` —
  должны отражать текущее состояние багов.
- `fps_unlock_physics_visual.md` — список визуальных эффектов в
  physics-пасе.
- `original_tick_rates/summary.md` — actionable выжимка research'а.

## Что НЕ покрыто (известные ограничения)

### Render interpolation (см. [`../render_interpolation/known_issues.md`](../render_interpolation/known_issues.md))

1. **Мельтешащие педестрианы** — изредка промельки при spawn'е новых
   NPC. **Активный баг.**
2. **Поворот машин рывками** — угол в `Genus.Vehicle->Draw.Angle`,
   не покрыт.
3. **Партиклы рывками** — отдельный pool, не покрыт.
4. **Short-path lerp при быстром вращении** — теоретически может
   крутить в обратную сторону на >180°/тик.

### Открытые issues (см. [`../fps_unlock_issues.md`](../fps_unlock_issues.md))

- **#3** Дребезжание HUD таймера миссии (рассинхрон physics-тика и render-кадра)
- **#5** Сирена/мигалка машины — реакция на пробел зависит от physics Hz
- **#7** HUD-маркер прицела — дребезжание + анимация ускоряется от physics Hz
- **#10** Белый туман на земле — ускоряется от render FPS
- **#11** Меню паузы — анимация ускоряется от render FPS
- **#12** Главное меню — fade ускоряется при very high FPS (>120) — `>>3` truncation
- **#13** Кровь под Дарси — растекание ускоряется от render FPS
- **#15** Отзывчивость управления зависит от physics Hz (частично улучшено)
- **#16** Ревизия таймингов старых фиксов под 20 Hz (частично закрыта VISUAL_TURN)
- **#17** Spiral of death защита — текущей достаточно или нет, не подтверждено
- **#18** Скорость мерцания истёкшего таймера зависит от FPS

### Чисто визуальные в physics-пасе (низкий приоритет)

См. [`../fps_unlock_physics_visual.md`](../fps_unlock_physics_visual.md):
- #4 Анимация огня
- #8 Анимация вращения брошенного оружия
- #9 Разлетающиеся листья после пробежки Дарси (borderline)

## Краткий self-test для ревьюера

После прочтения этого файла и связанных документов должно быть понятно:

- ✅ Зачем задача (визуальная плавность на high render rate, без ломки геймплея)
- ✅ Почему три rate'а (20/30/15 Hz) — не один (architecture rationale, см. summary.md)
- ✅ Как architecturally сделано (accumulator + render-interpolation frame-scope + VISUAL_TURN)
- ✅ Какие файлы трогать (код)
- ✅ Что проверить руками (master test — wall-clock invariance)
- ✅ Где могут быть скрытые регрессии
- ✅ Какие документы должны быть консистентны и что в них проверить

Если что-то непонятно — это пробел в документации, имеет смысл его исправить.

## Формат вывода ревью

В конце ревью должно быть:

1. **Список найденных проблем в коде** — упорядоченный по приоритету
   (BUG / РЕГРЕССИЯ / DOC-vs-CODE рассогласование / MINOR / INFO).
   Каждая находка с file:line и кратким описанием что не так.
2. **Список проблем в документации** — устаревшие утверждения,
   противоречия, битые ссылки, дублирование, фактические ошибки.
3. **Test plan checklist** — что проверено руками, что не проверено
   (с указанием почему не проверено если не получилось).
4. **Краткий вердикт** — ветка готова к merge / нужны правки / нужно
   дополнительное обсуждение.

## Линки на детальную документацию

- Базовый принцип → [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md)
- Главный технический документ → [`../fps_unlock.md`](../fps_unlock.md)
- Tick rates оригинала (research) → [`../original_tick_rates/summary.md`](../original_tick_rates/summary.md), [`../original_tick_rates/findings.md`](../original_tick_rates/findings.md)
- Render interpolation → [`../render_interpolation/architecture.md`](../render_interpolation/architecture.md), [`../render_interpolation/coverage.md`](../render_interpolation/coverage.md), [`../render_interpolation/known_issues.md`](../render_interpolation/known_issues.md), [`../render_interpolation/plans.md`](../render_interpolation/plans.md)
- Debug инструмент → [`../debug_physics_render_rate.md`](../debug_physics_render_rate.md)
- Issues → [`../fps_unlock_issues.md`](../fps_unlock_issues.md), [`../fps_unlock_issues_resolved.md`](../fps_unlock_issues_resolved.md)
- Чисто визуальные в physics-пасе → [`../fps_unlock_physics_visual.md`](../fps_unlock_physics_visual.md)
