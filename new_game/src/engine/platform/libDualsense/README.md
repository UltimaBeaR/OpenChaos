# libDualsense

Minimal DualSense controller library for the **OpenChaos** project.
Handles USB and Bluetooth on Windows, macOS, and Linux via SDL3 hidapi.

## Architecture

```
Game code
  │
  ▼
ds_bridge.h  ← public API (game never includes libDualsense headers)
  │
  ▼
ds_bridge_own.cpp  ← translates game types ↔ libDualsense types
  │
  ├─ ds_device   — HID transport (SDL3 hidapi)
  ├─ ds_input    — input report parser
  ├─ ds_output   — output report builder
  ├─ ds_trigger  — adaptive trigger effect factories
  └─ ds_crc      — CRC32 for Bluetooth output
```

Game code calls `ds_init()`, `ds_poll_registry()`, `ds_update_input()`,
`ds_get_input()`, `ds_set_lightbar()`, `ds_trigger_weapon()`, etc. —
all declared in `ds_bridge.h`. The bridge file translates between the
game's `DS_InputState` struct and libDualsense's internal types.

libDualsense itself has no knowledge of the game — it only knows about
DualSense HID reports.

## Files

| File               | What it does                                            |
|--------------------|---------------------------------------------------------|
| `ds_device.h/cpp`  | Device discovery, open/close, non-blocking read/write   |
| `ds_input.h/cpp`   | Parses raw HID input report → `InputState` struct       |
| `ds_output.h/cpp`  | Builds HID output report from `OutputState` struct      |
| `ds_trigger.h/cpp` | Adaptive trigger effect byte packing (all modes)        |
| `ds_crc.h/cpp`     | CRC32 for Bluetooth output reports                      |
| `LICENSE`          | MIT license (OpenChaos)                                  |
| `THIRD_PARTY_LICENSES.md` | Upstream MIT notices (DS5W CRC, duaLib triggers) |
| `README.md`        | This file                                                |
| `API.md`           | Detailed usage guide with code examples                  |

## Supported features

**Input:** sticks, analog triggers, all buttons (face, shoulders,
d-pad, L3/R3, share, options, PS, touchpad click, mute), battery
level, headphone connected flag, adaptive trigger feedback status
(state nybble + effect-active bit per trigger).

**Output:** lightbar RGB, player LEDs, rumble (two-motor DS4-style),
adaptive trigger effects (Off, Feedback, Simple_Feedback, Weapon,
Vibration, Bow, Machine).

**Transport:** USB and Bluetooth, auto-detected at device open.

## Not supported (by design)

- DualShock 4
- DualSense Edge paddles / Fn keys
- Gyro / accelerometer
- Touchpad coordinates (only click)
- Audio haptics (PCM → voice-coil)
- Sensor calibration
- Microphone LED control

## Licensing

MIT — see `LICENSE`. Some code adapted from third-party MIT projects;
see `THIRD_PARTY_LICENSES.md`.

## Background

Replaced vendored rafaelvaloto/Dualsense-Multiplatform (~10K lines)
which had broken adaptive trigger packings and missing trigger
feedback status reading. This library is ~800 lines, purpose-built,
and empirically verified on Windows USB/BT.
