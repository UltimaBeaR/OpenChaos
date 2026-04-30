# FPS Unlock — Review Findings

> Полное ревью staging-изменений ветки `fps_unlock_3` относительно `main`.
> Карта ревью → [`overview.md`](overview.md). Этот документ — только находки.

Приоритет:
- **BUG** — некорректное поведение, которое повлияет на пользователя.
- **REGRESSION** — нарушение по сравнению с предыдущим состоянием кода.
- **DOC-vs-CODE** — расхождение документации с реальным состоянием кода.
- **MINOR** — мелкие неточности кода/комментариев, не критично.
- **INFO** — наблюдение/вопрос, не баг.

## Код

### BUG-1 — Аудио retrigger при render rate > 30 Hz (sound.cpp)

[`engine/audio/sound.cpp:258`](../../../new_game/src/engine/audio/sound.cpp#L258), [`:262`](../../../new_game/src/engine/audio/sound.cpp#L262):
```cpp
if ((VISUAL_TURN & 0x3f) == 0)
    MFX_play_ambient(WEATHER_REF, S_RAIN_START, MFX_LOOPED);
```

`process_weather()` вызывается **раз в render frame**, а `VISUAL_TURN` тикает на 30 Hz wall-clock. При render > 30 fps каждое значение `VISUAL_TURN` "висит" более одного render-кадра ⇒ условие `==0` срабатывает несколько раз подряд (2× при 60 fps, 8× при 240 fps), и `MFX_play_ambient` вызывается несколько раз. На PS1 (30 Hz рендер = 30 Hz GAME_TURN) условие срабатывало строго один раз за цикл.

**`MFX_play_ambient` не идемпотентен** — VISUAL_TURN-агент проверил `mfx.cpp:747`, dedupe нет, каждый вызов рестартит/замешивает звук. Это подтверждённый BUG.

Аналогично (sample, но в physics-tick где меньше критично):
- [`effects/combat/pyro.cpp:482`](../../../new_game/src/effects/combat/pyro.cpp#L482) — `(THING_NUMBER + VISUAL_TURN) & 7 == 0` для bonfire `MFX_play_thing`. `PYRO_fn_normal` в physics-tick (20 Hz), так что VISUAL_TURN сэмплится при 20 Hz а не render — гейт открывается реже, retrigger меньше выражен. Но семантика "ровно раз в N тиков" нарушена (20 Hz сэмплинг 30 Hz счётчика → пропуски/повторы).

**Чинить:** edge-detect — запоминать `last_fire_visual_turn` (как уже сделано в wind-gust блоке [`sound.cpp:269`](../../../new_game/src/engine/audio/sound.cpp#L269) — там `tick_tock` advance делает корректный edge), либо bucket-strobe accumulator (как в `SPARK_show_electric_fences`).

### BUG-2 — `physics_acc_ms = 0` на pause приводит к "rollback" одного тика рендера

[`game/game.cpp:1110`](../../../new_game/src/game/game.cpp#L1110):
```cpp
if (!should_i_process_game()) physics_acc_ms = 0.0;
```

При входе в pause physics_acc_ms сбрасывается в 0 ⇒ `g_render_alpha = 0`. RenderInterpFrame применяет `lerp(prev, curr, 0) = prev` — рендерится **предыдущее** состояние, а не последнее (`curr`). Это видимый "step back" на один physics tick в момент паузы.

**Чинить:** на паузе либо клампать `g_render_alpha = 1.0` (рендерить `curr`), либо не сбрасывать `physics_acc_ms` (если только защититься от unbounded-роста аккумулятора).

### BUG-3 — `RENDER_INTERP_LOG = 1` оставлен включённым

[`engine/graphics/render_interp.cpp:11`](../../../new_game/src/engine/graphics/render_interp.cpp#L11):
```cpp
#define RENDER_INTERP_LOG 1
```

Логирует каждый FIRST_CAPTURE и каждый FREE в stderr. **Уже отмечено в overview.md** как известная точка для отключения перед merge — упоминаю для полноты, чтобы не забыли. Включает запись в `stderr.log`/`crash_log.txt` буфер при каждом `free_thing()`.

### BUG-4 — Сайты заявленные как мигрированные в overview.md, но не модифицированные

Overview таблица (раздел "5. Visual GAME_TURN-gated эффекты — миграция на VISUAL_TURN") перечисляет следующие файлы как мигрированные, но в staging diff их нет:

- [`ui/hud/panel.cpp:1978`](../../../new_game/src/ui/hud/panel.cpp#L1978) — UI flash effect (`fabs(sin(GAME_TURN * 0.2F))`). Overview claim: "закрывает основу issue #18". **Не закрывает** — сайт остаётся на `GAME_TURN`. Анимация мерцания timer'а замедлится в 1.5× по сравнению с PS1 (20 Hz vs 30 Hz).
- [`combat/combat.cpp:990`](../../../new_game/src/combat/combat.cpp#L990) — hit-wave sound `S_PUNCH_START + (GAME_TURN & 3)`. Overview claim: "Hit-wave sound variation". **Сайт остаётся на `GAME_TURN`**.

**Чинить:** либо мигрировать (если решение остаётся за миграцией), либо удалить эти строки из таблицы overview.md / обновить статус в issue #18.

### REGRESSION-1 — `attract` lock_frame_rate был жёстко 60, стал unlimited по умолчанию

[`ui/frontend/attract.cpp:158`](../../../new_game/src/ui/frontend/attract.cpp#L158):
```cpp
- lock_frame_rate(60);
+ lock_frame_rate(g_render_fps_cap);
```

`g_render_fps_cap = 0` ⇒ unlimited. Функционально это правильно (по CORE_PRINCIPLE: `lock_frame_rate` чисто debug-busy-wait), но семантически attract-цикл больше не имеет 60-fps кэпа. Если в attract-меню есть таймер-зависимая логика на render-cadence (issue #11 анимация меню паузы / #12 fade), они теперь фигачат на 240+ FPS. Это ожидаемо в новой модели и описано в issues, **но не регрессия по семантике** — скорее изменение поведения.

> INFO, не BUG: уберите слово "регрессия" если предпочтительно.

### MINOR-1 — `playback_game_keys` единственный путь без `check_debug_timing_keys`

`special_keys` ⇒ `check_debug_timing_keys` срабатывает в основном loop'е, в `attract`/`outro`/`playcuts`/`video_player` зеркала добавлены. Но `do_leave_map_form` и FORM-loop'ы не пробрасываются — debug-клавиши там не работают. Не критично, но overview test plan ожидает "Hotkeys работают везде".

### MINOR-2 — Дублирование констант debug-клавиш в `video_player.cpp`

[`engine/video/video_player.cpp:288-301`](../../../new_game/src/engine/video/video_player.cpp#L288):
```cpp
case SDL_SCANCODE_1:
    g_physics_hz = (g_physics_hz == 20) ? 5 : 20;
```

Hardcoded `20`, `5`, `25`, `1`. Те же значения, что в `check_debug_timing_keys`, но не разделяют constexpr. Если кто-то поменяет `PHYS_HZ_TOGGLE_LOW` в game.cpp — здесь не поправит. Перенести константы в общий header.

### MINOR-3 — `TURN_CAP_RUN/JUMP/CRAWL` именованные, но не в общем header

[`game/input_actions.cpp:1530-1532`](../../../new_game/src/game/input_actions.cpp#L1530) — `static const SLONG TURN_CAP_*` локальные. ОК для одного места, но если их захотят тюнить через config — надо вытащить.

### MINOR-4 — `s_fps_value` алиас нескольких реализаций FPS-readout

Сейчас в коде ТРИ независимых FPS-счётчика:
1. [`game/debug_timing_overlay.cpp:31`](../../../new_game/src/game/debug_timing_overlay.cpp#L31) — `s_fps_value`, sliding window 500 ms (новый, корректный).
2. [`game/ui_render.cpp:144-156`](../../../new_game/src/game/ui_render.cpp#L144) — `cheat == 2` блок, EMA по wall-clock dt (корректный).
3. [`engine/graphics/pipeline/aeng.cpp:5421-5436`](../../../new_game/src/engine/graphics/pipeline/aeng.cpp#L5421) — legacy `AENG_draw_messages`, считает `VISUAL_TURN - last_visual_turn` ⇒ всегда показывает ~30, не реальный render rate. Комментарий честно отмечает "superseded by debug_timing_overlay", но код активен. Если выводится одновременно с `cheat=2` — пользователь увидит 30 vs 144 на одном экране.

**Чинить:** удалить FPS-чтение из `AENG_draw_messages` (переменные `last_visual_turn`, `fps`), либо оставить заглушку без отображения.

### MINOR-5 — `render_interp.cpp:328` модифицирует snapshot-поле `has_angles` при apply

[`engine/graphics/render_interp.cpp:328`](../../../new_game/src/engine/graphics/render_interp.cpp#L328):
```cpp
} else {
    s.has_angles = false;  // local guard for restore path
}
```

Семантически странно — snapshot pure data, но apply-path меняет его флаг. На следующем `capture` всё перезаписывается, поэтому в долгосрочной перспективе ОК, но читаемость страдает. Можно использовать локальную переменную внутри ctor вместо записи в снапшот.

### MINOR-6 — Anim-transition детектор не сбрасывает `last_mesh_id` когда `dt` исчезает

[`engine/graphics/render_interp.cpp:161-167`](../../../new_game/src/engine/graphics/render_interp.cpp#L161):
```cpp
if (dt) {
    if (s.last_mesh_id != dt->MeshID || s.last_anim != dt->CurrentAnim) {
        s.skip_once = true;
        s.last_mesh_id = dt->MeshID;
        s.last_anim = dt->CurrentAnim;
    }
}
```

Если `DrawType` меняется на не-Tween (`dt = nullptr`), `last_mesh_id` остаётся со старого захвата. Когда снова станет Tween, сравнение использует устаревший снимок ⇒ может не задетектить переход. Edge case, проявится только если `Thing` переключает `DrawType` в полёте. Маловероятно.

### MINOR-7 — Комментарий в `special_keys` не упоминает клавишу 3

[`game/game.cpp:775`](../../../new_game/src/game/game.cpp#L775):
```cpp
// Keys 1/2/9/0: debug timing hotkeys — see check_debug_timing_keys.
check_debug_timing_keys();
```

Пропущена клавиша 3 (toggle интерполяции). doc-комментарий выше (lines 626-630) корректный.

### MINOR-8 — Комментарий в `game_globals.h` не упоминает клавиши 2/3

[`game/game_globals.h:60-62`](../../../new_game/src/game/game_globals.h#L60):
```cpp
// Diagnostic hotkeys (1/9/0) wiggle this value at runtime to verify physics<->render
// decoupling
```

Должно быть `1/2/3/9/0` (или хотя бы упомянуть что есть ещё 2/3).

### MINOR-9 — Outdated комментарий "max_frame_rate hardcoded"

[`game/game.cpp:867`](../../../new_game/src/game/game.cpp#L867):
```cpp
// max_frame_rate hardcoded: env_frame_rate default (30) from game_globals.cpp
round_again:;
```

Пережиток до accumulator'а — больше нет hardcoded max_frame_rate в этой функции (`g_render_fps_cap` управляет). Удалить или переписать.

### INFO-1 — Save/load `render_interp_reset` после `MEMORY_quick_init`

После `MEMORY_quick_init()` (в `round_again` лейбле, где идёт смена миссии без выхода из game_loop), `render_interp_reset()` вызывается **до** входа в while-loop, но не повторно после `MEMORY_quick_init`. Поскольку `goto round_again` сбрасывает `physics_acc_ms`, `prev_frame_ms`, и `render_interp_reset()` вызывается прямо перед while ⇒ всё норм. Подтвердить тестом save/load.

### INFO-2 — `lock_frame_rate(20)` в `playcuts.cpp` использует `UC_PHYSICS_DESIGN_HZ`

[`missions/playcuts.cpp:558`](../../../new_game/src/missions/playcuts.cpp#L558):
```cpp
lock_frame_rate(UC_PHYSICS_DESIGN_HZ);
```

Не `g_render_fps_cap` — потому что cutscene packet rate должен быть строго 20. Хорошо, что `g_physics_hz` (debug-tunable) **не** используется тут — иначе бы debug-key 1 ломал cutscene playback. Корректно.

### INFO-3 — `should_i_process_game` имеет недостижимый код после `return UC_TRUE`

[`game/game.cpp:822-825`](../../../new_game/src/game/game.cpp#L822) — dead code preserved as historical reference. Не баг, но стилистически грязно. Существовало до этой ветки.

### INFO-4 — Multi-tick frame & `PARTICLE_Run`

`PARTICLE_Run` использует wall-clock dt через `cur_tick - prev_tick`. При multi-tick frames второй вызов имеет `dt ≈ 0` ⇒ `local_ratio < 256` ⇒ logic skip, только integration позиций. Это нормально (та же семантика, что в оригинале), просто отметка для понимания.

### SUSPICIOUS-1 — `aeng.cpp` `oi->yaw` пишется из render path, читается gameplay-кодом

[`engine/graphics/pipeline/aeng.cpp:3771`](../../../new_game/src/engine/graphics/pipeline/aeng.cpp#L3771):
```cpp
oi->yaw = (VISUAL_TURN << 1) & 2047;   // prim 133 — slowly rotates
```
[`engine/graphics/pipeline/aeng.cpp:5173`](../../../new_game/src/engine/graphics/pipeline/aeng.cpp#L5173):
```cpp
oi->yaw = VISUAL_TURN;                 // prim 235 (warehouse)
```

`oi->yaw` читается `collide.cpp` / `map/mav.cpp` / `pcom.cpp` — это collision/AI-используемое состояние, не чисто визуальное. Запись из render-path по render-clock'у некорректна архитектурно (game-state должен идти от physics-tick'а). Не регрессия от этой ветки (изначально было `GAME_TURN` тоже из render-path в оригинале), но миграция на `VISUAL_TURN` не закрывает этот pre-existing concern. Восстанавливает PS1 30 Hz cadence визуально — это плюс — но семантика "render пишет в game-state" остаётся.

**Чинить (если поднимать):** перенести запись `oi->yaw` в physics-tick path с тем же VISUAL_TURN (читать его, но писать в physics).

### SUSPICIOUS-2 — `figure.cpp` LRU кэш использует GAME_TURN

[`engine/graphics/geometry/figure.cpp:958, 980, 1016, 1027`](../../../new_game/src/engine/graphics/geometry/figure.cpp#L958) — LRU eviction использует `GAME_TURN` как age. Раньше тикал per-render-frame; сейчас per-physics-tick (20 Hz). Функционально работает (relative ordering сохраняется), но render-side кэш по-хорошему должен жить на render-clock'е. Минорно.

### SUSPICIOUS-3 — `person.cpp:13678` SPARK_create в render path под VISUAL_TURN gate

[`things/characters/person.cpp:13678`](../../../new_game/src/things/characters/person.cpp#L13678):
```cpp
if (VISUAL_TURN & 1) { ... SPARK_create(...) }
```

`DRAWXTRA_MIB_destruct` вызывается из `AENG_draw_city` per render frame. Гейт `VISUAL_TURN & 1` открыт ~16.6 ms (полпериода), и при unlimited FPS открыт несколько render-frames подряд ⇒ `SPARK_create` (game-state SPARK pool) спавнится больше раз/сек при высоком render rate. Не регрессия от этой ветки (тот же дефект существовал до миграции с `GAME_TURN`), но `SPARK` объекты — game state, должны быть на physics-tick.

## Документация

> Полный анализ от doc-consistency агента ниже.

### CRITICAL (фактические ошибки в актуальной части доков)

1. **`CORE_PRINCIPLE.md:8-12, 119-128`** — упоминает `NORMAL_TICK_TOCK = 50ms` как текущий локальный override и `NORMAL_TICK_TOCK = 66.67ms` как глобальный. **Удалены из кода.** Заменено на `THING_TICK_BASE_MS` / `PSYSTEM_TICK_BASE_MS`. Заменить упоминания.
2. **`fps_unlock.md:41`** — `Debug-инструмент (клавиши 1/2/9/0)` — пропущена клавиша 3 (toggle интерполяции).
3. **`fps_unlock.md:76, 78-79, 88-99, 108-112`** — несколько утверждений "physics 30 Hz default", "обновляет позиции объектов раз в 33 мс", "`physics_fps=30, render_fps=60`". Default — 20 Hz. Текст устарел.
4. **`fps_unlock_issues.md:1-4`** — title "Проблемы найденные при тестировании debug-инструмента (клавиши 1/2/9/0)". Пропущена 3.
5. **`fps_unlock_issues.md:41-42, 99-106`** — issue #5 / #7 говорят "При нормальной физике (30 Hz) — реагирует". Default — 20 Hz.
6. **`fps_unlock_physics_visual.md:21, 39, 56`** — issues #4/#8/#9 ссылаются на "штатные 30 Hz". Default — 20 Hz.
7. **`original_tick_rates/summary.md:25-26`** — объявляет `UC_PHYSICS_DESIGN_TICK_MS = 50.0f`. Эта константа **не определена** в `game_types.h` (есть только `UC_VISUAL_CADENCE_TICK_MS`). Либо добавить в код, либо убрать из доки.
8. **`README.md:40`** — ссылка `[review/findings.md](review/findings.md)` — на момент ревью этого файла **не существовало** (этот файл — он самый, создаётся прямо сейчас в рамках ревью). После создания ссылка станет валидной.
9. **`README.md:62`** — "Physics по умолчанию 20 Hz (дизайновая частота, NORMAL_TICK_TOCK=50ms)" — должно ссылаться на `THING_TICK_BASE_MS`.
10. **`render_interpolation/architecture.md:8`** — `NORMAL_TICK_TOCK=50ms` — заменить на `THING_TICK_BASE_MS`.
11. **`render_interpolation/plans.md:84`** — то же самое.

### MEDIUM (противоречия между доками)

1. **Default physics rate**: `fps_unlock.md` (78, 89, 108) говорит 30 Hz, остальные доки — 20. fps_unlock.md outlier.
2. **Debug key set**: `fps_unlock.md:41`, `fps_unlock_issues.md:3` — `1/2/9/0`. Остальные — `1/2/3/9/0`.
3. **`NORMAL_TICK_TOCK` lifecycle**: CORE_PRINCIPLE/README/architecture/plans считают живым; overview.md и код — удалён.
4. **Issue #16 status**: `fps_unlock_issues.md` отмечает open, overview.md (line 503) говорит "частично закрыта VISUAL_TURN".

### MINOR (стиль, дублирование)

1. Дубль "тайминги геймплея 1:1" в CORE_PRINCIPLE.md (119-135), fps_unlock.md (117-129), overview.md.
2. Bucket-strobe pattern описан в CORE_PRINCIPLE (139-198) и в abridged-форме в coverage.md, summary.md, overview.md. Принято.
3. `fps_unlock_issues.md` — issue #16 идёт после #18 (порядок).

### Doc → Overview claims которые расходятся с кодом

См. **BUG-4** выше — `panel.cpp:1978` и `combat.cpp:990` перечислены в overview-таблице раздела "5. Visual GAME_TURN-gated эффекты — миграция", но не модифицированы.

## Test plan checklist

Тестов руками **не делал** — это код-ревью без runtime проверки.

Чтобы закрыть ревью, рекомендуется прогнать вручную из overview.md:
- [ ] Master test wall-clock invariance (раздел "Wall-clock эффекты")
- [ ] Pause behavior (BUG-2 — есть подозрение на "step back")
- [ ] FMV / cutscenes / attract debug keys
- [ ] Save/load (INFO-1)
- [ ] Render interpolation toggle (key 3)
- [ ] High render rate (240+ fps) — sound retrigger (BUG-1)

## Краткий вердикт

**Нужны правки** перед merge:
1. **BUG-1** — sound retrigger, либо edge-detect, либо доказать идемпотентность `MFX_play_ambient/thing`.
2. **BUG-2** — pause "step back" — корректно клампать alpha=1 на паузе.
3. **BUG-3** — выключить `RENDER_INTERP_LOG` (если bug #1 в render_interpolation/known_issues закрыт).
4. **BUG-4 / Doc-vs-Code** — либо доделать миграцию `panel.cpp:1978` и `combat.cpp:990`, либо убрать из таблицы overview.md.
5. **DOC fixes** — обновить упоминания `NORMAL_TICK_TOCK` → `THING_TICK_BASE_MS`, выбор клавиш `1/2/3/9/0`, default 20 Hz, `UC_PHYSICS_DESIGN_TICK_MS`.

Архитектура (accumulator-loop, RenderInterpFrame, capture/lerp, VISUAL_TURN отделение) — концептуально корректна и хорошо реализована. Wall-clock эффекты (drip, puddle, ribbon, spark, AENG_draw_rain bucket-strobe) — правильно через accumulator на `UC_VISUAL_CADENCE_TICK_MS`. Главные риски — в edge-cases использования `VISUAL_TURN & MASK == VAL` (множественный retrigger при render >30 Hz).

---

## Подтверждение от VISUAL_TURN-агента (cross-check)

VISUAL_TURN-агент (отдельно прошёлся по миграции GAME_TURN→VISUAL_TURN) подтвердил:

- Все AI cooldowns / mission scripted timers / save-load (`pcom.cpp`, `eway.cpp`, `combat.cpp:133/144/161/340`, `vehicle.cpp:1956`, `psystem.cpp:271`, `darci.cpp:541`, `game_tick.cpp:1183`, `memory.cpp`, `game.cpp:1066` bench-health) — **корректно** оставлены на `GAME_TURN`.
- Все мигрированные визуальные сайты (vehicle siren / tire skid, sky wibble, mesh debug colour, shape sparky UV, wibble post-effect, special bloom, grenade trail/path, person punch sounds, game_tick Darci dirt water + flashlight, figure flames, pyro flame phase, aeng object yaws/camera oscillation) — корректно классифицированы как визуал.
- Sound variation indices (`S_PUNCH_START + (X & 3)`) единообразно классифицируются как визуал по CORE_PRINCIPLE — что подсвечивает несоответствие в `combat.cpp:990` (см. BUG-4).
- `MFX_play_ambient` подтверждённо НЕ идемпотентен (`mfx.cpp:747`) — BUG-1 валиден.
