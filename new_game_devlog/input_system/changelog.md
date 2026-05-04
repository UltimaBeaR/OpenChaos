# Input System — журнал работ

Хронологический лог изменений по задаче переработки input-системы. Самые свежие записи сверху.

Контекст и план → [current_plan.md](current_plan.md). Большой план с actions/ремапом → [full_plan_deferred.md](full_plan_deferred.md).

---

## 2026-05-05 — Phase 1: скелет модуля `input_frame`

Инфраструктура есть, потребители ещё не мигрировали. Билд проходит, поведение игры не меняется.

**Новое:**
- [`new_game/src/engine/input/input_frame.h`](../../new_game/src/engine/input/input_frame.h) — API в стиле Unity: `input_key_held / just_pressed / just_released / press_pending / consume`, `input_btn_*` для геймпад-кнопок, `input_stick_*` для виртуальных направлений (заглушки), `input_stick_x/y` для continuous, `input_trigger`.
- [`new_game/src/engine/input/input_frame.cpp`](../../new_game/src/engine/input/input_frame.cpp) — реализация.
  - `input_frame_init()` — обнуление снапшотов и sticky-флагов.
  - `input_frame_update()` — зовётся раз в начале каждого render frame; вызывает `ReadInputDevice()` для актуального gamepad-стейта, копирует curr→prev и перезаписывает curr из `Keys[]` / `gamepad_state.rgbButtons[]`.
  - Edge-detect через прямое сравнение `curr` и `prev` массивов.
  - Sticky `s_keys_press_pending[]` для клавиатуры — set в `input_frame_on_key_down`, clear только через `input_key_consume()`. Геймпад без sticky (он polling, не event-driven — fast presses между двумя poll'ами невидимы в принципе).
  - Stick deadzone: 8192/65535 ≈ 25% (`STICK_RAW_DEADZONE`), float-маппинг с deadzone-shift'ом.
  - Stub: stick virtual directions всегда false (Phase 2). Auto-repeat = just_pressed (Phase 3).

**Изменения в существующем коде:**
- [`keyboard.cpp:keyboard_key_down`](../../new_game/src/engine/input/keyboard.cpp) — добавлен `input_frame_on_key_down(scancode)` для set'а sticky-флага.
- [`host.cpp:SetupHost`](../../new_game/src/engine/platform/host.cpp) — `input_frame_init()` после SetupKeyboard.
- [`host.cpp:LibShellActive`](../../new_game/src/engine/platform/host.cpp) — `input_frame_update()` после `sdl3_poll_events` и `restore_surfaces`. Запускается в каждом главном цикле (game / menu / frontend / cutscene / attract) поскольку все используют `SHELL_ACTIVE` макрос.
- [`new_game/CMakeLists.txt`](../../new_game/CMakeLists.txt) — `input_frame.cpp` добавлен в Engine: input.

**Side effect:** `ReadInputDevice()` теперь зовётся раз за frame через `input_frame_update`, в дополнение к существующим вызовам (game.cpp, gamemenu.cpp, frontend.cpp, input_actions.cpp). Эти существующие вызовы становятся near-no-ops (события уже дренированы) но продолжают работать. Игровое поведение не должно меняться.

**Что дальше:**
- Phase 2 — stick virtual directions с hysteresis. Нужны для меню (stick-as-arrows).
- Phase 3 — universal auto-repeat (`input_*_just_pressed_or_repeat`).
- Phase 4-N — миграция потребителей (vehicle siren, gamemenu, frontend, debug-keys, weapon switch).

---

## 2026-05-05 — Phase 2 + 3: stick virtual directions + universal auto-repeat

Заглушки заменены реальной логикой. Билд проходит, поведение игры не меняется (потребители ещё не мигрированы).

**Phase 2 — stick virtual directions:**

В `input_frame_update` добавлен пересчёт `s_stick_dir_curr[2][4]` после snapshot'а геймпада:
- Каждое направление (Up/Down/Left/Right для левого и правого стика) рассчитывается из post-deadzone float значения с **hysteresis**: для входа в pressed нужно отклонение `STICK_DIR_PRESS_THRESHOLD = 0.5f`, для выхода — `STICK_DIR_RELEASE_THRESHOLD = 0.25f`. Это устраняет дребезг на границе порога.
- **Mutual exclusion:** одновременное Up+Down или Left+Right → оба → false (cancel). Стик отклонённый по диагонали Up+Left даёт оба направления одновременно — это ОК, не отрицание.
- `input_stick_held / just_pressed / just_released` — теперь работают по реальным данным.

**Phase 3 — universal auto-repeat:**

Константы `INPUT_REPEAT_INITIAL_MS = 400` и `INPUT_REPEAT_PERIOD_MS = 150` — единые для всех меню (взяты из текущего gamemenu.cpp чтобы миграция не меняла ощущение).

Реализованы `input_key_just_pressed_or_repeat`, `input_btn_just_pressed_or_repeat`, `input_stick_just_pressed_or_repeat` через массивы `s_*_next_fire` (uint64_t timestamps от `sdl3_get_ticks`). Pattern: rising edge → set next_fire = now + initial. На каждом fire после — advance на repeat period.

**Изменения:**
- [`input_frame.cpp`](../../new_game/src/engine/input/input_frame.cpp) — добавлены массивы состояний, helper `compute_dir`, расширены init/update, реализованы Phase 2/3 функции взамен заглушек.

Включён `#include "engine/platform/sdl3_bridge.h"` для `sdl3_get_ticks`.

**Что дальше:**
- Phase 4-N — миграция потребителей. Каждый — отдельный коммит с тестом. Порядок:
  1. `gamemenu.cpp` — самое наглядное место для проверки Phase 2+3 (stick navigation + auto-repeat). Заменяет ad-hoc `gm_last_*` + `gm_dir_next_fire` + `kb_next_fire` на единый API.
  2. `vehicle.cpp` siren toggle (#5).
  3. `frontend.cpp` навигация.
  4. `check_debug_timing_keys`.
  5. `game_tick.cpp` weapon switch (#20) — сначала верифицировать что баг ещё актуален.
