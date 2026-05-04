# Input System — текущая работа: модуль `input_frame`

> **Статус:** в работе. Прогресс → [changelog.md](changelog.md).
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

### Auto-repeat

```cpp
// Возвращает true:
//  - на первое нажатие (just_pressed)
//  - и далее каждые ~150 ms если клавиша зажата и прошёл initial delay 400 ms.
// Параметры (initial_delay_ms, repeat_period_ms) — единые для всех менюшек,
// настраиваются константами в input_frame.cpp.
bool input_key_just_pressed_or_repeat(SLONG kb_code);
bool input_btn_just_pressed_or_repeat(SLONG gamepad_btn_idx);
```

### Stick-as-direction (виртуальные стрелки)

```cpp
// Виртуальные направления стика — для меню/UI, где нужны discrete events.
// Threshold с гистерезисом: один раз стик отклоняется > inner_threshold —
// считается "нажат" в этом направлении до возврата в < outer_threshold.
// Одновременное Up + Down → нет нажатия (cancel).

enum StickDir { STICK_UP = 0, STICK_DOWN, STICK_LEFT, STICK_RIGHT };
enum StickId  { STICK_LEFT_PAD = 0, STICK_RIGHT_PAD };

bool input_stick_held(StickId stick, StickDir dir);
bool input_stick_just_pressed(StickId stick, StickDir dir);
bool input_stick_just_released(StickId stick, StickDir dir);
bool input_stick_just_pressed_or_repeat(StickId stick, StickDir dir);
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

**Где set'ится:** в SDL event handler'е (где сейчас пишется `Keys[X] = 1`). Параллельно с записью в `Keys[]` ставится `input_key_press_pending_[X] = 1`.

**Где clear'ится:** только при `input_key_consume(X)` — никогда не сбрасывается автоматически. Это значит pending живёт до тех пор пока кто-то не обработал. Если несколько потребителей читают один и тот же ключ — первый кто consume'ит, очистит для всех остальных в **этом** frame'е (но если новое нажатие придёт после consume и до конца frame'а — sticky снова поднимется).

**Правило для потребителей:** если ты обрабатываешь pending-press → consume сразу после обработки. Если только смотришь "был ли press" без действия → не consume.

**Поведение `just_pressed`:** рассчитывается из snapshot'а и prev-snapshot'а на render frame, не из sticky-флага. То есть `just_pressed` отражает edge "видимый из render frame", `pending` — edge "случившийся хоть когда-то с прошлого consume".

Различие важно: `just_pressed` ловит edge ровно на одном frame'е (как Unity GetKeyDown), `pending` — sticky catch-up для медленных потребителей (физика которая может прийти на следующий frame).

## Implementation phases

### Phase 1 — Скелет модуля (коммит 1)

- [ ] `input_frame.h` — API
- [ ] `input_frame.cpp` — реализация с заглушками для stick/auto-repeat (остальное работает)
- [ ] `input_frame_globals.{h,cpp}` если нужны globals
- [ ] Hook в SDL event handler — найти где ставится `Keys[X] = 1` и добавить sticky set
- [ ] Hook в gamepad handler — аналогично для button events
- [ ] Вызов `input_frame_update()` в начале game loop'а (game.cpp), до `GAMEMENU_process`
- [ ] Build pass — никаких изменений в поведении, только новая инфраструктура

### Phase 2 — Stick-as-direction (коммит 2)

- [ ] Реализация виртуальных направлений с hysteresis для левого и правого стика
- [ ] Edge-detect для них
- [ ] Build + ручное тестирование что стик корректно отдаёт just_pressed_up etc

### Phase 3 — Universal auto-repeat (коммит 3)

- [ ] Реализация `just_pressed_or_repeat` для клавиш, кнопок, stick-направлений
- [ ] Единые константы `INPUT_REPEAT_INITIAL_DELAY_MS` и `INPUT_REPEAT_PERIOD_MS`

### Phase 4-N — Миграция потребителей (по одному коммиту на место)

Порядок (от наиболее болезненного к наименее):

1. **`vehicle.cpp` siren toggle** — закрывает архитектурную часть #5. Использовать `input_btn_press_pending` или `_just_pressed` для соответствующих физических кнопок.
2. **`gamemenu.cpp`** — заменить ad-hoc `gm_last_start/triangle/cross/dir` + `gm_dir_next_fire` + `kb_next_fire/kb_last_dir` на `just_pressed`/`just_pressed_or_repeat`/`stick_just_pressed_or_repeat`.
3. **`frontend.cpp`** — главное меню навигация (`dir_next_fire` → universal repeat).
4. **`check_debug_timing_keys`** — debug-клавиши.
5. **`game_tick.cpp` weapon switch (#20)** — сначала эмпирически проверить что баг ещё актуален; если да — мигрировать на новый API.
6. Прочие места (по обнаружении).

Каждая миграция — отдельный коммит, отдельное тестирование. Если что-то ломается — коммит откатывается без затрагивания инфраструктуры.

## Open questions

- **Что точно считать "одним нажатием" на стике** — порог? hysteresis ширина? Подобрать эмпирически на тестировании.
- **Auto-repeat константы** — текущие `400ms initial / 150ms repeat` (из gamemenu.cpp) — проверить что одинаково ок для всех меню после миграции.
- **Sticky pending — нужен ли он для геймпад-кнопок?** Геймпад опрашивается через DirectInput / SDL gamepad polling — не event-driven, состояние читается. Если poll cadence = render rate, то fast press между двумя poll'ами **не виден вообще**. То есть sticky для геймпада возможно бесполезен (всё равно его не словить если poll < event rate). Изучить при Phase 1 hook.
- **Где живёт SDL event handler для key down/up** — найти в Phase 1.

## Связь с FPS unlock

Этот модуль закрывает архитектурную часть нескольких open issue'ев:

- **#5** (siren toggle) — когда мигрируем `vehicle.cpp` (Phase 4 шаг 1), архитектурно решается: edge-detect / sticky pending не зависит от physics Hz.
- **#15** (input latency) — частично закрывается через sticky pending: physics tick не теряет edge'и которые произошли между его сэмплами. Не решает continuous-state latency, но это и не входит в scope (см. выше).
- **#20** (weapon switch) — после Phase 1+2+3 готова инфраструктура; миграция в Phase 4 шаг 5 закрывает любой остаточный bug если он есть.

Все три issue остаются open в `fps_unlock_issues.md` пока соответствующие миграции не завершены и не подтверждены пользователем. После — переезжают в resolved с ссылкой на input_system.

## Что писать в changelog

Каждый коммит / итерация → запись в [changelog.md](changelog.md) с датой, фазой и описанием. Чтобы при возврате в задачу через сколько-то времени или из нового агентского контекста было видно где остановились.
