# libDualsense

Minimal, self-contained DualSense controller library. Handles USB and
Bluetooth on Windows, macOS, and Linux via SDL3 hidapi. Has no
dependencies beyond SDL3 and knows nothing about any particular
consumer — just DualSense HID reports.

## Architecture

```
ds_device       — HID transport (SDL3 hidapi): open, read, write,
                  feature reports
ds_input        — input report parser → InputState struct
ds_output       — output report builder from OutputState struct
ds_trigger      — adaptive trigger effect byte packing (all modes)
ds_feature      — feature reports: firmware info, sensor calibration,
                  BT patch version
ds_test         — test-command handshake (0x80 / 0x81): factory data,
                  IDs, battery voltage, system state flags
ds_calibration  — apply factory IMU calibration (raw → g, deg/s)
ds_crc          — CRC32 for Bluetooth output and feature reports
```

Public entry points live in `ds_device.h`, `ds_input.h`, `ds_output.h`,
`ds_trigger.h`, `ds_feature.h`, `ds_test.h`, `ds_calibration.h`. All
types live in `namespace oc::dualsense`. See [`API.md`](API.md) for a
usage guide.

## Files

| File                   | What it does                                            |
|------------------------|---------------------------------------------------------|
| `ds_device.h/cpp`      | Device discovery, open/close, I/O, feature reports      |
| `ds_input.h/cpp`       | Parses raw HID input report → `InputState` struct       |
| `ds_output.h/cpp`      | Builds HID output report from `OutputState` struct      |
| `ds_trigger.h/cpp`     | Adaptive trigger effect byte packing (all modes)        |
| `ds_feature.h/cpp`     | Firmware info, sensor calibration, BT patch parsers     |
| `ds_test.h/cpp`        | Test-command handshake + factory-data getters           |
| `ds_calibration.h/cpp` | Apply IMU calibration to raw gyro/accel samples         |
| `ds_crc.h/cpp`         | CRC32 for Bluetooth output and feature reports          |
| `LICENSE`              | MIT license                                              |
| `THIRD_PARTY_LICENSES.md` | Upstream MIT notices (DS5W CRC, duaLib triggers) |
| `README.md`            | This file                                                |
| `API.md`               | Detailed usage guide with code examples                  |

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

**Feature reports (read-only):** firmware info (build date/time, fwType,
hwInfo, SW/SBL/DSP/MCU versions, device info, update version), sensor
calibration (gyroscope + accelerometer factory data, applied to raw
samples for deg/s + g output), BT patch version.

**Factory data (read-only test commands):** MCU unique ID, Bluetooth
MAC address, PCBA ID (legacy + full), serial number, assemble parts
info, battery barcode, VCM (adaptive-trigger motor) barcodes left and
right, precise battery voltage, position tracking state, always-on
startup state, auto-switchoff flag.

**Transport:** USB and Bluetooth, auto-detected at device open.

## Not supported

- DualShock 4
- DualSense Edge paddles / Fn keys
- Audio haptics (PCM → voice-coil)
- Microphone audio volume (field exists in HID layout but not wired
  up in daidr-tester reference either — no empirical verification
  available; deferred until feature reports give confirmation)
- Power-save mic mute control
- Audio control 2 byte (purpose not documented by any public reference)

### ⚠️ Deferred — potentially dangerous factory write/erase commands

Read operations are safe and fully implemented. **Write and erase**
operations are deliberately NOT exposed because they can permanently
damage or brick the controller:

- Calibration writes/erase (`WRITE_RAW_CALIBRATION_DATA`,
  `ERASE_CALIBRATION_DATA`, `WRITE_CALIBRATION_COEFFICIENT`) —
  overwrites factory IMU calibration; **Sony service required to
  restore**, home recovery not possible.
- Factory data writes (`WRITE_FACTORY_DATA`, `WRITE_HWVERSION`,
  `WRITE_PCBAID(_FULL)`, `WRITE_TRACABILITY_INFO*`) — irreversibly
  overwrites factory-assigned identifiers.
- eFuse writes (`WRITE_EFUSE`) — **PERMANENT**, fuse bits cannot
  be un-blown by any means.
- Bluetooth identity (`WRITE_BDADR`, `WRITE_INIT`) — alters BT MAC,
  breaks existing pairings.
- Bootloader operations (`TEST_ACTION_BOOTLOADER_*`) — flashes
  firmware; **bricks on interruption or error**.
- NVS operations (`NVS_LOCK/UNLOCK/GET_STATUS`, `SAVE_CONFIG`,
  `ERASE_CONFIG`) — direct non-volatile storage access.
- Codec register direct access (`CTRL_CODEC_REG_WRITE/READ`).
- Invasive self-tests (`SOLOMON_SELF_TEST`, `CYPRESS_SELF_TEST`,
  `AGING_*`).

See `new_game_devlog/dualsense_libs_reference/daidr_dualsense_tester_review.md`
§2.4 "Deferred dangerous operations" for the action-ID lookup table
and notes if these are ever revisited.

## Licensing

MIT — see `LICENSE`. Some code adapted from third-party MIT projects;
see `THIRD_PARTY_LICENSES.md`.

## Trademark notice

"DualSense" and "PlayStation" are trademarks of Sony Interactive
Entertainment LLC. This library is an independent open-source project
created via reverse engineering of the publicly-documented HID
protocol. It is not affiliated with, endorsed by, or sponsored by Sony
Interactive Entertainment or any of its subsidiaries.
