# Input System — текущая работа: модуль `input_frame`

> **Статус (2026-05-05):** инфраструктура (Phases 1–3) готова; **gamemenu, frontend, vehicle siren мигрированы**.
> Остаются миграции debug-keys, weapon switch.
> Детальная хронология → [changelog.md](changelog.md), грабли при миграции →
> [migration_checklist.md](migration_checklist.md).
> **Большой план с actions/ремапом** → [full_plan_deferred.md](full_plan_deferred.md) (отложен).

## Зачем

Обработка ввода в проекте сейчас неоднородна:

- **Edge-detect размазан** — `gamemenu.cpp` хранит свои `gm_last_start/triangle/cross/dir`, `check_debug_timing_keys` — свой `static prev`, `frontend.cpp` — свой `dir_next_fire`, gamepad-libs — свои `s_was_active/last_*`. Везде разная логика, разные тайминги, легко рассинхронизировать.
- **Часть code paths читает level-trigger без edge-detect** — `apply_button_input_car` из render-frame ставит `VEH_SIREN` в `DControl` уровневым OR'ом, `do_car_input` per physics tick читает уровень → siren toggle делает несколько раз пока кнопка зажата (#5).
- **Continuous state vs discrete events перемешаны** — стик хорошо работает для движения (continuous), но в меню нужно "стик ВВЕРХ нажат" (discrete press event), и каждое меню реализует это по-своему.
- **Auto-repeat ("залипания" в меню)** — везде с разной скоростью и логикой. Должно ощущаться одинаково во всех менюшках.

## Цель

Один центральный модуль на render-frame boundary, который **готовит** все необходимые input-стейты до начала любой игровой логики. Потребители читают готовое, не дублируют edge-detect / auto-repeat / stick-as-direction логику.

## Scope

### Что входит (текущая итерация)

1. **Sampling on render-frame boundary** — снимок состояния клавиатуры и геймпада в начале game loop.
2. **Edge-detect** — `just_pressed`, `just_released` для всех физических клавиш и кнопок.
3. **Sticky press flag** — set в SDL event handler'е на key-down, ловит fast presses (нажатие + отпускание между двумя render frame'ами). Cleared на consume.
4. **Stick-as-direction emulation** — стики (левый и правый) приводятся к виртуальным "стрелкам" up/down/left/right с edge-detect. Решает одновременное нажатие противоположных направлений (cancel).
5. **Universal auto-repeat** — единая cadence "залипаний" (initial delay + repeat rate) для меню. API типа `just_pressed_or_repeat(key)` отдаёт true на первое нажатие и каждый repeat-tick.
6. **API в стиле Unity Input** — простые функции, читаемые из любого места кода.

### Что отложено

- **Actions / mask abstractions** — текущая обработка через `Player->Pressed & INPUT_MASK_*` остаётся как есть, новый модуль работает на уровне физических клавиш (`KB_*`) и геймпад-кнопок (rgbButtons indexes). Action-слой можно надстроить позже.
- **Remap UI** — переназначение клавиш не входит. Будет в большом плане → [full_plan_deferred.md](full_plan_deferred.md).
- **Контекстные группы (на ходу / в машине)** — тоже большой план.
- **Reverse-lookup glyph'ов** — тоже большой план.
- **Continuous-state latency для движения и камеры** — на 20 Hz физики работает приемлемо (по словам пользователя), не блокер. Для меню эту проблему закрывает stick-as-direction emulation выше (там нужны discrete events, не continuous).

### Что точно НЕ ломается

- `Keys[]` массив, `the_state.rgbButtons[]`, `INPUT_MASK_*`, `Player->Pressed`, `apply_button_input*`, `get_hardware_input` — всё остаётся доступным как было. Новый модуль работает **параллельно**, не вместо. Старые потребители продолжают работать пока их по одному не мигрируем.

## API (черновик)

Расположение: `new_game/src/engine/input/input_frame.{h,cpp}`.

### Tick

```cpp
// Зовётся раз в начале каждого render frame, до GAMEMENU_process / process_controls /
// процесса физики. Делает snapshot, считает edges, тикает auto-repeat таймеры.
void input_frame_update();
```

### Базовое состояние клавиш и кнопок

```cpp
// Текущее состояние (level). Эквивалент чтения Keys[X] / rgbButtons[X].
bool input_key_held(SLONG kb_code);
bool input_btn_held(SLONG gamepad_btn_idx);

// Rising edge на текущий render frame. True ровно один frame.
bool input_key_just_pressed(SLONG kb_code);
bool input_btn_just_pressed(SLONG gamepad_btn_idx);

// Falling edge.
bool input_key_just_released(SLONG kb_code);
bool input_btn_just_released(SLONG gamepad_btn_idx);

// Sticky: был ли press (rising edge) с момента последнего consume — даже если
// клавиша уже отпущена. Закрывает fast-press проблему когда press пришёл и
// ушёл между render frame'ами / между physics-тиками.
bool input_key_press_pending(SLONG kb_code);
bool input_btn_press_pending(SLONG gamepad_btn_idx);

// Очистить pending после обработки. Если consumer не вызвал — флаг остаётся
// до следующего consume или явного reset'а.
void input_key_consume(SLONG kb_code);
void input_btn_consume(SLONG gamepad_btn_idx);
```

### Auto-repeat (single source)

```cpp
// Возвращает true:
//  - на первое нажатие (just_pressed)
//  - и далее каждые INPUT_REPEAT_PERIOD_MS если клавиша зажата и прошёл
//    INPUT_REPEAT_INITIAL_MS.
// Константы — единые для всех меню (`INPUT_REPEAT_INITIAL_MS = 400`,
// `INPUT_REPEAT_PERIOD_MS = 150`) в input_frame.cpp.
//
// "Armed" флаг: auto-repeat стреляет только если just_pressed был ЗАХВАЧЕН
// в этой функции — т.е. в этом потребителе была rising edge. Если клавиша
// зажата с другого контекста (gameplay → меню), held=true но armed=false →
// не стреляет до release+repress.
bool input_key_just_pressed_or_repeat(SLONG kb_code);
bool input_btn_just_pressed_or_repeat(SLONG gamepad_btn_idx);
```

### Auto-repeat (combined sources) — `InputAutoRepeat` struct

Когда одно логическое действие имеет **несколько физических источников** (menu nav down = клава ↓ ИЛИ стик ↓ ИЛИ D-pad ↓): использовать единый throttle на OR'нутом combined input. **Не** OR'ить два single-source `_just_pressed_or_repeat` — у них независимые timer'ы, combined OR'ит на ~2× rate из-за фазового сдвига.

```cpp
struct InputAutoRepeat {
    bool was_held = false;
    bool armed = false;
    uint64_t next_fire = 0;
    bool tick_combined(bool any_just_pressed, bool any_held);
};
```

Использование:

```cpp
static InputAutoRepeat ar_down;

const bool any_dn_jp   = input_key_just_pressed(KB_DOWN)
                      || input_stick_just_pressed(INPUT_STICK_LEFT, INPUT_STICK_DIR_DOWN);
const bool any_dn_held = input_key_held(KB_DOWN)
                      || input_stick_held(INPUT_STICK_LEFT, INPUT_STICK_DIR_DOWN);

const bool nav_down = ar_down.tick_combined(any_dn_jp, any_dn_held);
```

Combined rising edge = `any_just_pressed && !was_held`. Это:
- НЕ стреляет если combined уже был зажат с другого контекста (was_held набрался до того как мы захотели его прочитать — wait, нет, was_held стартует false; защита через `any_just_pressed` который от snapshot-level rising edge).
- НЕ перезапускает timer когда добавляется второй источник пока первый ещё зажат.
- Для antagonist (up + down оба зажаты) — suppression делается на стороне consumer'а (см. [`migration_checklist.md`](migration_checklist.md) пункт 5).

### Stick-as-direction (виртуальные стрелки)

```cpp
// Виртуальные направления стика — для меню/UI, где нужны discrete events.
// Threshold с гистерезисом: вход в pressed при > STICK_DIR_PRESS_RAW (4096)
// от центра, выход в released при < STICK_DIR_RELEASE_RAW (2048).
// Одновременное Up + Down (или Left + Right) → cancel оба (в одном стике).

enum InputStickId  { INPUT_STICK_LEFT = 0, INPUT_STICK_RIGHT };
enum InputStickDir { INPUT_STICK_DIR_UP = 0, INPUT_STICK_DIR_DOWN,
                     INPUT_STICK_DIR_LEFT, INPUT_STICK_DIR_RIGHT };

bool input_stick_held(InputStickId stick, InputStickDir dir);
bool input_stick_just_pressed(InputStickId stick, InputStickDir dir);
bool input_stick_just_released(InputStickId stick, InputStickDir dir);
bool input_stick_just_pressed_or_repeat(InputStickId stick, InputStickDir dir);
```

Continuous-доступ к стикам тоже есть (для движения и камеры) — но он по сути обёртка вокруг `the_state.lX/lY/lRx/lRy`:

```cpp
// Float [-1.0, 1.0] с применённым deadzone'ом.
float input_stick_x(StickId stick);
float input_stick_y(StickId stick);
```

### Триггеры (адаптивные)

```cpp
// Float [0.0, 1.0]. Просто wrapper для удобства, сам по себе FPS-bug'ов не вносит.
float input_trigger(SLONG trigger_idx);  // 15=L2, 16=R2 в the_state.rgbButtons
```

## Sticky-флаг — реализация

Цель — не пропускать fast presses когда между двумя render frame'ами клавиша успела пойти вниз и обратно.

**Где set'ится:** в SDL event handler'е (`keyboard_key_down` → `input_frame_on_key_down(scancode)`). Параллельно с записью в `Keys[]` ставится `s_keys_press_pending[X] = 1`.

**Где clear'ится:** только при `input_key_consume(X)` — никогда не сбрасывается автоматически. Это значит pending живёт до тех пор пока кто-то не обработал.

**Правило для потребителей:** если ты обрабатываешь pending-press → consume сразу после обработки. Если только смотришь "был ли press" без действия → не consume.

**Поведение `just_pressed`:** рассчитывается из snapshot'а и prev-snapshot'а на render frame, не из sticky-флага. `just_pressed` отражает edge "видимый из render frame", `pending` — edge "случившийся хоть когда-то с прошлого consume".

Различие важно: `just_pressed` ловит edge ровно на одном frame'е (как Unity GetKeyDown), `pending` — sticky catch-up для медленных потребителей (физика которая может прийти на следующий frame).

## Источник истины snapshot'ов — независим от `Keys[]`

input_frame **не читает** `Keys[]` для своего snapshot'а. Вместо этого хранит свои собственные `s_keys_event_held[]` (set в `keyboard_key_down`, clear в `keyboard_key_up`) и `s_keys_pressed_during_frame[]` (для same-frame press+release visibility).

**Почему так:** потребители (меню) делают `Keys[KB_X] = 0` после consume — это leak'ало бы в snapshot следующего кадра, ломая auto-repeat для зажатых клавиш. См. [changelog.md](changelog.md) "Баг 1: keyboard auto-repeat не работал".

Геймпад snapshot'ится из `gamepad_state.rgbButtons[]` — там никто не консьюмит, поэтому проблемы нет.

## Implementation phases

### ✅ Phase 1 — Скелет модуля

- [x] `input_frame.h` — API
- [x] `input_frame.cpp` — реализация со снапшотами и edge-detect
- [x] Hook в SDL event handler (`keyboard.cpp` → `input_frame_on_key_down/up`)
- [x] Геймпад snapshot из `gamepad_state.rgbButtons[]` (polling, не events)
- [x] Вызов `input_frame_update()` в `LibShellActive` после `sdl3_poll_events`
- [x] Build pass — поведение не менялось

### ✅ Phase 2 — Stick virtual directions

- [x] Реализация виртуальных направлений с hysteresis (press 4096 raw, release 2048 raw — distance from center 32768)
- [x] Edge-detect через prev/curr snapshot
- [x] Mutual exclusion: одновременное Up+Down или Left+Right в одном стике → cancel
- [x] D-pad покрыт автоматически (миррорится в `lY/lX` на gamepad layer)

### ✅ Phase 3 — Universal auto-repeat

- [x] `input_*_just_pressed_or_repeat` для клавиш / кнопок / stick-направлений
- [x] Константы `INPUT_REPEAT_INITIAL_MS = 400`, `INPUT_REPEAT_PERIOD_MS = 150` (взяты из gamemenu.cpp)
- [x] Armed-флаг — auto-repeat стреляет только если `just_pressed` был захвачен в этой call-chain
- [x] `InputAutoRepeat` struct для combined-source auto-repeat'а с `tick_combined(any_just_pressed, any_held)`

### Phase 4 — Миграция потребителей

⚠️ **Перед каждой миграцией сверяться с** [`migration_checklist.md`](migration_checklist.md).

- [x] **`gamemenu.cpp`** (2026-05-05) — pause menu. Triangle/Cross/Start через `input_btn_just_pressed`, Up/Down nav через `InputAutoRepeat::tick_combined` с OR'ом kb+stick+D-pad, antagonist suppression для одновременного UP+DOWN.
- [x] **`frontend.cpp`** (2026-05-05) — главное меню. 4× `InputAutoRepeat` (UP/DOWN/LEFT/RIGHT), все 3 источника (kb + stick + D-pad), antagonist на обеих осях, dominant-axis pick для stick-only диагоналей через `lX_raw/lY_raw`. Confirm/cancel через `input_btn_just_pressed(0/3)`.
- [x] **D-Pad как независимый источник** (2026-05-05) — архитектурный фикс gamepad layer'а: добавлены `lX_raw/lY_raw/rX_raw/rY_raw` в `GamepadState` (snapshot ДО D-Pad override'а). `input_stick_*` API теперь читает raw, D-Pad через `rgbButtons[11..14]`. Исправлен баг "стик + D-pad opposite directions: второй overrid'ит первый, antagonist не работает". См. [`migration_checklist.md`](migration_checklist.md) пункт 3 для актуальной guidance.
- [x] **`vehicle.cpp` siren toggle** (2026-05-05) — sticky press_pending для гейпада добавлен в input_frame. `apply_button_input_car` ставит VEH_SIREN только если `input_*_press_pending` true (siren-кнопки на kb и pad). Consume безусловный → drain каждый физтик во время вождения. `do_packets` else-ветка дренит pending'и каждый физтик когда не в машине → нет leak'а от Triangle (menu cancel) / SPACE (jump). Чинит multi-toggle на любых Hz **И** lost-press на низких physics Hz (1 Hz debug). Drain rule зафиксирован в [input_frame.h](../../new_game/src/engine/input/input_frame.h).

Остаётся:
- [ ] **`check_debug_timing_keys`** — debug-клавиши с собственным static prev-state. Самая простая миграция — просто `input_key_just_pressed`.
- [ ] **`game_tick.cpp` weapon switch (#20)** — **сначала эмпирически проверить** что баг актуален. `Player->Pressed` уже edge-detect, теоретически OK. Если воспроизводится — мигрировать.
- [ ] **Прочие места** (по обнаружении).

Каждая миграция — **отдельный коммит** с тестированием. Если что-то ломается — коммит откатывается без затрагивания инфраструктуры.

## Resolved questions (бывшие open)

- ✅ **Stick threshold** — эмпирически использовали `STICK_DIR_PRESS_RAW = 4096` raw distance from center (= оригинальный gamemenu `GM_NOISE_TOLERANCE`), `STICK_DIR_RELEASE_RAW = 2048` (hysteresis). Подтверждено тестированием на gamemenu — ощущается идентично оригиналу.
- ✅ **Auto-repeat константы** — `400ms initial / 150ms repeat` оставлены (из gamemenu). Подтверждено что ок на gamemenu. Если будут жалобы при миграции frontend — пересмотреть.
- ✅ **Sticky pending для геймпад-кнопок** — НЕ реализован (бесполезен): геймпад polling-based, fast press между poll'ами невидим в принципе. API-symmetry `input_btn_consume` оставлен как no-op. Для siren toggle (low Hz physics) sticky pending не поможет — `gamepad_consume_until_released` в gamepad layer'е уже частично закрывает; полное решение через event-queue выходит за scope.
- ✅ **SDL event handler location** — `keyboard_key_down/up` в `engine/input/keyboard.cpp`. Геймпад через `gamepad_poll()` в `gamepad.cpp` (polling, не events для button state).

## Текущие open questions

- **Weapon switch (#20)** — actual reproducibility unverified. Сначала проверить что баг ещё на месте.

## Связь с FPS unlock

Этот модуль закрывает архитектурную часть нескольких open issue'ев:

- **#5** (siren toggle) — когда мигрируем `vehicle.cpp` (Phase 4 шаг 1), архитектурно решается: edge-detect / sticky pending не зависит от physics Hz.
- **#15** (input latency) — частично закрывается через sticky pending: physics tick не теряет edge'и которые произошли между его сэмплами. Не решает continuous-state latency, но это и не входит в scope (см. выше).
- **#20** (weapon switch) — после Phase 1+2+3 готова инфраструктура; миграция в Phase 4 шаг 5 закрывает любой остаточный bug если он есть.

Все три issue остаются open в `fps_unlock_issues.md` пока соответствующие миграции не завершены и не подтверждены пользователем. После — переезжают в resolved с ссылкой на input_system.

## Что писать в changelog

Каждый коммит / итерация → запись в [changelog.md](changelog.md) с датой, фазой и описанием. Чтобы при возврате в задачу через сколько-то времени или из нового агентского контекста было видно где остановились.
