# Карта дизайн-частот оригинала — выводы (для применения)

> Краткая выжимка для агентов работающих над rate-зависимыми эффектами.
> Полное расследование с code-citations и обоснованиями →
> [`findings.md`](findings.md).

## TL;DR

В оригинале **три разные частоты** для разных подсистем:

| Hz | Что | Природа |
|---|---|---|
| **20 Hz** | Физика, геймплей, EWAY, Thing, Vehicle, Bat, Pow, mission timer | **Design rate**, hardcoded в формулах (`Vehicle.cpp` GRAVITY, `bat.cpp` `/20`, `pow.cpp` `/20`, `Thing.cpp` `NORMAL_TICK_TOCK`, PSX gamemenu `50ms`, network override) |
| **30 Hz** | Drip, puddle, mist, vehicle siren flash, wheel rotation, GAME_TURN-gated визуалы, rain bucket-strobe | **Render-tied legacy** — `env_frame_rate = 30` default из config.ini, под этим тестировались wall-clock эффекты на PS1 |
| **15 Hz** | Particle velocity (`psystem.cpp`) | **Scaling artefact** — глобальный `NORMAL_TICK_TOCK = 1000/15` который не подтянули после введения 20 Hz override |

**"20 Hz везде" в new_game** — корректное упрощение для физики, **некорректное** для wall-clock визуалов и GAME_TURN-gated эффектов (они станут 1.5× медленнее PS1).

---

## Single source of truth — три константы

```cpp
// Design rate физики/геймплея. Захардкожен формулами оригинала.
#define UC_PHYSICS_DESIGN_HZ        20
#define UC_PHYSICS_DESIGN_TICK_MS   50.0f   // = 1000/20

// Render-tied legacy на котором тестировались wall-clock эффекты в
// оригинале (PS1 hardware lock 30 Hz, PC retail config default 30 Hz).
#define UC_VISUAL_CADENCE_HZ        30
#define UC_VISUAL_CADENCE_TICK_MS   33.333f  // = 1000/30

// Particle velocity scaling base (не design rate, scaling artefact).
// Wall-clock-correct через local_ratio compensation в psystem.cpp.
#define UC_PARTICLE_SCALING_HZ      15
```

**Статус (2026-04-30):** все три константы выше уже определены в
[`game_types.h`](../../../new_game/src/game/game_types.h). Старое имя
`UC_ORIG_TICK_HZ` переименовано в `UC_VISUAL_CADENCE_HZ`. См. шаги
1–3 этой задачи и `review/findings.md` для деталей.

---

## Таблица: эффект → calibration Hz → действие в new_game

| Эффект / подсистема | Calibration Hz | Действие в new_game |
|---|---|---|
| Thing velocity (NPC, vehicles, projectiles, barrels, doors, plats) | 20 Hz design | Physics-tick через TICK_RATIO. Уже корректно. |
| Vehicle gravity / suspension / wheels (под капотом) | 20 Hz design (× 4 sub-step = 80 Hz) | Physics-tick. Уже корректно. |
| Bat / Pow / Pyro Thing (фейерверки, fire-balls, splatter) | 20 Hz design | Physics-tick через TICK_RATIO. Уже корректно. |
| EWAY mission timer (`EWAY_time`, `EWAY_tick`, `EWAY_count_up`) | 20 Hz design | Physics-tick. На 20 Hz mission-таймер идёт 1:1 с wall-clock (формула truncation-free). |
| EWAY scripted camera | 20 Hz design | Physics-tick через TICK_RATIO. |
| Cutscene packet playback (`PLAYCUTS_Play`) | 20 Hz design (locked в исходнике) | Уже исправлено через `lock_frame_rate(20)`. |
| Particle velocity (`PARTICLE_T`) | 15 Hz scaling | Physics-tick через `local_ratio`. Не трогать (или унифицировать через пересчёт всех spawn-velocity, не приоритет). |
| Person Timer1/Timer2 cooldowns | 20 Hz design (с оговоркой ¹) | Physics-tick. Если конкретный таймер ощущается криво — добавить TICK_RATIO scaling точечно. |
| **Drip ripple** (fade -= 16, size += 4) | **30 Hz visual** | Wall-clock accumulator с `UC_VISUAL_CADENCE_TICK_MS = 33.33`. |
| **Puddle splash** (s1/s2 -= 1) | **30 Hz visual** | Wall-clock accumulator с `UC_VISUAL_CADENCE_TICK_MS`. |
| **Mist UV-drift / yaw turn** | **30 Hz visual** | Wall-clock accumulator с `UC_VISUAL_CADENCE_TICK_MS`. |
| **Vehicle siren flash** (`(GAME_TURN<<7) & 2047`) | **30 Hz visual** | Wall-clock через `VISUAL_TURN` или прямой `g_frame_dt_ms`. |
| **Bike/car/van wheel rotation** (`(GAME_TURN<<3) & 2047`) | **30 Hz visual** | Wall-clock через `VISUAL_TURN`. |
| **Spark fence gate** (`GAME_TURN & 0x1f`) | **30 Hz visual** | Wall-clock через `VISUAL_TURN` (= ~1 sec gate cycle). |
| **Object yaw rotation** (`(GAME_TURN<<1) & 2047`) | **30 Hz visual** | Wall-clock через `VISUAL_TURN`. |
| **Touch fire** (Дарси фонарик, BLOOM+GAME_TURN) | **30 Hz visual** | Wall-clock через `VISUAL_TURN`. |
| **Aim crosshair animation** | **30 Hz visual** (предположение, требует верификации) | Wall-clock через `g_frame_dt_ms`. |
| **Combat hit-wave RNG** (`GAME_TURN & 3`) | 30 Hz visual | Не критично визуально, на 20 Hz просто реже шум. |
| **Bench health re-enable** (`GAME_TURN & 0x3ff == 314`) | 30 Hz visual (=34 sec) | На 20 Hz = 51 sec. Не критично если не подбирали под 34 sec баланс. |
| **Music alternation, wind gust, heli rotation, bird flapping** (все GAME_TURN-gated) | 30 Hz visual | Wall-clock через `VISUAL_TURN`. |
| **Rain bucket-strobe** (per-render regen) | 30 Hz visual | На `UC_VISUAL_CADENCE_HZ = 30`. |
| **Pause menu animation** (`GAMEMENU_process`) | wall-clock (`GetTickCount`) | `g_frame_dt_ms`. В оригинале уже wall-clock-correct. |
| **Frontend menu fade** (`fade_speed = millisecs >> 3`) | wall-clock (`GetTickCount`) | `g_frame_dt_ms` с float-scaling (избежать `>>3` truncation на 240+ FPS). |
| **HUD mission timer rendering** (`PANEL_draw_timer`) | 20 Hz design (через EWAY) | Уже корректно — отображает значение из EWAY. |
| **Ribbon scroll/Y/Life** | 30 Hz visual ² | Wall-clock 30 Hz strobe (как drip/puddle), либо честно per-physics-tick без вранья в сигнатуре `dt_ms`. |
| **Fire animation `FIRE_process`** | dead code в pre-release ³ | Если в new_game вызывается — проверить семантику отдельно. |

¹ Алгебраически Timer1/Timer2 в `person.cpp` per-tick без TICK_RATIO scaling
(74 occurrences, кроме одного с `* TICK_RATIO >> 8`). Без эмпирических
измерений между нашим билдом и PS1 нельзя 100% сказать был ли calibration
20 Hz design или 30 Hz visual. Сильный косвенный сигнал в сторону 20 Hz
(см. PSX gamemenu hardcode и системные `/20` дивизоры). Дефолт: оставить
20 Hz, точечно фиксить только если конкретное место ощущается криво.

² Ribbon в physics block, но без TICK_RATIO compensation. На PS1 (physics=render
lock 30 Hz) тикал 30 calls/sec. Ground truth calibration MuckyFoot — 30 Hz.

³ `FIRE_process` декларирована в `fire.h:30`, но **никем не вызывается** в
pre-release исходниках (grep по всему `original_game/fallen/Source/`). Возможно
в финальной игре вернули. В new_game если она вызывается — отдельная
проверка нужна.

---

## Группировка по действию

### A. **20 Hz design** — оставить на physics-tick, ничего не править

Thing velocity, vehicles, EWAY (всё), pyro, bat, pow, mission timer, cutscene
packet rate, network game.

**Уже корректно** через `g_physics_hz = UC_PHYSICS_DESIGN_HZ = 20` + TICK_RATIO в
process_things_tick.

### B. **30 Hz visual** — перевести на wall-clock с шагом 33.33ms

Drip, puddle, mist, vehicle siren flash, wheel rotation, GAME_TURN-gated
визуалы, rain bucket-strobe, touch fire, aim crosshair, ribbon, spark fence
gate, music alternation/wind/heli/birds.

**Действие:**
- Wall-clock accumulator с `UC_VISUAL_CADENCE_TICK_MS = 33.33`.
- Для GAME_TURN-gated визуалов — ввести отдельный `VISUAL_TURN` который
  тикает 30/sec wall-clock, отделить от game-state `GAME_TURN` (20/sec
  physics).

### C. **wall-clock через GetTickCount/g_frame_dt_ms** — уже корректно в оригинале

Pause menu, frontend menu fade.

**Действие:** перевести на `g_frame_dt_ms` или `sdl3_get_ticks()`. Семантически
эквивалентно, исправить только truncation-issues на high-FPS (issue #12).

### D. **15 Hz scaling** — оставить как есть

Particle velocity в `psystem.cpp`. Wall-clock-correct через `local_ratio`.
Унификация на 20 Hz требует пересчёта всех `PARTICLE_Add` velocity-arg'ов
× 20/15 — большая работа без визуального выигрыша.

---

## GAME_TURN: разделить game-state и visual cadence

В оригинале `GAME_TURN++` — **per render frame** (`Game.cpp:2683`, после
`lock_frame_rate`). Default = 30/sec, на паузе тикает дальше.

В new_game `GAME_TURN++` перенесён в physics-tick = 20/sec ⇒ **все
GAME_TURN-gated эффекты в 1.5× медленнее PS1**.

**Рекомендуемое решение:**

```cpp
// game-state counter (AI events, scripted timing) — physics rate
SLONG GAME_TURN;        // 20/sec через physics-tick

// visual cadence counter (visual gates, animations) — render legacy rate
SLONG VISUAL_TURN;      // 30/sec через wall-clock accumulator
```

**Перевести на `VISUAL_TURN`** все use-сайты которые касаются визуала:

- `aeng.cpp` wheel/yaw rotations
- `Vehicle.cpp` siren flash
- `Controls.cpp` music alternation, wind gust
- `Combat.cpp` hit-wave RNG
- `facet.cpp`/`superfacet.cpp` crinkles, fence gaps
- `figure.cpp` bird flapping
- `aeng.cpp:11077-11078` heli rotation
- `Darci.cpp:1031` blood/effect gating
- `dirt.cpp:2016` dirt-effect gating

**Оставить на `GAME_TURN`**: AI patrol-cycles, scripted EWAY events (которые
читают GAME_TURN), bench-health timer, и подобное game-state.

---

## Open questions

1. **Person Timer1/Timer2 — 20 Hz или 30 Hz по факту?**
   Алгебраически per-tick, не TICK_RATIO scaled. Системные сигналы → 20 Hz.
   Эмпирически — нужны измерения конкретных механик (например AK47
   continuous fire rate) между нашим билдом и PS1. Дефолт: 20 Hz.

2. **Aim crosshair — где конкретно?**
   `panel.cpp` есть, точное место не верифицировано. Нужен grep по "aim",
   "crosshair", "target" в panel.cpp при правке #7.

3. **Blood pool под Дарси — отдельная подсистема?**
   `TRACKS_AddQuad` для blood-splat'ов — это per-hit spawn, не "growing
   pool". Постоянная лужа может быть отдельной механикой. Нужен focused
   grep при правке #13.

4. **Sky / cloud animation calibration?**
   В `fps_unlock_issues.md` упоминается, но я не нашёл `sky_yaw`/
   `cloud_speed` в `aeng.cpp`. Нужен дополнительный grep если sky тоже
   окажется rate-зависим.

5. **`FIRE_process` dead code в pre-release.**
   В оригинале pre-release не вызывается. Возможно в финальной игре
   вернули. В new_game если вызывается — отдельная семантическая
   проверка.

6. **Particle scale-конверсия 15 → 20.**
   Если унификация — нужно умножить все `PARTICLE_Add` velocity-args на
   20/15 = 1.33×. Большая работа, не приоритет.

---

## Ссылки

- Полное расследование с code-citations и обоснованиями: [`findings.md`](findings.md)
- Исходный research brief: [`research.md`](research.md)
- Архитектурный принцип FPS unlock: [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md)
