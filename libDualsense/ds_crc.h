#pragma once

// CRC32 for DualSense Bluetooth reports.
//
// DualSense BT reports are protected by a CRC32 in the last 4 bytes of
// the report payload. The CRC is computed over `[prefix_byte, reportId,
// data...]` using standard CRC32 (poly 0xEDB88320, init 0xFFFFFFFF,
// final XOR 0xFFFFFFFF). The prefix byte depends on report direction:
//
//   0xA2  output reports   (host → controller)
//   0x53  feature reports  (bidirectional)
//
// Input reports over BT also carry a CRC but libDualsense doesn't
// verify it — the HID layer has its own integrity checks and the cost
// of a mismatched report is only one dropped frame.

#include <cstddef>
#include <cstdint>

namespace oc::dualsense {

// Compute CRC32 for a DualSense BT **output** report.
//
// Expects `buffer[0] == reportId (0x31)` and the caller to have
// pre-prepended nothing else — the 0xA2 prefix byte is baked into
// the seed/table so the loop processes `buffer[0..len)` directly.
// Used by build_output_report().
std::uint32_t crc32_compute(const std::uint8_t* buffer, std::size_t len);

// Compute CRC32 for a DualSense BT **feature** report.
//
// Prepends [0x53, report_id] to the checksum computation, then runs
// over `data[0..len)`. Caller writes the returned CRC into the
// last 4 bytes of the feature report buffer (little-endian) before
// sending.
std::uint32_t crc32_compute_feature_report(std::uint8_t report_id,
                                           const std::uint8_t* data,
                                           std::size_t len);

} // namespace oc::dualsense
