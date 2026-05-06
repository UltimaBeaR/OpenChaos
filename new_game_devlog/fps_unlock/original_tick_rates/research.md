# Research-задача: герцовки оригинальной кодовой базы Urban Chaos

> **Это research-задача для агента.** Только чтение. Никаких изменений кода.
> Цель — собрать **однозначную карту** дизайн-частот всей оригинальной игры
> чтобы перевод эффектов на wall-clock делался не "на глазок", а с понятным
> baseline'ом для каждого эффекта.

## Контекст и проблема

В новой версии (ветка `fps_unlock` и в целом) wall-clock-эффекты переводятся на
real-time через accumulator с фиксированной cadence (`UC_ORIG_TICK_HZ`). Сейчас
агенты при переводе **гадают** под какую частоту эффект калибровался — берут
"30 Hz" просто потому что PS1 был на 30, или "20 Hz" потому что design rate
физики 20.

На самом деле **в оригинале минимум 3 разных дизайн-частоты** для разных
подсистем (см. ниже). Без чёткой карты "какой эффект под какой rate" — любой
перевод визуально может оказаться неправильным (эффект ускоряется или
замедляется в 1.5×, бывает грубый разброс).

**Цель research'а:** для каждой подсистемы / эффекта оригинальной игры
определить **под какую герцовку калибровались её константы**, и зафиксировать
это в виде таблицы с подтверждением из кода.

После research'а — отдельно с пользователем будем обсуждать **что и как
переводить** на real-time, и какие master-константы вводить в новой кодовой базе.

## Что уже известно (хинты)

### Двойное определение `NORMAL_TICK_TOCK` — оригинал MuckyFoot

- Глобально: `#define NORMAL_TICK_TOCK (1000/15)` = 66ms (15 Hz reference) в
  [`original_game/fallen/Headers/Game.h:485`](../../../original_game/fallen/Headers/Game.h#L485)
- Локально в `Thing.cpp`: `#undef NORMAL_TICK_TOCK; #define NORMAL_TICK_TOCK (1000/20)` = 50ms (20 Hz reference) в
  [`original_game/fallen/Source/Thing.cpp:535-536`](../../../original_game/fallen/Source/Thing.cpp#L535)

Это **архитектурное решение MuckyFoot**, а не наша правка. Скорее всего
исторически: ранний дизайн был под 15 Hz все, потом для thing-physics подняли до
20 Hz, а psystem не подтянули.

### Известные anchor'ы 20 Hz design

- `Thing.cpp` `NORMAL_TICK_TOCK = 1000/20` (выше).
- `playcuts.cpp:526` — `UBYTE env_frame_rate = 20;` (local override глобального
  30 Hz cap'а, специально для cutscene playback).
- EWAY таймер: комментарий "50 \* 20 ticks/sec = 1000 units/sec" в new_game (взят
  из логики оригинала).

### ⭐ Главный эмпирический хинт: реальные замеры HUD таймера миссий

**Пользователь лично измерял** игровой таймер миссий (тот что показывается на
HUD в миссиях на время — например миссия с полицейской машинкой, таймер 1:30).
Сравнивал темп таймера с реальным временем (по секундомеру) на трёх версиях:

| Версия | Physics rate | Поведение HUD таймера миссии |
|---|---|---|
| **PS1** (эмулятор) | ~30 Hz | Таймер игры идёт **быстрее** реального времени (~30 Hz tick) |
| **PC retail** (Steam) / PieroZ v0.2.0 | **22 Hz** (см. ниже) | Таймер игры идёт быстрее реального (но медленнее PS1) |
| **Наш билд при `g_debug_physics_fps = 20`** | **20 Hz** | Таймер игры идёт **ровно 1:1 с реальным временем** ✓ |

**Дополнительное подтверждение для PC retail:** в **самой релизной PC-версии**
(Steam) есть встроенный FPS-counter, и он показывает **~22 FPS** (не 30). То
есть 22 Hz — это **измеренный** factual rate PC retail из самой игры, не
теоретическая оценка. PC retail не успевал держать локnut 30 FPS из-за нагрузки
рендерера, физика тикала медленнее.

**Это самое сильное доказательство** что **20 Hz = design rate** игры. MuckyFoot
рассчитывали миссионные тайминги под честный 1:1 wall-clock, и при 20 Hz это
работает точно. PS1 (~30) и PC retail (22) **оба не достигали** design rate из-за
hardware-ограничений — на PS1 таймер шёл быстрее (тиков больше), на PC retail
тоже быстрее но меньше.

Опираться на это как на ground truth: **20 Hz — design частота для всего что
завязано на game-time** (таймеры, EWAY events, physics, gameplay).

### Известные anchor'ы 15 Hz и 30 Hz

- 15 Hz: глобальный `NORMAL_TICK_TOCK = 1000/15` — используется в `psystem.cpp` и
  всех файлах не делающих override. Скорее всего legacy от ранней разработки.
- 30 Hz: `env_frame_rate = 30` (глобально), `lock_frame_rate(env_frame_rate)` в
  main game loop = hardware render lock на PS1 (= также physics rate на PS1 в
  результате тождества).

### Что компенсируется TICK_RATIO, что нет

- **Компенсируется**: формулы вида `pos += vel * TICK_RATIO >> TICK_SHIFT`. Это
  velocity, движение объектов. При смене tick rate wall-clock-скорость
  сохраняется.
- **НЕ компенсируется**: `Timer1 -= 1`, `Timer2 -= 1`, `GAME_TURN++`,
  условия `(GAME_TURN & 0x1f)`, decrement-counters. Это известное ограничение —
  при смене physics rate анимации/cooldown'ы убывают пропорционально.
- **НЕ компенсируется в wall-clock эффектах**: `DRIP_DFADE = 16` (drip fade
  per call), `Y += 22` (ribbon convect), splash decrement и т.п. — эти
  константы tied к rate at which the function is called.

## Что нужно сделать

### Этап 1 — аудит оригинала

Сканировать **только `original_game/fallen/Source/`** и **`original_game/fallen/Headers/`**.

**Что искать:**

1. Все определения и use-сайты `NORMAL_TICK_TOCK`. Все `#undef NORMAL_TICK_TOCK`
   — это места где MuckyFoot осознанно поменял design rate для модуля.
2. Все use-сайты `TICK_RATIO` и `TICK_SHIFT`. Где формулы velocity-нормирования,
   где другое использование.
3. Все hardcoded "time-like" magic numbers: `1000/15`, `1000/20`, `1000/30`,
   `1000/22`, `33`, `50`, `66`. Контекст каждого — это calibration или
   несвязанное число?
4. Все локальные shadow'ы `env_frame_rate` (как `playcuts.cpp:526`). Где
   ещё MuckyFoot осознанно использовали другой rate для отдельных loop'ов.
5. Структура main game loop в `Game.cpp` — где physics-tick блок
   (`if (should_i_process_game())`), где "Always process these" блок (per
   render frame). Какие функции в каком блоке. Это критично для понимания —
   в оригинале на PS1 разница между physics и render-frame была формальной
   (тождество), но разделение в коде показывает design intent.
6. Все calibration constants в wall-clock эффектах: `DRIP_DFADE`, `DRIP_DSIZE`,
   `s1`/`s2` decrement в puddle, `Scroll`/`Y`/`Life` decrement в ribbon, spark
   gating (`GAME_TURN & 0x1f`), `AENG_NUM_RAINDROPS`, fire animation rate,
   blood pool growth, etc. Где функция вызывалась — physics block или render
   block? Это даёт design rate.
7. Все Timer1/Timer2/GAME_TURN-зависимые тайминги в анимациях/cooldown'ах,
   AI patrol cycles, EWAY scripted events. Под какой rate тестировались
   эти тайминги.

**Полезные грепы:**

```
grep -rn "NORMAL_TICK_TOCK" original_game/
grep -rn "TICK_RATIO" original_game/
grep -rn "env_frame_rate" original_game/
grep -rn "lock_frame_rate" original_game/
grep -rEn "1000\s*/\s*(15|20|22|30)" original_game/
grep -rn "GAME_TURN" original_game/
grep -rn "Timer[12]" original_game/
```

### Этап 2 — карта

Сделать таблицу формата:

| Подсистема / эффект | Calibration Hz | Где вызывается в оригинале | Что подтверждает rate |
|---|---|---|---|
| Thing velocity (NPC, vehicles, ...) | 20 | `process_things_tick` | `Thing.cpp:535-536` local override `NORMAL_TICK_TOCK = 1000/20` |
| Partycle velocity | 15 | `PARTICLE_Run` через global NORMAL_TICK_TOCK | `Game.h:485` global = 1000/15, не override'ится в `psystem.cpp` |
| Cutscene packet playback | 20 | `PLAYCUTS_Play` loop | `playcuts.cpp:526` `env_frame_rate = 20` |
| EWAY mission timer | 20 | `EWAY_process_penalties`, `EWAY_DO_VISIBLE_COUNT_UP` | формула `50 * TICK_RATIO >> TICK_SHIFT` подразумевает 20 ticks/sec |
| Drip ripple fade (DFADE=16, DSIZE=4) | ? | `DRIP_process` per-render-frame в `Game.cpp:2587` | render rate PS1 = 30 → возможно 30. Но проверить — может калибровалось под design 20 |
| Puddle splash decay (s1/s2 -= 1) | ? | `PUDDLE_process` per-render-frame в `Game.cpp:2584` | render rate PS1 = 30 → возможно 30 |
| Rain density (NUM_RAINDROPS=128) | ? | `AENG_draw_rain` per-render-frame | per-frame counter, на PS1 30 раз/сек |
| Ribbon scroll/Y/Life | ? | `RIBBON_process` в physics block (`Game.cpp:2554`) | physics на PS1 = 30, design = 20. Какой имели в виду? |
| Spark gating (GAME_TURN & 0x1f) | ? | `SPARK_show_electric_fences` в physics block (`Game.cpp:2553`) | per-tick gate, GAME_TURN на 20 Hz design = 1.6 сек, на 30 Hz PS1 = 1.07 сек |
| Fire animation | ? | ? | TODO |
| Touch fire / cone (Дарси фонарик) | ? | `GAME_TURN`-параметризованный | per-tick → 20 Hz design |
| Blood pool growth (под Дарси) | ? | ? — найти | TODO |
| White ground fog | ? | `aeng.cpp` — найти | TODO |
| Pause menu animations | ? | `gamemenu.cpp` — найти | TODO |
| Front-end fade speed | ? | `frontend.cpp` (`millisecs >> 3`) | wall-clock based, но clamp подбирался эмпирически |
| Vehicle siren toggle / animation | ? | `vehicle.cpp` — найти | TODO |
| Aim crosshair animation | ? | `panel.cpp`/`overlay.cpp` — найти | TODO |
| ... | ... | ... | ... |

Заполнить **все строки** для каждого эффекта который мы знаем (см. также
`fps_unlock_issues.md` и `fps_unlock_physics_visual.md` — там список багов
с упоминанием rate-зависимых эффектов).

### Этап 3 — выводы и рекомендации

После таблицы — раздел "выводы":

1. **Список аномалий**: где MuckyFoot осознанно использовал разные rate (15
   vs 20 vs 30). Чем объясняется. Какие — design, какие — артефакт.
2. **Группировка эффектов по design rate**: какие эффекты идут на 15, на 20,
   на 30. Категория "неоднозначно — может быть и так и так" — отдельная.
3. **Рекомендации по переводу на wall-clock**: для каждой группы — какой
   baseline rate использовать в new_game. Какие константы как пересчитать.
4. **Single source of truth план**: предложить структуру именованных констант
   (`UC_THING_DESIGN_HZ`, `UC_PARTICLE_DESIGN_HZ`, `UC_VISUAL_*_HZ` и т.п.)
   и где их разместить в new_game.

## Подводные камни

- **`30` не всегда time-related**: HP, distance threshold, color value etc.
  Проверять контекст каждого матча.
- **TICK_RATIO ≠ universal fix**: компенсирует только velocity-формулы
  (через NORMAL_TICK_TOCK). Не компенсирует Timer/GAME_TURN counters,
  не компенсирует константы decay в wall-clock эффектах.
- **PS1 ≠ design**: PS1 рендерил на 30 Hz, но это **hardware lock**, не
  design choice. Design rate в оригинале для thing-physics — 20 Hz (см.
  Thing.cpp override). PS1 просто не достигал design'а из-за тождества
  physics=render. PC retail ~22 Hz — ещё дальше от design.
- **`should_i_process_game()` block в оригинале ≈ render-frame block**: на
  PS1 они тикали с одной частотой (30 Hz), потому что physics=render lock.
  Но **формальное разделение в коде** (physics block vs "Always process
  these" block) показывает design intent — какая категория должна была быть
  rate-независимой.
- **Wall-clock эффекты в оригинале НЕ были wall-clock**: они были tied к
  game loop iteration rate. Когда переводим в real-time через accumulator
  — нужно решить под какой rate "оригинала" эмулировать (30 PS1?
  22 PC retail? 20 design?). Ответ может быть разным для разных эффектов.

## Где начать

1. Прочитать [`../fps_unlock.md`](../fps_unlock.md), [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md),
   [`../fps_unlock_issues.md`](../fps_unlock_issues.md),
   [`../fps_unlock_physics_visual.md`](../fps_unlock_physics_visual.md) — понять
   контекст и список эффектов которые уже идентифицированы как rate-зависимые.
2. Прочитать `original_game_knowledge_base/` — поиск по "tick", "rate", "frame",
   "Timer", "GAME_TURN" — может быть уже есть полезный анализ.
3. Запустить грепы из секции "Полезные грепы" и проанализировать каждый матч.
4. Прочитать `original_game/fallen/Source/Game.cpp` целиком — особенно main
   game loop. Это backbone всей timing-системы.
5. Заполнить таблицу.

## Что НЕ делать

- **НЕ менять код**. Никаких правок. Только чтение оригинала + краткие правки
  в этой папке (новые .md файлы с findings или дополнения к этому).
- **НЕ запускать сборку**. Pure code research.
- **НЕ ограничиваться поверхностным анализом**. Цель — однозначная карта,
  а не "примерно так". Если для какого-то эффекта rate неоднозначен — явно
  написать "неоднозначно, кандидаты: X, Y, Z" с пояснением.
- **НЕ trust комментариям** в текущем new_game/ коде безоговорочно. Многие
  комментарии устарели или были написаны агентами на основе неверных
  предположений. Проверять по оригинальному коду.

## Куда писать результаты

**Все результаты research'а — в отдельный файл рядом с этим:**

📄 [`new_game_devlog/fps_unlock/original_tick_rates/findings.md`](findings.md)

(этот файл сейчас не существует — агент создаёт его сам в начале работы)

Структура файла findings:

1. **Таблица "подсистема → calibration Hz → подтверждение"** (см. шаблон в
   разделе "Этап 2" этого документа). Заполнить максимально полно.
2. **Список аномалий** — где разные части кода MuckyFoot имеют разные design
   rate. Чем объясняется. Какие — design, какие — артефакт.
3. **Группировка эффектов по design rate** — какие на 15, на 20, на 30, какие
   неоднозначны.
4. **Рекомендации по переводу wall-clock эффектов на real-time** — для каждой
   группы какой baseline rate использовать в new_game, какие константы как
   пересчитать.
5. **Single source of truth план** — структура именованных констант
   (`UC_THING_DESIGN_HZ`, etc) и где их разместить.
6. **Открытые вопросы / unknown'ы** — где не получилось однозначно определить
   rate, что нужно проверить экспериментально.

**Этот файл (текущий, `research.md`) — не трогать.**
Это task brief, он остаётся как есть для истории.

После research'а пользователь сядет с агентом и будем по `findings`-файлу
обсуждать дальнейшую стратегию (переводы эффектов, master-константы, etc).
