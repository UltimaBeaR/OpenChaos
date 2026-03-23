#include "assets/bink_client.h"

// uc_orig: s32 (fallen/DDLibrary/Source/BinkClient.cpp)
typedef int s32;
// uc_orig: u32 (fallen/DDLibrary/Source/BinkClient.cpp)
typedef unsigned int u32;

// uc_orig: BinkMessage (fallen/DDLibrary/Source/BinkClient.cpp)
void BinkMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Bink library not linked; no-op.
    /*if (!bink)	return;
    switch (message) {
    case WM_SETFOCUS:  BinkPause(bink, 0); focus = 1; break;
    case WM_KILLFOCUS: BinkPause(bink, 1); focus = 0; break;
    }*/
}

// uc_orig: BinkPlay (fallen/DDLibrary/Source/BinkClient.cpp)
void BinkPlay(const char* filename, IDirectDrawSurface* lpdds, bool (*flip)())
{
    // Bink library not linked; video playback not implemented.
    // Full implementation was here but removed because bink.h is not available.
}
