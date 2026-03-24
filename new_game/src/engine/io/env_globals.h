#ifndef ENGINE_IO_ENV_GLOBALS_H
#define ENGINE_IO_ENV_GLOBALS_H

#include <windows.h>
#include "engine/core/types.h"

// Full path to the resolved INI file, built from the working directory in ENV_load().
// uc_orig: inifile (fallen/Source/env2.cpp)
extern CBYTE env_inifile[_MAX_PATH];

// Scratch buffer used for GetPrivateProfileString results and sprintf formatting.
// uc_orig: strbuf (fallen/Source/env2.cpp)
extern CBYTE env_strbuf[_MAX_PATH];

#endif // ENGINE_IO_ENV_GLOBALS_H
