// DualSense HID device implementation over SDL3 hidapi.
// Ported from the uc_hid namespace in engine/platform/ds_bridge.cpp.

#include <libDualsense/ds_device.h>

#include <SDL3/SDL_hidapi.h>
#include <SDL3/SDL_hints.h>

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

    SDL_hid_device_info* match = nullptr;
    for (auto* cur = devs; cur; cur = cur->next) {
        if (cur->product_id == DUALSENSE_PID || cur->product_id == DUALSENSE_EDGE_PID) {
            match = cur;
            break;
        }
    }
    if (!match) {
        SDL_hid_free_enumeration(devs);
        return false;
    }

    SDL_hid_device* dev = SDL_hid_open_path(match->path);
    if (!dev) {
        SDL_hid_free_enumeration(devs);
        return false;
    }

    SDL_hid_set_nonblocking(dev, 1);

    out->handle     = static_cast<void*>(dev);
    out->path       = match->path;
    out->connected  = true;
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

} // namespace oc::dualsense
