# Plan — писать свою DualSense библиотеку

**Цель**: заменить вендоренную [`rafaelvaloto/Dualsense-Multiplatform`](https://github.com/rafaelvaloto/Dualsense-Multiplatform)
в `new_game/libs/dualsense-multiplatform/` на собственный минимальный
OpenChaos-owned модуль. Мотивация — найдено что у либы сломаны
большинство trigger effect packing'ов, плюс отсутствует чтение
trigger feedback status. Вместо поддержки двух локальных патчей и
потенциально больше — проще написать ровно то что нам нужно.

Документ предназначен для агента с **чистым контекстом** который
подхватывает эту задачу. Читайте этот файл первым — он
self-contained и ссылается на все остальные источники.

---

## 1. Быстрый контекст

- **Ветка**: `dualsence_test` (НЕ `main`). Содержит патчи в вендоренной
  либе + тестовый инструмент `weapon_feel_test.cpp`.
- **Что уже работает (через локальный патч)**:
  - Trigger feedback status reading (bytes 0x29/0x2A входного report'а)
  - Weapon25 effect correct packing
- **Что не работает** (поэтому хочется своя либа):
  - Почти все остальные trigger effect packing'и в либе поломаны
    (Bow, Vibration/MachineGun26, Machine27)
  - Отсутствуют реализации Feedback (0x21), Simple_*, Limited_*,
    MultiplePosition*, SlopeFeedback
  - Наши локальные патчи надо поддерживать на каждом update либы

Полная предыстория и контекст всех находок — обязательно прочитай:
- [`dualsense_lib_pr_notes.md`](dualsense_lib_pr_notes.md) — что сломано и как правильно паковать
- [`dualsense_protocol_reference.md`](dualsense_protocol_reference.md) — протокол целиком
- [`dualsense_adaptive_trigger_facts.md`](dualsense_adaptive_trigger_facts.md) — эмпирические измерения

---

## 2. API surface который нужен OpenChaos

Посмотри что именно игра использует из либы сейчас (grep по `new_game/src/engine/platform/ds_bridge.*` и `new_game/src/engine/input/gamepad.*`):

### Input (читаем)
- Left/right analog sticks (0..255 per axis, center 128)
- L2/R2 analog triggers (0..255)
- All face buttons, shoulders, d-pad, start/share/ps
- L3/R3 stick click, touchpad click (optional)
- **Trigger feedback status** (one byte per trigger, bit 4 = effect active)
- Battery level

### Output (пишем)
- Lightbar RGB
- Slow/fast rumble (DS4-style двумотор)
- Adaptive trigger effects — **минимум** Weapon25 и Feedback/Resistance
  для тормоза в машине. **Желательно** Vibration (0x26) правильный
  для будущей вибрации триггера на AK47.

### Lifecycle
- Init/shutdown
- Device discovery (USB + Bluetooth)
- Hotplug (connect/disconnect in-game)
- Per-tick poll (read input)
- Per-tick flush (write output only when state changed)

### НЕ нужно (упрощает)
- DualShock 4 support
- DualSense Edge paddles/Fn keys
- Gyro/accelerometer
- Touchpad multi-touch coordinates
- Audio haptic (PCM→voice-coil)
- Player LED business logic (мы сами считаем health-based LED)
- Calibration maths
- Microphone LED
- Haptic audio route switching

---

## 3. Архитектура предлагаемой либы

Всё живёт в `new_game/src/engine/platform/dualsense/`. Никаких
отдельных подкаталогов как у vendored, просто плоская структура:

```
new_game/src/engine/platform/dualsense/
  ds_device.h         -- FDeviceContext-like struct + constants
  ds_device.cpp       -- device discovery, connect, disconnect via SDL3 hidapi
  ds_input.h          -- input parser public API
  ds_input.cpp        -- parses HID input report into struct DS_Input
  ds_output.h         -- output builder public API  
  ds_output.cpp       -- builds HID output report from struct DS_Output
  ds_trigger.h        -- trigger effect factory (public)
  ds_trigger.cpp      -- все эффекты — адаптировано из duaLib
                        triggerFactory.cpp (MIT, copyright preserved)
  ds_crc.h            -- CR32 для BT output reports
  ds_crc.cpp          -- реализация (можно адаптировать из rafaelvaloto
                        либы или duaLib)
```

Верхний уровень — уже существующий `ds_bridge.h` / `ds_bridge.cpp`
остаётся как основной вход из игрового кода. Он просто переключается
с вызова vendored либы на вызовы нашего модуля.

**Важно**: не создавай namespace-цирк как в rafaelvaloto
(`FDualSenseTriggerComposer`, `FGamepadOutput`, `FInputContext`,
`TGenericHardwareInfo<Policy>`, etc). Просто C-style функции и POD
struct'ы в `namespace oc::dualsense { ... }`.

### HID transport — SDL3 hidapi

Уже используется в ds_bridge.cpp через `SDL_hid_*`. Логика открытия
устройства, nonblocking reads, enumeration — всё уже есть в текущем
`ds_bridge::uc_hid` namespace. Можно целиком перенести эту часть.

### USB vs BT handling

DualSense разный pad layout для USB и Bluetooth:
- **USB**: Report ID `0x01` (input), `0x02` (output)
- **Bluetooth**: Report ID `0x31` (input), `0x31` (output), CRC32 в
  хвосте output'а

После strip Report ID и BT-specific extra header — byte offsets
становятся ОДИНАКОВЫМИ для обоих transports. Это уже так работает в
rafaelvaloto либе (см. `FDualSenseLibrary::UpdateInput` с Padding=1 или 2).

---

## 4. Reference libraries (локальные копии)

Все референсные библиотеки лежат в [`libs/`](libs/) (под gitignore).
Если папка пустая — следуй [`libs_contents.md`](libs_contents.md)
чтобы восстановить все клоны с нуля (это точная инструкция, ~5 команд
`git clone`).

### 4.1 duaLib (Nielk1, MIT) ⭐ ГЛАВНЫЙ

- Локально: `libs/duaLib/`
- Repo: https://github.com/Nielk1/duaLib
- Autor: John "Nielk1" Klein — **тот же** кто написал
  референс-спеку gist.
- Ключевой файл: `libs/duaLib/src/source/triggerFactory.cpp` —
  авторитетная реализация **всех** trigger effect packing'ов. MIT
  лицензия позволяет копировать с сохранением copyright notice.
- **Что брать**: весь `triggerFactory.cpp` с минимальной адаптацией
  под наш стиль (буферы, типы).

### 4.2 DualSense-Windows (Ohjurot, DS5W) ⭐ для input parsing

- Локально: `libs/DualSense-Windows/`
- Repo: https://github.com/Ohjurot/DualSense-Windows
- Ключевые файлы (все пути относительны `libs/DualSense-Windows/`):
  - `VS19_Solution/DualSenseWindows/src/DualSenseWindows/DS5_Input.cpp` —
    полный input report parser, все offsets комментированы
  - `VS19_Solution/DualSenseWindows/src/DualSenseWindows/DS5_Output.cpp` —
    output builder (упрощённый, только 3 эффекта, но общая структура
    полезна для референса)
  - `VS19_Solution/DualSenseWindows/src/DualSenseWindows/DS_CRC32.cpp` —
    CRC32 для BT
  - `VS19_Solution/DualSenseWindows/src/DualSenseWindows/IO.cpp` —
    device open/close/read/write
- **Важно**: feedback offsets подтверждены **идентичны** нашим —
  `hidInBuffer[0x29]` для R2, `hidInBuffer[0x2A]` для L2.
- Стиль кода — C++17 с голыми struct, похож на то что нам надо.

### 4.3 nondebug/dualsense (документация)

- Локально: `libs/nondebug-dualsense/`
- Repo: https://github.com/nondebug/dualsense
- Ключевой файл: `libs/nondebug-dualsense/dualsense-explorer.html` —
  HTML страница с live HID monitoring. В JS-коде этой страницы
  хардкожены offsets всех полей input report'а. Grep по `axes4`,
  `axes5`, `r2feedback`, `l2feedback`, `battery`, `touchpad1` и т.п.

### 4.4 rafaelvaloto pristine (для diff reference)

- Локально: `libs/rafaelvaloto-pristine/`
- Repo: https://github.com/rafaelvaloto/Dualsense-Multiplatform
- Зачем: у нас в `../new_game/libs/dualsense-multiplatform/` уже
  вендоренная с **патчами** (маркер `OPENCHAOS-PATCH`). Чистая
  копия нужна чтобы (а) видеть оригинальное состояние не патченных
  функций, (б) при подготовке PR в апстрим делать diff против чистого
  baseline'а.

### 4.5 rafaelvaloto patched (зеркало нашего форка)

- Локально: `libs/rafaelvaloto-patched/` (mirror копия из
  `../new_game/libs/dualsense-multiplatform/`)
- Удобно для прямого `diff -ru` против `rafaelvaloto-pristine/` —
  показывает полный эффект наших локальных патчей одним выводом.

### 4.6 DualSenseX (Paliverse, closed) — опционально

- Локально: `libs/DualSenseX-Paliverse/` (не обязательно)
- Repo: https://github.com/Paliverse/DualSenseX
- Только UDP example полезен — показывает API contract что
  коммерческий продукт считает правильными parameter ranges для
  каждого эффекта. Полезен для sanity check что наши константы
  валидны.

---

## 5. Предлагаемый пошаговый план

### Фаза 1: Подготовка (~30 мин)
1. Прочитать этот файл + три DualSense devlog doc'а.
2. Пробежаться по структуре `dualsense_libs_reference/duaLib/` и
   `DualSense-Windows/` чтобы понимать code style обеих либ.
3. Прочитать текущий `ds_bridge.cpp` и `ds_bridge.h` в OpenChaos —
   понять что именно уже работает через SDL3 hidapi и что из этого
   можно перенести как есть в нашу либу.

### Фаза 2: Каркас (~1 час)
4. Создать `new_game/src/engine/platform/dualsense/` структуру.
5. Вынести `uc_hid::` namespace (HID layer на SDL3) из `ds_bridge.cpp`
   в `ds_device.cpp`. Это уже наш код, просто переложить.
6. Добавить `OPENCHAOS-PATCH`-style copyright header в файлы которые
   адаптируются из duaLib или DS5W (MIT).
7. Обновить CMakeLists — добавить новые sources, убрать `add_subdirectory`
   для vendored liba (пока только замена используется — оставить vendored
   параллельно как backup, переключать через feature flag).

### Фаза 3: Input (~1-2 часа)
8. Адаптировать `DS5_Input.cpp` логику в `ds_input.cpp`. Использовать
   наши offsets (те же что у DS5W). Добавить trigger feedback чтение
   (у DS5W уже есть).
9. Результат кладётся в наш `DS_InputState` struct из `ds_bridge.h` —
   чтобы интеграция с gamepad.cpp осталась неизменной.
10. Протестировать что все поля читаются правильно через существующий
    `weapon_feel_test` тулинг.

### Фаза 4: Output (~2-3 часа)
11. Адаптировать CRC32 из duaLib или DS5W в `ds_crc.cpp` (MIT, ~30 строк).
12. Написать `ds_output.cpp` с базовой структурой output report
    (lightbar, rumble, feature flags). Ссылаться на rafaelvaloto
    `GamepadOutput.cpp::OutputDualSense` как reference offsets —
    но реализовывать с нуля чтобы избежать их багов.
13. Скопировать `triggerFactory.cpp` из duaLib в `ds_trigger.cpp`
    целиком с MIT copyright notice. Адаптация минимальная — только
    интеграция с нашим буфером (duaLib пишет в `uint8_t(&forces)[11]`,
    у нас `TriggerCompose` struct с `std::uint8_t Compose[10]` или
    что-то подобное — просто mapping).
14. Обёртки `oc::dualsense::trigger_weapon(start, end, strength)` и
    подобные для других эффектов — тонкий уровень.

### Фаза 5: Device discovery (~1 час)
15. Перенести существующую `uc_hid::detect` логику. Ничего нового.

### Фаза 6: Интеграция (~1 час)
16. Переключить `ds_bridge.cpp` с vendored liba на нашу через feature
    flag (например `OC_DUALSENSE_OWN_LIB` define в CMakeLists или
    runtime env var).
17. Build + run + протестировать `weapon_feel_test` в обоих режимах.
18. Убедиться что поведение идентично (или лучше — без багов).

### Фаза 7: Cleanup (~30 мин)
19. После подтверждения что наша либа работает стабильно — удалить
    `new_game/libs/dualsense-multiplatform/` из репо (или оставить
    как backup с комментарием что unused).
20. Обновить `tech_and_architecture.md` с пометкой про переход на
    свою DualSense либу.
21. Закрыть `dualsense_lib_pr_notes.md` как архивный (помётка что
    PR апстриму больше не нужен — мы ушли на свою либу).

### Фаза 8: Cross-platform валидация (по потребности)
22. Если владельцу проекта надо — прогнать на всех 6 конфигурациях
    (Win BT/USB, macOS BT/USB, Steam Deck BT/USB) как раньше с
    `weapon_feel_test` инструментом.

---

## 6. Детали о текущем состоянии кода

### Что есть сейчас (ветка `dualsence_test`):

- **Патчи в либе** (обёрнуты `=== OPENCHAOS-PATCH BEGIN/END ===`):
  - `InputContext.h` — новые fields для feedback
  - `GamepadInput.h::DualSenseRaw` — чтение offsets 41/42
  - `GamepadTrigger.h::Weapon25` — правильный packing

- **Плюмбинг в игровом коде**:
  - `ds_bridge.h` — поля `*_trigger_feedback_state` и
    `*_trigger_effect_active` в `DS_InputState`
  - `ds_bridge.cpp::ds_get_input` — копирует их из FInputContext
  - `gamepad.cpp` — static cache + getters
    (`gamepad_get_right_trigger_effect_active` и т.п.)
  - `gamepad.h` — декларации getter'ов
  - `weapon_feel.h` — `WeaponFeelProfile` имеет
    `trigger_start_zone`, `trigger_end_zone`, `trigger_strength` поля
  - `weapon_feel.cpp` — профили с тестовыми значениями (пистолет
    сейчас `start=2 end=8 strength=8` для extreme теста)
  - `gamepad.cpp::apply_trigger_mode` — передаёт профильные значения
    в `ds_trigger_weapon`

- **Тестовый инструмент**:
  - `weapon_feel_test.h` / `.cpp` — observation mode, активируется
    `\` или L3 stick click, рисует live `r2=`, `fb=`, `act=` на
    экране через `FONT_buffer_add`

### Что ждёт commit (несколько doc-only файлов):

- `dualsense_lib_pr_notes.md` — разделы 6.3 (duaLib главный источник),
  6.4 (DSX), 6.5+ (ранее были 6.3+) перенумерованы, обновлены источники
- `dualsense_protocol_reference.md` — добавлены DS5W + DSX + duaLib в
  sources
- `dualsense_adaptive_trigger_facts.md` — обновлены independent
  sources (DSX, DS5W, duaLib)
- Этот файл (`own_dualsense_lib_plan.md`) — новый

---

## 7. Что хранится в main (после previous merge)

В main ветке уже живёт **PR #1** функциональность — чтение trigger
feedback status через патч либы. А также обновлённый `weapon_haptic_and_adaptive_trigger.md`
с планом использования `act` bit для fire detection (который мы
хотели сделать **после** либы, но по факту не начали — это следующая
цель после либы готова будет).

Вся foundation для `act`-based fire detection в main: getters
`gamepad_get_right_trigger_effect_active()` и прочие. Можно
использовать сразу как только стабилизируемся на своей либе.

---

## 8. Открытые вопросы и notes

- **DS5W's 0x26 vs duaLib's 0x26** — разные packing layouts для того
  же mode code. duaLib — MIT от автора спеки, используем его. Но
  потенциально при future тюне вибрации триггера стоит знать что
  existsues второй рабочий layout (упрощённый DS5W-style с 3
  forces). См. `dualsense_lib_pr_notes.md` раздел с DS5W.

- **Mode code для Off**: gist говорит 0x05, vendored либа использует
  0x00, duaLib использует 0x05. Оба работают. В нашей либе — 0x05
  как каноничный вариант.

- **Calibration данные**: rafaelvaloto либа читает feature report 0x05
  при открытии устройства для gyro/accel калибровки. **Нам это не
  нужно** потому что мы не используем IMU. Можно пропустить этот
  шаг при нашем device open — будет просто быстрее.

- **Audio haptic**: rafaelvaloto либа имеет 147-байтный audio buffer
  и `ProcessAudioHaptic` функцию. **Не нужно**, удаляем.

---

## 9. Связь с weapon_feel работой

После готовности собственной либы **следующий приоритет** —
переписать `weapon_feel_evaluate_fire` в `weapon_feel.cpp` чтобы
использовать **прямой сигнал `act` bit** из feedback status вместо
эмпирического угадывания момента щелчка по позиции `r2`. Подробности
плана — [`weapon_haptic_and_adaptive_trigger.md`](../new_game_devlog/weapon_haptic_and_adaptive_trigger.md)
раздел "План использования новых данных в weapon системе".

Это та задача ради которой всё это и началось. Собственная либа —
инфраструктурная подготовка к ней.

---

## 10. Критерии "готово"

- [ ] Наша либа полностью заменяет rafaelvaloto для OpenChaos use
      cases
- [ ] `weapon_feel_test` показывает идентичное поведение (r2, fb, act)
      как и с патченной rafaelvaloto либой
- [ ] Все trigger effects которые мы используем (Weapon25, Feedback
      для тормоза, опционально Vibration для AK47) работают корректно
- [ ] Cross-platform тестирование на 6 конфигурациях (Win BT/USB,
      macOS BT/USB, Steam Deck BT/USB) проходит
- [ ] Старая `libs/dualsense-multiplatform/` удалена из репо или
      отмечена как unused backup
- [ ] Документация обновлена (`tech_and_architecture.md`, пометка в
      `dualsense_lib_pr_notes.md` что PR в апстрим больше не
      актуален)
- [ ] `weapon_feel_evaluate_fire` **пока не трогаем** — это отдельная
      задача после миграции на свою либу

---

## 11. Ресурсы по объёму

Оценка усилий:
- Кода написать: **~800-1200 строк** на C++ (из них ~500 копия
  triggerFactory.cpp)
- Удалить: **~10k строк** vendored rafaelvaloto либы
- Время на написание: **1-2 рабочих дня** + тестирование
- Cross-platform валидация: **~1-2 часа** с учётом переключения
  между устройствами (уже делали)
