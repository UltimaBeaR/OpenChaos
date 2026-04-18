#pragma once

// Apply DualSense factory IMU calibration to raw gyro/accel samples
// from InputState.
//
// The calibration data (from feature report 0x05, see ds_feature.h)
// provides per-axis bias and full-range reference points. These
// helpers convert raw int16 samples into physical units:
//
//   Accelerometer → g (gravitational units, 1.0 = 1g)
//   Gyroscope     → deg/s
//
// Without calibration, raw samples are usable only for relative motion
// and gestures. With calibration, they can be integrated for attitude,
// motion-aimed aiming, etc.
//
// All calibration is per-device — every controller has slightly
// different bias and scaling, so one set of constants is not
// substitutable for another.

#include <cstdint>

namespace oc::dualsense {

struct SensorCalibration;  // fwd decl from ds_feature.h

// ---- Accelerometer --------------------------------------------------

// Convert raw accelerometer X sample to g. The math is well-defined
// from Sony's calibration scheme:
//   bias  = (X_plus + X_minus) / 2   (raw value at 0 g)
//   range = (X_plus - X_minus) / 2   (raw value at 1 g, half-range)
//   g     = (raw - bias) / range
//
// Returns 0.0f when calibration range is zero (uncalibrated device).
float calibrate_accel_x_g(std::int16_t raw, const SensorCalibration& c);
float calibrate_accel_y_g(std::int16_t raw, const SensorCalibration& c);
float calibrate_accel_z_g(std::int16_t raw, const SensorCalibration& c);

// ---- Gyroscope ------------------------------------------------------

// Convert raw gyro sample to deg/s.
//
// Formula (matches the Linux hid-playstation kernel driver):
//   speed_2x = gyro_speed_plus + gyro_speed_minus
//   range    = axis_plus - axis_minus
//   deg/s    = (raw - bias) * speed_2x / range
//
// The calibration constants vary per device and per firmware revision;
// the formula above is community-verified across DS4/DS5 reverse
// engineering efforts (DS5W, nondebug, hid-playstation, DS4Windows).
//
// Returns 0.0f when the axis range is zero (uncalibrated device).
float calibrate_gyro_pitch_deg_per_sec(std::int16_t raw, const SensorCalibration& c);
float calibrate_gyro_yaw_deg_per_sec  (std::int16_t raw, const SensorCalibration& c);
float calibrate_gyro_roll_deg_per_sec (std::int16_t raw, const SensorCalibration& c);

} // namespace oc::dualsense
