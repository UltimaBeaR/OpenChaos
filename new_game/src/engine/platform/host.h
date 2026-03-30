#ifndef ENGINE_PLATFORM_HOST_H
#define ENGINE_PLATFORM_HOST_H

#include "engine/platform/uc_common.h"   // MFTime

// Application entry point — called from src/main.cpp.
int HOST_run(int argc, char* argv[]);

// uc_orig: SetupHost (fallen/DDLibrary/Source/GHost.cpp)
// Creates the SDL3 window, initialises memory, keyboard, and sound.
// Returns UC_TRUE on success.
BOOL SetupHost(ULONG flags);

// uc_orig: ResetHost (fallen/DDLibrary/Source/GHost.cpp)
// Tears down everything initialised by SetupHost: sound, keyboard, memory,
// and destroys the SDL3 window.
void ResetHost(void);

// uc_orig: ShellPaused (fallen/DDLibrary/Source/GHost.cpp)
void ShellPaused(void);

// uc_orig: ShellPauseOn (fallen/DDLibrary/Source/GHost.cpp)
void ShellPauseOn(void);

// uc_orig: ShellPauseOff (fallen/DDLibrary/Source/GHost.cpp)
void ShellPauseOff(void);

// uc_orig: LibShellActive (fallen/DDLibrary/Source/GHost.cpp)
// Polls the SDL3 event queue. Returns UC_TRUE while the app is alive.
BOOL LibShellActive(void);

// uc_orig: LibShellChanged (fallen/DDLibrary/Source/GHost.cpp)
BOOL LibShellChanged(void);

// uc_orig: LibShellMessage (fallen/DDLibrary/Source/GHost.cpp)
BOOL LibShellMessage(const char* pMessage, const char* pFile, ULONG dwLine);

// uc_orig: Time (fallen/DDLibrary/Source/GHost.cpp)
void Time(MFTime* the_time);

// uc_orig: TraceText (MFStdLib/Source/StdLib/StdFile.cpp)
void TraceText(char* fmt, ...);

#endif // ENGINE_PLATFORM_HOST_H
