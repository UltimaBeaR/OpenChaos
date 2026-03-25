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
