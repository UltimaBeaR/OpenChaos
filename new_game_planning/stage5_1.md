# Этап 5.1 — Поддержка геймпадов (Xbox + DualSense)

**Статус: ✅ ЗАВЕРШЁН**

**Цель:** прямая поддержка Xbox контроллера и DualSense (PS5) контроллера.

Вынесено из этапа 10 — приоритетная фича, реализуется сразу после фиксов пре-релизных багов,
до перехода на OpenGL.

## Правила

- **Девлог:** после каждого завершённого шага (A0, A1, A2, ...) записывать результат в `new_game_devlog/stage5_1_log.md` — что сделано, какие проблемы встретились, как решены.

## Контекст

**Текущее состояние (после шагов A0-A4 + системный проход по менюшкам):**
- SDL3 заменил SDL2 (аудио + геймпады). DirectInput удалён.
- SDL3 хедеры изолированы в `engine/platform/sdl3_bridge.cpp` (единственный файл с `/Zp8`).
- Gamepad абстракция: `engine/input/gamepad.h/cpp` → `sdl3_bridge` → SDL3.
- Joystick legacy shim: `the_state` = `gamepad_state` (через `#define`), `ReadInputDevice()` → `gamepad_poll()`.
- Маппинг кнопок: PS1 Config 0. Аналоговый стик: плавная скорость.
- Меню: Cross=confirm, Triangle=back, Start=pause. Гистерезис на стике, ticker repeat, hotplug-safe.
- `gamepad_consume_until_released()` — предотвращает протекание кнопок из меню в геймплей.
- Dualsense-Multiplatform vendored в `libs/`, пока не подключена (итерация B).
- C++20 указан в CMake (`CMAKE_CXX_STANDARD 20`), Clang 20+ required.

**Итерации A и B — ЗАВЕРШЕНЫ. Этап 5.1 закрыт.**

**Завершено:**
- A0-A4: SDL3, gamepad абстракция, DirectInput→SDL3, маппинг PS1 Config 0, аналоговый стик
- Системный проход по всем менюшкам (кнопки, debounce стика, repeat delay)
- A5: Вибрация — PS1-style `gamepad_set_shock(fast, slow)` + tick decay
- A6: Hotplug — SDL3 events (ADDED/REMOVED), active_input_device автодетект, все экраны
- Фикс: stick drift deadzone для аналоговых бит (Xbox)
- Фикс: outro gamepad polling + Cross/A выход

## Архитектура

```
┌──────────────────────────────────────────────────┐
│  Игровой код (input_actions, frontend, pause...) │
│              ↕ (GamepadState)                    │
├──────────────────────────────────────────────────┤
│  gamepad.h/cpp — абстракция + автодетект         │
│                                                  │
│  Активный режим (переключается по last input):   │
│  ┌────────────┐ ┌────────────────────┐ ┌──────┐  │
│  │   SDL3     │ │ Dualsense-         │ │ Клав.│  │
│  │ Xbox/other │ │ Multiplatform      │ │ мышь │  │
│  │ input+out  │ │ input+output(ALL)  │ │      │  │
│  └────────────┘ └────────────────────┘ └──────┘  │
└──────────────────────────────────────────────────┘
```

- **Три режима ввода**, переключение автоматическое по последнему активному устройству:
  - **Клавиатура/мышь** — текущий код, без изменений. Мыши в игре сейчас нет, но учитываем в enum для будущего
  - **Xbox/generic gamepad** → **SDL3** (input + rumble)
  - **DualSense** → **Dualsense-Multiplatform** (ВСЁ: input + adaptive triggers + haptics + LED + touchpad + gyro)
- При переключении меняется `active_input_device` enum — в будущем определяет иконки кнопок в UI
- Один контроллер (оригинал однопользовательский)
- SDL3 всё равно инициализируется всегда (для аудио + для детекции подключения контроллеров).
  При обнаружении DualSense — SDL3 отпускает устройство, Dualsense-Multiplatform берёт на себя.

## Расположение файлов

```
new_game/src/engine/input/
  gamepad.h          — НОВЫЙ: GamepadState, API, enum InputDeviceType
  gamepad.cpp        — НОВЫЙ: абстракция, автодетект, SDL3/DS backends
  gamepad_globals.h  — НОВЫЙ: extern state
  gamepad_globals.cpp— НОВЫЙ: definitions
  joystick.h         — ПЕРЕПИСАН: legacy API shim (GetInputDevice, ReadInputDevice)
  joystick.cpp       — ПЕРЕПИСАН: реализация через gamepad layer
  joystick_globals.h — ПЕРЕПИСАН: GamepadState the_state вместо DIJOYSTATE
  joystick_globals.cpp— ПЕРЕПИСАН: убран DirectInput

new_game/libs/dualsense-multiplatform/ — vendored library
```

Старые `joystick.*` сохраняются как тонкая обёртка — чтобы не менять все вызовы
`ReadInputDevice()` и `the_state` по всей кодовой базе. Реальная логика — в `gamepad.*`.
Позже (после того как всё работает) — заменить все вызовы по кодовой базе и удалить joystick.*.

## GamepadState — совместимость с DIJOYSTATE

Ключевое решение: `GamepadState` повторяет поля `DIJOYSTATE` с тем же диапазоном значений.
SDL3 оси (-32768..+32767) транслируются в DI диапазон (0..65535) через `+ 32768`.
Это позволяет НЕ менять логику деадзоны, паковки аналога в биты и весь downstream код.

## Две итерации

### Итерация A — Базовый геймпад (воспроизведение PS1 DualShock)

Цель: контроллер работает. Маппинг кнопок = PS1 Config 0.
Вибрация с двумя моторами и затуханием как на PS1.
Полноценное аналоговое управление левым стиком (плавное движение, не D-Pad эмуляция).
В оригинале PC-код уже поддерживает аналог через биты 18-31 в input (`GET_JOYX/GET_JOYY`),
нужно лишь корректно передавать значения из SDL3.

### Итерация B — DualSense-специфика

Цель: adaptive triggers, LED (здоровье), тачпад, гироскоп, audio-to-haptic.
Делается после того как Итерация A полностью работает.

---

## Итерация A: пошаговый план

### Шаг A0 — Вендоринг Dualsense-Multiplatform

- `git clone https://github.com/rafaelvaloto/Dualsense-Multiplatform.git` внутрь `new_game/libs/dualsense-multiplatform/`
  → получаем `new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/` (папка репы)
  → удаляем `Dualsense-Multiplatform/.git/`
  → коммитим файлы как часть нашей репы
- Из клона **удаляем ТОЛЬКО** `Dualsense-Multiplatform/.git/`
- **Ничего больше не удаляем** — ненужное для сборки игнорируем через `.gitignore`
- Наши файлы живут **на уровень выше** (рядом с папкой репы, не внутри):
  - `.gitignore` — игнорирует всё в Dualsense-Multiplatform/ кроме Source/, Libs/, CMakeLists.txt, LICENSE
  - `VENDORED.md` — коммит-хеш, дата, URL, инструкция обновления
  - `README.md` — наш README с объяснением: что это, зачем, как обновлять
    (удалить папку Dualsense-Multiplatform/, git clone, удалить .git/, готово)
- LICENSE остаётся внутри Dualsense-Multiplatform/ из репы
- Добавить запись в `THIRD_PARTY_LICENSES.md`

**Полная структура:**
```
new_game/libs/
  dualsense-multiplatform/
    .gitignore                    ← НАШ: игнорируем ненужное из репы
    README.md                     ← НАШ: что это, как обновлять
    VENDORED.md                   ← НАШ: коммит-хеш, дата
    Dualsense-Multiplatform/      ← содержимое репы (без .git/)
      Source/                     ← ТРЕКАЕТСЯ
        Public/
        Private/
        CMakeLists.txt
      Libs/                       ← ТРЕКАЕТСЯ (miniaudio для audio→haptic)
        miniaudio/
      CMakeLists.txt              ← ТРЕКАЕТСЯ
      LICENSE                     ← ТРЕКАЕТСЯ
      Tests/                      ← игнорируется .gitignore
      .github/                    ← игнорируется .gitignore
      README.md                   ← игнорируется .gitignore
      ...                         ← всё остальное игнорируется
```

### Шаг A1 — Миграция SDL2 → SDL3 (ПЕРВЫМ ДЕЛОМ, до любой работы с геймпадом)

⚠️ **Этот шаг делается полностью отдельно и проверяется отдельно.**
Цель: убедиться что SDL3 работает в нашем проекте, звук не сломался.

**Файлы:**
- `new_game/vcpkg.json` — `"sdl2"` → `"sdl3"`
- `new_game/CMakeLists.txt` — `find_package(SDL2)` → `find_package(SDL3)`, линковка `SDL3::SDL3`
- `new_game/src/engine/audio/mfx.cpp` — SDL2→SDL3 API аудио:
  - `#include <SDL2/SDL_audio.h>` → `<SDL3/SDL_audio.h>`
  - `SDL_LoadWAV()` — изменилась сигнатура (возвращает bool)
  - `SDL_FreeWAV()` → `SDL_free()`

**Проверка:** `make build-release` компилируется. Запустить игру, проверить что:
- Фоновая музыка играет
- Звуковые эффекты работают (шаги, удары, взрывы)
- Голосовые файлы воспроизводятся
Только после подтверждения что звук ок — переходим к геймпаду.

### Шаг A2 — Создание абстракции gamepad

**Новые файлы:**
- `new_game/src/engine/input/gamepad.h` — GamepadState struct + API
- `new_game/src/engine/input/gamepad.cpp` — SDL3 backend + Dualsense-Multiplatform стаб
- `new_game/src/engine/input/gamepad_globals.h` — extern state
- `new_game/src/engine/input/gamepad_globals.cpp` — definitions

**GamepadState:**
```
lX, lY          — левый стик, 0..65535 (центр 32768) — как DIJOYSTATE
rgbButtons[32]  — кнопки, совместимо с DIJOYSTATE.rgbButtons
connected       — подключён ли
```

**InputDeviceType enum:**
```
INPUT_DEVICE_KEYBOARD_MOUSE  — клавиатура/мышь
INPUT_DEVICE_XBOX            — Xbox / generic gamepad (через SDL3)
INPUT_DEVICE_DUALSENSE       — DualSense (через Dualsense-Multiplatform)
```

**Логика gamepad.cpp:**
- `gamepad_init()` → `SDL_Init(SDL_INIT_GAMEPAD)` + Dualsense-Multiplatform init
- `gamepad_poll()`:
  1. Dualsense-Multiplatform проверяет наличие DualSense (по HID vendor/product ID)
  2. Если DualSense найден → читаем input через DS-lib, заполняем GamepadState
  3. Если нет DualSense → SDL3 events + `SDL_GetGamepadAxis/Button`, заполняем GamepadState
  4. Определяем `active_input_device` по последнему устройству давшему input
- `gamepad_rumble(small, big, duration)`:
  - DualSense → через DS-lib
  - Xbox → через `SDL_RumbleGamepad()`
- `gamepad_get_device_type()` → возвращает текущий InputDeviceType (для UI иконок в будущем)

**CMakeLists.txt:** добавить новые файлы в SOURCES_NEW, добавить Dualsense-Multiplatform.

**Проверка:** компилируется, ещё не подключено к игре.

### Шаг A3 — Замена DirectInput на gamepad layer

**Модифицируемые файлы:**

1. `new_game/src/engine/input/joystick.cpp` — **полная перезапись:**
   - Убрать весь DirectInput код (COM init, enum, acquire, poll)
   - `GetInputDevice(JOYSTICK,...)` → вызов `gamepad_init()`
   - `ReadInputDevice()` → вызов `gamepad_poll()`, копирование state в `the_state`

2. `new_game/src/engine/input/joystick_globals.h/cpp`:
   - `DIJOYSTATE the_state` → `GamepadState the_state`
   - Убрать IDirectInput*, IDirectInputDevice*, etc.
   - Убрать `#include <dinput.h>`

3. `new_game/src/game/input_actions.cpp`:
   - `the_state.lX/lY/rgbButtons` — работают без изменений (поля совпадают)
   - Убрать `extern DIJOYSTATE` — подключить правильный хедер

4. `new_game/src/ui/frontend/frontend.cpp`:
   - Убрать extern DIJOYSTATE, подключить хедер

5. `new_game/src/ui/menus/pause.cpp`:
   - Убрать extern DIJOYSTATE и extern ReadInputDevice, подключить хедеры

6. `new_game/src/game/game.cpp`:
   - Убрать extern DIJOYSTATE

7. `new_game/src/outro/core/outro_os.cpp` + `outro_os_globals.h`:
   - Убрать DirectInput типы
   - `OS_joy_poll()` переписать через gamepad API

8. `new_game/src/engine/platform/uc_common.h`:
   - Убрать `#include <dinput.h>`

9. `new_game/CMakeLists.txt`:
   - Убрать `dinput8` и `dxguid` из link libraries

**Проверка:** `make build-release`, контроллер подключён и работает в меню + в игре.

### Шаг A4 — Маппинг кнопок PS1 Config 0

**Файлы:**
- `new_game/src/game/input_actions.cpp` — `init_joypad_config()`:
  - Дефолтный маппинг = PS1 Config 0 (Classic), адаптированный под Xbox/DualSense layout:
    - Cross/A = JUMP
    - Circle/B = ACTION
    - Square/X = PUNCH
    - Triangle/Y = KICK
    - L1/LB = CAMERA
    - R1/RB = STEP_RIGHT
    - L2/LT = CAM_LEFT
    - R2/RT = CAM_RIGHT
    - Start = START (пауза)
    - Select/Back = SELECT (инвентарь)
  - `config.ini` [Joypad] по-прежнему оверрайдит

**Архитектура для будущих layout'ов:**
- Маппинг хранится в массиве `joypad_button_use[]` — уже hot-swappable (перезаписать = сменить layout)
- В будущем: добавить preset'ы (Classic / Modern) переключаемые из меню без перезагрузки
- Пока реализуем только Classic (PS1 Config 0), но архитектура готова к расширению

**Проверка:** все действия триггерятся правильными кнопками.

### Шаг A5 — Вибрация (PS1-style)

**Вибрация подтверждена в оригинале.** Функция `PSX_SetShock(fast, slow)` вызывается из:
- `Combat.cpp:2468` — получение урона в бою
- `Darci.cpp:513` — урон Дарси
- `Person.cpp:2085,2965,6321,6473,19288` — удары, попадания, падения
- `Vehicle.cpp:4177` — столкновения (shock)
- `fc.cpp:2171` — тряска камеры
- `engine.cpp:781` — вибрация от двигателя
- `Wadmenu.cpp:1673` — меню
- `mdec.cpp:316` — видеовставки (привязка к кадрам, MDEC_vibra[])

Эти вызовы есть только в `original_game/` (PSX код вырезан из new_game).
Нужно портировать `PSX_SetShock()` в новый код как `gamepad_rumble()`.

**Файлы:**
- `new_game/src/engine/input/gamepad.cpp` — реализация `gamepad_rumble(small, big)`
  - DualSense → через DS-lib API
  - Xbox → через `SDL_RumbleGamepad()`
- Добавить вызовы `gamepad_rumble()` в аналогичные места в new_game/:
  - Combat, Person, Vehicle, камера, двигатель
- Затухание как на PS1: малый мотор `>>=1`, большой `*7>>3` каждый тик
- `vibra_mode` (меню вкл/выкл) — уважать

**Проверка:** контроллер вибрирует при получении урона, взрывах, в машине, в видеовставках.

### Шаг A6 — Hotplug и динамическое переключение

**Требования:**
- Подключение контроллера во время игры → сразу подхватывается
- Отключение → автоматически переключается на клавиатуру
- Переключение между устройствами: тронул клаву → режим клавиатуры,
  тронул контроллер → режим контроллера. Моментально, без задержек.
- Работает на ВСЕХ экранах (меню, геймплей, пауза, outro)

**Файлы:**
- `new_game/src/engine/input/gamepad.cpp`:
  - SDL events: `SDL_EVENT_GAMEPAD_ADDED` → определить тип (DualSense/Xbox), инициализировать нужный backend
  - `SDL_EVENT_GAMEPAD_REMOVED` → disconnected, обнулить state, fallback на клаву
  - DS-lib: отдельная детекция по HID vendor/product ID
  - `active_input_device` обновляется каждый poll по последнему устройству давшему input

**Проверка:**
- Старт без контроллера → клавиатура работает
- Подключить Xbox → сразу работает
- Отключить Xbox → клавиатура снова
- Подключить DualSense → сразу работает (LED загорается, adaptive triggers если итерация B)
- Переключение клава↔контроллер mid-game

---

## Итерация B: DualSense-специфика (Dualsense-Multiplatform)

**Подробный план:** [stage5_1_dualsense.md](stage5_1_dualsense.md) — API библиотеки, архитектура, все шаги.

Библиотека: `libs/dualsense-multiplatform/` (vendored, шаг A0).
API: C++20, HID напрямую, input + output (LED, triggers, haptics, gyro, touch).
**Конфликт с SDL3:** обе открывают HID. При обнаружении DualSense → SDL3 отпускает
(`SDL_HINT_JOYSTICK_HIDAPI_PS5 = "0"`), DS-lib берёт всё на себя.

**⚠️ Учитывать `active_input_device`:** все DualSense-фичи (LED, adaptive triggers, haptics,
gyro, touchpad, audio-to-haptic) должны активироваться **только когда `active_input_device`
= `INPUT_DEVICE_DUALSENSE`**. Если игрок переключился на клавиатуру — все output-фичи
отключаются (LED гаснет, триггеры сбрасываются, вибрация не отправляется). При переключении
обратно на DualSense — возобновляются. Аналогично rumble уже работает для Xbox (проверка
в `gamepad_rumble_tick()`).

### Шаг B0 — Подготовка + Интеграция библиотеки

**B0 часть 1 — C++20 в CMake (СДЕЛАНО):**
- `CMAKE_CXX_STANDARD 20` + проверка Clang >= 20
- Убран `register` в `Arctan()` (запрещён с C++17)
- Обновлены SETUP.md и tech_and_architecture.md

**B0 часть 2 — Интеграция Dualsense-Multiplatform (СДЕЛАНО):**
- GamepadCore подключён к CMake (`add_subdirectory`, static library)
- HID platform layer через SDL3 hidapi (кросс-платформенный, один код для Win/Linux/Mac)
- `ds_bridge.h/cpp` — мост между GamepadCore и игрой (аналог sdl3_bridge)
- `gamepad.cpp` — DualSense → ds_bridge, Xbox → sdl3_bridge, автопереключение
- `SDL_HINT_JOYSTICK_HIDAPI_PS5 = "0"` — SDL3 не трогает DualSense
- miniaudio подтянут для audio module (из Gamepad-Core-Audio)
- Workaround бага в GamepadCore CMake: подключаем Source/ напрямую (vendored код не тронут)
- Проверка: оба конфига собираются, требуется ручная проверка с DualSense

### Шаг B1 — Input через DS-lib (СДЕЛАНО в рамках B0)
- Input через GamepadCore работает, проверено с DualSense ✅

### Шаг B2 — LED lightbar (СДЕЛАНО)
- Цвет по здоровью: зелёный (75%+) → жёлтый (50%) → оранжевый (25%) → красный мигающий (<25%)
- Мигалка красно-синяя при сирене в полицейской машине
- Синий дефолт: старт, hotplug, пауза, выход из уровня
- Учитывает здоровье машины если в машине ✅

### Шаг B3 — Adaptive triggers + L2/R2 repurpose (СДЕЛАНО)
- L2/R2 убрана камера (дублирует правый стик), добавлены альтернативные действия
- В машине: R2=газ (аналоговый), L2=тормоз (аналоговый) + adaptive resistance на L2
- При стрельбе (first-person + gun out): R2=стрельба (+ adaptive weapon click на R2)
- Остальное: триггеры свободные, без adaptive effect
- Оригинальные кнопки (Cross/Square) сохранены, L2/R2 — альтернатива

### Шаг B4 — Вибрация через DS-lib (СДЕЛАНО в рамках B0)
- `gamepad_rumble_tick()` при DualSense → `ds_set_vibration()` + `ds_update_output()` ✅

### Отложено (после основной интеграции):

### Шаг B5 — Тачпад
- EnableTouch(), читать TouchPosition/TouchRelative
- Исследовать: камера? карта? быстрый доступ к инвентарю?

### Шаг B6 — Гироскоп
- EnableMotionSensor(), читать Gyroscope/Accelerometer
- Точное прицеливание в first-person mode

### Шаг B7 — Audio-to-haptic (miniaudio)
- AudioHapticUpdate() — конвертация игровых звуков в тактильную обратную связь
- Приоритетные эффекты: стрельба (отдача), взрывы, удары,
  двигатель машины (ощущение оборотов), шаги по разным поверхностям
- Используется miniaudio (уже включён в библиотеку)

---

## Что НЕ делаем на этом этапе

- Полная переработка управления (улучшения UX → этап 10)
- Замена клавиатуры/мыши на SDL3 (можно, но не обязательно)
- Рисование иконок кнопок в UI (инфраструктура `InputDeviceType` готова, UI — позже)

## Зависимости

- SDL3 заменяет SDL2 (vcpkg)
- Dualsense-Multiplatform (vendored в `new_game/libs/dualsense-multiplatform/`)
- Текущий input через DirectInput (`engine/input/joystick.*`) — заменяется на SDL3/DS-lib
- `game/input_actions.*` — маппинг input → game actions, расширяется под новые контроллеры

## Критические файлы

| Файл | Что делаем |
|------|-----------|
| `new_game/vcpkg.json` | SDL2 → SDL3 |
| `new_game/CMakeLists.txt` | SDL3, Dualsense-Multiplatform, убрать dinput |
| `new_game/libs/dualsense-multiplatform/` | **НОВЫЙ** — vendored library |
| `new_game/src/engine/audio/mfx.cpp` | SDL2→SDL3 audio API |
| `new_game/src/engine/input/gamepad.h/cpp` | **НОВЫЙ** — абстракция |
| `new_game/src/engine/input/gamepad_globals.h/cpp` | **НОВЫЙ** — globals |
| `new_game/src/engine/input/joystick.h/cpp` | Перезапись: DI → gamepad shim |
| `new_game/src/engine/input/joystick_globals.h/cpp` | DIJOYSTATE → GamepadState |
| `new_game/src/game/input_actions.cpp` | Маппинг, extern типы |
| `new_game/src/ui/frontend/frontend.cpp` | Extern типы |
| `new_game/src/ui/menus/pause.cpp` | Extern типы |
| `new_game/src/game/game.cpp` | Extern типы |
| `new_game/src/outro/core/outro_os.cpp` | DI → gamepad |
| `new_game/src/engine/platform/uc_common.h` | Убрать dinput.h |

## Проверка (end-to-end)

**Тестовое оборудование:** Xbox контроллер + DualSense (оба доступны у разработчика).

1. `make build-release` && `make build-debug` — без ошибок на каждом шаге
2. **Клавиатура:** запуск без контроллера — всё работает как раньше
3. **Xbox контроллер:**
   - Меню: навигация стиком, выбор кнопками
   - Gameplay: все действия (ходьба, бег, прыжок, удары, камера, инвентарь)
   - Аналоговый стик: плавное движение (не рваное D-Pad)
   - Вибрация при получении урона, взрывах, в машине
4. **DualSense:** то же что Xbox + (итерация B) adaptive triggers, LED, тачпад, гироскоп
5. **Hotplug:**
   - Подключение/отключение Xbox mid-game
   - Подключение/отключение DualSense mid-game
   - Переключение клава→контроллер→клава без задержек
   - Работает на всех экранах (меню, геймплей, пауза, outro)
6. **config.ini** [Joypad] — кастомный маппинг работает
7. **Звук (после шага A1):** фоновая музыка, SFX, голоса — всё как раньше

## Референсы

- `PieroZ/MuckyFoot-UrbanChaos` коммит `57c4560` — реализация controller support (DirectInput-based). Глянуть подход при реализации нашей SDL3 версии.
- `PieroZ/MuckyFoot-UrbanChaos` коммит `1ef00b7` (Kai) — аналоговое управление ломает клавиатуру. Учесть при реализации.
- `original_game/fallen/psxlib/Source/GHost.cpp` — PS1 DualShock: аналог, вибрация, конфиги кнопок.
- `original_game/fallen/psxlib/Source/GDisplay.cpp` — `PSX_SetShock()`: API вибрации.
- `original_game_knowledge_base/subsystems/psx_controls.md` — полная документация PS1 управления.
- `original_game_knowledge_base/subsystems/controls.md` — полная документация PC управления.
