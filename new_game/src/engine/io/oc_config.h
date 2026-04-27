#ifndef ENGINE_IO_OC_CONFIG_H
#define ENGINE_IO_OC_CONFIG_H

// OpenChaos config system.
// Reads/writes openchaos/config.json (relative to working directory).
// On first run: creates config.json with ALL known options populated from
// config.ini (if present) + hardcoded defaults. On subsequent runs: loads
// whatever is in the file; missing keys silently use hardcoded defaults and
// are NOT re-added on save.
// All ENV_get_value_number / ENV_set_value_number calls delegate here.

// Load config on startup. ini_path is the legacy config.ini path used for one-time migration.
void OC_CONFIG_load(const char* ini_path);

// Read an integer value. Returns def if the key is not present.
int OC_CONFIG_get_int(const char* section, const char* key, int def);

// Read a float value. Returns def if the key is not present.
float OC_CONFIG_get_float(const char* section, const char* key, float def);

// Write an integer value and immediately flush to disk.
void OC_CONFIG_set_int(const char* section, const char* key, int value);

#endif // ENGINE_IO_OC_CONFIG_H
