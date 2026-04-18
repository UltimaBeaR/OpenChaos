#pragma once

// DualSense feature reports: firmware info, sensor calibration, BT
// patch version. All operations are read-only.
//
// Feature reports are read via `device_get_feature_report` from
// ds_device. The parsing layouts and field semantics below are derived
// from the daidr/dualsense-tester reference implementation (MIT) and
// from well-known DualSense reverse-engineering projects (DS5W,
// nondebug).
//
// All parsers are **tolerant** — short or partial reports return
// zeroed / empty fields without crashing. Callers check the return
// value of the `get_*` helpers to detect "controller does not support
// this feature report".

#include <cstddef>
#include <cstdint>

namespace oc::dualsense {

struct Device;  // fwd decl from ds_device.h

// ---- Firmware info (feature report 0x20) ----------------------------
//
// 64-byte report containing build date/time, firmware type, software
// series, hardware info, and a handful of subsystem firmware versions.
// Available on all production DualSense firmwares.
struct FirmwareInfo {
    // UTF-8 ASCII strings, null-terminated at the size limit.
    char buildDate[12];     // e.g. "May 20 2022"  (11 bytes + NUL)
    char buildTime[9];      // e.g. "15:22:54"     (8 bytes + NUL)

    std::uint16_t fwType;             // firmware type code
    std::uint16_t swSeries;           // SW series
    std::uint32_t hwInfo;             // hardware info (board version in low 16 bits)
    std::uint32_t mainFwVersion;      // main application firmware (major<<24 | minor<<16 | patch)
    std::uint8_t  deviceInfo[12];     // opaque device identifier bytes
    std::uint16_t updateVersion;
    std::uint8_t  updateImageInfo;
    std::uint32_t sblFwVersion;       // secondary bootloader
    std::uint32_t dspFwVersion;       // DSP firmware (audio)
    std::uint32_t spiderDspFwVersion; // MCU/DSP combined firmware

    bool valid;  // false if the report was too short to parse
};

// Retrieve the firmware info report. Returns true on success.
bool get_firmware_info(Device* dev, FirmwareInfo* out);

// ---- Sensor calibration (feature report 0x05) -----------------------
//
// 41-byte report with gyroscope and accelerometer factory calibration
// data. Apply via ds_calibration.h (calibrate_gyro / calibrate_accel)
// to convert raw InputState values into physical units.
//
// Layout is the DualSense 6-axis IMU calibration (little-endian int16
// per field), the same shape used since the DualShock 4 (DS5W/nondebug
// layouts agree).
struct SensorCalibration {
    std::int16_t gyro_pitch_bias;   // raw offset at rest, subtract from samples
    std::int16_t gyro_yaw_bias;
    std::int16_t gyro_roll_bias;

    std::int16_t gyro_pitch_plus;   // +calibration point (raw units at +speed_ref deg/s)
    std::int16_t gyro_pitch_minus;  // -calibration point (raw units at -speed_ref deg/s)
    std::int16_t gyro_yaw_plus;
    std::int16_t gyro_yaw_minus;
    std::int16_t gyro_roll_plus;
    std::int16_t gyro_roll_minus;

    std::int16_t gyro_speed_plus;   // reference speed in scaled units (numerator)
    std::int16_t gyro_speed_minus;  // (divisor for conversion to deg/s)

    std::int16_t accel_x_plus;      // raw units at +1g
    std::int16_t accel_x_minus;     // raw units at -1g
    std::int16_t accel_y_plus;
    std::int16_t accel_y_minus;
    std::int16_t accel_z_plus;
    std::int16_t accel_z_minus;

    bool valid;  // false if the report was too short to parse
};

// Retrieve the sensor calibration report. Returns true on success.
bool get_sensor_calibration(Device* dev, SensorCalibration* out);

// ---- BT patch info (feature report 0x22) ----------------------------
//
// 32-bit build number of the Bluetooth firmware patch. Informational
// only — used by daidr as a diagnostic. Available on firmware revisions
// where `hwInfo lower 16 bits >= 777`.
//
// Returns true on success and writes the version to `*out`.
bool get_bt_patch_version(Device* dev, std::uint32_t* out);

} // namespace oc::dualsense
