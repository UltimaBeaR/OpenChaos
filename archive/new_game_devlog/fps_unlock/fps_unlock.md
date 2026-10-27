# FPS Unlock — технический документ

Статус и быстрый старт → [README.md](README.md)

---

## Что уже реализовано

### Accumulator-based game loop (`game.cpp`)

Физика отвязана от рендера. Реальное время кадра накапливается в `physics_acc_ms`,
физика прогоняется фиксированными шагами:

```cpp
double phys_step_ms = 1000.0 / double(g_physics_hz);   // 50 при 20 Hz design
while (physics_acc_ms >= phys_step_ms) {
    process_things(1, phys_tick_diff);   // phys_tick_diff = 1000/g_physics_hz
    PARTICLE_Run(); OB_process(); ...
    physics_acc_ms -= phys_step_ms;
}
draw_screen();
lock_frame_rate(g_render_fps_cap);
```

`phys_tick_diff` передаётся в `process_things_tick` как `tick_diff_override` —
перекрывает реальный wall-clock delta, физика получает константный шаг.

### `lock_frame_rate` на performance counter (`game.cpp`)

Оригинальный spin-loop на integer ms дрейфовал (33 мс вместо 33.333 → 30.3 FPS вместо 30).
Заменён на performance counter: таргетирует точно `pc_freq / fps` тиков,
хвостовой spin последних 2 мс убирает джиттер планировщика ОС.

### FPS-каунтер (`ui_render.cpp`, Shift+F12 / cheat=2)

Измеряет wall-clock время между вызовами `ui_render_post_composition` —
реальный интервал рендер-кадра, независимо от числа physics-тиков внутри.
EMA α=0.05 (~20-кадровая постоянная времени).

### Debug-инструмент (клавиши 1/2/3/9/0)

Runtime смена physics/render Hz, тогл render-интерполяции. Оверлей в левом верхнем углу.
Подробности реализации → [debug_physics_render_rate.md](debug_physics_render_rate.md)

---

## Почему поднять physics Hz — не просто

### Timer-счётчики не масштабируются

`TICK_RATIO = (TICK_TOCK << TICK_SHIFT) / NORMAL_TICK_TOCK` масштабирует только
_движение_ (`pos += vel * TICK_RATIO >> TICK_SHIFT`). Таймеры декрементируют на 1 за тик:

```cpp
Timer1 -= 1;   // анимация выстрела в STATE_GUN
Timer2 -= 1;   // cooldown, анимации состояний
```

При 60 Hz таймеры убывают вдвое быстрее. Анимации и кулдауны в 2× короче.
Объём сломанного кода пропорционален Hz. В `person.cpp` одном — сотни таких мест.

### `GAME_TURN` не масштабируется

`GAME_TURN++` — раз в physics-тик. На нём завязаны: патрульные циклы NPC,
скриптованные события `eway.cpp`, таймеры переспавна, `GF_DISABLE_BENCH_HEALTH`
(`GAME_TURN & 0x3ff == 314`). При 60 Hz AI-события наступают вдвое раньше.

### Integer truncation в `phys_tick_diff`

`phys_tick_diff = 1000 / g_physics_hz` — целочисленное деление.
При 60 Hz: `1000/60 = 16` вместо 16.666 → `TICK_RATIO ≈ 62` вместо 64.5.
Физические скорости занижены на ~3.8%. При любом Hz кроме 30/15 — дрейф.

---

## Почему render > 20 Hz без интерполяции не даёт плавности

При `physics_fps=20`, `render_fps=60`:
- Физика обновляет позиции объектов раз в 50 мс
- Рендер рисует кадр каждые 16 мс, но объекты между physics-тиками не двигались
- Визуально: дублированные кадры, плавности нет

**Что даёт:** input latency сокращается (кадр меньше ждёт), но визуальная плавность — нет.

---

## Пути к реальному FPS unlock

### А. Render interpolation (реализовано в этой ветке)

- Физика остаётся на 20 Hz (дизайновая частота) — геймплей корректный
- Рендер на 60/120 Hz
- Перед рисованием: lerp позиций/ротаций объектов между предыдущим и текущим
  physics-снапшотом с коэффициентом `alpha = physics_acc_ms / phys_step_ms`
- Сохраняем `WorldPos` / ротацию на конец каждого тика (снапшоты t-1 и t)
- Затрагивает все draw-функции персонажей, объектов; камера тоже интерполируется
- Игровая логика не меняется совсем

**Это правильный путь.** Геймплей точный (20 Hz физика), визуал плавный (60+ Hz рендер).
Реализация → [`render_interpolation/`](render_interpolation/README.md).

### Б. Повышение physics Hz (очень нетривиально)

- Нужен полный аудит всех Timer1/Timer2/GAME_TURN-зависимых мест
- Таймеры в тиках → перевести на реальное время или `TICK_RATIO`-scaling
- Аудит: `grep -rn "Timer[12][-= ]" new_game/src/` (~сотни мест в person.cpp)
- Фактически реинжиниринг всей игровой логики

### В. Гибрид: render 60 Hz, physics 20 Hz, без интерполяции

- Только уменьшение input latency
- Визуально неотличимо от 20 FPS
- Достигается отключением интерполяции (debug-key 3) на текущей инфраструктуре

---

## Physics Hz в разных версиях

Замеры (2026-04-29). `THING_TICK_BASE_MS = 1000 / UC_PHYSICS_DESIGN_HZ = 50ms` → дизайновая частота = 20 Hz.

| Версия | Physics Hz | Таймер миссий | Примечание |
|---|---|---|---|
| PS1 (эмулятор) | ~30 Hz | ~90% реального времени | Хардварный лок 30 FPS |
| PC retail / Piero v0.2.0 | ~22 Hz | ~88% реального времени | Лок 30 FPS, но рендерер не успевал |
| Наша версия (было) | 30 Hz | ~90% реального времени | Совпадало с PS1 |
| **Наша версия (текущая)** | **20 Hz** | **100% (1:1)** | Дизайновая частота оригинала |

**Вывод:** оригинал проектировался под 20 Hz (локальный override `NORMAL_TICK_TOCK = 1000/20 = 50ms`
в `Thing.cpp` оригинала; в нашем коде — `THING_TICK_BASE_MS = 1000 / UC_PHYSICS_DESIGN_HZ = 50ms`).
При 20 Hz таймер миссий идёт 1:1 с реальным временем. Ни PS1 ни PC-релиз этого не достигали из-за
аппаратных ограничений. Мы держим 20 Hz через accumulator.

## Текущее решение для 1.0

Physics 20 Hz (дизайновая частота), render без ограничений.
Accumulator и perf counter lock в коде, готовы.
Debug-инструмент оставить под `bangunsnotgames` перед релизом.

**До 1.0:** render interpolation (lerp позиций по `alpha = acc / phys_step`) —
без неё 20 Hz физики выглядит тормозным при любом render FPS.
