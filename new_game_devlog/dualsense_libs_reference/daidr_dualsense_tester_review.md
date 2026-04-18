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

## Этап 2 — паритет фич (TBD после этапа 1)

Пока не начат. Когда закроем баг с non-Win USB — перехожу сюда.

Предварительный список фич daidr которые надо сравнить с libDualsense:
- Гироскоп / акселерометр (у нас в "Not supported")
- Mute LED полный контроль (у нас "Not supported")
- Audio haptics (у нас "Not supported")
- Speaker volume, headphone volume, mic volume (у нас нет в `OutputState`)
- Haptic volume (у нас нет)
- Sensor calibration (у нас "Not supported")
- DualSense Edge-specific (paddles, Fn keys) (у нас "Not supported")
- Test commands / telemetry (PCBA ID, serial, BD MAC, battery barcode,
  VCM barcode — у них это сделано через feature report 0x80/0x81,
  у нас не реализовано)
- lightbarSetup (у нас не выставляется)
- powerSaveMuteControl (у нас нет)

Каждая — отдельный подробный ask пользователю перед добавлением (см.
memory feedback_libdualsense_expansion).
