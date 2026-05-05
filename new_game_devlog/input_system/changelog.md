# Input System — журнал работ

Хронологический лог изменений по задаче переработки input-системы. Самые свежие записи сверху.

Контекст и план → [current_plan.md](current_plan.md). Большой план с actions/ремапом → [full_plan_deferred.md](full_plan_deferred.md).

---

## 2026-05-05 — Phase 4-Wide Commit A: миграция `get_hardware_input` + расширение API

**Задача:** последний крупный потребитель который читал hardware напрямую — central pipeline `get_hardware_input` в [`input_actions.cpp`](../../new_game/src/game/input_actions.cpp) (lines ~3020-3275). До этой сессии это было задокументированное exception #3 в "ТЕКУЩЕЕ СОСТОЯНИЕ" секции ниже — теперь закрыто.

**Расширен API в [`input_frame.h`](../../new_game/src/engine/input/input_frame.h) / [`.cpp`](../../new_game/src/engine/input/input_frame.cpp):**

- `input_gamepad_connected()` — обёртка над `gamepad_state.connected`.
- `input_dpad_active()` — обёртка над `gamepad_state.dpad_active`.
- `input_stick_x_axis(stick)` / `input_stick_y_axis(stick)` — `int` 0..65535 (post-override, D-Pad clamp включён). Для bit-packing в `Player->Pressed` биты 18-31 (analog magnitude) и для уровневой digital direction detection в `get_hardware_input`. Дополняет `input_stick_x/y` (float -1..1, deadzone, для magnitude) и `input_stick_held` (pre-override + 4096 menu threshold) — три use case'а, три API.
- `input_trigger_raw(idx)` — `int` 0..255. Для `weapon_feel_evaluate_fire` который оперирует byte-resolution thresholds; float `input_trigger()` теряет precision через round-trip.

**Миграция [`get_hardware_input`](../../new_game/src/game/input_actions.cpp) (1:1, поведение не меняется):**

- `the_state.connected` / `the_state.dpad_active` → `input_gamepad_connected()` / `input_dpad_active()`.
- `the_state.lX/lY` (3+3 чтения для packing + digital flags) → `input_stick_x_axis(LEFT)` / `_y_axis(LEFT)` через локальный `axis_x/y`.
- `the_state.rgbButtons[N]` (~13 чтений: JUMP, Triangle, R1 kick, START, R3 select, CAMERA, ACTION, cheat combo Select/L1/L2/D-pad) → `input_btn_held(N)`.
- `the_state.trigger_left/right` → `input_trigger_raw(15/16)`.
- `if (ReadInputDevice())` → `if (input_gamepad_connected())`. `ReadInputDevice` теперь зовётся **только** из `input_frame_update` — gamepad polling централизован.
- Удалены: macro `BUTTON_IS_PRESSED(value) (value)` (рудимент, identity-обёртка над DirectInput-style read), include `engine/input/joystick.h` (`ReadInputDevice` больше не нужен здесь).

**Не входит в этот коммит (отдельные шаги):**

- `apply_button_input_car` lines 2947-2948 (R2/L2 для accel/brake) — Commit B.
- `apply_button_input_first_person` lines ~3479-3481 (rX/rY для look-around) — Commit B.
- `game.cpp:608` playback skip loop (`rgbButtons[0..9]`) — Commit B.
- `fc.cpp` rX/rY camera, `vehicle.cpp` triggers, `outro_os.cpp` lX/lY/buttons — Commit C.
- Финальный grep-аудит и обновление "ТЕКУЩЕЕ СОСТОЯНИЕ" handoff-секции — Commit D.

**Build:** `make build-release` 298/298 PASS, link OK.

**Тестовые сценарии:** не тестировано пользователем — миграция чисто механическая (1:1 substitution через input_frame layer который уже свежий — `input_frame_update` зовётся раньше `get_hardware_input`). Любая регрессия указала бы на расхождение между `gamepad_state` reads через input_frame vs прямое чтение, но input_frame accessors — straight-line wrappers без логики.

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

---

## 2026-05-05 — Phase 4 шаг 1: миграция `gamemenu.cpp`

Первый потребитель переехал на `input_frame`. Не должно быть видимых изменений в навигации — пороги, тайминги auto-repeat и edge-detect взяты bit-identical с прежним поведением.

**До:**
- Пять статиков edge-detect: `gm_last_start / triangle / cross / dir`, `gm_dir_next_fire`.
- Else-ветка "menu not active" вручную сбрасывала `gm_last_*` через чтение `the_state.rgbButtons[]`.
- Stick threshold `GM_NOISE_TOLERANCE = 4096` (без hysteresis).
- Двойной auto-repeat: один в gamepad-блоке (`gm_dir_next_fire`), второй в keyboard-блоке (`kb_next_fire`) поверх `Keys[KB_UP/DOWN]`.

**После:**
- Все статики удалены.
- Триггеры через `input_btn_just_pressed(0/3/6)` для Cross/Triangle/Start. Edge-detect автоматически: `just_pressed` не сработает на кнопке зажатой ещё **до** открытия меню (нет rising edge в snapshot'е) — закрывает класс багов про "залипший gameplay-press утёк в menu action".
- Up/Down навигация унифицирована **внутри** `GAMEMENU_background > 200` гейта: OR от двух источников
  (`input_key_just_pressed_or_repeat(KB_UP/DOWN)` + `input_stick_just_pressed_or_repeat(LEFT, UP/DOWN)`),
  каждый со своим auto-repeat'ом. Запись в `Keys[KB_UP/DOWN]` сохраняется как мост для существующего menu-handler'а.
- Stick threshold через input_frame (`STICK_DIR_PRESS_RAW = 4096` press, `2048` release) — те же 4096 от центра для входа в pressed как раньше, плюс hysteresis для устранения дребезга на границе.
- Auto-repeat cadence (400ms initial / 150ms period) — те же значения, теперь в одном месте `INPUT_REPEAT_INITIAL_MS / PERIOD_MS`.
- Удалены `#define GM_AXIS_*` и `#include "engine/input/joystick.h"` — больше не нужны.

**Изменения:**
- [`gamemenu.cpp`](../../new_game/src/ui/menus/gamemenu.cpp) — миграция input-обработки в `GAMEMENU_process`.

**Тестирование (для пользователя):**
- Открыть pause menu (ESC или Start). Навигация стрелками клавиатуры — стандартный темп зажатия (400ms ждать, потом 150ms интервалы).
- То же стиком геймпада — должно ощущаться идентично.
- D-pad на Xbox / DualSense — миррорится в lX/lY на gamepad layer'е, поэтому идёт через stick virtual directions без отдельного code path.
- Triangle/Y закрывает меню (back/cancel), Cross/A подтверждает, Start тогглит.
- Если зажать Cross в gameplay перед открытием меню — меню должно открыться, но Cross-press не должен сразу подтвердить пункт (нужно отпустить и нажать заново).

---

## 2026-05-05 — Phase 4 шаг 1.1: фиксы по итогам тестирования gamemenu миграции

Три бага найдены тестированием миграции, починены в инфраструктуре `input_frame` (не в gamemenu) — все будущие потребители получат правильное поведение автоматически.

### Баг 1: keyboard auto-repeat не работал

**Симптом:** на клавиатуре зажатие стрелки давало одно срабатывание, дальше залипания не было. Геймпадом залипание работало.

**Причина:** input_frame snapshot'ил из глобального `Keys[]`. Меню после consume делает `Keys[KB_UP] = 0` — это значение leak'ало в snapshot следующего кадра. SDL не повторяет key-down events для зажатой клавиши, только edge-events. Получалось:
- Кадр N: SDL key-down → `Keys[KB_UP]=1`, snapshot=1, `just_pressed` → меню consume `Keys[KB_UP]=0`
- Кадр N+1: SDL событий нет (клавиша держится), но `Keys[KB_UP]` уже 0 (consume'или), snapshot=0 → `held=false` → auto-repeat молчит

(Геймпад не пострадал т.к. `gamepad_state.rgbButtons[]` обновляется per-frame от polling, никто его не консьюмит.)

**Фикс:** input_frame теперь хранит **собственное** held-state клавиатуры через event hooks (`on_key_down` / `on_key_up`), независимое от мутируемого `Keys[]`. Snapshot читает из своих `s_keys_event_held[]` + `s_keys_pressed_during_frame[]` (для same-frame press+release visibility), а не из `Keys[]`.

Изменения:
- [`input_frame.h`](../../new_game/src/engine/input/input_frame.h) — добавлена `input_frame_on_key_up`.
- [`input_frame.cpp`](../../new_game/src/engine/input/input_frame.cpp) — добавлены `s_keys_event_held` и `s_keys_pressed_during_frame`. Snapshot читает из них. `Keys[]` больше не источник истины.
- [`keyboard.cpp:keyboard_key_up`](../../new_game/src/engine/input/keyboard.cpp) — добавлен hook `input_frame_on_key_up`.

### Баг 2: auto-repeat сразу срабатывал при открытии меню если стик был зажат в gameplay

**Симптом:** зажать стик во время игры → нажать Start → меню открывается → курсор сразу едет вниз с auto-repeat-частотой.

**Причина:** `next_fire` для auto-repeat'а инициализирован 0. Если `_just_pressed_or_repeat` не вызывался во время gameplay (а он зовётся только из gamemenu), то к моменту первого вызова из меню `now >> next_fire = 0` → `now >= next_fire` тривиально true → fires immediately.

**Фикс:** добавлены флаги `s_*_repeat_armed` per (key/btn/stick-dir). Auto-repeat стреляет только если `just_pressed` был **захвачен в этой функции** (в этой call-chain'е) — т.е. в этом потребителе была rising edge. На release (`!held`) флаг сбрасывается, чтобы каждое новое нажатие требовало свежего just_pressed.

Сценарий после фикса: стик зажат с gameplay → меню открывается → `just_pressed_or_repeat` зовётся первый раз → `held=true`, `just_pressed=false` (rising edge давно прошла), `armed=false` (никогда не set'или) → `false`. После release+repress → новый `just_pressed` → armed=true → fires.

### Баг 3: 2× скорость прокрутки при одновременном нажатии клавиатуры и стика/D-pad

**Симптом:** в меню зажать стрелку клавы и одновременно D-pad/стик → прокрутка вдвое быстрее.

**Причина:** в gamemenu OR от двух `_just_pressed_or_repeat` вызовов — у каждого свой независимый `next_fire` timer. С небольшим фазовым сдвигом между источниками их fire-моменты интерливятся → combined OR срабатывает в 2× rate.

Оригинальный (до миграции) gamemenu имел два throttle'а **в серии** — сначала gamepad's `gm_dir_next_fire` инжектил в `Keys[]`, потом keyboard's `kb_next_fire` дросселировал OR'ный union. Combined rate был ограничен последовательным throttle'ом. Миграция разорвала эту унификацию.

**Фикс:** добавлен `InputAutoRepeat` struct в `input_frame` — single auto-repeat throttle на caller-supplied combined-source booleans:

```cpp
struct InputAutoRepeat {
    bool was_held = false;
    bool armed = false;
    uint64_t next_fire = 0;
    bool tick_combined(bool any_just_pressed, bool any_held);
};
```

API принимает **два** boolean'а а не один:
- `any_just_pressed` — OR snapshot-level `_just_pressed` от всех источников.
- `any_held` — OR `_held`.

Combined rising edge определяется как `any_just_pressed && !was_held`. Это даёт правильное поведение во всех edge-cases:

| Сценарий | `any_just_pressed` | `any_held` | `was_held` | Поведение |
|---|---|---|---|---|
| Зажал стик в gameplay → откр. меню | false | true | false (initial) | НЕ fire ✓ |
| Tap нажатие в меню | true | true | false | fire + start timer ✓ |
| Auto-repeat tick | false | true | true | fire (если armed && timer истёк) ✓ |
| Зажал клаву + добавил стик мid-repeat | true (stick) | true | true | НЕ restart timer (combined уже держался) ✓ |
| Отжал клаву, держит стик | false | true | true | продолжает текущий timer ✓ |
| Отпустил всё | — | false | — | disarm, reset was_held ✓ |

Почему именно так: одно-bool API (`any_held` only) ломалось на сценарии "стик зажат с gameplay" — `was_held` не обновлялся пока `tick_combined` не зывался во время gameplay, поэтому первый вызов из меню видел `was_held=false, any_held=true → just_pressed`. Двух-bool API использует caller-passed `any_just_pressed` который от **snapshot-level** input_frame edge (всегда корректен независимо от того кто вызывал tick_combined).

Использование в gamemenu:
```cpp
static InputAutoRepeat ar_up, ar_down;

const bool nav_up = ar_up.tick_combined(
    /*any_just_pressed=*/ input_key_just_pressed(KB_UP)
        || input_stick_just_pressed(INPUT_STICK_LEFT, INPUT_STICK_DIR_UP),
    /*any_held=*/ input_key_held(KB_UP)
        || input_stick_held(INPUT_STICK_LEFT, INPUT_STICK_DIR_UP));
```

Изменения:
- [`input_frame.h`](../../new_game/src/engine/input/input_frame.h) — struct `InputAutoRepeat` с методом `tick_combined(any_just_pressed, any_held)`.
- [`input_frame.cpp`](../../new_game/src/engine/input/input_frame.cpp) — реализация.
- [`gamemenu.cpp`](../../new_game/src/ui/menus/gamemenu.cpp) — миграция nav на `InputAutoRepeat::tick_combined` с OR'ом источников на входе.

`input_*_just_pressed_or_repeat` (single-source) **остаются** в API для случаев одного логического источника без OR.

---

## 2026-05-05 — Phase 4 шаг 1.2: antagonist suppression + чеклист миграции

### Баг 4: дёргание курсора при одновременном UP+DOWN

**Симптом:** в меню зажать стрелку ↑ и ↓ одновременно (на клаве, или kb+stick) — курсор дёргается между позициями.

**Причина:** `ar_up` и `ar_down` — независимые `InputAutoRepeat` instances. Каждый видит свой combined input как held → оба armed, оба тикают, оба пишут в `Keys[KB_UP/DOWN]` → меню обрабатывает оба → cursor крутит туда-сюда.

**Фикс:** consumer-side antagonist suppression в gamemenu — если оба направления зажаты, output обоих суррессен. Timers продолжают тикать чтобы при отпускании одной — вторая сразу резюмировала auto-repeat без свежей 400ms задержки.

```cpp
if (any_up_held && any_dn_held) {
    nav_up = false;
    nav_down = false;
}
```

3 строки в gamemenu, никаких изменений в input_frame.

Изменения:
- [`gamemenu.cpp`](../../new_game/src/ui/menus/gamemenu.cpp) — antagonist suppression блок.

### Создан чеклист миграции

Документ [`migration_checklist.md`](migration_checklist.md) — собрание всех граблей которые мы наступили на gamemenu миграции, с правильными решениями. Сверяться **при каждой** будущей миграции потребителя:

- Какой API использовать (по типу действия — toggle / continuous / repeat / sticky / etc.)
- Источник истины — input_frame, не Keys[]
- D-pad через stick virtual directions, не через rgbButtons[11..14]
- Combined-source auto-repeat → один `InputAutoRepeat`, не OR двух `_just_pressed_or_repeat`
- Antagonist suppression для axis nav
- Edge-cases для ручного тестирования (held-from-context, mid-repeat add source, и т.д.)
- Не звать `ReadInputDevice()` сам
- Bridge to existing `Keys[]` handlers допустим
- Чистить unused includes
- Обновлять changelog после миграции

---

## 2026-05-05 — Phase 4 шаг 2: миграция `frontend.cpp::FRONTEND_input`

Главное меню (frontend) переехало на `input_frame`. Паттерн идентичен gamemenu, расширен на 4 направления (UP/DOWN/LEFT/RIGHT — слайдеры громкости в Audio settings, переключение районов на карте миссий, multi-choice пункты).

**До:**
- `held_x / held_y` — собственный hysteresis stick→direction (4096 press / 2048 release). Идентичные пороги уже есть в input_frame.
- `dir_next_fire` — единый таймер auto-repeat'а на combined `dir_input` (400/150ms).
- `last_input` xor edge-detect для buttons; `last_button` post-bind suppression; `first_pad` carry-over guard.
- Чтение `Keys[KB_UP/DOWN/LEFT/RIGHT]` напрямую; чтение Triangle/Y через `get_hardware_input(INPUT_TYPE_JOY) & INPUT_MASK_CANCEL`.
- Dominant-axis pick (no diagonals on stick) реализован вручную через сравнение `dx vs dy`.

**После:**
- 4× `InputAutoRepeat` (`ar_up/dn/lt/rt`) — combined-source throttle на kb + stick + D-pad одновременно.
- Antagonist suppression на UP+DOWN и LEFT+RIGHT (3 источника одновременно зажаты в противоположные стороны → cancel).
- Dominant-axis pick для **stick-only** диагоналей сохранён (small Y wobble не должен перебивать намеренный X), читает `lX_raw / lY_raw` (см. фикс ниже).
- Buttons: `input_btn_just_pressed(0)` для Cross/A confirm, `input_btn_just_pressed(3)` для Triangle/Y cancel — natural rising-edge снимает `first_pad` и `last_button` статики (carry-over пресс не даёт rising edge в snapshot'е).
- `grabbing_pad` режим биндинга: ESC через `input_key_just_pressed(KB_ESC)` + явный `Keys[KB_ESC] = 0` consume (иначе следующий кадр FE_BACK триггернётся в обычной нав-ветке). Биндинг — через `input_btn_held(i)` (любая зажатая gamepad-кнопка).
- `last_input` static оставлен как gate для `grabbing_pad && !last_input` (vestigial — с edge semantics практически всегда 0, но семантика сохранена).
- Удалены: `held_x`, `held_y`, `dir_next_fire`, `last_button`, `first_pad`, локальные `#define AXIS_CENTRE/ACTIVATE_ZONE/RELEASE_ZONE`, явный `ReadInputDevice()`.

**Изменения:**
- [`frontend.cpp::FRONTEND_input`](../../new_game/src/ui/frontend/frontend.cpp) — миграция input-обработки.

**Открытые вопросы при тестировании:** обнаружен баг "стик + D-pad одновременно в противоположные стороны = второй overrid'ит первый, antagonist не работает". См. следующую запись.

---

## 2026-05-05 — Архитектурный фикс: D-Pad как отдельный input source

**Симптом** (найден тестированием frontend): зажать стик UP + D-pad DOWN (или наоборот) → второй нажатый направление становится приоритетным, первый "теряется", antagonist suppression не срабатывает. Только при паре stick+D-pad — kb+kb / kb+stick / kb+D-pad работают корректно.

**Причина:** в `gamepad.cpp` D-pad **деструктивно** перезаписывает `lX/lY`:
```cpp
if (ds.dpad_left)  gamepad_state.lX = 0;
if (ds.dpad_right) gamepad_state.lX = 65535;
// и т.д.
```
Так что когда D-pad нажат — стик физически невидим (axis уже override'нут). `input_stick_held` читал post-override `lX/lY`, и стик-сигнал в противоположную D-pad сторону пропадал.

**Фикс:** дать input_frame отдельные raw-каналы стика, и читать D-pad независимо через `rgbButtons[11..14]`:

1. **`GamepadState`** — добавлены поля `lX_raw / lY_raw / rX_raw / rY_raw`. Заполняются в обоих gamepad poll paths (DualSense, SDL3) **до** D-pad override'а.
2. **`gamepad_globals.cpp`** — initializer расширен с 4 до 8 значений (4 stick + 4 raw, все centered = 32768).
3. **`input_frame.cpp::input_frame_update`** — стик-виртуальные направления читают `lX_raw / lY_raw` (pre-override). D-pad mirror через `lX/lY` больше не покрывает стик автоматически.
4. **`gamemenu.cpp` + `frontend.cpp`** — D-pad добавлен как **третий независимый источник** в `any_*_held / any_*_jp`:
   ```cpp
   const bool any_up_held = input_key_held(KB_UP)
       || input_stick_held(INPUT_STICK_LEFT, INPUT_STICK_DIR_UP)
       || input_btn_held(11);   // D-pad UP
   ```
5. **frontend.cpp::FRONTEND_input** — dominant-axis pick читает `the_state.lX_raw/lY_raw` (актуальное физическое отклонение стика, не override'нутое D-pad'ом).

После фикса все пары источников проходят antagonist: kb+kb, kb+stick, kb+D-pad, **stick+D-pad**.

**Геймплей не затронут.** `input_actions.cpp`, `fc.cpp`, `outro_os.cpp` читают `lX/lY` напрямую — D-pad-as-full-deflection семантика для движения и камеры сохраняется ровно как раньше. `input_stick_x/y` continuous getters в input_frame тоже остались на post-override `lX/lY` (они для геймплейного continuous-чтения, не для меню).

**Изменения:**
- [`gamepad.h`](../../new_game/src/engine/input/gamepad.h) — `GamepadState` + 4 raw-поля.
- [`gamepad_globals.cpp`](../../new_game/src/engine/input/gamepad_globals.cpp) — расширенный initializer.
- [`gamepad.cpp`](../../new_game/src/engine/input/gamepad.cpp) — копия raw до override'а в обоих путях (DS, SDL3).
- [`input_frame.cpp`](../../new_game/src/engine/input/input_frame.cpp) — чтение raw для stick virtual directions.
- [`gamemenu.cpp`](../../new_game/src/ui/menus/gamemenu.cpp), [`frontend.cpp`](../../new_game/src/ui/frontend/frontend.cpp) — D-pad как третий источник.
- [`migration_checklist.md`](migration_checklist.md) пункт 3 — обновлено правило про D-pad: ЧИТАТЬ через `rgbButtons[11..14]` параллельно стику, **НЕ** полагаться на стик-mirror (он деструктивный).

**Подтверждено пользователем:** все четыре пары источников теперь корректно гасят друг друга, нав плавная.

---

## 2026-05-05 — Phase 4 шаг 3: vehicle siren — sticky press_pending для гейпада + drain pattern

Закрыт класс багов "сирена переключается несколько раз при зажатии" + lost-press на низких physics Hz.

**До:**
```cpp
if (input & INPUT_CAR_KB_SIREN)  veh->DControl |= VEH_SIREN;  // ← level-trigger каждый physics tick
if (input & INPUT_CAR_PAD_SIREN) veh->DControl |= VEH_SIREN;
```
`do_car_input` каждый тик: `if (veh->DControl & VEH_SIREN) siren(veh, !veh->Siren);` → toggle на каждом тике пока зажата → конечное состояние зависит от physics_hz × hold_duration.

**Эволюция фикса:**

**Попытка 1 (отвергнута):** local static edge-detect на level-сигнале (`was_kb_siren`, `was_pad_siren`) внутри `apply_button_input_car`. Чинит multi-toggle на любых Hz. **НО:** не ловит tap'ы целиком уместившиеся между двумя physics-тиками. На 1 Hz physics (debug-mode) обычное нажатие 150ms полностью между тиками → level signal на обоих тиках = "не зажата" → нажатие потеряно. Пользователь в тестировании это поймал.

**Попытка 2 (принята):** sticky press_pending для гейпада в input_frame + drain pattern в каждом physics-тике (даже когда не в машине).

**Реализация:**

1. **Gamepad sticky press_pending в input_frame** (раньше был только для клавиатуры):
   - `s_btns_press_pending[]` массив. Latched на rising edge каждого render-frame snapshot'а в `input_frame_update`.
   - `input_btn_press_pending(idx)` / `input_btn_consume(idx)` — API mirrored с keyboard версией.
   - Семантика: "был ли rising edge с момента последнего consume" — survives across render frames.

2. **`apply_button_input_car`** — siren bit ставится только если `input_*_press_pending` true. Consume вызывается **безусловно** (drain каждый physics tick во время вождения).

3. **`do_packets` else-ветка (когда игрок не в машине)** — также консьюмит pending обеих кнопок каждый physics tick. Это решает leak'и: Triangle для menu cancel или SPACE для jump action ставят pending где угодно, но если apply_button_input_car не запущен (не в машине) — некому consume'ить → flag сидел бы навсегда. С drain'ом каждый physics tick (вне машины) flag успевает очиститься до того как игрок сядет в машину → нет spurious toggle на первом тике вождения.

**Drain rule (общий принцип для будущих consumer'ов press_pending):**

Если потребитель имеет "active state" (когда обрабатывает pending) и "inactive state" (когда не обрабатывает), а кнопка которой он слушает — dual-use (используется ещё кем-то для другого), то в inactive state потребитель должен **тоже** consume'ить pending'и каждый physics tick. Это превращает "sticky flag indefinitely" в "sticky flag только на 1 physics tick" — достаточно для физического consumer'а, не достаточно для накопления leak'а через context switches.

Этот паттерн задокументирован в `input_frame.h` для будущих consumer'ов.

**Почему именно press_pending, а не just_pressed:**

`just_pressed` true ровно один render frame. Если physics tick запускается на render frame'е без rising edge — пропуск. На 240 FPS render + 20 Hz physics = только 1 из 12 render frame'ов имеет физтик → 1/12 шанс что физтик попадёт ровно на frame с rising edge → большая вероятность пропуска. Sticky pending устраняет этот race.

**Что закрыто:**
- Issue #13 в [`fps_unlock_issues_resolved.md`](../fps_unlock/fps_unlock_issues_resolved.md) — multi-toggle на любых Hz **И** lost-press на низких Hz.

**Изменения:**
- [`input_frame.h`](../../new_game/src/engine/input/input_frame.h) — `input_btn_press_pending` / `input_btn_consume` API.
- [`input_frame.cpp`](../../new_game/src/engine/input/input_frame.cpp) — `s_btns_press_pending[]` array, latch в update, getter/consumer.
- [`apply_button_input_car`](../../new_game/src/game/input_actions.cpp) — siren-bit setting через press_pending+consume.
- [`do_packets` else-branch](../../new_game/src/game/input_actions.cpp) — drain pending'ов каждый физтик когда не в машине.

---

## 2026-05-05 — Phase 4 шаг 4: миграция `check_debug_timing_keys` (game.cpp)

Простая миграция — debug-клавиши со своими static prev-state'ами на унифицированный API. Не было бага, просто чище.

**До:**
5 static prev переменных (`k1_prev`, `k2_prev`, `k3_prev`, `k9_prev`, `k0_prev`), у каждой паттерн `bool kN = Keys[KB_N] != 0; if (kN && !kN_prev) { ... } kN_prev = kN;`.

**После:**
`if (input_key_just_pressed(KB_N)) { ... }` для каждой клавиши. 5 static удалены.

**Touchpad оставлен на static prev (`tp_prev`)** — DualSense touchpad click читается через `ds_get_input` напрямую, не через `gamepad_state.rgbButtons[]`, так что input_frame его не видит. Это **legitimate exception**, не leak — input_frame не претендует на полный покрытие всех источников DualSense, только rgbButtons + axes.

**Функционально идентично** (rising edge → action), плюс слегка устойчивее в edge-кейсах: input_frame snapshot независим от мутаций `Keys[]` другими consumer'ами (если кто-то консьюмит `Keys[KB_N] = 0` между двумя вызовами check_debug_timing_keys, статический prev пропустит rising edge — input_frame нет).

**Изменения:**
- [`game.cpp::check_debug_timing_keys`](../../new_game/src/game/game.cpp) — миграция 5 keyboard edge-detect'ов.

---

## 2026-05-05 — Phase 4 шаг 5: миграция weapon switch (game_tick.cpp::process_controls)

Закрыт #20 в fps_unlock — "одно нажатие кнопки смены оружия прокручивает инвентарь несколько раз на высоком render FPS".

**До:**
```cpp
if (NET_PLAYER(0)->Genus.Player->Pressed & INPUT_MASK_SELECT) {
    CONTROLS_new_inventory(...);
    CONTROLS_rot_inventory(...);
    CONTROLS_inventory_mode = 3000;
}
```

`Player->Pressed = ThisInput & ~LastInput` — корректный edge-detect, но обновляется per physics tick (внутри `process_things` → `process_hardware_level_input_for_player`). `process_controls` где это читается — per render frame. При 240 FPS render + 20 Hz physics между физтиками 12 render frames → все они видят залипший edge-флаг → 12 прокруток за одно нажатие.

**После:**
Edge-detect перенесён на render-frame уровень через input_frame:
```cpp
const SLONG kb_select_key = keybrd_button_use[JOYPAD_BUTTON_SELECT];
constexpr SLONG pad_select_btn = 8; // R3 / right stick click
if (input_key_just_pressed(kb_select_key) || input_btn_just_pressed(pad_select_btn)) {
    ...
}
```

`input_*_just_pressed` true ровно один render frame на rising edge → одно нажатие = одна прокрутка, независимо от render rate.

**Изменения:**
- [`game_tick.cpp::process_controls`](../../new_game/src/game/game_tick.cpp) — триггер weapon switch на input_frame edge-detect.
- Issue #20 [перенесён в resolved](../fps_unlock/fps_unlock_issues_resolved.md).

**Bonus fix — `CONTROLS_inventory_mode` timer FPS-dependence:**

При тестировании выявлен второй связанный баг: popup меню оружия пропадало быстрее на высоком render FPS и на низком physics Hz. Было:
```cpp
CONTROLS_inventory_mode -= TICK_TOCK;
```
`TICK_TOCK` — per-physics-tick interval (~50ms на 20 Hz, saturated 25..100ms range). Декремент происходил per render frame в `process_controls` → 240 FPS × 50ms = 12000 units/sec → 3000-unit timer expire за 0.25 сек вместо ~3 сек.

Фикс — использовать `g_frame_dt_ms` (wall-clock per-render-frame delta):
```cpp
CONTROLS_inventory_mode -= (SLONG)g_frame_dt_ms;
```
Теперь 3000 ms всегда занимает 3 секунды реального времени, независимо от render FPS и physics Hz. Также фиксит "при пропадании меню меняется логика выбора оружия" — это было следствием слишком быстрого expire'а timer'а.

---

## 2026-05-05 — Точка остановки: все известные баги input-системы закрыты

**Состояние коммита (стейб):**

- Phase 1-3 (infrastructure) ✅
- Phase 4.1 (gamemenu pause menu) ✅
- Phase 4.2 (frontend main menu + D-pad-as-separate-source archi fix) ✅
- Phase 4.3 (vehicle siren — sticky press_pending + drain pattern) ✅
- Phase 4.4 (debug timing keys) ✅
- Phase 4.5 (weapon switch render-frame edge-detect) ✅

**Все issue'ы из fps_unlock связанные с input закрыты:**
- #5 (siren toggle) ✅
- #20 (weapon switch) ✅
- #23 (DualSense LED siren / low-HP blink) ✅
- #13 в resolved дополнен описанием siren architectural fix.

**Что дальше — Phase 4-Wide:**

Полная миграция всех discrete + continuous input'ов на input_frame для унификации (см. [`current_plan.md`](current_plan.md) раздел "Phase 4-Wide"). Не блокер, можно делать после стабилизации текущего стека миграционных фиксов.

Финальный пункт после Wide-миграции — аудит что `Keys[]` / `rgbButtons[]` / `the_state.l*/r*/trigger_*` не используются как источник истины потребителями (только как legacy backing-store).

---

## 2026-05-05 — Phase 4-Wide шаг 1: `special_keys` (game.cpp) — debug F-клавиши

Начало Phase 4-Wide migration. 6 edge-detect патернов мигрированы в одной функции — все либо static `was_pressed`, либо `Keys[X] = 0` consume-паттерны.

| Клавиша | Действие | До | После |
|---|---|---|---|
| Ctrl+Q | quit | `if (Keys[KB_Q])` (level) | `input_key_just_pressed(KB_Q)` |
| F2 | CRT shader toggle | static `f2_was_pressed` | `input_key_just_pressed(KB_F2)` |
| F8 | single-step toggle | `Keys[X] = 0` consume | `input_key_just_pressed(KB_F8)` |
| F10 | farfacet debug | static `f10_was_pressed` | `input_key_just_pressed(KB_F10)` |
| F11 | input debug panel | static + consume | `input_key_just_pressed(KB_F11)` |
| KB_INS | step-once | `Keys[X] = 0` consume | `input_key_just_pressed(KB_INS)` |

**Изменения:** [`game.cpp::special_keys`](../../new_game/src/game/game.cpp) — 78 строк → 27 (–24 строки).

`ControlFlag` (level-state Ctrl modifier) **не трогается** — это отдельный механизм отслеживания модификатора, не discrete-event input. Будет рассмотрен отдельно если понадобится.

Все 6 действий — debug-only (`allow_debug_keys` гейт на 5 из 6, только Ctrl+Q использует ControlFlag для гейта). Влияние на обычный геймплей нулевое.

---

## 2026-05-05 — Phase 4-Wide шаг 2: `process_controls` (game_tick.cpp) — массовая миграция

Большая партия — ~30 разных edge-detect и level-read паттернов в одной функции, преимущественно debug-клавиши и cheat-комбинации.

**Категории миграции:**

| Паттерн до | Паттерн после | Использовалось в |
|---|---|---|
| `if (Keys[X]) { Keys[X] = 0; ... }` (consume) | `if (input_key_just_pressed(X)) { ... }` | RBRACE/LBRACE (camera switch), P (camera focus, save game), F12+Shift (cheat), F3 (load/save), F4 (clouds), TILD (detail), E (vehicle spawn), P7/P5/P2/P3 (pyrotest), L (dlight), FORESLASH (stealth), O (object), A/I/J (PCOM), D+Shift/D!Shift (slight/debugger), G (gun/teleport), H (plan view), M (mine), и т.д. |
| `if (Keys[X])` без consume (level-read intent) | `if (input_key_held(X)) { ... }` | J/I (continuous overlay draw), W (continuous water spawn), POINT (continuous smoke), Y (continuous fastnav debug), U (continuous barrel hit), Q (continuous road debug) |
| `Keys[X] = 0` init reset | удалить | KB_D на GAME_TURN<=1 (input_frame не leak'ает held-from-before) |
| `Keys[KB_LSHIFT] \|\| Keys[KB_RSHIFT]` direct read | `ShiftFlag` | console text input shift modifier check |

**Edge-cases которые сохранены verbatim** (pre-existing quirks):
- KB_D handler вне `allow_debug_keys` гейта (line 1190+) — debug-сообщение для не-debug пользователей. Pre-existing scope leak, не задача миграции.
- KB_D с `&& !ShiftFlag` внутри `if (ShiftFlag)` блока — dead code (всегда false). Comment добавлен, код preserved.

**Изменения:** [`game_tick.cpp::process_controls`](../../new_game/src/game/game_tick.cpp) — net –24 строки (62 insertions, 86 deletions).

**Проверка**: `grep "Keys\[KB_" game_tick.cpp` после миграции находит только matches в комментариях. Чтения старого API из этой функции — нет.

**Что НЕ закрыто:**
- ShiftFlag/ControlFlag — derived модификаторные обёртки maintained event hook'ом из `Keys[KB_LSHIFT/RSHIFT/LCONTROL]`. Это легитимный backing-store слой (по плану), потребители читают именно ShiftFlag, не Keys[]. Не трогаем.
- Console input area — `LastKey` (text-input source-of-truth), `InkeyToAscii[]` table — это text-input mechanism, не discrete-event input. Не миграция input_frame'а.

---

## 2026-05-05 — Phase 4-Wide шаг 3: `frontend.cpp` (oбщие меню-handler'ы)

Помимо уже мигрированной `FRONTEND_input` (Phase 4.2), были 22 остаточных `Keys[KB_]` чтения в FRONTEND-related функциях. Все мигрированы.

| Клавиши | Действие | Паттерн до | Паттерн после |
|---|---|---|---|
| KB_1/2/3/4 | menu_theme select (debug-only) | `if (Keys[X]) { Keys[X]=0; ... }` consume | `if (input_key_just_pressed(X)) { ... }` |
| KB_END / KB_HOME | jump к last/first menu item | consume | just_pressed |
| KB_ENTER / KB_SPACE / KB_PENTER | confirm | consume | just_pressed (по OR на 3 источника) |
| KB_ESC (regular nav) | cancel/back | consume | just_pressed |
| KB_PPLUS / KB_ASTERISK (Ctrl+Shift) | debug cheat (advance complete_point) | consume | just_pressed |

**Изменения:** [`frontend.cpp`](../../new_game/src/ui/frontend/frontend.cpp) — net –16 строк.

**Оставшиеся `Keys[]` writes** (legitimate legacy bridges, не reads):
- [`frontend.cpp:2383`](../../new_game/src/ui/frontend/frontend.cpp#L2383) — `Keys[KB_ESC] = 0` consume в `grabbing_pad` ESC handler'е (предотвращает double-fire в regular nav на следующем кадре). Уже задокументирован комментарием.
- [`frontend.cpp:2529`](../../new_game/src/ui/frontend/frontend.cpp#L2529) — `Keys[LastKey] = 0` в grabbing_key text-input path (после ребинда). Text-input mechanism, не discrete-event input.

**Проверка**: `grep "Keys\[KB_"` в frontend.cpp находит только 1 match (line 2383, legitimate legacy bridge).

---

## 2026-05-05 — Phase 4-Wide шаг 4: cutscene / attract loops (playcuts.cpp, attract.cpp)

Custom render loops которые крутятся отдельно от основного game_loop, но через `SHELL_ACTIVE` (= `LibShellActive`) вызывают `input_frame_update` каждую итерацию → snapshot fresh.

**playcuts.cpp::PLAYCUTS_Play** (dev-only test cutscene player):
- `Keys[KB_SPACE] = 0` (init и post) → `input_key_consume(KB_SPACE)` (init) + drop (post — input_key_just_pressed self-clears).
- `!Keys[KB_SPACE]` (loop exit condition) → `!input_key_just_pressed(KB_SPACE)`.

**attract.cpp** (главный attract loop):
- `if (ControlFlag && Keys[KB_Q])` (debug quit) → `if (ControlFlag && input_key_just_pressed(KB_Q))`.
- `if (Keys[KB_LEFT] || Keys[KB_RIGHT] || Keys[KB_UP] || Keys[KB_DOWN] || Keys[KB_SPACE] || Keys[KB_ENTER])` (wake screen) → 6× `input_key_held(X)` (level intent — экран остаётся пока юзер взаимодействует).
- Обновлён комментарий про стейл `Keys[]` — больше не актуален т.к. input_frame snapshot обновляется в начале каждой итерации `SHELL_ACTIVE`.

**Проверка**: `grep "Keys\["` находит 0 matches кода (только комментарий в attract.cpp).

**outro_main.cpp** не трогаем — там grep'ом нашёлся только comment, никаких реальных reads.

**video_player.cpp** — вынесен в шаг 5 (мигрирован).

---

## 2026-05-05 — Phase 4-Wide шаг 5: input_actions.cpp + video_player.cpp

**input_actions.cpp** — массовая миграция central input layer'а.

`get_hardware_input` (keyboard side, ~lines 3290-3373):
- Movement (FORWARDS/BACK/LEFT/RIGHT) → `input_key_held` per direction (continuous level intent).
- SELECT, JUMP, PUNCH, KICK, ACTION → `input_key_held` (level masks).
- F5/F6/F7 (camera type 1/2/3) → `input_key_just_pressed` (consume pattern).
- JOYPAD_BUTTON_CAMERA (CAM_BEHIND), CAM_LEFT, CAM_RIGHT → `input_key_just_pressed` (consume pattern).
- Pre-existing typo dropped: `Keys[JOYPAD_BUTTON_CAM_LEFT] = 0` (line 3348, missing keybrd_button_use indirection — would clear scancode equal to enum value, dead consume).

`apply_button_input_first_person`:
- 1stPerson toggle → `input_key_held` + `input_btn_held(joypad_button_use[JOYPAD_BUTTON_1STPERSON])` (replaced `the_state.rgbButtons[]` direct read).
- Aim mode pitch/yaw arrow keys (4×) → `input_key_held`.

`apply_button_input` weapon hotkeys KB_1..KB_8 → `input_key_just_pressed` (consume pattern, 8 patterns collapsed into oneliners).

**video_player.cpp** — FMV playback custom event loop. Раньше использовал raw SDL events напрямую (бypassed input_frame entirely). Теперь:

1. Добавлен `sdl3_to_game_scancode` (public wrapper в sdl3_bridge.h) для перевода SDL scancode → game KB_*.
2. В custom SDL_PollEvent loop key-events диспатчатся через `keyboard_key_down/up` (которые в свою очередь обновляют input_frame через `input_frame_on_key_down/up` event hook).
3. `input_frame_update()` вызывается per loop iteration после event drain → snapshot fresh.
4. SDL_SCANCODE_X switch (debug timing keys 1/2/3/9/0) → `input_key_just_pressed(KB_X)`.
5. Skip-on-key (ESC/Enter/Space) → `input_key_just_pressed`.
6. SDL_EVENT_GAMEPAD_BUTTON_DOWN + DualSense ds_prev edge-detect → unified `input_btn_just_pressed(0..16)` loop. Cross/Circle/Start (DualSense) попадают в индексы 0/1/6 через rgbButtons mirror.
7. **Mouse skip остался на raw SDL_EVENT_MOUSE_BUTTON_DOWN** — input_frame ещё не отслеживает mouse buttons. Помечено как future work.

При входе в loop вызывается одно `input_frame_update()` чтобы baseline snapshot был установлен → buttons/keys held coming into video не fire'ят как rising edge.

**Изменения:**
- [`input_actions.cpp`](../../new_game/src/game/input_actions.cpp) — get_hardware_input + first_person + weapon hotkeys.
- [`sdl3_bridge.h/.cpp`](../../new_game/src/engine/platform/sdl3_bridge.h) — экспорт `sdl3_to_game_scancode`.
- [`video_player.cpp`](../../new_game/src/engine/video/video_player.cpp) — полная миграция FMV loop на input_frame.

**Что ещё не закрыто (выявлено grep'ом):**
- `panel.cpp`, `aeng.cpp`, `person.cpp`, `widget.cpp`, `thing.cpp`, `outro_os.cpp`, `outro_main.cpp`, `farfacet.cpp`, `debug_help.cpp`, `input_debug.cpp`, `message.cpp`, `ware.cpp`, `elev.cpp`, `pcom.cpp` — ещё имеют чтения `Keys[]`. Будут мигрированы следующими шагами.

---

## 2026-05-05 — Phase 4-Wide шаг 6: финальный массовый pass по оставшимся файлам

Большой batch: все остальные потребители `Keys[KB_*]` мигрированы на input_frame. После этого pass `Keys[KB_*]` reads встречаются **только в внутренних message-bus каналах** (gamemenu, widget) — это документированное исключение с пояснением в коде.

| Файл | Что мигрировано |
|---|---|
| [`panel.cpp`](../../new_game/src/ui/hud/panel.cpp) | KB_V version overlay → `input_key_held` (continuous level read) |
| [`aeng.cpp`](../../new_game/src/engine/graphics/pipeline/aeng.cpp) | Ctrl+L outside toggle → `input_key_just_pressed`, KB_S screenshot → `input_key_just_pressed` |
| [`person.cpp`](../../new_game/src/things/characters/person.cpp) | Shift+Q debug speed multiplier → `input_key_held` |
| [`thing.cpp`](../../new_game/src/things/core/thing.cpp) | KB_COLON slow-mo → `input_key_just_pressed` |
| [`outro_os.cpp`](../../new_game/src/outro/core/outro_os.cpp) | KB_ESC outro skip → `input_key_just_pressed` |
| [`debug_help.cpp`](../../new_game/src/engine/debug/debug_help/debug_help.cpp) | KB_F1 helper overlay → `input_key_just_pressed` |
| [`input_debug.cpp`](../../new_game/src/engine/debug/input_debug/input_debug.cpp) | refresh_nav (custom prev-state edge detect) → `input_key_just_pressed`. Page switch keys (1/2/3), TAB, ESC → `input_key_just_pressed`. `input_debug_key_held()` helper → `input_key_held`. |
| [`message.cpp`](../../new_game/src/engine/console/message.cpp) | Numpad +/- /Enter scroll → `input_key_held` (continuous) |
| [`ware.cpp`](../../new_game/src/buildings/ware.cpp) | KB_T early-return gate → `input_key_held` |
| [`game.cpp`](../../new_game/src/game/game.cpp) (missed in earlier pass) | Ctrl press toggle (debug_overlay_locked_on) → `input_key_just_pressed`; playback_game_keys SPACE/ENTER/PENTER → `input_key_just_pressed`; deadcivs warning loop break condition → `input_key_just_pressed` |

**Файлы где Keys[] осталось как комментарий или legacy bridge write — не мигрировано:**
- `outro_main.cpp` — только comment.
- `farfacet.cpp` — только comment.
- `pcom.cpp` — только закомментированный код.
- `elev.cpp` (lines 1962-1972) — bulk write `Keys[X] = 0` для cleanup перед elevator action. Write-only, no read. Legacy backing-store maintenance, оставлен.

**Внутренние message-bus каналы через Keys[] (legitimate exception, документированы в коде):**
- [`widget.cpp::FORM_KeyProc`](../../new_game/src/ui/menus/widget.cpp) — gamepad→keyboard bridge для menu forms. Bridge pишет `Keys[KB_ENTER]=1`, FORM_KeyProc читает. Замкнутый внутренний канал.
- [`gamemenu.cpp`](../../new_game/src/ui/menus/gamemenu.cpp) — gamepad→ESC/ENTER bridge (Start/Triangle/Cross → синтезированный keyboard press). Тот же паттерн.

См. [current_plan.md](current_plan.md) раздел "Документированное исключение — внутренние message-bus каналы".

**Архитектурное решение:** для миграции этих оставшихся каналов нужен "synthesize input event" API в input_frame (с правильным timing'ом, поскольку input_frame_update уже прошёл для текущего кадра). Это отдельная задача — пока не блокер.

**Всего после этого pass:**
- Hardware input reads через input_frame: ✅ полностью.
- Internal bridges Keys[] message-bus: ⚠️ остаются 2 места (widget, gamemenu) — задокументированы.
- Legacy backing-store writes (`Keys[X] = 0` cleanup): остаются как pre-existing maintenance.

---

## 2026-05-05 — Phase 4-Wide шаг 7: фиксы по результатам тестирования

Найдены 2 связанных бага по итогам пользовательского теста:

### Bug 1: Triangle для закрытия pause menu в полицейской машине → siren toggle

**Симптом:** в pause menu Triangle закрывает меню → выходишь обратно в гейплей за рулём патрульной машины → siren сразу включается/выключается.

**Причина:** gamemenu's gamepad bridge видит `input_btn_just_pressed(3)` → пишет `Keys[KB_ESC]=1` для закрытия меню. Но **не дренит** `input_btn_press_pending(3)` который был set'нут на rising edge Triangle. Pause-menu fade-out фаза → physics всё ещё тикает (slow-motion) → `apply_button_input_car` читает press_pending(3) (всё ещё set) → siren toggle.

**Фикс:** в gamemenu's gamepad bridge добавлены `input_btn_consume(3)` и `input_btn_consume(0)` при потреблении Triangle/Cross для menu action. Pending дренится сразу, физика после resume не видит stale flag'а.

### Bug 2: SPACE/ENTER/PENTER/ESC для menu confirm/cancel → действие в гейплее после resume

**Симптом:** SPACE в меню паузы → выбирается "Resume" → меню закрывается → перс прыгает (SPACE = JUMP key для геймплея).

**Причина:** SPACE = `keybrd_button_use[JOYPAD_BUTTON_JUMP]`. INPUT_MASK_JUMP set'ится в `get_hardware_input` через **level read** `input_key_held(KB_SPACE)`. Меню закрывается мгновенно (`GAMEMENU_initialise(NONE)` сразу сбрасывает slowdown в 0x10000 — fade-out НЕТ, проверено grep'ом). Физика возобновляется на следующем тике. SPACE всё ещё физически держится → level read = true → INPUT_MASK_JUMP → jump.

Та же проблема для ENTER/PENTER (если они привязаны к чему-то в геймплее) и ESC. Общий класс: **любая кнопка которой меню "закрылось" остаётся held → её level read leak'ит в гейплей до физического release**.

**Сначала пробовала "fade-out gate" в get_hardware_input** (`if (GAMEMENU_menu_type == NONE && GAMEMENU_slowdown < FULL) return 0`) — но обнаружила что `GAMEMENU_initialise` сбрасывает `slowdown = 0x10000` мгновенно, gate никогда не срабатывает. Откатила.

**Попытка 1 (отвергнута) — `keyboard_key_up(KB_X)`** при menu consume:
Идея: вызвать стандартный `keyboard_key_up` для синтеза release. Но он только обновляет `s_keys_event_held = 0`. Snapshot `s_keys_curr` уже зафиксирован в начале текущего `LibShellActive` (перед запуском GAMEMENU_process). Downstream consumers (например `process_controls` weapon switch reading `input_key_just_pressed(KB_ENTER)`) **в этом же кадре** видят `s_keys_curr = 1` → just_pressed остаётся true → action триггерится.

ENTER bug: ENTER в меню paus close menu → процесс_controls в том же кадре читает `input_key_just_pressed(kb_select_key)` (KB_ENTER) → попап выбора оружия открывается.

**Правильный фикс — `input_key_force_release(KB_X)`:**
Новая API в input_frame, которая обнуляет ВСЁ:
- `s_keys_curr[X] = 0` — текущий snapshot (immediate effect для same-frame reads).
- `s_keys_event_held[X] = 0` — для следующего кадра.
- `s_keys_pressed_during_frame[X] = 0` — same-frame press marker.
- `s_keys_press_pending[X] = 0` — sticky pending.

После force_release `input_key_held(X)` и `input_key_just_pressed(X)` сразу возвращают false. Дополнительные SDL key_down events при физическом release+repress нормально сбросят и поднимут event_held.

Реализовано для:
- KB_ESC (toggle pause / back/cancel).
- KB_ENTER, KB_SPACE, KB_PENTER (confirm).

Для геймпада (Triangle/Cross) — отдельно через press_pending drain (см. Bug 1 выше). Между keyboard и gamepad механики разные: keyboard event-driven (force_release работает на event-tracked state), gamepad polled (rgbButtons обновляются каждый input_frame_update, force_release не имеет смысла; press_pending drain — правильный механизм).

**Изменения:**
- [`input_frame.h/.cpp`](../../new_game/src/engine/input/input_frame.h) — новая API `input_key_force_release(scancode)`.
- [`gamemenu.cpp`](../../new_game/src/ui/menus/gamemenu.cpp) — gamepad bridge: drain `input_btn_press_pending(3/0)`. Keyboard consume: вызов `input_key_force_release(KB_X)` параллельно `Keys[KB_X] = 0`.
- Гейт в get_hardware_input откачен (был мёртвый код — slowdown сбрасывается мгновенно).

### Bonus: подтверждено пользователем

- Баг "геймпадный стик не работает в первые секунды миссии" из known_issues_and_bugs.md — пользователь подтвердил что Phase 4-Wide миграция случайно его пофикcила (точная причина не изолирована, но архитектурный pipeline теперь правильно инициализируется). Перенесён в resolved.

---

## 2026-05-05 — ТЕКУЩЕЕ СОСТОЯНИЕ (для handoff)

### ✅ Полностью мигрировано (потребители читают через input_frame)

- Все `Keys[KB_*]` reads в потребительских функциях. Подтверждено grep'ом: `grep "Keys\[KB_"` без legitimate exceptions возвращает только writes (legacy backing-store).
- Все `the_state.rgbButtons[]` reads вне `get_hardware_input` слоя.
- Меню навигация (gamemenu, frontend), debug-клавиши, weapon switch, vehicle siren, video player, cutscene skip, attract loop.
- Menu close leak fixed через новый API `input_key_force_release(scancode)` — обнуляет CURRENT snapshot чтобы same-frame downstream consumers видели released сразу.

### ⚠️ Documented exceptions (НЕ мигрированы, легитимно)

1. **`keyboard.cpp` event hook** (lines 12-14): `ShiftFlag = Keys[KB_LSHIFT] || Keys[KB_RSHIFT]; ControlFlag = Keys[KB_LCONTROL]; AltFlag = Keys[KB_LALT] || Keys[KB_RALT];` — это **layer который поддерживает legacy backing-store**. Source-of-truth слой, не consumer.
2. **Internal Keys[]-as-message-bus** в [`widget.cpp::FORM_KeyProc`](../../new_game/src/ui/menus/widget.cpp) и [`gamemenu.cpp` ESC handler](../../new_game/src/ui/menus/gamemenu.cpp) — gamepad→keyboard bridge synthesises `Keys[KB_X]=1`, тот же файл читает обратно. Замкнутый канал, не hardware input.
3. **`get_hardware_input` в `input_actions.cpp`** (lines ~3026-3263): central input pipeline. Читает `the_state.rgbButtons[]`, `the_state.lX/lY/rX/rY`, `the_state.trigger_left/right` напрямую для агрегации в `INPUT_MASK_*` биты. Это **implementation слой input_frame'а соседствующий**, не leaf consumer. Однако строго говоря **тоже потребитель** — может быть мигрирован на `input_btn_held` / `input_stick_x_raw` / `input_trigger_raw` если расширить input_frame API.
4. **`game.cpp:608`** `for (i=0; i<=9; i++) if (the_state.rgbButtons[i])` — playback skip detection, читает все кнопки. Легко мигрируется на цикл `input_btn_held(i)`.
5. **`elev.cpp` lines 1962-1972** — bulk `Keys[X] = 0` writes (cleanup), не reads. Не критично.
6. **`outro_main.cpp`, `farfacet.cpp`, `pcom.cpp`** — только comment'ы.

### 🚧 Что можно ещё сделать (если пользователь хочет 100% покрытия)

1. **Migrate `get_hardware_input` core**:
   - Добавить в input_frame API: `input_btn_held(N)` уже есть, использовать вместо `the_state.rgbButtons[N]`.
   - Добавить `input_stick_x_raw(stick) / y_raw(stick)` returning raw int 0..65535 — для packing в bits 18-31 которое использует get_hardware_input.
   - Добавить `input_trigger_raw(idx)` returning byte 0..255 — для weapon_feel (currently reads `the_state.trigger_left/right`).
   - Добавить `input_gamepad_connected()` / `input_gamepad_dpad_active()` accessors.
   - Заменить ~25 чтений в get_hardware_input на новые APIs.
2. **Migrate `game.cpp:608`** playback skip — `for (i=0; i<=9; i++) if (input_btn_held(i))`.
3. **Migrate widget.cpp / gamemenu.cpp internal bridge** — потребует "synthesize input event" API в input_frame с правильным timing'ом (поскольку input_frame_update уже прошёл в текущем кадре). Архитектурно сложнее.

### Не Phase 4-Wide, отдельные задачи

- Mouse buttons не в input_frame — добавить если будет необходимо (пока не блокер).
- DualSense touchpad — отдельный источник через `ds_get_input`, не через rgbButtons.

### API расширения этой сессии

- **`InputAutoRepeat::tick_combined(any_jp, any_held)`** — combined-source auto-repeat (gamemenu/frontend nav).
- **`input_btn_press_pending(idx)` / `input_btn_consume(idx)`** — gamepad sticky pending для physics-tick consumer'ов (vehicle siren).
- **`input_key_force_release(scancode)`** — обнуляет CURRENT snapshot для same-frame consumed-press suppression (menu close).
- **`gamepad_state.lX_raw/lY_raw/rX_raw/rY_raw`** — pre-D-Pad-override stick values (for menu antagonist stick+D-pad).

### Связь с fps_unlock

Все open issue'ы которые решались миграцией закрыты:
- #5 (siren toggle) ✅
- #20 (weapon switch responsiveness) ✅
- #23 (DualSense LED) ✅
- Ranking PCOM #15 (общая отзывчивость управления зависит от physics Hz) частично закрыт через push в input_frame, но lost-press на extreme low Hz остаётся теоретическим — не репродуцится в обычном диапазоне.

Open в fps_unlock_issues.md:
- #14 (визуальный статтер при переключении render cap) — не input.
- #15 (общая latency physics-bound input) — частично закрыто.
- #17 (spiral of death) — не input.
- #16 (ревизия таймингов после 20 Hz) — не input.
- #22 (perf-diag режим) — не input.
- #24 (граната не интерполируется) — записан в этой сессии, не Phase 4 work.
