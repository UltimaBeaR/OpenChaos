# Dualsense-Multiplatform — техническая документация

Справочник по API библиотеки и особенностям интеграции.

## Библиотека

**Репо:** `libs/dualsense-multiplatform/Dualsense-Multiplatform/` (vendored)
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

## API библиотеки

### Жизненный цикл

```cpp
TBasicDeviceRegistry<Policy> Registry;
Registry.PlugAndPlay(DeltaTime);              // каждый кадр — детекция hotplug
ISonyGamepad* Gamepad = Registry.GetLibrary(DeviceId);

Gamepad->UpdateInput(DeltaTime);              // читает HID input report
Gamepad->UpdateOutput();                       // отправляет HID output (LED, triggers, rumble)

Gamepad->ShutdownLibrary();
```

### Input (FInputContext)

```cpp
FDeviceContext* ctx = Gamepad->GetMutableDeviceContext();
FInputContext* input = ctx->GetInputState();

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
Gamepad->SetPlayerLed(EDSPlayer::One, 255);
Gamepad->SetMicrophoneLed(EDSMic::MicOff);
Gamepad->ResetLights();
Gamepad->UpdateOutput();
```

### Output: Adaptive Triggers

```cpp
Gamepad->SetResistance(startZone, strength, EDSGamepadHand::Right);
Gamepad->SetBow22(startZone, snapBack, EDSGamepadHand::Right);
Gamepad->SetWeapon25(startZone, amplitude, behavior, trigger, hand);
Gamepad->SetMachineGun26(startZone, behavior, amplitude, frequency, hand);
Gamepad->SetMachine27(startZone, behaviorFlag, force, amplitude, period, frequency, hand);
Gamepad->SetGameCube(hand);
Gamepad->StopTrigger(hand);
Gamepad->UpdateOutput();
```

**EDSGamepadHand:** `Left` (L2), `Right` (R2), `AnyHand` (оба).

### Output: Sensors

```cpp
Gamepad->EnableMotionSensor(true);
Gamepad->ResetGyroOrientation();
Gamepad->EnableTouch(true);
```

### Output: Audio-to-Haptic

```cpp
Gamepad->GetIGamepadHaptics()->AudioHapticUpdate(std::vector<uint8_t> data);  // 8-bit PCM
Gamepad->GetIGamepadHaptics()->AudioHapticUpdate(std::vector<int16_t> data);  // 16-bit PCM
// Max 64 bytes per update
```

### Статус контроллера

```cpp
Gamepad->IsConnected();
Gamepad->GetBattery();
Gamepad->GetDeviceType();      // DualSense / DualSenseEdge / DualShock4
Gamepad->GetConnectionType();  // Usb / Bluetooth
```

## Технические нюансы интеграции

### Конфликт SDL3 и DS-lib

Обе библиотеки открывают HID устройство DualSense. Решение:
1. `SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "0")` — SDL3 не трогает DualSense
2. SDL3 работает для Xbox/generic и аудио
3. DS-lib берёт DualSense полностью (input + output)

### /Zp1 (struct packing)

Проект компилируется с `/Zp1` для совместимости с бинарными форматами ресурсов. DS-lib — стандартное выравнивание. Решение — bridge файл:
- `ds_bridge.h` — C-style API (без DS-lib типов)
- `ds_bridge.cpp` — компилируется с `/Zp8`
- `set_source_files_properties(ds_bridge.cpp PROPERTIES COMPILE_OPTIONS "/Zp8")` в CMake

### Thread safety

DS-lib использует mutex'ы для input/output буферов. Для нашего однопоточного движка прозрачно — `UpdateInput` + `UpdateOutput` каждый кадр.

### GamepadCore CMake

`add_subdirectory` — `CMAKE_SOURCE_DIR` указывает на наш проект → miniaudio не найдётся → audio module disabled. Для audio-to-haptic нужно будет поправить путь.

## Расположение файлов

```
engine/platform/
  sdl3_bridge.h/cpp     — SDL3 обёртка
  ds_bridge.h/cpp       — DualSense-Multiplatform обёртка

engine/input/
  gamepad.h/cpp         — абстракция, выбор backend (SDL3 vs DS-lib)
  gamepad_globals.h/cpp — shared state
```

### Маппинг DS-lib → GamepadState

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
