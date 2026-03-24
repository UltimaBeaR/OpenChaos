#include <windows.h>
#include "engine/console/console_globals.h"

// uc_orig: CONSOLE_mess (fallen/DDEngine/Source/console.cpp)
CONSOLE_Mess CONSOLE_mess[CONSOLE_MAX_MESSES];

// uc_orig: Data (fallen/DDEngine/Source/console.cpp)
ConsoleText console_Data[CONSOLE_LINES];

// uc_orig: status_text (fallen/DDEngine/Source/console.cpp)
CBYTE console_status_text[_MAX_PATH];

// uc_orig: last_tick (fallen/DDEngine/Source/console.cpp)
// claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
DWORD console_last_tick;
// uc_orig: this_tick (fallen/DDEngine/Source/console.cpp)
// claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
DWORD console_this_tick;
