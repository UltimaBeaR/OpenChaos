// DualSense IMU calibration math.

#include <libDualsense/ds_calibration.h>

#include <libDualsense/ds_feature.h>

namespace oc::dualsense {

static float accel_g(std::int16_t raw, std::int16_t plus, std::int16_t minus)
{
    // Bias is the midpoint between +1g and -1g calibration anchors;
    // range is half-span which equals "raw units per 1 g". See header.
    const int32_t range = (static_cast<int32_t>(plus) - static_cast<int32_t>(minus)) / 2;
    if (range == 0) return 0.0f;
    const int32_t bias  = (static_cast<int32_t>(plus) + static_cast<int32_t>(minus)) / 2;
    return static_cast<float>(static_cast<int32_t>(raw) - bias) / static_cast<float>(range);
}

float calibrate_accel_x_g(std::int16_t raw, const SensorCalibration& c) {
    return accel_g(raw, c.accel_x_plus, c.accel_x_minus);
}
float calibrate_accel_y_g(std::int16_t raw, const SensorCalibration& c) {
    return accel_g(raw, c.accel_y_plus, c.accel_y_minus);
}
float calibrate_accel_z_g(std::int16_t raw, const SensorCalibration& c) {
    return accel_g(raw, c.accel_z_plus, c.accel_z_minus);
}

static float gyro_deg_per_sec(std::int16_t raw,
                               std::int16_t bias,
                               std::int16_t plus, std::int16_t minus,
                               std::int16_t speed_plus, std::int16_t speed_minus)
{
    const int32_t range = static_cast<int32_t>(plus) - static_cast<int32_t>(minus);
    if (range == 0) return 0.0f;
    const int32_t speed_2x = static_cast<int32_t>(speed_plus) + static_cast<int32_t>(speed_minus);
    return static_cast<float>(static_cast<int32_t>(raw) - static_cast<int32_t>(bias))
         * static_cast<float>(speed_2x)
         / static_cast<float>(range);
}

float calibrate_gyro_pitch_deg_per_sec(std::int16_t raw, const SensorCalibration& c) {
    return gyro_deg_per_sec(raw, c.gyro_pitch_bias,
                             c.gyro_pitch_plus, c.gyro_pitch_minus,
                             c.gyro_speed_plus, c.gyro_speed_minus);
}
float calibrate_gyro_yaw_deg_per_sec(std::int16_t raw, const SensorCalibration& c) {
    return gyro_deg_per_sec(raw, c.gyro_yaw_bias,
                             c.gyro_yaw_plus, c.gyro_yaw_minus,
                             c.gyro_speed_plus, c.gyro_speed_minus);
}
float calibrate_gyro_roll_deg_per_sec(std::int16_t raw, const SensorCalibration& c) {
    return gyro_deg_per_sec(raw, c.gyro_roll_bias,
                             c.gyro_roll_plus, c.gyro_roll_minus,
                             c.gyro_speed_plus, c.gyro_speed_minus);
}

} // namespace oc::dualsense
