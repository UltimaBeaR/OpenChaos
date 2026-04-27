#ifndef ENGINE_IO_ENV_H
#define ENGINE_IO_ENV_H

#include "engine/core/types.h"

// Loads the INI configuration file. Path is relative to the working directory.
// Must be called once at startup before any ENV_get_* calls.
// uc_orig: ENV_load (fallen/Headers/env.h)
void ENV_load(CBYTE* fname);

// Returns a numeric value from the INI file, or 'def' if not found.
// As a side effect, also writes the value back to the INI file
// (unless the section is "Secret").
// uc_orig: ENV_get_value_number (fallen/Headers/env.h)
SLONG ENV_get_value_number(CBYTE* name, SLONG def, CBYTE* section = "Game");

// Writes a numeric value to the INI file.
// uc_orig: ENV_set_value_number (fallen/Headers/env.h)
void ENV_set_value_number(CBYTE* name, SLONG value, CBYTE* section = "Game");

// Generic INI file access (cross-platform replacement for Win32 GetPrivateProfile*).
// These operate on arbitrary INI files, not just config.ini.
int INI_get_int(const char* filepath, const char* section, const char* key, int def);
bool INI_get_string(const char* filepath, const char* section, const char* key, char* out, int out_size);
bool INI_get_section(const char* filepath, const char* section, char* out, int out_size);

#endif // ENGINE_IO_ENV_H
