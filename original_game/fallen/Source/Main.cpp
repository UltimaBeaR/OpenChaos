// Main.cpp
// Guy Simmons, 17th October 1997.

#include	"Game.h"
#ifndef	PSX
#include	"DDLib.h"
#ifndef TARGET_DC
#include	"sedit.h"
#include	"ledit.h"
#endif
#include	"renderstate.h"
#include	"Drive.h"
#endif

#ifdef GUY
#include	"GEdit.h"
#endif

#ifndef	EXIT_SUCCESS

// Not defined on PSX
#define	EXIT_SUCCESS		1
#endif


#include	"Sound.h"
#include	"Memory.h"
#include	"env.h"



/*
#include "finaleng.h"


void mkt_test(void)
{
	if (FINALENG_init())
	{
		FINALENG_initialise_for_new_level();

		while(SHELL_ACTIVE)
		{
			if (Keys[KB_Q])
			{
				break;
			}

			FINALENG_draw();
		}

		FINALENG_fini();
	}
}
*/




//---------------------------------------------------------------

#ifdef	PSX
ULONG	old_stack;

#ifndef VERSION_DEMO
void	main2(void)
{
#ifdef FS_ISO9600
	SetMem(2);
#endif
	old_stack=GetSp();
	SetSp(0x1f8003fc);

	if(SetupHost(H_CREATE_LOG))
	{
		while(1)
		{
			game();
		}
	}

	SetSp(old_stack);
}
#endif

#ifdef VERSION_DEMO
extern SLONG demo_mode,demo_timeout;
SLONG hold_stack[16],*tmp_i,*tmp_s;
SLONG game_timeout;
#endif


SLONG main(UWORD argc, CBYTE *argv[])
{
#ifndef VERSION_DEMO
	main2();

	return	EXIT_SUCCESS;

#else
#ifdef FS_ISO9600
//	SetMem(2);
#endif

	old_stack=GetSp();
	tmp_i=old_stack;
	tmp_s=hold_stack;
	while((SLONG)tmp_i<0x80200000)
	{
		*tmp_s++=*tmp_i++;
	}
	SetSp(0x1f8003f0);

#ifndef FS_ISO9660
	demo_mode=0;
	demo_timeout=60;
	game_timeout=demo_timeout*20;
#else
	demo_mode=((int *)argv)[0];
	demo_timeout=((int *)argv)[1];
	game_timeout=demo_timeout*20;
#endif

	//printf("Mode = %d, Timeout = %d\n",demo_mode,demo_timeout);

	if(SetupHost(H_CREATE_LOG))
	{
		game();
	}

	PSXOverLay(OVERLAY_NAME,OVERLAY_SIZE);

extern void Wadmenu_DemoSplash(void);
extern void	ReleaseHardware(void);

	if (demo_mode==0)
		Wadmenu_DemoSplash();
	ReleaseHardware();

	//printf("End of Demo.\n");
	tmp_s=hold_stack;
	tmp_i=old_stack;
	while((SLONG)tmp_i<0x80200000)
	{
		*tmp_i++=*tmp_s++;
	}
	SetSp(old_stack);
	return 0;
#endif

}

#else


// VerifyDirectX
//
// init DirectDraw and Direct3D and check they're OK

static int numdevices = 0;

static HRESULT CALLBACK D3DEnumDevicesCallback(GUID FAR* lpGuid, 
											   LPTSTR lpDeviceDescription, 
											   LPTSTR lpDeviceName, 
											   LPD3DDEVICEDESC lpD3DHWDeviceDesc, 
											   LPD3DDEVICEDESC lpD3DHELDeviceDesc,
											   LPVOID lpContext)
{
	if (lpD3DHWDeviceDesc->dwFlags)
	{
		TRACE("HARDWARE Device %s \"%s\"\n", lpDeviceDescription, lpDeviceName);
		numdevices++;
	}
	else
	{
		TRACE("SOFTWARE Device %s \"%s\"\n", lpDeviceDescription, lpDeviceName);
		numdevices++;	// count these too
	}
	return D3DENUMRET_OK;
}

extern HINSTANCE hGlobalThisInst;

SLONG main(UWORD argc, TCHAR *argv[])
{
#ifdef TARGET_DC
	// DC doesn't use relative names, only full path names.
#ifdef FILE_PC
	// Serve files from the PC.
	FileSetBasePath ( "\\PC\\Fallen\\" );
#else
	// Serve files from the CD.
	FileSetBasePath ( "\\CD-ROM\\fallen\\" );
#endif

#else
	// Does nothing exciting otherwise.
	FileSetBasePath ( "" );
#endif

#ifdef TARGET_DC
extern void ENV_init ( void );
	ENV_init();
#else
	ENV_load("config.ini");
#endif

	LocateCDROM();

	AENG_read_detail_levels();	// get engine defaults

#if 0
	{
		SLONG i;

		for (i = 0; i < 100; i++)
		{
			TRACE("Random() & 0xff = %d\n", Random() & 0xff);
		}
	}
#endif

#ifndef USE_A3D
//	ASSERT(the_qs_sound_manager.Init() == ERROR_QMIX_OK);
//	the_qs_sound_manager.Init() == ERROR_QMIX_OK;
#endif

#ifdef EDITOR
#ifndef NDEBUG

	int sdown = GetKeyState(VK_SHIFT);
	int ldown = GetKeyState(VK_F12 );

	if (sdown & ~1)
	{
		if(!SetupMemory())
			return	EXIT_FAILURE;
		init_memory();
		SEDIT_do();

		return EXIT_SUCCESS;
	}

	if (0||(ldown & ~1))
	{
		if(!SetupMemory())
			return	EXIT_FAILURE;
		init_memory();
		LEDIT_do();

		return EXIT_SUCCESS;
	}

#endif
#endif

	if(SetupHost(H_CREATE_LOG))
	{
//		mkt_test();

		game();
	}
	ResetHost();

	return	EXIT_SUCCESS;
}
#endif

//---------------------------------------------------------------
