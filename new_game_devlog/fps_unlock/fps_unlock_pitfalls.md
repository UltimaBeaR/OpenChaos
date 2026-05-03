# FPS-unlock pitfalls — что ломается при отвязке эффектов от FPS

> **Памятка живая.** Если при работе над FPS-unlock'ом находишь новый класс
> бага — дописывай в этот файл сразу. Цель: чтобы на следующем эффекте
> сразу проверять весь чеклист, а не натыкаться на те же грабли.

> Обязательно прочитать перед любой правкой эффекта в рамках FPS-unlock:
> [`CORE_PRINCIPLE.md`](CORE_PRINCIPLE.md) (что куда привязывать) +
> этот файл (типичные ошибки).

## Базовый чеклист «правильный wall-clock эффект»

Когда переписываешь эффект под FPS-unlock — пробежать по списку:

1. **Spawn rate** — в чём gated? Если в render path и через level-trigger
   (`(SOMETHING & 1)`, `if (some_per_frame_counter % N)`) — это **не** rate
   gate, фаерит per-render-frame пока условие true. Нужен **edge-detect**
   через wall-clock bucket: `cur_phase = sdl3_get_ticks() / interval_ms`,
   фаер только при `cur_phase != last_phase`.
2. **Per-effect lifetime** — счётчик типа `counter += K` или `--die` тикает
   per-physics-tick? Если **K** (или `--`) фиксированный, lifetime будет
   масштабироваться с physics rate, не wall-clock. Применить
   `* TICK_RATIO >> TICK_SHIFT` к шагу для wall-clock-correctness.
3. **Per-frame state ticks (Pos, counter, animation)** — где обновляются?
   Если в render-path функции (типа `process_controls()` в render-loop) —
   будут масштабироваться с FPS. Либо перенести в physics tick, либо
   wall-clock-gate в render path (`sdl3_get_ticks() / step` edge-detect).
4. **Render-time random** — `rand()` / `Random()` в коде, который вызывается
   per-render-frame? Каждый кадр — новое значение, эффект «дрожит» с FPS.
   Заменить на детерминированный hash от **identity + wall-clock bucket**
   (см. `spark_noise` в [`spark.cpp`](../../new_game/src/effects/combat/spark.cpp)).
5. **Hash inputs** — если используешь bucket-based hash, **inputs** должны
   быть стабильными в bucket'е! Если seed = endpoint coords, а endpoints
   render-interp'ятся — hash меняется per frame. Используй **identity**
   (slot index, segment index), не coords.
6. **Self-contained 1-frame визуал** (dlight, mark-and-remove pattern) —
   не gating'уй! Эти эффекты нормализуются автоматически: 1 instance per
   frame на любом FPS = 100% lit time. Gating даст строб с FPS-зависимой
   плотностью.
7. **Random параметры в render-path** (radius, color, position offset) —
   даже если эффект сам ungated, рандомные параметры будут меняться
   per-frame → видимая FPS-bound пульсация. Использовать
   bucket-stable noise.

## Конкретные грабли по каждому пункту

### 1. Level-triggered гейт вместо edge-detect

**Симптом:** на high FPS spawn density растёт линейно с FPS, lens flares
overdraw, plotность объектов растёт.

**Пример A — counter-based level gate:** `if (VISUAL_TURN & 1) spawn();` —
`VISUAL_TURN` тикает 30 Hz wall-clock, гейт true в течение ~33ms каждые 67ms.
На render 60 FPS гейт виден 2 кадрами per cycle → 2 спавна. На 280 FPS —
~5 спавнов per cycle. Density = render_fps / 2.

**Пример B — probabilistic per-frame gate:** `if (!(Random() & 7)) spawn();` —
выглядит как throttle ("1/8 attempts"), но это **тоже** level-trigger, просто
со стохастическим успехом. На каждом render frame делается **новая** попытка.
Density = render_fps × P(success) = render_fps / 8. На 30 FPS оригинала
≈ 3.75 spawn/sec; на 240 FPS ≈ 30 spawn/sec (8× больше).
Канонический пример — PYRO_BONFIRE smoke spawn в `pyro.cpp` PYRO_draw_pyro.

**Распознавание:** любое условие в render-path функции, которое управляет
spawn'ом и зависит **только** от random/per-frame counter (не от wall-clock,
не от per-instance edge-detect state) — level-trigger. Чинится одинаково.

**Фикс:** edge-detect через UBYTE per-thing field:
```cpp
constexpr SLONG SPAWN_INTERVAL_MS = 67;  // 15 Hz
UBYTE cur_phase = UBYTE(sdl3_get_ticks() / SPAWN_INTERVAL_MS);
bool spawn_fire = (cur_phase != p_thing->LastSpawnPhase);
if (spawn_fire) p_thing->LastSpawnPhase = cur_phase;
if (spawn_fire) { /* spawn */ }
```

**Multi-instance:** `static last_phase` НЕ годится — первый thing «съедает»
phase, остальные пропускают. Хранить per-thing.

### 2. Lifetime масштабируется с physics rate

**Симптом:** на низком physics rate эффект живёт **дольше**, накапливается
больше одновременно живых instance'ов → over-bright additive blending.

**Пример:** PYRO_TWANGER `counter += 16` per physics tick, free at `>= 240`
= 15 ticks = 0.75s @ 20Hz / 3s @ 5Hz. Spawn fixed 15 Hz wall-clock →
population 11 vs 45 → 4x lens flare brightness.

**Фикс:** `counter += (K * TICK_RATIO) >> TICK_SHIFT` — total per-second
константа 320/sec на любой physics rate, lifetime = 0.75s wall-clock.

**Скобки!** clang ругается false-positive если `var += A * B >> C` без
скобок (из-за `+=`). Писать `var += (A * B) >> C`.

### 3. Tick'и в render-path функциях

**Симптом:** счётчики/позиции эффекта обновляются с FPS-частотой.

**Пример:** `process_controls()` зовётся **per render frame**. Внутри
`SPARK_process()` делал `Pos += dx>>4; --die;` per call → искры умирали
за 0.1s @ 280 FPS, за 1.5s @ 25 FPS. И двигались тоже FPS-кратно.

**Фикс — два варианта:**
- **Wall-clock gate в render path** (предпочтительно для визуальных
  эффектов): обернуть вызов в `if (cur_phase != last_phase) { ... }`
  по wall-clock bucket. Эффект остаётся графической подсистемой, но
  рейт фиксирован. Например 15 Hz cadence для MIB destruct.
- **Перенос в physics tick** (только для game-state эффектов): добавить
  вызов в physics tick loop в game.cpp. **НЕ для визуала** —
  CORE_PRINCIPLE требует чтобы визуал был render-attached.

#### 3a. Маскировка под dt: переменная-«тактовая константа» в render-frame accumulator'е

**Особо коварная подформа #3.** Код выглядит как корректный wall-clock
accumulator (`var += dt`), но на деле `dt` — не реальный wall-clock delta,
а **constant** physics-tick value, обновляемый раз в physics tick. На
render-frame частоте `var += K` (фиксированный K), значит accumulation rate
= render_fps × K → масштабируется с render FPS.

**Пример:** `sea_offset += tick_tock_unclipped` в `AENG_draw_city`. Имя
`tick_tock_unclipped` звучит как dt, но это TICK_TOCK = `tick_diff` физики
(= 50 ms на 20 Hz, не меняется между render frames). На 30 FPS оригинала
= 1500 inc/sec, на 240 FPS = 12000 inc/sec. Также зависело от physics rate
(на 5 Hz physics tick_tock = 200 → 4× быстрее).

**Распознавание:** если `XYZ += K_per_tick` происходит в render-path коде,
а K — physics-tick константа (TICK_TOCK / tick_tock_unclipped / TICK_RATIO /
любая «per-tick» переменная) — это баг. Имя обманывает.

**Фикс:** прямой wall-clock derivation, без накопления:
```cpp
// На 30 FPS оригинала: 30 × K_per_tick increments/sec.
// Заменяем накопитель на функцию от sdl3_get_ticks().
constexpr SLONG PERIOD = ...; // период sin/cos в единицах var
const SLONG var = SLONG((sdl3_get_ticks() * RATE_PER_SEC / 1000) % PERIOD);
```
Modulo выбрать так, чтобы `var % PERIOD` сохранял phase identity (для формул
`(var >> N) & MASK` это PERIOD = (MASK+1) << N).

### 4. Render-time `rand()`

**Симптом:** форма / цвет / позиция эффекта «дрожит» с FPS-частотой.

**Пример:** `SPARK_find_midpoint` использовал `rand() % dist` для случайного
смещения зигзага молнии. Каждый render frame midpoint пересчитывался с
новым `rand()` → bolt visibly twitched at render FPS.

**Фикс:** Детерминированный hash:
```cpp
static SLONG spark_noise(SLONG s1, SLONG s2, SLONG s3, SLONG salt) {
    uint32_t h = uint32_t(s1) * 0x9E3779B1u;
    h ^= uint32_t(s2) * 0x85EBCA77u;
    h ^= uint32_t(s3) * 0xC2B2AE3Du;
    h ^= uint32_t(salt) * 0x27D4EB2Fu;
    h ^= h >> 16;
    h *= 0x85EBCA77u;
    return SLONG(h & 0x7FFFFFFFu);
}
SLONG bucket = sdl3_get_ticks() / BUCKET_MS;
SLONG offset = spark_noise(identity, 0, bucket, salt) % range;
```

В bucket'е hash стабилен → визуал стабилен. Между bucket'ами hash меняется → visible animation/wiggle на bucket cadence (= 15 Hz wall-clock на любом FPS).

### 5. Hash inputs зависят от render-interp'нутых полей

**Симптом:** даже с bucket-based hash эффект всё равно FPS-bound.

**Пример:** seed = endpoint coords → но endpoints `SPARK_TYPE_LIMB`
получаются через `Draw.Tweened->AnimTween` (render-interp), меняются
каждый кадр. Bucket был, но input менялся → output тоже менялся per
frame. Получили render-rate wiggle.

**Фикс:** seed = **identity**, не coords. Identity = stable per-instance:
slot index, segment index, sub-index. Передавать через параметры функций
если нужно.

### 6. Не gate'ить self-contained 1-frame эффекты

**Симптом (если зегейтить такое):** строб с FPS-зависимой плотностью.

**Пример:** dlight (NIGHT_dlight_create + REMOVE flag) живёт 1 render
frame. Если spawn раз в 67ms (15 Hz) при render 30 FPS → 50% кадров
освещены, при 300 FPS → 5% кадров → строб.

**Правило:** если эффект существует ровно 1 кадр (immediate-mode-style),
**spawn'ить per render frame без гейта**. Один экземпляр per frame на
любом FPS = постоянный визуал.

### 7. Random параметры в ungated render-path вызовах

**Симптом:** даже без gating'а самого вызова, **рандомные аргументы**
к нему меняются per frame → визуал пульсирует с FPS.

**Пример:** dlight ungated (правильно), но `radius = 90 + Random() & 0x1f`
менялся каждый кадр → world brightness flicker rate = render FPS.

**Фикс:** вместо `Random()` использовать bucket-stable noise:
```cpp
const SLONG bucket = SLONG(sdl3_get_ticks() / 67);
uint32_t h = uint32_t(bucket) * 0x9E3779B1u;
h ^= h >> 16; h *= 0x85EBCA77u;
SLONG radius = 90 + SLONG(h & 0x1f);  // меняется 15 Hz wall-clock
```

## Workflow при отладке нового эффекта

1. **Сначала логирование, потом фикс.** Считать ВСЕ rate'ы (call/sec,
   spawn/sec, tick/sec, draw/sec) на cap'ed и unlimited FPS, найти что
   масштабируется. Не угадывать.
2. **Изоляция**: временно `#if 0` отдельные блоки эффекта, чтобы
   подтвердить какой именно компонент пользователь видит как FPS-bound.
3. **Проверять чеклист 1-7 выше** для каждого render-path вызова в эффекте.
4. **После фикса — перезапустить логирование на обоих FPS** для верификации.

## Конкретные исправленные эффекты (история)

| Эффект | Симптом | Корень | Фикс |
|---|---|---|---|
| MIB destruct PYRO+SPARK spawn | density растёт с FPS | `(VISUAL_TURN & 1)` level gate | edge-detect через `LastDestructSpawn` UBYTE field |
| MIB destruct dlight strobe (после первого фикса) | flicker rate = FPS | spawn per-frame с random radius | `radius` через bucket-noise |
| SPARK lifetime + Pos motion | искры на 280 FPS живут 0.1s, на 25 FPS 1.5s | `SPARK_process` per render frame | wall-clock-gate (15 Hz) в render path |
| SPARK zigzag wiggle | форма меняется с FPS | `rand()` в `SPARK_find_midpoint` per render frame | bucket-based deterministic hash, seed = identity |
| PYRO_TWANGER lens flare overdraw на низкой физике | over-bright на 5 Hz | lifetime physics-tick-bound, spawn wall-clock | `counter += (K * TICK_RATIO) >> TICK_SHIFT` |
| PYRO_BONFIRE smoke spawn | density растёт с render FPS (8× на 240 FPS vs 30 FPS) | `if (!(Random() & 7))` per-frame — probabilistic level-trigger | edge-detect через `Pyro::LastSmokeSpawn` UBYTE field, bucket = ~33ms (UC_VISUAL_CADENCE_TICK_MS) |
| MIST UV wibble (ground fog) | скорость анимации scales с render FPS (8× на 240 FPS) | `MIST_get_turn += 1` per render frame в `MIST_get_start`, фаза = `MIST_get_turn / divisor` | заменён frame counter на `sdl3_get_ticks() / UC_VISUAL_CADENCE_TICK_MS` (smooth wall-clock phase, мёртвый глобал удалён) |
| Vehicle engine smoke (взорванная машина) | density растёт с render FPS (8× на 240 FPS) | `if ((Random() & 0x7f) > Health)` per render frame — probabilistic level-trigger | edge-detect через `Vehicle::LastSmokeSpawn` UBYTE field, bucket = ~33 ms (UC_VISUAL_CADENCE_TICK_MS) |
| Water surface waves (Stern's Revenge sea_offset) | скорость волн scales с render FPS И с inverse physics rate | `sea_offset += tick_tock_unclipped` per render frame — accumulator с physics-tick constant маскированный под dt (см. #3a) | заменён на `sdl3_get_ticks() * 3 / 2 % 4096` (1500 inc/sec wall-clock = 30 FPS оригинала, modulo гладкий) |
| POLY_wibble (underwater distortion) | скорость scales с render FPS, зависела от physics rate через TICK_RATIO | `POLY_wibble_turn += 256 * TICK_RATIO >> TICK_SHIFT` per render frame | `sdl3_get_ticks() * 7680 / 1000 % (1024*2048)` — wall-clock, modulo = clean wrap для любого integer g |
| Blood splut growth (TRACKS_DrawTrack) | растекание лужи крови 8× быстрее на 240 FPS | `walk->splut++` per render frame, splut → splutmax определяет размер | bucket-delta через `Track::LastSplutPhase` UBYTE field, bucket = 50 ms (UC_PHYSICS_DESIGN_HZ — original calibration rate). Init phase = current bucket в spawn'е — delta=0 на первый draw |

См. также [`fps_unlock_issues.md`](fps_unlock_issues.md) Issue #19 для полной хронологии MIB destruct.
