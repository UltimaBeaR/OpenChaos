# libDualsense API Guide

All types live in `namespace oc::dualsense`. Game code does NOT
include libDualsense headers directly — it uses `ds_bridge.h` which
wraps the library. This guide documents the library's own API for
anyone maintaining or extending the bridge.

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
order — without draining the game would read stale data from ~1 second
ago. This is critical.

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
| `output_report_size` | `size_t`   | wire size for writes (USB=74, BT=78)         |

---

## 2. Input parsing (`ds_input.h`)

```cpp
InputState input = {};
// buf = raw HID report from device_read_latest
parse_input_report(buf + dev.input_report_strip, &input);
```

### InputState fields

**Sticks** — raw unsigned 0..255, center 128. Y+ is down (device
convention). The bridge converts to float [-1,+1] with Y flipped.

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
typical range during a Weapon effect). The bridge masks it with
`& 0x0F` before exposing to game code.

**Battery / misc:**

| Field                   | Type      | Description           |
|-------------------------|-----------|-----------------------|
| `battery_level_percent` | `uint8_t` | 0..100                |
| `battery_charging`      | `bool`    | AC/USB power          |
| `battery_full`          | `bool`    | Fully charged         |
| `headphone_connected`   | `bool`    | 3.5mm jack plugged in |

---

## 3. Output report (`ds_output.h`)

### OutputState struct

Set fields, then call `build_output_report` to serialize.

| Field                   | Type         | Description                         |
|-------------------------|--------------|-------------------------------------|
| `lightbar_r/g/b`       | `uint8_t`    | Lightbar color, 0..255 per channel  |
| `rumble_left`           | `uint8_t`    | Low-frequency motor, 0..255         |
| `rumble_right`          | `uint8_t`    | High-frequency motor, 0..255        |
| `player_led_mask`       | `uint8_t`    | 5-bit mask (see `PlayerLed::`)      |
| `player_led_brightness` | `uint8_t`    | 0 = brightest                       |
| `trigger_right[10]`     | `uint8_t[]`  | R2 effect slot (filled by triggers) |
| `trigger_left[10]`      | `uint8_t[]`  | L2 effect slot (filled by triggers) |

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
Raw byte mode, no zone math. Used for vehicle brake (`position=20,
strength=200`). Simple continuous resistance.

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
`strength`, and is free again after `end`. Used for gun fire.

The trigger feedback status reports `effect_active = true` while the
trigger is inside the click zone, which can be used for hardware-based
fire detection.

#### Vibration (mode 0x26)
```cpp
trigger_vibration(out, position, amplitude, frequency);
// position:  0..9 — zone where vibration starts
// amplitude: 0..8 — vibration strength (0 = off)
// frequency: 1..255 Hz
```
Sustained periodic vibration from `position` onwards. Intended for
automatic weapons (e.g. AK47 continuous trigger buzz).

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
Two-beat alternating pattern, like a machine gun with recoil.

### Effect mode codes (enum TriggerEffect)

| Name             | Code | Used in game |
|------------------|------|--------------|
| Off              | 0x05 | Yes          |
| Simple_Feedback  | 0x01 | Yes (brake)  |
| Feedback         | 0x21 | Not yet      |
| Weapon           | 0x25 | Yes (pistol) |
| Bow              | 0x22 | Not yet      |
| Vibration        | 0x26 | Not yet      |
| Machine          | 0x27 | Not yet      |
| Galloping        | 0x23 | Not yet      |
| Simple_Weapon    | 0x02 | No           |
| Simple_Vibration | 0x06 | No           |
| Limited_Feedback | 0x11 | No           |
| Limited_Weapon   | 0x12 | No           |

---

## 5. CRC32 (`ds_crc.h`)

```cpp
uint32_t crc = crc32_compute(buffer, length);
```

Standard CRC32 with DualSense-specific seed (`0xeada2d49`). Used
by `build_output_report` for Bluetooth — game code doesn't call
this directly.

---

## 6. Typical game-loop integration

```cpp
// Startup
hid_init();

// Per-frame
ds_poll_registry(dt);       // try to find device if disconnected
ds_update_input(dt);        // read latest HID report

DS_InputState input;
if (ds_get_input(&input)) {
    // use input.left_stick_x, input.cross, etc.
}

// Set outputs as needed
ds_set_lightbar(r, g, b);
ds_set_vibration(left, right);
ds_trigger_weapon(4, 6, 5, 0, 1);   // R2 weapon click

ds_update_output();          // flush to device (only if dirty)

// Shutdown
ds_shutdown();
```

Note: the functions above (`ds_*`) are from `ds_bridge.h`, not
libDualsense directly. They wrap the library and handle unit
conversion (raw bytes ↔ game floats), dirty tracking, and reconnect
logic.

---

## 7. Debug overlay

Press **F2** in-game to toggle the gamepad debug overlay. Shows:
- Stick values (DI range 0..65535)
- Analog trigger values (0..255)
- All button states (green = pressed)
- Trigger feedback status (state nybble + effect-active bit)
- Raw 10-byte trigger effect slot dump (L and R)

Useful for verifying that the correct effect bytes are being sent
and that input parsing is working.
