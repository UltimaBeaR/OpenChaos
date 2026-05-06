# Миграция consumer'а на `input_frame` — чеклист

При **каждой** миграции потребителя input'а на новый `input_frame` API — проходить по этому документу. Это собрание граблей которые мы уже наступали и решений к ним.

Контекст и API → [current_plan.md](current_plan.md). История фиксов → [changelog.md](changelog.md).

---

## 1. Какой API использовать

Зависит от типа input'а:

| Тип действия | API | Пример |
|---|---|---|
| Toggle / one-shot button (кнопка делает раз) | `input_*_just_pressed` | siren toggle, weapon switch, menu confirm |
| Continuous state (кнопка работает пока зажата) | `input_*_held` | gameplay movement (`if held → move forward`) |
| Auto-repeat **single source** (одна логическая клавиша) | `input_*_just_pressed_or_repeat` | debug-key с auto-repeat'ом |
| Auto-repeat **multi-source** (несколько источников = одно действие) | `InputAutoRepeat::tick_combined(any_jp, any_held)` | menu nav (kb up + stick up + D-pad up) |
| Sticky press для физики | `input_*_press_pending` + `input_*_consume` | physics-tick consumer который может пропустить кадр |
| Stick continuous | `input_stick_x` / `input_stick_y` | gameplay movement, camera |
| Stick virtual direction (как стрелка) | `input_stick_*` (held/just_pressed/etc.) | menu nav, paginate |

## 2. Источник истины — input_frame, НЕ Keys[]

❌ **Не делать:** `if (Keys[KB_X]) { Keys[KB_X] = 0; ... }` для нового кода — `Keys[]` мутируется потребителями (consume), это leak'ает в snapshot и ломает auto-repeat для **других** потребителей.

✅ **Делать:** `if (input_key_just_pressed(KB_X))` или соответствующий API. input_frame snapshot'ит из своего event-tracked held state, не из `Keys[]`.

**Допустимое исключение:** оставлять запись в `Keys[KB_X] = 1/0` если это **bridge** к существующему unmigrated handler'у (как в gamemenu — gamepad input → Keys[KB_ESC] → существующая ESC-логика). Запись в Keys нормальна — это **чтение** Keys как источника истины проблематично.

## 3. D-pad — ЧИТАТЬ через `rgbButtons[11..14]`, НЕ полагаться на стик-mirror

✅ **Делать:** D-pad — отдельный третий источник параллельно стику и клаве:

```cpp
const bool any_up_held = input_key_held(KB_UP)
    || input_stick_held(INPUT_STICK_LEFT, INPUT_STICK_DIR_UP)
    || input_btn_held(11);   // D-pad UP
// 12 = DOWN, 13 = LEFT, 14 = RIGHT
```

Все три источника OR'ятся в один combined-source boolean, который идёт в `InputAutoRepeat::tick_combined`. Один throttle на combined OR — не "double-rate" как было бы при OR'е независимых `_just_pressed_or_repeat`.

❌ **Не делать:** полагаться что D-pad покрывается стиком "автоматически" через mirror в `lX/lY`. Mirror **деструктивный**: gamepad layer (`gamepad.cpp`) при нажатом D-pad клампит `lX/lY` в 0/65535, прячет конкурентное отклонение стика в противоположном направлении. Сейчас `input_stick_held` читает `lX_raw / lY_raw` (pre-override), а D-pad через `rgbButtons[11..14]` — два независимых сигнала. Это нужно для antagonist suppression на пары kb+kb / kb+stick / kb+D-pad / **stick+D-pad** (последняя пара ломалась когда D-pad покрывался через стик-mirror).

**Историческая справка:** в первой версии этого пункта рекомендовалось читать D-pad **только** через `input_stick_held` потому что mirror казался удобным "автоматическим" покрытием. Тестирование gamemenu+frontend (2026-05-05) выявило что зажатие стика и D-pad в противоположные стороны теряет один из сигналов — фикс: `lX_raw / lY_raw` в `GamepadState` + явное чтение D-pad'а в каждом consumer'е.

## 4. Combined-source auto-repeat ⇒ один `InputAutoRepeat`

❌ **Не делать:** OR двух `_just_pressed_or_repeat` вызовов:

```cpp
nav_down = input_key_just_pressed_or_repeat(KB_DOWN)
        || input_stick_just_pressed_or_repeat(INPUT_STICK_LEFT, INPUT_STICK_DIR_DOWN);
```

→ Каждый источник имеет **свой** throttle. Зажав оба одновременно — combined OR срабатывает с ~2× rate из-за фазового сдвига.

✅ **Делать:** один `InputAutoRepeat` instance на логическое действие, OR'ить только raw inputs:

```cpp
static InputAutoRepeat ar_down;

const bool any_dn_jp   = input_key_just_pressed(KB_DOWN)
                      || input_stick_just_pressed(INPUT_STICK_LEFT, INPUT_STICK_DIR_DOWN);
const bool any_dn_held = input_key_held(KB_DOWN)
                      || input_stick_held(INPUT_STICK_LEFT, INPUT_STICK_DIR_DOWN);

const bool nav_down = ar_down.tick_combined(any_dn_jp, any_dn_held);
```

## 5. Antagonist directions — suppress output если оба зажаты

Для axis-навигации (up/down, left/right) — если **оба** противоположных направления зажаты одновременно: подавить output.

✅ **Pattern:**

```cpp
bool nav_up   = ar_up.tick_combined(any_up_jp, any_up_held);
bool nav_down = ar_down.tick_combined(any_dn_jp, any_dn_held);

if (any_up_held && any_dn_held) {
    // No clear intent — suppress output. Timers keep ticking so releasing
    // one direction resumes the other immediately (no fresh initial delay).
    nav_up = false;
    nav_down = false;
}
```

Без этого: при нажатии up+down курсор "дёргается" между позициями.

Это suppression на уровне consumer'а, **не** в input_frame. На уровне stick virtual directions input_frame **уже** это делает (см. [`compute_dir`](../../new_game/src/engine/input/input_frame.cpp) — там mutual exclusion для одного стика). Здесь — для **combined** keyboard+stick.

## 6. Edge-cases которые надо проверить ручным тестированием

После каждой миграции — пройти эти сценарии:

- **Базовый press+release** одной кнопки — действие срабатывает один раз.
- **Hold (auto-repeat если есть):** зажать → 400ms задержка → 150ms интервалы.
- **Combined held simultaneously** (если применимо): зажать ВСЕ источники одного действия одновременно — один темп, не быстрее.
- **Стейл из-другого-контекста:** держать input в gameplay → войти в новый context (меню) → input НЕ должен сразу сработать. Нужно release+repress.
- **Antagonist** (для axis nav): нажать up+down одновременно — ничего не происходит (suppression).
- **Add source mid-repeat:** зажать одну, дождаться auto-repeat'ов, добавить вторую — НЕ должно быть restart 400ms задержки.
- **Release one of two:** зажать обе → отпустить одну → вторая продолжает auto-repeat без перерыва.
- **Зажат до открытия context'а:** зажать кнопку в gameplay → перейти в menu/другой context → отпустить → нажать снова → должно работать как первое нажатие.

Если что-то из этого ломается — записать в [changelog.md](changelog.md), исправить.

## 7. Не звать `ReadInputDevice()` сам

`input_frame_update()` уже зовёт его раз в начале frame'а в `LibShellActive`. Лишние вызовы из consumer'а — дубликаты (idempotent но ненужно). Старые места которые звали (gamemenu и др. до миграции) — удалять при миграции.

## 8. Bridge to existing Keys[] handler — нормально

Если существующий handler читает `if (Keys[KB_ESC])` — мигрируя gamepad-источник на `input_btn_just_pressed`, можно записывать `Keys[KB_ESC] = 1` как мост:

```cpp
if (input_btn_just_pressed(3))   // Triangle/Y
    Keys[KB_ESC] = 1;            // bridge: existing ESC handler reads this
```

Это **не** проблема для input_frame потому что мы не читаем `Keys[KB_ESC]` через input_frame для своего источника (input_frame видит `KB_ESC` через keyboard SDL events, и записи from gamepad не leak'ают в input_frame's event-held state).

## 9. Удалять unused includes после миграции

После миграции часто становятся ненужны:
- `#include "engine/input/joystick.h"` — `ReadInputDevice` больше не зовётся
- ad-hoc edge-detect macros / defines (в gamemenu были `GM_AXIS_*`, удалили)

Пройтись и почистить.

## 10. После миграции — обновить changelog

Запись в [changelog.md](changelog.md) с:
- Дата.
- Что мигрировал (имя файла / функции).
- Что **до** (какие static state'ы были, какая логика).
- Что **после** (какой API, какие константы).
- Список тестовых сценариев (какие edge-cases прошли).
- Любые открытые вопросы.

Чтобы при возврате через время или из нового агента было видно где остановились и что ещё подозрительно.
