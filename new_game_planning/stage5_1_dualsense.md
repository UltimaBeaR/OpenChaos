# Итерация B: DualSense-специфика (Dualsense-Multiplatform)

Подробный план интеграции библиотеки Dualsense-Multiplatform для поддержки
расширенных фич DualSense контроллера. Ссылка из [stage5_1.md](stage5_1.md).

## Библиотека

**Репо:** `libs/dualsense-multiplatform/Dualsense-Multiplatform/` (vendored шаг A0)
**Язык:** C++20, static library
**CMake target:** `GamepadCore`
**Платформы:** Windows, Linux, macOS
**Лицензия:** MIT

### Структура

```
Dualsense-Multiplatform/
  CMakeLists.txt              — top-level, включает Source/ и опционально Libs/miniaudio
  Source/
    CMakeLists.txt            — собирает GamepadCore static library
    Public/                   — публичные хедеры (API)
      GCore/
        Interfaces/           — ISonyGamepad, IGamepadStatus, IGamepadLightbar, etc.
        Templates/            — TBasicDeviceRegistry (device discovery)
        Types/                — ECoreGamepad enums, InputContext, DeviceContext
      GImplementations/
        Libraries/DualSense/  — FDualSenseLibrary implementation
        Utils/                — GamepadAudio helper
    Private/                  — реализация
      GCore/                  — базовая реализация интерфейсов
      GImplementations/       — DualSense/DualShock4 HID protocol, platform layer
  Libs/
    miniaudio/                — audio-to-haptic (опционально)
```

## API библиотеки (ключевые части)

### Жизненный цикл

```cpp
// Device registry — обнаруживает контроллеры
TBasicDeviceRegistry<Policy> Registry;
Registry.PlugAndPlay(DeltaTime);              // каждый кадр — детекция hotplug
ISonyGamepad* Gamepad = Registry.GetLibrary(DeviceId);

// Per-frame loop
Gamepad->UpdateInput(DeltaTime);              // читает HID input report
Gamepad->UpdateOutput();                       // отправляет HID output (LED, triggers, rumble)

// Shutdown
Gamepad->ShutdownLibrary();
```

### Device Registry Policy

Нужно реализовать policy struct для нашего движка:

```cpp
struct UCDevicePolicy {
    using EngineIdType = uint32_t;
    struct Hasher { size_t operator()(uint32_t id) const { return std::hash<uint32_t>{}(id); } };

    uint32_t AllocEngineDevice() { return ++s_next_id; }
    void DisconnectDevice(uint32_t id) { /* пометить disconnected */ }
    void DispatchNewGamepad(uint32_t id) { /* уведомить gamepad.cpp */ }

    static uint32_t s_next_id;
};
```

### Input (FInputContext)

```cpp
FDeviceContext* ctx = Gamepad->GetMutableDeviceContext();
FInputContext* input = ctx->GetInputState();  // thread-safe, game thread

// Стики: -1.0..+1.0
input->LeftAnalog.X, input->LeftAnalog.Y
input->RightAnalog.X, input->RightAnalog.Y

// Триггеры: 0.0..1.0
input->LeftTriggerAnalog, input->RightTriggerAnalog

// Кнопки (bool)
input->bCross, input->bSquare, input->bTriangle, input->bCircle
input->bDpadUp, input->bDpadDown, input->bDpadLeft, input->bDpadRight
input->bLeftShoulder (L1), input->bRightShoulder (R1)
input->bLeftTriggerThreshold (L2), input->bRightTriggerThreshold (R2)
input->bPSButton, input->bShare, input->bStart, input->bMute
input->bLeftStick, input->bRightStick

// Тачпад
input->bIsTouching, input->TouchPosition, input->TouchRelative, input->TouchFingerCount

// Гироскоп/акселерометр
input->Gyroscope (DSVector3D), input->Accelerometer, input->Gravity, input->Tilt

// Батарея
input->BatteryLevel (float)
```

### Output: Вибрация

```cpp
Gamepad->SetVibration(uint8_t left, uint8_t right);  // 0-255
Gamepad->UpdateOutput();
```

### Output: LED (Lightbar)

```cpp
DSCoreTypes::FDSColor color{R, G, B, 1};  // 0-255 per channel
Gamepad->SetLightbar(color);

// Player LED (5 индикаторов)
Gamepad->SetPlayerLed(EDSPlayer::One, 255);  // One/Two/Three/All

// Mic LED
Gamepad->SetMicrophoneLed(EDSMic::MicOff);  // MicOn/MicOff/Pulse

Gamepad->ResetLights();
Gamepad->UpdateOutput();
```

### Output: Adaptive Triggers

```cpp
// Линейное сопротивление
Gamepad->SetResistance(startZone, strength, EDSGamepadHand::Right);

// Лук/арбалет (натяжение + отдача)
Gamepad->SetBow22(startZone, snapBack, EDSGamepadHand::Right);

// Оружие (точка сопротивления)
Gamepad->SetWeapon25(startZone, amplitude, behavior, trigger, hand);

// Пулемёт (быстрые пульсации)
Gamepad->SetMachineGun26(startZone, behavior, amplitude, frequency, hand);

// Машина/механизм (сложный эффект)
Gamepad->SetMachine27(startZone, behaviorFlag, force, amplitude, period, frequency, hand);

// GameCube-style (жёсткий стоп на 50%)
Gamepad->SetGameCube(hand);

// Отключить
Gamepad->StopTrigger(hand);

Gamepad->UpdateOutput();
```

**EDSGamepadHand:** `Left` (L2), `Right` (R2), `AnyHand` (оба).

### Output: Sensors

```cpp
Gamepad->EnableMotionSensor(true);     // включить гироскоп + акселерометр
Gamepad->ResetGyroOrientation();        // калибровка
Gamepad->EnableTouch(true);             // включить тачпад
```

### Output: Audio-to-Haptic

```cpp
Gamepad->GetIGamepadHaptics()->AudioHapticUpdate(std::vector<uint8_t> data);  // 8-bit PCM
Gamepad->GetIGamepadHaptics()->AudioHapticUpdate(std::vector<int16_t> data);  // 16-bit PCM
// Max 64 bytes per update
```

### Статус контроллера

```cpp
Gamepad->IsConnected();                    // bool
Gamepad->GetBattery();                     // float (0.0-1.0)
Gamepad->GetDeviceType();                  // DualSense / DualSenseEdge / DualShock4
Gamepad->GetConnectionType();              // Usb / Bluetooth
```

## Критический вопрос: конфликт SDL3 и DS-lib

Обе библиотеки открывают HID устройство DualSense напрямую. Не могут работать одновременно.

**Решение:**
1. При старте: `SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "0")` — SDL3 не трогает DualSense
2. SDL3 по-прежнему работает для Xbox/generic контроллеров и аудио
3. DS-lib берёт на себя DualSense полностью (input + output)
4. При обнаружении DualSense через DS-lib registry → `s_is_dualsense = true`
5. `gamepad_poll()`: если DualSense → читать из DS-lib, иначе → SDL3

## Технические нюансы

### /Zp1 (struct packing)

Наш проект компилируется с `/Zp1` для совместимости с бинарными форматами ресурсов.
DS-lib использует стандартное выравнивание. Решение — **bridge файл** (как sdl3_bridge):

- `ds_bridge.h` — наш API (C-style, без DS-lib типов)
- `ds_bridge.cpp` — компилируется с `/Zp8`, включает DS-lib хедеры
- `set_source_files_properties(ds_bridge.cpp PROPERTIES COMPILE_OPTIONS "/Zp8")` в CMake
- Остальной код никогда не включает DS-lib хедеры напрямую

### C++ стандарт

DS-lib требует C++20. Решение: обновить весь проект на C++20 (`target_compile_features(Fallen PUBLIC cxx_std_20)`).
Также обновить Clang с 18 до 20 (последний стабильный). Оригинальный код C-like, проблем не будет.

### Thread safety

DS-lib использует mutex'ы для input/output буферов:
- `UpdateInput()` читает HID в background буфер, `SwapInputBuffers()` → game thread
- `GetInputState()` — thread-safe для game thread
- `UpdateOutput()` — отправляет output из game thread

Для нашего однопоточного движка это прозрачно — вызываем `UpdateInput` + `UpdateOutput` каждый кадр.

### GamepadCore CMake

Библиотека собирается через `add_subdirectory`. Нюанс: её `CMAKE_SOURCE_DIR` будет
указывать на наш проект, а не на библиотеку → miniaudio не найдётся → audio module
disabled. Это ок для B0-B4. Для B7 (audio-to-haptic) нужно будет поправить путь.

## Пошаговый план

### B0 — Интеграция библиотеки в сборку

**Файлы:**
- `CMakeLists.txt` — add_subdirectory, link GamepadCore, /Zp8 для bridge, C++20
- `engine/platform/ds_bridge.h` — C API: init, shutdown, poll, detect, LED, triggers, rumble
- `engine/platform/ds_bridge.cpp` — обёртка DS-lib, компилируется с /Zp8 + C++20
- `engine/input/gamepad.cpp` — SDL_HINT для DualSense, вызов ds_bridge при s_is_dualsense

**Проверка:** компилируется, DualSense определяется через DS-lib (лог в консоль).

### B1 — Input через DS-lib

**Файлы:**
- `ds_bridge.cpp` — функция чтения input → заполнение GamepadState
- `gamepad.cpp` — при s_is_dualsense: читать из ds_bridge вместо SDL3

**Маппинг DS-lib → GamepadState:**
```
LeftAnalog.X  → lX (масштаб -1..+1 → 0..65535)
LeftAnalog.Y  → lY
bCross        → rgbButtons[0]
bCircle       → rgbButtons[1]
bSquare       → rgbButtons[2]
bTriangle     → rgbButtons[3]
bShare        → rgbButtons[4]  (Select/Back)
bStart        → rgbButtons[6]
bLeftShoulder → rgbButtons[9]  (L1)
bRightShoulder→ rgbButtons[10] (R1)
bDpadUp       → rgbButtons[11]
bDpadRight    → rgbButtons[12]
bDpadDown     → rgbButtons[13]
bDpadLeft     → rgbButtons[14]
bLeftTriggerThreshold  → rgbButtons[15] (L2 digital)
bRightTriggerThreshold → rgbButtons[16] (R2 digital)
bDpad*        → dpad_active + lX/lY override
```

**Проверка:** все действия работают через DS-lib. Меню, геймплей, пауза, outro.

### B2 — LED по здоровью

**Файлы:**
- `ds_bridge.h/cpp` — `ds_set_lightbar(r, g, b)`, `ds_set_player_led(num)`
- Новый файл или в gamepad.cpp — `gamepad_update_led()` вызывается каждый кадр
- Читает `Health` игрока, маппит в цвет:
  - 100-60% → зелёный (0, 255, 0)
  - 60-30% → жёлтый (255, 255, 0)
  - 30-10% → оранжевый (255, 128, 0)
  - <10% → красный мигающий (255, 0, 0) с toggle каждые 500ms

**Проверка:** LED меняет цвет при получении урона.

### B3 — Adaptive triggers

**Файлы:**
- `ds_bridge.h/cpp` — `ds_set_trigger_resistance()`, `ds_set_trigger_weapon()`, `ds_stop_triggers()`
- Вызовы из game code:
  - **По умолчанию (пеший):** лёгкое сопротивление на L2/R2 (`SetResistance(50, 80)`)
  - **Прицеливание (1st person):** `SetWeapon25` на R2 (ощущение курка)
  - **В машине:** `SetMachine27` (ощущение педалей газа/тормоза)
  - **При выходе из машины/прицеливания:** `StopTrigger` → дефолт

**Проверка:** ощущается сопротивление при нажатии L2/R2 в разных режимах.

### B4 — Вибрация через DS-lib

**Файлы:**
- `ds_bridge.h/cpp` — `ds_set_vibration(left, right)`
- `gamepad.cpp` — в `gamepad_rumble_tick()`: при s_is_dualsense вызывать ds_set_vibration
  вместо sdl3_gamepad_rumble

**Проверка:** вибрация работает как раньше (PS1-style decay), но через DS-lib API.

### B5-B7 — Отложено

**B5 — Тачпад:** `EnableTouch()`, читать `TouchPosition`/`TouchRelative`.
Варианты использования: управление камерой? карта? инвентарь?
Требует дизайн-решения — отложено.

**B6 — Гироскоп:** `EnableMotionSensor()`, читать `Gyroscope`.
Точное прицеливание в first-person. Требует калибровку и UI.
Отложено.

**B7 — Audio-to-haptic:** `AudioHapticUpdate()` через miniaudio.
Нужно поправить CMake path для miniaudio, интегрировать с MFX звуковой системой.
Самая сложная фича — отложена.

## Расположение файлов (итоговое)

```
engine/platform/
  sdl3_bridge.h/cpp     — SDL3 обёртка (уже есть)
  ds_bridge.h/cpp       — DualSense-Multiplatform обёртка (НОВЫЙ)

engine/input/
  gamepad.h/cpp         — абстракция, выбор backend (SDL3 vs DS-lib)
  gamepad_globals.h/cpp — shared state
```
