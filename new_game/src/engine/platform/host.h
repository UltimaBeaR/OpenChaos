#ifndef ENGINE_PLATFORM_HOST_H
#define ENGINE_PLATFORM_HOST_H

#include "engine/platform/uc_common.h"

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

// uc_orig: LibShellActive (fallen/DDLibrary/Source/GHost.cpp)
// Polls the SDL3 event queue. Returns UC_TRUE while the app is alive.
BOOL LibShellActive(void);

// Apply any window-resize event accumulated since the last call. Normally
// called once per LibShellActive iteration. Secondary loops that pump
// SDL events themselves (notably the video player) call this once per
// iteration too — otherwise resize events queued during their loop
// would never drive ge_resize_display.
void host_process_pending_resize(void);

// SDL3 focus-event callback target. Updates app_inactive and the OS
// cursor visibility (the latter only in fullscreen — windowed mode
// lets the cursor stay visible at the desktop level). Mouse capture
// release on focus loss is handled separately via mouse_capture_update
// reading app_inactive.
void host_on_focus_changed(bool focused);

#endif // ENGINE_PLATFORM_HOST_H
