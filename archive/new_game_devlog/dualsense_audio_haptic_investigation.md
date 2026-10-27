# DualSense audio-to-haptic — расследование GamepadCore (2026-04-15)

## Контекст

В [stage12.md](../new_game_planning/stage12.md) п.4 и [stage13_dualsense.md](../new_game_planning/stage13_dualsense.md) (Audio-to-haptic B7) заявлено:
"API: `AudioHapticUpdate()` через miniaudio (уже включён в библиотеку), конвертация
игровых звуков в тактильную обратную связь".

При попытке реально пойти этим путём (вибрация DualSense от звука выстрела M16)
выяснилось что **"чистого" Sony SDK-style audio-to-haptic у нас в GamepadCore
сегодня нет** — оба имеющихся пути нерабочие по разным причинам.

## Что есть в GamepadCore

Интерфейс [`IGamepadAudioHaptics`](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Public/GCore/Interfaces/Segregations/IGamepadAudioHaptics.h)
объявляет две перегрузки `AudioHapticUpdate`:

```cpp
virtual void AudioHapticUpdate(const std::vector<std::uint8_t>& AudioData) = 0;
virtual void AudioHapticUpdate(const std::vector<std::int16_t>& AudioData) = 0;
```

Они реализованы в [`FDualSenseLibrary`](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Private/GImplementations/Libraries/DualSense/DualSenseLibrary.cpp)
(строки 303 и 319) и представляют собой **два совершенно разных механизма**.

---

## Путь 1 — `AudioHapticUpdate(vector<int16_t>)` — через miniaudio

### Что делает

`FDualSenseLibrary::AudioHapticUpdate(int16_t)` проверяет
`Context->AudioContext->IsValid()` и вызывает `AudioContext->WriteHapticData(samples)`.
`AudioContext` — это `shared_ptr<FAudioDeviceContext>`, где `FAudioDeviceContext`
([AudioContext.h](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Public/GCore/Types/Structs/Context/AudioContext.h))
открывает **отдельное miniaudio playback-устройство** и стримит туда интерлив-стерео
float32 сэмплы через ring buffer.

Ключевой момент: это **не хаптика в моторы контроллера**. Это просто второй
аудиоканал — miniaudio открывает обычный OS audio device (WASAPI / CoreAudio /
ALSA), и сэмплы играют на том железе, которое miniaudio выбрал как устройство.

### Зачем это может работать как "audio-haptic"

DualSense подключённый **по USB-кабелю** появляется в системе как **USB Audio
Class-устройство** — помимо HID-игрового. Т.е. через OS audio stack можно играть
звук прямо в встроенный динамик DualSense (тот что у тачпада). В теории если
нацелить miniaudio именно на это устройство, PCM-стрим пойдёт в DualSense и будет
воспроизводиться на встроенной динамике/тачпаде.

Это и есть то что Sony DualSense называет "audio haptics" — использование
динамика/пьезо внутри контроллера как тактильного отклика. Ощущается буквально
как "выстрел в руке" потому что играется настоящий звук выстрела.

### Почему не работает у нас сейчас

1. **`AudioContext` нигде не инициализируется.** В [DeviceContext.h:58](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Public/GCore/Types/Structs/Context/DeviceContext.h#L58) он объявлен как
   `std::shared_ptr<FAudioDeviceContext>` и остаётся nullptr. Проверка
   `!Context->AudioContext` в `AudioHapticUpdate(int16_t)` сразу возвращает.
   GamepadCore ожидает что пользователь сам создаст `AudioContext` и вызовет
   `InitializeWithDeviceId`, но этого нигде нет.

2. **Хук `InitializeAudioDevice` в нашей политике — заглушка.**
   [ds_bridge.cpp:178](../new_game/src/engine/platform/ds_bridge.cpp#L178)
   (`No-op for now — audio-to-haptic is step B7, deferred`).

3. **Если просто инициализировать с `device_id=nullptr`, miniaudio откроет
   системный дефолтный playback-девайс** = основные колонки пользователя. Т.е.
   каждый выстрел в игре будет играть вторым каналом поверх основного микса
   через MFX/OpenAL. Катастрофа.

4. **Чтобы нацелиться именно на DualSense-audio**, нужно:
   - Перечислить playback-устройства через `ma_context_enumerate_devices`.
   - Найти устройство где имя/producer содержит "DualSense" / "Wireless
     Controller" (название зависит от платформы и драйвера).
   - Получить его `ma_device_id` и передать в `InitializeWithDeviceId`.
   - Это разное на Windows / macOS / Linux и хрупкое (разные драйверные имена).

5. **Только USB-кабель.** DualSense **по Bluetooth не экспонирует USB-audio** —
   только HID. Т.е. этот путь в принципе не может работать на беспроводном
   подключении. Вообще.

6. **Задержка и синхронизация.** Miniaudio играет через OS audio stack со своей
   латентностью (10-30ms в лучшем случае), не синхронизированной с OpenAL/MFX
   который играет основной звук. Даже если заработает — будут рассинхроны
   между основным звуком выстрела и "хаптическим" каналом в DualSense.

### Вывод по пути 1

**Технически возможно, но не готово.** Требует 2-3 полных дня работы:
инициализация AudioContext, enumeration + matching device ID по платформам,
подключение к MFX/OpenAL для получения PCM, обработка рассинхронов. И
**принципиально не работает по Bluetooth**, что ограничивает ~половину
пользователей.

---

## Путь 2 — `AudioHapticUpdate(vector<uint8_t>)` — 64-байтовые HID-чанки

### Что делает

[`FDualSenseLibrary::AudioHapticUpdate(uint8_t)`](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Private/GImplementations/Libraries/DualSense/DualSenseLibrary.cpp#L303)
копирует **ровно 64 байта** пользовательских данных в `Context->BufferAudio[13..77]`
и вызывает `FGamepadOutput::SendAudioHapticAdvanced(Context)`.

В `BufferAudio` перед этим уже записаны магические байты через
[DualSenseLibrary.cpp:111-120](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Private/GImplementations/Libraries/DualSense/DualSenseLibrary.cpp#L111):

```
BufferAudio[0] = 0x32;     // Output report ID (Bluetooth)
BufferAudio[1] = 0x00;
BufferAudio[2] = 0x91;
BufferAudio[3] = 0x07;
BufferAudio[4] = 0xFE;
BufferAudio[5..8] = 55 55 55 55;
BufferAudio[9]  = 0xFF;
BufferAudio[10] = sequence++;
BufferAudio[11] = 0x92;
BufferAudio[12] = 0x40;
BufferAudio[13..76] = <64 user bytes>
// CRC32 at [138..141]
```

Это выглядит как **закрытый DualSense HID output-report для "advanced audio
haptics"**. Магические байты (0x91 0x07 0xFE, потом 0x92 0x40) намекают на
какой-то подфичу аудио-хаптики в прошивке DualSense, но что именно означают
эти 64 байта — **не документировано** ни в коде GamepadCore, ни в открытых
реверс-инжиниринговых репозиториях которые я знаю.

Возможные варианты формата (не проверено):
- Raw 8-bit signed PCM (64 сэмпла = ~1.3ms при 48кГц — слишком мало).
- 64 сэмпла при 8 кГц = 8ms, один "тик" хаптики.
- ADPCM или другой компрессированный формат.
- Вектор огибающей частотных полос (feature vector).

### Что работает в транспорте

[`SendAudioHapticAdvanced`](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Private/GImplementations/Utils/GamepadOutput.cpp#L246)
собирает CRC32 и вызывает `IPlatformHardwareInfo::Get().ProcessAudioHaptic(ctx)`.
В нашем SDL-HID bridge это [ds_bridge.cpp:165 `process_audio_haptic`](../new_game/src/engine/platform/ds_bridge.cpp#L165):

```cpp
if (ctx->ConnectionType == EDSDeviceConnection::Bluetooth) {
    int written = SDL_hid_write(dev, ctx->BufferAudio, 147);
    if (written < 0) {
        SDL_hid_write(dev, ctx->BufferAudio, 78);
    }
}
```

Т.е. **это реально отправляет HID output report в DualSense**, причём **только по
Bluetooth** (на USB функция ничего не делает — явный gate и в GamepadCore, и в
нашем bridge).

### Почему не работает у нас сейчас

1. **Не проброшено наружу.** В [ds_bridge.h](../new_game/src/engine/platform/ds_bridge.h)
   нет публичной функции которая бы выставляла этот путь игровому коду. Значит
   — нужно добавить.

2. **Формат 64 байт неизвестен.** Даже если пробросить API — непонятно что туда
   класть. Если положить raw PCM — DualSense, скорее всего, проиграет мусор
   или не проиграет ничего. Потребуется экспериментальный реверс:
   - Пробовать 8-bit signed/unsigned PCM при разных частотах.
   - Пробовать амплитудные envelope'ы.
   - Сравнивать с тем что делает PS5 SDK (но он закрыт).

3. **Только Bluetooth.** Этот путь в принципе не активирован на USB, потому что
   на USB DualSense использует другой HID output report (0x02). Чтобы это
   заработало на кабеле, нужно дополнительно поддержать USB-вариант — ещё
   отдельная работа по реверсу.

4. **Риск побить устройство / прошивку.** Отправка неправильных output reports
   в DualSense не должна его сломать физически, но может привести к тому что
   контроллер "залипнет" до перезагрузки. Экспериментировать стоит осторожно.

### Вывод по пути 2

**Bluetooth-only, формат неизвестен, это не готовый API.** Для использования
требует реверс-инжиниринга DualSense HID-протокола — **1-2 недели работы** с
риском что не заработает вообще или будет глюкавить.

---

## Итоговый вывод

**"True audio-to-haptic" через Sony-style API в GamepadCore у нас сегодня не
работает ни по одному из заявленных путей.** То что написано в
[stage13_dualsense.md#audio-to-haptic-b7](../new_game_planning/stage13_dualsense.md):
"API уже включён в библиотеку" — **фактически неверно**. Интерфейс объявлен и
частично реализован, но вся инфраструктура (инициализация, выбор устройства,
формат данных) либо отсутствует, либо не документирована.

**Оценка реального объёма работы для полноценного audio-haptic:**
- **Путь 1 (miniaudio через USB-audio)**: 2-3 дня, работает только на USB,
  кросс-платформенные грабли с device matching.
- **Путь 2 (HID chunks по Bluetooth)**: 1-2 недели реверса, без гарантии что
  заработает.
- **Оба вместе (чтобы покрыть и USB и BT)**: 2+ недель.

Это существенно больше чем оценивалось в stage13_dualsense.md ("самая сложная
фича"). Строку "перенесено в 1.0" можно считать оптимистичной — реалистично
это post-1.0.

## Практический путь для 1.0 — envelope → rumble motors

Вместо true audio-haptic использовать **огибающую реального игрового звука**
(WAV-сэмпл `S_AK47_BURST` и т.п.) как программу для **обычных рамбл-моторов
DualSense** через уже работающий `ds_set_vibration(left, right)`.

Почему это близко к "audio-haptic" по ощущениям:
- Моторы DualSense — это **voice-coil актуаторы**, гораздо более быстрые и
  точные чем Xbox ERM-моторы. Они способны воспроизводить амплитудную
  модуляцию до ~200Hz без проблем.
- Драйв огибающей настоящего выстрела (а не синтетического паттерна) даёт
  тактильный "баБах" с правильной атакой/спадом.
- Работает **и по USB, и по Bluetooth**, на всех платформах, без реверса
  протоколов.
- Реализация: загрузить WAV → посчитать огибающую (RMS по ~5ms окнам) → хранить
  как массив → при выстреле стримить в моторы тик за тиком.

Это **не то что Sony называет audio-haptic**, но это **единственный способ
получить "вибрацию от реального звука оружия" сегодня без недель реверса**.
Результат ожидается "достаточно хороший", финального sony-уровня хаптики в
1.0 не будет.

---

## Что дальше

Для 1.0 — идём путём envelope → моторы для M16 (и потом для дробовика и
пистолета). Audio-haptic через Sony API — post-1.0, отдельной задачей.
