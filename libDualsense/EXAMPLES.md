# libDualsense — Examples

One short example per API area. Snippets assume `using namespace
oc::dualsense;` and a valid `Device dev` where applicable; add your
own `#include` directives and error handling as appropriate.

For range / units / gotchas — see [`README.md`](README.md).
For function-by-function reference — see [`API.md`](API.md).

---

## 1. Device lifecycle

```cpp
#include <libDualsense/ds_device.h>
#include <libDualsense/ds_input.h>
#include <libDualsense/ds_output.h>

using namespace oc::dualsense;

// Startup
if (!hid_init()) { /* SDL hidapi init failed */ return 1; }

Device dev;
if (!device_open_first(&dev)) {
    // No DualSense plugged in. You can retry later on a timer.
} else {
    // BT handshake — required before LEDs apply on Bluetooth.
    // No-op on USB, safe to call unconditionally.
    device_send_init_packet(&dev);
}

// ... your per-frame loop here ...

// Graceful shutdown — stops any audio tone, zeros rumble / LEDs /
// lightbar / triggers, then closes the handle.
device_shutdown(&dev);
hid_shutdown();
```

Use `device_close(&dev)` instead of `device_shutdown` only when you
don't need the cleanup (e.g. the disconnect already happened and the
device is gone).

---

## 2. Reading input — one frame

```cpp
InputState in = {};
int n = device_read_input(&dev, &in);
if (n < 0) {
    // Disconnected — device_read_input already closed the handle.
    // Schedule a reconnect attempt.
    return;
}
if (n == 0) {
    // No new report this frame. `in` unchanged; reuse last-known state.
    return;
}

// New report — `in` is fresh.
// Sticks: raw uint8 0..255 with centre at 128.
float lx = normalize_stick_axis(in.left_stick_x);  // clamps to [-1, +1]
float ly = normalize_stick_axis(in.left_stick_y);  // Y grows DOWN
// Flip Y if your game treats "up" as positive:
ly = -ly;

// Triggers: raw uint8 0..255.
float rt = normalize_trigger(in.right_trigger);    // [0, 1]

// Buttons are plain bools.
bool shoot = in.r2;                                // digital threshold
bool jump  = in.cross;

// Battery % — already decoded.
int battery = in.battery_level_percent;            // 0..100
bool charging = in.battery_charging;
```

---

## 3. Rumble and lightbar

```cpp
OutputState out = {};
// Rumble — low/high motor amplitudes, raw 0..255.
// "left" is the LOW-frequency motor, "right" is HIGH (DS4 convention).
out.rumble_left  = 180;
out.rumble_right = 80;

// Lightbar RGB, 0..255 per channel.
out.lightbar_r = 255;
out.lightbar_g = 0;
out.lightbar_b = 64;

device_send_output(&dev, out);
```

Send a cleared OutputState when you want rumble to stop — the lib
doesn't auto-decay, it just writes whatever you give it.

---

## 4. Player LEDs (mask + brightness)

```cpp
OutputState out = {};

// Hardware has 5 LEDs but mirrors bits 0↔4 and 1↔3 — use the named
// constants, not raw bits.
out.player_led_mask = PlayerLed::Center | PlayerLed::Inner;
// Equivalent predefined patterns: PlayerLed::Off / Outer / All.

// ⚠️ COUNTER-INTUITIVE: 0 = brightest, 2 = dimmest.
out.player_led_brightness = 0;

device_send_output(&dev, out);
```

---

## 5. Mic mute LED and haptic volume

```cpp
OutputState out = {};
out.mute_led = MuteLed::On;        // Off / On (steady) / Blink

// Overall rumble gain on top of per-motor amplitude.
// Raw 0..7 (3-bit field), NOT a percent.
out.haptic_volume = 5;

device_send_output(&dev, out);
```

---

## 6. Audio output — opt-in

```cpp
OutputState out = {};

// Opt in before any volume / route field applies.
out.audio_volumes_enabled = true;
out.audio_route   = OutputState::AudioRoute::Speaker;  // or Headphone
out.speaker_volume   = 200;
out.headphone_volume = 0;      // silence the inactive output if you
                               // care about acoustic cross-leak

device_send_output(&dev, out);

// To stop driving audio, set audio_volumes_enabled = false. Note that
// the controller will keep whatever volume was last written — toggling
// the flag does NOT restore pre-takeover state.
```

---

## 7. Adaptive trigger effects — full-name variants

All factory functions write into `OutputState::trigger_left[10]` or
`trigger_right[10]`. `hand` convention in OpenChaos's ds_bridge is
0=left, 1=right, 2=both — but at the lib level you pick the slot
yourself.

```cpp
OutputState out = {};

// Single mechanical click between zones 3 and 5, strength 6 (0..8).
trigger_weapon(out.trigger_right, /*start=*/3, /*end=*/5, /*strength=*/6);

// Continuous resistance from zone 4, strength 5 (0..8).
trigger_feedback(out.trigger_left, /*position=*/4, /*strength=*/5);

// Vibration from zone 0 onwards, amplitude 6, 40 Hz.
trigger_vibration(out.trigger_right, /*position=*/0, /*amplitude=*/6,
                  /*frequency_hz=*/40);

// Bow: draw resistance zones 2→6, strength 6, snap-back 7.
trigger_bow(out.trigger_right, 2, 6, /*strength=*/6, /*snap=*/7);

// Machine (two-beat): zones 1→8, amplitudes 7 & 3, 8 Hz, period 80.
trigger_machine(out.trigger_left, 1, 8, 7, 3, /*freq=*/8, /*period=*/80);

// Galloping: zones 0→9, foot positions 2 & 5 inside the cycle, 20 Hz.
trigger_galloping(out.trigger_right, 0, 9, 2, 5, /*freq=*/20);

// Clear any effect.
trigger_off(out.trigger_left);

device_send_output(&dev, out);
```

Zones are 0..9 along trigger travel (10 discrete positions). Strength
/ amplitude typically 0..8. See `ds_trigger.h` for exact per-effect
parameter ranges.

---

## 8. Adaptive trigger effects — simple / limited (raw bytes)

```cpp
OutputState out = {};

// Simple Feedback: raw-byte position + strength, NOT zone indices.
trigger_simple_feedback(out.trigger_left, /*pos=*/128, /*str=*/200);

// Simple Weapon: raw click zone boundaries, raw strength.
trigger_simple_weapon(out.trigger_right, /*start=*/64, /*end=*/160, /*str=*/255);

// Simple Vibration: raw position + amplitude + frequency.
trigger_simple_vibration(out.trigger_right, 0, 128, 40);

// Limited Feedback: raw position + strength 0..10 (narrower range).
trigger_limited_feedback(out.trigger_left, /*pos=*/128, /*str=*/5);

// Limited Weapon: start_raw ≥ 0x10, (end - start) ≤ 100, str 0..10.
trigger_limited_weapon(out.trigger_right, 0x40, 0x80, /*str=*/8);

device_send_output(&dev, out);
```

⚠️ **Simple / Limited variants do NOT report trigger feedback status.**
Their `*_trigger_effect_active` stays `false` and the low nibble of
`*_trigger_feedback` stays `0` regardless of actual trigger state.
Only full-name effects (section 7) report.

---

## 9. Reading trigger feedback status

```cpp
InputState in = {};
if (device_read_input(&dev, &in) > 0) {
    // Pre-decoded by the library:
    bool engaged  = in.right_trigger_effect_active;   // bit 4
    uint8_t state = in.right_trigger_feedback & 0x0F; // low nibble

    // `engaged` is the hardware-detected moment you're inside a Weapon
    // effect's click zone — ideal for "moment of fire" detection.
    if (engaged) {
        // trigger just crossed into the click zone
    }

    // `state` (3..9 typical during a Weapon effect) is the motor's
    // internal state-machine value. Usage is effect-specific; for most
    // consumers the `engaged` bool is what you want.
}
```

---

## 10. IMU — raw read

```cpp
InputState in = {};
if (device_read_input(&dev, &in) > 0) {
    // Raw int16 device units — relative motion / gestures.
    int16_t gyro_yaw   = in.gyro_yaw;
    int16_t accel_x    = in.accel_x;
    uint32_t ts_us     = in.motion_timestamp;   // wraps at uint32 max
    int8_t temperature = in.motion_temperature; // raw int8
}
```

---

## 11. IMU — calibrated read (deg/s, g)

```cpp
#include <libDualsense/ds_feature.h>
#include <libDualsense/ds_calibration.h>

// Fetch factory calibration ONCE per device session (feature 0x05).
SensorCalibration calib;
if (!get_sensor_calibration(&dev, &calib) || !calib.valid) {
    // Calibration unavailable — fall back to raw int16.
}

// In your per-frame loop:
InputState in = {};
if (device_read_input(&dev, &in) > 0 && calib.valid) {
    float yaw_dps   = calibrate_gyro_yaw_deg_per_sec(in.gyro_yaw, calib);
    float pitch_dps = calibrate_gyro_pitch_deg_per_sec(in.gyro_pitch, calib);
    float roll_dps  = calibrate_gyro_roll_deg_per_sec(in.gyro_roll, calib);

    float ax_g = calibrate_accel_x_g(in.accel_x, calib);
    float ay_g = calibrate_accel_y_g(in.accel_y, calib);
    float az_g = calibrate_accel_z_g(in.accel_z, calib);
    // az_g ≈ -1.0 when controller is face-up and still
}
```

Calibration is per-device — don't reuse one `SensorCalibration` across
different physical controllers.

---

## 12. Firmware info

```cpp
FirmwareInfo fw;
if (get_firmware_info(&dev, &fw) && fw.valid) {
    // Strings are null-terminated.
    printf("Built: %s %s\n", fw.buildDate, fw.buildTime);
    printf("HW:    0x%08X (board gen %u)\n",
           fw.hwInfo, fw.hwInfo & 0xFFFFu);
    printf("Main:  %u.%u.%u\n",
           (fw.mainFwVersion >> 24) & 0xFF,
           (fw.mainFwVersion >> 16) & 0xFF,
           fw.mainFwVersion & 0xFFFF);
}
```

---

## 13. Bluetooth patch version

```cpp
uint32_t bt_patch = 0;
if (get_bt_patch_version(&dev, &bt_patch)) {
    printf("BT patch: 0x%08X\n", bt_patch);
}
// Available only when hwInfo & 0xFFFF >= 777.
```

---

## 14. Factory data — test-command getters

All getters share the same "request-and-poll" protocol internally.
Each call is a synchronous HID round-trip (5-50 ms).

```cpp
#include <libDualsense/ds_test.h>

// 64-bit MCU unique ID
uint64_t mcu = 0;
if (get_mcu_unique_id(&dev, &mcu)) {
    printf("MCU: 0x%016llX\n", (unsigned long long)mcu);
}

// 6-byte BT MAC (big-endian order: out[0] = high byte for printing)
uint8_t mac[6];
if (get_bd_mac_address(&dev, mac)) {
    printf("BT MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// 32-byte serial (Shift-JIS raw bytes — decode as Shift-JIS if you
// want readable text; ASCII subset prints fine as-is).
uint8_t serial[32];
size_t n = get_serial_number(&dev, serial);
if (n > 0) { /* use serial[0..n) */ }

// Factory barcodes (also Shift-JIS raw, 32 bytes each)
uint8_t batt_bc[32], vcm_l[32], vcm_r[32];
get_battery_barcode(&dev, batt_bc);
get_vcm_left_barcode(&dev, vcm_l);
get_vcm_right_barcode(&dev, vcm_r);

// PCBA IDs
uint64_t pcba = 0;
get_pcba_id(&dev, &pcba);         // legacy 48-bit form
uint8_t pcba_full[24];
get_pcba_id_full(&dev, pcba_full); // modern 24-byte form (newer HW)

// Battery voltage (format is TBD — raw wire bytes only)
BatteryVoltageRaw bv;
if (get_battery_voltage(&dev, &bv) && bv.valid) {
    /* bv.data[0..bv.len) is the raw response */
}

// System state flags
uint8_t tracking = 0, always_on = 0, auto_off = 0;
get_position_tracking_state(&dev, &tracking);
get_always_on_startup_state(&dev, &always_on);
get_auto_switchoff_flag(&dev, &auto_off);
```

---

## 15. Generic test command (advanced)

```cpp
// Any custom deviceId / actionId combination — used internally by
// all the getters above. `params` appended after the deviceId/actionId
// bytes in the 0x80 request; response body (without the 4-byte header)
// copied into out_buf.
uint8_t out_buf[64];
size_t  out_len = 0;
TestResult r = test_command(&dev,
                            TestDevice::System, /*action=*/21,
                            /*params=*/nullptr, /*params_len=*/0,
                            out_buf, sizeof(out_buf), &out_len);
if (r == TestResult::Ok) {
    /* out_buf[0..out_len) is the response payload */
}

// Fire-and-forget variant — sends the 0x80 request without polling
// for a 0x81 response. Faster, but some firmware actions (e.g. audio
// WAVEOUT_CTRL) ignore fire-only writes until a poll primes them;
// for those, use test_command() above and ignore the PollFailed.
bool sent = test_command_set(&dev, TestDevice::Bluetooth,
                             /*action=*/6, /*params=*/nullptr, 0);
```

---

## 16. Audio test tone (factory diagnostic)

```cpp
#include <libDualsense/ds_test.h>

constexpr uint8_t ACTION_ROUTE_PREPARE = 4;   // BUILTIN_MIC_CALIB_DATA_VERIFY
constexpr uint8_t ACTION_WAVEOUT_CTRL  = 2;

// 1. Pick physical output (speaker or jack) — 20-byte payload built
//    by the library; consumer doesn't embed byte offsets.
uint8_t route[20];
build_waveout_route_payload(WaveOutRoute::Speaker, route);

uint8_t rx[64]; size_t rx_n = 0;
test_command(&dev, TestDevice::Audio, ACTION_ROUTE_PREPARE,
             route, sizeof(route), rx, sizeof(rx), &rx_n);

// 2. Start the 1 kHz tone.
const uint8_t start_tone[3] = { /*enable=*/1, 1, 0 };
test_command(&dev, TestDevice::Audio, ACTION_WAVEOUT_CTRL,
             start_tone, sizeof(start_tone), rx, sizeof(rx), &rx_n);

// ... later ...

// 3. Stop the tone.
const uint8_t stop_tone[3] = { 0, 1, 0 };
test_command(&dev, TestDevice::Audio, ACTION_WAVEOUT_CTRL,
             stop_tone, sizeof(stop_tone), rx, sizeof(rx), &rx_n);
```

Factory diagnostic — NOT a production audio path. Cross-leaks, doesn't
always stop on HID close (§14 in `own_dualsense_lib_plan.md`).
`device_shutdown` makes a best-effort disable on exit.

---

## 17. CRC32 — low-level (rarely needed)

Output and feature reports both carry CRC32 on Bluetooth. `device_send_output`
and `send_test_request` append it automatically; direct use is only
relevant if you're building a custom wire-level tool.

```cpp
#include <libDualsense/ds_crc.h>

// BT output report (prefix 0xA2 is baked into this function).
uint32_t crc_out = crc32_compute(buffer, 74);

// BT feature report (prefix 0x53 + reportId).
uint32_t crc_feat = crc32_compute_feature_report(/*report_id=*/0x80,
                                                  data, data_len);
```

---

## 18. Low-level I/O — power-user path

For wire-level debugging, packet capture, or implementing features
the high-level API doesn't cover.

```cpp
// Raw read — no queue drain logic, caller handles framing strip.
uint8_t raw[96];
int n = device_read_latest(&dev, raw, sizeof(raw));
if (n > 0) {
    InputState in = {};
    parse_input_report(raw + dev.input_report_strip, &in);
}

// Raw write — caller builds the wire buffer.
uint8_t out_buf[96] = {};
OutputState state = {};
state.rumble_left = 100;
bool bluetooth = (dev.connection == Connection::Bluetooth);
build_output_report(state, out_buf, bluetooth);
device_write(&dev, out_buf, dev.output_report_size);

// Direct feature report access
uint8_t feat_buf[64] = {};
int got = device_get_feature_report(&dev, /*report_id=*/0x20,
                                    feat_buf, sizeof(feat_buf));
```

---

## 19. Full typical integration (frame loop)

```cpp
// One-time
hid_init();
Device dev;
bool opened = device_open_first(&dev);
if (opened) device_send_init_packet(&dev);

// Optional: load IMU calibration once per session
SensorCalibration calib = {};
bool calib_ok = opened && get_sensor_calibration(&dev, &calib)
                       && calib.valid;

// Per-frame (assumes your app has a dt and a polled state)
while (running) {
    if (!dev.connected) {
        // Try to reconnect every second (or on your own cadence)
        if (time_to_reconnect()) {
            if (device_open_first(&dev)) {
                device_send_init_packet(&dev);
                calib_ok = get_sensor_calibration(&dev, &calib)
                        && calib.valid;
            }
        }
    }

    // Input
    InputState in = {};
    int n = device_read_input(&dev, &in);
    if (n > 0) {
        // Fresh state — wire it into game input
        float lx = normalize_stick_axis(in.left_stick_x);
        float ly = -normalize_stick_axis(in.left_stick_y);  // flip Y
        bool fire = in.r2;
        // ...
    } else if (n < 0) {
        // Disconnected — handle already closed; reconnect loop above
        // will re-open next frame.
    }

    // Output (only if something changed — lib doesn't diff for you)
    static OutputState out = {};
    static OutputState prev = {};
    out.rumble_left = compute_rumble_left();
    out.lightbar_r  = compute_lightbar_r();
    // ... etc ...
    if (std::memcmp(&out, &prev, sizeof(out)) != 0) {
        device_send_output(&dev, out);
        prev = out;
    }
}

// Shutdown
device_shutdown(&dev);
hid_shutdown();
```

---

## 20. Thread safety

Public API is **not thread-safe per Device**. If you share a `Device*`
across threads (e.g. a main thread reading input and a background
thread loading telemetry), serialise access:

```cpp
std::mutex dev_mutex;

// Main thread
{
    std::lock_guard<std::mutex> lk(dev_mutex);
    device_read_input(&dev, &in);
    device_send_output(&dev, out);
}

// Background thread
{
    std::lock_guard<std::mutex> lk(dev_mutex);
    get_firmware_info(&dev, &fw);
    get_mcu_unique_id(&dev, &mcu);
}
```

Take the lock for each individual HID call (not across a whole
sequence) so the main thread isn't starved during long telemetry
loads. Different `Device*` values are independent — one thread per
controller is fine without a shared lock.
