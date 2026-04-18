// DualSense HID device implementation over SDL3 hidapi.
// Ported from the uc_hid namespace in engine/platform/ds_bridge.cpp.

#include <libDualsense/ds_device.h>

#include <libDualsense/ds_crc.h>
#include <libDualsense/ds_output.h>
#include <libDualsense/ds_test.h>
#include <libDualsense/ds_trigger.h>

#include <SDL3/SDL_hidapi.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_timer.h>

#include <cstdio>
#include <cstring>

namespace oc::dualsense {

// Sony vendor / product IDs
static constexpr std::uint16_t SONY_VENDOR_ID     = 0x054C;
static constexpr std::uint16_t DUALSENSE_PID      = 0x0CE6;
static constexpr std::uint16_t DUALSENSE_EDGE_PID = 0x0DF2;

// DualSense input report sizes over the wire
static constexpr std::size_t INPUT_REPORT_USB_BYTES = 64;
static constexpr std::size_t INPUT_REPORT_BT_BYTES  = 78;

// DualSense output report sizes over the wire. USB = 48 bytes
// (Report ID 0x02 + 47-byte payload per Sony spec). BT = 78 bytes
// (Report ID 0x31 + 1-byte sub-ID + 47-byte payload + 25 reserved + 4-byte CRC32).
static constexpr std::size_t OUTPUT_REPORT_USB_BYTES = 48;
static constexpr std::size_t OUTPUT_REPORT_BT_BYTES  = 78;

static bool s_hid_initialized = false;

bool hid_init()
{
    if (s_hid_initialized) return true;
    // Ask SDL3's joystick subsystem to not claim DualSense — we handle
    // it directly through hidapi.
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "0");
    if (SDL_hid_init() != 0) return false;
    s_hid_initialized = true;
    return true;
}

void hid_shutdown()
{
    if (!s_hid_initialized) return;
    SDL_hid_exit();
    s_hid_initialized = false;
}

bool device_open_first(Device* out)
{
    if (!out || !s_hid_initialized) return false;
    *out = Device{};

    SDL_hid_device_info* devs = SDL_hid_enumerate(SONY_VENDOR_ID, 0);
    if (!devs) return false;

    // DualSense on USB exposes several HID interfaces (gamepad, audio
    // control, touchpad, vendor-defined). Feature reports 0x20 / 0x22 /
    // 0x05 and the 0x80/0x81 test-command state machine are only
    // available on the **gamepad** interface (Generic Desktop / Game
    // Pad, usage_page=0x0001, usage=0x0005). Opening a different HID
    // interface still lets input/output reports flow but every feature
    // report comes back short / -1 / gibberish.
    //
    // We try three strategies in order:
    //   1. usage_page=0x0001 + usage=0x0005 — the canonical descriptor
    //      filter. Works when SDL hidapi populates these fields
    //      (Linux + hidraw always; Windows when the HID parser ran).
    //   2. Open each VID/PID candidate and probe with feature report
    //      0x20. If we get a plausible reply (>= 20 bytes starting
    //      with 0x20), it's the gamepad interface.
    //   3. First VID/PID candidate — same behaviour as before this
    //      filter. Last-resort fallback so we don't regress to "no
    //      device found" on unusual platforms.

    constexpr std::uint16_t GENERIC_DESKTOP_PAGE = 0x0001;
    constexpr std::uint16_t GAME_PAD_USAGE       = 0x0005;

    // Diagnostic: log what we're seeing so the user can paste stderr.log
    // if detection picks the wrong interface. Under normal conditions
    // (correct interface picked on first try) this is a handful of lines.
    {
        int idx = 0;
        for (auto* cur = devs; cur; cur = cur->next) {
            if (cur->product_id != DUALSENSE_PID &&
                cur->product_id != DUALSENSE_EDGE_PID) continue;
            std::fprintf(stderr,
                "[libDualsense] HID candidate #%d: path=%s iface=%d usage_page=0x%04X usage=0x%04X\n",
                idx++,
                cur->path ? cur->path : "<null>",
                cur->interface_number,
                (unsigned)cur->usage_page,
                (unsigned)cur->usage);
        }
    }

    // Strategy 1 — filter by HID usage.
    SDL_hid_device_info* match = nullptr;
    for (auto* cur = devs; cur; cur = cur->next) {
        if (cur->product_id != DUALSENSE_PID &&
            cur->product_id != DUALSENSE_EDGE_PID) continue;
        if (cur->usage_page == GENERIC_DESKTOP_PAGE &&
            cur->usage      == GAME_PAD_USAGE) {
            match = cur;
            break;
        }
    }

    SDL_hid_device* dev = nullptr;
    if (match) {
        std::fprintf(stderr,
            "[libDualsense] opening by usage filter: %s\n",
            match->path ? match->path : "<null>");
        dev = SDL_hid_open_path(match->path);
    }

    // Strategy 2 — probe each candidate by reading feature report 0x20.
    // The gamepad interface returns a populated firmware-info report;
    // other interfaces return -1 or a short nonsensical reply.
    if (!dev) {
        for (auto* cur = devs; cur; cur = cur->next) {
            if (cur->product_id != DUALSENSE_PID &&
                cur->product_id != DUALSENSE_EDGE_PID) continue;
            SDL_hid_device* candidate = SDL_hid_open_path(cur->path);
            if (!candidate) continue;
            std::uint8_t probe[96] = {};
            probe[0] = 0x20;
            const int n = SDL_hid_get_feature_report(candidate, probe, sizeof(probe));
            if (n >= 20 && probe[0] == 0x20) {
                std::fprintf(stderr,
                    "[libDualsense] opening by 0x20 probe: %s (n=%d)\n",
                    cur->path ? cur->path : "<null>", n);
                dev   = candidate;
                match = cur;
                break;
            }
            SDL_hid_close(candidate);
        }
    }

    // Strategy 3 — first VID/PID match, whatever it is.
    if (!dev) {
        for (auto* cur = devs; cur; cur = cur->next) {
            if (cur->product_id == DUALSENSE_PID ||
                cur->product_id == DUALSENSE_EDGE_PID) {
                match = cur;
                break;
            }
        }
        if (match) {
            std::fprintf(stderr,
                "[libDualsense] opening by VID/PID fallback: %s\n",
                match->path ? match->path : "<null>");
            dev = SDL_hid_open_path(match->path);
        }
    }

    if (!dev || !match) {
        SDL_hid_free_enumeration(devs);
        return false;
    }

    SDL_hid_set_nonblocking(dev, 1);

    out->handle        = static_cast<void*>(dev);
    out->path          = match->path;
    out->connected     = true;
    // Seed the BT silence clock with "now" so the first poll doesn't
    // immediately trip the disconnect threshold before any real reports
    // have had a chance to arrive.
    out->last_input_ms = SDL_GetTicks();
    // interface_number == -1 typically means Bluetooth on all SDL backends.
    out->connection = (match->interface_number == -1)
        ? Connection::Bluetooth
        : Connection::Usb;

    if (out->connection == Connection::Bluetooth) {
        out->input_report_strip = 2;
        out->output_report_size = OUTPUT_REPORT_BT_BYTES;
    } else {
        out->input_report_strip = 1;
        out->output_report_size = OUTPUT_REPORT_USB_BYTES;
    }

    SDL_hid_free_enumeration(devs);
    return true;
}

void device_close(Device* dev)
{
    if (!dev) return;
    if (dev->handle) {
        SDL_hid_close(static_cast<SDL_hid_device*>(dev->handle));
    }
    dev->handle    = nullptr;
    dev->connected = false;
    dev->path.clear();
}

int device_read_latest(Device* dev, std::uint8_t* buf, std::size_t buf_capacity)
{
    if (!dev || !dev->handle || !buf || buf_capacity == 0) return -1;
    auto* sd = static_cast<SDL_hid_device*>(dev->handle);

    const std::size_t wire_len = (dev->connection == Connection::Bluetooth)
        ? INPUT_REPORT_BT_BYTES
        : INPUT_REPORT_USB_BYTES;
    if (buf_capacity < wire_len) return -1;

    // Drain the HID queue and keep only the most recent report.
    // Non-blocking reads return 0 when the queue is empty.
    int last = 0;
    int result;
    while ((result = SDL_hid_read(sd, buf, wire_len)) > 0) {
        last = result;
    }
    if (result < 0) {
        // Disconnect / error — close the handle.
        SDL_hid_close(sd);
        dev->handle    = nullptr;
        dev->connected = false;
        return -1;
    }
    return last;
}

int device_write(Device* dev, const std::uint8_t* buf, std::size_t len)
{
    if (!dev || !dev->handle || !buf || len == 0) return -1;
    auto* sd = static_cast<SDL_hid_device*>(dev->handle);

    const int result = SDL_hid_write(sd, buf, len);
    if (result < 0) {
        SDL_hid_close(sd);
        dev->handle    = nullptr;
        dev->connected = false;
        return -1;
    }
    return result;
}

int device_get_feature_report(Device* dev,
                              std::uint8_t report_id,
                              std::uint8_t* buf,
                              std::size_t buf_capacity)
{
    if (!dev || !dev->handle || !buf || buf_capacity < 2) return -1;
    auto* sd = static_cast<SDL_hid_device*>(dev->handle);

    buf[0] = report_id;
    const int result = SDL_hid_get_feature_report(sd, buf, buf_capacity);
    if (result < 0) {
        // Feature report failures are often "report not supported by
        // this device/firmware" and should NOT close the device — the
        // main input/output streams are independent. Return -1 and
        // leave the device open.
        return -1;
    }
    return result;
}

int device_send_feature_report(Device* dev,
                               const std::uint8_t* buf,
                               std::size_t len)
{
    if (!dev || !dev->handle || !buf || len < 2) return -1;
    auto* sd = static_cast<SDL_hid_device*>(dev->handle);

    const int result = SDL_hid_send_feature_report(sd, buf, len);
    if (result < 0) {
        // See device_get_feature_report — failure is not fatal to the
        // input/output streams.
        return -1;
    }
    return result;
}

// --- Graceful shutdown ----------------------------------------------
//
// Magic numbers are deliberately named here — `device_shutdown`'s
// whole point is that consumers shouldn't need to know any of this.
//
// ACTION_WAVEOUT_CTRL = 2: test-command action ID for "start / stop the
//   built-in 1 kHz wave generator". Value taken from daidr's
//   `ds.type.ts::DualSenseTestActionId::WAVEOUT_CTRL`.
// WAVEOUT_CTRL_DISABLE payload: [enable=0, 1, 0] — first byte is the
//   on/off switch, remaining two bytes are opaque frame markers that
//   daidr always sends as 1 and 0 respectively (see `controlWaveOut`).
// SHUTDOWN_TONE_DISABLE_RETRIES / SHUTDOWN_TONE_DISABLE_GAP_MS: empirical
//   — the first disable write often sits in the firmware's test-command
//   queue until a 0x81 poll primes it. One retry with an 80 ms gap
//   covers "waited for queue to drain, now the write actually lands".
// SHUTDOWN_FINAL_LATCH_MS: small delay after the zeroed output report
//   so the controller has time to apply the reset before we drop the
//   HID handle.
static constexpr std::uint8_t  ACTION_WAVEOUT_CTRL              = 2;
static constexpr int           SHUTDOWN_TONE_DISABLE_RETRIES    = 2;
static constexpr std::uint32_t SHUTDOWN_TONE_DISABLE_GAP_MS     = 80;
static constexpr std::uint32_t SHUTDOWN_FINAL_LATCH_MS          = 50;

void device_shutdown(Device* dev)
{
    if (!dev) return;
    if (!dev->connected) return;

    // Step 1: best-effort audio tone disable. See §14.3 — this may or
    // may not stick before the handle closes; we try our best.
    const std::uint8_t disable_ctrl[3] = { 0, 1, 0 };
    for (int i = 0; i < SHUTDOWN_TONE_DISABLE_RETRIES; ++i) {
        std::uint8_t rx[64] = {};
        std::size_t  rx_n   = 0;
        test_command(dev, TestDevice::Audio, ACTION_WAVEOUT_CTRL,
                     disable_ctrl, sizeof(disable_ctrl),
                     rx, sizeof(rx), &rx_n);
        SDL_Delay(SHUTDOWN_TONE_DISABLE_GAP_MS);
    }

    // Step 2: zeroed output — rumble / triggers / lightbar / player LED /
    // mute LED all go dark. `audio_volumes_enabled = false` means we
    // don't re-engage host-managed audio mode on our way out.
    OutputState quiet = {};
    trigger_off(quiet.trigger_left);
    trigger_off(quiet.trigger_right);
    device_send_output(dev, quiet);

    // Step 3: latch gap so the zeroed state lands before handle close.
    SDL_Delay(SHUTDOWN_FINAL_LATCH_MS);

    // Step 4: actually close the handle.
    device_close(dev);
}

bool device_send_init_packet(Device* dev)
{
    if (!dev || !dev->connected) return false;
    if (dev->connection != Connection::Bluetooth) {
        // USB does not need the handshake.
        return true;
    }

    // BT init packet layout:
    //   buf[0]       0x31   Report ID
    //   buf[1]       0x00   initial seq (high nibble = seq, starts at 0)
    //   buf[2]       0x10   magic
    //   buf[3..49]   47-byte payload with all validFlag bits set
    //                  (validFlag0 at p[0], validFlag1 at p[1],
    //                   validFlag2 at p[38] — see ds_output.cpp for
    //                   the payload layout).
    //                  Payload bytes other than validFlag triplet stay
    //                  zero — this is a "no-op state, but please hand
    //                  over control" signal, not a real output packet.
    //   buf[50..73]  reserved zero
    //   buf[74..77]  CRC32 (prefix 0xA2 baked into crc32_compute's seed)
    std::uint8_t buf[OUTPUT_REPORT_BT_BYTES] = {};
    buf[0] = 0x31;
    buf[1] = 0x00;
    buf[2] = 0x10;
    buf[3 + 0]  = 0xFF;   // validFlag0 — all enables
    buf[3 + 1]  = 0xFF;   // validFlag1 — all enables (bit 3
                          //   "ReleaseLedsDefault" is safe to set on
                          //   init; subsequent packets clear it).
    buf[3 + 38] = 0xFF;   // validFlag2 — all enables

    const std::uint32_t crc = crc32_compute(buf, 74);
    buf[74] = static_cast<std::uint8_t>((crc >>  0) & 0xFF);
    buf[75] = static_cast<std::uint8_t>((crc >>  8) & 0xFF);
    buf[76] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
    buf[77] = static_cast<std::uint8_t>((crc >> 24) & 0xFF);

    return device_write(dev, buf, sizeof(buf)) > 0;
}

} // namespace oc::dualsense
