# libDualsense vs daidr/dualsense-tester — review

Рабочий документ ревью libDualsense против референсной реализации
`daidr/dualsense-tester` (Web HID, TypeScript). Репа склонирована в
`libs/dualsense-tester/` (под gitignore).

## Зачем это всё

1. **Диагностика бага** "non-Windows + DualSense USB: вибрация почти не
   работает" (см. `new_game_planning/known_issues_and_bugs.md`).
   На ds.daidr.me вибра работает на **всех** OS+USB, у нас — только Win.
   Значит есть объективная разница в wire-байтах.
2. **Паритет фич.** Сайт daidr тестирует гироскоп, mute LED, calibration
   и т.д., которых в libDualsense ещё нет.

---

## Этап 1 — расследование non-Win USB rumble бага

### 1.1 Wire-формат USB output у daidr

Файлы: [`src/utils/dualsense/ds.util.ts`](libs/dualsense-tester/src/utils/dualsense/ds.util.ts), [`src/router/DualSense/views/_OutputPanel/outputStruct.ts`](libs/dualsense-tester/src/router/DualSense/views/_OutputPanel/outputStruct.ts).

**Отправка (`sendOutputReportFactory`, USB ветка):**
```ts
await device.sendReport(0x02, new Uint8Array(data))  // data = 47 bytes
```
Web HID API автоматически добавляет Report ID 0x02 как первый байт,
итого в transport идёт **48 байт**: `[0x02] + 47_bytes_data`.

**47-байтный data (порядок из `OutputStruct.sort`):**

| Offset | Поле                     | Дефолт   |
|--------|--------------------------|----------|
| 0      | validFlag0               | 0        |
| 1      | validFlag1               | **0xF7** |
| 2      | bcVibrationRight         | 0        |
| 3      | bcVibrationLeft          | 0        |
| 4      | headphoneVolume          | 0        |
| 5      | speakerVolume            | 0        |
| 6      | micVolume                | 0        |
| 7      | audioControl             | 0        |
| 8      | muteLedControl           | 0        |
| 9      | powerSaveMuteControl     | 0        |
| 10     | adaptiveTriggerRightMode | 0        |
| 11-20  | adaptiveTriggerRightParam0..9 | 0   |
| 21     | adaptiveTriggerLeftMode  | 0        |
| 22-31  | adaptiveTriggerLeftParam0..9 | 0    |
| 32-35  | Reserved0..3             | 0        |
| 36     | hapticVolume             | 0        |
| 37     | audioControl2            | 0        |
| 38     | validFlag2               | 0        |
| 39-40  | Reserved7..8             | 0        |
| 41     | lightbarSetup            | 0        |
| 42     | ledBrightness            | 0        |
| 43     | playerIndicator          | 0        |
| 44-46  | ledCRed/Green/Blue       | 0/255/0  |

**`validFlag0` — селективная bitmap-маска** (биты выставляются только
когда соответствующее поле реально меняется):
- bit 0 | bit 1: правый/левый rumble
- bit 2 | bit 3: правый/левый trigger effect
- bit 4: headphone vol, bit 5: speaker vol, bit 7: audio control
- итд (из [`OutputPanel.vue`](libs/dualsense-tester/src/router/DualSense/views/OutputPanel.vue): `setValidFlag0(0..7)`)

**`validFlag1` дефолт 0xF7** — маска "что имеет право на запись в
подсистему LED/mic/lightbar". Конкретные биты выставляются/гасятся в
`OutputPanel.vue` по контексту.

**`validFlag2` дефолт 0** — биты ledBrightness (0) и т.п.

### 1.2 Wire-формат USB output у libDualsense

Файл: [`libDualsense/ds_output.cpp`](../../libDualsense/ds_output.cpp).

**Размер wire-буфера:** `OUTPUT_REPORT_USB_BYTES = 74 bytes`.
- `buf[0] = 0x02` — Report ID
- `buf[1]..buf[73]` — payload (73 байта)

**Payload (offset относительно `buf+1`):**

| Offset | Поле                  | Значение                |
|--------|-----------------------|-------------------------|
| 0      | VIBRATION_MODE_DEFAULT| **0xFF** (всегда)       |
| 1      | FEATURE_MODE_DEFAULT  | **0x57** (всегда)       |
| 2      | rumble_right          | state.rumble_right      |
| 3      | rumble_left           | state.rumble_left       |
| 10-19  | trigger_right[10]     | state.trigger_right[]   |
| 21-30  | trigger_left[10]      | state.trigger_left[]    |
| 39     | HapticLowPassFilter   | **0x07** (всегда)       |
| 42     | player_led_brightness | state.player_led_brightness |
| 43     | player_led_mask       | state.player_led_mask   |
| 44-46  | lightbar RGB          | state.lightbar_r/g/b    |
| 47-72  | Reserved              | 0 (memset)              |

### 1.3 Ключевые различия (USB)

| Параметр          | daidr                        | libDualsense                |
|-------------------|------------------------------|-----------------------------|
| **Wire size**     | **48 байт** (1+47)           | **74 байта** (1+73)         |
| validFlag0        | selective bitmask            | constant 0xFF               |
| validFlag1        | 0xF7 default + selective     | constant 0x57               |
| Байт 39           | 0 (Reserved8)                | **0x07** (HapticLowPassFilter) |
| Хвост             | отсутствует (47 полей всего) | 26 лишних нулевых байт      |

### 1.4 Гипотезы по багу

**Главная гипотеза — размер отчёта (48 vs 74).**
Комментарий в [`ds_device.cpp:24-28`](../../libDualsense/ds_device.cpp#L24-L28) признаётся:
> USB report is 48 bytes by struct definition, but empirically the
> controller may interpret short reports differently (e.g. trigger
> motor power defaults). Sending 74 bytes ... avoids this.

На **Windows** SDL hidapi, возможно, отправляет 74 байта, а контроллер
либо стрипит хвост, либо терпит. На **macOS/Linux** SDL hidapi
передаёт 74 байта в USB урл как есть, контроллер читает "лишние" 26
нулевых байт и интерпретирует их как поля, которые сбрасывают
конфигурацию вибромоторов в ноль (или в дефолт). На BT этой проблемы
нет потому что там спец-слот 77 байт с чёткой CRC.

**Второстепенные гипотезы:**
1. Статичные `validFlag0 = 0xFF` / `validFlag1 = 0x57` → контроллер
   нервничает от выставленного bit 3 validFlag0 или bits в validFlag1.
   Но на Windows работает → скорее не это.
2. `payload[39] = 0x07` → теоретически HapticLowPassFilter конфликтует
   с чем-то на non-Win. Но 1 байт в 74-байтном бандле — слабая гипотеза.
3. SDL hidapi на non-Win мог бы менять размер `SDL_hid_write`. Нужно
   перепроверить через `write()` → `libusb_interrupt_transfer()` путь.

**Первый шаг фикса:**
Попробовать уменьшить `OUTPUT_REPORT_USB_BYTES` с 74 до 48 (исправить
на спек-каноничное значение из daidr). Если на Windows после этого
остаётся работать — баг снят.

**Риск:** комментарий в ds_device.cpp намекает что 48 байт может
сломать Windows (например "trigger motor power defaults"). Если так —
нужно раздельное поведение per-OS (что криво).

**Запасной план:** если 48 ломает Windows → искать байт который
daidr не ставит, но который нужен контроллеру. Сравнить side-by-side
wire dump с ds.daidr.me (Wireshark на HID USB).

### 1.6 Применён фикс — USB 74 → 48 байт

**Правки:**
- `libDualsense/ds_device.cpp`: `OUTPUT_REPORT_USB_BYTES = 48` (было 74),
  комментарий переписан под новое (спек-каноничное) значение.
- `libDualsense/ds_output.cpp`: `wire_len = bluetooth ? 78u : 48u`
  (было `: 74u`).

Никакие другие параметры не трогались — пробуем **минимальное**
изменение, чтобы при откате понимать что именно повлияло. Константы
`VIBRATION_MODE_DEFAULT=0xFF` / `FEATURE_MODE_DEFAULT=0x57` / `payload[39]=0x07`
оставлены как были.

**Что проверять (3 сценария × 2 транспорта = 6 случаев):**

| OS           | USB                             | Bluetooth                      |
|--------------|---------------------------------|--------------------------------|
| Windows      | **должно по-прежнему работать** (регрессия?) | должно работать            |
| macOS        | **должно заработать** (основной фикс) | должно работать           |
| Steam Deck   | **должно заработать** (основной фикс) | должно работать           |

Если Windows USB сломался → откатить, искать другую причину.
Если non-Win USB заработал, Windows не сломался → баг закрыт одной
строкой, обновить `known_issues_and_bugs.md`.

**Результаты тестирования (2026-04-18):**
- **macOS USB — ✅ ПОЧИНИЛОСЬ.** Вибра работает, все остальные функции
  (триггеры, LED, парсинг инпута) — как раньше, регрессий нет.
  Пользователь дополнительно откатился на версию ДО фикса и
  воспроизвёл проблему — подтвердило что именно эти два изменения
  (wire size 74 → 48) починили.
- **Windows USB — ✅ регрессий нет.** Все функции работают как раньше.
- **Windows Bluetooth — ✅ регрессий нет.** Все функции работают как раньше.
- **Steam Deck USB** — не тестировался отдельно, но поведение у Mac
  и Deck в исходной проблеме было 1:1 → с высокой вероятностью тоже
  починилось. Подтвердить при следующей возможности.
- **macOS/Deck BT** — не тестировалось, код BT-ветки не менялся.

**Статус:** баг закрыт, запись перемещена в
[`known_issues_and_bugs_resolved.md`](../../new_game_planning/known_issues_and_bugs_resolved.md).

### 1.5 Дополнительные отличия для справки

**BT output (у нас работает, у daidr работает):**
- daidr: `[Report ID 0x31, data[0] = seq<<4, data[1] = 0x10, data[2..48] = 47 payload bytes, CRC в последних 4 байтах]`. Sequence counter инкрементирует, 0..255.
- libDualsense: `buf[0] = 0x31, buf[1] = 0x02 (sub-ID, константа), buf[2..73] = payload, buf[74..77] = CRC32`. Sequence counter **не** реализован; вместо него на offset 40 тоглится 1 бит (`s_bt_seq ^= 0x01`).

  ⚠️ У нас нет rotating sequence number. daidr честно инкрементирует 0..255. Потенциально может вызывать дропы пакетов на BT если контроллер их дедуплицирует. Но вибра на BT у нас работает → скорее всего это не критично, а просто не по спеке.

**CRC32:** обе реализации дают одинаковый результат, записаны по-разному.
- daidr: стандартный CRC32 (полином 0xEDB88320), init 0xFFFFFFFF, prefix `[0xA2, 0x31]`, финальный XOR с 0xFFFFFFFF.
- libDualsense: pre-computed seed `0xeada2d49` (= CRC state после
  обработки префикса `[0xA2]` начиная с init 0xFFFFFFFF), таблица с
  уже вложенным финальным XOR (первый элемент `0xd202ef8d`, а не
  0x00000000). Эквивалентно математически, но зашифровано плотнее.
  Байт 0x31 у нас попадает в основной цикл как `buf[0]`.

---

## Этап 2 — паритет фич

### 2.1 Полный диф по input/output структурам

**Источники:**
- daidr: [`src/utils/dualsense/ds.type.ts`](libs/dualsense-tester/src/utils/dualsense/ds.type.ts), [`src/router/DualSense/views/_OutputPanel/outputStruct.ts`](libs/dualsense-tester/src/router/DualSense/views/_OutputPanel/outputStruct.ts)
- libDualsense: [`ds_input.h`](../../libDualsense/ds_input.h), [`ds_output.h`](../../libDualsense/ds_output.h)

#### Input (парсинг входящих отчётов)

| Поле / группа                          | daidr | libDualsense | Группа |
|----------------------------------------|-------|--------------|--------|
| Sticks L/R (XY, 0..255)                | ✅    | ✅           | —      |
| Triggers analog (L2/R2, 0..255)        | ✅    | ✅           | —      |
| Face buttons / D-pad / shoulders       | ✅    | ✅           | —      |
| L3/R3, Options, Share, PS, Touchpad    | ✅    | ✅           | —      |
| Mic mute button                        | ✅    | ✅           | —      |
| Touchpad fingers (x2)                  | ✅    | ✅           | —      |
| Battery level / charging / full        | ✅    | ✅           | —      |
| Headphone connected flag               | ✅    | ✅           | —      |
| Trigger feedback status (per trigger)  | ✅    | ✅           | —      |
| **Gyroscope** (pitch/yaw/roll int16)   | ✅    | ❌           | **A**  |
| **Accelerometer** (X/Y/Z int16)        | ✅    | ❌           | **A**  |
| **Motion timestamp** (uint32)          | ✅    | ❌           | **A**  |
| **Motion temperature** (int8)          | ✅    | ❌           | **A**  |
| Trigger level raw (per trigger)        | ✅    | ❌           | B      |
| Host timestamp (BT only)               | ✅    | ❌           | B      |
| Device timestamp                       | ✅    | ❌           | B      |
| Status0/1/2 — полные биты              | ✅ полные | частично (battery/headphone/mute) | B |
| **DSE paddles** (bL/bR)                | ✅    | ❌           | **E**  |
| **DSE function keys** (fnL/fnR)        | ✅    | ❌           | **E**  |
| DSE active profile byte                | ✅    | ❌           | E      |
| AES CMAC / seq tag (BT security)       | ✅    | ❌           | —      |
| CRC32 of input report (BT check)       | ❌    | ❌           | —      |

#### Output (сборка исходящих отчётов)

| Поле                                   | daidr        | libDualsense      | Группа |
|----------------------------------------|--------------|-------------------|--------|
| Rumble left / right                    | ✅           | ✅                | —      |
| Adaptive trigger L/R (10 params)       | ✅           | ✅                | —      |
| Player LED mask                        | ✅           | ✅                | —      |
| Player LED brightness                  | ✅           | ✅                | —      |
| Lightbar RGB                           | ✅           | ✅                | —      |
| validFlag0/1/2 — селективные биты      | ✅           | ⚠️ constants      | (см. ниже) |
| BT output seq counter (rotating 0..255)| ✅           | ⚠️ xor-toggle     | (см. ниже) |
| **Mute LED control** (3 режима)        | ✅           | ❌                | **B**  |
| **Haptic volume**                      | ✅           | ❌                | **B**  |
| **Lightbar setup byte**                | ✅           | ❌                | B      |
| **Speaker volume**                     | ✅           | ❌                | **C**  |
| **Headphone volume**                   | ✅           | ❌                | C      |
| **Mic volume**                         | ✅           | ❌                | C      |
| **Audio control + audio control 2**    | ✅           | ❌                | C      |
| **Power save mute control**            | ✅           | ❌                | C      |

**Про validFlag и seq counter:** оба не ломают текущую функциональность
(после фикса USB size), но отклоняются от спеки.

- `validFlag0 = 0xFF, validFlag1 = 0x57` (константы) — у daidr это
  **селективные маски**, биты выставляются только когда поле реально
  меняется. Часть фич группы B/C потребует явного управления битами
  (например `clearValidFlag1(3)` для brightness), что у нас сейчас
  "повезло работает" потому что 0x57 удачно не выставляет бит 3.

- "BT seq counter" у нас — при перечитывании [`ds_output.cpp:37-43`](../../libDualsense/ds_output.cpp#L37-L43)
  выяснилось что это вообще **не** seq counter: `s_bt_seq ^= 0x01` на
  `buf[40]` тоглит бит 0 `validFlag2` (= `AllowLightBrightnessChange`),
  не sequence. Настоящий BT seq counter (0..255 rotator в первом байте
  framing перед payload) у нас **нигде не шлётся**. Контроллер терпит,
  но это не по спеке. Переименовать/переписать вместе с валидфлагами.

**Решение (2026-04-18, user):** менять не сейчас, а **вместе с добавлением
output-фич группы B/C** — одним заходом перевести всё на селективные
`validFlag*` и настоящий BT seq counter. Причины: (1) только что
закрыли баг, не хочется снова трогать wire-формат без необходимости;
(2) когда добавим mute LED / volumes, всё равно придётся выставлять
конкретные биты validFlag — дешевле один раз перейти, чем два раза
переписывать. Тестирование — smoke-test на всех 4 комбинациях
(Win/Mac × USB/BT), проверять rumble + trigger + LED + lightbar.
Если что-то отвалится — Wireshark (USBPcap) для побайтового дампа.

#### Feature reports / telemetry (у daidr огромный набор)

| Группа feature reports             | Что даёт                            | Группа |
|------------------------------------|-------------------------------------|--------|
| Sensor calibration (gyro/accel)    | точная калибровка IMU для игр       | **D**  |
| Firmware info (build date, ver.)   | диагностика                         | D      |
| Battery voltage precise            | точный charging status              | D      |
| MCU unique ID, BD MAC, Serial      | телеметрия устройства               | D      |
| PCBA ID, VCM barcode, battery bc   | фабричные идентификаторы            | D      |
| Position tracking enable/disable   | motion tracking feature             | D      |
| Audio/mic calibration tests        | заводское тестирование              | —      |
| Bootloader / NVS / DSP FW commands | прошивка (зона риска)               | —      |

#### Прочее

- **Audio haptics (PCM → voice coil)** — sub-область, у daidr используется
  через specific test commands. Sony-недокументированная, требует
  отдельного расследования. Группа **F**.
- **Lightbar color presets (DualSenseColorMap)** — UI helper (мапит
  серию контроллера к цвету), не фича либы.

### 2.2 Группировка фич по приоритету/сложности

- **Группа A — Motion (input, просто парсинг):** gyro, accel, motion
  timestamp, motion temperature. Hardware всегда шлёт, надо лишь
  прочитать байты и выставить в `InputState`.
- **Группа B — мелкие output-фичи:** mute LED, haptic volume, lightbar
  setup. Один байт или enum в output report.
- **Группа C — аудио подсистема:** speaker/headphone/mic volume, audio
  control flags, power-save mute. Связные фичи, стоит делать вместе.
- **Группа D — feature reports / telemetry / calibration:** sensor
  calibration (полезно для gyro), firmware info, battery voltage,
  telemetry.
- **Группа E — DualSense Edge:** paddles, Fn-keys, profile mode,
  профиль-зависимые настройки.
- **Группа F — Audio haptics:** Sony-специфично, сложно, вне scope
  для минимальной либы.

Порядок добавления — от A к F. Внутри группы — по одной фиче с явным
вопросом пользователю (memory `feedback_libdualsense_expansion`).

**Решения по группам (2026-04-18, user):**
- **Группа A, B, C, D** — делаем.
- **Группа E (DualSense Edge)** — **пропускаем** сейчас (у пользователя
  нет железа DSE, приоритета нет).
- **Группа F (Audio haptics)** — **пересмотреть после групп A-D.**
  Связана с нашей игровой задачей "аудио → вибрация" (см.
  [`dualsense_audio_haptic_investigation.md`](../dualsense_audio_haptic_investigation.md)),
  где мы ранее заключили "true audio-haptic пути нет, post-1.0".
  Но раз daidr это делает (функция `controlWaveOut` в
  [`ds.util.ts:582-600`](libs/dualsense-tester/src/utils/dualsense/ds.util.ts)
  + подразумеваемый wave-output pipeline), путь всё-таки
  существует. При переходе к F нужно залезть в их UI (router/DualSense/views/)
  и перечитать pipeline от PCM до отправки на motor. Возможно
  вывод про "post-1.0" был преждевременным.

### 2.3 План работы (согласован 2026-04-18)

Последовательность:

1. **Группа A — Motion (input):** gyro, accel, motion timestamp, motion
   temperature. Простой парсинг байтов. Каждая фича — отдельный ask
   пользователю.
2. **Группа B — мелкий output:** mute LED, haptic volume, lightbar setup.
   ⚠️ При первой output-фиче этой группы — **одновременно** перевести
   всю либу на селективные `validFlag0/1/2` + настоящий BT seq counter
   (см. §2.1 "Про validFlag и seq counter"). Не делать отдельным заходом.
3. **Группа C — аудио подсистема:** speaker/headphone/mic volume, audio
   control, power-save mute. Связный блок.
4. **Группа D — feature reports / telemetry / calibration:** sensor
   calibration (важно для точного gyro!), firmware info, battery
   voltage, unique IDs.
5. **Группа E — DualSense Edge:** ⏸ **отложено** (не отменено). Причина —
   у пользователя нет железа DSE для тестирования, приоритета сейчас
   нет. Вернёмся после F. Цель либы — полное покрытие, Edge входит в
   него, просто не сейчас.
6. **Группа F — Audio haptics:** **АУДИТ ПРОВЕДЁН 2026-04-18,
   результаты ниже.**

   **Результаты аудита daidr audio-кода:**
   - Единственная audio-функция которой у нас нет — `controlWaveOut`
     ([`ds.util.ts:582-600`](libs/dualsense-tester/src/utils/dualsense/ds.util.ts#L582)).
     Это **1kHz тестовый синус** через встроенный динамик или 3.5mm
     jack. UI — две кнопки "hold to play" ([`AudioControlWidget.vue`](libs/dualsense-tester/src/router/DualSense/views/AudioControlWidget.vue)).
   - Wire: два test command'a через уже-имплементированный
     `test_command()` примитив:
     - Optional pre: `AUDIO / BUILTIN_MIC_CALIB_DATA_VERIFY` (action 4),
       20-байтные params (routing `[_, _, _, _, 4, _, 6]` для headphone,
       `[_, _, 8]` для speaker).
     - Core: `AUDIO / WAVEOUT_CTRL` (action 2), 3-байтные params
       `[enable, 1, 0]`.
   - Это **НЕ audio haptic.** Это hardware self-test — способ услышать
     1kHz sine из динамика контроллера чтобы проверить что audio
     подсистема работает. К PCM→voice-coil pipeline отношения не имеет.
   - **daidr ВООБЩЕ не реализует true audio haptic** — ни PCM→voice-coil,
     ни envelope→rumble. Единственное "haptic"-поле у них — `hapticVolume`
     (0..7 глобальный gain рамбла), который мы **уже имплементили в
     группе B**.

   **Пересмотр старого вывода "post-1.0" из
   [`../dualsense_audio_haptic_investigation.md`](../dualsense_audio_haptic_investigation.md)
   (2026-04-15):**
   - Вывод "true audio haptic требует 2-14 дней и это post-1.0"
     **ОСТАЁТСЯ ВЕРНЫМ.** daidr эту историю не закрывает — они просто
     её не делают. Т.е. true audio haptic **не входит в "паритет с
     daidr"**.

   **Revised scope группы F:**
   - **F1 — `controlWaveOut` (1kHz test tone).** Тривиально поверх
     `test_command()` из группы D, ~20 строк helper'а в ds_test или
     новом ds_audio_test. **Статус (2026-04-18):** пользователь решил
     **НЕ имплементить сейчас** — test-tone штука чисто диагностическая,
     игре не нужна, добавим если понадобится при in-game тестере.
     Отложено.
   - **F2 — true Sony-style audio haptic (PCM→voice coil).** Остаётся
     **post-1.0**, вне scope паритета с daidr. Детали путей — в
     старом investigation doc. Если вернёмся — отдельной задачей
     (путь 1 = miniaudio через USB-audio, путь 2 = HID chunks 0x32
     с магическими 64 байтами).

   **Следующий шаг плана:** переход к пункту 7 (чистка утечек
   абстракций). Группа F фактически закрыта "по умолчанию"
   имплементированного + явно отложенного.
7. **Чистка утечек абстракций в libDualsense.** Когда все фичи перенесены
   (A + B + C + D + F), **перед** включением в наш debug layer — пройтись
   по либе и убрать всё что требует от пользователя знать внутренние
   детали протокола. Цель: чтобы либа была удобной для использования
   извне, не заставляла потребителя разбираться в CRC-байтах, layout'е
   output-report'а, BT framing, и магических флагах.
   - Известный кейс на сегодня: init-packet для BT живёт в
     [`new_game/src/engine/platform/ds_bridge.cpp`](../../new_game/src/engine/platform/ds_bridge.cpp)
     (пользователь должен вручную строить пакет с CRC). Перенести в
     либу — автоматически слать при `device_open_first()` на BT, либо
     публичной функцией `device_send_init_output()`.
   - Возможно ещё кейсы — ревизовать весь публичный API: если для
     использования какого-то сетаппа требуется магия в коде потребителя
     — вынести её внутрь либы.
8. **Финальный шаг — verification tester.** Когда всё выше сделано
   (A + B + C + D + F + чистка абстракций) — сделать собственный in-game
   тестер (debug-панель или standalone экран) который прокликивает
   **все фичи libDualsense**: показывает gyro/accel графики, позволяет
   триггернуть каждый adaptive trigger mode, поднять каждый mute LED
   state, крутить каждую volume, послать PCM тест на audio-haptics, и т.д.
   Цель — убедиться что libDualsense действительно покрывает всё что
   умеет daidr, на нашей стороне (SDL hidapi) и на всех OS/транспортах.

**Правила на всё время работы (из memory `feedback_libdualsense_expansion`):**
- Перед каждой новой фичей — явный вопрос пользователю с подробным
  описанием (что делает, HID-байты, параметры, риски, где в коде).
- Не бандлить несколько фич в один ask.
- Документировать в `libDualsense/README.md` + `API.md` сразу при
  добавлении.
- Все заметки — только в этот файл (§2.4 ниже).

### 2.4 Статус добавления фич

- **2026-04-18 — Группа A добавлена.** Gyroscope (pitch/yaw/roll),
  accelerometer (X/Y/Z), motion timestamp, motion temperature. Поля
  добавлены в [`ds_input.h`](../../libDualsense/ds_input.h) `InputState`,
  парсинг в [`ds_input.cpp::parse_input_report`](../../libDualsense/ds_input.cpp)
  (little-endian int16 / uint32 / int8). Offsets взяты из daidr
  [`offset.util.ts`](libs/dualsense-tester/src/router/DualSense/_utils/offset.util.ts)
  — 0x0F/0x11/0x13 (gyro), 0x15/0x17/0x19 (accel), 0x1B (timestamp),
  0x1F (temperature). Все значения **raw device units** без
  калибровки (калибровка требует feature report 0x05 — задача
  группы D). Документация — [`README.md`](../../libDualsense/README.md)
  + [`API.md`](../../libDualsense/API.md). **Проверено:** всё работает.

- **2026-04-18 — Группа B добавлена + большой BT framing рефакторинг.**

  **Контекст происхождения старого BT формата:** комментарий в
  бывшем `ds_output.cpp` прямо указывал что значения унаследованы от
  `rafaelvaloto/Dualsense-Multiplatform`, которая в свою очередь
  построена на Windows-ориентированных референсах (DS5W и др.).
  DualSense HID protocol не имеет официальной спеки от Sony — всё
  это community reverse-engineering (gcc-wiki, Nielk1, nondebug,
  DS5W, duaLib, daidr). Windows-стек HID (WinAPI) толерантно
  дополняет/переформатирует output, поэтому "неточный" layout
  работал на Windows. На Mac/Linux через SDL hidapi (ближе к сырой
  передаче) тот же layout давал сбои — в случае USB это привело к
  багу с вибрацией (только что закрыт через 74 → 48 байт), в BT
  случайно работало потому что контроллер толерантен именно к
  именно тем отклонениям что у нас были. **Мораль:** daidr — это
  authoritative кросс-платформенный референс (проверенный в
  браузере на всех OS). Переходим полностью на его layout.

  **Новые поля в `OutputState`:**
  - `mute_led` (enum `MuteLed::Off / On / Blink`) — мик-мьют LED.
  - `haptic_volume` (0..7) — общий уровень рамбла поверх per-motor.
  - `lightbar_setup` — байт-override для lightbar.

  **Рефакторинг wire-формата:**
  - **USB:** `[0x02][47-byte payload]` = 48 байт. Уже было после
    прошлого фикса, не меняется.
  - **BT:** `[0x31][seq<<4][0x10 magic][47-byte payload][24 reserved][4 CRC]` = 78 байт.
    Раньше было `[0x31][0x02][47-byte payload][reserved][CRC]` — без
    magic, без настоящего seq counter (было xor-тогл на validFlag2
    бит 0), payload начинался на 1 байт раньше.
  - **Payload (47 байт)** — идентичен для USB и BT. Структура из
    daidr `OutputStruct` (см. шапку `ds_output.cpp`): validFlag0/1,
    rumble L/R, audio/mic volumes (0 — не wired up), mute LED,
    power-save mute (0), trigger slots, haptic volume, validFlag2,
    lightbar setup, player LED brightness/mask, lightbar RGB.
  - **validFlag0/1/2** теперь **per-bit селективные** через
    `namespace ValidFlag0/1/2::{RumbleRight, MicMute, LightbarRGB, ...}`.
    Значения выставляются только для полей которые libDualsense
    реально пишет. Идентично даидру за вычетом пока-не-реализованных
    audio-фич (группа C).
  - **BT CRC** — без изменений, формула та же (0xA2 prefix baked
    into `ds_crc` table).

  **Init-packet в ds_bridge.cpp** переписан под новый framing:
  validFlag0/1/2 = 0xFF на init'е (all-on), потом обычный пакет с
  селективными флагами применяет конкретные LED/lightbar/triggers.

  **Что критично протестировать (⚠️ ломающее изменение BT-формата):**
  Все 6 комбинаций OS × transport:

  | OS           | USB                          | Bluetooth                   |
  |--------------|------------------------------|-----------------------------|
  | Windows      | проверить: rumble, triggers, LED, lightbar | **критично** — проверить то же |
  | macOS        | проверить то же              | **критично** — проверить то же |
  | Steam Deck   | проверить то же              | **критично** — проверить то же |

  Если BT где-то отвалится — сравнить дампы через USBPcap/Wireshark
  с ds.daidr.me. Если USB где-то отвалится — значит правки в common
  payload layout неточны (это менее вероятно, т.к. USB не изменился,
  только добавились новые поля и per-bit флаги).

  **Результаты тестирования:**
  - **Первая итерация (2026-04-18)** — поломала lightbar, player LED
    и BT rumble. Причина: в моём коде layout'а был сдвиг на 2 байта
    в хвосте payload: `Reserved0..3` записал на `p[30..33]` вместо
    правильного `p[32..35]`. Все последующие поля (hapticVolume,
    validFlag2, lightbar setup, brightness, playerIndicator, RGB)
    съехали вперёд на 2 байта.
    - Lightbar "всегда красный" — `lightbar_r` попадал в p[42]
      (реально brightness), `lightbar_b` — в p[44] (реально
      ledCRed). Зелёный и синий компонент уходили в никуда.
    - Вибра слабее на BT — `validFlag2` (=0x03) попадал в p[36],
      а это реально `hapticVolume`. Значение 3 из 7 уменьшало
      общую громкость почти вдвое.
    - Player LED включён всегда — `lightbar_g` писался в p[43]
      (реально playerIndicator), и любое G ≠ 0 зажигало LED.
    - Исправлено тем же днём. Также синхронизирован init-packet
      в `ds_bridge.cpp` (validFlag2 на `buf[3 + 38]` вместо `+36`).
  - **Вторая итерация (2026-04-18)** — **Windows USB + BT: ✅ всё работает.**
    Lightbar, player LED (mask + brightness), вибрация обычной силы,
    триггеры, мигалка полицейской машины (сине-красная).
  - **macOS USB + BT (2026-04-18) — ✅ всё работает.** Обе транспорта
    подтверждены пользователем. Steam Deck не проверялся отдельно
    (запускать там геморно), но т.к. macOS и Deck используют один и
    тот же SDL hidapi код-путь и исходный USB rumble баг воспроизводился
    на обеих платформах 1:1 — с очень высокой вероятностью Deck тоже
    работает. Подтвердить при ближайшем удобном случае, не блокер.

- **2026-04-18 — Группа C (частично) добавлена: speaker + headphone volume.**

  **Scope пересмотрен:** из 5 фич группы C реально делаем 2 —
  `speaker_volume` и `headphone_volume`. Оставшиеся 3 (`mic_volume`,
  `audio_control_2`, `power_save_mute`) **отложены**: их даже daidr не
  выводит в UI (поля присутствуют в `OutputStruct` но ни один slider
  в `OutputPanel.vue` их не использует). Без живого референса нет
  способа проверить что нужные биты / значения. Вернёмся к ним либо
  по сигналу из группы D (feature reports могут раскрыть назначение),
  либо по ходу группы F (audio-haptic может задействовать
  `audio_control_2`).

  **API дизайн:** mutually-exclusive routing между speaker и headphone —
  **артефакт daidr UI** (каждый слайдер-drag переписывает audioControl
  nibble), не свойство хардвара. Поля в OutputStruct независимые, оба
  всегда валидны. Мы у себя храним оба независимо и всегда шлём оба
  volume в payload.

  **Opt-in флаг:** добавил `OutputState::audio_volumes_enabled = false`
  по умолчанию. Когда `false` — либа **не** выставляет validFlag0
  биты 4/5/7 и не пишет payload[4..7]; аудио-состояние контроллера
  остаётся нетронутым (OS / предыдущее приложение). Когда `true` —
  `speaker_volume` и `headphone_volume` пишутся в p[4]/p[5], audioControl
  в p[7] = 0x00 (headphone route по умолчанию — контроллер сам разрулит
  по jack-detect), и биты validFlag0 {4,5,7} выставляются.

  **audioControl nibble захардкожен на 0x00.** daidr переключает 0x00/0x30
  в зависимости от последнего drag'a — это UI-семантика, у нас в
  streaming-либе одного источника "какой slider последним трогали"
  нет. Фиксируем 0x00 (headphone route), controller сам выбирает
  физический выход по jack-detect. Если на тестах окажется что роутинг
  влияет — добавить явный enum `AudioRoute` в OutputState.

  **Места изменений:**
  - [`ds_output.h`](../../libDualsense/ds_output.h) — 3 новых поля в OutputState
  - [`ds_output.cpp`](../../libDualsense/ds_output.cpp) — wire-up p[4]/p[5]/p[7]
    + conditional validFlag0 bits 4/5/7
  - [`API.md`](../../libDualsense/API.md) — таблица полей + параграф
    про opt-in семантику
  - [`README.md`](../../libDualsense/README.md) — supported features
    обновлён; в "Not supported" оставлены mic volume / power-save mute /
    audio_control_2 с пояснением почему

  **Что тестировать (Windows USB/BT приоритет, Mac опционально):**
  - `audio_volumes_enabled = false` + все существующие фичи (rumble,
    триггеры, LED, lightbar) → регрессий быть не должно.
  - `audio_volumes_enabled = true`, `speaker_volume = 200`,
    `headphone_volume = 0` → звук из контроллера (без наушников).
  - `audio_volumes_enabled = true`, `speaker_volume = 0`,
    `headphone_volume = 200` → звук из 3.5mm jack (наушники
    подключены, без них — пусто).
  - Промежуточные значения и динамические изменения — плавное изменение
    громкости на слайдере.

  **Как протестировать без вживления в игру:** в debug layer (клавиша
  или панель в `ds_bridge.cpp` / отладочный экран) добавить временный
  hotkey который ставит `output.audio_volumes_enabled = true` и крутит
  volume. Или запустить ds.daidr.me параллельно на другом устройстве
  для side-by-side (нельзя — devices эксклюзивны, только одно за раз).
  Альтернативы: проверить что при `audio_volumes_enabled = true` игра
  не крашится и не ломаются остальные фичи — минимальная verification
  того что wire-layout корректен; громкость как таковую можно оставить
  на финальный in-game тестер (пункт 8 плана).

  **Результаты тестирования (2026-04-18):**
  - **Windows USB + BT — ✅ регрессий нет.** Все существующие фичи
    (rumble, триггеры, lightbar, player LED, полицейская мигалка)
    работают как раньше. Audio-волюм как таковой не проверялся —
    в игре не задействован, `audio_volumes_enabled = false` по
    умолчанию.
  - macOS и Steam Deck — не тестировались. Т.к. default-путь
    (`audio_volumes_enabled = false`) не меняет wire-формат
    относительно пост-групповой-B baseline, регрессий на других
    OS быть не должно. Audio-функциональность как таковая —
    финальный in-game тестер (пункт 8).

- **2026-04-18 — Группа D (read-only часть) добавлена.**

  **Scope:** все read-only feature reports и test-command геттеры из
  daidr. Write/erase операции **отложены** (см. подсекцию
  "Deferred dangerous operations" ниже).

  **Новые модули в libDualsense:**
  - [`ds_crc.cpp/h`](../../libDualsense/ds_crc.cpp) — расширен вторым
    вариантом CRC32 (`crc32_compute_feature_report`). Feature-report
    CRC использует префикс `[0x53, reportId]` vs output-report CRC с
    `[0xA2, reportId]`. Существующая baked-seed реализация была
    специфична для output; фьютуры требуют стандартный CRC32
    (полная таблица 0xEDB88320, init/final 0xFFFFFFFF). Обе реализации
    сосуществуют, старый API не тронут.
  - [`ds_device.cpp/h`](../../libDualsense/ds_device.cpp) — добавлены
    `device_get_feature_report` / `device_send_feature_report` —
    тонкие обёртки над `SDL_hid_get/send_feature_report`. Ошибки
    не закрывают device handle (feature reports могут быть
    unsupported на отдельных firmware версиях, но input/output
    остаются рабочими).
  - [`ds_feature.cpp/h`](../../libDualsense/ds_feature.cpp) — прямые
    feature report парсеры:
    - **Firmware info (0x20)** — 60+ байт: buildDate (11 ASCII),
      buildTime (8 ASCII), fwType, swSeries, hwInfo, mainFwVersion,
      deviceInfo (12 bytes), updateVersion, updateImageInfo,
      sblFwVersion, dspFwVersion, spiderDspFwVersion. Layout из
      daidr `FactoryInfo.vue::getFirmwareInfo`.
    - **Sensor calibration (0x05)** — 35+ байт: gyro bias + plus/minus
      points, gyro speed reference, accel plus/minus per axis. Layout
      из DS5W / nondebug / hid-playstation (все референсы совпадают).
    - **BT patch info (0x22)** — uint32 на offset 31.
  - [`ds_test.cpp/h`](../../libDualsense/ds_test.cpp) — handshake
    0x80 write / 0x81 poll + чанкинг (56 байт на чанк, статусы
    COMPLETE/COMPLETE_2 из daidr TestStatus enum). Публичный
    generic-примитив `test_command(...)` и удобные геттеры:
    `get_mcu_unique_id`, `get_bd_mac_address`, `get_pcba_id`,
    `get_pcba_id_full`, `get_serial_number`, `get_assemble_parts_info`,
    `get_battery_barcode`, `get_vcm_{left,right}_barcode`,
    `get_battery_voltage`, `get_position_tracking_state`,
    `get_always_on_startup_state`, `get_auto_switchoff_flag`.
    BT-ветка CRC32 считает через feature-report CRC
    (`crc32_compute_feature_report`). USB без CRC (HID handles it).
  - [`ds_calibration.cpp/h`](../../libDualsense/ds_calibration.cpp) —
    helpers применяющие SensorCalibration к raw int16 из InputState:
    - Accel → g: `bias = (plus + minus) / 2`, `range = (plus - minus) / 2`,
      `g = (raw - bias) / range`. Математически точно (Sony
      специфицирует plus/minus как ±1 g точки).
    - Gyro → deg/s: `speed_2x = speed_plus + speed_minus`,
      `range = axis_plus - axis_minus`, `deg/s = (raw - bias) * speed_2x / range`.
      Формула из Linux `hid-playstation` kernel driver, community-verified
      через DS5W / nondebug / DS4Windows.

  **Изменения в CMakeLists.txt:** добавлены `ds_feature.cpp`,
  `ds_test.cpp`, `ds_calibration.cpp` в список источников
  библиотеки.

  **Документация обновлена:** README (Architecture, Files, Supported
  features, ⚠️ Deferred dangerous commands), API.md (новые секции 5-8
  про feature reports, test commands, IMU calibration).

  **Риски которые надо проверить при билде/тесте:**
  - CRC32 для BT feature reports — **отдельная** реализация, не
    пересекается с output CRC. Если ошибка в расчёте → контроллер
    молча отклонит 0x80 request на BT → test commands на BT не
    вернут данные. USB будет работать сразу (без CRC).
  - Размеры feature report'ов захардкожены по конвенции DS5W /
    nondebug (0x80 = 64 USB / 68 BT, 0x81 same). Если SDL hidapi
    на какой-то платформе требует иного размера — feature calls
    могут вернуть -1 или пустой отчёт. SDL обычно толерантен.
  - Gyro calibration формула — формулу взял из hid-playstation
    Linux driver; если в deg/s чиселки окажутся кривые (слишком
    большие/маленькие в разы), будет понятно, можно будет
    скорректировать scaling constant.
  - Существующий код (rumble, триггеры, LED, lightbar) — **не
    тронут**. Новые модули это чистые дополнения. Регрессий в
    output не должно быть.

  **Smoke-test (Windows USB приоритет):**
  1. Билд компилируется, новые модули линкуются.
  2. Существующий output (rumble/LED/lightbar/triggers) работает как
     раньше — регрессий нет.
  3. (Опционально, в новом debug-hotkey или разовом print) — вызвать
     `get_firmware_info`, проверить buildDate выглядит как дата;
     `get_sensor_calibration` → valid=true; `get_mcu_unique_id` →
     ненулевое значение. Это верифицирует что feature reports и
     test commands реально работают.

  **BT тест:** опционально. Если не сразу — не блокер, main game
  path (output reports + input) от этого не зависит.

  **Результаты тестирования (2026-04-18):**
  - **Windows USB + BT — ✅ регрессий нет.** Сборка проходит, все
    существующие фичи (rumble, триггеры, lightbar, player LED,
    полицейская мигалка) работают как раньше. Новые API feature
    reports / test commands пока не вызываются из игры (это чистые
    lib-API дополнения) — реальная верификация их работы отложена
    на финальный in-game тестер (пункт 8 плана).
  - macOS / Steam Deck — не тестировались. Default-путь (без вызовов
    новых функций) идентичен baseline'у группы C, регрессий быть не
    должно. Feature reports / test commands верифицируем на in-game
    тестере.

### 2.4.1 Deferred dangerous operations (реестр)

**⚠️ НЕ ИМПЛЕМЕНТИРОВАТЬ без выделенного исследования рисков
восстановления и обязательного согласия пользователя.**

Все операции ниже могут **необратимо** испортить или окирпичить
контроллер. Hard-reset кнопкой их почти наверняка не восстановит
(калибровка/eFuse/factory data лежат во flash/OTP, не в RAM).

**Calibration:**
- `WRITE_RAW_CALIBRATION_DATA` (deviceId=STICK, actionId=1)
- `ERASE_CALIBRATION_DATA` (deviceId=STICK, actionId=3)
- `WRITE_CALIBRATION_COEFFICIENT` (deviceId=STICK, actionId=64)
- `SET_VALID_CALIBRATION_METHOD` (deviceId=STICK, actionId=66)
- `WRITE_CALIB_DATA` (deviceId=ADAPTIVE_TRIGGER, actionId=33)
- `ERASE_CALIB_DATA` (deviceId=ADAPTIVE_TRIGGER, actionId=35)
- `WRITE_CALIB_DATA_DEV` (49), `ERASE_CALIB_DATA_DEV` (51) — per-device variants

**Factory identifiers (irreversible overwrite):**
- `WRITE_FACTORY_DATA` (SYSTEM, 7)
- `WRITE_HWVERSION` (SYSTEM, 5)
- `WRITE_PCBAID` (SYSTEM, 3), `WRITE_PCBAID_FULL` (SYSTEM, 16)
- `WRITE_TRACABILITY_INFO` (ADAPTIVE_TRIGGER, 36)
- `ERASE_TRACABILITY_INFO` (ADAPTIVE_TRIGGER, 38)
- `WRITE_TRACABILITY_INFO_DEV` (52), `ERASE_TRACABILITY_INFO_DEV` (54)

**eFuse (PERMANENT — physical fuse bits):**
- `WRITE_EFUSE` (SYSTEM, 9)

**Bluetooth identity:**
- `WRITE_BDADR` (BLUETOOTH, 1) — ломает system-wide BT pairings
- `WRITE_INIT` (BLUETOOTH, 1 — overloaded with WRITE_RAW_CALIBRATION_DATA)
- `SET_BT_ENABLE` (BLUETOOTH, 5) — runtime toggle, менее опасно но может disconnect

**Bootloader (BRICK risk):**
- `TEST_ACTION_BOOTLOADER_ENTER` (SYSTEM, 112)
- `TEST_ACTION_BOOTLOADER_LEAVE` (SYSTEM, 113)
- `TEST_ACTION_BOOTLOADER_FORMAT` (SYSTEM, 114)
- `TEST_ACTION_BOOTLOADER_WRITE` (SYSTEM, 115)
- `TEST_ACTION_BOOTLOADER_FLUSH` (SYSTEM, 116)

**NVS (non-volatile storage):**
- `NVS_LOCK` (MEMORY, 1)
- `NVS_UNLOCK` (MEMORY, 2)
- `NVS_GET_STATUS` (MEMORY, 3)
- `SAVE_CONFIG` (SYSTEM, 10)
- `ERASE_CONFIG` (SYSTEM, 11)

**Codec registers (can break audio subsystem):**
- `CTRL_CODEC_REG_WRITE` (AUDIO, 128)
- `CTRL_CODEC_REG_READ` (AUDIO, 129) — read, но trigger'ит state-machine

**Invasive self-tests (могут менять state):**
- `SOLOMON_SELF_TEST` (BLUETOOTH, 1 — overloaded)
- `CYPRESS_SELF_TEST` (ADAPTIVE_TRIGGER, 38)
- `AGING_STATE` (SYSTEM, 15)
- `AGING_INIT` (SYSTEM, 16)

**Если когда-нибудь будем имплементить:**
1. Сначала — отдельное исследование "как восстановить после ошибки"
   для каждой операции (тест на не жалко-контроллере).
2. Формально API-дизайн с `dangerous_` префиксом и compile-time
   gate (`#define LIBDUALSENSE_ENABLE_DANGEROUS_OPS 1`).
3. Логирование каждого вызова в debug build с stack trace.
4. Явное подтверждение пользователем (не просто "есть опция", а
   двойное подтверждение в API-вызове).
5. Желательно — sanity check что команда адресована recovery-controller'у
   (sentinel check), не production user-owned.

- **2026-04-18 — Пункт 7 (чистка утечек абстракций) выполнен.**

  **Найденные утечки в `ds_bridge.cpp`:**
  1. **BT init-packet** — `ds_poll_registry` руками строил весь wire-пакет
     (0x31 report ID, seq<<4, magic 0x10, validFlag'и на offset'ах 0/1/38,
     CRC32 на buf[74..77]). Потребитель знал layout payload'а и CRC
     wiring.
  2. **Input report stripping** — `ds_update_input` делал
     `s_input_raw + s_device.input_report_strip` перед `parse_input_report`.
  3. **Output wire size** — `ds_update_output` жонглировал переменными
     `bt` и `dev->output_report_size` вокруг `build_output_report` +
     `device_write`.

  **Решение — two-tier API:**
  - Добавлены 3 high-level функции в libDualsense:
    - [`device_send_init_packet`](../../libDualsense/ds_device.h) — BT
      handshake инкапсулирован, no-op на USB (возвращает `true`).
    - [`device_read_input`](../../libDualsense/ds_input.h) — drains
      queue + strips transport framing + parses в одной операции.
    - [`device_send_output`](../../libDualsense/ds_output.h) — builds
      + writes в одной операции.
  - Low-level примитивы (`device_read_latest`, `device_write`,
    `parse_input_report`, `build_output_report`) **оставлены** для
    power user'ов которым нужен доступ к wire-формату.

  **Результат в `ds_bridge.cpp`:**
  - `ds_poll_registry`: ~30 строк wire-магии → одна строка
    `device_send_init_packet(&s_device)`.
  - `ds_update_input`: manual strip + parse → `device_read_input`.
  - `ds_update_output`: manual build + write + size gymnastic → `device_send_output`.
  - Удалены: `#include <libDualsense/ds_crc.h>` (CRC теперь внутри
    либы), `s_input_raw[96]` static buffer (live в `device_read_input`).

  **API.md обновлён** — two-tier API документирован: high-level как
  рекомендуемый путь, low-level как power-user option. Пример "Typical
  integration" переписан с использованием high-level.

  **Что проверять:**
  - Билд компилируется — SDL hidapi header include chain OK
    (`ds_device.cpp` теперь включает `ds_crc.h`; `ds_input.cpp` и
    `ds_output.cpp` включают `ds_device.h`).
  - Windows USB + BT работают как раньше — все output-фичи (rumble,
    триггеры, lightbar, player LED, полицейская мигалка). Поведение
    не должно измениться: high-level функции делают ровно то же что
    раньше делал inline-код в bridge, просто в другом месте.

  **Что НЕ делал (оставлено как есть):**
  - Поля `Device::input_report_strip` и `Device::output_report_size`
    остались публичными — они нужны low-level пользователям. Для
    high-level пользователей они прозрачны.
  - `device_open_first` **НЕ** авто-вызывает init packet — оставил
    как отдельный явный шаг. Прозрачный вариант рассматривал, но
    решил что явный вызов честнее: видно что на BT что-то происходит
    при открытии.

  **Результаты тестирования (2026-04-18):**
  - **Windows USB + BT — ✅ регрессий нет.** Базовый smoke-test
    пройден, все существующие фичи работают идентично baseline'у.
    Поведение byte-identical (high-level функции делают то же что
    inline-код раньше, просто в другом месте).
  - macOS / Steam Deck — не тестировались, но т.к. wire-format
    не изменился (изменилось только кто его строит), регрессий быть
    не должно.

### 2.5 Side-findings (по ходу работы, не блокеры)

- **Trigger slot — 10 vs 11 байт.** У нас `OutputState::trigger_right[10]`
  покрывает `Mode + Param0..8` (10 байт). По daidr layout трусс slot =
  11 байт (`Mode + Param0..9`). Последний байт (Param9) у нас всегда
  остаётся 0 после `memset`, но у daidr он может нести данные для
  некоторых эффектов. Текущие эффекты (`trigger_weapon`, `trigger_machine`,
  `trigger_feedback`, etc.) используют ≤ 6 параметров — Param9 не
  задействуется, проблем нет. Если позже добавим экзотический эффект
  который требует Param9 — расширить `trigger_*[10]` до `[11]` и
  обновить все функции в [`ds_trigger.cpp`](../../libDualsense/ds_trigger.cpp).
  Пока — backward compatible.

- **rafaelvaloto-pristine/ папка пустая.** Есть только директории
  `Source/Public/GCore`, `Source/Private/GCore` и т.п. без файлов.
  Похоже `git clone` не прошёл до конца при последнем скачивании.
  Не критично — мы с этой либы слезли, она нужна только как
  исторический референс. При необходимости перескачать.

### 2.6 Гипотезы / открытые вопросы

- **Haptic volume и lightbar setup — биты validFlag.** Для `haptic_volume`
  (payload[34]) и `lightbar_setup` (payload[39]) точные биты validFlag
  подобраны по догадке: `ValidFlag2::LightbarSetup` (bit 1) и
  `ValidFlag2::ImprovedRumble` (bit 2, если будет нужен). В daidr
  `OutputPanel.vue` нет setter'ов для этих полей — значит их UI не
  использует, и биты не проверены на практике. Если фичи не работают
  — экспериментально подобрать через USBPcap-дамп работающего канала.
