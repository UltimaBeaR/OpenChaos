# FPS Unlock — решённые проблемы

Проблемы найденные при тестировании fps_unlock и уже исправленные.

Контекст → [README.md](README.md) | [fps_unlock_issues.md](fps_unlock_issues.md)

---

## 1. ✅ Меню: скорость авто-пролистывания зависит от render FPS

**Симптом:** при зажатии кнопки навигации в меню — пункты пролистываются быстрее
при более высоком `g_render_fps_cap`. Должно быть независимо от частоты рендера.

**Причина:** авто-пролистывание считало количество рендер-кадров (или physics-тиков)
вместо реального времени. При 60 FPS render — срабатывало вдвое чаще чем при 30.

**Место:** `frontend.cpp`, `FRONTEND_input` — таймер `dir_ticker` (счётчик кадров).

**Фикс:** переменная `dir_ticker` (счётчик кадров) заменена на `dir_next_fire`
(wall-clock таймер через `sdl3_get_ticks()`). Авто-пролистывание теперь срабатывает
через фиксированный интервал в миллисекундах, независимо от FPS.

---

## 2. ✅ Дождь и капли на лужах: скорость зависит от render FPS и physics FPS

**Симптом:**
- **Дождь (струи):** при увеличении `g_render_fps_cap` становился быстрее.
  При нормальном 30 FPS render — нормальная скорость.
- **Капли на лужах:** ускорялись и от `g_render_fps_cap`, и от `g_physics_hz`.
  То есть зависели сразу от обеих частот.

**Ожидаемое поведение:** оба эффекта — чисто визуальные (анимационные), это **не**
физическая симуляция. Скорость и капель и дождя должна зависеть **ТОЛЬКО** от
wall-clock дельтатайма. Ни от render FPS, ни от physics FPS.

**Причина:**
- Дождь был привязан к рендер-кадрам (обновлялся каждый draw-вызов).
- Капли на лужах — частично к рендер-кадрам, частично к physics-тикам (что-то
  обновлялось в `process_things`, что-то в draw-пасе).

**Место:** отрисовка дождя (rain/shape), анимация капель на лужах.

**Фикс:** оба эффекта переведены на wall-clock дельтатайм (`g_frame_dt_ms`),
полностью отвязаны от render-кадров и physics-тиков.

---

## 3. ✅ Анимация мерцания сирены машины не зависит от physics Hz

**Симптом (был):** скорость мерцания мигалки/сирены полицейской машины зависела
от `g_physics_hz` — чем медленнее физика, тем медленнее мигает. Должна идти по
wall-clock как чисто визуальный эффект.

**Статус:** проверено пользователем 2026-05-03 — мерцание сейчас работает
корректно, скорость не зависит от physics Hz. Закрыто прошлыми фиксами
(вероятно частью миграции визуальных `GAME_TURN`-gated сайтов на `VISUAL_TURN`,
см. issue #16). Точная commit-точка не зафиксирована.

**Связанная open-проблема:** toggle сирены (нажатие пробела) на низких
physics Hz регистрируется хуже —
помечено won't fix в [fps_unlock_issues.md #5](fps_unlock_issues.md).

---

## 4. ✅ Костёр (PYRO_BONFIRE): плотность дыма растёт с render FPS

**Симптом:** дым над костром (smoke particles) спавнится плотнее на высоком
render FPS. На 30 FPS — нормальная редкая россыпь, на 240 FPS — заметно гуще
и больше клубов одновременно. Сама анимация пламени костра (`draw_flames`)
от FPS не зависит — там фиксированное `iNumFlames`.

**Причина:** в `PYRO_draw_pyro` case `PYRO_BONFIRE` спавн smoke particles был
гейтован через `if (!(Random() & 7)) PARTICLE_Add(...)` per render frame.
Это **probabilistic level-trigger** (см. [fps_unlock_pitfalls.md #1](fps_unlock_pitfalls.md)
пример B): каждый render frame — новая попытка с вероятностью 1/8 успеха.
Density = render_fps × P(success) = render_fps / 8. На 30 FPS оригинала
≈ 3.75 spawn/sec, на 240 FPS ≈ 30 spawn/sec.

**Место:** [`pyro.cpp` PYRO_draw_pyro](../../new_game/src/effects/combat/pyro.cpp), case `PYRO_BONFIRE`.

**Фикс:** добавлено per-pyro поле `UBYTE LastSmokeSpawn` в `struct Pyro`
([pyro.h](../../new_game/src/effects/combat/pyro.h)). Гейт обёрнут в
wall-clock edge-detect: bucket = 33 ms (~UC_VISUAL_CADENCE_TICK_MS, 30 Hz),
`Random() & 7` теперь делается ровно раз в bucket → ~3.75 spawn/sec на любом
render FPS, идентично оригиналу на 30 FPS. Per-pyro state — несколько
костров в сцене не делят bucket. `init_pyros` делает `memset(0)` → корректная
начальная инициализация. Подтверждено пользователем 2026-05-03.

---

## 5. ✅ Белый туман на земле (MIST): скорость UV wibble зависит от render FPS

**Симптом:** анимация низкого стелющегося тумана — движение/wibble UV-смещений
текстуры ускорялась при увеличении `g_render_fps_cap`. На 30 FPS — нормальная
скорость, на 240 FPS — в 8× быстрее.

**Причина:** в `MIST_get_start()` ([mist.cpp](../../new_game/src/effects/weather/mist.cpp))
был чистый per-render-frame counter: `MIST_get_turn += 1;`. Функция зовётся
из render-path в `aeng.cpp`. Затем в `MIST_get_detail()` фаза вычислялась как
`yaw_odd = float(MIST_get_turn) / (...)` → sin/cos для UV offsets. Прямой
линейный scaling с FPS.

**Место:** [`MIST_get_start`](../../new_game/src/effects/weather/mist.cpp) и
[`MIST_get_detail`](../../new_game/src/effects/weather/mist.cpp).

**Фикс:** убран per-frame инкремент, `MIST_get_turn` глобал удалён (был мёртвый
код). Фаза теперь считается из wall-clock:
`turn_phase = float(sdl3_get_ticks()) / UC_VISUAL_CADENCE_TICK_MS`. Эквивалентно
оригинальному 30 Hz frame counter, но wall-clock-bound и **smooth** (не stepped)
— на high FPS получаем sub-tick interpolation бесплатно. Подтверждено
пользователем 2026-05-03.

---

## 6. ✅ Дым из капота взорванной машины: density растёт с render FPS

**Симптом:** разрушенная машина пускает чёрный дым из капота. На high FPS
поток гуще (плотнее облака per second), на low FPS — реже. Тот же класс
бага что и дым костра (#4).

**Причина:** в `draw_car()` ([vehicle.cpp](../../new_game/src/things/vehicles/vehicle.cpp))
спавн particles был гейтован только через `if ((Random() & 0x7f) > Health)` —
probabilistic level-trigger per render frame (см.
[fps_unlock_pitfalls.md #1](fps_unlock_pitfalls.md) пример B). При Health=0
вероятность ~99% → почти каждый render frame спавн. Density = render_fps × P(success).
На 240 FPS dim 8× плотнее чем на 30 FPS оригинала.

**Место:** [`draw_car`](../../new_game/src/things/vehicles/vehicle.cpp), строка ~750.

**Фикс:** добавлено per-vehicle поле `UBYTE LastSmokeSpawn` в `struct Vehicle`
([vehicle.h](../../new_game/src/things/vehicles/vehicle.h)). Спавн обёрнут в
wall-clock edge-detect (bucket = 33 ms ≈ UC_VISUAL_CADENCE_TICK_MS, 30 Hz):
`Random()`-гейт пробуется ровно раз в bucket → density на 30 FPS оригинала
сохранена, на любом другом FPS то же. Per-vehicle phase — несколько разрушенных
машин не делят bucket. `reinit_vehicle` инициализирует поле в 0. Подтверждено
пользователем 2026-05-03.

---

## 7. ✅ Колышущаяся вода (Stern's Revenge etc.): скорость волн scales с render FPS и physics rate

**Симптом:** водная поверхность (видно особенно в начале миссии Stern's Revenge)
колышется быстрее на высоком render FPS. Также скорость зависела от physics
rate — на низкой физике (debug-клавиша 1, 5 Hz) волны замедлялись.

**Причина:** в `AENG_draw_city()` ([aeng.cpp](../../new_game/src/engine/graphics/pipeline/aeng.cpp))
было `static int sea_offset = 0; sea_offset += tick_tock_unclipped;` per render
frame. Затем `sea_offset` использовался в формуле модуляции Y-координат
water-вершин: `world_y += (COS((x + sea_offset >> 1) & 2047) + SIN(...)) >> 13`.

`tick_tock_unclipped` звучит как dt, но это **constant** physics-tick ms
(50 ms на 20 Hz physics), не wall-clock delta. Поэтому
`sea_offset` per second = render_fps × tick_tock_ms — на 30 FPS оригинала
1500/sec, на 240 FPS 12000/sec (8× быстрее). Дополнительно — на 5 Hz physics
tick_tock = 200 → 4× быстрее даже без смены render rate.

**Место:** [`AENG_draw_city`](../../new_game/src/engine/graphics/pipeline/aeng.cpp),
строки ~2037 (declaration), ~2044 (accumulation), ~2203 (use).

**Фикс:** удалены `static int sea_offset = 0;` и `+= tick_tock_unclipped;`.
Теперь `const SLONG sea_offset = SLONG((sdl3_get_ticks() * 3 / 2) % 4096);` —
wall-clock-derived (1500 ms-incs/sec ровно как 30 FPS оригинала). Modulo 4096
(= 2 полных sin/cos цикла в `(sea_offset >> 1) & 2047`) — wrap гладкий, sin/cos
непрерывны на стыке. Зависимость от physics rate тоже удалена — это визуал.
Подтверждено пользователем 2026-05-03.

---

## 8. ✅ POLY_wibble (underwater distortion / wibble_key vertices): per-frame phase accumulator

**Симптом:** общий wibble-эффект (применяется к POLY с `wibble_key` set —
underwater distortion и подобные эффекты) ускорялся с render FPS.

**Причина:** в `POLY_set_wibble()` ([poly.cpp](../../new_game/src/engine/graphics/pipeline/poly.cpp))
было `POLY_wibble_turn += 256 * TICK_RATIO >> TICK_SHIFT;` per render frame.
TICK_RATIO зажимается в `[128..384]` в thing.cpp — но даже без clamp accumulation
per-render-frame даёт scaling с render FPS. Также зависимость от physics rate
через TICK_RATIO.

**Место:** [`POLY_set_wibble`](../../new_game/src/engine/graphics/pipeline/poly.cpp), строка ~127.

**Фикс:** заменён accumulator на wall-clock derivation:
`total_inc = sdl3_get_ticks() × 7680 / 1000` (= 30 FPS оригинала × 256/frame),
modulo `1024 × 2048 = 2097152` (= 2 × period of `sin(turn × g >> 9)` для любого
integer g — wrap гладкий). Зависимость от physics rate удалена. Это **не** была
вода Stern's Revenge — то отдельный эффект (`sea_offset`, см. #7), но фикс
правильный для underwater distortion и других wibble_key поверхностей.
