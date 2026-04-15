# DualSense protocol reference

Справочник по работе с DualSense из OpenChaos. Sony официальной публичной
документации не выпускала — всё ниже это **результаты community
reverse-engineering**, проверенные кросс-источниками. Указано откуда
конкретный факт.

См. также:
- [`dualsense_adaptive_trigger_facts.md`](dualsense_adaptive_trigger_facts.md) — наши собственные эмпирические замеры
- [`weapon_haptic_and_adaptive_trigger.md`](../new_game_devlog/weapon_haptic_and_adaptive_trigger.md) — история отладки adaptive trigger в проекте
- Наша обёртка: [`new_game/src/engine/platform/ds_bridge.cpp`](../new_game/src/engine/platform/ds_bridge.cpp)
- Используемая либа: `new_game/libs/dualsense-multiplatform/`

## 1. Подключение и transport

DualSense поддерживает **USB** и **Bluetooth**.

- **USB**: полная функциональность — sticks, triggers, gyro, гаптика,
  адаптивные курки, динамик, микрофон, тачпад, lightbar.
- **Bluetooth**: всё то же, **но**: гаптика и audio-passthrough на
  3.5мм jack/встроенный спикер **не работают** на BT. Adaptive triggers
  через BT работают, но **зависит от реализации игры/драйвера** —
  некоторые игры/драйверы не справляются с BT-таймингами.
- **BT round-trip latency**: типично 7-30 мс HID туда-обратно.
- В нашем проекте `dualsense-multiplatform` абстрагирует transport,
  но это нужно учитывать для timing-чувствительных эффектов.

## 2. Input report (контроллер → PC)

### USB layout (Report ID 0x01, 64 bytes)

Байты приведены **относительно начала payload** (после Report ID).
Оффсеты могут отличаться на 1 если считать с Report ID byte.

| Offset | Размер | Поле |
|--------|--------|------|
| 0x00 | 1 | Left stick X (0..255, 128 = центр) |
| 0x01 | 1 | Left stick Y (инвертированный) |
| 0x02 | 1 | Right stick X |
| 0x03 | 1 | Right stick Y |
| 0x04 | 1 | **L2 trigger analog** (0..255) |
| 0x05 | 1 | **R2 trigger analog** (0..255) |
| 0x06 | 1 | Sequence counter |
| 0x07 | 1 | Buttons: dpad (low nibble) + face buttons (high nibble) |
| 0x08 | 1 | L1/R1/L2btn/R2btn/Create/Options/L3/R3 |
| 0x09 | 1 | PS/Touchpad/Mute/Edge buttons |
| 0x0A-0x0D | 4 | Timestamp |
| 0x0E-0x13 | 6 | Gyroscope (X, Y, Z, signed 16-bit) |
| 0x14-0x19 | 6 | Accelerometer (X, Y, Z, signed 16-bit) |
| 0x1A-0x1D | 4 | Sensor timestamp |
| 0x20-0x27 | 8 | Touchpad (2 fingers) |
| 0x29 | 1 | **TriggerRightStatus** (high nibble = state nybble) |
| 0x2A | 1 | **TriggerLeftStatus** |
| 0x34 | 1 | Battery level (low nibble = 0..10) |
| 0x35 | 1 | Headset/charging/mic flags |

⚠️ **Trigger feedback status байт** (USB offsets 41/42, BT 42/43 — после
strip Report ID; в нормализованном буфере либы — 41/42 для R2/L2):

- **bit 4 (act flag)** — `1` если курок находится в зоне активного
  эффекта (мотор взведён/работает), `0` если не в зоне. **Прямой
  сигнал что эффект физически работает прямо сейчас.**
- **bits 0-3 (state nybble)** — внутренний state machine моторчика.
  Изначально community-документация (Nielk1 gist) описывала это как
  3 значения: 0 = до зоны, 1 = внутри, 2 = после. **На реальном железе
  значения 4-9**, схема намного сложнее. См. эмпирическую таблицу в
  [`dualsense_adaptive_trigger_facts.md`](dualsense_adaptive_trigger_facts.md).

**Как использовать**: для определения факта щелчка используй **bit 4**:
переход `act: 1 → 0` одновременно с растущим r2 = щелчок физически
произошёл. Переход `act: 1 → 0` с падающим r2 = release без щелчка.

Поддерживается эффектами `Off` (0x05), `Feedback` (0x21), `Weapon`
(0x25), `Vibration` (0x26). Для других эффектов (`Bow`, `Galloping`)
поведение байта не задокументировано.

### Bluetooth layout

BT input report имеет другой Report ID и сдвинутые оффсеты, но та же
семантика. Раскладку нужно сверять отдельно если работаем с raw HID.
В нашем случае `dualsense-multiplatform` абстрагирует это.

## 3. Output report (PC → контроллер)

PC шлёт большой output report, содержащий:

- Lightbar RGB
- Player indicator LED
- Slow rumble (LRA voice-coil) и fast rumble (haptic)
- Левый и правый trigger effect: mode + parameter bytes
- Mute LED, audio settings, microphone control

Структура output report большая (~78 bytes). Каждый trigger effect
занимает блок ~10 байт: первый байт = mode code, остальные = параметры.
Точные оффсеты см. в `dualsense-multiplatform/.../GamepadTrigger.h`.

## 4. Adaptive trigger effect modes

Каждый эффект имеет уникальный mode code, кладётся в первый байт
trigger effect блока. Имена `<смысл><hex-код>` — community-конвенция,
не Sony.

| Mode | Имя (по гисту Nielk1) | Описание |
|------|-----------------------|----------|
| 0x00 | Off (legacy) | Нет эффекта (старый способ) |
| 0x01 | Simple_Feedback | Raw `position, strength` — плавное сопротивление |
| 0x02 | Simple_Weapon | Raw `startPosition, endPosition, strength` — щелчок |
| 0x05 | Off | Нет эффекта (рекомендованный) |
| 0x06 | Simple_Vibration | Raw `frequency, amplitude, position` |
| 0x11 | Limited_Feedback | Raw с ограничениями |
| 0x12 | Limited_Weapon | Raw с жёсткими ограничениями |
| 0x21 | **Feedback** | Linear resistance, bitmap-based per-zone control |
| 0x22 | Bow | Натяжение тетивы — зона сопротивления + snap force |
| 0x23 | Galloping | Имитация скачущей лошади |
| 0x25 | **Weapon** | Точка щелчка курка пистолета |
| 0x26 | **Vibration** | Вибрация триггера в зоне |
| 0x27 | Machine | Сложная вибрация с period/frequency |

Также есть "Multiple position" и "Slope" варианты эффектов 0x21/0x26
с массивом значений на 10 зон или линейной интерполяцией.

### 4.1 Статус реализации в `dualsense-multiplatform`

Библиотека вендорена в `new_game/libs/dualsense-multiplatform/` и имеет
**массовые проблемы с packing trigger эффектов**. Полный аудит с
деталями живёт в
[`dualsense_lib_pr_notes.md`](dualsense_lib_pr_notes.md) раздел 6.6.
Короткая сводка:

| Функция либы | Mode | Статус | Комментарий |
|--------------|------|--------|-------------|
| `Off` | 0x00 | ✅ ок | Просто сбрасывает режим |
| `Resistance` | 0x01 | ✅ ок | Совпадает с `Simple_Feedback` — raw packing, всё правильно |
| `GameCube` | 0x02 | ✅ ок | Хардкоженный preset Simple_Weapon, packing правильный |
| `Bow22` | 0x22 | ❌ broken | Не строит bitmap, пишет raw StartZone вместо правильного layout |
| `Galloping23` | 0x23 | ⚠️ частично | Bitmap корректный, но кодировка feet неправильная + integer div bug |
| `Weapon25` | 0x25 | ❌ broken | Функция сломана, у нас локальный фикс через OPENCHAOS-PATCH |
| `MachineGun26` | 0x26 | ❌ broken | Magic-числа вообще не соответствующие спеке Vibration |
| `Machine27` | 0x27 | ❌ broken | Нет endPosition параметра, нет bitmap, нет feature parity |
| `CustomTrigger` | 0xFF | ✅ ок | Raw bytes, ответственность на вызывающем |

**Отсутствуют совсем**: `Simple_Vibration (0x06)`, `Limited_Feedback (0x11)`,
`Limited_Weapon (0x12)`, **правильный** `Feedback (0x21)` (linear
resistance), правильный `Vibration (0x26)` (есть только сломанный
`MachineGun26`), и все `MultiplePosition*` / `SlopeFeedback` варианты.

**Практические следствия** для OpenChaos:
- `Weapon25` — работает только с нашим патчем.
- `Resistance (0x01)` / `GameCube (0x02)` — единственные не-патченые
  эффекты которыми можно пользоваться "как есть".
- Если захотим использовать **настоящий Feedback (0x21)** для тормоза
  в машине — надо добавить реализацию в либу. Сейчас там используется
  `Resistance (0x01)` что близко по смыслу но другой эффект.
- Если захотим **вибрацию триггера** для AK47 — `MachineGun26` в текущем
  виде непригоден, нужен правильный `Vibration (0x26)` packing.
- `Bow`, `Galloping`, `Machine27` — не работают, использовать нельзя.

### Weapon (0x25) — реальная спека параметров

Источник: [Nielk1 gist](https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db).

```
byte0 = effect mode (0x25)
byte1 = startPosition  (2..7)         — где начинается клик-зона
byte2 = endPosition    (start+1..8)   — где заканчивается клик-зона
byte3 = strength       (0..8)         — сила сопротивления/щелчка
```

Курок поделён на **8 зон** (zone 0 = полностью отпущен, zone 8 =
полностью выжат). Эффект работает только в `[startPosition, endPosition]`.
Курок свободный до startPosition, сопротивляется + клац внутри, свободный
после endPosition.

⚠️ Наша либа `dualsense-multiplatform` называет параметры
`StartZone, Amplitude, Behavior, Trigger` и пакует их немного иначе:
```
compose[0] = StartZone << 4 | (Amplitude & 0x0F)
compose[1] = Behavior
compose[2] = Trigger & 0x0F
```
Соответствие нашим именам vs. реальной спеке **не до конца понятно**.
Возможно `Amplitude` это `endPosition`, а `Behavior`/`Trigger` это
`strength` или их комбинация. Требует валидации экспериментом.

### MachineGun (0x26) — параметры

```
byte1 = startPosition
byte2 = behavior flag (0x03 если Behavior > 0)
...
byte4 = amplitude bits (зависит от Amplitude param)
byte5 = больше amplitude bits
byte9 = frequency
```
Эффект — непрерывная вибрация триггера в зоне. Хорош для авто-оружия.

### Feedback (0x21), Resistance — параметры

```
byte1 = startPosition (2..9)
byte2 = strength (0..8)
```
Плавное сопротивление от точки и до конца хода, без щелчка. Хорош для
тормоза в машине.

## 5. Гаптика (haptics)

DualSense имеет два voice-coil актуатора (LRA), а не классические ERM
моторы как у Xbox. Это даёт **широкополосную** вибрацию способную
следовать за audio-сигналом.

**Два режима гаптики:**

1. **Простой rumble** — два uint8_t значения (slow / fast motor), как
   у DualShock 4. Совместимо с обычными rumble API.
2. **Audio-haptic** — Sony эксклюзивный режим. PC шлёт **PCM аудио**
   которое напрямую идёт в voice-coils. Даёт настоящие тактильные
   текстуры (выстрелы, шаги, дождь). Требует Sony API/драйвер,
   **не работает через BT**, и community-либы плохо поддерживают.

В нашем проекте используется envelope-based подход: считаем RMS
огибающую WAV сэмпла оружия один раз, проигрываем как простой rumble.
Не настоящий audio-haptic, но даёт per-weapon "текстуру" без
сложностей с Sony API. Подробно — `weapon_haptic_and_adaptive_trigger.md`.

## 6. Что предоставляет наша либа `dualsense-multiplatform`

**Что есть:**
- Подключение USB и BT, абстракция transport
- Чтение sticks, triggers, buttons, gyro/accel, touchpad, battery
- Lightbar, player LED, mute LED
- Простой rumble (slow/fast)
- Все adaptive trigger режимы как методы (`SetWeapon25`,
  `SetMachineGun26`, `SetResistance`, `SetBow22`, `SetGalloping23`,
  `SetFeedback21`, `SetVibration26`, `TriggerOff`)
- Touchpad multi-touch

**Чего НЕТ в апстриме:**
- ❌ Audio-haptic mode (PCM в voice-coils) — не реализован.
- ❌ Точная документация что значат параметры `Behavior`, `Trigger` в
  `SetWeapon25` — комментарии в коде расплывчатые.

**Локальные патчи в нашей вендоренной копии:**
- ✅ **Trigger feedback status** теперь читается из input report
  (правки в `GamepadInput.h::DualSenseRaw` + `InputContext.h`). Всю
  механику патча, обоснование, тесты, и инструкции для подготовки PR
  в апстрим см. в [`dualsense_lib_pr_notes.md`](dualsense_lib_pr_notes.md).

## 7. Полезные ссылки

- [Game Controller Collective Wiki — Sony DualSense](https://controllers.fandom.com/wiki/Sony_DualSense) — community wiki про DualSense. Содержит секции Input Reports, Output Reports, Feature Reports, и Data Structures с C++11 определениями (enum Direction, PowerState, TriggerLeftStatus/TriggerRightStatus и т.п.). WebFetch к fandom возвращает 403, поэтому открывать нужно в браузере.
- [nondebug/dualsense GitHub](https://github.com/nondebug/dualsense) — детальная информация о HID layout
- [Nielk1 trigger effect factories gist](https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db) — реальная спека параметров всех trigger effects. Главный источник истины по packing байт для каждого эффекта.
- [Nielk1 gist — raw](https://gist.githubusercontent.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db/raw) — raw текст того же файла, иногда нужен вместо HTML версии если WebFetch возвращает 403
- [DualSenseX/DSX UDP API example](https://github.com/Paliverse/DualSenseX) — коммерческий популярный PC-тул для DualSense (1.5k stars, продолжение на Steam как DSX). Сам закрытый, но публичный UDP API example подтверждает parameter ranges из Nielk1 спеки для Weapon (0x25) и других эффектов. Полезен как независимый источник истины про семантику параметров trigger-эффектов.
- [`Ohjurot/DualSense-Windows`](https://github.com/Ohjurot/DualSense-Windows) — 400⭐ open-source C++ library (DS5W). Независимо подтверждает feedback offsets 0x29/0x2A (в `DS5_Input.cpp`). Output packing ограничен 3 эффектами: `ContinuousResistance (0x01)` raw, `SectionResistance (0x02)` raw, `EffectEx (0x26)` — нестандартный layout с 3 forces (begin/middle/end) который отличается от Nielk1 bitmap-based варианта того же mode. Интересно как свидетельство что mode 0x26 может поддерживать несколько рабочих packing'ов.
- [SensePost DualSense Reverse Engineering blog](https://sensepost.com/blog/2020/dualsense-reverse-engineering/) — original RE pioneering work
- [DualSenseX](https://github.com/Paliverse/DualSenseX) — open-source PC tool, можно подсмотреть реализации
- [dogtopus DualSense descriptor gist](https://gist.github.com/dogtopus/894da226d73afb3bdd195df41b3a26aa) — HID descriptor

## 8. Quick reference: типичные задачи

**"Хочу добавить новый adaptive trigger эффект для оружия"**
→ Изменить `WeaponFeelProfile` для нужного `SPECIAL_*` в
   `weapon_feel_init()`. Параметры `trigger_start_zone` и
   `trigger_amplitude` напрямую идут в `ds_trigger_weapon()`.
   Для совсем других эффектов (вибрация на очереди и т.п.) — добавить
   новый mode в `gamepad.cpp::TriggerMode` и `apply_trigger_mode()`.

**"Хочу узнать был ли физический щелчок"**
→ Сейчас невозможно, либа не читает trigger feedback nybble. Варианты:
  (а) патчить либу чтобы читала offsets 0x29/0x2A
  (б) наша эмпирическая калибровка через `weapon_feel_test` SPACE-sampling

**"Хочу более длинную вибрацию на выстрел"**
→ Тюнить `WeaponFeelProfile.haptic_max_seconds` и `haptic_gain`/`ceiling`.
   Помни: должно быть короче cooldown между шотами очереди иначе сольётся
   в buzz.

**"Хочу настроить тормоз в машине"**
→ `apply_trigger_mode(TRIGGER_MODE_CAR)` в `gamepad.cpp` использует
   `ds_trigger_resistance()` (это Feedback 0x21). Параметры — startPosition
   и strength.
