#ifndef ENGINE_IO_OC_CONFIG_H
#define ENGINE_IO_OC_CONFIG_H

#include <cfloat> // FLT_MAX
#include <climits> // INT_MIN / INT_MAX

// OpenChaos config system.
// Reads/writes openchaos/config.json (relative to working directory).
// On first run: creates config.json with ALL known options populated from
// hardcoded defaults. On subsequent runs: loads whatever is in the file;
// missing keys silently use hardcoded defaults and are NOT re-added on save.
// All ENV_get_value_number / ENV_set_value_number calls delegate here.
//
// CLAMPING: the read functions take optional [lo, hi] bounds and clamp the
// returned value into that range. This guards against a user hand-editing
// config.json with an out-of-range (but right-type) value — it is trimmed
// to the nearest valid bound instead of breaking the game. For a 0..1
// fraction pass (0.0f, 1.0f); for an unbounded value omit the bounds.

// Load config on startup. ini_path is the legacy config.ini path used for one-time migration.
void OC_CONFIG_load(const char* ini_path);

// Read an integer value, clamped to [lo, hi]. Returns def (clamped) if the key is not present.
int OC_CONFIG_get_int(const char* section, const char* key, int def,
                      int lo = INT_MIN, int hi = INT_MAX);

// Read a float value, clamped to [lo, hi]. Returns def (clamped) if the key is not present.
float OC_CONFIG_get_float(const char* section, const char* key, float def,
                          float lo = -FLT_MAX, float hi = FLT_MAX);

// Write an integer value and immediately flush to disk.
void OC_CONFIG_set_int(const char* section, const char* key, int value);

// Write a float value and immediately flush to disk.
void OC_CONFIG_set_float(const char* section, const char* key, float value);

#endif // ENGINE_IO_OC_CONFIG_H
