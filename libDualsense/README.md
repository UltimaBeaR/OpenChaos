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
(state nybble + effect-active bit per trigger), touchpad fingers,
motion sensors (gyroscope pitch/yaw/roll, accelerometer X/Y/Z,
motion timestamp, motion temperature — all in raw device units).

**Output:** lightbar RGB, player LEDs (mask + brightness), rumble
(two-motor DS4-style), mic mute LED (off / on / blink), overall haptic
volume (0..7), lightbar setup byte, built-in speaker volume, 3.5 mm
headphone jack volume, adaptive trigger effects — Off, Feedback,
Simple_Feedback, Weapon, Simple_Weapon, Vibration, Simple_Vibration,
Bow, Galloping, Machine, Limited_Feedback, Limited_Weapon (12 total).

**Transport:** USB and Bluetooth, auto-detected at device open.

## Not supported

- DualShock 4
- DualSense Edge paddles / Fn keys
- Audio haptics (PCM → voice-coil)
- Sensor calibration (motion data is raw; no official calibration applied)
- Microphone audio volume (field exists in HID layout but not wired
  up in daidr-tester reference either — no empirical verification
  available; deferred until feature reports give confirmation)
- Power-save mic mute control
- Audio control 2 byte (purpose not documented by any public reference)

## Licensing

MIT — see `LICENSE`. Some code adapted from third-party MIT projects;
see `THIRD_PARTY_LICENSES.md`.

## Trademark notice

"DualSense" and "PlayStation" are trademarks of Sony Interactive
Entertainment LLC. This library is an independent open-source project
created via reverse engineering of the publicly-documented HID
protocol. It is not affiliated with, endorsed by, or sponsored by Sony
Interactive Entertainment or any of its subsidiaries.
