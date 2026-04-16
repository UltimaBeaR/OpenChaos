# libDualsense

A minimal, self-contained DualSense controller library written for the
**OpenChaos** project (Urban Chaos modernization). Not a general-purpose
library — only what the game needs, nothing more.

## What it does

- HID I/O with Sony DualSense (USB + Bluetooth) via SDL3 hidapi
- Parses the input report (sticks, triggers, buttons, battery, adaptive
  trigger feedback status)
- Builds output reports (lightbar, rumble, adaptive trigger effects)
- Implements all documented adaptive trigger effect modes (Off, Feedback,
  Weapon, Vibration, Bow, Machine) with correct byte-level packing

## What it deliberately does NOT do

- No DualShock 4 support
- No DualSense Edge paddles / Fn keys
- No gyro / accelerometer
- No touchpad coordinates (only the click)
- No audio haptics (PCM → voice-coil)
- No player LED policy (the game decides what to show)
- No sensor calibration

If any of the above are ever needed, they can be added. Until then they
stay out so the library remains small and auditable.

## Files

| File                       | Purpose                                        |
|----------------------------|------------------------------------------------|
| `ds_device.h/.cpp`         | HID device discovery, open/close, read/write   |
| `ds_input.h/.cpp`          | Input report parser (HID bytes → struct)       |
| `ds_output.h/.cpp`         | Output report builder (struct → HID bytes)    |
| `ds_trigger.h/.cpp`        | Adaptive trigger effect factories              |
| `ds_crc.h/.cpp`            | CRC32 for Bluetooth output reports             |
| `LICENSE`                  | MIT, OpenChaos project                         |
| `THIRD_PARTY_LICENSES.md`  | Attribution for adapted upstream code          |
| `README.md`                | This file                                      |

> Note: `ds_device.*` and `ds_output.*` are added in a later iteration.

## Licensing

This library is MIT-licensed — see `LICENSE`.

Some pieces (the CRC32 table, the adaptive trigger packing logic) are
adapted from third-party MIT-licensed projects. Their upstream copyright
notices are collected in `THIRD_PARTY_LICENSES.md` rather than scattered
through source comments, so the sources stay clean.

## Why a custom library instead of an existing one?

The project previously used the vendored
[rafaelvaloto/Dualsense-Multiplatform](https://github.com/rafaelvaloto/Dualsense-Multiplatform)
library. During development it was discovered that most of the adaptive
trigger effect packings in that library were incorrect, and it was missing
features that OpenChaos needs (such as reading the trigger feedback status
bytes). Maintaining local patches to a large external library with an
ambitious abstraction layer turned out to be harder than just writing a
~1000-line purpose-built replacement from scratch, based on the
better-validated upstream references.

See `dualsense_libs_reference/own_dualsense_lib_plan.md` at the repository
root for the full design and migration plan.
