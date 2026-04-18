// DualSense test command implementation.
//
// Protocol — for each test command:
//   1. Host sends feature report 0x80 with [deviceId, actionId, params...].
//      On Bluetooth the last 4 bytes must be the CRC32 (prefix 0x53).
//   2. Host polls feature report 0x81 repeatedly, filtering for reports
//      that match [0x81, deviceId, actionId, status].
//   3. Status codes (from daidr TestStatus enum):
//        0 = IDLE
//        1 = RUNNING
//        2 = COMPLETE      — data is final, read `out_capacity` bytes from offset 4
//        3 = COMPLETE_2    — 56-byte chunk, keep polling for more chunks
//        255 = TIMEOUT
//   4. If chunked, host concatenates 56-byte slices from each COMPLETE_2
//      response until a final COMPLETE arrives with the tail slice.
//
// This implementation fetches all data synchronously with short sleeps
// between 0x81 polls. Typical total latency is 10–50 ms for short
// responses.

#include <libDualsense/ds_test.h>

#include <libDualsense/ds_crc.h>
#include <libDualsense/ds_device.h>

#include <SDL3/SDL_timer.h>

#include <cstring>

namespace oc::dualsense {

// Wire sizes. On USB the 0x80 feature report is 64 bytes; on BT the
// report descriptor reserves room for the 4-byte CRC, giving 68 bytes
// total. 0x81 read response follows the same size on each transport.
//
// Values match daidr (via HID descriptor) and are consistent with
// DS5W / nondebug RE notes.
static constexpr std::size_t FEATURE_80_SIZE_USB = 64;
static constexpr std::size_t FEATURE_80_SIZE_BT  = 68;
static constexpr std::size_t FEATURE_81_SIZE_USB = 64;
static constexpr std::size_t FEATURE_81_SIZE_BT  = 68;

// Polling parameters for 0x81. Controller usually responds in 1-2 ms
// per chunk; polling every 2 ms with 100 retries gives ~200 ms total
// which comfortably covers any reasonable response including chunked.
static constexpr std::uint32_t POLL_SLEEP_MS  = 2;
static constexpr int           POLL_MAX_TRIES = 100;

// Response format:
//   buf[0] = 0x81
//   buf[1] = deviceId (echo of request)
//   buf[2] = actionId (echo of request)
//   buf[3] = status (TestStatus value)
//   buf[4..] = data payload (up to 56 bytes per chunk)
static constexpr std::size_t RESPONSE_HEADER_SIZE = 4;
static constexpr std::size_t CHUNK_DATA_SIZE      = 56;

enum TestStatusCode : std::uint8_t {
    TS_IDLE       = 0,
    TS_RUNNING    = 1,
    TS_COMPLETE   = 2,
    TS_COMPLETE_2 = 3,
    TS_TIMEOUT    = 255,
};

// Internal: send a 0x80 request with given deviceId/actionId and
// optional param bytes. Handles transport-specific sizing and CRC.
static bool send_test_request(Device* dev,
                              std::uint8_t device_id,
                              std::uint8_t action_id,
                              const std::uint8_t* params,
                              std::size_t params_len)
{
    const bool bt = (dev->connection == Connection::Bluetooth);
    const std::size_t wire_size = bt ? FEATURE_80_SIZE_BT : FEATURE_80_SIZE_USB;

    // Feature report 0x80 layout:
    //   buf[0]     = reportId 0x80
    //   buf[1]     = deviceId
    //   buf[2]     = actionId
    //   buf[3..]   = params (zero-padded to wire_size; last 4 bytes
    //                reserved for CRC on BT)
    std::uint8_t buf[96] = {};
    if (wire_size > sizeof(buf)) return false;
    if (params_len > wire_size - (bt ? 7u : 3u)) return false;

    buf[0] = 0x80;
    buf[1] = device_id;
    buf[2] = action_id;
    if (params && params_len > 0) {
        std::memcpy(&buf[3], params, params_len);
    }

    if (bt) {
        // CRC32 over [0x53, 0x80, buf[1..wire_size-4]]. Data passed
        // to crc32_compute_feature_report is the buffer *contents*
        // after reportId (the helper adds the 0x53+reportId prefix).
        const std::uint32_t crc = crc32_compute_feature_report(
            0x80, &buf[1], wire_size - 1u - 4u);
        buf[wire_size - 4] = static_cast<std::uint8_t>((crc >>  0) & 0xFF);
        buf[wire_size - 3] = static_cast<std::uint8_t>((crc >>  8) & 0xFF);
        buf[wire_size - 2] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        buf[wire_size - 1] = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
    }

    return device_send_feature_report(dev, buf, wire_size) > 0;
}

// Internal: poll 0x81 until a response matching (device_id, action_id)
// arrives. Returns the status byte (TS_*), or a sentinel on failure.
static std::uint8_t poll_test_response(Device* dev,
                                        std::uint8_t device_id,
                                        std::uint8_t action_id,
                                        std::uint8_t* out_buf,
                                        std::size_t out_capacity,
                                        std::size_t* out_n)
{
    const bool bt = (dev->connection == Connection::Bluetooth);
    const std::size_t wire_size = bt ? FEATURE_81_SIZE_BT : FEATURE_81_SIZE_USB;
    if (out_capacity < wire_size) return TS_TIMEOUT;

    for (int tries = 0; tries < POLL_MAX_TRIES; ++tries) {
        const int n = device_get_feature_report(dev, 0x81, out_buf, out_capacity);
        if (n >= static_cast<int>(RESPONSE_HEADER_SIZE)
            && out_buf[0] == 0x81
            && out_buf[1] == device_id
            && out_buf[2] == action_id)
        {
            const std::uint8_t status = out_buf[3];
            if (status == TS_COMPLETE || status == TS_COMPLETE_2) {
                *out_n = static_cast<std::size_t>(n);
                return status;
            }
        }
        SDL_Delay(POLL_SLEEP_MS);
    }
    return TS_TIMEOUT;
}

TestResult test_command(Device* dev,
                        TestDevice device_id,
                        std::uint8_t action_id,
                        const std::uint8_t* params, std::size_t params_len,
                        std::uint8_t* out_data, std::size_t out_capacity,
                        std::size_t* out_len)
{
    if (!dev || !dev->handle || !out_data || !out_len) return TestResult::SendFailed;
    *out_len = 0;

    const std::uint8_t did = static_cast<std::uint8_t>(device_id);
    if (!send_test_request(dev, did, action_id, params, params_len)) {
        return TestResult::SendFailed;
    }

    std::uint8_t rx[96];
    std::size_t  rx_n = 0;
    std::size_t  written = 0;

    // Chunked-response loop. Each COMPLETE_2 yields 56 data bytes; a
    // COMPLETE yields the final (possibly short) chunk. Stop at
    // COMPLETE.
    for (;;) {
        const std::uint8_t status = poll_test_response(dev, did, action_id,
                                                        rx, sizeof(rx), &rx_n);
        if (status == TS_TIMEOUT) return TestResult::PollFailed;

        // Copy payload from this chunk.
        const std::size_t payload = (rx_n > RESPONSE_HEADER_SIZE)
            ? rx_n - RESPONSE_HEADER_SIZE
            : 0u;
        std::size_t copy = payload;
        if (status == TS_COMPLETE_2) {
            // Mid-stream chunk: always the full chunk size of data.
            copy = (payload < CHUNK_DATA_SIZE) ? payload : CHUNK_DATA_SIZE;
        }
        if (written + copy > out_capacity) {
            copy = out_capacity - written;
        }
        if (copy > 0) {
            std::memcpy(&out_data[written], &rx[RESPONSE_HEADER_SIZE], copy);
            written += copy;
        }

        if (status == TS_COMPLETE) break;
        if (written >= out_capacity) break;
    }

    *out_len = written;
    return TestResult::Ok;
}

// ---- Convenience getters -------------------------------------------

bool get_mcu_unique_id(Device* dev, std::uint64_t* out)
{
    if (!out) return false;
    *out = 0;
    std::uint8_t buf[16] = {};
    std::size_t n = 0;
    if (test_command(dev, TestDevice::System, TestAction::GetMcuUniqueId,
                     nullptr, 0, buf, sizeof(buf), &n) != TestResult::Ok)
        return false;
    // Response: buf[0] = 0x00 status byte, buf[1..8] = 64-bit ID (LE).
    if (n < 9 || buf[0] != 0) return false;
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<std::uint64_t>(buf[1 + i]) << (i * 8);
    }
    *out = v;
    return true;
}

bool get_bd_mac_address(Device* dev, std::uint8_t out[6])
{
    if (!out) return false;
    std::memset(out, 0, 6);
    std::uint8_t buf[16] = {};
    std::size_t n = 0;
    if (test_command(dev, TestDevice::Bluetooth, TestAction::ReadBdAddr,
                     nullptr, 0, buf, sizeof(buf), &n) != TestResult::Ok)
        return false;
    if (n < 6) return false;
    // Report returns 6 little-endian bytes (byte 0 = low byte). Present
    // as big-endian (high byte first) which is how MAC addresses are
    // usually printed: XX:XX:XX:XX:XX:XX.
    for (int i = 0; i < 6; ++i) out[i] = buf[5 - i];
    return true;
}

bool get_pcba_id(Device* dev, std::uint64_t* out)
{
    if (!out) return false;
    *out = 0;
    std::uint8_t buf[8] = {};
    std::size_t n = 0;
    if (test_command(dev, TestDevice::System, TestAction::ReadPcbaId,
                     nullptr, 0, buf, sizeof(buf), &n) != TestResult::Ok)
        return false;
    if (n < 6) return false;
    std::uint64_t v = 0;
    for (int i = 0; i < 6; ++i) {
        v |= static_cast<std::uint64_t>(buf[i]) << (i * 8);
    }
    *out = v;
    return true;
}

static std::size_t get_fixed_length_data(Device* dev,
                                          TestDevice dev_id,
                                          std::uint8_t action_id,
                                          std::uint8_t* out,
                                          std::size_t expected_len)
{
    std::size_t n = 0;
    if (test_command(dev, dev_id, action_id,
                     nullptr, 0, out, expected_len, &n) != TestResult::Ok)
        return 0;
    return n;
}

std::size_t get_pcba_id_full(Device* dev, std::uint8_t out[24])
{
    return get_fixed_length_data(dev, TestDevice::System,
                                 TestAction::ReadPcbaIdFull, out, 24);
}

std::size_t get_serial_number(Device* dev, std::uint8_t out[32])
{
    return get_fixed_length_data(dev, TestDevice::System,
                                 TestAction::ReadSerialNumber, out, 32);
}

std::size_t get_assemble_parts_info(Device* dev, std::uint8_t out[32])
{
    return get_fixed_length_data(dev, TestDevice::System,
                                 TestAction::ReadAssemblePartsInfo, out, 32);
}

std::size_t get_battery_barcode(Device* dev, std::uint8_t out[32])
{
    return get_fixed_length_data(dev, TestDevice::System,
                                 TestAction::ReadBatteryBarcode, out, 32);
}

std::size_t get_vcm_left_barcode(Device* dev, std::uint8_t out[32])
{
    return get_fixed_length_data(dev, TestDevice::System,
                                 TestAction::ReadVcmLeftBarcode, out, 32);
}

std::size_t get_vcm_right_barcode(Device* dev, std::uint8_t out[32])
{
    return get_fixed_length_data(dev, TestDevice::System,
                                 TestAction::ReadVcmRightBarcode, out, 32);
}

bool get_battery_voltage(Device* dev, BatteryVoltageRaw* out)
{
    if (!out) return false;
    *out = BatteryVoltageRaw{};
    std::uint8_t buf[8] = {};
    std::size_t  n      = 0;
    if (test_command(dev, TestDevice::Power, TestAction::BatteryVoltage,
                     nullptr, 0, buf, sizeof(buf), &n) != TestResult::Ok)
        return false;
    if (n == 0 || n > sizeof(out->data)) return false;
    std::memcpy(out->data, buf, n);
    out->len   = static_cast<std::uint8_t>(n);
    out->valid = true;
    return true;
}

static bool get_single_byte_state(Device* dev, TestDevice did,
                                   std::uint8_t action_id, std::uint8_t* out)
{
    if (!out) return false;
    *out = 0;
    std::uint8_t buf[4] = {};
    std::size_t n = 0;
    if (test_command(dev, did, action_id,
                     nullptr, 0, buf, sizeof(buf), &n) != TestResult::Ok)
        return false;
    if (n < 1) return false;
    *out = buf[0];
    return true;
}

bool get_position_tracking_state(Device* dev, std::uint8_t* out)
{
    return get_single_byte_state(dev, TestDevice::Motion,
                                 TestAction::GetPositionTrackingState, out);
}

bool get_always_on_startup_state(Device* dev, std::uint8_t* out)
{
    return get_single_byte_state(dev, TestDevice::System,
                                 TestAction::GetAlwaysOnStartupState, out);
}

bool get_auto_switchoff_flag(Device* dev, std::uint8_t* out)
{
    return get_single_byte_state(dev, TestDevice::System,
                                 TestAction::GetAutoSwitchoffFlag, out);
}

} // namespace oc::dualsense
