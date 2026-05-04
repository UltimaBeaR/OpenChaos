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

---

## 9. ✅ Кровь под раненым/убитым персонажем: скорость растекания scales с render FPS + z-fight с землёй

**Симптом A (FPS-bound):** лужа крови растекается под Darci/NPC по `splut` счётчику.
На high render FPS лужа достигает финального размера в 8× быстрее (на 240 FPS
~0.24 sec, на 30 FPS ~1.9 sec).

**Симптом B (z-fight):** лужа крови сидит ровно на ground plane → z-файтится
с землёй и пересекающимися спрайтами (листья / газетки / лужи воды).

**Причина A:** `walk->splut++` per render frame в
[`TRACKS_DrawTrack`](../../new_game/src/effects/environment/tracks.cpp). Функция
зовётся из render-path в `aeng.cpp` (drawxtra). Без dt scaling — типичный
паттерн #3 из pitfalls.

**Причина B:** во всех 4 углах quad'а Y координата = `wpy` (ground level).
В [`TRACKS_Bloodpool`](../../new_game/src/effects/environment/tracks.cpp) при
spawn только `+ 257` (= 1 world unit) — недостаточно для надёжного отрыва от
ground/leaves/trash в depth buffer'е.

**Место:** [`tracks.cpp`](../../new_game/src/effects/environment/tracks.cpp),
[`tracks.h`](../../new_game/src/effects/environment/tracks.h).

**Фикс A:** добавлено per-track поле `UBYTE LastSplutPhase` в `struct Track`.
Spawn инициализирует phase в текущий wall-clock bucket (delta=0 на первом
draw'e). В `TRACKS_DrawTrack` `walk->splut += delta` где
`delta = UBYTE(cur_phase - walk->LastSplutPhase)`. Bucket = 50 ms = 20 Hz
(UC_PHYSICS_DESIGN_HZ — оригинальная calibration rate; user explicit choice
2026-05-04). UBYTE wrap = 256 buckets ≈ 12.8 sec, splut окно ≈ 2.8 sec.

**Фикс B:** `BLOOD_Y_LIFT = 4` world units добавлен ко всем 4 углам только
для `POLY_PAGE_BLOODSPLAT`. Other track types (tyre marks, foot prints) не
затронуты — они z-fight'а не дают.

Подтверждено пользователем 2026-05-04.

---

## 10. ✅ Бочки и конусы (DT_MESH props): рывковая ориентация при отбрасывании

**Симптом:** бочки и конусы (один и тот же `Barrel` struct, типы
BARREL_TYPE_NORMAL/CONE/BURNING/BIN) при ударе/взрыве/толчке быстро
крутятся в воздухе с tumble. Position уже интерполировалась через
`INTERP_THING_POS` (общий путь для всех Things), а ориентация
(`Draw.Mesh->Angle`/`Tilt`/`Roll`) обновлялась только на physics tick →
видимый stutter на render rate, особенно заметный на быстрой crутке.

**Причина:** до фикса `render_interp` интерполировал angle/tilt/roll
только для CLASS_VEHICLE (через `Genus.Vehicle->`). DT_MESH things
(barrels, cones, ряд static props) не имели capture/apply пути для
mesh angles. См. `draw_type_uses_tween()` — там DT_MESH намеренно не
включён (mesh вращение независимо от Tween).

**Место:** [`render_interp.cpp`](../../new_game/src/engine/graphics/render_interp.cpp),
[`debug_interpolation_config.h`](../../new_game/src/debug_interpolation_config.h).

**Фикс:** расширил `ThingSnap` полями `mesh_angle/tilt/roll_prev/curr`,
`mesh_*_vel_hint` (signed unwrapped per-tick delta в [-1024, 1024]) и
`mesh_ptr_curr` (для restore через cached pointer аналогично vehicle).
В `render_interp_capture` добавлен путь для DT_MESH с identity check
на `DrawMesh*`. В `RenderInterpFrame::ctor` apply через
`lerp_angle_2048_directed(prev, curr, alpha, vel_hint)` — каждая ось
со своим vel_hint, что предотвращает backward-rotation через 0/2048
wrap при fast tumble (>180°/tick). В dtor — restore через cached ptr.
Добавлен compile-time flag `INTERP_MESH_ANGLE` (default true).

Static DT_MESH (specials, platforms): vel_hint=0, prev==curr → lerp
возвращает curr → unchanged behavior. Slot-reuse safe (DT_MESH→другое
сбрасывает `has_mesh_angles`). Подтверждено пользователем 2026-05-04.

---

## 11. ✅ HUD-маркер прицела: дребезжание + анимация ускоряется от physics Hz

**Симптом (был):**
- **Дребезжание:** маркер прицела на HUD пропадал на один-несколько кадров
  и возвращался, как у таймера миссии. Зависело от `g_physics_hz` и
  `g_render_fps_cap`.
- **Скорость анимации маркера:** при увеличении `g_physics_hz` анимация
  (поворот/мерцание/смена кадров) ускорялась. На штатных 20 Hz — норма.

**Статус:** проверено пользователем 2026-05-04 — не воспроизводится. Закрыто
прошлыми фиксами (вероятно частью миграции визуальных `GAME_TURN`-gated сайтов
на `VISUAL_TURN` либо общими render-interpolation работами). Точная commit-точка
не зафиксирована.

---

## 12. ✅ Игровой таймер миссии (время на HUD) зависит от physics Hz

**Симптом (был):** скорость декремента mission timer масштабировалась с
`g_physics_hz` — на нестандартной физике таймер шёл быстрее или медленнее
реального времени.

**Статус:** работает как задумано. При штатной физике `g_physics_hz = 20`
(дизайновая частота оригинала) таймер идёт **1:1 с реальным временем** — это
лучше, чем у PS1 (~90%) или PC retail (~88%). Миссионные тайм-ауты
рассчитывались под "кривое" оригинальное железо, не под идеальные 1:1 — баланс
может немного отличаться, но принципиально не ломается.

**Замеры (2026-04-29):** миссия с машинкой, таймер 1:30.
- Наша версия (20 Hz): таймер 1:1 с реальным временем.
- PS1 (эмулятор, ~30 Hz): таймер на ~5-9 сек медленнее реального.
- PC retail (Steam) и PieroZ v0.2.0: таймер заканчивается на ~5 сек позже нашей
  (PC физика тикала медленнее из-за рендерной нагрузки).

**Почему безопасно менять `g_physics_hz` (debug-режим):** TICK_RATIO компенсирует
скорость физики (movement per second = BASE × TICK_RATIO × Hz ≈ const).
На скорость таймера влияет integer truncation в `5 * TICK_RATIO >> 8` —
именно этим и можно точечно калибровать таймер, не ломая gameplay.

**Место:** `EWAY_process` в `eway.cpp`, формула `EWAY_tick = 5 * TICK_RATIO >> TICK_SHIFT`.
Default physics Hz: `g_physics_hz = 20` в `game_globals.cpp`.

Подтверждено пользователем 2026-05-04: на штатной 20 Hz физике таймер 1:1 с
реальным временем — закрыто.

---

## 13. ✅ Сирена/мигалка машины: реакция на пробел зависит от physics Hz

**Симптом (был):** при низком `g_physics_hz` нажатие пробела часто не
срабатывало — игра как будто не видит ввод. При штатной физике (20 Hz) —
реагирует. FPS рендера на это не влияет.

**Причина:** опрос кнопки сирены делается в physics-тике (`process_controls`
или vehicle-обработка в `process_things`). При замедленной физике пробел
нажат и отпущен между тиками — edge не регистрируется.

**Статус:** работает как задумано на штатной физике. Управление калибруется под
20 Hz (дизайновая частота). На 20 Hz toggle сирены работает корректно. На низких
physics Hz (debug-клавиша 1 → 5 Hz) регистрация хуже — нажатия часто проскакивают
между тиками, но удержанием можно переключить (проверено 2026-05-03). Низкие Hz —
debug-режим, на них поведение ввода не оптимизируется.

**Анимация мерцания сирены** — закрыта прошлыми фиксами, см. #3 выше.

**Архитектурно решится** переработкой управления — см. [`../input_system/current_plan.md`](../input_system/current_plan.md)
(edge-detect + sticky press в render-tick'е снимут зависимость от physics Hz).
Это часть общей задачи #15 — input привязан к physics-тику. Точечный фикс toggle
сирены не делаем; всё закроется input_system task'ом.

Подтверждено пользователем 2026-05-04.

---

## 14. ✅ Таймер HUD: дребезжание при изменении physics/render Hz (вне красного состояния)

**Симптом (был):** UI HUD таймер миссии дребезжал — пропадал на 1 или более
кадров и возвращался. Скорость дребезжания зависела от обеих переменных:
`g_physics_hz` и `g_render_fps_cap`.

**Гипотеза причины:** числовая рассинхронизация между physics-тиком который
декрементирует таймер и рендер-кадром который его рисует. При нестандартных Hz
соотношение кадров/тиков неравномерное — таймер иногда оказывался в нуле
(или отрицательным) на момент отрисовки.

**Статус:** проверено пользователем 2026-05-04 — не воспроизводится. Закрыто
прошлыми фиксами (вероятно частью render-interpolation работ либо миграции на
`VISUAL_TURN`). Точная commit-точка не зафиксирована.

**Остаточная проблема (была):** скорость красно-белого мерцания таймера когда время
истекло — это был **отдельный** issue, закрыт ниже как #15.

---

## 15. ✅ Таймер HUD: скорость мерцания (когда истёк, красный) зависит от FPS

**Симптом (был):** когда mission timer истекал и становился красным, он начинал
мерцать красный↔pink-white. Скорость мерцания зависела и от `g_physics_hz`,
и от `g_render_fps_cap` — на высоком FPS мерцал быстрее, на низком — медленнее.

**Корневая причина:** в [`PANEL_draw_buffered`](../../new_game/src/ui/hud/panel.cpp)
на time<30 sec был per-render-frame accumulator: `static unsigned short pulse = 0;
pulse += (TICK_RATIO * 80) >> TICK_SHIFT;`. Функция зовётся per render frame, поэтому
pulse advances scaled с render FPS. Дополнительно `TICK_RATIO` clamp'ится в
`[128..384]` → шаг зависит от physics Hz на нестандартных частотах. SIN(pulse & 2047)
циклится за 2048 единиц → период мерцания = render_fps × physics-tied step.

**Фикс:** заменён accumulator на wall-clock derivation — `pulse` теперь считается
из `sdl3_get_ticks()` с фиксированной cadence 80 × `UC_VISUAL_CADENCE_HZ` =
2400 units/sec (= оригинальная 30 Hz render-rate × 80 unit/frame). Период мерцания
≈ 0.85 sec wall-clock на любом render FPS / physics Hz. Маска `& 2047` перенесена
ДО cast'а в SLONG чтобы избежать implementation-defined unsigned→signed на длинном
аптайме (>~10 дней).

Подтверждено пользователем 2026-05-04.

---

## 16. ✅ MIB destruct (электрический эффект смерти Men in Black)

**Где:** [`person.cpp:13601 DRAWXTRA_MIB_destruct`](../../new_game/src/things/characters/person.cpp). Вызывается из render path (`AENG_draw_city`) per render frame пока MIB в state электрической смерти. Содержит несколько визуальных компонентов:

1. **Зигзагообразная синяя/белая линия "молния"** от пелвиса до земли — `POLY_add_line_tex_uv` с `POLY_PAGE_LITE_BOLT`. Без VT/time gate.
2. **Dynamic light flash + PYRO_TWANGER spawn** — gated `if (ctr > 1200 + ammo_packs_pistol)` (где `ctr = Timer1`). Самотротль через ammo_packs_pistol после первого spawn остаётся вечно открытым → spawn на каждом render frame.
3. **SPARK_create** "электрические искры" — gated `if (VISUAL_TURN & 1)`.
4. **WorldPos.Y oscillation** через `+= SIN(ctr >> 2) >> 7` — модификация позиции для "вибрации" тела.

### Симптомы (все закрыты)

**Симптом 1 — анимация молний быстрее на high render rate.** ✅ ЗАКРЫТО (2026-05-03).

При unlimited render эффекты "движутся быстрее" чем при 25 fps cap. Проявлялось как ускоренная анимация электрических разрядов и накопительный overdraw lens flare на высоком FPS.

**Что исправлено (этап 1):** PYRO_TWANGER + dlight спавн в `DRAWXTRA_MIB_destruct` ([`person.cpp`](../../new_game/src/things/characters/person.cpp)) был gated через self-throttle на `ammo_packs_pistol` который срабатывал per render frame → плотность спавна линейно росла с FPS. Заменено на `VISUAL_TURN & 1` gate.

**Что исправлено (этап 2):** `VISUAL_TURN & 1` оказался **level-triggered** — внутри "1"-полуфазы фаерил каждый render frame, поэтому на render > 30 Hz плотность всё ещё росла с FPS (60 FPS ≈ 30 спавнов/сек, 144 FPS ≈ 75/сек, unlimited ≈ сотни/сек). Каждый PYRO_TWANGER рисует свою lens flare в `PYRO_draw_pyro` per render frame → накопление популяции = накопительная additively-blended яркость. Заменено на честный wall-clock edge-detect: `cur_phase = (sdl3_get_ticks() / SPAWN_INTERVAL_MS) & 0xff`, фаерим только когда `cur_phase != Person->LastDestructSpawn`. `LastDestructSpawn` — UBYTE поле в Person ([person_types.h](../../new_game/src/things/characters/person_types.h)). Один `spawn_fire` гейт используется для обоих блоков (dlight+PYRO_TWANGER и SPARK_create) — синхронизирует все эффекты. SPAWN_INTERVAL_MS = 67ms (~15 Hz cadence) — тюнится отдельным параметром.

Lightning line (`POLY_add_line_tex_uv` с `POLY_PAGE_LITE_BOLT`) — single line per frame, не зависит от FPS по яркости (даже без гейта).

**Симптом 2 — тело MIB не поднимается с включённой интерполяцией.** ✅ ЗАКРЫТО (2026-05-03).

В оригинальной анимации тело MIB при destruct'е поднимается вверх. До фикса: с render interpolation OFF — поднималось но скорость взлёта зависела от FPS (быстрее на unlimited). С interp ON — лежало на месте.

**Корневая причина:** `WorldPos.Y += SIN(ctr >> 2) >> 7` в `DRAWXTRA_MIB_destruct` (render-path) — per render frame аккумулировался asymmetric integer shift (`SIN >> 7` дает bias на отрицательных значениях), создавал drift вверх. Скорость drift пропорциональна FPS. С interp ON `RenderInterpFrame::dtor` восстанавливал saved_pos.Y из снапшота → wipe модификаций каждый кадр.

**Фикс:** перенесли `WorldPos.Y +=` в `fn_person_dead` ([`person.cpp:10785`](../../new_game/src/things/characters/person.cpp#L10785)) — physics-tick handler. Шаг скейлится через `* TICK_RATIO >> TICK_SHIFT` чтобы скорость drift'а соответствовала оригинальной 30 Hz cadence независимо от `g_physics_hz`. Render-interp теперь захватывает уже обновлённый WorldPos авторитетно, лерпит между prev/curr → плавный подъём независимо от render rate. Подтверждено пользователем что работает корректно с interp ON и OFF на любом FPS.

**Симптом 4 — Lens flare во время destruct рисуется FPS-зависимо (overdraw).** ✅ ЗАКРЫТО (2026-05-03) — оказался один и тот же корень с Симптомом 1 (level-triggered VISUAL_TURN gate → FPS-scaled population PYRO_TWANGER → каждый рисует lens flare per render frame через `BLOOM_flare_draw` в `PYRO_draw_pyro` → суммарная яркость = N*per-frame, N растёт с FPS). Чинится тем же edge-detect фиксом — см. этап 2 в Симптоме 1.

**Симптом 3 — анимация ментов в Urban Shakedown дёргается на 20 Hz physics.** ✅ ЗАКРЫТО ранее (подтверждено пользователем 2026-05-03 что проблемы нет).

Конкретно: в начале катсцены где мёртвые менты — они интерполировались перед тем как упасть. Закрыто отдельным фиксом раньше Phase 5 pose snapshot work; точная commit-точка не зафиксирована, но сейчас не воспроизводится.

Также возможна ускоренная/замедленная скорость animation playback из-за смены physics rate с 30 → 20 Hz (см. [fps_unlock_issues.md #16](fps_unlock_issues.md) — ревизия таймингов) — это **отдельная** проблема (animation duration constants могли калиброваться под 30 Hz), не связана с интерполяцией.

**Симптом 5 — Длительность destruct'а 7 сек у нас vs 11 сек на release PC.** ✅ ЗАКРЫТО (2026-05-03).

Pre-release исходники (которые мы используем) содержат `Timer1 >= 20 * 20 * 5` (= 2000) с комментарием **«Was 32 * 20 * 5 for the PC, less time for the DC...»** ([original_game/fallen/Source/Person.cpp:17640](../../original_game/fallen/Source/Person.cpp#L17640)). То есть pre-release был сборкой под Dreamcast — там специально укоротили эффект. Release PC использовал 3200. Наша математика: при 20 Hz физики Timer1 += 16/тик × 20 = 320/sec → 2000 / 320 = **6.25 сек**. С threshold 3200: 3200 / 320 = **10 сек**, что близко к наблюдаемым 11 сек на release PC.

**Фикс:** заменено `20 * 20 * 5` на `32 * 20 * 5` в `fn_person_dead` ([person.cpp:10700](../../new_game/src/things/characters/person.cpp#L10700)) с комментарием объясняющим происхождение DC value. Подтверждено пользователем.

**Симптом 6 — Тело взлетает ~2x ниже чем на release PC.** ✅ ЗАКРЫТО (2026-05-03) — фикс Симптома 5 покрыл (увеличение длительности с 6.25→10 сек дало +70% к суммарному drift'у, чего хватило). Подтверждено пользователем.

**Симптом 7 — «Синяя хрень» (lightning ribbons SPARK) рисуется FPS-зависимо, форма дёргается с render rate.** ✅ ЗАКРЫТО (2026-05-03).

**Корневая причина:** `SPARK_find_midpoint` ([spark.cpp:485](../../new_game/src/effects/combat/spark.cpp#L485)) использовал `rand()` для случайного смещения зигзаг-midpoint'ов. Вызывался из `SPARK_get_next` в render path → midpoints перегенерировались каждый render frame с новыми случайными значениями → форма bolt'а дёргалась с FPS-частотой.

Дополнительно: `SPARK_get_next` имел второй `rand()` для skip-логики sub-segments при `ss->die` countdown.

**Фикс:** заменены оба `rand()` на детерминированный hash `spark_noise(identity, 0, bucket, salt)`:
- **identity** = `(SPARK_get_spark << 8) | segment_idx` — стабильный per-(spark, segment), НЕ endpoint coords (важно: endpoints SPARK_TYPE_LIMB интерполируются render_interp'ом per render frame; использовать их как seed → hash меняется per frame → wiggle FPS-bound)
- **bucket** = `sdl3_get_ticks() / 67ms` (~15 Hz cadence) — wall-clock-bound, одинаков на любом FPS
- **salt** различает 3 вызова midpoint в одном segment'е и x/y/z оси

**Также фиксы в render path SPARK:**
- `SPARK_process` ([game_tick.cpp:1257](../../new_game/src/game/game_tick.cpp#L1257)) обёрнут в wall-clock gate (15 Hz). Раньше `process_controls` per render frame вызывал `SPARK_process`, который делал `Pos += dx>>4` и `--die` per call → искры умирали за 0.1 сек на 280 FPS, за 1.5 сек на 25 FPS, и Pos двигался FPS-кратно.

**Симптом 8 — На уменьшенной физике lens flare overdraw'ит, эффект слишком яркий.** ✅ ЗАКРЫТО (2026-05-03).

**Корневая причина:** PYRO_TWANGER lifetime был physics-tick-bound (`counter += 16` per tick, free at 240 = 15 ticks). При снижении physics rate lifetime растягивался → spawn (15 Hz wall-clock) накапливал больше одновременно живых twanger'ов → каждый рисует additive lens flare per render frame → over-bright bloom.

При первой попытке использовать `* TICK_RATIO >> TICK_SHIFT` оказалось что `TICK_RATIO` зажимается в `[128..384]` ([thing.cpp:478-479](../../new_game/src/things/core/thing.cpp#L478-L479)) — на 5 Hz физики реальный множитель должен был быть 1024, но clamp давал 384 → недокомпенсация.

**Фикс:** использован **uncompressed** `tick_tock_unclipped` ([pyro.cpp:653](../../new_game/src/effects/combat/pyro.cpp#L653)) — `pyro->counter += (16 * tick_tock_unclipped) / 50` где 50 = THING_TICK_BASE_MS (1000/20). Lifetime теперь ~0.75 сек wall-clock на любом physics rate (включая 5 Hz). Population стабильно ~11 alive.

Дополнительно: яркость lens flare'а PYRO_TWANGER уменьшена в 2x (`str >>= 1` в [pyro.cpp:1675](../../new_game/src/effects/combat/pyro.cpp#L1675)) чтобы попасть в диапазон обычных light source'ов сцены (фары/лампы), а не выглядеть disproportionately blown out.

**Симптом 9 — `POLY_PAGE_LITE_BOLT` rendering glitch: тёмные ректангулы где transparent texels одной молнии перекрывают другие.** ✅ ЗАКРЫТО (2026-05-03).

**Корневая причина:** в setup'е POLY_PAGE_LITE_BOLT ([poly_render.cpp:253](../../new_game/src/engine/graphics/pipeline/poly_render.cpp#L253)) была опечатка — `SetDstBlend` вызывался дважды, `SetSrcBlend` вообще не вызывался. И `SetDepthWrite(false)` отсутствовал → каждый additive quad молнии писал в depth buffer, блокируя последующие overlapping quad'ы.

**Фикс:** добавлен `SetSrcBlend(One)` (additive blending как и задумывалось) + `SetDepthWrite(false)` (transparent additive primitives не должны писать depth). Подтверждено пользователем.

**Симптом 10 — Анимация текстуры LITE_BOLT слишком быстрая.** ✅ ЗАКРЫТО (2026-05-03) — `SHAPE_sparky_line` ([shape.cpp:344](../../new_game/src/engine/graphics/geometry/shape.cpp#L344)) циклит `which = (VISUAL_TURN + i) & 3` — VISUAL_TURN тикает 30 Hz wall-clock. Замедлено в 2x: `vt_for_anim = VISUAL_TURN >> 1` → ~15 Hz cadence, в один темп с остальными MIB destruct визуалами.

---

## 17. ✅ Mission timer залипает на экране после level complete (regression)

**Симптом (был):** после прохождения миссии (level complete / level fail) HUD-таймер
миссии оставался на экране замороженным на последнем значении. Время не тикало,
но если таймер был красным на момент завершения — pulse-анимация продолжала
работать (она wall-clock-derived, см. resolved #15). В оригинале такого не было —
таймер исчезал сразу при level complete.

**Корневая причина (regression нашей итерации):** оригинал звал `PANEL_draw_timer`
**inline** из waypoint-кода каждый physics tick, и значение хранилось в глобале
`slPANEL_draw_timer_time` который **автосбрасывался в -1 после каждой отрисовки**.
Если waypoint код не вызвал — таймер исчезал сам.

В нашем рефакторе (видимо для устранения дребезжания на render > physics, см.
resolved #14) вынесли draw в отдельную функцию `EWAY_draw_hud_timers` с
персистентной переменной `EWAY_hud_countdown_value` (в [eway.cpp:4187](../../new_game/src/missions/eway.cpp#L4187),
гейт `>= 0`). Декремент остался в waypoint processing — но при level
won/lost оно скипается ([eway.cpp:4219](../../new_game/src/missions/eway.cpp#L4219)),
поэтому значение замораживалось без сброса.

**Фикс:** в ветке `if (GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON))` добавлено
явное `EWAY_hud_countdown_value = -1;` — компенсирует потерю автосброса оригинала.

Подтверждено пользователем 2026-05-04.

---

## 18. ✅ Меню паузы: анимация фона и появления букв зависят от render FPS

**Симптом (был):** при открытии меню паузы анимация появления backdrop'а и
побуквенного fade-in текста пунктов меню ускорялись с ростом `g_render_fps_cap`.
На штатных 30-100 FPS — нормальная скорость, выше 100 FPS — заметно быстрее.

**Корневая причина:** в [`GAMEMENU_process`](../../new_game/src/ui/menus/gamemenu.cpp)
delta вычисляется как wall-clock `millisecs = sdl3_get_ticks() - tick_last`, что
правильно. Но затем `SATURATE(millisecs, 10, 200)` поднимает значение **до 10ms**
если оно меньше. На render > 100 FPS реальный per-frame delta < 10ms (8ms на
120 FPS, 4ms на 240 FPS) — clamp поднимает его, и `GAMEMENU_background +=
millisecs` / `_fadein_x += millisecs << 7` накапливают больше времени чем прошло
по wall-clock:

| render FPS | real ms/frame | после clamp | "ms"/sec |
|-----------|---------------|-------------|----------|
| 30        | 33            | 33          | 1000 ✓   |
| 100       | 10            | 10          | 1000 ✓   |
| 120       | 8.3           | **10**      | 1200 ✗   |
| 240       | 4.2           | **10**      | 2400 ✗   |

Floor 10ms — пережиток времён когда unlimited FPS не было. Нижняя граница как
guard от sub-ms delta была нужна на оригинальном железе; сейчас мешает.

**Фикс:** в [gamemenu.cpp:106](../../new_game/src/ui/menus/gamemenu.cpp#L106)
clamp изменён на `SATURATE(millisecs, 0, 200)`. Sub-ms кадры дают delta=0 —
анимация не двигается на этом кадре, на следующем подкатит. Верхний clamp 200ms
сохранён (alt-tab / breakpoint guard).

Подтверждено пользователем 2026-05-04.

---

## 19. ✅ Главное меню: переход между экранами ускоряется при высоком render FPS

**Симптом (был):** анимация wipe/fade при переходе между экранами главного меню
(`fade_state` 0..63) ускорялась с ростом `g_render_fps_cap`. На 240+ FPS — заметно
быстрее, чем на 30/60/120.

**Корневая причина:** в [`FRONTEND_loop`](../../new_game/src/ui/frontend/frontend.cpp)
было `fade_speed = millisecs >> 3` (= `/8`) с floor'ом `< 1 → 1`. На render ≤ 120 FPS
работало корректно (33→4, 16→2, 8→1, все дают 120 units/sec). На 240 FPS millisecs=4,
4>>3=0, floor поднимает до 1 → 240 units/sec вместо design 120 (2× ускорение).
Промежуточные значения 180-220 FPS — примерно 1.5×.

**Фикс:** в [frontend.cpp:3171-3187](../../new_game/src/ui/frontend/frontend.cpp#L3171)
заменён shift-and-floor на (1) input-clamp millisecs до одного design tick = 33ms
(`= 1000 / UC_VISUAL_CADENCE_HZ`), эквивалент оригинального FADE_SPEED_MAX guard
для первого кадра после load (где elapsed time может быть до 250ms), и (2) float
accumulator для дробных per-frame инкрементов — sub-step time переносится в следующий
кадр. Floor `< 1 → 1` удалён: sub-tick кадры дают 0, accumulator подкатит.

Математика на разных FPS:

| render FPS | per-frame | units/sec |
|-----------|-----------|-----------|
| 30        | 4.0 exact | 120 ✓     |
| 60        | ~2 (avg)  | 120 ✓     |
| 100       | ~1.21     | 121 ✓     |
| 240       | ~0.485    | ~116 ✓    |

Подтверждено пользователем 2026-05-04.
