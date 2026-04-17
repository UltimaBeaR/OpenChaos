# libDualsense

Minimal, self-contained DualSense controller library. Handles USB and
Bluetooth on Windows, macOS, and Linux via SDL3 hidapi. Has no
dependencies beyond SDL3 and knows nothing about any particular
consumer — just DualSense HID reports.

## Architecture

```
ds_device   — HID transport (SDL3 hidapi): open, read, write
ds_input    — input report parser → InputState struct
ds_output   — output report builder from OutputState struct
ds_trigger  — adaptive trigger effect byte packing (all modes)
ds_crc      — CRC32 for Bluetooth output reports
```

Public entry points live in `ds_device.h`, `ds_input.h`, `ds_output.h`,
`ds_trigger.h`. All types live in `namespace oc::dualsense`. See
[`API.md`](API.md) for a usage guide.

## Files

| File               | What it does                                            |
|--------------------|---------------------------------------------------------|
| `ds_device.h/cpp`  | Device discovery, open/close, non-blocking read/write   |
| `ds_input.h/cpp`   | Parses raw HID input report → `InputState` struct       |
| `ds_output.h/cpp`  | Builds HID output report from `OutputState` struct      |
| `ds_trigger.h/cpp` | Adaptive trigger effect byte packing (all modes)        |
| `ds_crc.h/cpp`     | CRC32 for Bluetooth output reports                      |
| `LICENSE`          | MIT license                                              |
| `THIRD_PARTY_LICENSES.md` | Upstream MIT notices (DS5W CRC, duaLib triggers) |
| `README.md`        | This file                                                |
| `API.md`           | Detailed usage guide with code examples                  |

## Supported features

**Input:** sticks, analog triggers, all buttons (face, shoulders,
d-pad, L3/R3, share, options, PS, touchpad click, mute), battery
level, headphone connected flag, adaptive trigger feedback status
(state nybble + effect-active bit per trigger), touchpad fingers.

**Output:** lightbar RGB, player LEDs, rumble (two-motor DS4-style),
adaptive trigger effects — Off, Feedback, Simple_Feedback, Weapon,
Simple_Weapon, Vibration, Simple_Vibration, Bow, Galloping, Machine,
Limited_Feedback, Limited_Weapon (12 total).

**Transport:** USB and Bluetooth, auto-detected at device open.

## Not supported

- DualShock 4
- DualSense Edge paddles / Fn keys
- Gyro / accelerometer (candidate for future addition — low priority)
- Audio haptics (PCM → voice-coil)
- Sensor calibration
- Microphone LED control

## Licensing

MIT — see `LICENSE`. Some code adapted from third-party MIT projects;
see `THIRD_PARTY_LICENSES.md`.
