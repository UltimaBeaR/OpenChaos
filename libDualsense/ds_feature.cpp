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

    // Feature report 0x20 is 64 bytes total. Buffer sized generously.
    std::uint8_t buf[96];
    const int n = device_get_feature_report(dev, 0x20, buf, sizeof(buf));

    // Layout (daidr FactoryInfo.vue::getFirmwareInfo):
    //   buf[0]           reportId 0x20
    //   buf[1..11]       buildDate   (11 bytes UTF-8)
    //   buf[12..19]      buildTime   (8 bytes UTF-8)
    //   buf[20..21]      fwType      (u16 LE)
    //   buf[22..23]      swSeries    (u16 LE)
    //   buf[24..27]      hwInfo      (u32 LE)
    //   buf[28..31]      mainFwVersion (u32 LE)
    //   buf[32..43]      deviceInfo  (12 bytes)
    //   buf[44..45]      updateVersion (u16 LE)
    //   buf[46]          updateImageInfo (u8)
    //   buf[48..51]      sblFwVersion (u32 LE)
    //   buf[52..55]      dspFwVersion (u32 LE)
    //   buf[56..59]      spiderDspFwVersion (u32 LE)
    //
    // Required minimum: 60 bytes to cover spiderDspFwVersion.
    const int REQUIRED = 60;
    if (n < REQUIRED) return false;

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

    // Feature report 0x05 is 41 bytes total (36 payload bytes of int16
    // fields after the reportId, + padding for BT CRC which SDL hides).
    std::uint8_t buf[64];
    const int n = device_get_feature_report(dev, 0x05, buf, sizeof(buf));

    // Layout (DualSense 6-axis calibration, matches DS5W/nondebug RE):
    //   buf[0]           reportId 0x05
    //   buf[1..2]        gyro_pitch_bias (int16 LE)
    //   buf[3..4]        gyro_yaw_bias
    //   buf[5..6]        gyro_roll_bias
    //   buf[7..8]        gyro_pitch_plus
    //   buf[9..10]       gyro_pitch_minus
    //   buf[11..12]      gyro_yaw_plus
    //   buf[13..14]      gyro_yaw_minus
    //   buf[15..16]      gyro_roll_plus
    //   buf[17..18]      gyro_roll_minus
    //   buf[19..20]      gyro_speed_plus
    //   buf[21..22]      gyro_speed_minus
    //   buf[23..24]      accel_x_plus
    //   buf[25..26]      accel_x_minus
    //   buf[27..28]      accel_y_plus
    //   buf[29..30]      accel_y_minus
    //   buf[31..32]      accel_z_plus
    //   buf[33..34]      accel_z_minus
    //
    // Required minimum: 35 bytes.
    const int REQUIRED = 35;
    if (n < REQUIRED) return false;

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

    // Feature report 0x22 contains a build number at offset 31..34.
    std::uint8_t buf[64];
    const int n = device_get_feature_report(dev, 0x22, buf, sizeof(buf));
    if (n < 35) return false;
    if (buf[0] != 0x22) return false;

    *out = rd_u32(&buf[31]);
    return true;
}

} // namespace oc::dualsense
