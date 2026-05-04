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

## 2026-05-05 — Точка остановки: gamemenu готов, остальные потребители ждут

**Состояние коммита (стейб):**

- Phase 1 (skeleton) ✅
- Phase 2 (stick virtual directions) ✅
- Phase 3 (universal auto-repeat + InputAutoRepeat для combined sources) ✅
- Phase 4.1 (gamemenu) ✅ — все edge-cases отработаны (4 бага найдены и пофикшены в процессе тестирования)
- `migration_checklist.md` написан со всеми граблями ✅

**Что готово:** инфраструктура, паттерн миграции отлажен на gamemenu. Будущий потребитель + чеклист = должно идти гладко.

**Список оставшихся потребителей (по убыванию приоритета):**

1. **`frontend.cpp`** — главное меню (своя ad-hoc nav через `dir_next_fire`). Аналогично gamemenu — InputAutoRepeat + antagonist suppression. Закрывает unification "залипаний" во всех меню.
2. **`vehicle.cpp`** siren toggle (#5 уже won't-fix → resolved, но архитектурно через input_frame будет нормально работать на любых Hz).
3. **`check_debug_timing_keys`** — debug-клавиши, простая миграция (только `input_key_just_pressed`).
4. **`game_tick.cpp`** weapon switch (#20) — проверить актуальность бага сначала.
5. Прочие места по обнаружении.

**Каждая миграция = отдельный коммит** с тестированием. Сверяться с [migration_checklist.md](migration_checklist.md) перед каждой.

**Связь с fps_unlock:**
- #5, #15, #20 в `fps_unlock_issues.md` остаются открытыми пока соответствующие миграции не подтверждены пользователем.
- При завершении миграции потребителя который закрывает один из issue'ев — переносить issue в `fps_unlock_issues_resolved.md` со ссылкой на input_system commit.
