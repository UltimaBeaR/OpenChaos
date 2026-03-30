#ifndef ENGINE_IO_ENV_GLOBALS_H
#define ENGINE_IO_ENV_GLOBALS_H

#include "engine/core/types.h"

#define ENV_MAX_PATH 260

// Full path to the resolved INI file, built from the working directory in ENV_load().
// uc_orig: inifile (fallen/Source/env2.cpp)
extern CBYTE env_inifile[ENV_MAX_PATH];

// Scratch buffer used for INI read results and sprintf formatting.
// uc_orig: strbuf (fallen/Source/env2.cpp)
extern CBYTE env_strbuf[ENV_MAX_PATH];

#endif // ENGINE_IO_ENV_GLOBALS_H
