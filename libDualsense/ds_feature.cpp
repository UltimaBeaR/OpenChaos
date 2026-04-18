// DualSense feature reports: firmware info, sensor calibration, BT
// patch info. Read-only parsers.

#include <libDualsense/ds_feature.h>

#include <libDualsense/ds_device.h>

#include <cstring>

namespace oc::dualsense {

// Helper: little-endian multi-byte reads from a byte buffer.
static inline std::uint16_t rd_u16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}
static inline std::uint32_t rd_u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]
        | (static_cast<std::uint32_t>(p[1]) << 8)
        | (static_cast<std::uint32_t>(p[2]) << 16)
        | (static_cast<std::uint32_t>(p[3]) << 24));
}
static inline std::int16_t rd_i16(const std::uint8_t* p) {
    return static_cast<std::int16_t>(p[0] | (p[1] << 8));
}

bool get_firmware_info(Device* dev, FirmwareInfo* out)
{
    if (!dev || !out) return false;
    *out = FirmwareInfo{};

    // Feature report 0x20: descriptor-defined 64 bytes (reportId + 63
    // data bytes). Layout and field offsets are taken directly from
    // daidr's FactoryInfo.vue::getFirmwareInfo.
    std::uint8_t buf[64] = {};
    if (device_get_feature_report(dev, 0x20, buf, sizeof(buf)) <= 0) return false;

    std::memcpy(out->buildDate, &buf[1], 11);
    out->buildDate[11] = '\0';
    std::memcpy(out->buildTime, &buf[12], 8);
    out->buildTime[8] = '\0';
    out->fwType             = rd_u16(&buf[20]);
    out->swSeries           = rd_u16(&buf[22]);
    out->hwInfo             = rd_u32(&buf[24]);
    out->mainFwVersion      = rd_u32(&buf[28]);
    std::memcpy(out->deviceInfo, &buf[32], 12);
    out->updateVersion      = rd_u16(&buf[44]);
    out->updateImageInfo    = buf[46];
    out->sblFwVersion       = rd_u32(&buf[48]);
    out->dspFwVersion       = rd_u32(&buf[52]);
    out->spiderDspFwVersion = rd_u32(&buf[56]);
    out->valid              = true;
    return true;
}

bool get_sensor_calibration(Device* dev, SensorCalibration* out)
{
    if (!dev || !out) return false;
    *out = SensorCalibration{};

    // Feature report 0x05: descriptor-defined 64 bytes (reportId + 63
    // data). 6-axis IMU calibration at offsets 1..34 (int16 LE pairs).
    std::uint8_t buf[64] = {};
    if (device_get_feature_report(dev, 0x05, buf, sizeof(buf)) <= 0) return false;

    out->gyro_pitch_bias  = rd_i16(&buf[1]);
    out->gyro_yaw_bias    = rd_i16(&buf[3]);
    out->gyro_roll_bias   = rd_i16(&buf[5]);
    out->gyro_pitch_plus  = rd_i16(&buf[7]);
    out->gyro_pitch_minus = rd_i16(&buf[9]);
    out->gyro_yaw_plus    = rd_i16(&buf[11]);
    out->gyro_yaw_minus   = rd_i16(&buf[13]);
    out->gyro_roll_plus   = rd_i16(&buf[15]);
    out->gyro_roll_minus  = rd_i16(&buf[17]);
    out->gyro_speed_plus  = rd_i16(&buf[19]);
    out->gyro_speed_minus = rd_i16(&buf[21]);
    out->accel_x_plus     = rd_i16(&buf[23]);
    out->accel_x_minus    = rd_i16(&buf[25]);
    out->accel_y_plus     = rd_i16(&buf[27]);
    out->accel_y_minus    = rd_i16(&buf[29]);
    out->accel_z_plus     = rd_i16(&buf[31]);
    out->accel_z_minus    = rd_i16(&buf[33]);
    out->valid            = true;
    return true;
}

bool get_bt_patch_version(Device* dev, std::uint32_t* out)
{
    if (!dev || !out) return false;
    *out = 0;

    // Feature report 0x22: descriptor-defined 64 bytes (reportId + 63
    // data). Build number lives at offset 31..34.
    std::uint8_t buf[64] = {};
    if (device_get_feature_report(dev, 0x22, buf, sizeof(buf)) <= 0) return false;

    *out = rd_u32(&buf[31]);
    return true;
}

} // namespace oc::dualsense
