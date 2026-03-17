// BinkClient.h
//
// simple client for BINK file playback

#ifndef FALLEN_DDLIBRARY_HEADERS_BINKCLIENT_H
#define FALLEN_DDLIBRARY_HEADERS_BINKCLIENT_H

extern void BinkPlay(const char* filename, IDirectDrawSurface* lpdds, bool (*flip)());

extern void BinkMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif // FALLEN_DDLIBRARY_HEADERS_BINKCLIENT_H
