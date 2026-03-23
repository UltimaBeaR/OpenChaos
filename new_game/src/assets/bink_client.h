#ifndef ASSETS_BINK_CLIENT_H
#define ASSETS_BINK_CLIENT_H

#include <windows.h>

struct IDirectDrawSurface;

// Bink video playback stubs.
// BinkPlay is not implemented; BinkMessage handles window messages for an active Bink session.

// uc_orig: BinkMessage (fallen/DDLibrary/Source/BinkClient.cpp)
void BinkMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// uc_orig: BinkPlay (fallen/DDLibrary/Source/BinkClient.cpp)
void BinkPlay(const char* filename, IDirectDrawSurface* lpdds, bool (*flip)());

#endif // ASSETS_BINK_CLIENT_H
