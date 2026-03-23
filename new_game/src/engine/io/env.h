#ifndef ENGINE_IO_ENV_H
#define ENGINE_IO_ENV_H

#include "core/types.h"

// Loads the INI configuration file. Path is relative to the working directory.
// Must be called once at startup before any ENV_get_* calls.
// uc_orig: ENV_load (fallen/Headers/env.h)
void ENV_load(CBYTE* fname);

// Returns a string value from the INI file, or NULL if not found.
// Returned pointer is into a static buffer — copy if you need to keep it.
// uc_orig: ENV_get_value_string (fallen/Headers/env.h)
CBYTE* ENV_get_value_string(CBYTE* name, CBYTE* section = "Game");

// Returns a numeric value from the INI file, or 'def' if not found.
// As a side effect, also writes the value back to the INI file
// (unless the section is "Secret").
// uc_orig: ENV_get_value_number (fallen/Headers/env.h)
SLONG ENV_get_value_number(CBYTE* name, SLONG def, CBYTE* section = "Game");

// Writes a string value to the INI file.
// uc_orig: ENV_set_value_string (fallen/Headers/env.h)
void ENV_set_value_string(CBYTE* name, CBYTE* value, CBYTE* section = "Game");

// Writes a numeric value to the INI file.
// uc_orig: ENV_set_value_number (fallen/Headers/env.h)
void ENV_set_value_number(CBYTE* name, SLONG value, CBYTE* section = "Game");

#endif // ENGINE_IO_ENV_H
