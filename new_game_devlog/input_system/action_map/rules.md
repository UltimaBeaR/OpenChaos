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
| `_GDIR` | Gamepad stick discrete direction (для меню) | `input_stick_held(GAXIS_*, GDIR_*)` |
| `_GAXIS` | Gamepad stick analog axis (-1..1) | `input_stick_x_axis(GAXIS_*) / input_stick_y_axis(GAXIS_*)` |
| `_GTRIG` | Gamepad trigger analog (0..1) | `input_trigger(GTRIG_*)` |
| `_DBTN` | DualSense unique button (touchpad click, mute, rgbButtons[17..18]) | `input_btn_held(DBTN_*)` |
| `_VAXIS` | **Виртуальная** ось «намерения движения» — НЕ железо. Один аналоговый вектор, в который пакуется И стик, И WASD (биты 18-31 игрового слова `input`). Читается на точках РЕАКЦИИ (поворот/руль/grapple). | `input_virtual_axis(input, VAXIS_*)` |

**Первая буква суффикса = ИСТОЧНИК однозначно.** `K`=клавиатура, `M`=мышь, `G`=геймпад, `D`=DualSense-эксклюзив, `V`=виртуальный (постмультиплексный) источник. Если появляется новый отдельный вид источника (что-то виртуальное, на что мапятся несколько физических устройств) — заводи новую первую букву и описывай источник в `input_codes.h`.

Если для одного и того же действия можно дать привязку к нескольким устройствам
(например клавиша + кнопка геймпада) — заводи **отдельные константы рядом**,
каждая со своим суффиксом. Никакой композиции в одной константе не делаем.

**Тип-суффикс всегда последний.** Любой уточнитель (номер варианта, `ALT`,
`L`/`R` и т.п.) — часть `<SEMANTIC_NAME>` и идёт ДО типа, не после. Когда на
одно действие несколько привязок одного типа (Enter / Space / NumpadEnter на
«подтвердить») — нумеруй в семантике перед суффиксом:

```
ACT_MENU_CONFIRM_1_KKEY   // правильно
ACT_MENU_CONFIRM_KKEY_1   // НЕЛЬЗЯ — цифра после типа
ACT_MENU_NAV_UP_ALT_KKEY  // правильно (ALT в семантике)
ACT_MENU_NAV_UP_KKEY_ALT  // НЕЛЬЗЯ — квалификатор после типа
```

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

### VAXIS_* (виртуальная ось намерения движения)
- `VAXIS_X`, `VAXIS_Y` — горизонтальная / вертикальная компонента единого
  вектора «намерения движения», упакованного в биты 18-31 игрового слова
  `input`. Источник device-agnostic (стик ИЛИ WASD). Читается
  `input_virtual_axis(input, VAXIS_*)` (см. `input_actions.h`).

### Прочие источники-границы
- `GBTN_COMMON_COUNT` — количество общих для всех геймпадов кнопок (0..16);
  граница для wildcard-сканов «любая кнопка» (video-skip).

## 8. Принципы покрытия

1. **Смысл — в точке РЕАКЦИИ, не считывания.** ACT-константа называется по
   тому, что ввод ДЕЛАЕТ в игре («поворот на ногах», «руль машины»), а не
   «считывание X стика». Даже если железо считано раньше в другом месте —
   именуем и ссылаемся там, где игра реагирует. Чтобы при смене кнопки сразу
   видеть все затронутые места и ловить конфликты (одна клавиша с разными
   смыслами, срабатывающими ОДНОВРЕМЕННО в одном режиме).
2. **Весь ввод — через реестр, без исключений.** Ни одного чтения
   клавиши/кнопки/мыши/стика/модификатора в обход `ACT_*`. Включая мёртвый код
   (его удаляем) — кроме намеренно сохранённого с явным комментом «почему».
3. **Никаких неиспользуемых констант.** Константа-тег, которую невозможно
   подставить в вызов (паромная мышь `input_mouse_consume_rel`, чтение
   глобального флага `if (ShiftFlag)`), НЕ заводится — вместо неё роль
   документируется комментом (мышь — в `act_foot.h`; модификаторы — в
   `act_modifiers.h`).
4. **Аналог и виртуальные источники — тоже в реестре.** Стик-оси заворачиваются
   в `_GAXIS`-константы (передаются аргументом в `input_stick_*`); виртуальная
   ось движения — в `_VAXIS` (аргумент `input_virtual_axis`).

**Что законно читает сырьё (НЕ нарушение, не привязки):**
- **Слой абстракции/драйвер** — `gamepad.cpp` (SDL→`rgbButtons`), `input_frame.cpp`
  (сборка снапшотов), `sdl3_bridge.cpp` (SDL scancode→`KKEY_*` таблица),
  `keyboard.cpp`, `mouse_capture.cpp`. Это РЕАЛИЗАЦИЯ, через которую `ACT_*`
  и разрешаются.
- **Input-debug тестер** — `input_debug_*.cpp` показывает сырое состояние КАЖДОЙ
  клавиши/кнопки (диагностика железа, как тест-экран контроллера); привязок нет.
- **Wildcard-сканы** — «любая кнопка пропускает» (video/playback/outro): цикл по
  диапазону, не привязка к конкретной кнопке (бенд — `GBTN_COMMON_COUNT`).
- **Текстовый ввод** — консоль / поля ввода читают `input_last_key()` как «любой
  скан-код → символ»; это ввод текста, не игровая привязка.

Файлы action-map:
- `input_codes.h` — источники: `KKEY_*`, `MBTN_*`, `MAXIS_*`, `GBTN_*`, `DBTN_*`, `GTRIG_*`, `VAXIS_*`, `GBTN_COMMON_COUNT`
- `act_menu.h` — frontend / pause / won/lost / attract / input-debug-panel nav / modal-acknowledge / form-widget keys
- `act_foot.h` — gameplay на ногах (вкл. стик-оси движения/камеры/прицела, виртуальную ось движения)
- `act_car.h` — в машине (вкл. руль через `_VAXIS`)
- `act_cinematic.h` — outro / playcuts / video skip / generic skip / replay-exit (вкл. стик прицела в outro)
- `act_bangunsnotgames.h` — runtime debug-keys (после `bangunsnotgames` cheat; F1-модификатор)
- `act_dev_console.h` — text-input в открытой консоли
- `act_dev_perf.h` — compile-time-gated perf-diag (OC_DEBUG_PERF, OC_DEBUG_PHYSICS_TIMING)
- `act_modifiers.h` — modifier keys (LCtrl/LShift/RShift → ControlFlag/ShiftFlag; роли в комментах)

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
