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
types live in `namespace oc::dualsense`.

**Where to look:**
- This README — conventions, ranges, counter-intuitive semantics,
  things the raw API won't tell you from the signatures alone.
- [`API.md`](API.md) — detailed reference: every public function, every
  struct field, parameter ranges, return values.
- [`EXAMPLES.md`](EXAMPLES.md) — one short example per API area.

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
| `README.md`            | This file — overview, conventions, gotchas               |
| `API.md`               | Detailed per-function / per-struct reference             |
| `EXAMPLES.md`          | One short example per API area                           |

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

## Using the API — conventions and non-obvious things

The current API is **raw** — values are in HID-native units (bytes,
zones, raw int16), not in "nice" units (percent, float, physical).
A convenience layer that normalises everything to floats / enums /
physical units is planned but not shipped. Until then, this section
documents every rough edge a consumer has to know.

### Ranges and units — input

**Sticks.** `uint8_t` 0..255 with centre at **128**. Y grows **downward**
(device convention; up = smaller value). To convert to signed float
[-1, +1] with centre at 0, use `normalize_stick_axis(raw)`. Raw 0
would produce -128/127 ≈ -1.008 — the helper clamps to [-1, +1] so
downstream fixed-point scaling doesn't overflow. Y-axis flip for
"up is positive" is a **game-level convention** — apply it in your
own code, the library returns raw device orientation.

**Analog triggers.** `uint8_t` 0..255 pull. Use `normalize_trigger(raw)`
for [0, 1] float. Digital thresholds are reported separately as
`bool l2` / `bool r2`.

**Battery level.** `battery_level_percent` is **already decoded** to
0..100 from a raw nibble 0..10 (10 % bins). Raw nibble 11 means
UNKNOWN; in that case the library **keeps the previous percent value**
to avoid UI flicker. `battery_charging` and `battery_full` are bools
decoded from the high nibble of status byte 0 (charge status). The
raw charge-status nibble also encodes error codes (10 = abnormal
voltage, 11 = abnormal temperature, 15 = charging error) — the library
currently exposes only charging / full booleans; consumers that want
the error codes can read `r[0x34]` themselves via raw HID.

**Audio presence.** `headphone_connected` / `mic_connected` bools —
3.5 mm TRRS jack state. Mic is "microphone detected on the jack",
not "mic button on the controller".

**Touchpad fingers.** Hardware resolution is **1920 × 1080** (X: 0..1919,
Y: 0..1079). Two simultaneous fingers tracked. `*_down` is `true` when
the finger is touching — note that on the wire the bit is **inverted**
(wire says "lifted", library returns "down"). When a finger is lifted,
its X/Y fields hold the **last position while it was down**, not a
current sample. `*_id` is a per-touch identifier 0..127 (bit 7 of the
wire byte holds the lifted flag; library masks it off).

**IMU sensors.** Gyro (pitch / yaw / roll) and accel (X / Y / Z) are
**raw `int16_t` device units, no calibration applied.** Without
calibration they are only meaningful for relative motion and gestures.
To get physical units (deg/s and g), read factory calibration once
via `get_sensor_calibration` (feature report 0x05), then convert each
sample with `ds_calibration.h` helpers (`calibrate_gyro_*` /
`calibrate_accel_*`). Calibration is per-device — every controller
has slightly different bias and scaling.

**Motion timestamp.** `motion_timestamp` is **device microseconds**
that wraps around at `uint32_t` max. Differences across consecutive
frames are meaningful; absolute values are not. Handle wrap-around if
you integrate it.

**Motion temperature.** Raw `int8_t`, device units — no conversion to
celsius documented.

**Trigger feedback byte.** `left_trigger_feedback` / `right_trigger_feedback`
are the **full raw bytes** from HID offsets 0x29 / 0x2A. The library
pre-decodes two common fields out of them: `*_trigger_effect_active`
(bit 4) and the low nibble (bits 0..3) is the motor state machine
value (typical range 3..9 during a Weapon effect; semantics vary per
effect). Consumers doing a "game reacts to click" flow want
`*_trigger_effect_active` — that's the hardware-detected moment the
trigger crossed into the click zone. ⚠️ **Important:** only full-name
effects (`Feedback`, `Weapon`, `Vibration`, `Bow`, `Machine`,
`Galloping`) report feedback. **Simple / Limited variants
(`Simple_*`, `Limited_*`) always report zero** regardless of what the
trigger is doing — that's a firmware behaviour, not a bug.

**D-pad.** Wire format is a single "hat" value 0..8 (0 = up,
clockwise to 7 = up-left, 8 = centred). Library decomposes into four
`dpad_up / down / left / right` booleans.

### Ranges and units — output

**Rumble.** `rumble_left` is the **low-frequency** motor (DS4 naming
convention — don't expect it to rumble "on the left side"),
`rumble_right` is the high-frequency motor. Both `uint8_t` 0..255.

**Lightbar RGB.** 8-bit per channel 0..255. No gamma, no colour space
conversion — what you write is what the LED drives.

**Player LED mask.** 5-bit bitmask (`uint8_t` 0x00..0x1F) but the
hardware **mirrors bits 0 ↔ 4 and 1 ↔ 3** so only 3 pairs are
independently addressable: outer pair, inner pair, centre. Use the
`PlayerLed::` constants (`Off / Center / Inner / Outer / All`) rather
than assembling bits by hand.

**Player LED brightness.** ⚠️ **Counter-intuitive:** `uint8_t`
**0 = brightest, 2 = dimmest.** Values greater than 2 are clamped to
0 by `ds_set_player_led_brightness`. In the convenience layer this
will be flipped so 0 = off / 1 = brightest, but the raw field keeps
the hardware convention.

**Haptic volume.** `uint8_t` **0..7** overall rumble gain (a 3-bit
field on the wire — this is _not_ a percent). Applied by the
controller on top of `rumble_left` / `rumble_right` amplitudes.

**Lightbar setup.** `uint8_t` raw wire byte. daidr's own UI does not
expose it; no observable effect has been seen on real hardware for
any value. Kept in the API for completeness; 0 = default.

**Mic mute LED.** `MuteLed` enum — `Off` / `On` / `Blink`. On = steady
("mic is muted by controller"), Blink = "system-wide all-mic mute".

**Audio volumes — opt-in.** ⚠️ This is the one that bites. The
`audio_volumes_enabled` flag (`bool`, default `false`) exists to
distinguish "my app is managing audio" from "don't touch audio":

- With `audio_volumes_enabled = false` (default), the library does **not**
  write `speaker_volume` / `headphone_volume` / `audio_route` to the
  wire **at all** — those fields are effectively ignored. The
  controller's audio state is left at whatever the host OS / previous
  process set it to.
- Set `audio_volumes_enabled = true` and assign the three fields to
  actively drive audio. Both volumes are sent every frame; `audio_route`
  picks which physical output actually plays (speaker vs headphone
  jack).
- Toggling `audio_volumes_enabled` back to `false` does **not** restore
  the previous state — the controller simply stops hearing from us,
  keeping whatever volume was last written.

**Audio route.** `AudioRoute::{Headphone, Speaker}` enum — picks
physical output. Only meaningful when `audio_volumes_enabled = true`.
⚠️ Hardware quirk: the two outputs **cross-leak** acoustically
(especially on BT — you'll hear the speaker faintly even with
headphone routing). If your UI cares, zero out the inactive output's
volume at app level; the library has no workaround. See §14.1 in
`own_dualsense_lib_plan.md`.

**Adaptive trigger slot layout.** Each trigger gets a 10-byte slot
(`OutputState::trigger_left[10]` / `trigger_right[10]`). Format:
`out[0]` = mode code, `out[1..9]` = mode-specific parameters. Use the
factory functions in `ds_trigger.h` (`trigger_off`, `trigger_weapon`,
`trigger_feedback`, etc.) — they know every mode's exact bit layout.
Hand-rolling slot bytes without reading `ds_trigger.cpp` will almost
certainly produce garbage or Off.

**Adaptive trigger — zone vs raw parameter conventions:**

- **Full-name effects** (`Feedback`, `Weapon`, `Vibration`, `Bow`,
  `Machine`, `Galloping`) take **zone indices 0..9** for positions
  along trigger travel (10 discrete zones). Strength / amplitude
  parameters are usually 0..8. Frequency is Hz 0..255 where applicable.
- **Simple / Limited variants** (`Simple_Feedback`, `Simple_Weapon`,
  `Simple_Vibration`, `Limited_Feedback`, `Limited_Weapon`) take **raw
  bytes 0..255** for positions, and strength is 0..10 or 0..255
  depending on variant. These are the variants that **don't report
  feedback status** — trade-off: finer control, no hardware-side
  effect-active signal.
- Zero amplitude generally maps to `Off` automatically. Out-of-range
  parameters zero the slot and return `false` from the factory
  function.

See API.md §4 for every effect's full parameter table.

### Device lifecycle — call order matters

```
hid_init();                          // once at startup
Device dev;
if (device_open_first(&dev)) {
    device_send_init_packet(&dev);   // BT handshake; no-op on USB
    // per-frame loop:
    //   device_read_input(&dev, &in);
    //   ... set OutputState fields ...
    //   device_send_output(&dev, out);
}
device_shutdown(&dev);               // graceful: tone off + quiet + close
hid_shutdown();                      // once at exit
```

**`device_shutdown` vs `device_close`.** Use `device_shutdown` for a
graceful exit — it stops any audio test tone, zeros rumble / LEDs /
lightbar / triggers / mute LED, waits briefly for the controller to
latch the cleared state, then closes the handle. `device_close` is
the low-level "just release the handle" primitive; any active
rumble / LEDs / tone will persist on the controller until it's
power-cycled or re-paired. Prefer `device_shutdown` unless you
know the device is already gone (e.g. mid-disconnect).

**`device_send_init_packet` on BT.** Until the host sends this
handshake, Bluetooth keeps LED / lightbar / player-LED subsystems
in a "managed by controller" state — every LED write you make is
silently ignored. Rumble and adaptive triggers don't need it, but
LEDs do. Call **once** after a successful `device_open_first`.
No-op on USB (returns `true` immediately).

### Disconnect detection — what the library does for you

`device_read_input` handles **both** transports transparently:

- **USB:** `hid_read` returns -1 on cable yank → library closes the
  handle, `device_read_input` returns -1 → your `bool` return
  becomes `false`.
- **Bluetooth:** once the controller drops off BT (turned off, out of
  range, unpaired), `hid_read` returns 0 **forever** with no error
  signal. The library watches for prolonged silence
  (`BT_SILENCE_DISCONNECT_MS = 2000`) — exceeded → handle closes,
  `device_read_input` returns -1.

Consumers just check the return value; no external silence counter,
no `delta_time` parameter needed.

### Reconnect / hotplug

The library has no built-in reconnect policy — `device_open_first`
walks the HID enumeration once. A typical consumer calls it on an
interval (e.g. every 1 second when `!connected`) until it succeeds.
See `ds_bridge.cpp::ds_poll_registry` in the consumer tree for a
working example.

### Thread safety

Public API is **not thread-safe per Device**. Concurrent calls on the
same `Device*` will race — e.g. a background telemetry loader
reading feature reports while the main thread calls
`device_read_input` → either a torn HID response or, if one thread
closes the handle, a use-after-free on the underlying SDL_hid_device*.
Serialise access externally. The OpenChaos bridge wraps a `std::mutex`
around every `Device*`-touching call and exposes a `DSDebugDeviceLock`
RAII helper for debug code on other threads — see
`ds_bridge.cpp` / `ds_bridge_debug.h` for the pattern.

Different `Device*` values are independent — it's fine to have one
thread per controller.

### Feature reports and test commands

`ds_feature.h` exposes three direct feature-report parsers (firmware
info 0x20, sensor calibration 0x05, BT patch version 0x22). These go
through a single HID round-trip each (~1-20 ms) and are fine to call
from any thread holding the Device lock.

`ds_test.h` implements the "test command" protocol: write a
[deviceId, actionId, ...params] request via feature report 0x80, then
poll feature report 0x81 until a matching response arrives, possibly
reassembling 56-byte chunks until a COMPLETE status lands. The
`test_command()` helper does all of that synchronously with a short
polling budget (~50 ms max per chunk). Cost per call: 5-50 ms
depending on response size and transport.

Two test-command primitives:
- `test_command(..., out_data, out_capacity, out_len)` — writes the
  request AND polls for a response, reassembling chunks. Use this
  for read-style actions ("give me the serial number").
- `test_command_set(...)` — writes the request only, doesn't poll.
  Intended for set-style actions that don't return data. ⚠️
  Empirically some set-style actions (audio WAVEOUT_CTRL, notably)
  are **ignored by firmware** until a 0x81 poll primes the state
  machine — for those, use `test_command()` even though you don't
  care about the response (it'll just time out harmlessly).

### Hardware quirks — what's NOT the library's fault

These behaviours reproduce on other DualSense tools (daidr,
dualsense-tester) too — they're firmware / audio ASIC limits, not
library bugs. Listed so you don't spend time looking for a fix that
doesn't exist on the host side.

- **Audio test tone cross-leaks across outputs.** Routing the 1 kHz
  tone to headphone still leaks audibly into the speaker, and vice
  versa. Especially on BT. Workaround: zero the inactive output's
  volume at UI level.
- **Speaker volume jumps when jack is plugged** and speaker routing
  is active. No host-side fix.
- **Audio test tone can outlive HID handle close.** `device_shutdown`
  does best-effort disable but sometimes the tone keeps playing until
  the controller is unplugged or re-paired.

Full details + more quirks: `own_dualsense_lib_plan.md` §14.

## Licensing

MIT — see `LICENSE`. Some code adapted from third-party MIT projects;
see `THIRD_PARTY_LICENSES.md`.

## Trademark notice

"DualSense" and "PlayStation" are trademarks of Sony Interactive
Entertainment LLC. This library is an independent open-source project
created via reverse engineering of the publicly-documented HID
protocol. It is not affiliated with, endorsed by, or sponsored by Sony
Interactive Entertainment or any of its subsidiaries.
