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

### 4.1 duaLib — ⭐ ГЛАВНЫЙ источник

- Локально: `libs/duaLib/`
- Repo: https://github.com/WujekFoliarz/duaLib
- **Владелец репы**: WujekFoliarz (это open-source реализация
  `libScePad`)
- **Автор ключевого файла `triggerFactory.cpp`**:
  John "Nielk1" Klein. В шапке файла стоит
  `Copyright (c) 2021-2022 John "Nielk1" Klein`, лицензия **MIT**.
  Это тот же человек что написал [trigger effect factories gist](https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db)
  на который мы ссылаемся во всех наших доках.
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

## 6. Результаты реализации (2026-04-16)

### Что сделано

Либа написана и работает. Живёт на корне репы в `libDualsense/`
(sibling к `new_game/`) как автономный модуль со своим `CMakeLists.txt`.
Подключается к игре через `add_subdirectory` из `new_game/CMakeLists.txt`.
Потребители пишут `#include <libDualsense/ds_*.h>`. Полная
документация — `README.md` и `API.md` в папке либы.

**Фазы 1-6 выполнены:**
- Каркас: 6 модулей (ds_device, ds_input, ds_output, ds_trigger, ds_crc) + ds_bridge_own.cpp
- CMake: опция `OC_DUALSENSE_OWN_LIB` (default ON), vendored либа выключена
- Лицензирование: `LICENSE` (MIT, OpenChaos) + `THIRD_PARTY_LICENSES.md` (DS5W CRC, duaLib triggers)
- Тестирование: Windows USB ✅, Windows BT ✅ — input, output, LED, rumble, Weapon effect, Simple_Feedback (тормоз)
- **2026-04-17**: libDualsense расширена до 12 эффектов — добавлены factory функции
  `trigger_galloping`, `trigger_simple_weapon`, `trigger_simple_vibration`,
  `trigger_limited_feedback`, `trigger_limited_weapon` (packing 1:1 из duaLib).
  Бридж получил соответствующие врапперы + `ds_trigger_bow_full` /
  `ds_trigger_machine_full` с полным набором параметров для тестера.
  Проверка новых эффектов — через F11 → DualSense → TRIGGERS (см. девлог
  `input_debug_panel.md`).

**Фазы 7-8 (cleanup + cross-platform) — ещё не сделаны**, см. критерии готовности ниже.

### Баги найдены и починены при тестировании

- **Stick overflow:** raw=0 давал float -1.008, downstream DI scaling уходил в отрицательный → left-stick-left шёл в другую сторону. Фикс: `std::clamp` к [-1, +1].
- **USB output report size:** посылали 48 байт вместо 74 (как у rafaelvaloto). Контроллер мог интерпретировать короткий report по-другому.
- **BT sequence counter:** memset каждый кадр обнулял buf[40], XOR давал всегда 0x01 вместо alternating. Фикс: static переменная.
- **Trigger init mode:** slot'ы инициализировались 0x00 (undefined) вместо 0x05 (Off).
- **Feedback state masking:** пропускали raw байт вместо `& 0x0F`. Показывало 2..41 вместо 3..9.
- **BT one-shot init пакет** (обнаружено 2026-04-17): на Bluetooth контроллер молча игнорирует LED и player-LED поля во всех steady-state пакетах (`flag2=0x57`) пока не получит сразу после connect **одноразовый init-пакет** с обоими feature flags = `0xFF`. Rumble и триггеры работают без этого, LED — нет. Rafaelvaloto это делал внутри `DualSenseLibrary::Initialize()` при BT-подключении; когда мы в iteration1/2 оставили ds_bridge маршрутизацию через rafaelvaloto, LED работал, а после `5a8e3550` (полный переход на libDualsense) — сломался, и мы заметили только при тестировании lightbar widget в debug-панели. Фикс: в `ds_bridge::ds_poll_registry` после успешного `device_open_first` шлём init-пакет (flag1=0xFF, flag2=0xFF, CRC32 в хвосте) если connection == Bluetooth. USB не требует.

### Архитектурные решения

- Копирайты **только** в `THIRD_PARTY_LICENSES.md`, source files чистые (одна строка-pointer).
- Папка `libDualsense/` — полностью автономна, не зависит от ничего кроме SDL3.
- Namespace `oc::dualsense`, C-style функции + POD struct'ы, никаких классов.
- `ds_bridge_own.cpp` сохраняет тот же `ds_bridge.h` public API — game code не знает о переключении.

### Инструменты

- **Input debug panel (F11, после `bangunsnotgames`):** модальная панель
  с тремя sub-page'ами для DualSense — VIEW (лейаут контроллера:
  стики, кнопки, триггеры, touchpad XY, feedback/act), TESTS (rumble,
  lightbar RGB, player LEDs), TRIGGERS (тестер всех 12 adaptive-trigger
  эффектов). Ранее отдельный F2 overlay удалён — его функционал
  переехал в панель. Полный контекст:
  [`new_game_devlog/input_debug_panel.md`](../input_debug_panel.md).
- **Test mode (L3):** теперь независим от weapon профиля — hardcoded extreme `2/8/8`.

### TODO

- **non-Windows USB вибрация не работает** (обнаружено 2026-04-17, подтверждено
  cross-platform). Картина:
  - ✅ Windows USB: работает
  - ✅ Windows BT: работает
  - ✅ macOS BT: работает
  - ❌ **macOS USB: почти не работает** (мотор пытается стартовать и затухает)
  - ✅ Steam Deck BT: работает
  - ❌ **Steam Deck USB: почти не работает** — **1:1 как на маке**

  Значит проблема **не macOS-специфична** — общая для non-Windows USB-пути.
  Проверено с libDualsense и со старой vendored `dualsense-multiplatform`
  — одинаковое поведение → дело не в нашем слое, скорее в output report
  или в транспорте (SDL hidapi / raw HID API non-Windows). Сам контроллер
  не ломан — в онлайн-тестере [**ds.daidr.me**](https://ds.daidr.me/)
  (Web HID API через Chrome) на тех же устройствах+USB вибрация
  нормально работает.

  **Исходники тестера открыты:**
  **https://github.com/daidr/dualsense-tester** — можно читать код
  напрямую, не ревёрсить минифицированный build.

  **Nuance:** на Steam Deck тест через докстанцию, на маке тоже возможно
  был hub/dock. Стоит проверить прямым кабелем хотя бы на Steam Deck
  для изоляции (вдруг это hub-специфично).

  **План:** склонировать `daidr/dualsense-tester`, найти в нём
  output-report builder (вероятно TypeScript/JS), сравнить layout
  с нашим `libDualsense/src/ds_output.cpp::build_output_report`. Гипотезы:
  - (a) На non-Win USB нужен другой Report ID (не 0x02?).
  - (b) Transport padding отличается (SDL hidapi macOS/Linux vs Windows).
  - (c) Feature flags в output report'е (flag1=0xFF, flag2=0x57) не
    подходят для non-Win USB-пути.
  - (d) Размер write отличается — возможно 48 байт вместо 74, или
    наоборот.
  - (e) Первый output пакет на non-Win USB нужен init-style как на BT.

  Также проверить:
  - Что `device_open_first` подхватывает правильный interface number
    (macOS/Linux могут репортить DualSense с отличными от Windows
    параметрами).
  - Что `device_write` не режет данные (полный размер 74 байт?).
  - Что output report первого пакета после connect корректный.

  Запись в known_issues: `known_issues_and_bugs.md` раздел "Управление"
  — "non-Windows + DualSense USB: вибрация почти не работает". Там же
  параллельный вопрос про Xbox USB (похоже OS/driver issue, не наше).

### TODO (отложено, не срочно)

- **Гироскоп / акселерометр.** Хочется добавить в libDualsense парсинг
  IMU данных (gyro + accel из HID input report — оба уже доступны,
  просто у нас не читаются) и визуализацию в input debug panel на
  VIEW sub-page DualSense вкладки (какой-нибудь простой 3D или
  2D-проекционный индикатор угла наклона). Для геймплея пока не
  планируется использовать — чисто технический бэклог. Если
  понадобится калибровка (смещения в покое) — feature report 0x05
  содержит calibration data (rafaelvaloto читает его при открытии
  устройства; мы пока пропускаем).

- **True audio-to-haptic** (Sony-style PCM → voice-coil актуаторы DS).
  С переходом на собственную либу **инфраструктурно стало проще** —
  у нас есть полный контроль над HID output через `device_write`,
  знание транспорта (`Connection::Bluetooth` / `Usb`), CRC32 для BT,
  и намного чище код чем в вендоренной либе. Добавить в libDualsense
  функцию `audio_haptic_update(const int16_t* pcm, size_t n)` и
  соответствующий враппер в `ds_bridge` — **1-2 дня работы** чистой
  инфраструктуры. **Но "сложная часть" осталась та же** — формат
  данных и что именно DualSense ждёт в output report'е неизвестны
  для путей требующих реверса. Два пути:
  - **Путь A (miniaudio через USB-audio device):** на USB DualSense
    появляется как USB Audio Class устройство. Можно через
    miniaudio / OS audio stack играть PCM прямо в это устройство,
    DualSense будет модулировать voice-coil моторы. **Не зависит
    от libDualsense вообще** — параллельная подсистема, интеграция
    уровня "найти правильный audio output device + стримить PCM".
    Работает только на USB, ~2-3 дня. Грабли: кросс-платформенный
    device matching (на Windows один ID, на Linux/macOS — другой).
  - **Путь B (64-байтовые HID chunks по Bluetooth):** DualSense
    принимает какой-то закрытый HID output report с аудио-хаптик
    данными (в старой вендоренной либе это звалось
    `SendAudioHapticAdvanced`, 147 байт на BT). Транспортный код
    теперь тривиально добавляется в нашу либу, **но формат 64
    байт неизвестен** — нужно эмпирически реверсить (PCM?
    envelope? ADPCM?). Ни duaLib ни DS5W его не реализуют
    (только упоминают в roadmap). ~1-2 недели реверса с риском
    что не заработает. Только BT.
  - **Оба пути вместе** чтобы покрыть USB+BT: 2+ недель.

  Полный контекст исследования (до перехода на свою либу) —
  [`dualsense_audio_haptic_investigation.md`](../dualsense_audio_haptic_investigation.md).
  Для 1.0 используется envelope → rumble моторы (`weapon_feel`),
  true audio-haptic — post-1.0.

---

## 7. Открытые вопросы

- **DS5W's 0x26 vs duaLib's 0x26** — разные packing layouts для того
  же mode code. duaLib — MIT от автора спеки, используем его. Но
  при future тюне вибрации триггера стоит знать что existsues второй
  рабочий layout (упрощённый DS5W-style с 3 forces).

- **Calibration данные**: rafaelvaloto либа читает feature report 0x05
  при открытии устройства для gyro/accel калибровки. Мы пропускаем —
  IMU не используется (см. TODO про гироскоп в разделе "Инструменты").

---

## Связанная работа — панель отладки ввода

Модальная F11-панель для проверки и диагностики ввода живёт в
`new_game/src/engine/debug/input_debug/`. Активно использует наш
libDualsense / ds_bridge для тестирования DualSense output'ов (lightbar,
player LED, вибрация, adaptive triggers в будущем) и чтения DualSense-
специфичного state'а (touchpad XY, feedback/act).

Полный контекст архитектуры, итераций и **открытых багов панели** — в
[`new_game_devlog/input_debug_panel.md`](../input_debug_panel.md).

**Закрытые баги** (подробности в девлоге выше):
- ✅ ~~Точки стиков / заливка триггеров / точки тачпада не рисуются в своих
  боксах (только фоны видны) — баг рендера, возможно z-order/layer~~
  (Iteration 5 — z-order константы перевёрнуты).
- ✅ ~~Не все DualSense-кнопки промотируют `active_input_device` — touchpad
  click / mute / пальцы по тачпаду не переключали в DUALSENSE~~
  (Iteration 11 — цикл расширен до 19, добавлена проверка
  пальцев через `ds_get_input`).
- ✅ ~~DualSense не активируется в панели после hotplug / не все варианты
  хотсвапа между контроллерами работали~~
  (Iterations 11-14) — оказалось комбинация из трёх проблем: (1)
  activity-скан был гейтован `s_is_dualsense` вместо фактического
  успеха поллинга; (2) SDL ивенты дропались пока DS primary;
  (3) BT silence не детектился (на USB disconnect ловился через
  read-error, на BT `hid_read` молча возвращал 0 вечно);
  (4) `SDL3_GamepadEvent` не различал устройства по `instance_id` и
  REMOVED для DS убивал Xbox handle. Полностью протестировано юзером
  2026-04-17 — все комбинации USB/BT хотсвапа работают.

---

## 9. Связь с weapon_feel работой

После готовности собственной либы **следующий приоритет** —
переписать `weapon_feel_evaluate_fire` в `weapon_feel.cpp` чтобы
использовать **прямой сигнал `act` bit** из feedback status вместо
эмпирического угадывания момента щелчка по позиции `r2`. Подробности
плана — [`weapon_haptic_and_adaptive_trigger.md`](../weapon_haptic_and_adaptive_trigger.md)
раздел "План использования новых данных в weapon системе".

Это та задача ради которой всё это и началось. Собственная либа —
инфраструктурная подготовка к ней.

---

## 9. Критерии "готово"

- [x] Наша либа полностью заменяет rafaelvaloto для OpenChaos use
      cases
- [x] `weapon_feel_test` показывает идентичное поведение (r2, fb, act)
      как и с патченной rafaelvaloto либой
- [x] Используемые trigger effects работают: Weapon25 (пистолет),
      Simple_Feedback (тормоз машины)
- [x] Проверить неиспользуемые trigger effects: Feedback (0x21),
      Vibration (0x26), Bow (0x22), Machine (0x27), Galloping (0x23),
      Simple_Weapon (0x02), Simple_Vibration (0x06),
      Limited_Feedback (0x11), Limited_Weapon (0x12) — все отрабатывают
      на Windows, параметры из UI тестера влияют на поведение.
      Побочно обнаружено: simple/limited варианты **не репортят**
      feedback status (act bit + position nybble), в отличие от
      full-name эффектов. Детали — `dualsense_adaptive_trigger_facts.md`
      раздел "Feedback-репортинг зависит от семейства эффекта".
      macOS/Steam Deck — ещё не проверены.
- [x] Windows USB ✅ Windows BT ✅
- [ ] macOS USB/BT, Steam Deck USB/BT — не проверены
- [x] Старая `libs/dualsense-multiplatform/` удалена из репо
      (коммит `5a8e3550`)
- [x] Документация: `tech_and_architecture.md` обновлять не нужно
      (файл помечен как исторический референс, не обновляется).
      `dualsense_lib_pr_notes.md` оставлен как архив — PR в апстрим
      больше не делаем, ушли на свою либу
- [x] `weapon_feel_evaluate_fire` переписан на act-bit path для
      DualSense + click weapons (2026-04-17). Threshold path остался
      как fallback для auto-fire / click-less / non-DS. Детали в
      `new_game_devlog/weapon_haptic_and_adaptive_trigger.md` раздел
      "Миграция на act-bit детект щелчка".

---

## 11. Ресурсы по объёму

Оценка усилий:
- Кода написать: **~800-1200 строк** на C++ (из них ~500 копия
  triggerFactory.cpp)
- Удалить: **~10k строк** vendored rafaelvaloto либы
- Время на написание: **1-2 рабочих дня** + тестирование
- Cross-platform валидация: **~1-2 часа** с учётом переключения
  между устройствами (уже делали)

---

## 12. Future work — convenience layer (TODO)

**Задача:** ввести **чёткое двухуровневое разделение API** в
libDualsense:

- **Raw API (internal / low-level)** — работает в сырых HID-units
  (байты, raw int16, зоны, битовые маски). Это то что сейчас
  реализовано. Остаётся как «escape hatch» для случаев когда нужен
  точный контроль над протоколом.
- **Convenience API (public / high-level)** — работает в
  «человеческих» единицах (проценты 0..100% или float 0..1,
  нормализованные float, физические величины °/s и g, enum'ы
  вместо магических байтов). По умолчанию потребитель библиотеки
  использует именно этот слой.

### 12.1 Текущая inconsistency (что где уже есть)

Сейчас в репозитории **пёстрая картина** — часть полей уже в
«удобных» единицах, часть всё ещё raw, часть в промежуточных
zone-units. Это и создаёт ощущение «по-разному»:

**Output — почти всё raw:**
| Поле                          | Тип         | Диапазон                          | Статус |
|-------------------------------|-------------|-----------------------------------|--------|
| `rumble_left` / `rumble_right`| `uint8_t`   | 0..255                            | raw    |
| `lightbar_r/g/b`              | `uint8_t`   | 0..255                            | raw    |
| `player_led_mask`             | `uint8_t`   | bitmask                           | raw    |
| `player_led_brightness`       | `uint8_t`   | 0..2 (**0=brightest, перевёрнуто**)| raw + counterintuitive |
| `haptic_volume`               | `uint8_t`   | 0..7                              | raw    |
| `lightbar_setup`              | `uint8_t`   | raw byte                          | raw    |
| `speaker_volume`              | `uint8_t`   | 0..255                            | raw    |
| `headphone_volume`            | `uint8_t`   | 0..255                            | raw    |
| `mute_led`                    | `MuteLed`   | enum Off/On/Blink                 | ✅ convenience |
| `audio_route`                 | `AudioRoute`| enum Headphone/Speaker            | ✅ convenience |
| `audio_volumes_enabled`       | `bool`      | opt-in flag                       | ✅ convenience (policy, not raw) |
| Adaptive trigger effects      | uint8 params| **zones 0..9** + Hz               | semi (думаем зонами, но не в %) |

**Input — смешанный:**
| Поле                      | В `InputState` (lib-facing) | В `DS_InputState` (game-facing) |
|---------------------------|----------------------------|----------------------------------|
| Stick axes                | `uint8_t` 0..255 centre 128 | ✅ `float` -1..+1                |
| Triggers                  | `uint8_t` 0..255            | ✅ `float` 0..1                  |
| Battery level             | ✅ `uint8_t` 0..100 (%)     | ✅ `uint8_t` 0..100 (%)          |
| Gyro (pitch/yaw/roll)     | ⚠️ raw `int16`               | ⚠️ raw `int16`                    |
| Accel (X/Y/Z)             | ⚠️ raw `int16`               | ⚠️ raw `int16`                    |
| Motion timestamp          | `uint32` мкс (natural unit) | (не проброшено)                  |
| Motion temperature        | raw `int8`                  | (не проброшено)                  |
| Trigger feedback byte     | raw byte                    | ✅ state-nibble + act-bit          |
| Touchpad finger x/y       | `uint16` hardware pixels    | `uint16` hardware pixels         |

**Что это значит:** при работе с либой потребитель каждый раз
**угадывает** «это поле в байтах или в %?», «это zone-unit или
percent?», «надо ли самому калибровать gyro?». Нет последовательного
правила. Задача 12 — ввести это правило через явное разделение слоёв.

**Дополнительно — «скрытые» удобства в `InputState` (lib-facing), не
попавшие в таблицы выше:** эти поля уже декодируются из сырых битов
внутри `parse_input_report`, потребитель получает булевые / отдельные
флаги вместо сырых масок. При раскатке easy-фасада эти декоды
формально «уже convenience», но сам фасад должен их переиспользовать
без дублирования:

- `battery_charging` / `battery_full` — bool, вычисляются из
  `status0` high-nibble (0=discharge, 1=charge, 2=full, 10/11/15=error).
  Потребители: bridge (пробрасывает в `DS_InputState`), тестер (INPUT
  sub-page текст).
- `headphone_connected` / `mic_connected` — bool, биты 0/1 `status1`.
  Потребители: тестер (INPUT чекбоксы), bridge не пробрасывает.
- `left/right_trigger_effect_active` — bool, бит 4 fb-байта.
  Рядом лежит «сырой» `*_trigger_feedback` (весь байт). Потребители:
  bridge (обе формы в `DS_InputState`), weapon_feel (читает act-bit),
  тестер.
- `touchpad_finger_*_down` — bool, **инвертирован** от wire-бита 7
  (daidr convention: бит 7 = finger LIFTED). Потребители: bridge,
  тестер (VIEW точки).
- `battery_level_percent` — uint8 0..100, из 4-битного nibble 0..10 ×
  10 + cap по charging_complete. Потребители: тестер (INPUT текст).

**`normalize_stick_axis` / `normalize_trigger` helpers в `ds_input.h`**
(float -1..+1 / 0..1 с clamp) — тоже формально convenience, но живут
как функции а не как поле InputState. При build'e easy-фасада
использовать их напрямую.

### 12.2 Что должно быть в convenience API

- **Проценты 0..100% / float 0..1** для: rumble, audio volumes,
  haptic volume, trigger strength/amplitude, lightbar brightness.
- **Normalized float -1..+1** для sticks; **float 0..1** для trigger
  pull.
- **Physical units** для IMU: `°/s` (gyro) и `g` (accel) —
  автоматическое применение кешированной calibration (сейчас
  `calibrate_*` helpers есть, но их надо звать вручную — перенести
  в автоматический flow).
- **Нормальная шкала LED brightness**: 0 = off, 1 = brightest
  (инвертируется относительно raw 0=brightest/2=dim).
- **Enum'ы вместо битовых масок** для player LED (вместо сырой
  5-битной маски — набор типа `PlayerLedPattern::Off/Centre/Inner/Outer/All`
  + raw-slot для экзотики).
- **Trigger effects в процентах**: вместо zone 0..9 — float 0..1
  как позиция вдоль хода курка, strength/amplitude тоже 0..1.

### 12.3 Где разместить

- Держать в том же `libDualsense/` namespace `oc::dualsense` (не
  отдельная lib).
- Отдельный заголовок `ds_easy.h` (+ `.cpp`), либо пара
  `ds_easy_output.h` / `ds_easy_input.h`. Реализация **поверх**
  существующих `ds_output.h` / `ds_input.h` — никаких дублей
  wire-кода.
- Пример формы:
  ```cpp
  // ds_easy.h
  namespace oc::dualsense {

  struct EasyOutput {
      // 0..1 floats
      float rumble_left      = 0.0f;
      float rumble_right     = 0.0f;
      float speaker_volume   = 0.0f;
      float headphone_volume = 0.0f;
      float haptic_volume    = 0.0f;
      float led_brightness   = 1.0f;      // 0 = off, 1 = brightest

      // Enums
      MuteLed          mute_led    = MuteLed::Off;
      AudioRoute       audio_route = AudioRoute::Headphone;
      PlayerLedPattern player_leds = PlayerLedPattern::Off;

      // Lightbar — 0..1 floats
      float lightbar_r = 0.0f;
      float lightbar_g = 0.0f;
      float lightbar_b = 0.0f;

      EasyTrigger trigger_left;   // 0..1 params
      EasyTrigger trigger_right;
  };

  void apply_output(Device* dev, const EasyOutput& e);

  struct EasyInput {
      float left_stick_x, left_stick_y;    // -1..+1
      float right_stick_x, right_stick_y;
      float left_trigger,  right_trigger;  // 0..1

      float gyro_pitch_dps, gyro_yaw_dps, gyro_roll_dps;   // calibrated
      float accel_x_g, accel_y_g, accel_z_g;               // calibrated

      // buttons, battery %, headphone/mic flags pass through unchanged
  };

  bool read_input(Device* dev, EasyInput* out);
  }
  ```

### 12.4 Scope границы (НЕ идёт в convenience API)

Остаются **в потребителе** (игра / in-game тестер), не в либе:
- Диагностика (test tone, probe helpers) — per [`lib_scope.md`](lib_scope.md).
- Политики: когда брать audio ownership, как распределять rumble
  по событиям игры, какие trigger-эффекты проигрывать при стрельбе,
  cadence / smoothing / decay поверх моментальных значений.

### 12.5 Переделка in-game тестера (часть этой же задачи)

Когда convenience-слой появится — **пройтись по тестеру** в
[`new_game/src/engine/debug/input_debug/input_debug_dualsense.cpp`](../../new_game/src/engine/debug/input_debug/input_debug_dualsense.cpp)
и перевести всю UI-часть на friendly-форматы. Сейчас тестер во
многих местах показывает именно сырые байты / HID units — это было
сделано для диагностики на этапе ловли wire-багов, но теперь оно
мешает обычному использованию панели.

Конкретно надо:
- **OUTPUT sub-page** — все слайдеры перевести на проценты 0..100%
  (сейчас отображение в байтах 0..255 где применимо, 0..7 для haptic,
  0..2 для brightness). Шаг step'а подстраивать тоже в % (например
  5% / клик).
- **TRIGGERS sub-page** — параметры effects (start/end/strength/
  amplitude/frequency) в процентах вместо zone-units 0..9. Frequency
  оставить в Hz (это natural unit, уже понятно).
- **Trigger feedback read-only** — оставить state-nibble + act-bit
  как human-readable (уже так) + убрать сырой `slot XX XX ...`
  hex-dump (он для wire-отладки; если ещё нужен — только за кнопкой
  «show raw» / press key). Убрать «motor state %u (low nibble)» из
  основного отображения — заменить на подпись state'а словами
  («resting / engaged / released», что бы controller не reporting).
- **INPUT sub-page** — touchpad finger X/Y уже в hardware pixels,
  можно оставить; IMU бары уже в °/s + g когда calibration
  загружена, без калибровки fallback на raw — это теперь станет
  основным путём (easy API вернёт 0 если cal недоступна, UI
  отобразит «--»). Убрать в индикаторе trigger feedback отображение
  сырого byte 0x%02X — вместо этого текстовая интерпретация.
- **TELEMETRY sub-page** — hex-dump'ы Shift-JIS сериалов / barcodes
  / BT MAC / voltage заменять на декодированные ASCII строки там
  где daidr это делает (serial, battery barcode, VCM barcodes —
  это ASCII; assemble parts info — тоже ASCII; MAC format
  XX:XX:... уже есть; battery voltage конвертировать в mV по
  формуле если reverse-engineer её).
- **Debug-режим «показать raw»** опционально — глобальный toggle
  (например Shift+TAB) который переключает представление в
  «developer view» где всё в байтах как сейчас. Для 99% случаев
  тестирования он не нужен.

Общее правило: **без лишних байтов там где они не необходимы**.
Пользователь тестера не должен видеть HID-wire детали чтобы понять
«работает ли оно и что сейчас происходит».

### 12.6 Приоритет

**Низкий.** Raw API функционален и достаточен для OpenChaos. Convenience
layer — чисто UX. Делать **после**:
1. Полной стабилизации raw API на всех платформах (Win/Mac/Linux ×
   USB/BT).
2. Закрытия оставшихся известных багов (см. review doc §2.4.x).

Только потом имеет смысл строить easy layer поверх — иначе easy
будет легализовывать баги raw API.

---

## 13. Future work — audio output и audio haptics (TODO)

На DualSense есть **три независимых способа** сделать так чтобы
пользователь что-то услышал / ощутил:

1. **WAVEOUT_CTRL** — 1 kHz синусоида из прошивки. **Уже реализовано**
   в тестере через `test_command` (см. [`lib_scope.md`](lib_scope.md):
   diagnostic-only, не в либе). Заводской тест-генератор, больше ни
   для чего не годится.
2. **USB Audio Class (обычный PCM в спикер / 3.5 мм jack)** —
   отдельная от HID аудио-дорожка, см. §13.1 ниже. **Не планируется
   реализовывать**, остаётся документацией.
3. **HID audio haptics (PCM через output report 0x32 в вибромоторы)** —
   Sony-специфично, см. §13.2 ниже. **Отложено до post-1.0.**

### 13.1 PCM audio output — обычный звук в спикер / наушники контроллера

**Что это:** DualSense на USB exposes себя в ОС как standard **USB
Audio Class 1.0** output device. В системном списке звуковых устройств
он виден как «Wireless Controller». Любое приложение / игра открывает
его через **обычный audio API** (SDL_Audio, miniaudio, WASAPI,
CoreAudio, ALSA) — точно так же как любую другую звуковую карту —
и шлёт туда stereo PCM 48 kHz. Контроллер играет это на встроенном
спикере (моно downmix) или на 3.5 мм jack (стерео).

**К libDualsense этот путь отношения не имеет** — это полностью
системный аудио-тракт, параллельный нашей HID-части. Игра решает
этот вопрос через свой audio subsystem, не через нас.

**Транспорт:**
- USB: ✅ работает (USB Audio Class endpoint).
- Bluetooth: ❌ **не работает никак**. BT HID profile DualSense
  **не несёт A2DP** (профиль stereo-audio). В списке audio devices
  BT-контроллер попросту отсутствует. Sony сделали так намеренно —
  A2DP добавил бы сотни мс latency и испортил бы синхронизацию
  звука с игрой / haptics.

**Статус в OpenChaos:** **не планируется реализовывать.** Игра сейчас
выводит звук через обычный SDL_Audio default device (колонки ПК).
Принудительно переключать на controller output = лишить игрока
выбора устройства + не работает на BT → суммарно минусы.

**Причина упоминания здесь** — чтобы в будущем не возникало путаницы
«а может сделаем чтобы в наушники контроллера играло?»: на USB
технически достижимо одной строчкой `SDL_OpenAudioDevice` с именем
controller'а, но архитектурно не нужно и не даёт ничего полезного.

### 13.2 Audio haptics — PCM через HID 0x32 в вибромоторы

**Что это:** DualSense поддерживает передачу PCM-звука через HID
output report **0x32** 64-байтными chunk'ами. Контроллер **не
воспроизводит** этот звук через динамик — вместо этого его DSP
превращает сигнал в **вибрацию**: амплитуда/форма волны модулируют
моторы rumble + возможно adaptive trigger'ы. Это то что Astro's
Playroom использует для «физичных» эффектов: звук шагов по снегу ты
не только слышишь, но и **чувствуешь** ладонями — вибрация повторяет
огибающую этого звука.

**Формат:** Sony-undocumented. HID output report 0x32, частично
reverse-engineered сообществом. Требует специфический DSP кодек /
sample-rate (предположительно 3 kHz PCM или подобное), формат 64-байт
chunk'ов неясен без дополнительного расковыривания (см. старый
investigation doc [`../dualsense_audio_haptic_investigation.md`](../dualsense_audio_haptic_investigation.md)).

**Транспорт:**
- USB: ✅ работает.
- Bluetooth: ✅ работает (идёт через HID, не требует отдельного
  аудио-профиля — в этом главное преимущество перед §13.1).

**Задачи для реализации:**
- Reverse-engineer точный wire-format chunk'ов 0x32 (сэмплирование,
  chunk header'ы, flow control).
- Определить какой sample rate и битность controller принимает.
- Реализовать resampler (игровой audio обычно 44.1/48 kHz stereo,
  haptics вероятно 3 kHz mono).
- Реализовать streaming API в libDualsense: `start_haptic_stream()`,
  `push_haptic_samples(buffer, len)`, `stop_haptic_stream()`.
- Интеграция со звуковым движком игры — сигнал для haptics берётся
  либо из специального LFE-канала, либо из основной stereo-дорожки
  через фильтр (low-pass / envelope extraction).

**Статус в OpenChaos:** **отложено до post-1.0.** daidr это **не
делает** (никакого audio haptic pipeline'а у них нет, несмотря на
«tester» в имени), так что в рамках «паритет с daidr» эта задача
вообще не стоит. Но для OpenChaos как игровой фичи — интересно, и
это единственный способ доставить пользовательский аудиоконтент на
BT-контроллер. Делать после 1.0.

**Где разместить в либе:**
- Отдельный модуль `ds_haptic_stream.h/.cpp` в `libDualsense/`.
- Streaming API (не request/response) — thread-safe (поток игрового
  аудио-миксера пишет PCM, внутренний поток либы отсылает chunks в
  HID с нужным темпом).
- Полностью независим от `OutputState` / output report 0x02/0x31 —
  это отдельный HID-канал, параллельный обычному output.

### 13.3 Сравнительная таблица (какой путь для чего)

| Задача                                | §13.1 USB Audio | §13.2 Audio haptics (0x32) | §12.x test tone (WAVEOUT_CTRL) |
|---------------------------------------|-----------------|------------------------------|----------------------------------|
| «Играть музыку в наушники контроллера»| ✅ USB only     | ❌ (это вибрация, не звук)   | ❌ (только 1 kHz синус)          |
| «Тактильный эффект от выстрела»       | ❌              | ✅                           | ❌                               |
| «Проверить что спикер живой»          | возможно, но оверкил | ❌                      | ✅ (для этого и нужен)            |
| Работает по BT                        | ❌              | ✅                           | ✅                               |

### 13.4 Приоритет

**§13.1 — не делаем.** Документация для будущих читателей кода
«да, физически возможно на USB, но нам не нужно».

**§13.2 — делаем, но post-1.0.** Не блокирует релиз. Требует
отдельного R&D-захода (reverse-engineering wire-format'а, ~2-14 дней
по старой оценке из [`dualsense_audio_haptic_investigation.md`](../dualsense_audio_haptic_investigation.md)).

---

## 14. Known hardware / firmware limitations (починить вряд ли получится)

Здесь — поведения контроллера, которые выглядят как баги, но
**воспроизводятся и у daidr** на том же железе (проверено 2026-04-19).
Значит это характеристики DS5 firmware / audio ASIC, не косяки нашей
либы. Записано чтобы потомки не тратили время на «починку» того что
не чинится с host-стороны.

### 14.1 Audio cross-leak между выходами

**Симптом:** при запуске `WAVEOUT_CTRL` с routing на headphone
(`audioControl = 0x00`, prepare-payload `[4]=4, [6]=6`) тон физически
слышен **и в наушниках, и в встроенном спикере одновременно**.
Аналогично при routing на speaker (0x30) тон частично leak'ит в jack
если наушники подключены. Наблюдается на BT особенно явно; на USB
тихо, но есть.

**Причина (гипотеза):** внутренний audio chain DS5 не полностью
разделяет выходы при генерации тестового тона — генератор
подключается к обоим amp'ам, а `audioControl` nibble управляет
только relative gain'ом, не physical switch'ом.

**Обход (используется у нас и у daidr):** UI-слой при включении
тона **принудительно выставляет volume неактивного выхода в 0**.
Это заглушает leak amplitude'но. В `input_debug_dualsense.cpp`
функция `audio_apply_state()` делает это автоматически (single-slider
UI model: активный выход получает `s_volume`, неактивный — 0).

**Почему не фиксить:** с host-стороны невозможно — `audioControl`
nibble единственный routing control и он уже установлен корректно.
Проблема на уровне контроллерного DSP. Sony FW update мог бы
починить, но мы не Sony.

### 14.2 Speaker gain boost при подключении наушников в speaker mode

**Симптом:** при активном `audio_route = Speaker`
(`audioControl = 0x30`) и включённом test tone speaker играет на
одном уровне. Подключаем наушники в jack → **speaker становится
заметно громче**, хотя host отправляет те же самые байты в output
report (headphone_volume = 0, speaker_volume не меняется).

**Причина (гипотеза):** когда `audioControl = 0x30` явно запрашивает
speaker output, firmware не выключает speaker при plug-detect
(нормальное consumer-устройство — выключило бы). Более того, видимо
активируется какой-то gain compensation / dual-rail power mode —
возможно контроллер отдаёт speaker'у больше тока когда часть audio
chain'а заодно висит на jack amp'е.

**Обход:** нет. Пользователю надо либо смириться, либо не
подключать наушники когда сознательно выбрал speaker output.

**Почему не фиксить:** те же самые output-байты, та же самая команда
— разница только в physical state jack'а, который контроллер
обрабатывает на своём уровне. Нет HID-пути чтобы сказать контроллеру
«не бойся jack'а, держи speaker на том же gain'е».

### 14.3 Test tone не глохнет при выходе из приложения

**Симптом:** пользователь включил `WAVEOUT_CTRL` test tone через
отладочную панель, потом закрыл игру (штатно, через Ctrl+Q / закрытие
окна). LED-индикация / lightbar / rumble / triggers корректно
обнуляются на выходе (последний output report отрабатывает), **но
тон продолжает звучать из контроллера** после того как процесс игры
уже завершился. Ручно гасится только переподключением контроллера
(unplug USB / отключить BT).

**Что пробовали:**
- `test_command(WAVEOUT_CTRL, {0,1,0})` с polling 0x81 — ответ
  приходит timeout'ом, но сам 0x80 уходит. Эффекта нет.
- Две попытки disable подряд с паузой 80 мс между — не помогает.
- Отправка quiet output до / после disable, с/без
  `audio_volumes_enabled` — не помогает.
- Удлинение `SDL_Delay` до 200+ мс перед `device_close` — не помогает.

**Причина (гипотеза):** audio generator DS5 — отдельный firmware
subsystem от HID-output пути. Когда process завершается и HID handle
закрывается, controller возвращает audio управление ОС, которая
восстанавливает default volumes — и тон, запущенный через
`WAVEOUT_CTRL`, **продолжает играть в hardware** пока его явно не
остановить. Наш disable-0x80 уходит, но firmware не обрабатывает
его в этот момент (возможно привязано к 0x81 polling cadence
которого после закрытия handle не будет; возможно просто race с
process teardown).

**Обход:** нет удобного. Current `ds_shutdown` делает best-effort
попытку (двойной disable с паузой, quiet output, 50 мс pre-close
delay) — иногда срабатывает, чаще нет. Оставлено как best-effort,
не гарантия.

**Общий вывод про WAVEOUT_CTRL test tone:**

Весь этот путь (`WAVEOUT_CTRL` + `BUILTIN_MIC_CALIB_DATA_VERIFY`) —
**заводской диагностический механизм Sony**, не production-grade
feature. Он flaky сразу в нескольких местах:
- Требует 0x81 polling чтобы запущенная команда реально обработалась
  (§12.x поведение: fire-only отправка вообще игнорируется
  firmware'ом; см. обсуждение в in-game тестере).
- Cross-leak между выходами (§14.1).
- Speaker gain boost при jack plug (§14.2).
- Не выключается на shutdown (§14.3 выше).

Это не наша проблема, это качество заводского диагностического
API. Использовать его только для того для чего он предназначен —
«проверить что audio hardware в принципе дышит», и то с оговорками.

### 14.4 Общее правило для ds-quirks

Если поведение **воспроизводится в daidr** на том же железе — это
hardware/firmware limit, не наш bug. Записываем сюда и идём дальше.
Тратить время на «попробовать ещё один флаг» бессмысленно — daidr
уже перебрал все разумные варианты за нас.
