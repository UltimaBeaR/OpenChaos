#pragma once

// DualSense HID device: discovery, open/close, non-blocking I/O.
// All HID transport goes through SDL3 hidapi (SDL_hid_*), which is
// already an OpenChaos dependency — no extra libs needed.

#include <cstddef>
#include <cstdint>
#include <string>

namespace oc::dualsense {

enum class Connection : std::uint8_t {
    Usb       = 0,
    Bluetooth = 1,
};

// Opened DualSense device. POD with an SDL_hid_device* stored as
// void* to keep SDL3 headers out of ds_device.h.
struct Device {
    void*       handle        = nullptr;  // SDL_hid_device*
    std::string path;
    Connection  connection    = Connection::Usb;
    bool        connected     = false;

    // USB reports are 64 bytes, Bluetooth 78 bytes. Size of the
    // Report ID / BT framing that must be stripped before feeding
    // the buffer into parse_input_report().
    //
    //   USB: input reports start with 0x01, strip 1 byte → parser sees offset 0
    //   BT:  input reports start with 0x31 + 1 framing byte, strip 2 bytes
    std::uint8_t input_report_strip  = 1;

    // Output report total size (including Report ID / BT framing).
    // USB = 48 bytes, BT = 78 bytes.
    std::size_t  output_report_size  = 48;  // DualSense USB default
};

// Initialize SDL hidapi subsystem. Safe to call multiple times.
// Returns false if SDL_hid_init() fails.
bool hid_init();

// Shutdown the hidapi subsystem.
void hid_shutdown();

// Find the first connected DualSense (VID=0x054C, PID=0x0CE6 or
// 0x0DF2 for Edge) and open it. On success `out` is populated.
// Returns true iff a device was successfully opened.
bool device_open_first(Device* out);

// Close and invalidate the device.
void device_close(Device* dev);

// Read the latest input report into `buf`. Drains the queue of stale
// reports (SDL hid_read returns FIFO order on macOS, so without
// draining we'd always read ~1 second behind). Returns number of
// bytes of the most recent report, or 0 if nothing was read, or -1
// on error / disconnect (in which case `dev` is closed).
int device_read_latest(Device* dev, std::uint8_t* buf, std::size_t buf_capacity);

// Write an output report. Returns number of bytes written, or -1 on
// error / disconnect (in which case `dev` is closed).
int device_write(Device* dev, const std::uint8_t* buf, std::size_t len);

} // namespace oc::dualsense
