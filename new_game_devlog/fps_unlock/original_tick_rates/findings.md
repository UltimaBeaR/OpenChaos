# Дизайн-частоты оригинала — полное расследование

> Code-research по оригинальному источнику Urban Chaos. Подкладка для
> [`summary.md`](summary.md) —
> там выводы и таблица "что куда привязать". Этот файл — обоснования,
> code-citations, аномалии, разбор архитектуры.
>
> Читай этот файл если: (а) сомневаешься в выводах summary, (б) пишешь
> рекомендацию для нового rate-зависимого эффекта который нет в таблице,
> (в) делаешь ревью изменений касающихся tick-rate.
>
> Источник: `original_game/fallen/Source/`, `Headers/`, `DDEngine/Source/`,
> `original_game_knowledge_base/`. Никаких правок кода.

## 1. Архитектура game loop'а

### 1.1. Структура одного render-кадра (PC, [`Game.cpp:2440-2702`](../../../original_game/fallen/Source/Game.cpp#L2440))

```
1. process_controls()                    ← per render frame, всегда
   ├─ MIST_process()                       ← per render frame
   ├─ SPARK_process()                      ← per render frame (≠ SPARK_show_electric_fences)
   ├─ GLITTER_process()                    ← per render frame
   └─ HOOK_process()                       ← per render frame

2. check_pows()                          ← per render frame

3. if (should_i_process_game()) {         ← НЕ в паузе/cutscene/lost/won/tutorial
       process_things(1) → process_things_tick → пересчёт TICK_RATIO
       PARTICLE_Run()
       OB_process()
       TRIP_process()
       DOOR_process()
       EWAY_process()
       SPARK_show_electric_fences()       ← gated by GAME_TURN & 0x1f
       RIBBON_process()
       DIRT_process()
       ProcessGrenades()
       BALLOON_process()
       MAP_process()
       POW_process()
       FC_process()
   }

4. ALWAYS:                                ← даже в паузе
   PUDDLE_process()                       ← s1/s2 -= 1 per call
   DRIP_process()                         ← fade -= 16, size += 4 per call

5. draw_screen()                          ← рендер (включает AENG_draw_rain)
6. OVERLAY_handle()                       ← HUD рисуется
7. CONSOLE_draw()
8. GAMEMENU_draw()                        ← внутри: GAMEMENU_process с GetTickCount() — wall-clock
9. PANEL_fadeout_draw()
10. screen_flip()
11. lock_frame_rate(env_frame_rate)       ← spin-loop, default 30 FPS
12. handle_sfx()
13. GAME_TURN++                           ← per render frame!
14. GF_DISABLE_BENCH_HEALTH check         ← (GAME_TURN & 0x3ff) == 314
```

**Критическое наблюдение:** на PS1 `should_i_process_game()` блок и render
блок тикали с одинаковой частотой (30 Hz hardware lock), потому что
physics=render тождество. Поэтому **формальное разделение блоков в коде
показывает design intent**, не фактическую частоту:

- "physics block" = "функции которые должны останавливаться на паузе"
- "Always process" = "функции которые должны тикать всегда"
- "post-render" = "функции которые тикают раз в видимый кадр"

В new_game с decoupled accumulator эти блоки по-разному масштабируются:

- physics block тикает с `UC_PHYSICS_DESIGN_HZ = 20`
- "Always process" и "post-render" блоки сейчас тикают с render rate
  (unlimited)

### 1.2. Где TICK_RATIO считается

[`Thing.cpp:980-1037`](../../../original_game/fallen/Source/Thing.cpp#L980):

```c
void process_things_tick(SLONG frame_rate_independant)
{
    static SLONG prev_tick = 0;
    SLONG cur_tick;
    SLONG tick_diff;
    static BOOL first_pass = TRUE;

    cur_tick = GetTickCount();
    tick_diff = cur_tick - prev_tick;
    prev_tick = cur_tick;

    if (first_pass) {
        tick_diff = NORMAL_TICK_TOCK;     // ← 50ms (override!)
        first_pass = FALSE;
    }

    if (CNET_network_game)         tick_diff = 1000/20;  // 50ms
    if (frame_rate_independant==0) tick_diff = 1000/25;  // 40ms (dead branch)
    if (record_video)              tick_diff = 40;        // 25 fps

    TICK_TOCK  = tick_diff;
    TICK_RATIO = (TICK_TOCK << 8) / NORMAL_TICK_TOCK;    // base = 50ms
    TICK_RATIO = SmoothTicks(TICK_RATIO);                 // ring buffer 4 frames
    SATURATE(TICK_TOCK, NORMAL_TICK_TOCK>>1, NORMAL_TICK_TOCK*2);   // [25..100ms]
    SATURATE(TICK_RATIO, MIN_TICK_RATIO, MAX_TICK_RATIO);           // [128..384]
}
```

**База = `NORMAL_TICK_TOCK = 50ms` (= 20 Hz)** — через override в
`Thing.cpp:535-536`. То есть `TICK_RATIO = 256` ровно при 50ms tick_diff.

**Use-сайты TICK_RATIO**: 260 occurrences в 43 файлах. Большинство —
velocity-формулы вида `pos += vel * TICK_RATIO >> 8` (wall-clock-stable
относительно 20 Hz design на любом реальном tick rate).

### 1.3. Двойное определение `NORMAL_TICK_TOCK`

| Место | Значение | Используется в |
|---|---|---|
| [`Game.h:485`](../../../original_game/fallen/Headers/Game.h#L485) | `1000/15 = 66.67ms` | Глобальный default. Использует `psystem.cpp` (нет override) |
| [`Thing.cpp:535-536`](../../../original_game/fallen/Source/Thing.cpp#L535) | `1000/20 = 50ms` (override через `#undef`) | Thing.cpp где **реально считается** TICK_RATIO |

**Архитектурное наследие, а не наш баг.** Override был в pre-release
исходниках MuckyFoot.

**Гипотеза эволюции:**
1. Игра изначально проектировалась под 15 Hz (= ранние low-spec PC цели).
2. На каком-то этапе MuckyFoot подняли target до 20 Hz, для thing-physics
   ввели локальный override.
3. **Не подтянули остальные подсистемы** — psystem остался на 15 Hz base.

**Практический вывод:** глобальный 15 Hz — **scaling artefact** для
psystem, не design intent. Particle velocity-units несовместимы с Thing
velocity-units без конверсии (но через local_ratio оба wall-clock-correct).

### 1.4. `process_things_tick` — три hardcoded fallback'а

`Thing.cpp:1001-1022`:

```c
if (CNET_network_game)         tick_diff = 1000/20;  // 50ms — sync 20 Hz
if (frame_rate_independant==0) tick_diff = 1000/25;  // 40ms — dead branch
if (record_video)              tick_diff = 40;       // 25 fps fixed
```

- **Network game = 20 Hz** — явный design choice MuckyFoot для
  multi-player synchronisation. **Подтверждение что 20 Hz — design rate.**
- **frame_rate_independant=0 → 25 fps** — мёртвая ветка, никем не
  вызывается с `(0)`. Все callers используют `process_things_tick(1)`.
- **record_video = 25 fps** — техническая аномалия для записи. Возможно
  синхронизация с PAL video standard (25 fps). Не design rate.

### 1.5. Anchors дизайн-частот в коде (полный список)

| Anchor | Значение | Источник | Что доказывает |
|---|---|---|---|
| `Vehicle.cpp:140-142` | `TICKS_PER_SECOND = 20*4 = 80` | hardcode | Vehicle gravity = 10м/с² ÷ (80 sub-ticks/sec)² ⇒ **20 Hz design hardcoded в формулу gravity** |
| `Thing.cpp:535-536` | `NORMAL_TICK_TOCK = 1000/20 = 50ms` | hardcode override | TICK_RATIO base нормирован на 20 Hz |
| `bat.cpp:1510` | `(BAT_TICKS_PER_SECOND / 20) * TICK_RATIO >> 8` | hardcode `/20` | Bat physics нормирован на 20 ticks/sec |
| `pow.cpp:671` | `(POW_TICKS_PER_SECOND / 20) * TICK_RATIO >> 8` | hardcode `/20` | Pow-up animation нормирован на 20 |
| `eway.cpp:6869` | `EWAY_tick = 5 * TICK_RATIO >> 8` | structural | Truncation-free только при TICK_RATIO=256 (= 50ms = 20 Hz) |
| `eway.cpp:4010` | `EWAY_cam_skip = 100; // Can't skip for 1 second` | comment | 100/EWAY_tick(=5) = 20 ticks = 1s — **только при 20 Hz** |
| `eway.cpp:6929` | `EWAY_count_up += (50 * TICK_RATIO >> 8) * step` | comment "50 * 20 ticks per second i.e. 1000 units/sec" | MuckyFoot прямым текстом пишут "20 ticks per second" |
| `gamemenu.cpp:189` | `millisecs = 50; // 20 fps` (PSX) | hardcode comment | **PSX hardware на 30 Hz, но MuckyFoot явно используют 20 fps как abstract game-tick rate** — самое сильное свидетельство |
| `Game.h:485` | `NORMAL_TICK_TOCK = 1000/15 = 66.67ms` | hardcode | Particle velocity scaling base (artefact, не design rate) |
| `Game.cpp:2169` | `env_frame_rate = ENV_get_value_number("max_frame_rate", 30, "Render")` | config default | Render lock = 30 Hz по умолчанию |
| `Game.cpp:2672` | `lock_frame_rate(env_frame_rate);` | call | Spin-loop в конце render-кадра |
| `playcuts.cpp:526` | `UBYTE env_frame_rate = 20;` (shadow) | hardcode | Cutscene packet playback locked на 20 Hz через локальный shadow глобальной 30 |
| `Attract.cpp:562` | `lock_frame_rate(60);` | hardcode | Frontend меню = 60 Hz "because 30 is so slow i want to gnaw my own liver out rather than go through the menus" (комментарий MuckyFoot) |
| `Thing.cpp:1001` | `if (CNET_network_game) tick_diff = 1000/20;` | hardcode | Network game synced на 20 Hz |
| `Thing.cpp:1022` | `if (record_video) tick_diff = 40;` | hardcode | Video record = 25 fps fixed (PAL?) |

**Сводно:** "20 Hz" появляется **многократно как первичная константа**
(Vehicle GRAVITY формула, bat /20, pow /20, eway 5×, network override,
PSX gamemenu 50ms). "30 Hz" появляется **только** как config-default
для render lock. "15 Hz" — только в Game.h как scaling base. "60 Hz"
— только для frontend меню. "25 Hz" — для video recording.

### 1.6. `GAME_TURN` — где инкрементируется в оригинале

[`Game.cpp:2683`](../../../original_game/fallen/Source/Game.cpp#L2683) —
**per render frame**, **после** `lock_frame_rate(env_frame_rate)`:

```c
lock_frame_rate(env_frame_rate);
handle_sfx();
GAME_TURN++;                          // ← здесь!
if ((GAME_TURN & 0x3ff) == 314) {     // bench health re-enable
    GAME_FLAGS &= ~GF_DISABLE_BENCH_HEALTH;
}
```

**Темп GAME_TURN:**
- PS1: 30/sec (= render = physics lock)
- PC retail (config 30, реально ~22): ~22/sec
- На паузе: продолжает тикать (НЕ gated на `should_i_process_game`)

**Все GAME_TURN-gated эффекты в оригинале калибровались под render rate**,
не physics rate (потому что они по сути тождественны на PS1, но темп GAME_TURN
определяется render-rate'ом).

### 1.7. Что компенсируется TICK_RATIO, что не компенсируется

**Компенсируется (wall-clock-stable через TICK_RATIO):**

- Velocity-формулы: `pos += vel * TICK_RATIO >> 8`
- Movement: `WorldPos.X += DeltaVelocity.X * TICK_RATIO >> 8`
- Acceleration accumulators: `Velocity += accel * TICK_RATIO >> 8`
- Любая формула где умножение на TICK_RATIO с последующим `>> 8`

**НЕ компенсируется:**

- Per-tick decrement counters: `Timer1 -= 1`, `Timer2 -= 1`
- `GAME_TURN++` (одно увеличение в кадр)
- Условия gating: `(GAME_TURN & 0x1f)`, `(GAME_TURN & 0x3ff) == 314`
- Wall-clock decrement в "Always process" эффектах: `DRIP_DFADE = 16`,
  `Y += 22` (ribbon convect), splash `s1/s2 -= 1`
- Per-call float math: mist UV-drift `mp->u += mp->du`

---

## 2. Wall-clock эффекты — детальный разбор

### 2.1. Drip ([`drip.cpp`](../../../original_game/fallen/Source/drip.cpp))

```c
#define DRIP_DFADE 16    // line 40
#define DRIP_DSIZE 4     // line 41
#define DRIP_SFADE 255   // line 37 — start fade

void DRIP_process() {
    for (each drip) {
        fade -= DRIP_DFADE;   // line 122 — per-call, no TICK_RATIO
        size += DRIP_DSIZE;   // line 123
        if (fade < 0) fade = 0;
    }
}
```

**Calibration:** Drip живёт `255/16 ≈ 16` calls. Вызывается из "Always
process" блока в game.cpp = per render frame.

- На PS1 (render 30 Hz): drip живёт 16/30 = 533ms. Это и есть
  "оригинальный визуал".
- На PC retail (~22 Hz): 16/22 = 727ms.
- На наших 20 Hz physics-tick: 16/20 = 800ms (1.5× дольше PS1).
- При unlimited render rate в new_game без bucket-strobe: drip
  исчезает мгновенно.

**Calibration rate:** **30 Hz visual** (default render rate, под который
MuckyFoot тестировали). Не "design rate" — просто render-tied baseline.

### 2.2. Puddle splash ([`puddle.cpp`](../../../original_game/fallen/Source/puddle.cpp))

```c
void PUDDLE_process() {                  // line 810
    for (each puddle) {
        if (s1 > 0) s1 -= 1;             // line 827 — per-call, no TICK_RATIO
        if (s2 > 0) s2 -= 1;             // line 828
    }
}
```

Initial values из `PUDDLE_ripple` (line 633): `s1=40, s2=45`. Затухание
40-45 calls = 1.33-1.50 sec @ 30 Hz, 2.0-2.25 sec @ 20 Hz.

**Calibration rate:** **30 Hz visual** (same reasoning как drip).

### 2.3. Ribbon ([`ribbon.cpp`](../../../original_game/fallen/Source/ribbon.cpp))

```c
void RIBBON_process() {                  // line 44
    for (each ribbon) {
        Ribbons[i].Scroll += Ribbons[i].SlideSpeed;     // line 49 — per-call
        if (Ribbons[i].Flags & RIBBON_FLAG_CONVECT)
            for (each point) Ribbons[i].Point[j].Y += 22;  // line 51
        if (Ribbons[i].Life > 0)
            Ribbons[i].Life--;                            // line 53
    }
}
```

В physics block (`should_i_process_game()`), но без TICK_RATIO. На PS1
(physics=render lock 30 Hz) тикает 30 calls/sec.

**Calibration rate:** **30 Hz visual** — потому что на PS1 physics block
синхронен render rate'у.

### 2.4. Spark fence gate ([`spark.cpp:1271`](../../../original_game/fallen/Source/spark.cpp#L1271))

```c
void SPARK_show_electric_fences() {
    if (GAME_TURN & 0x1f) return;        // line 1271 — fire only every 32 turns
    // spawn sparks at all electric fences
}
```

Gate cycle = 32 GAME_TURN steps:
- PS1 (GAME_TURN @ 30/sec): 32/30 = 1.07 sec/cycle
- PC retail (~22/sec): 32/22 = 1.45 sec/cycle
- Наш билд (GAME_TURN @ 20/sec): 32/20 = 1.6 sec/cycle (1.5× медленнее)

**Calibration rate:** **30 Hz visual** через GAME_TURN.

### 2.5. Mist ([`mist.cpp`](../../../original_game/fallen/Source/mist.cpp))

```c
void MIST_process() {                    // line 337
    for (each MIST_point) {
        mp->u += mp->du;                 // line 347 — per-call float
        mp->v += mp->dv;
        mp->dv -= mp->dv * 0.25F;
        mp->du -= mp->du * 0.25F;
        mp->du -= mp->u  * 0.005F;
        mp->dv -= mp->v  * 0.005F;
    }
}

void MIST_get_start() {                  // line 376
    MIST_get_upto = 0;
    MIST_get_turn += 1;                  // line 379 — per-call gated
}
```

`MIST_process` вызывается из `process_controls` ([Controls.cpp:2435](../../../original_game/fallen/Source/Controls.cpp#L2435))
= per render frame. `MIST_get_start` вызывается из render `AENG_draw`
([aeng.cpp:10844](../../../original_game/fallen/DDEngine/Source/aeng.cpp#L10844))
= per render frame.

**Calibration rate:** **30 Hz visual** (default render rate).

### 2.6. Vehicle siren flash ([`Vehicle.cpp:1399-1421`](../../../original_game/fallen/Source/Vehicle.cpp#L1399))

```c
if (info->FLZ && (vp->Siren == 1)) {
    rx = SIN((SLONG(p_car) + (GAME_TURN<<7)) & 2047) >> 8;     // line 1403
    rz = COS((SLONG(p_car) + (GAME_TURN<<7)) & 2047) >> 8;     // line 1404
    // draw bloom at flash position
}
```

`(GAME_TURN<<7) & 2047` = full SIN cycle every 16 GAME_TURN steps.
- PS1: 16/30 = 0.53 sec/cycle
- Наш билд (GAME_TURN @ 20): 16/20 = 0.80 sec/cycle (1.5× медленнее)

**Calibration rate:** **30 Hz visual** через GAME_TURN.

### 2.7. Wheel rotation ([`aeng.cpp:10111-10133`](../../../original_game/fallen/DDEngine/Source/aeng.cpp#L10111))

```c
AENG_set_bike_wheel_rotation((GAME_TURN << 3) & 2047, PRIM_OBJ_BIKE_BWHEEL);
AENG_set_bike_wheel_rotation((GAME_TURN << 3) & 2047, PRIM_OBJ_VAN_WHEEL);
AENG_set_bike_wheel_rotation((GAME_TURN << 3) & 2047, PRIM_OBJ_CAR_WHEEL);
```

Full rotation = 256 GAME_TURN steps. На PS1: 256/30 = 8.5 sec/rotation.

**Calibration rate:** **30 Hz visual** через GAME_TURN.

### 2.8. Bench health re-enable ([`Game.cpp:2688`](../../../original_game/fallen/Source/Game.cpp#L2688))

```c
if ((GAME_TURN & 0x3ff) == 314) {
    GAME_FLAGS &= ~GF_DISABLE_BENCH_HEALTH;
}
```

1024 GAME_TURN cycle:
- PS1 (30 Hz): 34.1 sec
- Наш билд (20 Hz): 51.2 sec (1.5× медленнее)

**Calibration rate:** **30 Hz visual**.

### 2.9. Pause menu ([`gamemenu.cpp:175-197`](../../../original_game/fallen/Source/gamemenu.cpp#L175))

```c
SLONG GAMEMENU_process() {
    SLONG millisecs;
#ifndef PSX
    static SLONG tick_last = 0;
    static SLONG tick_now = 0;
    tick_now = GetTickCount();
    millisecs = tick_now - tick_last;
    tick_last = tick_now;
#else
    millisecs = 50;    // 20 fps — PSX hardcode
#endif
    SATURATE(millisecs, 10, 200);
    // ... всё что использует millisecs:
    GAMEMENU_background += millisecs >> 0;
    GAMEMENU_slowdown   -= millisecs << 6;
    GAMEMENU_fadein_x   += millisecs << 7;
}
```

**Уже wall-clock в оригинале** через `GetTickCount`. PSX hardcode `50ms`
дополнительно подтверждает 20 Hz design intent.

### 2.10. Frontend menu fade ([`frontend.cpp:4474-4490`](../../../original_game/fallen/Source/frontend.cpp#L4474))

```c
static SLONG last = 0, now = 0;
SLONG millisecs;
now = GetTickCount();
if (last < now - 250) last = now - 250;    // clamp huge gaps
millisecs = now - last;
last = now;
SLONG fade_speed = (millisecs >> 3);       // → 4 при 30 FPS = 120/sec
if (fade_speed < 1) fade_speed = 1;
fade_state += fade_speed;
```

**Уже wall-clock через GetTickCount.** Truncation issue на high-FPS
(issue #12) — `>>3` теряет точность когда `millisecs < 8` (= >120 FPS).

### 2.11. EWAY mission timer ([`eway.cpp:6867-6869`](../../../original_game/fallen/Source/eway.cpp#L6867))

```c
EWAY_time_accurate += 80 * TICK_RATIO >> TICK_SHIFT;     // line 6867
EWAY_time           = EWAY_time_accurate >> 4;            // line 6868
EWAY_tick           = 5 * TICK_RATIO >> TICK_SHIFT;       // line 6869
```

**Алгебраический разбор формулы EWAY_tick:**

`5 * TICK_RATIO >> 8`:
- При 20 Hz (50ms tick, TICK_RATIO=256): `5*256/256 = 5` (truncation = 0)
  → 20 ticks/sec × 5 = **100 units/sec** (точно дизайн)
- При 30 Hz (33ms tick, TICK_RATIO=168): `5*168/256 = 840/256 = 3` (трункация теряет 0.28)
  → 30 × 3 = **90 units/sec** (10% медленнее design)
- При 22 Hz (45ms tick, TICK_RATIO=232): `5*232/256 = 1160/256 = 4` (трункация теряет 0.55)
  → 22 × 4 = **88 units/sec** (12% медленнее design)

**Структурное доказательство:** формула `5 * TICK_RATIO >> 8` truncation-free
**только** при TICK_RATIO = 256, то есть только при 50ms tick (= 20 Hz).
MuckyFoot осознанно выбрали такую формулу чтобы при 20 Hz получить
clean 100 units/sec. На любой другой частоте integer truncation
делает таймер медленнее.

**Эмпирическое подтверждение:** пользователь измерил, что mission timer
1:30 на 20 Hz идёт 1:1 с реальным временем. На PS1 (~30 Hz) — slower
из-за trunc loss. **20 Hz — design rate EWAY** прямо из формулы.

### 2.12. Vehicle gravity (самое сильное доказательство 20 Hz)

[`Vehicle.cpp:140-142`](../../../original_game/fallen/Source/Vehicle.cpp#L140):

```c
#define TICK_LOOP        (4)
#define TICKS_PER_SECOND (20*TICK_LOOP)    // = 80
#define GRAVITY          (-(UNITS_PER_METER*10*256)/(TICKS_PER_SECOND*TICKS_PER_SECOND))
```

GRAVITY = -(units/m × 10 м/с² × 256 fixed-point) / (80 sub-ticks/sec)².

**`20` захардкожен в делителе.** При physics rate ≠ 20 Hz формула gravity
становится численно неверной — машины падали бы на 1.5× медленнее на
30 Hz physics. То что MuckyFoot **зашили `20` в формулу** означает что
**они проектировали vehicle gravity под 20 Hz design tick rate**, и
TICK_LOOP × 20 = 80 sub-ticks/sec — фундамент vehicle physics.

То же в `bat.cpp:1510` и `pow.cpp:671` — `(*_TICKS_PER_SECOND / 20) *
TICK_RATIO >> 8`. **Дивизор `20` системно появляется в нескольких
независимых подсистемах.**

---

## 3. Аномалии и неоднозначности

### 3.1. Двойной NORMAL_TICK_TOCK (15 Hz vs 20 Hz)

См. §1.3. **Артефакт ранней разработки** — psystem не подтянули после
введения 20 Hz override в Thing.cpp. Не ломает корректность (через
local_ratio всё wall-clock-stable), но создаёт две reference base'ы
одновременно.

### 3.2. 25 Hz divisor в `pow.cpp:64`

```c
#define POW_MAX_FRAME_SPEED (POW_TICKS_PER_SECOND / 25)
```

Но в `pow.cpp:671` тот же `POW_TICKS_PER_SECOND` делится на 20:

```c
ticks = (POW_TICKS_PER_SECOND / 20) * TICK_RATIO >> 8;
```

Два разных дивизора в одном файле. **Внутреннее противоречие** — скорее
всего опечатка/leftover. Семантика подразумевает 20 (как в других
TICKS_PER_SECOND-формулах). 25 здесь — клад для какого-то edge-case
(возможно cap на frame-skip), не design rate.

### 3.3. `frame_rate_independant == 0` — мёртвая ветка

`Thing.cpp:1004-1005`:

```c
if (frame_rate_independant==0)
    tick_diff = 1000/25;  // assume 25 fps
```

Поиск `process_things_tick(0)` в исходниках — **0 совпадений**. Все
вызовы `process_things_tick(1)`. Ветка никогда не активируется.

### 3.4. `RIBBON_process` и `SPARK_show_electric_fences` — physics block, no compensation

Обе в physics block (after `should_i_process_game()`), но без TICK_RATIO
и без dt_ms. Темп = темп вызова physics block.

- На PS1 (physics=render lock 30 Hz) = 30 calls/sec.
- На PC retail (~22 Hz) = ~22 calls/sec.

MuckyFoot их видели как часть визуального оформления и не нормировали
формально. **Calibration rate — render rate (30 Hz) на котором они их
тестировали.**

В new_game если оставлять в physics-tick — будут на 20/sec ⇒ замедление.
Лучше перевести на wall-clock 30 Hz (= визуально совпадает с PS1).

### 3.5. `pyro.cpp:1154` — `9 * TICK_RATIO` без `>> TICK_SHIFT`

```c
if (pyro->radii[0] > 2560) pyro->radii[0] += 9 * TICK_RATIO;    // line 1154
pyro->radii[0] += TICK_RATIO;                                    // line 1155
```

Без `>> 8` это даёт 256× скорости относительно "1 unit/tick" базы. То есть
радиус огненного шара "взрывается" на тысячи единиц за тик. Возможно
**намеренная** "explosion expansion" характеристика — масштаб формулы
это подтверждает (radii уходит за 0xFFFF за миллисекунды). Через
TICK_RATIO scaling всё ещё wall-clock-stable, просто абсолютная скорость
в 256× больше "стандартной".

### 3.6. PSX `gamemenu.cpp:189` — `millisecs = 50; // 20 fps`

```c
#ifndef PSX
    millisecs = GetTickCount() - tick_last;
#else
    millisecs = 50;    // 20 fps
#endif
```

PSX hardware на 30 Hz, но MuckyFoot **явно используют 50ms (= 20 fps)**
как abstract game-tick rate для menu animations. Это **самое сильное
свидетельство design intent**: 20 Hz и есть game-time rate, 30 Hz —
просто render lock.

### 3.7. Recording mode — 25 fps fixed

`Thing.cpp:1022`: `if (record_video) tick_diff = 40;` (= 25 fps). Не 20,
не 30. Возможно синхронизация с PAL video standard (25 fps) для записи
демок. **Не design rate**, технический артефакт record-режима.

### 3.8. Frontend меню — 60 Hz hardcode

`Attract.cpp:562`: `lock_frame_rate(60); // because 30 is so slow i want
to gnaw my own liver out rather than go through the menus`. Прямой
комментарий MuckyFoot — 30 Hz для меню "слишком медленно". Frontend
menu animations через `GetTickCount`, так что 60 Hz хардкод тут — про
input responsiveness, не темп анимации.

---

## 4. Person Timer1/Timer2 — анализ неопределённости

74 occurrences `Timer1*/Timer2*` в `person.cpp`. Большинство —
per-tick decrement без TICK_RATIO scaling. Один exception:

```c
// Person.cpp:11681
p_person->Genus.Person->Timer1 += 400 * TICK_RATIO >> TICK_SHIFT;
```

Все остальные — формы:

```c
Timer1 = (Random()&0x1ff)+400;     // set
Timer1 = (Random()&0xff)+100;
Timer1 = 100;
Timer1--;                          // decrement per physics tick
if (--Timer1 == 0) { ... }
Timer1 -= ticks;                   // (один случай: Person.cpp:15724)
Timer1 += 1, += 2, += 5, += 20;    // accumulate
```

**Что значит "Timer1 = 400, Timer1--":**
- При 20 Hz physics: 400 ticks = 20 sec
- При 30 Hz (что MuckyFoot могли видеть на PS1): 400/30 = 13.3 sec
- При 22 Hz (PC retail): 400/22 = 18.2 sec

**Без эмпирических измерений невозможно сказать** какую секунду имели в
виду MuckyFoot. Вариант "20 sec design" vs "13 sec PS1 feel" даёт разную
интерпретацию.

**Косвенные сигналы в сторону 20 Hz design:**
- PSX gamemenu hardcode 50ms = 20 fps
- Vehicle GRAVITY формула с `20*TICK_LOOP`
- Bat, Pow, EWAY systemic `/20`
- Network override 1000/20

**Косвенные сигналы в сторону 30 Hz feel:**
- Default `env_frame_rate = 30` config
- На dev-машинах MuckyFoot скорее всего тестировали при 30 FPS render
- Эмпирическая подгонка значений типа `400, (Random()&0x1ff)+400` это
  "feel-driven" не алгебраически выведенные

**Дефолт:** оставить 20 Hz (design). Если конкретный таймер ощущается
"не так как в оригинале" — добавить TICK_RATIO scaling точечно.
Massовое преобразование без ground truth измерений может сломать больше
чем починить.

**Ground truth можно получить** измерив конкретные механики с известной
оригинальной длительностью между нашим билдом и PS1. Например:
AK47 continuous fire interval (`Person.cpp:6844` `Timer1=0`),
nervous-civ wander timing (`Timer1=(Random()&0x1ff)+400`).

---

## 5. KB cross-check

В `original_game_knowledge_base/` отдельной странички про tick rates
нет. В `DENSE_SUMMARY.md` есть несколько фактов:

- "**lock_frame_rate(30)** — spin-loop, дефолт 30 FPS" (line 22, 121)
- "**TICK_RATIO**: масштаб скорости, **256**=норма, clamp **128-512**;
  **SMOOTH_TICK_RATIO**: кольцевой буфер **4** значения" (line 25)
- "**`NORMAL_TICK_TOCK=1000/15`** = **66.67мс** (15 FPS логики)" (line 119)
- "Vehicle: `TICK_LOOP=4`, `TICKS_PER_SECOND=80`" (line 120)
- "**GAME_TURN++** инкрементируется каждый кадр (не связан с реальным
  временем напрямую)" (Game.cpp:2682-2683 цитата)
- "Bench cooldown: `(GAME_TURN & 0x3ff)==314` → каждые **1024** кадра ≈
  **~34с**" (line 24)

**KB корректно фиксирует render rate = 30 Hz default** и **vehicle
TICKS_PER_SECOND = 80** (косвенно подтверждает 20 Hz). Но **не выделяет
20 Hz как design rate** — это следствие что не было задачи "найти design
rate", KB описывает "что в коде" без интерпретации.

**Ничего в KB не противоречит** выводам данного research'а.

---

## 6. Сводная таблица всех найденных rate-anchors

| Файл:строка | Что | Rate | Тип |
|---|---|---|---|
| `Thing.cpp:535-536` | NORMAL_TICK_TOCK override | 20 Hz | design |
| `Thing.cpp:1001` | network game tick_diff | 20 Hz | design (sync) |
| `Thing.cpp:1005` | frame_rate_independant=0 fallback | 25 Hz | dead branch |
| `Thing.cpp:1022` | record_video tick_diff | 25 Hz | technical |
| `Thing.cpp:1026` | TICK_RATIO formula | (computes from tick_diff) | — |
| `Thing.cpp:1037` | TICK_RATIO clamp | 128-384 | safety |
| `Vehicle.cpp:140-142` | TICKS_PER_SECOND, GRAVITY | 20 Hz × 4 = 80 | design |
| `bat.cpp:1510` | bat ticks divisor | /20 | design |
| `pow.cpp:64` | POW_MAX_FRAME_SPEED | /25 | anomaly |
| `pow.cpp:671` | pow ticks divisor | /20 | design |
| `eway.cpp:6867` | EWAY_time_accurate | implicit 20 (1600 units/sec) | design |
| `eway.cpp:6869` | EWAY_tick formula | structural 20 | design |
| `eway.cpp:4010` | EWAY_cam_skip = 100 | implicit 20 (1 sec) | design |
| `Game.h:485` | NORMAL_TICK_TOCK global | 15 Hz | scaling artefact |
| `Game.cpp:2169` | env_frame_rate config default | 30 Hz | render lock |
| `Game.cpp:2672` | lock_frame_rate(env_frame_rate) | 30 Hz | render lock |
| `Game.cpp:2683` | GAME_TURN++ | render rate (=30) | counter cadence |
| `Game.cpp:2688` | bench health re-enable | 1024/30 ≈ 34s | gated |
| `playcuts.cpp:526` | env_frame_rate shadow | 20 Hz | cutscene-locked |
| `Attract.cpp:562` | lock_frame_rate(60) | 60 Hz | frontend |
| `gamemenu.cpp:189` | PSX millisecs hardcode | 20 fps | design |
| `psystem.cpp:127` | local_ratio formula | 15 Hz base | scaling |
| `drip.cpp:40-41` | DRIP_DFADE/DSIZE | per-call (visual cadence) | render-tied |
| `puddle.cpp:619+` | PUDDLE_ripple s1/s2 | per-call (visual cadence) | render-tied |
| `mist.cpp:347-354` | MIST UV-drift | per-call (visual cadence) | render-tied |
| `mist.cpp:379` | MIST_get_turn++ | per-render-frame | render-tied |
| `ribbon.cpp:49-53` | Scroll/Y/Life | per-physics-tick (= render on PS1) | physics-block visual |
| `spark.cpp:1271` | GAME_TURN & 0x1f gate | render rate gated | render-tied |
| `Vehicle.cpp:1403-1404` | siren flash GAME_TURN<<7 | render rate gated | render-tied |
| `aeng.cpp:9813,12335` | object yaw GAME_TURN<<1 | render rate gated | render-tied |
| `aeng.cpp:10111-10133` | wheel rotation GAME_TURN<<3 | render rate gated | render-tied |
| `aeng.cpp:11077-11078` | heli rotation sin/cos(GAME_TURN) | render rate gated | render-tied |
| `Combat.cpp:2411` | hit-wave GAME_TURN&3 | render rate gated | render-tied |

---

## 7. Связанные документы

- Выводы и рекомендации: [`summary.md`](summary.md)
- Исходный research brief: [`research.md`](research.md)
- Архитектурный принцип FPS unlock: [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md)
- Багов которые касаются rate-зависимых эффектов: [`../fps_unlock_issues.md`](../fps_unlock_issues.md)
