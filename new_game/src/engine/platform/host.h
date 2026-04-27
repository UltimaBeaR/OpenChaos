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
// called once per LibShellActive iteration, but secondary loops that
// pump SDL events themselves (notably the video player, which uses raw
// SDL_PollEvent and therefore bypasses LibShellActive) need to call this
// too — otherwise resize events queued during their loop would never
// drive ge_resize_display, and the scene FBO would stay at the pre-loop
// size until the next LibShellActive iteration. Safe no-op when nothing
// is pending.
void host_process_pending_resize(void);

#endif // ENGINE_PLATFORM_HOST_H
