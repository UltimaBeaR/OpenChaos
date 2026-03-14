// WindProcs.cpp
// Guy Simmons, 14th November 1997.

#include	"DDLib.h"
//#include	"finaleng.h"
#include	"BinkClient.h"
#include	"..\headers\music.h"
#include	"..\headers\game.h"

extern void MFX_QUICK_stop(void);

SLONG app_inactive;
SLONG restore_surfaces;

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);

LRESULT	CALLBACK	DDLibShellProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int				result;
	SLONG			w,h,bpp,refresh;
	ChangeDDInfo	change_info;
	GUID			*D3D_guid,
					*DD_guid;
	HMENU			hMenu;
	HINSTANCE		hInstance;

#ifndef TARGET_DC
	BinkMessage(hWnd, message, wParam, lParam);
#endif
	
	switch(message)
	{
#ifndef TARGET_DC
		case WM_ACTIVATEAPP:

			if (!wParam)
			{
				//
				// Lost focus...
				//

				app_inactive = TRUE;
			}
			else
			{
				app_inactive     = FALSE;
				restore_surfaces = TRUE;
			}

			break;

	    case WM_SIZE:
	    case WM_MOVE:

			//
			// Tell the new engine the that window has moved.
			//

//			FINALENG_window_moved(LOWORD(lParam),HIWORD(lParam));

	        if(the_display.IsFullScreen())
	        {
				SetRect(&the_display.DisplayRect,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
	        }
			else
			{
	            GetClientRect(hWnd,&the_display.DisplayRect);
	            ClientToScreen(hWnd,(LPPOINT)&the_display.DisplayRect);
	            ClientToScreen(hWnd,(LPPOINT)&the_display.DisplayRect+1);
			}
			break;
		case	WM_MOUSEMOVE:
		case	WM_RBUTTONUP:
		case	WM_RBUTTONDOWN:
		case	WM_RBUTTONDBLCLK:
		case	WM_LBUTTONUP:
		case	WM_LBUTTONDOWN:
		case	WM_LBUTTONDBLCLK:
		case	WM_MBUTTONUP:
		case	WM_MBUTTONDOWN:
		case	WM_MBUTTONDBLCLK:
			MouseProc(message,wParam,lParam);
			break;
#endif //#ifndef TARGET_DC

		case	WM_KEYDOWN:
		case	WM_KEYUP:
			KeyboardProc(message,wParam,lParam);
			break;
		case	WM_CLOSE:
			// normally, we should call DestroyWindow().
			// instead, let's set flags so the normal quit process goes thru.
			GAME_STATE=0;
			break;
		case	WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return	DefWindowProc(hWnd,message,wParam,lParam);
	}
	return	0;
}
