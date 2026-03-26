# Девлог Этапа 5.1 — Поддержка геймпадов (Xbox + DualSense)

## Шаг A0 — Вендоринг Dualsense-Multiplatform (2026-03-26)

**Модель:** Opus (1M контекст)

Библиотека склонирована в `new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/`.
Удалена только `.git/` — остальное (Tests/, .github/, clang-format и т.д.) игнорируется через `.gitignore`
на уровне `dualsense-multiplatform/`. Git трекает только Source/, Libs/, CMakeLists.txt, LICENSE.

Созданы наши файлы: README.md (инструкция обновления), VENDORED.md (коммит-хеш b8a76ff, дата 2026-02-16).
Добавлена запись в THIRD_PARTY_LICENSES.md (MIT).
Создан `new_game/DEVELOPMENT.md` — вынесены code formatting и vendored libraries из SETUP.md.

## Шаг A1 — Миграция SDL2 → SDL3 (2026-03-26)

**Модель:** Opus (1M контекст)

### Изменения

- `vcpkg.json`: `sdl2` → `sdl3`
- `CMakeLists.txt`: `find_package(SDL3)`, `SDL3::SDL3`, DLL copy обновлён
- `mfx.cpp`: `SDL_FreeWAV()` → `SDL_free()` (SDL3 API change)

### Проблема: `/Zp1` несовместим с SDL3 хедерами

Проект компилируется с `/Zp1` (1-byte struct packing) для совместимости с бинарными форматами ресурсов.
SDL3 (в отличие от SDL2) содержит `static_assert(sizeof(SDL_alignment_test) == 2 * sizeof(void*))`,
который падает при `/Zp1`.

**Попытки решения:**
1. `#pragma pack(push, 8)` вокруг `#include <SDL3/...>` — **не работает**. В clang-cl `/Zp1` реализован
   как `-fpack-struct=1` — это жёсткий потолок, `#pragma pack` его не пробивает.
2. Per-file `/Zp8` на mfx.cpp — **крэш**: mfx.cpp использует игровые packed-структуры (MFX_Sample и др.),
   которые при `/Zp8` получают неправильный layout.

**Решение — SDL3 bridge:**
Один файл `engine/platform/sdl3_bridge.cpp` — **единственная** единица трансляции включающая SDL3 хедеры.
Компилируется с `/Zp8` (один `set_source_files_properties` в CMakeLists). Весь остальной код обращается
к SDL3 исключительно через функции в `sdl3_bridge.h`.

### SDL3 debug DLL naming

SDL3 не добавляет суффикс `d` к debug DLL (в отличие от SDL2: `SDL2d.dll`).
Debug DLL называется просто `SDL3.dll`. Поправлено в CMakeLists copy commands.

### Проверка

Release и Debug собираются. Звук проверен: фоновая музыка, SFX, голоса — всё работает.

### Обновлены ссылки SDL2 → SDL3

Makefile, README.md, SETUP.md, legal/attribution.md, tools/st_4_2_dep_graph.py, THIRD_PARTY_LICENSES.md.
Исторические логи (stage3_log.md и др.) не менялись — это зафиксированная история.

## Шаг A2 — Создание абстракции gamepad (2026-03-26)

**Модель:** Opus (1M контекст)

### Новые файлы

- `engine/input/gamepad.h` — `GamepadState` (совместимый с DIJOYSTATE: lX/lY 0..65535, rgbButtons[32]),
  `InputDeviceType` enum (KEYBOARD_MOUSE / XBOX / DUALSENSE), API (init/shutdown/poll/rumble)
- `engine/input/gamepad.cpp` — SDL3 backend через sdl3_bridge, DualSense детекция по vendor/product ID,
  hotplug обработка, трансляция SDL3 осей в DI-диапазон (+32768), триггеры как digital buttons [15]/[16]
- `engine/input/gamepad_globals.h/cpp` — `gamepad_state`, `active_input_device`
- `engine/platform/sdl3_bridge.h/cpp` — расширен секцией Gamepad (init, poll events, open/close,
  get state, rumble, vendor/product ID)

### Архитектура

```
Игровой код → the_state (DIJOYSTATE)     ← joystick.cpp (DirectInput) — ТЕКУЩЕЕ
Игровой код → the_state (GamepadState)   ← joystick.cpp → gamepad → sdl3_bridge — ЦЕЛЕВОЕ (шаг A3)
```

Ещё не подключено к игре — компилируется рядом. Подключение на шаге A3.

## Шаг A3 — Замена DirectInput на gamepad layer (2026-03-26)

**Модель:** Opus (1M контекст)

### Изменения

**Полностью переписаны:**
- `joystick.cpp` — весь DirectInput код (COM init, enum, acquire, poll) заменён на вызовы
  `gamepad_init()` / `gamepad_poll()`. Файл стал тонкой обёрткой (~20 строк).
- `joystick_globals.h` — `DIJOYSTATE the_state` → `#define the_state gamepad_state`.
  Убраны `IDirectInput*`, `IDirectInputDevice*`, `OS_joy_*_range` переменные.
- `joystick_globals.cpp` — убраны все DI глобалы, файл пустой (kept for uc_orig traceability).
- `outro_os.cpp` — `OS_joy_poll()` переписан: читает из `gamepad_state` вместо DI.

**Убраны extern DIJOYSTATE:**
- `game.cpp`, `input_actions.cpp` (2 места), `pause.cpp` — заменены на include хедеров.

**Убран `#include <dinput.h>`:**
- `uc_common.h`, `joystick_globals.h`, `joystick.cpp`, `outro_os_globals.h`, `outro_os.cpp`.

**CMakeLists.txt:**
- Убран `dinput8` из link libraries. `dxguid` оставлен — нужен для DirectDraw/D3D IID.

### Проблема: types после удаления dinput.h

`joystick.h` использует `BOOL`, `UBYTE` — раньше приходили транзитивно через `dinput.h` → `windows.h`.
После удаления добавлены прямые `#include <windows.h>` и `#include "engine/core/types.h"`.

### Проверка

Release и Debug собираются. Проверено в игре:
- **Клавиатура** — работает как раньше.
- **Xbox контроллер** — работает! Подключение/отключение на лету — моментальное переключение.
- **Стик** — отклонение распознаётся, но нет плавного аналогового управления:
  Дарси сразу бежит при любом отклонении за деадзону.

### Найдена проблема: аналоговый режим стика

В `input_actions.cpp` стик обрабатывается двумя путями одновременно:
1. **Цифровые флаги** (строки 3183-3198): если стик за деадзоной → `INPUT_MASK_FORWARDS/RIGHT/...` → полная скорость бега.
2. **Аналоговые биты** (строки 3208-3211): значения стика пакуются в биты 18-31, но цифровые флаги перебивают.

На PS1 аналоговый стик напрямую управлял скоростью (маленькое отклонение = шаг, полное = бег).
PC-код ставит цифровые флаги от стика всегда → Дарси всегда бежит.
**Исправление → шаг A4** (вместе с маппингом кнопок).

## Шаг A4 — Маппинг кнопок PS1 Config 0 + аналоговый режим (2026-03-26)

**Модель:** Opus (1M контекст)

### Маппинг кнопок

Проверено по KB (`psx_controls.md`) и подтверждено пользователем на эмуляторе PS1:

| SDL3 index | Физическая | PS1 Config 0 | JOYPAD_BUTTON_* |
|---|---|---|---|
| 0 (SOUTH) | A / Cross | JUMP | JUMP |
| 1 (EAST) | B / Circle | ACTION | ACTION |
| 2 (WEST) | X / Square | PUNCH | PUNCH |
| 3 (NORTH) | Y / Triangle | KICK | KICK |
| 4 (BACK) | Back / Select | SELECT | SELECT |
| 6 (START) | Start | START | START |
| 9 (L_SHOULDER) | LB / L1 | CAMERA + 1STPERSON | CAMERA, 1STPERSON (обе на одну кнопку, как PS1) |
| 10 (R_SHOULDER) | RB / R1 | STEP_RIGHT | MOVE |
| 15 (LT digital) | LT / L2 | CAM_LEFT | CAM_LEFT |
| 16 (RT digital) | RT / R2 | CAM_RIGHT | CAM_RIGHT |

**Проблема config.ini:** старые значения DirectInput маппинга в config.ini перебивали дефолты через
`ENV_get_value_number()`. Почищены все три config.ini (Release, Debug, original_game_resources).

### Аналоговый режим — итеративная отладка

**Цель:** плавное управление скоростью стиком (малое отклонение = шаг, полное = бег), как на PS1.

**Итерация 1 — analogue=1 глобально:**
- Включил `analogue = 1` по дефолту.
- Убрал цифровые флаги от стика при analogue=1 (только аналоговые биты 18-31).
- **Проблема:** сломало клавиатуру. `get_hardware_input` при подключенном геймпаде возвращался
  из joystick-секции (ранний return), клавиатурная секция не доходила.

**Итерация 2 — убран ранний return из joystick-секции:**
- Joystick и keyboard секции теперь обе выполняются, input OR'ится.
- Keyboard секция сбрасывает `analogue = 0` при нажатии стрелок.
- **Проблема:** D-Pad не работал (не задавал оси), стик в меню не работал (нет цифровых флагов).

**Итерация 3 — D-Pad задаёт оси + цифровые флаги всегда:**
- В `gamepad.cpp`: D-Pad кнопки (11-14) теперь override'ят `lX/lY` (0 или 65535).
- В `get_hardware_input`: цифровые флаги ставятся всегда от осей (для меню), аналоговые биты тоже.
- `dpad_active` флаг в GamepadState — D-Pad → analogue=0, стик → analogue=1.
- **Результат:** стик и D-Pad работают, плавная скорость стиком, D-Pad = полная скорость.

**Итерация 4 — баг прыжка при analogue=1:**
- При беге вперёд на стике и прыжке — Дарси сразу падала вниз.
- **Причина:** `continue_moveing()` в `person.cpp` вызывается каждый кадр во время
  `SUB_STATE_RUNNING_JUMP_FLY`. При `analogue=1` проверяла деадзону стика (`QDIST2 < 8`)
  **до** проверки `STATE_JUMPING`. Если стик чуть отклонился от полного — return 0 →
  `set_person_drop_down()` → падение.
- **Первый фикс (неправильный):** переставил `if (STATE_JUMPING) return 1` перед деадзоной.
  Прыжок работал, но Дарси продолжала лететь даже после отпускания стика (не как на PS1).
- **Откат:** вернул оригинальный порядок. Баг с падением вернулся.
- **Настоящая причина:** `llabs(GET_JOYY(input))` — `GET_JOYY` возвращает **unsigned** (т.к. `input` = `ULONG`),
  clang-cl оптимизировал `llabs(unsigned)` как no-op (unsigned не бывает отрицательным, warning подтверждал).
  Результат: `dy = -126` вместо `126` → `QDIST2(24, -126) = 24 + (-63) = -39` → `(-39) < 8` → return 0 → падение.
- **Фикс:** `llabs(GET_JOYX(input))` → `abs((SLONG)GET_JOYX(input))` — приведение к signed перед abs.
  Warning пропал (80 → 79). Прыжок работает: стик вперёд = летит, отпустить = падает (как PS1).

**Итерация 5 — баг поворота клавиатурой (лево/право):**
- Стрелки лево/право на клаве не поворачивали Дарси, только дёргали анимацию.
- **Причина:** joystick-секция пакует аналоговые биты (18-31) всегда когда геймпад подключен.
  Даже при стике по центру `(32768 >> 9) = 64` → ненулевые верхние биты.
  `player_turn_left_right()` видит ненулевые верхние биты → считает input аналоговым →
  `wTurn = (wJoyX * wMaxTurn) >> 7` → но wJoyX=0 (центр) → поворот = 0.
- **Фикс:** keyboard секция теперь очищает биты 18-31 (`input &= 0x0003FFFF`) при нажатии стрелок.

### PS1 аналоговый режим — уточнение по KB vs реальность

KB (`psx_controls.md`) утверждает: "Аналог только эмулирует D-Pad (нет плавного движения)".
Пользователь проверил на эмуляторе PS1 — плавное движение **есть** (шаг при малом отклонении,
бег при полном). Вероятно финал PS1 был доработан после заморозки пре-релизных исходников.

В PC-коде инфраструктура для аналога уже была: `process_analogue_movement()`, `player_turn_left_right_analogue()`,
биты 18-31 в input word, `GET_JOYX/GET_JOYY`. Формула скорости `(velocity * 60) >> 8` — оригинальная PC,
**не менялась**. На Dreamcast использовалась квадратичная кривая (iXaxis *= iXaxis), на PC — линейная.

На PS1 с аналоговым режимом D-Pad отключался полностью (подтверждено пользователем).
У нас пока работают оба: стик → аналоговый режим, D-Pad → цифровой. Решение отложено.

### Менюшки — начатые фиксы (WIP)

- **Пауз-меню:** геймпад не работал — `rgbButtons` проверялись только [0..7],
  Start не ставил PAUSED_KEY_START. Расширен диапазон до [0..31], Start (index 6) отдельно.
- **Главное меню:** Circle и Cross обе подтверждали — `any_button` собирал все кнопки [0..7].
  Изменено: Cross (index 0) = подтверждение, Circle/B → INPUT_MASK_CANCEL (отмена).
- **Быстрая прокрутка стиком в меню** — пока не исправлено (нет debounce).
- **Остальные менюшки** — системный проход по всем меню запланирован.
- **Стрелки влево/вправо на клаве в игре** — фикс (очистка бит 18-31) написан, НЕ ПРОВЕРЕН.
- **D-Pad в меню** — пока работает в главном меню, не проверен в паузе и других.

**TODO следующей итерации — системный проход по менюшкам и контроллеру:**

Эталон: PS1 в аналоговом режиме. Клавиатура должна работать параллельно с геймпадом.

1. **Главное меню** (`ui/frontend/frontend.cpp`):
   - Стик листает пункты слишком быстро (нет debounce/repeat delay) — нужен repeat как на PS1
   - Circle (B) должен работать как "назад/отмена" (INPUT_MASK_CANCEL) — частично сделано, проверить
   - Cross (A) = подтверждение — частично сделано (only rgbButtons[0]), проверить
   - D-Pad в меню — проверить работает ли вверх/вниз/лево/право
   - Кнопки проверялись только [0..7] → расширить до [0..31] если нужно

2. **Пауз-меню** (`ui/menus/pause.cpp`):
   - Start (index 6) → PAUSED_KEY_START — сделано, проверить
   - Кнопки расширены до [0..31] — сделано, проверить
   - Стик/D-Pad навигация — проверить
   - Circle = отмена (снять паузу) — проверить

3. **Инвентарь** — найти код, проверить какие кнопки используются, привести к PS1 поведению

4. **Game over / mission complete** — найти код, проверить

5. **Briefing / mission select** — найти код, проверить

6. **Widget/Form GUI** (`ui/menus/widget.cpp`) — базовый GUI фреймворк, используется многими менюшками.
   Строка 1068: `get_hardware_input(INPUT_TYPE_JOY)` — проверить что кнопки корректны

7. **Outro** (`outro/core/outro_os.cpp`) — уже переписан OS_joy_poll через gamepad_state, проверить

8. **Стрелки влево/вправо на клаве в игре** — фикс написан (очистка бит 18-31), ПРОВЕРЕН, работает.

**Принцип:** Cross = подтверждение, Circle = отмена/назад, Start = пауза, стик/D-Pad = навигация.
Для каждого меню: найти где читаются кнопки (`rgbButtons`, `the_state`, `any_button`),
привести к новым индексам (0=Cross, 1=Circle, 6=Start, ...), добавить repeat delay для стика.

### SDL3 event handling fix

`sdl3_gamepad_poll_event()` использовал `SDL_PollEvent()` который **жрал ВСЕ** SDL события,
не только геймпадные. Заменён на `SDL_PumpEvents()` + `SDL_PeepEvents()` с фильтром
`SDL_EVENT_GAMEPAD_ADDED..SDL_EVENT_GAMEPAD_REMOVED`. Не-геймпадные события больше не теряются.
