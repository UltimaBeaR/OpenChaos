#ifndef ENGINE_GRAPHICS_GRAPHICS_API_HOST_H
#define ENGINE_GRAPHICS_GRAPHICS_API_HOST_H

#include "engine/platform/uc_common.h"   // MFTime
#include <windows.h>

// Real WinMain implementation — called from src/main.cpp.
int HOST_run(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPTSTR lpszArgs, int iWinMode);

// uc_orig: SetupHost (fallen/DDLibrary/Source/GHost.cpp)
// Registers the window class, creates the main window, initialises memory and keyboard,
// and starts the sound manager. Returns UC_TRUE on success.
BOOL SetupHost(ULONG flags);

// uc_orig: ResetHost (fallen/DDLibrary/Source/GHost.cpp)
// Tears down everything initialised by SetupHost: sound, keyboard, memory, and
// unregisters the window class.
void ResetHost(void);

// uc_orig: ShellPaused (fallen/DDLibrary/Source/GHost.cpp)
// Called from the game thread to wait while a pause has been requested by the shell thread.
void ShellPaused(void);

// uc_orig: ShellPauseOn (fallen/DDLibrary/Source/GHost.cpp)
// Requests that the game thread pause and waits for acknowledgement, then switches the
// display to GDI mode. Nesting is supported via PauseCount.
void ShellPauseOn(void);

// uc_orig: ShellPauseOff (fallen/DDLibrary/Source/GHost.cpp)
// Resumes the game thread after a ShellPauseOn. Switches the display back from GDI mode.
void ShellPauseOff(void);

// uc_orig: LibShellActive (fallen/DDLibrary/Source/GHost.cpp)
// Pumps the Windows message queue and yields CPU while the app is inactive and in
// fullscreen. Also restores DirectDraw surfaces when restore_surfaces is set.
// Returns UC_TRUE while the shell window is alive.
BOOL LibShellActive(void);

// uc_orig: LibShellChanged (fallen/DDLibrary/Source/GHost.cpp)
// Returns UC_TRUE (and clears the flag) if the display settings changed since the last call.
BOOL LibShellChanged(void);

// uc_orig: LibShellMessage (fallen/DDLibrary/Source/GHost.cpp)
// Displays an Abort/Retry/Ignore message box for developer error reporting.
// Abort exits the process, Retry triggers a debug break, Ignore continues.
BOOL LibShellMessage(const char* pMessage, const char* pFile, ULONG dwLine);

// uc_orig: Time (fallen/DDLibrary/Source/GHost.cpp)
// Fills *the_time with the current local wall-clock time via GetLocalTime / GetTickCount.
void Time(MFTime* the_time);

// uc_orig: TraceText (MFStdLib/Source/StdLib/StdFile.cpp)
// Printf-style debug trace: formats a message and sends it to the debugger via
// OutputDebugString. Defined as the TRACE macro in release builds.
void TraceText(char* fmt, ...);

#endif // ENGINE_GRAPHICS_GRAPHICS_API_HOST_H
