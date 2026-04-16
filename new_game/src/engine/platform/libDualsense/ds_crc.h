#pragma once

// CRC32 for DualSense Bluetooth output reports.
// Standard CRC32 table; the DualSense BT report is prefixed with a
// fixed seed byte (0xA2) before running CRC over the rest of the
// report, and the 4-byte result is appended in little-endian.

#include <cstddef>
#include <cstdint>

namespace oc::dualsense {

// Compute CRC32 over `buffer[0..len)` using the DualSense seed.
// For BT output reports, prepend 0xA2 to the buffer before computing
// (or supply buf pointing at a buffer whose first byte is 0xA2).
std::uint32_t crc32_compute(const std::uint8_t* buffer, std::size_t len);

} // namespace oc::dualsense
