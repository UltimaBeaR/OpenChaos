// BinkClient.cpp
//
// simple client for BINK file playback
//
// DO NOT PUT ANY OF YOUR USUAL CRAP INTO THIS FILE
// If you want to change anything, do it in do_bink_intro() or bink_flipper() in GDisplay.cpp

#include "DDLib.h"
// #include "bink.h"
#include "drive.h"

typedef int s32;
typedef unsigned int u32;

// static s32		focus = 1;		// we have focus
// static HBINK	bink = NULL;	// a bink file is loaded

// BinkMessage
//
// handle bink windows messages

void BinkMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    /*if (!bink)	return;

    switch (message)
    {
    case WM_SETFOCUS:
            BinkPause(bink, 0);
            focus = 1;
            break;

    case WM_KILLFOCUS:
            BinkPause(bink, 1);
            focus = 0;
            break;
    }*/
}

// BinkPlay
//
// play a bink video using the back surface and the flip function specified

void BinkPlay(const char* filename, IDirectDrawSurface* lpdds, bool (*flip)())
{
    //	// obtain surface info
    //	DDSURFACEDESC	DDSD;
    //
    //	memset(&DDSD, 0, sizeof(DDSD));
    //	DDSD.dwSize = sizeof(DDSD);
    //
    //	if (FAILED(lpdds->GetSurfaceDesc(&DDSD)))
    //	{
    //		MessageBox(0, "Can't playback the movie ... bad surface", "Error", MB_OK);
    //		return;
    //	}
    //
    //	s32 stype = BinkDDSurfaceType(lpdds);
    //
    //	if ((stype == -1) || (stype == BINKSURFACE8P))
    //	{
    //		MessageBox(0, "Can't playback the movie ... bad video colour depth", "Error", MB_OK);
    //		return;
    //	}
    //
    //	// set sound driver
    // #ifdef M_SOUND
    //	if (GetMilesDriver())
    //	{
    //		BinkSoundUseMiles(GetMilesDriver());
    //	}
    // #endif
    //
    //	// open the bink file & check it's size
    //	char	fullname[MAX_PATH];
    //	strcpy(fullname, GetMoviesPath());
    //	strcat(fullname, filename);
    //	bink = BinkOpen(fullname, 0);
    //	if (!bink)	return;
    //
    //	if ((bink->StretchWidth > DDSD.dwWidth) || (bink->StretchHeight > DDSD.dwHeight))
    //	{
    //		MessageBox(0, "Can't playback the movie ... bad video resolution", "Error", MB_OK);
    //		BinkClose(bink);
    //		bink = NULL;
    //		return;
    //	}
    //
    //	u32	xoff = (DDSD.dwWidth - bink->StretchWidth) / 2;
    //	u32	yoff = (DDSD.dwHeight - bink->StretchHeight) / 2;
    //
    //	Keys[KB_SPACE] = 0;
    //	Keys[KB_ENTER] = 0;
    //	Keys[KB_ESC]   = 0;
    //
    //
    //	// play each frame
    ////	for (;;)
    //	while(SHELL_ACTIVE)
    //	{
    //		MSG	msg;
    //
    //		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    //		{
    //			if (msg.message == WM_QUIT)	break;
    //			TranslateMessage(&msg);
    //			DispatchMessage(&msg);
    //		}
    //		else
    //		{
    //			if (!BinkWait(bink))
    //			{
    //				if (!NextBinkFrame(lpdds, &DDSD, stype, xoff, yoff, flip))	break;
    //			}
    //
    //			if (!focus)	Sleep(1);	// lost focus
    //		}
    //
    //		if (Keys[KB_SPACE] ||
    //			Keys[KB_ENTER] ||
    //			Keys[KB_ESC])
    //		{
    //			Keys[KB_SPACE] = 0;
    //			Keys[KB_ENTER] = 0;
    //			Keys[KB_ESC]   = 0;
    //
    //			break;
    //		}
    //	}
    //
    //	// clear the buffer
    //	// (this is done to avoid coloured crap on the mode change)
    //	EndMovie(lpdds, &DDSD, flip);
    //
    //	BinkClose(bink);
    //	bink = NULL;
}
