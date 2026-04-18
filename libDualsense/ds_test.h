#pragma once

// DualSense test commands — feature report 0x80 / 0x81 handshake.
//
// The DualSense exposes a large table of "test device" / "test action"
// pairs through feature reports. Host writes a 0x80 report with
// [deviceId, actionId, params...], then polls 0x81 reports until a
// matching response arrives. Responses may be chunked (56 bytes per
// chunk) and the host concatenates them.
//
// This library exposes **read-only** getters for factory data. Write
// and erase operations (WRITE_EFUSE, WRITE_FACTORY_DATA,
// WRITE_CALIBRATION, BOOTLOADER_*, etc.) are intentionally NOT
// exposed — see review doc §2.4 "Deferred dangerous operations" for
// the list and the hardware-risk reasoning.

#include <cstddef>
#include <cstdint>

namespace oc::dualsense {

struct Device;  // fwd decl

// ---- Test device/action IDs ----------------------------------------
//
// Mirrors the DualSense test command table (daidr/ds.type.ts). Only the
// IDs referenced by the getters below are included here — the full
// table has 200+ entries, most of which are factory-only and deferred.

enum class TestDevice : std::uint8_t {
    System    = 1,
    Power     = 2,
    Memory    = 3,
    Audio     = 6,
    Bluetooth = 9,
    Motion    = 10,
    Led       = 13,
};

namespace TestAction {
    // SYSTEM actions
    constexpr std::uint8_t ReadPcbaId           = 4;
    constexpr std::uint8_t ReadHwVersion        = 6;
    constexpr std::uint8_t ReadFactoryData      = 8;
    constexpr std::uint8_t GetMcuUniqueId       = 9;
    constexpr std::uint8_t ReadPcbaIdFull       = 17;
    constexpr std::uint8_t ReadSerialNumber     = 19;
    constexpr std::uint8_t ReadAssemblePartsInfo = 21;
    constexpr std::uint8_t ReadBatteryBarcode   = 24;
    constexpr std::uint8_t ReadVcmLeftBarcode   = 26;
    constexpr std::uint8_t ReadVcmRightBarcode  = 28;
    constexpr std::uint8_t GetAutoSwitchoffFlag = 32;
    constexpr std::uint8_t GetAlwaysOnStartupState = 23;

    // POWER actions
    constexpr std::uint8_t BatteryVoltage = 6;

    // BLUETOOTH actions
    constexpr std::uint8_t ReadBdAddr = 2;
    constexpr std::uint8_t GetBtEnable = 6;

    // MOTION actions
    constexpr std::uint8_t GetPositionTrackingState = 22;

    // LED actions
    constexpr std::uint8_t GetLedBrightnessAll = 15;
}

// ---- Generic test command ------------------------------------------

enum class TestResult : std::uint8_t {
    Ok = 0,        // data[0..data_len) is valid
    SendFailed,    // could not send the 0x80 report
    PollFailed,    // controller did not respond after max tries
    ControllerRejected, // controller returned FAIL status
};

// Execute a test command and collect the response.
//
// `params` / `params_len` — optional parameter bytes to append after
// deviceId/actionId in the 0x80 write. Pass null/0 if none.
//
// `out_data` is filled with the response body (bytes after the
// [0x81, deviceId, actionId, status] header). `out_capacity` bounds
// the write. `*out_len` receives the number of bytes written. On
// success, data is complete (all chunks concatenated).
//
// Not thread-safe — serialise calls externally if the device is shared.
TestResult test_command(Device* dev,
                        TestDevice device_id,
                        std::uint8_t action_id,
                        const std::uint8_t* params, std::size_t params_len,
                        std::uint8_t* out_data, std::size_t out_capacity,
                        std::size_t* out_len);

// Fire-and-forget variant: sends the 0x80 request (with optional
// params + BT CRC) but does NOT poll the 0x81 response. Intended for
// action IDs that don't return a data payload — typically "set" /
// "control" / "trigger" actions (e.g. the audio test-tone
// WAVEOUT_CTRL / routing pair). Returns `true` if the 0x80 feature
// report write itself succeeded.
//
// Rationale: mirroring daidr's `setTestCommandWithParams` — for write
// actions the controller does not always emit a matching 0x81
// response, and waiting for one just burns the polling timeout.
bool test_command_set(Device* dev,
                      TestDevice device_id,
                      std::uint8_t action_id,
                      const std::uint8_t* params, std::size_t params_len);

// ---- Audio test-tone routing payload -------------------------------
//
// The DualSense built-in 1 kHz test tone generator (started by
// `test_command(Audio, WAVEOUT_CTRL, {enable, 1, 0})`) needs a
// **physical routing decision** sent immediately before each "start":
// should the tone come out of the built-in speaker or the 3.5 mm
// headphone jack?
//
// The routing is a 20-byte payload with sparse non-zero bytes at
// specific offsets, sent via `test_command(Audio,
// BUILTIN_MIC_CALIB_DATA_VERIFY = 4, payload, 20, ...)`. The exact
// byte layout is opaque firmware-level magic (values taken from
// daidr's `ds.util.ts::controlWaveOut`). This helper builds the
// payload so callers don't embed the magic offsets in their own code.
//
// Typical wire recipe for playing a test tone, composed from public
// primitives:
//
//     std::uint8_t route[20];
//     build_waveout_route_payload(WaveOutRoute::Speaker, route);
//     test_command(dev, TestDevice::Audio, /*action=*/4,
//                  route, 20, rx, sizeof(rx), &rx_n);   // routing
//     const std::uint8_t ctrl[3] = { /*enable=*/1, 1, 0 };
//     test_command(dev, TestDevice::Audio, /*action=*/2,
//                  ctrl, 3, rx, sizeof(rx), &rx_n);     // start tone
//
// Stopping the tone needs only the ctrl write with enable=0.
//
// Disclaimer: the tone generator is Sony's factory diagnostic; see §14
// in `own_dualsense_lib_plan.md` for known hardware quirks (cross-leak,
// doesn't stop on HID close, etc.).
enum class WaveOutRoute : std::uint8_t {
    Speaker   = 0,  // built-in speaker (controller grille)
    Headphone = 1,  // 3.5 mm TRRS jack
};

// Fill `out[20]` with the routing payload for the given route. Caller
// is responsible for the surrounding `test_command(Audio, 4, ...)`
// call — this helper only builds the payload bytes.
void build_waveout_route_payload(WaveOutRoute route, std::uint8_t out[20]);

// ---- Convenience getters (read-only) -------------------------------

// MCU unique ID — 64-bit factory identifier (0 on error / not available).
bool get_mcu_unique_id(Device* dev, std::uint64_t* out);

// Bluetooth MAC address — 48-bit value in low 6 bytes of the result
// (big-endian byte order, i.e. out[0] = high byte). Returns true on
// success.
bool get_bd_mac_address(Device* dev, std::uint8_t out[6]);

// PCBA ID — 48-bit factory board identifier (legacy 6-byte form).
// Returns true on success.
bool get_pcba_id(Device* dev, std::uint64_t* out);

// PCBA ID (full, 24 bytes) — newer 24-byte encoded barcode string.
// Returns number of bytes written (always 24 on success, 0 on error).
std::size_t get_pcba_id_full(Device* dev, std::uint8_t out[24]);

// Serial number — 32 bytes, Shift-JIS encoded ASCII-like string.
// Returns bytes written (always 32 on success, 0 on error).
std::size_t get_serial_number(Device* dev, std::uint8_t out[32]);

// Assemble parts info — 32 bytes, opaque factory data.
std::size_t get_assemble_parts_info(Device* dev, std::uint8_t out[32]);

// Battery barcode — 32 bytes, Shift-JIS encoded.
std::size_t get_battery_barcode(Device* dev, std::uint8_t out[32]);

// VCM (Voice Coil Motor, i.e. the adaptive trigger motor) barcodes —
// left and right, 32 bytes each.
std::size_t get_vcm_left_barcode(Device* dev, std::uint8_t out[32]);
std::size_t get_vcm_right_barcode(Device* dev, std::uint8_t out[32]);

// Precise battery voltage. Returns raw value (exact units TBD, daidr
// labels this `BATTERY_VOLTAGE` but doesn't document the scaling;
// typically maps to millivolts or to ADC counts). Returns true on
// success; `*out_raw` receives the raw report bytes concatenated as
// little-endian integer of `*out_raw_byte_count` bytes.
struct BatteryVoltageRaw {
    std::uint8_t  data[8];
    std::uint8_t  len;
    bool          valid;
};
bool get_battery_voltage(Device* dev, BatteryVoltageRaw* out);

// Position tracking feature state (0 = disabled, 1 = enabled, other
// values defined by Sony firmware and not documented).
bool get_position_tracking_state(Device* dev, std::uint8_t* out);

// Always-on-startup flag state.
bool get_always_on_startup_state(Device* dev, std::uint8_t* out);

// Auto-switchoff flag state (0 = off-disabled, 1 = off-enabled).
bool get_auto_switchoff_flag(Device* dev, std::uint8_t* out);

} // namespace oc::dualsense
