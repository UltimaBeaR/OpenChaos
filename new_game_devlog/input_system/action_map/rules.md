# Правила именования action-констант

Эти правила применяются к каждой константе из `act_*.h` и к именам констант
устройств в `input_codes.h`. Каждый из этих файлов в шапке ссылается на этот
документ.

## 1. Префикс пакета

- `ACT_` — семантические action-константы (значение действия в контексте: «прыжок
  на ногах», «открыть pause»). Префикс выбран потому что `ACTION_` занят AI
  (`mav_action.h::MAV_Action`), а `INPUT_*` пересекается с `INPUT_MASK_*` из
  geyплейного слоя.
- Константы устройств в `input_codes.h` имеют префикс по типу устройства (см.
  раздел 3).

## 2. Структура имени action-константы

```
ACT_<CONTEXT>_<SEMANTIC_NAME>_<TYPE_SUFFIX>
```

| Часть | Что | Пример |
|------|-----|--------|
| `ACT_` | Маркер пакета | `ACT_` |
| `<CONTEXT>` | Игровой контекст где константа применяется | `FOOT`, `CAR`, `MENU`, `CINE`, `BANG`, `CONS` |
| `<SEMANTIC_NAME>` | Что делает (свободный формат через `_`) | `JUMP`, `OPEN_PAUSE`, `CAMERA_YAW`, `STEP_NEXT_FRAME` |
| `<TYPE_SUFFIX>` | Тип устройства/входа (см. раздел 3) | `_KKEY`, `_GBTN`, `_MAXIS`, ... |

Примеры:
- `ACT_FOOT_JUMP_KKEY = KKEY_SPACE`
- `ACT_FOOT_JUMP_GBTN = GBTN_SOUTH`
- `ACT_MENU_CONFIRM_KKEY = KKEY_ENTER`
- `ACT_MENU_CONFIRM_GBTN = GBTN_SOUTH`
- `ACT_FOOT_CAMERA_YAW_MAXIS = MAXIS_X`
- `ACT_FOOT_CAMERA_YAW_GAXIS = GAXIS_RIGHT_X`

**Порядок частей не меняется** — `CONTEXT` идёт до `SEMANTIC_NAME` чтобы поиск
по началу (`ACT_FOOT_`) выдавал все константы одного контекста, а поиск по
куску семантики (`_CAMERA_YAW_`) — все типы устройств одного действия.

## 3. Суффиксы типа устройства

Первая буква — устройство (K/M/G/D), остальное — разновидность.

| Суффикс | Значение | Откуда читается |
|---------|----------|-----------------|
| `_KKEY` | Keyboard key (scancode) | `input_key_held/just_pressed/...(KKEY_*)` |
| `_MBTN` | Mouse button | `mouse_capture_on_button(MBTN_*)` |
| `_MAXIS` | Mouse motion axis | `input_mouse_consume_rel(&dx, &dy)` (MAXIS_X = dx, MAXIS_Y = dy) |
| `_GBTN` | Gamepad button (общая для Xbox/DS, индексы rgbButtons[0..16]) | `input_btn_held/just_pressed(GBTN_*)` |
| `_GDIR` | Gamepad stick discrete direction (для меню) | `input_stick_held(GDIR_*)` |
| `_GAXIS` | Gamepad stick analog axis (-1..1) | `input_stick_x(GAXIS_*) / input_stick_y(GAXIS_*)` |
| `_GTRIG` | Gamepad trigger analog (0..1) | `input_trigger(GTRIG_*)` |
| `_DBTN` | DualSense unique button (touchpad click, mute, rgbButtons[17..18]) | `input_btn_held(DBTN_*)` |

Если для одного и того же действия можно дать привязку к нескольким устройствам
(например клавиша + кнопка геймпада) — заводи **отдельные константы рядом**,
каждая со своим суффиксом. Никакой композиции в одной константе не делаем.

## 4. Файлы по контекстам

| Файл | Префикс контекста в именах | Что внутри |
|------|---------------------------|-----------|
| `act_menu.h` | `ACT_MENU_*` | Frontend + pause/won/lost menus (навигация, confirm/cancel, переключение пунктов) |
| `act_foot.h` | `ACT_FOOT_*` | Геймплей на ногах (движение, атаки, прыжок, камера, ESC-открыть-пауза, F9-открыть-консоль) |
| `act_car.h` | `ACT_CAR_*` | Геймплей в машине (акселерация, поворот, сирена, выход, ESC, F9) |
| `act_cinematic.h` | `ACT_CINE_*` | Outro / playcuts / video player (skip-only) |
| `act_bangunsnotgames.h` | `ACT_BANG_*` | Отладочные клавиши режима после ввода `bangunsnotgames` (F1/F2/F8/F10/F11/Ctrl+*) |
| `act_dev_console.h` | `ACT_CONS_*` | Ввод текста внутри открытой dev-консоли (Enter подтвердить команду, Backspace стереть, ESC закрыть) |

Все файлы лежат в `new_game/src/game/action_map/`.

## 5. Правило переиспользования

> **При любых сомнениях — заводи разные константы.** Сложнее объединить лишнее
> на ревью, чем разломать чужую логику когда меняешь одну константу.

Можно объединять одну константу для нескольких мест использования **только**
если:
- Это **точно** одна кнопка одного действия в одной семантике (например ESC во
  всех ветках if внутри одного экрана меню — это одна и та же «отмена»).
- Контекст один и тот же (не переиспользуем `ACT_FOOT_*` внутри `ACT_CAR_*` и
  наоборот, даже если кнопка совпадает).

Кросс-контекстное переиспользование (одна кнопка для одного и того же действия
на ногах и в машине) **нельзя**: даже если сейчас совпадает, через 2 ревизии
может разойтись. Пиши `ACT_FOOT_OPEN_PAUSE_KKEY` и `ACT_CAR_OPEN_PAUSE_KKEY`
отдельно, рядом в файлах.

## 6. Комментарии над константами

**Обязательно** для каждой константы — короткий коммент над ней с указанием:
- Где она читается (если из нескольких мест — перечислить все либо сделать ссылку
  на основную функцию).
- Зачем нужна (если из имени не очевидно).

```cpp
// Прыжок на ногах. Читается в input_actions.cpp::apply_button_input
// → INPUT_MASK_JUMP, также в siren toggle (game_tick.cpp).
constexpr int ACT_FOOT_JUMP_KKEY = KKEY_SPACE;
constexpr int ACT_FOOT_JUMP_GBTN = GBTN_SOUTH;
```

Если в течение работы константа добавляется в новое место — дописывать в коммент
(не плодить пометки в самих местах вызова).

## 7. Имена констант устройств в `input_codes.h`

### KKEY_* (keyboard scancodes)
Имена по физическому ключу: `KKEY_SPACE`, `KKEY_ENTER`, `KKEY_W`, `KKEY_F1`,
`KKEY_LSHIFT`, `KKEY_UP/DOWN/LEFT/RIGHT`, `KKEY_NUM_0`..`KKEY_NUM_9` (numpad),
`KKEY_0`..`KKEY_9` (top row), и т.д. Значения = DirectInput scancodes (как в
старом `keyboard.h`).

### MBTN_* (mouse buttons)
`MBTN_LEFT = 1`, `MBTN_MIDDLE = 2`, `MBTN_RIGHT = 3` (SDL3 convention).

### MAXIS_* (mouse motion)
`MAXIS_X` (горизонтальная относительная ось), `MAXIS_Y` (вертикальная). Значения
условные (0 и 1) — это идентификаторы для семантики, не передаются в API
(API сейчас одноразовое — `input_mouse_consume_rel(&dx, &dy)`).

### GBTN_* (gamepad buttons, общие для Xbox/DS, rgbButtons[0..16])
Имена позиционные по SDL convention (SOUTH/EAST/WEST/NORTH), **в комменте**
указано как кнопка называется на DualSense и Xbox:

```cpp
// DS: Cross, Xbox: A
constexpr int GBTN_SOUTH = 0;
// DS: Circle, Xbox: B
constexpr int GBTN_EAST = 1;
// DS: Square, Xbox: X
constexpr int GBTN_WEST = 2;
// DS: Triangle, Xbox: Y
constexpr int GBTN_NORTH = 3;
// DS: Create/Share, Xbox: Back/View
constexpr int GBTN_SELECT = 4;
// DS: PS, Xbox: Guide
constexpr int GBTN_GUIDE = 5;
// DS: Options, Xbox: Start/Menu
constexpr int GBTN_START = 6;
// Нажатие левого стика. DS: L3, Xbox: LSB
constexpr int GBTN_L3 = 7;
// Нажатие правого стика. DS: R3, Xbox: RSB
constexpr int GBTN_R3 = 8;
// DS: L1, Xbox: LB
constexpr int GBTN_L1 = 9;
// DS: R1, Xbox: RB
constexpr int GBTN_R1 = 10;
constexpr int GBTN_DPAD_UP    = 11;
constexpr int GBTN_DPAD_DOWN  = 12;
constexpr int GBTN_DPAD_LEFT  = 13;
constexpr int GBTN_DPAD_RIGHT = 14;
// L2 как кнопка (цифровой порог). DS: L2, Xbox: LT.
// Для аналогового значения — GTRIG_L2.
constexpr int GBTN_L2_BTN = 15;
// R2 как кнопка. DS: R2, Xbox: RT.
// Для аналогового значения — GTRIG_R2.
constexpr int GBTN_R2_BTN = 16;
```

### DBTN_* (DualSense unique buttons)
```cpp
// DualSense touchpad click. На Xbox нет аналога.
constexpr int DBTN_TOUCHPAD = 17;
// DualSense mute button. На Xbox нет аналога.
constexpr int DBTN_MUTE     = 18;
```

### GDIR_* (gamepad stick discrete directions)
Виртуальные направления для меню-нав. Имена: `<stick>_<dir>`.
- `GDIR_LEFT_UP`, `GDIR_LEFT_DOWN`, `GDIR_LEFT_LEFT`, `GDIR_LEFT_RIGHT`
- `GDIR_RIGHT_UP`, `GDIR_RIGHT_DOWN`, `GDIR_RIGHT_LEFT`, `GDIR_RIGHT_RIGHT`

Значения — пара `(InputStickId, InputStickDir)` из `input_frame.h`. Реализация
константы — либо `struct` либо пара int, обсуждается в шаге 3a.

### GAXIS_* (gamepad stick analog)
- `GAXIS_LEFT_X`, `GAXIS_LEFT_Y`
- `GAXIS_RIGHT_X`, `GAXIS_RIGHT_Y`

### GTRIG_* (gamepad triggers analog)
- `GTRIG_L2`, `GTRIG_R2`

## 8. Покрытие — 100%

После полного прохода 3c + finishing-pass все обращения к `input_key_*` /
`input_btn_*` / `input_stick_*` / `input_trigger*` в `new_game/src/` идут
через `ACT_*` константы. Оставшиеся 4 совпадения по grep — комментарии
(описание API в `input_frame.h`, пояснения в `gamemenu.cpp` / `input_actions.cpp`),
не код.

Файлы action-map:
- `input_codes.h` — device-level: `KKEY_*`, `MBTN_*`, `MAXIS_*`, `GBTN_*`, `DBTN_*`, `GTRIG_*`
- `act_menu.h` — frontend / pause / won/lost / attract / input-debug-panel nav / modal-acknowledge
- `act_foot.h` — gameplay на ногах
- `act_car.h` — в машине
- `act_cinematic.h` — outro / playcuts / video skip / generic skip / replay-exit
- `act_bangunsnotgames.h` — runtime debug-keys (после `bangunsnotgames` cheat)
- `act_dev_console.h` — text-input в открытой консоли
- `act_dev_perf.h` — compile-time-gated perf-diag (OC_DEBUG_PERF, OC_DEBUG_PHYSICS_TIMING)
- `act_modifiers.h` — modifier keys (LAlt/RAlt/LCtrl/LShift/RShift → ShiftFlag/AltFlag/ControlFlag)

## 9. Как добавлять новую константу

1. Определи контекст → выбери файл `act_<context>.h`.
2. Дай уникальное `<SEMANTIC_NAME>` (поиск в файле что такого нет).
3. Заведи отдельные константы под каждое устройство которое читается (`_KKEY`,
   `_GBTN` и т.д.).
4. Над константой коммент: где используется + зачем.
5. Замени сырое чтение (`input_key_held(KKEY_SPACE)`) на чтение через новую
   константу (`input_key_held(ACT_FOOT_JUMP_KKEY)`).
6. Если значение устройства новое (например задействована кнопка которой ещё
   нет в `input_codes.h`) — сначала добавь её в `input_codes.h`, потом ссылайся.
