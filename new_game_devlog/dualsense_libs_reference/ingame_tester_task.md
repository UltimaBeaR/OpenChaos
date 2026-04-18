# Task: in-game tester for libDualsense (пункт 8 плана паритета с daidr)

Финальный шаг работы в ветке `daidr_lib_investigation`. Почти всё из
плана паритета с `daidr/dualsense-tester` сделано — осталось сделать
in-game тестер всех фич libDualsense.

## Что прочитать в этом порядке

1. **`CLAUDE.md`** — правила проекта. Критично:
   - магические числа только через именованные константы;
   - утверждения про оригинальную игру — только после проверки по KB
     (`KB_INDEX.md` для навигации);
   - git — только чтение, никаких мутаций;
   - коммиты делает пользователь, не ты;
   - целевое железо — Steam Deck / слабые ПК / Mac / Linux, не только
     Windows.

2. **`new_game_devlog/dualsense_libs_reference/daidr_dualsense_tester_review.md`**
   §2.3 (план) и §2.4 (что сделано по группам A/B/C/D + чистка
   абстракций). Пункт 8 плана — эта задача. В §2.4.1 — реестр
   deferred dangerous операций, **НЕ трогать ни одну из них**.

3. **`libDualsense/API.md`** — полный публичный API либы. Из него берёшь
   что тестировать.

4. **`libDualsense/README.md`** — "Supported features" / ⚠️ "Deferred
   dangerous" секции.

5. **`new_game/src/engine/platform/ds_bridge.h/.cpp`** — как игра уже
   использует либу. Для тестера возможно нужно расширить bridge либо
   добавить прямые вызовы libDualsense в debug-код.

6. **`new_game_devlog/debug_keys.md`** — система debug-клавиш в игре
   (`bangunsnotgames` режим). Тестер активируется через эту систему.

## Режим работы (memory `feedback_libdualsense_expansion`)

**Blanket-режим**: работай максимально крупными кусками, не спрашивай
разрешения на каждую мелочь. Стопайся ТОЛЬКО когда:

- большой chunk затянулся и имеет смысл протестить до продолжения;
- есть реальный риск регрессии существующего (rumble / lightbar /
  триггеры / полицейская мигалка);
- встретил что-то вне согласованного плана.

## Скоп тестера

In-game debug экран/панель, доступный в `bangunsnotgames` режиме через
hotkey. Показывает **живое** состояние всех фич libDualsense и
позволяет прокликать каждую output-фичу.

### Input visualization (read-only, live)

- Sticks L/R X/Y (raw 0..255 + normalized -1..+1)
- Triggers L/R (raw + normalized)
- Все кнопки: face (cross/circle/square/triangle), shoulders (L1/R1/L2/R2),
  d-pad, L3/R3, Options, Share, PS, touchpad click, mute.
- Touchpad fingers — X/Y координаты + down state для обоих.
- Battery level percent / charging / full flags.
- Headphone connected flag.
- Trigger feedback state nibble + effect-active bit (per trigger).
- Motion:
  - raw gyro (pitch/yaw/roll int16) + **калиброванные** deg/s через
    `ds_calibration.h`;
  - raw accel (X/Y/Z int16) + **калиброванные** g;
  - motion timestamp (uint32);
  - motion temperature (int8).
  - Калибровку получить через `get_sensor_calibration` один раз при
    открытии тестера, сохранить `SensorCalibration` в локальный state.

### Output control (interactive)

- Lightbar RGB (3 слайдера 0..255).
- Player LED mask (5 чекбоксов + preset кнопки: Off/Center/Inner/Outer/All).
- Player LED brightness (0..2 — High/Medium/Low).
- Rumble left/right (2 слайдера 0..255).
- Mute LED (3 кнопки: Off / On / Blink).
- Haptic volume (0..7 слайдер).
- Lightbar setup byte (hex input или 8 чекбоксов по битам).
- Speaker volume + Headphone volume + `audio_volumes_enabled` флаг.
- Adaptive trigger effects (для L и R отдельно) — все 12 режимов с
  настраиваемыми параметрами. Используй функции из `ds_trigger.h`:
  `trigger_off`, `trigger_feedback`, `trigger_simple_feedback`,
  `trigger_weapon`, `trigger_simple_weapon`, `trigger_vibration`,
  `trigger_simple_vibration`, `trigger_bow`, `trigger_galloping`,
  `trigger_machine`, `trigger_limited_feedback`, `trigger_limited_weapon`.

### Telemetry (read once, display)

- Firmware info (`get_firmware_info`): buildDate, buildTime, все FW
  versions (main/SBL/DSP/MCU), hwInfo, deviceInfo hex dump.
- PCBA ID (`get_pcba_id`) + full 24-byte PCBA (`get_pcba_id_full`).
- Serial number (`get_serial_number`) — 32 байта Shift-JIS. Если
  Shift-JIS decoder есть в игре — декодируй; если нет — hex dump.
- Battery barcode, VCM left/right barcodes (32 байт каждый).
- MCU Unique ID (`get_mcu_unique_id`) — 64-bit.
- BT MAC address (`get_bd_mac_address`) — 6 байт, показать как XX:XX:XX:XX:XX:XX.
- Battery voltage raw (`get_battery_voltage`) — raw bytes.
- Position tracking state / always-on-startup state / auto-switchoff
  flag (по одному байту каждый).

## Что НЕ делать

- DualSense Edge (paddles, fn-keys, профили) — нет железа.
- Audio haptic (группа F) — отложено.
- **Не расширять scope тестера за пределы существующих read-only
  функций libDualsense.** В либе **НЕТ** write/erase операций
  (calibration writes, eFuse, bootloader, NVS, factory writes, BT MAC
  writes, self-tests) — они в реестре §2.4.1 отмечены как deferred
  именно потому что не реализованы и могут окирпичить контроллер. Не
  используй generic-примитивы `test_command` / `device_send_feature_report`
  для вызова action ID из этого реестра, и не добавляй convenience
  wrappers для них. Задача тестера — **верифицировать уже
  имплементированное**, не расширять API.
- Не трогать оригинальный код в `original_game/`.

## Как рендерить

Используй существующие системы отрисовки игры:

- **Текст**: см. скилл `text-rendering` — доступные функции
  (`CONSOLE_text_at`, `FONT2D_DrawString`, etc.), когда какую применять,
  shadow-corruption gotchas.
- **2D примитивы** (прямоугольники, бары, слайдеры): см. скилл
  `hud-rendering`.

## Доступ к тестеру

Через систему debug-клавиш (`bangunsnotgames` режим). Подробности —
`new_game_devlog/debug_keys.md`. Если там нет готового способа —
добавь новый hotkey по документации скилла.

## Тестирование

После каждого крупного куска — smoke test на Windows (USB и BT).
Пользователь сам билдит и запускает: `make build-release` + `make r`.
Проси его протестить когда большой кусок готов.

Цель в конце: **все фичи libDualsense прокликаны руками на реальном
железе и работают**. Это финальная валидация паритета с
`daidr/dualsense-tester`.

## Правила записи результатов

- Все решения / находки / результаты тестов записывай в
  `daidr_dualsense_tester_review.md` (добавь новую подсекцию "§2.4
  — In-game tester" или следующим пунктом).
- НЕ создавай параллельные файлы про тестер — всё в review doc.
- Обновляй `libDualsense/README.md` если тестер выявит баги в либе.

## Правила скиллов

Если в процессе найдёшь что-то полезное для скилла (новая информация
про `text-rendering` / `hud-rendering` / debug-систему) — предложи
пользователю обновить скилл через плагин `skill-creator`. Не редактируй
скиллы напрямую через Write.
