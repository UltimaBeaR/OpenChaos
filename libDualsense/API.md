# libDualsense API Guide

All types live in `namespace oc::dualsense`. This guide documents the
library's public API. The library is deliberately standalone — it
knows nothing about any particular consumer, game, or application;
it only knows DualSense HID reports.

---

## 1. Device lifecycle (`ds_device.h`)

### Init / shutdown

```cpp
bool hid_init();      // call once at startup (after SDL_Init)
void hid_shutdown();   // call once at exit
```

`hid_init()` calls `SDL_hid_init()` and sets `SDL_HINT_JOYSTICK_HIDAPI_PS5`
to `"0"` so SDL's gamepad subsystem doesn't claim the DualSense.

### Open / close

```cpp
Device dev;
if (device_open_first(&dev)) {
    // dev.connected == true
    // dev.connection == Connection::Usb or Connection::Bluetooth
}

device_close(&dev);
```

`device_open_first` enumerates all Sony USB HID devices (VID 0x054C),
picks the first DualSense (PID 0x0CE6) or DualSense Edge (PID 0x0DF2),
opens it in non-blocking mode. Auto-detects USB vs BT based on
`interface_number`.

### Read (input)

```cpp
uint8_t buf[96];
int n = device_read_latest(&dev, buf, sizeof(buf));
// n > 0: got a report of n bytes in buf
// n == 0: no new report this frame (ok, keep previous state)
// n < 0: error / disconnect (dev is auto-closed)
```

**Queue draining:** `device_read_latest` reads ALL queued HID reports
and keeps only the most recent one. On macOS BT, SDL returns FIFO
order — without draining the caller would read stale data from ~1
second ago. This is critical.

### Write (output)

```cpp
int written = device_write(&dev, buf, dev.output_report_size);
// written < 0: error / disconnect (dev is auto-closed)
```

### Device struct fields

| Field                | Type       | Description                                  |
|----------------------|------------|----------------------------------------------|
| `handle`             | `void*`    | SDL_hid_device* (opaque)                     |
| `path`               | `string`   | HID device path                              |
| `connection`         | `Connection` | `Usb` or `Bluetooth`                       |
| `connected`          | `bool`     | true while open                              |
| `input_report_strip` | `uint8_t`  | bytes to skip before parsing (USB=1, BT=2)  |
| `output_report_size` | `size_t`   | wire size for writes (USB=48, BT=78)         |

---

## 2. Input parsing (`ds_input.h`)

```cpp
InputState input = {};
// buf = raw HID report from device_read_latest
parse_input_report(buf + dev.input_report_strip, &input);
```

### InputState fields

**Sticks** — raw unsigned 0..255, center 128. Y+ is down (device
convention). Callers that want a signed float typically do
`(raw - 128) / 127`, optionally flipping Y.

| Field            | Type      | Range      |
|------------------|-----------|------------|
| `left_stick_x`   | `uint8_t` | 0..255    |
| `left_stick_y`   | `uint8_t` | 0..255    |
| `right_stick_x`  | `uint8_t` | 0..255    |
| `right_stick_y`  | `uint8_t` | 0..255    |
| `left_trigger`   | `uint8_t` | 0..255    |
| `right_trigger`  | `uint8_t` | 0..255    |

**Buttons** — all `bool`.

| Field            | Button          |
|------------------|-----------------|
| `cross`          | X / Cross       |
| `circle`         | O / Circle      |
| `square`         | Square          |
| `triangle`       | Triangle        |
| `l1`, `r1`       | Shoulder        |
| `l2`, `r2`       | Trigger digital |
| `dpad_up/down/left/right` | D-Pad  |
| `start`          | Options         |
| `share`          | Create          |
| `ps_button`      | PS button       |
| `touchpad_click` | Touchpad press  |
| `mute`           | Mute button     |
| `l3`, `r3`       | Stick click     |

**Trigger feedback** — reported by the controller's trigger motor.

| Field                         | Type      | Description                      |
|-------------------------------|-----------|----------------------------------|
| `left_trigger_feedback`       | `uint8_t` | Raw byte from HID offset 0x2A   |
| `right_trigger_feedback`      | `uint8_t` | Raw byte from HID offset 0x29   |
| `left_trigger_effect_active`  | `bool`    | bit 4 of feedback byte           |
| `right_trigger_effect_active` | `bool`    | bit 4 of feedback byte           |

The low nibble (bits 0..3) is the motor state machine value (3..9
typical range during a Weapon effect). Callers that only need the
state value should mask with `& 0x0F`.

**Battery / misc:**

| Field                   | Type      | Description           |
|-------------------------|-----------|-----------------------|
| `battery_level_percent` | `uint8_t` | 0..100                |
| `battery_charging`      | `bool`    | AC/USB power          |
| `battery_full`          | `bool`    | Fully charged         |
| `headphone_connected`   | `bool`    | 3.5mm jack plugged in |

**Touchpad fingers** — up to 2 simultaneous contacts. X/Y are raw
touchpad pixels (X: 0..1919, Y: 0..1079). `*_down` is false when the
finger is lifted; X/Y then hold the last position the finger had
while down.

| Field                       | Type       | Description          |
|-----------------------------|------------|----------------------|
| `touchpad_finger_1_x/_y`    | `uint16_t` | Raw touchpad pixels  |
| `touchpad_finger_1_down`    | `bool`     | Finger in contact    |
| `touchpad_finger_1_id`      | `uint8_t`  | Per-touch ID (0..127)|
| `touchpad_finger_2_x/_y`    | `uint16_t` | Same for 2nd finger  |
| `touchpad_finger_2_down`    | `bool`     |                      |
| `touchpad_finger_2_id`      | `uint8_t`  |                      |

**Motion sensors** — raw device units, no calibration applied. Apply
Sony calibration data (feature report 0x05, not yet read by this
library) to convert gyro to deg/s and accel to g. Without calibration,
data is still usable for relative motion and gestures.

| Field                | Type       | Description                          |
|----------------------|------------|--------------------------------------|
| `gyro_pitch`         | `int16_t`  | Gyro X axis                          |
| `gyro_yaw`           | `int16_t`  | Gyro Y axis                          |
| `gyro_roll`          | `int16_t`  | Gyro Z axis                          |
| `accel_x/_y/_z`      | `int16_t`  | Accelerometer 3 axes                 |
| `motion_timestamp`   | `uint32_t` | Device microseconds, wraps around    |
| `motion_temperature` | `int8_t`   | Internal sensor temperature (raw)    |

---

## 3. Output report (`ds_output.h`)

### OutputState struct

Set fields, then call `build_output_report` to serialize.

| Field                   | Type         | Description                         |
|-------------------------|--------------|-------------------------------------|
| `lightbar_r/g/b`        | `uint8_t`    | Lightbar color, 0..255 per channel  |
| `rumble_left`           | `uint8_t`    | Low-frequency motor, 0..255         |
| `rumble_right`          | `uint8_t`    | High-frequency motor, 0..255        |
| `player_led_mask`       | `uint8_t`    | 5-bit mask (see `PlayerLed::`)      |
| `player_led_brightness` | `uint8_t`    | 0 = brightest                       |
| `mute_led`              | `MuteLed`    | Mic mute LED: `Off` / `On` / `Blink` |
| `haptic_volume`         | `uint8_t`    | Overall rumble volume, 0..7         |
| `lightbar_setup`        | `uint8_t`    | Lightbar setup/override byte, 0 = default |
| `audio_volumes_enabled` | `bool`       | Opt-in flag for audio volume fields (default `false`) |
| `speaker_volume`        | `uint8_t`    | Built-in speaker, 0..255 (only applied if audio_volumes_enabled) |
| `headphone_volume`      | `uint8_t`    | 3.5 mm jack, 0..255 (only applied if audio_volumes_enabled) |
| `trigger_right[10]`     | `uint8_t[]`  | R2 effect slot (filled by triggers) |
| `trigger_left[10]`      | `uint8_t[]`  | L2 effect slot (filled by triggers) |

### Audio volumes — opt-in semantics

The `audio_volumes_enabled` flag exists to distinguish "the game is
managing audio" from "don't touch audio". With the flag at its default
`false`, the library does **not** write the speaker/headphone volume
fields and does not set the corresponding validFlag bits — the
controller's internal audio state is left alone.

Set `audio_volumes_enabled = true` and assign the volumes to actively
control the DualSense audio output. Both values (speaker and headphone)
are sent every frame while the flag is true; the controller routes to
whichever physical output is active (jack detect). Toggling the flag
back to `false` does **not** restore the previous audio state — the
controller simply keeps whatever volume was last written.

### Player LED masks

```cpp
PlayerLed::Off     = 0x00   //  - - - - -
PlayerLed::Center  = 0x04   //  - - x - -
PlayerLed::Inner   = 0x0A   //  - x - x -
PlayerLed::Outer   = 0x11   //  x - - - x
PlayerLed::All     = 0x1F   //  x x x x x
```

### Building the HID report

```cpp
OutputState state;
state.lightbar_r = 255;
state.rumble_left = 100;
trigger_weapon(state.trigger_right, 4, 6, 5);

uint8_t buf[96] = {};
bool bt = (dev.connection == Connection::Bluetooth);
build_output_report(state, buf, bt);
device_write(&dev, buf, dev.output_report_size);
```

`build_output_report` handles:
- Report ID (USB=0x02, BT=0x31)
- Transport padding (USB=1 byte, BT=2 bytes)
- Feature control flags (0xFF / 0x57 defaults)
- Trigger slot placement (R2 at payload offset 10, L2 at 21)
- BT CRC32 at bytes 0x4A..0x4D
- BT alternating sequence counter at absolute offset 40

---

## 4. Adaptive trigger effects (`ds_trigger.h`)

All functions write into a 10-byte slot (`uint8_t out[10]`). The
slot maps directly to the output report: `out[0]` = mode code,
`out[1..9]` = mode-specific parameters.

Returns `true` on success, `false` if parameters are out of range
(slot is zeroed). Zero-strength falls back to Off.

### Available effects

#### Off (mode 0x05)
```cpp
trigger_off(out);
```
Disables any active effect. Trigger is free.

#### Simple Feedback / Resistance (mode 0x01)
```cpp
trigger_simple_feedback(out, position_raw, strength_raw);
// position_raw: 0..255 — where resistance starts
// strength_raw: 0..255 — how strong
```
Raw byte mode, no zone math. Simple continuous resistance.

#### Feedback (mode 0x21)
```cpp
trigger_feedback(out, position, strength);
// position: 0..9 — zone where resistance starts
// strength: 0..8 — resistance level (0 = off)
```
Zone-packed continuous resistance. More precise than Simple_Feedback
but parameter range is smaller. Resistance applies from `position`
to end of travel.

#### Weapon (mode 0x25)
```cpp
trigger_weapon(out, start, end, strength);
// start: 2..7 — zone where click zone begins
// end:   start+1..8 — zone where click zone ends
// strength: 0..8 — click force (0 = off)
```
Single mechanical click between two positions. The DualSense trigger
motor resists at `start`, clicks through at a force determined by
`strength`, and is free again after `end`.

The trigger feedback status reports `effect_active = true` while the
trigger is inside the click zone, useful for hardware-based detection
of trigger state transitions.

#### Vibration (mode 0x26)
```cpp
trigger_vibration(out, position, amplitude, frequency);
// position:  0..9 — zone where vibration starts
// amplitude: 0..8 — vibration strength (0 = off)
// frequency: 1..255 Hz
```
Sustained periodic vibration from `position` onwards.

#### Bow (mode 0x22)
```cpp
trigger_bow(out, start, end, strength, snap);
// start:    0..8 — zone where draw begins
// end:      start+1..8 — zone where snap occurs
// strength: 0..8 — draw resistance
// snap:     0..8 — snap-back force
```
Builds resistance like drawing a bowstring, then snaps back at `end`.

#### Machine (mode 0x27)
```cpp
trigger_machine(out, start, end, amplitude_a, amplitude_b, frequency, period);
// start:       0..8 — zone where pattern begins
// end:         start+1..9 — zone where pattern ends
// amplitude_a: 0..7 — first beat strength
// amplitude_b: 0..7 — second beat strength
// frequency:   1..255 Hz
// period:      pattern period
```
Two-beat alternating rhythmic pattern.

### Effect mode codes (enum TriggerEffect)

| Name             | Code |
|------------------|------|
| Off              | 0x05 |
| Simple_Feedback  | 0x01 |
| Feedback         | 0x21 |
| Weapon           | 0x25 |
| Bow              | 0x22 |
| Vibration        | 0x26 |
| Machine          | 0x27 |
| Galloping        | 0x23 |
| Simple_Weapon    | 0x02 |
| Simple_Vibration | 0x06 |
| Limited_Feedback | 0x11 |
| Limited_Weapon   | 0x12 |

---

## 5. Feature reports (`ds_feature.h`)

Feature reports are a separate HID channel used for telemetry, factory
data, and sensor calibration. All operations in libDualsense are
**read-only**.

### Firmware info (report 0x20)

```cpp
FirmwareInfo fw;
if (get_firmware_info(&dev, &fw)) {
    printf("Built: %s %s\n", fw.buildDate, fw.buildTime);
    printf("HW: 0x%08X, main FW: 0x%08X\n", fw.hwInfo, fw.mainFwVersion);
    // Board generation is in fw.hwInfo lower 16 bits:
    // >= 777 → newer traceability scheme, full PCBA ID supported.
}
```

Fields: `buildDate`, `buildTime`, `fwType`, `swSeries`, `hwInfo`,
`mainFwVersion`, `deviceInfo[12]`, `updateVersion`, `updateImageInfo`,
`sblFwVersion`, `dspFwVersion`, `spiderDspFwVersion`.

### Sensor calibration (report 0x05)

```cpp
SensorCalibration calib;
if (get_sensor_calibration(&dev, &calib)) {
    // Apply via ds_calibration.h:
    float ax_g = calibrate_accel_x_g(input.accel_x, calib);
    float gyro_yaw_dps = calibrate_gyro_yaw_deg_per_sec(input.gyro_yaw, calib);
}
```

### BT patch version (report 0x22)

```cpp
uint32_t patch;
if (get_bt_patch_version(&dev, &patch)) { /* ... */ }
```

---

## 6. Test commands (`ds_test.h`)

The DualSense exposes a large table of **test device / action** pairs
through feature reports 0x80 (request) and 0x81 (response). libDualsense
implements the full handshake state machine and exposes **read-only**
getters:

| Getter                        | Returns                               |
|-------------------------------|---------------------------------------|
| `get_mcu_unique_id`           | 64-bit MCU unique identifier          |
| `get_bd_mac_address`          | 6-byte Bluetooth MAC                  |
| `get_pcba_id`                 | Legacy 48-bit PCBA ID                 |
| `get_pcba_id_full`            | Modern 24-byte PCBA barcode string    |
| `get_serial_number`           | 32-byte Shift-JIS serial              |
| `get_assemble_parts_info`     | 32-byte factory assembly data         |
| `get_battery_barcode`         | 32-byte Shift-JIS battery barcode     |
| `get_vcm_left_barcode`        | 32-byte adaptive-trigger motor barcode |
| `get_vcm_right_barcode`       | 32-byte adaptive-trigger motor barcode |
| `get_battery_voltage`         | Raw precise battery voltage bytes     |
| `get_position_tracking_state` | 0 = disabled, 1 = enabled             |
| `get_always_on_startup_state` | Always-on-startup flag                |
| `get_auto_switchoff_flag`     | Auto-switchoff flag                   |

For advanced usage, the generic primitive:

```cpp
TestResult test_command(Device*, TestDevice, action_id,
                         params, params_len,
                         out_buf, out_capacity, out_len);
```

handles the full chunked-response state machine.

### Write / erase operations are intentionally NOT exposed

Factory writes (calibration, eFuse, traceability, bootloader, BT MAC,
NVS) **can permanently damage** or brick the controller. See review
doc §2.4 "Deferred dangerous operations" for the full deferred list
and the hardware-risk reasoning. These will be revisited only with
dedicated hardware-risk investigation and explicit opt-in.

---

## 7. IMU calibration (`ds_calibration.h`)

Convert raw gyro/accel samples from `InputState` into physical units
using per-device calibration from feature report 0x05.

```cpp
SensorCalibration calib;
get_sensor_calibration(&dev, &calib);

// In your frame loop:
float ax_g = calibrate_accel_x_g(input.accel_x, calib);
float ay_g = calibrate_accel_y_g(input.accel_y, calib);
float az_g = calibrate_accel_z_g(input.accel_z, calib);

float pitch_dps = calibrate_gyro_pitch_deg_per_sec(input.gyro_pitch, calib);
float yaw_dps   = calibrate_gyro_yaw_deg_per_sec  (input.gyro_yaw,   calib);
float roll_dps  = calibrate_gyro_roll_deg_per_sec (input.gyro_roll,  calib);
```

Accel → g is mathematically exact (Sony specifies `+plus`/`+minus`
points as ±1 g in raw units). Gyro → deg/s uses the
community-verified formula from the Linux hid-playstation driver —
matches DS5W / nondebug / DS4Windows.

---

## 8. CRC32 (`ds_crc.h`)

```cpp
uint32_t crc = crc32_compute(buffer, length);                    // output reports
uint32_t crc = crc32_compute_feature_report(reportId, data, len); // feature reports
```

DualSense BT reports use CRC32 with a prefix byte (0xA2 for output,
0x53 for feature) + reportId + data. Output variant uses a seeded
lookup table for efficiency; feature-report variant uses standard
CRC32 (init 0xFFFFFFFF, poly 0xEDB88320, final XOR). Callers normally
don't invoke these directly — higher-level modules handle CRC.

---

## 9. Typical integration

```cpp
// Startup
hid_init();
Device dev;
device_open_first(&dev);

InputState  input  = {};
OutputState output = {};

// Per-frame
uint8_t in_buf[96];
int n = device_read_latest(&dev, in_buf, sizeof(in_buf));
if (n > 0) {
    parse_input_report(in_buf, n,
                       dev.connection == Connection::Bluetooth,
                       input);
}
// use input.left_stick_x, input.cross, input.left_trigger_feedback, ...

// Set outputs as needed
output.lightbar_r = 0; output.lightbar_g = 128; output.lightbar_b = 255;
output.rumble_left = 100; output.rumble_right = 100;
trigger_weapon(output.trigger_right, 4, 6, 5);

uint8_t out_buf[96] = {};
build_output_report(output, out_buf,
                    dev.connection == Connection::Bluetooth);
device_write(&dev, out_buf, dev.output_report_size);

// Shutdown
device_close(&dev);
hid_shutdown();
```

Reconnect logic, dirty tracking, unit conversion to application types
(floats, etc.) are deliberately left to the caller — the library stays
minimal and has no opinion on frame cadence or how the caller wants
to marshal data into its own types.
