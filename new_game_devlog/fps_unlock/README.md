# FPS Unlock — рабочая папка

Ветка: `fps_unlock` | Начата: 2026-04-27 | Статус: **в работе**

## ⚠️ Перед любым кодом — прочитать [CORE_PRINCIPLE.md](CORE_PRINCIPLE.md)

Основной принцип задачи: **вся логика и отрисовка зависят ТОЛЬКО от wall clock**,
никогда — от номера кадра / цикла рендера / `lock_frame_rate(g_render_fps_cap)`.
Последний — отладочный busy-wait, на него ничего не должно опираться.

Любой код в этой задаче, который нарушает принцип, — баг. Не «срезать угол».

## Цель задачи

Поднять частоту рендеринга выше оригинальных 30 FPS без потери корректности геймплея.

## Текущее состояние

**Готова инфраструктура** (уже в коде):
- Accumulator-based game loop — физика отвязана от рендера
- `lock_frame_rate` на performance counter — точный frame cap без дрейфа
- FPS-каунтер (Shift+F12) — измеряет реальное wall-clock время кадра
- Debug-инструмент (клавиши 1/2/3/9/0) — runtime смена physics/render Hz и тогл интерполяции
- Physics 20 Hz (дизайновая частота оригинала) — таймер миссий 1:1 с реальным временем

**Цель частично достигнута** — render interpolation реализован для Things и камеры.
Дарси, NPC, машины и камера интерполируются на render-rate, движение плавное.
Остаточные баги (мельтешащие педестрианы, поворот машин рывками) — отлаживаются.

Подробности подзадачи → [`render_interpolation/`](render_interpolation/README.md).

**⚠️ Render interpolation нужна ДО 1.0** — без неё 20 Hz физики выглядит тормозным.
С интерполяцией: физика 20 Hz, рендер на любом Hz, движение плавное.

## Файлы

| Файл | Что внутри |
|------|-----------|
| [review/overview.md](review/overview.md) | **⚠️ Для ревью всего объёма правок — single entrypoint с картой файлов, test plan и опасными местами. Не замена полного ревью, а навигатор.** |
| [review/findings.md](review/findings.md) | Детальные находки ревью ветки (баги, расхождения с доками, doc-fixes) — упорядочено по приоритету. |
| [CORE_PRINCIPLE.md](CORE_PRINCIPLE.md) | **⚠️ Основной принцип задачи — читать ПЕРЕД любым кодом** |
| [fps_unlock.md](fps_unlock.md) | Главный документ: почему всё сложно, пути вперёд, технические детали |
| [render_interpolation/](render_interpolation/README.md) | **Подзадача:** render-side интерполяция (lerp между physics-снапшотами) — отдельная папка с архитектурой, coverage, известными багами и планами |
| [debug_physics_render_rate.md](debug_physics_render_rate.md) | Реализация debug-инструмента: глобалы, клавиши, код, нерешённые проблемы |
| [fps_unlock_issues.md](fps_unlock_issues.md) | Баги найденные при тестировании инструмента |
| [original_tick_rates/summary.md](original_tick_rates/summary.md) | **Карта дизайн-частот оригинала — выводы.** Какой эффект под какой Hz калибровался + что куда привязать в new_game. **Читать при любой правке rate-зависимого эффекта.** |
| [original_tick_rates/findings.md](original_tick_rates/findings.md) | Полное расследование: code-citations, алгебраические разборы формул, аномалии. Читать только если надо верифицировать вывод summary или исследовать новый эффект. |
| [original_tick_rates/research.md](original_tick_rates/research.md) | Исходный research brief (task spec). |

## С чего начать агенту

1. **[CORE_PRINCIPLE.md](CORE_PRINCIPLE.md)** — обязательно прочитать первым; описывает на чём вся задача держится и что считается багом по умолчанию
2. Этот файл — понять статус
3. [fps_unlock.md](fps_unlock.md) — почему нельзя просто поднять Hz, какой путь правильный
4. [fps_unlock_issues.md](fps_unlock_issues.md) — конкретные баги если нужно что-то чинить
5. [debug_physics_render_rate.md](debug_physics_render_rate.md) — детали реализации если нужно трогать debug-инструмент

## Ключевые концепции

**`g_physics_hz` / `g_render_fps_cap`** — глобалы в `game_globals.h`, управляют Hz.
Physics по умолчанию 20 Hz (дизайновая частота, `THING_TICK_BASE_MS = 1000 / UC_PHYSICS_DESIGN_HZ = 50ms`). Render без ограничений.

**`phys_tick_diff`** = `1000 / g_physics_hz` — целочисленный ms шаг физики.
Передаётся в `process_things` как `tick_diff_override`, перекрывает реальный wall-clock delta.

**`physics_acc_ms`** — accumulator. Накапливает реальное время, сбрасывает по `phys_step_ms`
за каждый physics-тик. Это позволяет рендеру быть независимым от физики.

**Почему нельзя просто поднять physics Hz:** `Timer1`/`Timer2` (анимации, кулдауны) и
`GAME_TURN` (AI, скрипты) декрементируются раз в тик без масштабирования. При 60 Hz
всё это происходит вдвое быстрее в реальном времени.

**Почему render > 20 Hz без интерполяции не даёт плавности:** позиции обновляются
с частотой физики. Рендер просто дублирует кадры.

**⚠️ Render interpolation — нужна ДО 1.0:** lerp позиций между t-1 и t
physics-снапшотами с коэффициентом `alpha = physics_acc_ms / phys_step_ms`.
Без неё 20 Hz физики выглядит тормозным даже при высоком render FPS.
**Реализация и текущие баги** → [`render_interpolation/`](render_interpolation/README.md).
